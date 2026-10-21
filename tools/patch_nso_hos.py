# patch_nso_hos.py — d2r-mobile NSO 改写器的 HarmonyOS(HOS)规则域。
#
# 派生自上游 dm/tools/patch_nso.py(Apple 规则域)。HOS 与 Apple 的关键差异:
#
#   Apple 版补丁代码用 `mrs tpidrro_el0` 取 pthread TSD 基址(Apple 内核把 TSD
#   数组指针放进 tpidrro)——Linux/OHOS 内核往 tpidrro_el0 写的是线程 TID(小
#   整数),Apple 产物不能直接跑;x18 在 OHOS 上虽不被内核征用,但它是客体
#   自写自读的寄存器,宿主不能拿它当槽基址。
#
#   HOS 方案:镜像经 hos_embed.py 编进 libdmhost.so 的同一可执行段,门桩直接
#   `bl dm_gate`(汇编期符号解析),彻底不需要每线程基址寄存器:
#     svc #n / mrs cntpct / mrs cntvct  → stp; bl dm_gate; ldp; b 返回
#     mrs tpidr_el0 / tpidrro_el0 / msr tpidr_el0 → 同上(宿主侧 pthread TSD)
#     x18 读写                          → 不改写(OHOS 内核不征用 x18,
#                                          客体自写自读,与 Horizon 语义一致)
#   CUTS / OBS / PARAMS 数据表直接从上游 patch_nso.py 源码切片提取,保持同步。
#
# 用法:python patch_nso_hos.py <NSO 文件> <输出目录> <模块名>
#   环境:DM_RULESET=hos(本脚本自身即 hos 域);DM_UPSTREAM 指向上游
#   dm/tools/patch_nso.py(默认:同目录上一级 ../d2r-mobile-src/dm/tools/)。
# 输出:与上游同名产物 + <名>.gatebl.tsv(镜像文件内须由 hos_embed.py 换成
#   `bl dm_gate` 的字偏移,每行一个)。
import os, struct, sys
import lz4.block

src, outdir, NAME = sys.argv[1], sys.argv[2], sys.argv[3]

def _find_upstream():
    here = os.path.dirname(os.path.abspath(__file__))
    for cand in (os.path.join(here, '..', 'd2r-mobile-src'),
                 os.path.join(here, '..', '..', 'd2r-mobile-src')):
        p = os.path.join(cand, 'dm', 'tools', 'patch_nso.py')
        if os.path.isfile(p):
            return p
    return None

UPSTREAM = os.environ.get('DM_UPSTREAM') or _find_upstream()
assert UPSTREAM, '找不到上游 patch_nso.py:设 DM_UPSTREAM 或把上游克隆到 ../d2r-mobile-src'
RULESET = os.environ.get('DM_RULESET', 'hos')
assert RULESET == 'hos', '本脚本只实现 hos 规则域'

# ---- 从上游源码提取纯数据表(CUTS/OBS/PARAMS),与上游保持同步 ----
# CUTS 与 OBS 两个字典字面量在上游源码里相邻(……]}\nBUILD_ID = ),一次切片全取。
def _slice(text, begin, end):
    i = text.index(begin)
    j = text.index(end, i)
    return text[i:j]
_up = open(UPSTREAM, encoding='utf-8').read()
_ns = {'struct': struct}
exec(_slice(_up, 'CUTS = {', '\nBUILD_ID = '), _ns)   # 定义 CUTS 与 OBS
CUTS = _ns['CUTS']
OBS = _ns['OBS']
exec(_slice(_up, 'PARAMS = {', '\nparams = '), _ns)
PARAMS = _ns['PARAMS']

d = open(src, 'rb').read()
assert d[:4] == b'NSO0'
flags = struct.unpack_from('<I', d, 0xc)[0]
segs = [struct.unpack_from('<III', d, 0x10 + i * 0x10) for i in range(3)]
bss = struct.unpack_from('<I', d, 0x3c)[0]
csz = struct.unpack_from('<III', d, 0x60)
IMG = bytearray(segs[2][1] + segs[2][2] + bss)
for i, (fo, mo, sz) in enumerate(segs):
    raw = d[fo:fo + csz[i]]
    if flags >> i & 1:
        raw = lz4.block.decompress(raw, uncompressed_size=sz)
    IMG[mo:mo + sz] = raw
