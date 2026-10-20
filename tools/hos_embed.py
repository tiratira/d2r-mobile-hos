# hos_embed.py — 把 patch_nso_hos.py 的重写产物打包成可编进 libdmhost.so 的汇编(容器 v3)。
#
# 对应上游 iOS 的 ios/tools/gen_embed.py。v3 关键变化(为装载器原地重定位设计):
#   - 镜像在节起始处、16K 对齐;重定位只写数据窗口(ro..patch_off),装载器对
#     [ro_off, patch_off) mprotect RW 即可,代码区(text 与补丁区)保持 .so 签名页
#   - 布局元数据(PATCH_OFF/TEXT_END/RO_OFF/DATA_OFF,取自 <名>.fixups.tsv)与
#     门表跟在镜像后(镜像补齐到 16K 之后),装载器在 align(image_size) 处解析
#   - arm64 发 `bl dm_gate`(汇编期解析,PC 相对偏移在 .so 内原地有效);
#     --data-only 供 x86_64 构建把镜像当纯数据嵌入(同一装载/重定位代码可测)
#
# 容器 v3 布局(小端):
#   [镜像 patched.bin 原样,补 0 到 16K 倍数]
#   DmGuestMeta(56B):{u32 magic 'D2RH'; u32 version=3; u32 module_kind; u32 gate_count;
#                     u64 image_size(未补齐); u64 entry_off; u64 patch_off; u64 text_end;
#                     u64 ro_off; u64 data_off}
#   门表(gate_count × 32B):{u64 ret_off; u32 kind; u32 arg; u64 orig_off; u64 rsv}
#   kind:0=svc 1=cntpct 2=cntvct 3=obs 4=tpidr_r 5=tpidr_w 6=tpidrro_r 7=param
#
# 用法:python hos_embed.py --outdir <产物目录> --name <模块名> --kind <0|1|2>
#           [--src <patched.bin 路径>] --out <guest_<名>.S 输出路径> [--data-only]
#       python hos_embed.py --placeholder --out dm_guest_placeholder.S
# 产物目录里需有 <名>.patched.bin / .gates.tsv / .gatebl.tsv / .fixups.tsv。
import argparse, os, struct

KIND_CODE = {'svc': 0, 'cntpct': 1, 'cntvct': 2, 'obs': 3,
             'tpidr_r': 4, 'tpidr_w': 5, 'tpidrro_r': 6, 'param': 7}
PAGE = 0x4000
MAGIC = 0x48523244
VERSION = 3


def parse_gates(path):
    gates = []
    if not os.path.isfile(path):
        return gates
    for line in open(path, encoding='utf-8'):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        ret, kind, arg, orig = line.split('\t')
        gates.append((int(ret, 16), kind, int(arg), int(orig, 16)))
    return gates


def parse_gatebl(path):
    out = []
    for line in open(path, encoding='utf-8'):
        line = line.strip()
        if line and not line.startswith('#'):
            out.append(int(line, 16))
    return sorted(out)


def parse_fixups(path):
    d = {}
    for line in open(path, encoding='utf-8'):
        if line.startswith('#') and '\t' in line:
            k, v = line[1:].strip().split('\t')
            try:
                d[k] = int(v, 16) if v.startswith('0x') else int(v)
            except ValueError:
                pass          # RULESET 等非数值行
    return d


def esc(p):
    return p.replace('\\', '/')


def emit(name, kind, src, img_sz, bls, gates, layout, out, gate_sym, data_only):
    pad = (-img_sz) % PAGE
    lines = [
        f'// 由 hos_embed.py 生成,请勿手改。模块 {name},门 {len(gates)} 个,'
        f'{"data-only(x86)" if data_only else "arm64 原地执行"}。',
        f'    .section .dm_guest_{name}, "{ "a" if data_only else "ax" }"',
        f'    .balign {PAGE}',
        f'    .globl __dm_guest_{name}_start',
        f'    .type __dm_guest_{name}_start, @object',
        f'__dm_guest_{name}_start:',
    ]
    if data_only:
        # x86:镜像为纯数据,bl 占位原样保留(不执行)
        lines.append(f'    .incbin "{esc(os.path.abspath(src))}"')
    else:
        # arm64:按 gatebl 偏移切片,切点处发 `bl dm_gate`(汇编期符号解析)
        pos = 0
        for off in bls:
            if off > pos:
                lines.append(f'    .incbin "{esc(os.path.abspath(src))}", {pos}, {off - pos}')
            lines.append(f'    bl {gate_sym}')
            pos = off + 4
        if pos < img_sz:
            lines.append(f'    .incbin "{esc(os.path.abspath(src))}", {pos}, {img_sz - pos}')
    if pad:
        lines.append(f'    .zero {pad}')
    # 元数据(56B):独立符号,装载器双指针加载并互验对齐
    lines.append('    .balign 8')
    lines.append(f'    .globl __dm_guest_{name}_meta')
    lines.append(f'__dm_guest_{name}_meta:')
    lines.append('    // DmGuestMeta v3')
    lines.append(f'    .long {MAGIC}')
    lines.append(f'    .long {VERSION}')
    lines.append(f'    .long {kind}')
    lines.append(f'    .long {len(gates)}')
    for v in (img_sz, layout.get('entry', 0), layout['PATCH_OFF'],
              layout['TEXT_END'], layout['RO_OFF'], layout['DATA_OFF']):
        lines.append(f'    .quad {v}')
    # 门表
    if gates:
        lines.append(f'    .globl __dm_guest_{name}_gates')
        lines.append(f'__dm_guest_{name}_gates:')
        for ret, k, arg, orig in gates:
            lines.append(f'    .quad {ret}    // {k} {arg} orig={orig:#x}')
            lines.append(f'    .long {KIND_CODE[k]}')
            lines.append(f'    .long {arg}')
            lines.append(f'    .quad {orig}')
            lines.append('    .quad 0')
    lines.append(f'    .size __dm_guest_{name}_start, .-__dm_guest_{name}_start')
    open(out, 'w', encoding='utf-8').write('\n'.join(lines) + '\n')
    print(f'{out}: image={img_sz}B(+{pad} pad) gates={len(gates)} '
          f'bls={len(bls) if not data_only else "-"} '
          f'patch={layout["PATCH_OFF"]:#x} text_end={layout["TEXT_END"]:#x}')


