#!/bin/bash
# extract_romfs_hactool.sh — 从基础版 + 更新包提取 v2228224(= 3.3.94012)的完整 RomFS。
#
# 这是本机验证过的可靠路径(2026-10):自研 BKTR 解密逆到最后一处计数器差异未收口,
# hactool 1.4.0 的 --baseromfs 路径可以直接解析 BKTR,但它有两个坑,本脚本都绕开了:
#   1) hactool 只有一个 --titlekey 槽位(加密态),基座与更新各需各的 → 分两次调用,
#      基座先整体导出成已解密的 romfs 镜像,再用 --baseromfs 喂回去
#   2) hactool 把重定位的基座物理偏移(节级)直接当镜像文件偏移用,而导出的镜像只有
#      数据层 → 在镜像前面垫 0xF70000(94012 基础节的数据层起点)个零字节对齐
#
# 验证方式:与自研提取器产出的基础版(已对 NCA 内 IVFC L4 哈希层逐块认证)做块级
# 比对,基座覆盖区 100% 一致;纯补丁文件(equivalent_manifest.psv 等)为可读文本且
# config-key 与 build_key.txt 吻合。
#
# 用法:
#   tools/extract_romfs_hactool.sh <基础.nsp> <更新.nsp> <prod.keys> <输出目录>
# 环境:HACTOOL=hactool 路径(缺省 PATH);NSZ_PY=nsz 便携包 python(抽 NCA/读 ticket)
# 磁盘:中间文件约 26 GB(做完自动删)。产物:<输出目录>/data/ 共 46 文件 8.36 GiB。
# 密钥与游戏包只在本机读取。
set -eu
[ $# -eq 4 ] || { sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'; exit 2; }
BASE_NSP=$1; UPD_NSP=$2; KEYS=$3; OUT=$4
HT=${HACTOOL:-hactool}
NSZ_PY=${NSZ_PY:-D:/Tools/nsz_v4.6.1_win64_portable/python.exe}
command -v "$HT" >/dev/null 2>&1 || { echo "hactool 不在 PATH,设 HACTOOL=<路径>" >&2; exit 1; }
[ -f "$BASE_NSP" ] && [ -f "$UPD_NSP" ] && [ -f "$KEYS" ] || { echo "输入文件不存在" >&2; exit 1; }
[ ! -e "$OUT" ] || { echo "已有 $OUT,不覆盖" >&2; exit 1; }

T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
mkdir -p "$T/base_nca" "$T/upd_nca"

# 1) 从两个 NSP 里抽出 Program NCA 与加密态 titlekey(ticket 字段,读它不需要密钥)
"$NSZ_PY" - "$BASE_NSP" "$UPD_NSP" <<'EOF' > "$T/keys.txt"
import sys
from pathlib import Path
sys.path.insert(0, r"D:\Tools\nsz_v4.6.1_win64_portable\Lib\site-packages")
from nsz import Fs
from nsz.Fs import Ticket

def ent_name(e): return getattr(e, '_path', '') or ''
def s(x): return (x if isinstance(x, str) else x.hex()).upper()
for label, p in [('base', sys.argv[1]), ('upd', sys.argv[2])]:
    c = Fs.factory(Path(p)); c.open(p, 'rb')
    for ent in c:
        if ent_name(ent).lower().endswith('.tik'):
            tik = Ticket.Ticket(); tik.open(ent)
            print(label, s(tik.getTitleKey()))
EOF
read -r _ BASE_TK < <(grep '^base' "$T/keys.txt")
read -r _ UPD_TK < <(grep '^upd' "$T/keys.txt")
BASE_ID=$("$NSZ_PY" - "$BASE_NSP" <<'EOF'
import sys
from pathlib import Path
sys.path.insert(0, r"D:\Tools\nsz_v4.6.1_win64_portable\Lib\site-packages")
from nsz import Fs
from nsz.Fs import Nca
def ent_name(e): return getattr(e, '_path', '') or ''
c = Fs.factory(Path(sys.argv[1])); c.open(sys.argv[1], 'rb')
for ent in c:
    n = ent_name(ent).lower()
    if not n.endswith('.nca') or n.endswith('.cnmt.nca'):
        continue
    nca = Nca.Nca()
    try:
        nca.open(ent)
    except Exception:
        continue
    if str(nca.header.contentType) == 'Content.PROGRAM':
        print(ent_name(ent).split('/')[-1]); break
EOF
)
UPD_ID=$("$NSZ_PY" - "$UPD_NSP" <<'EOF'
import sys
from pathlib import Path
sys.path.insert(0, r"D:\Tools\nsz_v4.6.1_win64_portable\Lib\site-packages")
from nsz import Fs
from nsz.Fs import Nca
def ent_name(e): return getattr(e, '_path', '') or ''
c = Fs.factory(Path(sys.argv[1])); c.open(sys.argv[1], 'rb')
for ent in c:
    n = ent_name(ent).lower()
    if not n.endswith('.nca') or n.endswith('.cnmt.nca'):
        continue
    nca = Nca.Nca()
    try:
        nca.open(ent)
    except Exception:
        continue
    if str(nca.header.contentType) == 'Content.PROGRAM':
        print(ent_name(ent).split('/')[-1]); break
EOF
)
echo "base NCA=$BASE_ID  upd NCA=$UPD_ID"

# 2) 抽出两个 Program NCA(nsz -x 按文件名正则)
NSZ_DIR=$(dirname "$NSZ_PY")
"$NSZ_DIR/nsz.exe" -x -o "$T/base_nca" --extractregex "^.*${BASE_ID%.nca}\.nca$" "$BASE_NSP" >/dev/null 2>&1
"$NSZ_DIR/nsz.exe" -x -o "$T/upd_nca" --extractregex "^.*${UPD_ID%.nca}\.nca$" "$UPD_NSP" >/dev/null 2>&1
BASE_NCA=$(find "$T/base_nca" -name '*.nca' | head -1)
UPD_NCA=$(find "$T/upd_nca" -name '*.nca' | head -1)
[ -n "$BASE_NCA" ] && [ -n "$UPD_NCA" ] || { echo "NCA 抽取失败" >&2; exit 1; }

# 3) 基座整体导出成已解密 romfs 镜像,并垫 0xF70000 零对齐到节级
echo "基座 romfs 镜像导出(约 8 GB)..."
"$HT" -k "$KEYS" --disablekeywarns --titlekey "$BASE_TK" --romfs "$T/base.img" "$BASE_NCA"
dd if=/dev/zero of="$T/base-sec.img" bs=4096 count=3952 status=none   # 0xF70000
cat "$T/base.img" >> "$T/base-sec.img"
rm "$T/base.img"

# 4) 更新包 BKTR 解析 + 目录展开
echo "更新包 BKTR 解析(约 8.4 GB)..."
"$HT" -k "$KEYS" --disablekeywarns --titlekey "$UPD_TK" \
    --baseromfs "$T/base-sec.img" --romfsdir "$OUT.part" "$UPD_NCA"
mv "$OUT.part" "$OUT"
echo "好了:$OUT(46 文件,8.36 GiB;含游戏需要的 18 个 .data 资源档案)"