TEXT_END = segs[0][2]
RO_OFF, DATA_OFF = segs[1][1], segs[2][1]
KEY_PAGE_OFF = (len(IMG) + 0x3FFF) & ~0x3FFF
IMG.extend(bytes(0x4000))
PATCH_OFF = (len(IMG) + 0xFFFF) & ~0xFFFF
BUILD_ID = d[0x40:0x50].hex()

# ---- 剪口:与上游同一份表(字节级补丁,与 OS 无关;'仅宿主'在 HOS 上我们就是宿主) ----
cuts = list(CUTS.get(BUILD_ID, []))
for c in cuts:
    if len(c) == 4:
        c = (*c, '全')
    off, ob, nb, why, scope = c
    ob, nb = bytes.fromhex(ob), bytes.fromhex(nb)
    assert len(ob) == len(nb) and IMG[off:off + len(ob)] == ob, \
        f'剪口原字节不符 {off:#x}:{bytes(IMG[off:off + len(ob)]).hex()}'
    IMG[off:off + len(nb)] = nb

# 键页下标(与上游一致;HOS 下 K18/KTLS/KTLR/KGATE 槽不被桩代码内联读取,
# KRES*/KASPECT* 由 param 门按同一序号读 pthread TSD)
KEY_INDEX = {k: i for i, k in enumerate(
    ('K18', 'KTLS', 'KTLR', 'KGATE', 'KRES0', 'KRES1', 'KASPECT', 'KASPECTUI'))}

patch = bytearray()
gates = []
gatebl = []          # <镜像文件内偏移>:该 4 字节须在 .S 里换成 `bl _dm_gate`
stats = {}
def stat(k): stats[k] = stats.get(k, 0) + 1
def here(): return PATCH_OFF + len(patch)
def emit(w): patch.extend(struct.pack('<I', w & 0xFFFFFFFF))
def B(frm, to):
    off = to - frm
    assert -(1 << 27) <= off < (1 << 27) and off % 4 == 0
    return 0x14000000 | ((off >> 2) & 0x3FFFFFF)
STP_PRE_2930 = 0xA9BF7BFD        # stp x29, x30, [sp, #-16]!
LDP_POST_2930 = 0xA8C17BFD       # ldp x29, x30, [sp], #16
NOP = 0xD503201F
BL_GATE = 0x94000000             # 占位:hos_embed.py 会在这些位置发 `bl _dm_gate`

def put_branch(at, to): struct.pack_into('<I', IMG, at, B(at, to))

def site_gate(kind, arg, orig, extra_words=()):
    """HOS 门桩:stp; bl _dm_gate(占位); ldp; [extra]; b 回原处+4。
    bl 的字偏移(= ret-4)记入 gatebl.tsv,由 hos_embed.py 换成汇编符号调用。"""
    start = here()
    emit(STP_PRE_2930)
    emit(BL_GATE)                       # ← gatebl 记录此字
    gatebl.append(PATCH_OFF + len(patch) - 4)
    ret = here()
    emit(LDP_POST_2930)
    for w in extra_words:
        emit(w)
    emit(B(here(), orig + 4))
    gates.append((ret, kind, arg, orig))
    return start

# ---- 线性扫描:HOS 只改写 5 类指令字(x18 一律保留) ----
kinds = {}
for o in range(0, TEXT_END, 4):
    w = struct.unpack_from('<I', IMG, o)[0]
    if w & 0xFFE0001F == 0xD4000001: kinds[o] = ('svc', (w >> 5) & 0xFFFF); continue
    if w & 0xFFFFFFE0 == 0xD53BD040: kinds[o] = ('tpidr_r', w & 31); continue
    if w & 0xFFFFFFE0 == 0xD51BD040: kinds[o] = ('tpidr_w', w & 31); continue
    if w & 0xFFFFFFE0 == 0xD53BD060: kinds[o] = ('tpidrro_r', w & 31); continue
    if w & 0xFFFFFFE0 == 0xD53BE020: kinds[o] = ('cntpct', w & 31); continue
    if w & 0xFFFFFFE0 == 0xD53BE040: kinds[o] = ('cntvct', w & 31); continue

