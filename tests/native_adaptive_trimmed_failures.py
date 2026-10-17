"""Actual selected-face caller refusals and atomic output cleanup.

Run on the fleet with the current strict native executable. Existing direct
service tests own cancellation/stale state; this script tests process boundaries.
Original fixtures use the existing writer. No successful mesh is fabricated.
"""
from dataclasses import replace
from pathlib import Path
import argparse
import csv
import shlex
import struct
import subprocess
import tempfile
import time

from native_adaptive_trimmed_caller import source_case
from native_adaptive_curves_caller import write_source
from brepgpu.imported_source import Product, SolidDefinition


def corrupt_map(raw):
    """Inject one unknown enum using the owned source wire's literal field layout."""
    result = bytearray(raw)
    at = 88
    for _ in range(struct.unpack_from('<I', result, 72)[0]):
        tag, count, size = struct.unpack_from('<8sQQ', result, at)
        at += 24
        if tag == b'COEDGES_':
            assert count > 0
            # row length u64, then4IDs,sense,reconstructed,t2,periodic_shift2.
            struct.pack_into('<I', result, at + 8 + 64, 99)
            return result
        at += size
    raise AssertionError('existing source writer omitted coedges')


def inventory(path):
    return tuple((str(item.relative_to(path)),
                  'directory' if item.is_dir() else item.read_bytes())
                 for item in sorted(path.rglob('*')))


def run(executable, workspace, selected):
    workspace.mkdir(parents=True, exist_ok=False)
    plane, _ = source_case('plane')
    wrong_owner = replace(plane,
        products=(*plane.products, Product(101, 'other', 'other', 201)),
        definitions=(*plane.definitions, SolidDefinition(201, 101, (), (), ())))
    mismatch = replace(plane, vertices=tuple(
        replace(v, xyz=(v.xyz[0], v.xyz[1], .125)) if v.id == 300 else v
        for v in plane.vertices))
    unavailable = replace(plane, coedges=tuple(
        replace(u, parameter_map_kind=3, parameter_map=((0., 0.), (1., 1.)))
        if u.id == 500 else u for u in plane.coedges))
    large = replace(plane, adapter_revision='source memory refusal ' + 'x' * (2 << 20))
    saddle, _ = source_case('saddle')
    # stage,ErrorCode(or exact),specific source-defined diagnostic.
    cases = (
        ('missing_definition', plane, ('--definition', '999'), None,
         ('source.decode', '0', 'unknown source ID 999')),
        ('wrong_definition_owner', wrong_owner, ('--definition', '201'), None,
         ('source.decode', '0', 'selected original face does not belong to definition')),
        ('missing_face', plane, ('--face', '999'), None,
         ('source.decode', '0', 'unknown source ID 999')),
        ('source_node', mismatch, (), None,
         ('query.construct_shared_boundary', '3', 'original endpoint source-node cap exceeded')),
        ('truncated', plane, (), lambda raw: raw[:-1],
         ('source.decode', '0', 'source section count/file length mismatch')),
        ('unknown_wire_map', plane, (), corrupt_map,
         ('source.decode', '0', 'unknown source parameter map kind')),
        ('unavailable_map', unavailable, (), None,
         ('source.compile_original_curves', '1', 'unimplemented original use parameter map')),
        ('memory', large, ('--memory-mib', '1'), None,
         ('source.decode', '2', 'run memory reservation exhausted')),
        ('depth', saddle, ('--max-depth', '1'), None,
         ('query.construct_complete_face', '3', 'adaptive requested depth exhausted')),
        ('work', plane, ('--construction-work', '1'), None,
         ('query.construct_complete_face', 'exact', 'exact work budget exhausted')),
        ('existing_target', plane, (), None,
         ('options', '6', 'output already exists; publication never replaces it')),
    )
    if selected:
        unknown = set(selected) - {case[0] for case in cases}
        if unknown:
            raise ValueError(f'unknown failure cases: {sorted(unknown)}')
        cases = tuple(case for case in cases if case[0] in selected)
    with (workspace / 'failures.tsv').open('w', newline='') as stream:
        rows = csv.writer(stream, delimiter='\t')
        rows.writerow(('case', 'status', 'stage', 'code', 'seconds'))
        for name, source, extra, mutation, expected in cases:
            case = workspace / name
            case.mkdir()
            source_path, target = case / 'original.bgsource', case / 'output'
            write_source(source, source_path)
            if mutation:
                source_path.write_bytes(mutation(source_path.read_bytes()))
            before = None
            if name == 'existing_target':
                (target / 'nested').mkdir(parents=True)
                (target / 'nested' / 'sentinel').write_bytes(b'existing result\x00unchanged\xff')
                before = inventory(target)
            command = [str(executable), '--source', str(source_path),
                '--definition', '200', '--face', '900', '--output', str(target),
                '--positional-mm', '.001' if name == 'depth' else '.01',
                '--node-source-mm', '1e-8', '--threads', '1', *extra]
            (case / 'command.txt').write_text(shlex.join(command) + '\n')
            start = time.perf_counter()
            result = subprocess.run(command, capture_output=True, text=True, timeout=30)
            elapsed = time.perf_counter() - start
            (case / 'stdout.txt').write_text(result.stdout)
            (case / 'stderr.txt').write_text(result.stderr)
            diagnostic = [line.split('\t', 3) for line in result.stderr.splitlines()
                          if line.startswith('error\t')]
            assert result.returncode == 1, (name, result.returncode, result.stderr)
            assert len(diagnostic) == 1 and len(diagnostic[0]) == 4, (name, result.stderr)
            _, stage, code, message = diagnostic[0]
            assert (stage, code) == expected[:2] and expected[2] in message, (
                name, expected, diagnostic[0])
            assert 'adaptive_trimmed_face\tcomplete' not in result.stdout, name
            assert not tuple(case.glob('output.tmp-*')), (name, 'temporary output remains')
            if before is None:
                assert not target.exists(), (name, 'partial output published')
            else:
                assert target.is_dir() and inventory(target) == before, (
                    name, 'existing output changed')
            rows.writerow((name, 'PASS', stage, code, f'{elapsed:.9f}'))
            stream.flush()
            print(f'{name}\tPASS\t{stage}\t{code}', flush=True)
    print(f'caller refusals PASS\t{len(cases)}', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--workspace', type=Path,
                        help='optional new retained directory; existing directory is never reused')
    parser.add_argument('--case', action='append',
                        help='run only named cases after an affected fix; default runs all')
    options = parser.parse_args()
    executable = options.executable.resolve(strict=True)
    if options.workspace:
        run(executable, options.workspace.resolve(), options.case)
    else:
        with tempfile.TemporaryDirectory(prefix='adaptive-trimmed-failures-') as temporary:
            run(executable, Path(temporary) / 'cases', options.case)


if __name__ == '__main__':
    main()
