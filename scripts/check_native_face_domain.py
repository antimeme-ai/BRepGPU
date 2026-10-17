#!/usr/bin/env python3
"""Scoped Linux native face-domain build, direct tests and actual-source analysis."""
import argparse
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
COMPILER = Path('/usr/bin/clang++-18')
PROFILES = {'debug': ('Debug', 'none'), 'release': ('Release', 'none'),
            'asan': ('Debug', 'address'), 'tsan': ('Debug', 'thread'),
            'fuzz': ('Debug', 'fuzz')}


def run(command, **kwargs):
    print(shlex.join(map(str, command)), flush=True)
    return subprocess.run(tuple(map(str, command)), check=True, **kwargs)


def tools():
    if sys.platform != 'linux':
        raise RuntimeError('Native face-domain execution belongs on the Linux fleet')
    versions = ((COMPILER, '18.1.3'), ('/usr/bin/clang-tidy-18', '18.1.3'),
                ('/usr/bin/clang-format-18', '18.1.3'),
                ('/usr/bin/cmake', '3.28.3'), ('/usr/bin/ninja', '1.11.1'))
    for tool, version in versions:
        result = run((tool, '--version'), capture_output=True, text=True)
        if not re.search(r'(?<![\d.])' + re.escape(version) + r'(?![\d.])', result.stdout):
            raise RuntimeError(f'Native tool version differs: {tool}')
        print(result.stdout.splitlines()[0], flush=True)
    macros = run((COMPILER, '-stdlib=libstdc++', '-std=c++20', '-dM', '-E', '-x',
                  'c++', '-include', 'version', '/dev/null'), capture_output=True, text=True)
    if '#define _GLIBCXX_RELEASE 13\n' not in macros.stdout:
        raise RuntimeError('Native CPU profile requires libstdc++13')
    checks = run(('/usr/bin/clang-tidy-18', '--list-checks',
                  '--config-file='+str(ROOT/'tooling/native-cpu.clang-tidy')),
                 capture_output=True, text=True)
    enabled = {line.strip() for line in checks.stdout.splitlines()}
    required = {'bugprone-use-after-move', 'bugprone-unchecked-optional-access',
                'bugprone-easily-swappable-parameters',
                'performance-unnecessary-copy-initialization',
                'performance-unnecessary-value-param'}
    if not required <= enabled or not any(x.startswith('clang-analyzer-') for x in enabled):
        raise RuntimeError('Required native static checks unavailable')


def lint(build, unit):
    modules = [ROOT / ('native/mesher' if unit == 'native-mesher' else 'native/face_domain')]
    if unit == 'direct-cdt':
        modules.append(ROOT / 'native/direct_cdt')
    if any(p.is_symlink() for module in modules for p in module.rglob('*')):
        raise RuntimeError('Owned native source symlinks require an explicit source policy')
    cpp = sorted(p for module in modules for p in module.rglob('*')
                 if p.suffix in ('.cpp', '.hpp', '.h'))
    if unit == 'native-mesher':
        cpp += sorted((ROOT/'tests').glob('native_mesher*.cpp'))
        cpp += [ROOT/'tests/exact_test.cpp', ROOT/'native/face_domain/exact.hpp']
    else:
        cpp += sorted((ROOT/'tests').glob('native_face_domain*.cpp'))
    if unit == 'direct-cdt':
        cpp += sorted((ROOT/'tests').glob('native_direct_cdt*.cpp'))
    cpp += [ROOT/'tooling/native_cpu_profile.hpp']
    if not cpp:
        raise RuntimeError('No owned native face-domain sources to analyze')
    run(('/usr/bin/clang-format-18', '--dry-run', '--Werror',
         '--style=file:'+str(ROOT/'tooling/native-cpu.clang-format'), *cpp))
    database = json.loads((build/'compile_commands.json').read_text())
    source_set = {p.resolve() for p in cpp if p.suffix == '.cpp'}
    units = [entry for entry in database if Path(entry['file']).resolve() in source_set]
    if {Path(e['file']).resolve() for e in units} != source_set:
        raise RuntimeError('Owned native source missing from actual compilation database')
    reached = set()
    for entry in units:
        args = entry.get('arguments') or shlex.split(entry['command'])
        if args[0] != str(COMPILER):
            raise RuntimeError('Wrong compiler in actual native compilation database')
        flags = []
        i = 1
        while i < len(args):
            if args[i] in ('-o', '-MT', '-MF', '-MQ'):
                i += 2
                continue
            if args[i] not in ('-c', '-MD', '-MMD'):
                flags.append(args[i])
            i += 1
        with tempfile.TemporaryDirectory(prefix='bgfd-deps-') as directory:
            dep = Path(directory)/'owned.d'
            run((COMPILER, *flags, '-MM', '-MT', 'bgfd-deps', '-MF', dep),
                cwd=entry['directory'])
            text = dep.read_text().replace('\\\n', '')
            if not text.startswith('bgfd-deps:'):
                raise RuntimeError('Unexpected native dependency report')
            reached.update((Path(entry['directory'])/p).resolve()
                           for p in shlex.split(text.split(':', 1)[1]))
        run(('/usr/bin/clang-tidy-18', '-p', build,
             '--config-file='+str(ROOT/'tooling/native-cpu.clang-tidy'), entry['file']),
            cwd=entry['directory'])
    orphaned = [p for p in cpp if p.suffix in ('.hpp', '.h') and p.resolve() not in reached]
    if orphaned:
        raise RuntimeError(f'Owned native headers have no actual analysis unit: {orphaned}')


