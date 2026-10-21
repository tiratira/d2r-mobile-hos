"""Prepare translated Switch shaders for HarmonyOS Vulkan (offline).

The GLSL round trip avoids Maleoon compiler/device-loss failures seen with the
translator's otherwise valid SPIR-V. Reflection bindings and constant banks are
preserved. Only 16-character shader keys are included; diagnostic files are ignored.
"""
from pathlib import Path
import argparse
import concurrent.futures
import json
import re
import shutil
import struct
import subprocess
import sys


def capabilities(data):
    words = list(struct.unpack('<%dI' % (len(data) // 4), data))
    if len(words) < 5 or words[0] != 0x07230203:
        raise ValueError('not SPIR-V')
    instructions = []
    i = 5
    while i < len(words):
        n = words[i] >> 16
        if not n or i + n > len(words):
            raise ValueError('invalid SPIR-V instruction')
        instructions.append((i, n, words[i] & 65535))
        i += n
    caps = {words[i + 1] for i, n, op in instructions if op == 17}
    ops = {op for i, n, op in instructions}
    required = set()
    if any(op == 25 and n >= 9 and words[i + 7] == 2 and words[i + 8] == 0
           for i, n, op in instructions):
        if 99 in ops:
            required.add(56)  # StorageImageWriteWithoutFormat
        if 98 in ops:
            required.add(55)  # StorageImageReadWithoutFormat
    if any(op == 25 and n >= 9 and words[i + 3] == 5 for i, n, op in instructions):
        required.add(46)  # SampledBuffer
    extra = [w for cap in sorted(required - caps) for w in (0x20011, cap)]
    return struct.pack('<%dI' % (len(words) + len(extra)), *(words[:5] + extra + words[5:]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--fallback', type=Path, help='override failed raw translations from this directory')
    parser.add_argument('--sdk-bin', '--spirv-tools', dest='bin', type=Path, required=True,
                        help='Vulkan SDK Bin directory containing spirv-opt, spirv-val, spirv-cross and glslangValidator')
    parser.add_argument('--no-normalize', action='store_true')
    parser.add_argument('--jobs', type=int, default=8)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    if args.source.resolve() == args.output.resolve():
        parser.error('output must differ from source')

    def tool(name):
        path = args.bin / name
        if not path.exists():
            path = path.with_suffix('.exe')
        if not path.exists():
            parser.error('missing tool: ' + str(path))
        return str(path)

    opt, val = tool('spirv-opt'), tool('spirv-val')
    cross, glslang = (tool('spirv-cross'), tool('glslangValidator')) if not args.no_normalize else (None, None)
    sources = {p.name: p for p in args.source.glob('*.spv') if re.fullmatch(r'[0-9a-f]{16}\.spv', p.name)}
    if args.fallback:
        sources.update({p.name: p for p in args.fallback.glob('*.spv') if re.fullmatch(r'[0-9a-f]{16}\.spv', p.name)})

    def run(command):
        result = subprocess.run(command, capture_output=True, text=True, encoding='utf-8', errors='replace')
        return result.returncode, (result.stdout + result.stderr).strip()

    def one(source):
        target = args.output / source.name
        temporary = target.with_suffix('.tmp.spv')
        metadata = source.with_suffix('.res.json')
        if not metadata.exists():
            return source.name, {'error': 'missing resource reflection'}
        temporary.write_bytes(capabilities(source.read_bytes()))
        code, message = run([opt, '--skip-validation', '--trim-capabilities', '--eliminate-dead-variables',
                             '--eliminate-dead-functions', str(temporary), '-o', str(target)])
        temporary.unlink()
        if code:
            return source.name, {'error': message}
        shutil.copyfile(metadata, target.with_suffix('.res.json'))
        fallback = ''
        if cross:
            stage = json.loads(metadata.read_text(encoding='utf-8'))['stage']
            extension = {'vs': 'vert', 'ps': 'frag', 'cs': 'comp', 'gs': 'geom', 'hs': 'tesc', 'ds': 'tese'}[stage]
            glsl = target.with_suffix('.' + extension)
            code, message = run([cross, str(target), '--vulkan-semantics', '--output', str(glsl)])
            if not code:
                code, message = run([glslang, '-V', '--target-env', 'vulkan1.1', str(glsl), '-o', str(temporary)])
            if code:
                fallback = message
            else:
                temporary.replace(target)
                glsl.unlink()
        code, message = run([val, '--target-env', 'vulkan1.1', str(target)])
        return source.name, {'error': message} if code else {'normalization_fallback': fallback} if fallback else {}

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = dict(pool.map(one, sources.values()))
    report = {key: value for key, value in results.items() if value}
    (args.output / 'validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    constants = args.source / 'constants.bin'
    if constants.exists():
        shutil.copyfile(constants, args.output / constants.name)
    errors = sum('error' in value for value in results.values())
    print('prepared', len(results), 'valid', len(results) - errors, 'errors', errors,
          'normalization fallbacks', sum('normalization_fallback' in v for v in results.values()))
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