def emit_placeholder(a, stub, gates, layout):
    """占位容器:16B 镜像内联(arm64 第二个字发 bl dm_gate),其余与 emit 相同。"""
    name, kind, gate_sym, data_only = 'placeholder', 0, a.gate_sym, a.data_only
    img_sz = len(stub)
    pad = PAGE - img_sz
    lines = [
        f'// 由 hos_embed.py 生成,请勿手改。占位容器(16B tpidr 门桩),'
        f'{"data-only(x86)" if data_only else "arm64"}。',
        f'    .section .dm_guest_placeholder, "{ "a" if data_only else "ax" }"',
        f'    .balign {PAGE}',
        f'    .globl __dm_guest_placeholder_start',
        f'    .type __dm_guest_placeholder_start, @object',
        f'__dm_guest_placeholder_start:',
        f'    .byte {", ".join(str(b) for b in stub[:4])}',
    ]
    if data_only:
        lines.append(f'    .byte {", ".join(str(b) for b in stub[4:8])}')
    else:
        lines.append(f'    bl {gate_sym}')
    lines.append(f'    .byte {", ".join(str(b) for b in stub[8:])}')
    lines.append(f'    .zero {pad}')
    lines.append('    .balign 8')
    lines.append('    .globl __dm_guest_placeholder_meta')
    lines.append('__dm_guest_placeholder_meta:')
    lines.append('    // DmGuestMeta v3')
    lines.append(f'    .long {MAGIC}')
    lines.append(f'    .long {VERSION}')
    lines.append(f'    .long {kind}')
    lines.append(f'    .long {len(gates)}')
    for v in (img_sz, 0, layout['PATCH_OFF'], layout['TEXT_END'],
              layout['RO_OFF'], layout['DATA_OFF']):
        lines.append(f'    .quad {v}')
    lines.append('    .globl __dm_guest_placeholder_gates')
    lines.append('__dm_guest_placeholder_gates:')
    for ret, k, arg, orig in gates:
        lines.append(f'    .quad {ret}')
        lines.append(f'    .long {KIND_CODE[k]}')
        lines.append(f'    .long {arg}')
        lines.append(f'    .quad {orig}')
        lines.append('    .quad 0')
    lines.append('    .size __dm_guest_placeholder_start, .-__dm_guest_placeholder_start')
    open(a.out, 'w', encoding='utf-8').write('\n'.join(lines) + '\n')
    print(f'{a.out}: placeholder image={img_sz}B(+{pad} pad) gates={len(gates)} '
          f'({"data-only" if data_only else "arm64"})')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--outdir')
    ap.add_argument('--name')
    ap.add_argument('--kind', type=int, default=0, choices=(0, 1, 2))
    ap.add_argument('--src')
    ap.add_argument('--out', required=True)
    ap.add_argument('--gate-sym', default='dm_gate',
                    help='宿主门入口符号(ELF 无下划线前缀,默认 dm_gate)')
    ap.add_argument('--data-only', action='store_true',
                    help='x86_64 构建:镜像纯数据嵌入,不做 bl 替换')
    ap.add_argument('--placeholder', action='store_true',
                    help='生成占位容器(16B tpidr 门桩自检用)')
    a = ap.parse_args()

    if a.placeholder:
        # 占位:镜像 16B(stp; bl dm_gate; ldp; ret),1 个 tpidr_r 门。
        # 字节直接内联(arm64 用 bl 替换第 2 个字,不落临时文件)
        stub = bytes.fromhex('fd7bbea9' + '00000094' + 'fd17c1a8' + 'c0035fd6')
        gates = [(8, 'tpidr_r', 0, 0)]
        layout = {'PATCH_OFF': PAGE, 'TEXT_END': 0x10, 'RO_OFF': 0, 'DATA_OFF': 0}
        emit_placeholder(a, stub, gates, layout)
        return

    src = a.src or os.path.join(a.outdir, f'{a.name}.patched.bin')
    img_sz = os.path.getsize(src)
    gates = parse_gates(os.path.join(a.outdir, f'{a.name}.gates.tsv'))
    bls = parse_gatebl(os.path.join(a.outdir, f'{a.name}.gatebl.tsv'))
    fx = parse_fixups(os.path.join(a.outdir, f'{a.name}.fixups.tsv'))
    if not a.data_only:
        blob = open(src, 'rb').read()
        for off in bls:
            w = struct.unpack_from('<I', blob, off)[0]
            assert w >> 26 == 0x25, f'gatebl {off:#x} 不是 bl 占位:{w:#x}'
    for k in ('PATCH_OFF', 'TEXT_END', 'RO_OFF', 'DATA_OFF'):
        assert k in fx, f'fixups.tsv 缺 {k}'
    emit(a.name, a.kind, src, img_sz, bls, gates, fx, a.out, a.gate_sym, a.data_only)


if __name__ == '__main__':
    main()
