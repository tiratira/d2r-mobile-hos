# hos_embed.py — 把 patch_nso_hos.py 的重写产物打包成可编进 libdmhost.so 的汇编。
#
# 对应上游 iOS 的 ios/tools/gen_embed.py:OHOS 与 iOS 同样禁止运行时生成可执行
# 页,重写镜像必须作为签名代码段编进 .so。镜像整体(patch 区已接在镜像尾)放
# 进一个 "ax" 段;段内 gatebl.tsv 列出的 4 字节位置(重写器留的 bl 占位)在汇编
# 里换成 `bl _dm_gate` —— 镜像与宿主门入口在同一 .so,汇编期即可解析符号。
#
# 容器布局(小端,与 cpp/loader/dm_guest_loader.h 的 v2 一致):
#   DmGuestImageHeader(32B) + patched.bin 原样 + 门表(gate_count × 32B)
#   门项:{u64 ret_off; u32 kind; u32 arg; u64 orig_off; u64 rsv}
#   kind:0=svc 1=cntpct 2=cntvct 3=obs 4=tpidr_r 5=tpidr_w 6=tpidrro_r 7=param
#
# 用法:python hos_embed.py --outdir <产物目录> --name <模块名> --kind <0|1|2>
#           [--src <patched.bin 路径>] --out <guest_<名>.S 输出路径>
# 产物目录里需有 <名>.patched.bin / .gates.tsv / .gatebl.tsv(缺 gatebl 视为无门)。
import argparse, os, struct

KIND_CODE = {'svc': 0, 'cntpct': 1, 'cntvct': 2, 'obs': 3,
             'tpidr_r': 4, 'tpidr_w': 5, 'tpidrro_r': 6, 'param': 7}

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
    if not os.path.isfile(path):
        return out
    for line in open(path, encoding='utf-8'):
        line = line.strip()
        if line and not line.startswith('#'):
            out.append(int(line, 16))
    return sorted(out)

def esc(p):
    return p.replace('\\', '/')

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--outdir', required=True)
    ap.add_argument('--name', required=True)
    ap.add_argument('--kind', type=int, default=0, choices=(0, 1, 2))
    ap.add_argument('--src')
    ap.add_argument('--out', required=True)
    ap.add_argument('--gate-sym', default='dm_gate',
                    help='宿主门入口符号(ELF 无下划线前缀,默认 dm_gate)')
    a = ap.parse_args()

    src = a.src or os.path.join(a.outdir, f'{a.name}.patched.bin')
    blob = open(src, 'rb').read()
    gates = parse_gates(os.path.join(a.outdir, f'{a.name}.gates.tsv'))
    bls = parse_gatebl(os.path.join(a.outdir, f'{a.name}.gatebl.tsv'))
    for off in bls:
        w = struct.unpack_from('<I', blob, off)[0]
        assert w >> 26 == 0x25, f'gatebl {off:#x} 不是 bl 占位:{w:#x}'
    for ret, kind, arg, orig in gates:
        assert kind in KIND_CODE, kind

    hdr = struct.pack('<IIIIQQ', 0x48523244, 2, a.kind, len(gates), len(blob), 0)
    gtab = b''.join(struct.pack('<QIIQQ', ret, KIND_CODE[kind], arg, orig, 0)
                    for ret, kind, arg, orig in gates)

    lines = [
        f'// 由 hos_embed.py 生成,请勿手改。模块 {a.name},门 {len(gates)} 个。',
        f'    .section .dm_guest_{a.name}, "ax"',
        '    .align 12',
        f'    .globl __dm_guest_{a.name}_start',
        f'    .type __dm_guest_{a.name}_start, @object',
        f'__dm_guest_{a.name}_start:',
    ]
    # 容器头(手写数据,保证与 C 端 DmGuestImageHeader 布局一致)
    lines.append('    // DmGuestImageHeader v2')
    lines.append('    .word 0x48523244')
    lines.append(f'    .word 2')
    lines.append(f'    .word {a.kind}')
    lines.append(f'    .word {len(gates)}')
    lines.append(f'    .xword {len(blob)}')
    lines.append('    .xword 0')          # entry_offset:宿主按模块约定另取
    # 镜像:按 gatebl 偏移切片,切点处发 `bl _dm_gate`
    pos = 0
    for off in bls:
        if off > pos:
            lines.append(f'    .incbin "{esc(os.path.abspath(src))}", {pos}, {off - pos}')
        lines.append(f'    bl {a.gate_sym}')
        pos = off + 4
    if pos < len(blob):
        lines.append(f'    .incbin "{esc(os.path.abspath(src))}", {pos}, {len(blob) - pos}')
    # 门表(数据,跟在镜像后;对齐 8)
    if gtab:
        lines.append('    .align 8')
        lines.append(f'    .globl __dm_guest_{a.name}_gates')
        lines.append(f'__dm_guest_{a.name}_gates:')
        for ret, kind, arg, orig in gates:
            lines.append(f'    .xword {ret}    // {kind} {arg} orig={orig:#x}')
            lines.append(f'    .word {KIND_CODE[kind]}')
            lines.append(f'    .word {arg}')
            lines.append(f'    .xword {orig}')
            lines.append('    .xword 0')
    lines.append(f'    .size __dm_guest_{a.name}_start, .-__dm_guest_{a.name}_start')
    open(a.out, 'w', encoding='utf-8').write('\n'.join(lines) + '\n')
    print(f'{a.out}: image={len(blob)}B gates={len(gates)} bls={len(bls)}')

if __name__ == '__main__':
    main()
