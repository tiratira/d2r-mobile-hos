#!/usr/bin/env python3
"""修 VUID-RuntimeSpirv-maxCombinedClipAndCullDistances-00710。

游戏着色器的 gl_PerVertex 声明 ClipDistance[8] + CullDistance[1](合计 9)
超过 Vulkan 上限 8。做法:定位 ClipDistance 成员的数组类型长度常量(8),
改成 7(7+1=8),其余指令/装饰保持不变。

用法: spv_fix_clipcull.py <spv 目录> <输出目录>
"""
import struct, os, sys, glob

def fix(path):
    d = open(path, "rb").read()
    if len(d) < 20 or d[:4] != b"\x03\x02\x23\x07":
        return None, 0
    n = len(d) // 4
    w = list(struct.unpack_from("<%dI" % n, d, 0))
    i, ins = 5, []
    while i < n:
        wc = w[i] >> 16; op = w[i] & 0xFFFF
        ins.append((i, wc, op)); i += wc
    structs, arrays, used = {}, {}, set()
    for j, wc, op in ins:
        if op == 30 and wc >= 3: structs[w[j + 1]] = (j + 2, wc - 2)   # OpTypeStruct
        if op == 28 and wc >= 4: arrays[w[j + 1]] = w[j + 3]            # OpTypeArray
    for j, wc, op in ins:
        if op == 72 and wc >= 5 and w[j + 3] == 11 and w[j + 4] == 3:   # MemberDecorate BuiltIn ClipDistance
            st, midx = w[j + 1], w[j + 2]
            if st in structs:
                base, cnt = structs[st]
                if midx < cnt and w[base + midx] in arrays:
                    used.add(arrays[w[base + midx]])
    changed = 0
    for j, wc, op in ins:
        if op == 43 and wc >= 4 and w[j + 2] in used and w[j + 3] == 8:  # OpConstant 8
            w[j + 3] = 7; changed += 1
    if not changed:
        return None, 0
    return struct.pack("<%dI" % len(w), *w), changed

if __name__ == "__main__":
    src, dst = sys.argv[1], sys.argv[2]
    os.makedirs(dst, exist_ok=True)
    tot = ch = 0
    for f in glob.glob(os.path.join(src, "*.spv")):
        r, c = fix(f); tot += 1
        open(os.path.join(dst, os.path.basename(f)), "wb").write(r if r else open(f, "rb").read())
        if r: ch += 1
    print(f"fix_clipcull: {tot} files, changed {ch}")
