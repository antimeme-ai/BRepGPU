"""Actual source -> native adaptive caller -> independent rational mesh reader.

Python is the external source/test boundary. No production geometry helpers are
used to check emitted coordinates, triangle interiors or complete patch coverage.
"""
from collections import Counter
from fractions import Fraction as F
import argparse
import math
from pathlib import Path
import struct
import subprocess

from native_mesher_run_fixture import make


class Reader:
    def __init__(self, data):
        self.data = data
        self.at = 0

    def unpack(self, fmt):
        size = struct.calcsize(fmt)
        assert self.at + size <= len(self.data), "truncated patch artifact"
        value = struct.unpack_from(fmt, self.data, self.at)
        self.at += size
        return value

    def rational(self):
        sign, nc, dc = self.unpack('<iII')
        assert sign in (-1, 0, 1) and dc and nc + dc <= (len(self.data)-self.at)//4
        nw = self.unpack(f'<{nc}I')
        dw = self.unpack(f'<{dc}I')
        assert dw[-1] and (not nw or nw[-1]), "noncanonical integer words"
        n = sum(w << (32*i) for i, w in enumerate(nw))
        d = sum(w << (32*i) for i, w in enumerate(dw))
        assert bool(n) == bool(sign) and math.gcd(n, d) == 1
        return F(sign*n, d)


