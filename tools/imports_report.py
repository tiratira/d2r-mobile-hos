#!/usr/bin/env python3
"""imports_report.py — patched HOS 模块的导入解析覆盖报告(开发机工具)。

读 `dm-work/build/hos/<模块>/<模块>.patched.bin`(patch_nso_hos.py 产物),
解析动态符号表,把每个未定义导入按解析去向分类:模块内定义 / 上游服务
HLE_FORCE(swforce)/ 未解析;并列出真机上会落记名桩的清单。x86_64 与
ARM64 宿主走同一套解析代码,所以这份报告就是「还差哪些 HLE」的排期表。

用法:python tools/imports_report.py [dm-work 根目录]
"""
import collections
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
WORK = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "dm-work")
UP = os.path.join(os.path.dirname(ROOT), "d2r-mobile-src", "dm", "host")


def parse_image(path):
    img = open(path, "rb").read()
    mod0 = struct.unpack_from("<I", img, 4)[0]
    assert img[mod0:mod0 + 4] == b"MOD0", (path, hex(mod0))
    dyn = {}
    p = mod0 + struct.unpack_from("<i", img, mod0 + 4)[0]
    while True:
        tag, val = struct.unpack_from("<QQ", img, p)
        p += 16
        if tag == 0:
            break
        dyn.setdefault(tag, val)
    symtab, strtab = dyn[6], dyn[5]
    n = (strtab - symtab) // 24
    undef, defined = [], []
    for i in range(1, n):
        st_name, st_info, _other, st_shndx, _val, _sz = struct.unpack_from(
            "<IBBHQQ", img, symtab + i * 24)
        end = img.find(b"\0", strtab + st_name)
        name = img[strtab + st_name:end].decode("utf-8", "replace")
        if not name:
            continue
        (undef if st_shndx == 0 else defined).append((name, st_info))
    return undef, defined


def upstream_force_names():
    force = set()
    if not os.path.isdir(UP):
        return force
    for fn in sorted(os.listdir(UP)):
        if not (fn.startswith("hle_") and fn.endswith(".c")):
            continue
        txt = open(os.path.join(UP, fn), encoding="utf-8", errors="replace").read()
        force.update(re.findall(r'HLE_FORCE\("([^"]+)"', txt))
    return force


def hos_registries():
    """本仓 host/*.c 的 HLE_FORCE(swforce)与 HLE(swhle)登记名。"""
    force, hle = set(), set()
    d = os.path.join(HERE, "..", "entry", "src", "main", "cpp", "host")
    if not os.path.isdir(d):
        return force, hle
    for fn in sorted(os.listdir(d)):
        if not fn.endswith(".c"):
            continue
        txt = open(os.path.join(d, fn), encoding="utf-8", errors="replace").read()
        force.update(re.findall(r'HLE_FORCE\("([^"]+)"', txt))
        hle.update(re.findall(r'HLE\("([^"]+)"', txt))
    return force, hle


def group(name):
    if not name.startswith("_ZN2nn"):
        return "non-nn"
    m = re.match(r"_ZN2nn(\d+)([A-Za-z0-9_]+)", name)
    if not m:
        return "nn-other"
    return m.group(2)[:int(m.group(1))]


def main():
    mods = {}
    for mod in ("main", "sdk", "subsdk0"):
        p = os.path.join(WORK, "build", "hos", mod, f"{mod}.patched.bin")
        if os.path.isfile(p):
            mods[mod] = parse_image(p)
    defs = set()
    for undef, defined in mods.values():
        defs.update(n for n, _ in defined)
    force = upstream_force_names()
    hos_force, hos_hle = hos_registries()
    # hle_libc.c 里以 hle_ent 手工登记的重定位范围数据符号(swhle)
    hos_hle |= {"__rel_dyn_start", "__rel_dyn_end", "__rel_plt_start", "__rel_plt_end"}
    force |= hos_force
    unresolved = collections.Counter()
    resolved = collections.Counter()
    resolved_unique = collections.defaultdict(set)
    for mod, (undef, _d) in mods.items():
        for name, info in undef:
            # 与 loader_hos.c 的解析顺序一致:swforce → 模块内定义 → swhle
            if name in force:
                kind = "swforce 强制接管"
            elif name in defs:
                kind = "模块内定义"
            elif name in hos_hle:
                kind = "HOS HLE(swhle)"
            else:
                kind = None
            if kind is None:
                unresolved[(group(name), name)] += 1
            else:
                resolved[kind] += 1
                resolved_unique[kind].add(name)
    print(f"模块: {', '.join(f'{m}({len(u)} 导入/{len(d)} 定义)' for m, (u, d) in mods.items())}")
    print(f"导入合计: {sum(len(u) for u, _ in mods.values())}(按符号×模块计数)")
    for k, v in resolved.most_common():
        print(f"  {k}: {len(resolved_unique[k])} 个唯一符号 / {v} 次出现")
    print(f"未解析(按服务分组,共 {sum(unresolved.values())}):")
    byg = collections.Counter()
    for (g, _n), c in unresolved.items():
        byg[g] += c
    for g, c in byg.most_common():
        print(f"  {g:12s} {c}")
    print("未解析明细(按名字重复计数):")
    for (g, n), c in sorted(unresolved.items()):
        print(f"  {g:12s} {n}")


if __name__ == "__main__":
    main()
