"""Run the actual register bridge on an Apple Silicon host (AAPCS64 x8/SIMD).

HarmonyOS builds also assemble this bridge; game creation is the device test.
"""
import pathlib
import platform
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


@unittest.skipUnless(platform.system() == 'Darwin' and platform.machine() == 'arm64',
                     'requires an Apple Silicon host')
class HostcallAbiTest(unittest.TestCase):
    def test_indirect_result_and_simd(self):
        source = (ROOT / 'entry/src/main/cpp/host/entry_hos.S').read_text()
        start = source.index('    .globl dm_guest_call_regs')
        end = source.index('    .globl dm_thread_enter', start)
        # Mach-O uses a leading underscore; the instructions are unchanged.
        bridge = '.text\n' + source[start:end].replace('dm_guest_call_regs', '_dm_guest_call_regs')
        with tempfile.TemporaryDirectory(prefix='d2r-abi-') as tmp:
            asm = pathlib.Path(tmp) / 'bridge.S'
            exe = pathlib.Path(tmp) / 'abi-test'
            asm.write_text(bridge)
            subprocess.run(['clang', '-O2', str(ROOT / 'tools/tests/hostcall_abi.c'),
                            str(asm), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    unittest.main()