def main(args):
    tools()
    if args.doctor:
        return
    build = (args.build_dir or ROOT/f'build/{args.unit}-{args.profile}').resolve()
    mode, instrumentation = PROFILES[args.profile]
    module = {'direct-cdt': 'direct_cdt', 'face-domain': 'face_domain',
              'native-mesher': 'mesher'}[args.unit]
    run(('/usr/bin/cmake', '-S', ROOT/'native'/module, '-B', build, '-G', 'Ninja',
         '-DCMAKE_CXX_COMPILER='+str(COMPILER), '-DCMAKE_MAKE_PROGRAM=/usr/bin/ninja',
         '-DCMAKE_BUILD_TYPE='+mode, '-DBREPGPU_NATIVE_PROFILE=linux-clang18',
         '-DBREPGPU_NATIVE_INSTRUMENTATION='+instrumentation))
    run(('/usr/bin/cmake', '--build', build, '--parallel', '4'))
    if args.lint:
        lint(build, args.unit)
    if args.build_only:
        return
    if args.profile == 'fuzz':
        raise RuntimeError('Select the actual owning fuzz target and its oracle explicitly')
    run(('/usr/bin/ctest', '--test-dir', build, '--output-on-failure', '--no-tests=error'))
    if args.unit == 'native-mesher':
        if args.tests:
            raise RuntimeError('Select native mesher CTest fixtures; no Python production host')
        return
    library = 'libbrepgpu_direct_cdt.so' if args.unit == 'direct-cdt' else 'libbrepgpu_face_domain.so'
    environment = dict(os.environ, CUDA_VISIBLE_DEVICES='',
                       PYTHONPATH=os.pathsep.join((str(ROOT/'python'), str(ROOT/'tests'), str(ROOT/'examples'))),
                       BREPGPU_FACE_DOMAIN_LIBRARY=str(build/library))
    if args.unit == 'direct-cdt':
        environment['BREPGPU_DIRECT_CDT_LIBRARY'] = str(build/library)
    # ASan shared-library tests must be hosted by a correctly instrumented process;
    # the caller must select a native fixture or an explicit matching ASan host.
    if args.profile in ('asan', 'tsan') and args.tests:
        raise RuntimeError('Instrumented Python host is not selected; use native CTest fixtures')
    if args.tests:
        run((sys.executable, '-B', '-m', 'unittest', '-v', *args.tests),
            cwd=ROOT, env=environment)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--unit', choices=('face-domain', 'direct-cdt', 'native-mesher'), default='face-domain')
    parser.add_argument('--profile', choices=tuple(PROFILES), default='debug')
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--tests', nargs='*', default=[])
    parser.add_argument('--lint', action='store_true')
    parser.add_argument('--build-only', action='store_true')
    parser.add_argument('--doctor', action='store_true')
    try:
        main(parser.parse_args())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f'Native face-domain check failed: {error}', file=sys.stderr)
        sys.exit(1)
