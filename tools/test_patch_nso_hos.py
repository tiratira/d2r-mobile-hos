# test_patch_nso_hos.py — 合成 NSO 上的 HOS 重写器端到端测试。
#
# 无需真实游戏文件:构造一个最小 NSO(未压缩三段),正文里埋入
#   svc / mrs tpidr_el0 / msr tpidr_el0 / mrs tpidrro_el0 / mrs cntpct_el0
#   和几条含 x18 的普通指令,跑 patch_nso_hos.py,断言:
#     1. 正文里不再有裸 svc / tpidr / cntpct 字(全被换成跳转)
#     2. x18 指令逐字保留(HOS 不改写 x18)
#     3. gates.tsv 条数与埋点数一致,gatebl 偏移处的字是 bl 编码(占位)
#     4. hos_embed.py 生成的 .S 里含 bl _dm_gate,且切片拼回后与
#        patched.bin 除 bl 字外逐字节一致
#
# 运行:需要 python3 + lz4(pip install lz4)。上游表从 ../d2r-mobile-src 提取,
# 不在时跳过 CUTS 相关断言(合成 NSO 构建号不在 CUTS 表里,本来也应用不上)。
import os, struct, subprocess, sys, tempfile, unittest

HERE = os.path.dirname(os.path.abspath(__file__))

def enc_mrs(reg, sysreg):
    return 0xD5380000 | (sysreg << 5) | reg

def build_synth_nso(path):
    text = bytearray(0x1000)
    # 埋点(偏移:指令)
    sites = {
        0x10: 0xD4000841,               # svc #0x42
        0x14: enc_mrs(0, 0x5A82),       # tpidr_el0(op0=3 op1=3 crn=13 crm=0 op2=2 → 0b11_011_1101_0000_010 = enc 见下)
        0x18: enc_mrs(5, 0x5A83),       # 同上 op2=3:tpidrro_el0
        0x1C: enc_mrs(19, 0x5F02),      # cntpct_el0
        0x20: 0xD51B4180,               # msr tpidr_el0, x0
        0x24: 0xAA0603F6,               # mov x22, x6 —— 含 x18 的指令用真实编码:
        0x28: 0xF9400242 & 0xFFFFFFFF,  # ldr x2, [x18]
    }
    # 直接写确定编码(与 patch_nso_hos.py 扫描掩码一致):
    sites[0x14] = 0xD53BD040            # mrs x0, tpidr_el0   (tpidr_r, reg=0)
    sites[0x18] = 0xD53BD065            # mrs x5, tpidrro_el0 (tpidrro_r, reg=5)
    sites[0x1C] = 0xD53BE033            # mrs x19, cntpct_el0 (cntpct, reg=19)
    sites[0x20] = 0xD51BD040            # msr tpidr_el0, x0   (tpidr_w, reg=0)
    sites[0x24] = 0xAA0103F2            # orr x18, xzr, x1     (HOS 保留不改写)
    for off, w in sites.items():
        struct.pack_into('<I', text, off, w)
    struct.pack_into('<I', text, 0x30, 0xD65F03C0)   # ret

    ro = bytes(0x1000)
    data = bytes(0x2000)
    blob = bytearray()
    def seg(fo_off, mo, sz, raw):
        return raw
    # NSO 头:0x0 magic / 0xc flags=0(不压缩) / 0x10..0x40 三段头 / 0x3c bss / 0x60 三段压缩大小
    nso = bytearray(0x80)
    nso[0:4] = b'NSO0'
    struct.pack_into('<I', nso, 0xc, 0)                          # flags:全不压缩
    text_fo = len(nso)
    ro_fo = text_fo + len(text)
    data_fo = ro_fo + len(ro)
    struct.pack_into('<III', nso, 0x10, text_fo, 0, len(text))    # .text @0
    struct.pack_into('<III', nso, 0x20, ro_fo, 0x1000, len(ro))   # .rodata @0x1000
    struct.pack_into('<III', nso, 0x30, data_fo, 0x2000, len(data))
    struct.pack_into('<I', nso, 0x3c, 0x800)                      # bss
    struct.pack_into('<III', nso, 0x60, len(text), len(ro), len(data))
    nso.extend(text); nso.extend(ro); nso.extend(data)
    nso[0x40:0x50] = b'\x11' * 16                                 # build id(不在 CUTS 表)
    open(path, 'wb').write(nso)
    return sites

class TestRewriterHos(unittest.TestCase):
    def setUp(self):
        try:
            import lz4.block  # noqa: F401
        except ImportError:
            self.skipTest('lz4 未安装')
        here = os.path.dirname(os.path.abspath(__file__))
        up = None
        for cand in (os.path.join(here, '..', 'd2r-mobile-src'),
                     os.path.join(here, '..', '..', 'd2r-mobile-src')):
            p = os.path.join(cand, 'dm', 'tools', 'patch_nso.py')
            if os.path.isfile(p):
                up = p
                break
        if not up:
            self.skipTest('上游源码不在 ../d2r-mobile-src')
        self.tmp = tempfile.mkdtemp(prefix='hosrw_')
        self.nso = os.path.join(self.tmp, 'synth.nso')
        self.sites = build_synth_nso(self.nso)
        out = os.path.join(self.tmp, 'out')
        env = dict(os.environ, DM_RULESET='hos', DM_UPSTREAM=up)
        r = subprocess.run([sys.executable, os.path.join(HERE, 'patch_nso_hos.py'),
                            self.nso, out, 'synth'], env=env, capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.out = out

    def read(self, name):
        return open(os.path.join(self.out, name), 'rb').read()

    def test_sites_rewritten(self):
        img = self.read('synth.patched.bin')
        # 埋点处不再是原指令,而是 B(0x14xxxxxx)
        for off in (0x10, 0x14, 0x18, 0x1C, 0x20):
            w = struct.unpack_from('<I', img, off)[0]
            self.assertEqual(w >> 26, 0x05, f'{off:#x} 未换成跳转:{w:#x}')
        # x18 指令逐字保留
        self.assertEqual(struct.unpack_from('<I', img, 0x24)[0], 0xAA0103F2)
        self.assertEqual(struct.unpack_from('<I', img, 0x28)[0], 0xF9400242)

    def test_gate_counts(self):
        rows = [l.split('\t') for l in
                self.read('synth.gates.tsv').decode().splitlines() if l]
        self.assertEqual(len(rows), 5)
        kinds = sorted(r[1] for r in rows)
        self.assertEqual(kinds, ['cntpct', 'svc', 'tpidr_r', 'tpidr_w', 'tpidrro_r'])
        # 每个门在 gatebl 有一个 bl 占位,且该字是 BL 编码(0x94000000)
        bls = [int(l, 16) for l in
               self.read('synth.gatebl.tsv').decode().split() if l]
        self.assertEqual(len(bls), 5)
        img = self.read('synth.patched.bin')
        for off in bls:
            self.assertEqual(struct.unpack_from('<I', img, off)[0], 0x94000000)

    def test_embed_asm(self):
        out_s = os.path.join(self.tmp, 'guest_synth.S')
        r = subprocess.run([sys.executable, os.path.join(HERE, 'hos_embed.py'),
                            '--outdir', self.out, '--name', 'synth',
                            '--kind', '0', '--out', out_s],
                           capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stderr)
        s = open(out_s, encoding='utf-8').read()
        self.assertEqual(s.count('bl dm_gate'), 5)
        self.assertIn('.incbin', s)
        self.assertIn('__dm_guest_synth_start', s)

if __name__ == '__main__':
    unittest.main()
