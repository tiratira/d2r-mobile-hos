#!/usr/bin/env python3
"""从离线 SPIR-V 中删除 SPV_EXT_fragment_shader_interlock 扩展声明。

背景:PC 端 Khronos 校验层报 VUID-VkShaderModuleCreateInfo-pCode-08742——
4342 个着色器声明了该扩展但未启用对应特性;移动 GPU 通常不支持
VK_EXT_fragment_shader_interlock,真机上表现为 vkCreateGraphicsPipelines
返回 OOM(gpipe_err=-1)。上游 Metal 路线同样不依赖该扩展(仅影响排序语义)。

用法: spv_strip_interlock.py <spv 目录> <输出目录>
注意: 仅删除 OpExtension 声明;若着色器声明了 interlock 能力/排序装饰,
      还需继续删除对应 OpCapability/OpDecorate,否则 spirv-val 会报
      -pCode-08737/-08740(见 docs/VULKAN_REPLAY_PLAN.md 的后续步骤)。
"""
import struct, os, sys, glob

def strip(path):
    d = open(path, "rb").read()
    if len(d) < 20 or d[:4] != b"\x03\x02\x23\x07":
        return None, 0
    n = len(d) // 4
    w = list(struct.unpack_from("<%dI" % n, d, 0))
    i, ins = 5, []
    while i < n:
        wc = w[i] >> 16; op = w[i] & 0xFFFF
        ins.append((i, wc, op)); i += wc
    drop, cnt = set(), 0
    for j, wc, op in ins:
        if op == 10:  # OpExtension
            s = d[(j + 1) * 4:].split(b"\0")[0].decode(errors="ignore")
            if s == "SPV_EXT_fragment_shader_interlock":
                drop.add(j); cnt += 1
    if not drop:
        return None, 0
    i, keep = 5, []
    while i < n:
        wc = w[i] >> 16; op = w[i] & 0xFFFF
        if i not in drop:
            keep.append((i, wc))
        i += wc
    out = w[:5]
    for j, wc in keep:
        out.extend(w[j:j + wc])
    return struct.pack("<%dI" % len(out), *out), cnt

if __name__ == "__main__":
    src, dst = sys.argv[1], sys.argv[2]
    os.makedirs(dst, exist_ok=True)
    tot = ch = cnt = 0
    for f in glob.glob(os.path.join(src, "*.spv")):
        r, c = strip(f); tot += 1; cnt += c
        open(os.path.join(dst, os.path.basename(f)), "wb").write(r if r else open(f, "rb").read())
        if r: ch += 1
    print(f"strip_interlock: {tot} files, changed {ch}, extensions removed {cnt}")
