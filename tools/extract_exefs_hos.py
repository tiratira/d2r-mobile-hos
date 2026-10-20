# extract_exefs_hos.py — 从你自己的游戏更新 NSP 里解出 ExeFS(main/sdk/subsdk0)。
#
# 等价上游 dm/tools/extract_game.sh 的「取程序文件」一步,但用 nsz 便携包
# (D:\Tools\nsz_v4.6.1_win64_portable)自带的 Python/nut 解密栈,无需 hactool。
# 密钥与游戏包只在本机读取,不离开本机。
#
# 用法(bundled python 运行):
#   D:/Tools/nsz_v4.6.1_win64_portable/python.exe tools/extract_exefs_hos.py \
#       <更新.nsp> <prod.keys> <输出目录>
# 产物:<输出目录>/{main,sdk,subsdk0},并核对 3.3.94012 的官方 sha256。
import os, sys, shutil, hashlib
from pathlib import Path

NSP = sys.argv[1]
KEYS = sys.argv[2]
OUT = Path(sys.argv[3])
NSZ_PKG = os.environ.get('NSZ_PY_PATH',
                         r'D:\Tools\nsz_v4.6.1_win64_portable\Lib\site-packages')

# nsz 的 Keys 模块在导入时会自动找密钥(脚本同目录 keys.txt 或 ~/.switch/prod.keys),
# 找不到会停在 input()。先把用户给的 prod.keys 放到 ~/.switch(nsz 的标准位置)。
home_keys = Path.home() / '.switch' / 'prod.keys'
if not home_keys.is_file():
    home_keys.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(KEYS, home_keys)

sys.path.insert(0, NSZ_PKG)
from nsz import Fs                      # noqa: E402
from nsz.Fs import Nca, Ticket          # noqa: E402
from nsz.nut import Keys, Titles, Print # noqa: E402

WANT = {
    'main':    '55fd909bb7230b002357e5b0d8499c11f06888f345c49e69eb4734df44a27d0e',
    'sdk':     '1c9f198a91c0bcfee5cf8f5b53fb941895e91334dfb766ce79fc68576797f017',
    'subsdk0': '67a38fcf89cd13e040a021eb3d157072f38ebf6680a907874c35c7b231b5d4e2',
}

Keys.load(str(home_keys))
Print.info('keys loaded')

nsp = Fs.factory(Path(NSP))
nsp.open(NSP, 'rb')

def ent_name(e):
    if isinstance(e, dict):
        return e['name']
    return getattr(e, '_path', '') or ''

# 1) NSP 层的文件对象(File,带 _path):先把 ticket 注册进 Titles
n_tickets = 0
for ent in nsp:
    name = ent_name(ent)
    if not name.lower().endswith('.tik'):
        continue
    tik = Ticket.Ticket()
    tik.open(ent)
    tkey = tik.getTitleKey().hex().upper()          # 加密标题密钥(解密在 NcaHeader 内做)
    tid = tik.rightsId[0:16].upper()
    t = Titles.Title()
    t.id = tid
    t.key = tkey
    Titles.data()[tid] = t
    n_tickets += 1
    Print.info(f'ticket: {tid} key={tkey}')
assert n_tickets > 0, 'NSP 里没有 ticket'

# 2) Program NCA 的 ExeFS 分区里取三个 NSO(ExeFS 层条目是 dict)
OUT.mkdir(parents=True, exist_ok=True)
written = {}
for ent in nsp:
    name = ent_name(ent)
    if not name.lower().endswith('.nca') or name.lower().endswith('.cnmt.nca'):
        continue
    nca = Nca.Nca()
    try:
        nca.open(ent)
    except Exception as e:
        Print.info(f'skip {name}: {e}')
        continue
    if str(nca.header.contentType) != 'Content.PROGRAM':
        continue
    exefs = nca[0]                                   # Program NCA 第 0 分区 = ExeFS
    Print.info(f'program nca: {name} build={nca.buildId()}')
    for sub in exefs.files:
        sname = ent_name(sub)
        if sname not in WANT:
            continue
        size = sub['size'] if isinstance(sub, dict) else sub.size
        f = exefs[sname]
        f.seek(0)
        data = f.read(size)
        (OUT / sname).write_bytes(data)
        written[sname] = hashlib.sha256(data).hexdigest()
        Print.info(f'  {sname}: {len(data)} bytes sha256={written[sname]}')
    break

# 3) 与 3.3.94012 官方基准核对
ok = True
for name, want in WANT.items():
    got = written.get(name, '(缺)')
    mark = 'OK ' if got == want else 'BAD'
    print(f'  [{mark}] {name}: {got}')
    ok = ok and got == want
if not ok:
    sys.exit('校验失败:更新包不是上游支持的 3.3.94012')
print(f'ExeFS 就绪:{OUT}')
