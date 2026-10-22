"""Exercise the real input layer with a recording HID sink."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class InputChordTest(unittest.TestCase):
    def test_atomic_chord_preserves_touch_buttons_and_stick(self):
        compiler = shutil.which('clang') or shutil.which('cc')
        if not compiler:
            self.skipTest('requires a C compiler')
        with tempfile.TemporaryDirectory(prefix='d2r-input-chord-') as tmp:
            folder = pathlib.Path(tmp)
            (folder / 'host').mkdir()
            (folder / 'host/dm.h').write_text('''
#include <stdint.h>
#define LOG(...) ((void)0)
void dmHidSet(uint64_t, int32_t, int32_t, int32_t, int32_t);
''')
            main = folder / 'main.c'
            main.write_text('''
#include <assert.h>
#include "input/dm_input.h"
static uint64_t buttons;
static int32_t x, y;
static int calls;
void dmHidSet(uint64_t b, int32_t lx, int32_t ly, int32_t rx, int32_t ry) {
    buttons=b; x=lx; y=ly; calls++; assert(rx==0 && ry==0);
}
int main(void) {
    uint32_t chord=(1u<<8)|(1u<<11);
    dmInputInit();
    dmInputSetButton(0,1);
    dmInputSetStick(0.5f,-0.25f);
    int before=calls;
    dmInputSetChord(chord,1);
    assert(calls==before+1 && buttons==(chord|1));
    assert(x==16383 && y==8191);
    dmInputSetButton(0,0); assert(buttons==chord);
    dmInputSetStick(0,0); assert(buttons==chord && x==0 && y==0);
    // Releasing one input owner must preserve another owner's held ZL.
    dmInputSetButton(8,1);
    dmInputSetChord(chord,0); assert(buttons==(1u<<8));
    dmInputSetChord(chord,1);
    dmInputSetButton(8,0); assert(buttons==chord);
    dmInputSetChord(chord,0); assert(buttons==0);
    dmInputSetButton(0,1); dmInputSetChord(chord,1);
    dmInputInit(); dmInputSetStick(0,0); assert(buttons==0);
    before=calls;
    dmInputSetButton(-1,1); dmInputSetButton(32,1); assert(calls==before);
    return 0;
}
''')
            exe = folder / 'input-test'
            subprocess.run([compiler, '-std=c11', '-O2', '-pthread', '-I', str(folder),
                            '-I', str(ROOT / 'entry/src/main/cpp'),
                            str(ROOT / 'entry/src/main/cpp/input/dm_input.c'),
                            str(main), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    unittest.main()