def read_patch(path, kind, epsilon):
    raw = path.read_bytes()
    reader = Reader(raw)
    magic, version, policy, carrier, np, nt, digest = reader.unpack('<8sIIQQQ32s')
    assert (magic, version, policy, carrier) == (b'BGAPCH01', 1, 1, 9)
    assert digest == bytes.fromhex('ac'*32)
    assert np >= 4 and nt >= 2 and np <= len(raw)//48 and nt <= len(raw)//12
    points = []
    for _ in range(np):
        xyz = reader.unpack('<3d')
        assert all(math.isfinite(x) for x in xyz)
        uv = (reader.rational(), reader.rational())
        assert all(0 <= q <= 1 for q in uv)
        points.append((tuple(F(x) for x in xyz), uv))
    assert len({uv for _, uv in points}) == np, "duplicate patch point identity"
    triangles = [reader.unpack('<3I') for _ in range(nt)]
    assert reader.at == len(raw), "trailing artifact bytes"
    edges = Counter()
    area = F(0)
    epsilon2 = F(epsilon)**2
    # Exact arithmetic on binary64 emitted L and original polynomial/rational S.
    for ids in triangles:
        assert len(set(ids)) == 3 and max(ids) < np
        p = [points[i] for i in ids]
        uv = [q[1] for q in p]
        determinant = ((uv[1][0]-uv[0][0])*(uv[2][1]-uv[0][1])
                       -(uv[1][1]-uv[0][1])*(uv[2][0]-uv[0][0]))
        assert determinant > 0, "nonpositive exact UV triangle"
        area += determinant
        a = [p[1][0][j]-p[0][0][j] for j in range(3)]
        b = [p[2][0][j]-p[0][0][j] for j in range(3)]
        cross = (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
        assert any(cross), "physically collapsed actual facet"
        for i in range(3):
            edges[(ids[i], ids[(i+1)%3])] += 1
        for weights in ((1, 0, 0), (0, 1, 0), (0, 0, 1),
                        (F(1, 3),)*3, (F(1, 2), F(1, 4), F(1, 4)),
                        (F(1, 4), F(1, 2), F(1, 4)),
                        (F(1, 4), F(1, 4), F(1, 2))):
            u, v = (sum(weights[i]*uv[i][j] for i in range(3)) for j in range(2))
            actual = [sum(weights[i]*p[i][0][j] for i in range(3)) for j in range(3)]
            if kind == 'weighted':
                denominator = 1+14*u-14*u*u
                expected = ((8*u-7*u*u)/denominator, v, 16*u*(1-u)/denominator)
            elif kind == 'c0':
                expected = (u, v, u/2 if u <= F(1, 2) else (1-u)/2)
            else:
                expected = (u, v, u*u)
            assert sum((actual[j]-expected[j])**2 for j in range(3)) <= epsilon2, (
                "original source -> SAME actual affine facet exceeds positional allowance", kind, u, v)
    assert area == 2, "patch does not cover requested exact unit rectangle"
    boundary_length = F(0)
    for (a, b), count in edges.items():
        assert count == 1, "repeated directed facet edge"
        if (b, a) in edges:
            continue
        uv, vv = points[a][1], points[b][1]
        assert any(uv[j] == vv[j] and uv[j] in (0, 1) for j in range(2)), (
            "unmatched internal edge / hanging point", uv, vv)
        boundary_length += abs(uv[0]-vv[0])+abs(uv[1]-vv[1])
    assert boundary_length == 4
    assert set(i for tri in triangles for i in tri) == set(range(np))
    return raw


def table(path):
    lines = path.read_text().splitlines()
    assert lines
    names = lines[0].split('\t')
    return [dict(zip(names, row.split('\t'), strict=True)) for row in lines[1:]]


def invoke(executable, source, output, tolerances, threads, device_args=()):
    command = [str(executable), '--source', str(source), '--carrier', '9',
               '--output', str(output), '--threads', str(threads),
               '--memory-mib', '512', '--max-nodes', '65536', '--max-depth', '40']
    for epsilon in tolerances:
        command += ['--positional-mm', str(epsilon)]
    command += list(device_args)
    return subprocess.run(command, text=True, capture_output=True, timeout=90, check=False)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('executable', type=Path)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--cuda-module', type=Path)
    args = parser.parse_args()
    args.destination.mkdir(parents=True, exist_ok=False)
    for kind in ('polynomial', 'weighted', 'c0'):
        source = args.destination/f'{kind}.bgsource'
        make(source, kind, 12 if kind == 'polynomial' else 1)
        # Fine/coarse/fine plus equal-quality remesh tests cache history while
        # independently checking every actual output against original source.
        qualities = (.02, .08, .02, .02)
        reference = None
        modes = [('cpu1', 1, ()), ('cpu4', 4, ())]
        if args.cuda_module:
            modes += [(f'p100{device}', 4, ('--backend', 'p100', '--device', str(device),
                                           '--cuda-module', str(args.cuda_module))) for device in (0, 1)]
        for name, threads, device_args in modes:
            output = args.destination/f'{kind}-{name}'
            run = invoke(args.executable, source, output, qualities, threads, device_args)
            assert run.returncode == 0, (name, kind, run.returncode, run.stderr, run.stdout)
            meshes = [read_patch(output/f'query-{i}.bgapch', kind, q) for i, q in enumerate(qualities)]
            assert meshes[0] == meshes[2] == meshes[3], "warm history changed mesh semantics"
            if reference is None:
                reference = meshes
            else:
                assert reference == meshes, "CPU threads / P100 changed semantic output"
            rows = table(output/'queries.tsv')
            assert len(rows) == len(qualities)
            for i, row in enumerate(rows):
                assert int(row['query']) == i and float(row['positional_mm']) == qualities[i]
                assert row['numeric_profile'] == 'rn64_anchor32_p128_v1'
                assert row['source_digest'] == 'ac'*32
                assert row['backend'] == ('p100' if device_args else 'cpu')
            assert int(rows[-1]['nodes_reused']) > 0 and int(rows[-1]['point_hits']) > 0
            assert int(rows[0]['original_roots']) == (2 if kind == 'c0' else 1), (
                'source compilation included unrequested carrier roots', kind, rows[0])
            if device_args:
                assert int(rows[0]['device_points']) > 0 and int(rows[0]['device_bounds']) > 0
                if kind != 'c0':
                    assert int(rows[0]['device_restrictions']) > 0
            assert (output/'timings.tsv').is_file()
            cold = args.destination/f'{kind}-{name}-cold-coarse'
            run = invoke(args.executable, source, cold, (.08,), threads, device_args)
            assert run.returncode == 0, (name, kind, run.stderr, run.stdout)
            assert read_patch(cold/'query-0.bgapch', kind, .08) == meshes[1], (
                'a prior fine query changed a later coarse mesh', name, kind)
        # Publication must never replace an existing successful result.
        existing = args.destination/f'{kind}-cpu1'
        retained = (existing/'query-0.bgapch').read_bytes()
        refused = invoke(args.executable, source, existing, (.02,), 1)
        assert refused.returncode != 0 and (existing/'query-0.bgapch').read_bytes() == retained
        # An exhausted finite capacity cannot publish partial success.
        exhausted = args.destination/f'{kind}-exhausted'
        command = [str(args.executable), '--source', str(source), '--carrier', '9',
                   '--output', str(exhausted), '--threads', '1', '--positional-mm', '1e-9',
                   '--max-nodes', '1']
        run = subprocess.run(command, capture_output=True, text=True, timeout=30, check=False)
        assert run.returncode != 0 and not exhausted.exists()
    print('original-source adaptive caller: source accuracy, complete coverage, history and publication PASS')


if __name__ == '__main__':
    main()