for a in sorted(kinds):
    k, arg = kinds[a]
    st = site_gate(k, arg, a)
    put_branch(a, st)
    stat(k)

# ---- 观测门:与上游同构(stub 里 bl 同样走 gatebl) ----
obs = list(OBS.get(BUILD_ID, []))
for off, oid, ow, why in obs:
    w = struct.unpack_from('<I', IMG, off)[0]
    assert f'{w:08x}' == ow, f'观测门原指令不符 {off:#x}:{w:08x}'
    assert off < TEXT_END and off not in kinds
    st = site_gate('obs', oid, off, extra_words=(w,))
    put_branch(off, st)
    stat('obs')

# ---- 参数位:A 跳参数跳板、B nop;跳板 = 门(param),arg = (键号<<5)|寄存器 ----
params = PARAMS.get(BUILD_ID, [])
for oa, wa, ob_, wb, r, key, why in params:
    for o, w0 in ((oa, wa), (ob_, wb)):
        w = struct.unpack_from('<I', IMG, o)[0]
        assert f'{w:08x}' == w0, f'参数位原指令不符 {o:#x}:{w:08x}'
        assert o not in kinds
    st = site_gate('param', (KEY_INDEX[key] << 5) | r, oa)
    put_branch(oa, st)
    struct.pack_into('<I', IMG, ob_, NOP)
    stat('param')

# ---- 输出(与上游同格式 + gatebl.tsv) ----
os.makedirs(outdir, exist_ok=True)
P = lambda ext: os.path.join(outdir, f'{NAME}.{ext}')
open(P('patched.bin'), 'wb').write(IMG + bytearray(PATCH_OFF - len(IMG)) + patch)
with open(P('fixups.tsv'), 'w', encoding='utf-8') as f:
    f.write(f'# RULESET\thos\n# PATCH_OFF\t{PATCH_OFF:#x}\n# KEY_PAGE_OFF\t{KEY_PAGE_OFF:#x}\n'
            f'# TEXT_END\t{TEXT_END:#x}\n# RO_OFF\t{RO_OFF:#x}\n# DATA_OFF\t{DATA_OFF:#x}\n')
    for k in KEY_INDEX: f.write(f'# KEY_{k}_IDX\t{KEY_INDEX[k]}\n')
with open(P('gates.tsv'), 'w', encoding='utf-8') as f:
    for ret, k, arg, orig in gates: f.write(f'{ret:#x}\t{k}\t{arg}\t{orig:#x}\n')
with open(P('sites.tsv'), 'w', encoding='utf-8') as f:
    for a in sorted(kinds): f.write(f'{a:#x}\t{kinds[a][0]}\n')
with open(P('gatebl.tsv'), 'w', encoding='utf-8') as f:
    for o in gatebl: f.write(f'{o:#x}\n')
with open(P('cuts.tsv'), 'w', encoding='utf-8') as f:
    f.write(f'# BUILD_ID\t{BUILD_ID}\n')
    for off, ob, nb, *rest in cuts:
        why = rest[-1] if isinstance(rest[-1], str) else ''
        scope = rest[0] if rest and isinstance(rest[0], str) and rest[0] in ('全', '仅宿主') else '全'
        f.write(f'{off:#x}\t{ob}\t{nb}\t{scope}\t{why}\n')
with open(P('params.tsv'), 'w', encoding='utf-8') as f:
    f.write(f'# BUILD_ID\t{BUILD_ID}\n')
    for oa, wa, ob_, wb, r, key, why in params: f.write(f'{oa:#x}\t{ob_:#x}\tx{r}\t{key}\t{why}\n')
with open(P('obs.tsv'), 'w', encoding='utf-8') as f:
    f.write(f'# BUILD_ID\t{BUILD_ID}\n')
    for off, oid, ow, why in obs: f.write(f'{off:#x}\t{oid}\t{ow}\t{why}\n')
print(NAME, stats, 'cuts', len(cuts), 'obs', len(obs), 'params', len(params),
      'patch', len(patch), 'PATCH_OFF', hex(PATCH_OFF), 'gates', len(gates))

