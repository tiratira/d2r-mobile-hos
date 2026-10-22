"""Run the real replay texture operations against a recording Vulkan sink.

The BC decoder and Tegra swizzler are real; only driver entry points are mocked.
No phone or Vulkan device is required.
"""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
CPP = ROOT / 'entry/src/main/cpp'


class TextureCopyTest(unittest.TestCase):
    def test_replay_texture_transfers(self):
        compiler = shutil.which('clang++') or shutil.which('c++')
        cc = shutil.which('clang') or shutil.which('cc')
        headers = next((p for p in [
            pathlib.Path('/usr/include'),
            pathlib.Path('/Applications/DevEco-Studio.app/Contents/sdk/default/openharmony/native/sysroot/usr/include'),
        ] if (p / 'vulkan/vulkan.h').exists()), None)
        if not compiler or not cc or not headers:
            self.skipTest('requires C/C++ compilers and Vulkan headers')
        source = (CPP / 'gfx/dm_vk_replay.cpp').read_text()

        def extract(start, end):
            a = source.index(start)
            return source[a:source.index(end, a)]

        actual = '\n'.join([
            extract('VkFormat format(int f)', 'void imageBarrier('),
            extract('VkImageView textureView(', 'Texture *texid('),
            extract('void clearTexture(', 'bool copyGpuResult('),
            extract('extern "C" void mtl_read_texels(', 'static void replayCleanup('),
        ])
        fixture = (ROOT / 'tools/tests/texture_copy.cpp').read_text()
        with tempfile.TemporaryDirectory(prefix='d2r-texture-copy-') as tmp:
            folder = pathlib.Path(tmp)
            # Do not put a cross-compilation sysroot ahead of the host C headers.
            shutil.copytree(headers / 'vulkan', folder / 'vulkan')
            if (headers / 'vk_video').exists():
                shutil.copytree(headers / 'vk_video', folder / 'vk_video')
            objects = []
            for name in ['nvn_tex', 'nvn_deswizzle', 'nvn_bcdec']:
                obj = folder / (name + '.o')
                subprocess.run([cc, '-std=c11', '-O1', '-g', '-fsanitize=address,undefined',
                                '-I', str(CPP / 'gfx/tex'), '-c', str(CPP / 'gfx/tex' / (name + '.c')),
                                '-o', str(obj)], check=True)
                objects.append(str(obj))
            test = folder / 'main.cpp'
            test.write_text(fixture.replace('// ACTUAL_REPLAY_CODE', actual))
            exe = folder / 'texture-copy-test'
            subprocess.run([compiler, '-std=c++17', '-O1', '-g', '-fsanitize=address,undefined',
                            '-Wno-c11-extensions', '-I', str(CPP), '-I', str(folder),
                            str(test), *objects, '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    unittest.main()
