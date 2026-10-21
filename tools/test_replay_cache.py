"""Exercise the replay cache's actual range invalidation without a GPU."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class ReplayCacheTest(unittest.TestCase):
    def test_overlapping_writes(self):
        compiler = shutil.which('clang++') or shutil.which('c++')
        if not compiler:
            self.skipTest('requires a C++17 compiler')
        source = (ROOT / 'entry/src/main/cpp/gfx/dm_vk_replay.cpp').read_text()
        start = source.index('std::map<std::pair<uint64_t, uint64_t>, Slice> copies;')
        end = source.index('Slice zeroSlice', start)
        test = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <map>
#include <tuple>
#include "gfx/nvn/index_range.h"
struct Slice {};
''' + source[start:end] + r'''
int main() {
    uint8_t unaligned[32]{};
    uint16_t indices16[]{3, 65535, 18, 2};
    memcpy(unaligned + 1, indices16, sizeof indices16);
    assert(nvnIndexLimit(unaligned + 1, 4, 1) == 19);
    uint32_t indices32[]{5, UINT32_MAX, 100000, 2};
    memcpy(unaligned + 1, indices32, sizeof indices32);
    assert(nvnIndexLimit(unaligned + 1, 4, 2) == 100001);
    uint8_t indices8[]{5, 255, 2};
    assert(nvnIndexLimit(indices8, 3, 0) == 256);
    assert(nvnIndexLimit(indices8, 0, 0) == 0);
    copies[{100, 20}] = {};
    copies[{120, 20}] = {};
    copies[{1000, 20}] = {};
    indexRanges[{100, 10, 1}] = 9; // 20 bytes
    indexRanges[{1000, 10, 2}] = 9; // 40 bytes
    invalidateCopies(120, 1);
    assert(copies.count({100, 20}) && !copies.count({120, 20}));
    assert(indexRanges.count({100, 10, 1})); // touching boundary is disjoint
    invalidateCopies(110, 1);
    assert(!copies.count({100, 20}) && !indexRanges.count({100, 10, 1}));
    assert(copies.count({1000, 20}) && indexRanges.count({1000, 10, 2}));
    invalidateCopies(1025, 1); // overlaps 32-bit indices, not the shorter slice
    assert(copies.count({1000, 20}) && indexRanges.empty());
    invalidateCopies(1000, 0);
    assert(copies.size() == 1);
    copies[{1, 64ull << 20}] = {};
    invalidateCopies(64ull << 20, 1); // last byte of maximum-sized slice
    assert(!copies.count({1, 64ull << 20}));
    invalidateCopies(UINT64_MAX - 1, 4); // malformed range clears conservatively
    assert(copies.empty() && indexRanges.empty());
}
'''
        with tempfile.TemporaryDirectory(prefix='d2r-cache-') as tmp:
            cpp = pathlib.Path(tmp) / 'cache.cpp'
            exe = pathlib.Path(tmp) / 'cache-test'
            cpp.write_text(test)
            subprocess.run([compiler, '-std=c++17', '-O2',
                            '-I', str(ROOT / 'entry/src/main/cpp'),
                            str(cpp), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    unittest.main()
