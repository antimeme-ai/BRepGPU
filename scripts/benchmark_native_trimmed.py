#!/usr/bin/env python3
"""Observe the existing complete selected-face native operation, serially.

Fixed families: plane, plane_many_holes, plane_reversed, plane_authoritative,
bowl, weighted and saddle. Each runs CPU1, CPU4, P1000 and P1001 in a fresh
process, requesting .01 then .005 mm in one retained session (28 processes,
56 queries). P100 uses one host thread. The node-source cap is 1e-10 mm;
qualified native memory/node defaults are unchanged. OS/file caches are
uncontrolled. This covers these authored selected-face families, not whole CAD
assemblies or general performance scaling.

The existing source-boundary and face readers run after native process timing.
Raw sources, artifacts and logs stay under ignored context; tracked TSVs retain
public relative paths, hashes, native stages and one observation per process.
Byte, decoded-record and UV/XYZ/oriented-connectivity comparisons are distinct.
No native process or files are produced without --run. Existing runs and data
are never overwritten. A fixed 120-second timeout records censored processes.
"""
from pathlib import Path
from fractions import Fraction as F
import argparse
import csv
import hashlib
import shlex
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests'))
from native_adaptive_trimmed_caller import (source_case, write_source, read_curves,
    check_plane_curves, read_face, qualify_face, source_value)

FAMILIES = ('plane', 'plane_many_holes', 'plane_reversed', 'plane_authoritative',
            'bowl', 'weighted', 'saddle')
MODES = (('cpu1', 'cpu', 1, None), ('cpu4', 'cpu', 4, None),
         ('p1000', 'p100', 1, 0), ('p1001', 'p100', 1, 1))
QUALITIES = (.01, .005)
NODE_CAP = 1e-10
RAW = Path('context/adaptive-trimmed-2026-10-17')
TRACKED = Path('results/adaptive-trimmed-2026-10-17')
SCOPE = 'existing_source_boundary_and_complete_selected_face_readers'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def relative(path):
    return path.resolve().relative_to(ROOT).as_posix()


def normalized(value):
    if isinstance(value, F):
        return ['fraction', str(value.numerator), str(value.denominator)]
    if isinstance(value, bytes):
        return ['bytes', value.hex()]
    if isinstance(value, dict):
        return {str(k): normalized(v) for k, v in sorted(value.items())}
    if isinstance(value, (tuple, list)):
        return [normalized(v) for v in value]
    return value


def decoded_sha(value):
    raw = repr(normalized(value)).encode('utf-8')
    return hashlib.sha256(raw).hexdigest()


def face_geometry(mesh):
    # Comparison only, not a geometric test. Existing readers already admit the
    # unique exact UV rows and actual XYZ. Remove transient row/facet IDs and
    # cyclic choice of first triangle vertex, preserving orientation/connectivity.
    indexed = sorted(enumerate(mesh['sites']), key=lambda p: (p[1]['uv'], p[1]['xyz']))
    rank = {old: new for new, (old, _) in enumerate(indexed)}
    vertices = [(p['uv'], p['xyz']) for _, p in indexed]
    triangles = []
    for facet in mesh['facets']:
        t = tuple(rank[i] for i in facet['sites'])
        triangles.append(min(t, t[1:]+t[:1], t[2:]+t[:2]))
    return (vertices, sorted(triangles), mesh['orientation'])


def write_tsv(path, rows):
    fields = list(dict.fromkeys(key for row in rows for key in row))
    temporary = path.with_suffix(path.suffix+'.pending')
    with temporary.open('w', newline='') as stream:
        writer = csv.DictWriter(stream, fields, delimiter='\t', lineterminator='\n')
        writer.writeheader()
        writer.writerows(rows)
    temporary.replace(path)


def read_tsv(path):
    with path.open(newline='') as stream:
        return list(csv.DictReader(stream, delimiter='\t'))


def decode_text(value):
    return value.decode(errors='replace') if isinstance(value, bytes) else value or ''


def main():
    parser = argparse.ArgumentParser(description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--module', type=Path, required=True)
    parser.add_argument('--revision', required=True, help='provided source revision, full 40-character hash')
    parser.add_argument('--timeout', type=float, default=120.)
    parser.add_argument('--run', action='store_true', help='execute fixed serial observations; default only describes plan')
    args = parser.parse_args()
    if not args.run:
        print('prepared: 7 families x CPU1/CPU4/P1000/P1001, two queries (.01,.005); no process run')
        return
    executable = (ROOT / args.executable).resolve()
    module = (ROOT / args.module).resolve()
    assert args.timeout == 120., 'fixed reviewed timeout is 120 seconds'
    assert re.fullmatch(r'[0-9a-f]{40}', args.revision), 'expected full committed revision'
    # Fleet is the root-managed source archive, intentionally without .git.
    # Revision is provided by the caller; actual binary/module/container hashes
    # independently identify exactly the files consumed by this observation.
    # No private paths appear in the tracked fields below.
    executable_relative, module_relative = relative(executable), relative(module)
    executable_sha, module_sha = sha(executable), sha(module)
    raw, tracked = ROOT / RAW, ROOT / TRACKED
    assert not raw.exists(), 'observation directory already exists; never overwrite a prior run'
    assert not any((tracked / name).exists() for name in ('processes.tsv','queries.tsv','stages.tsv'))
    (raw / 'sources').mkdir(parents=True)
    tracked.mkdir(parents=True, exist_ok=True)
    process_rows, query_rows, stage_rows, baselines = [], [], [], {}
    config = {'revision': args.revision, 'revision_provenance': 'provided_source_revision', 'executable': executable_relative,
        'executable_sha256': executable_sha, 'module': module_relative,
        'module_sha256': module_sha, 'families': FAMILIES, 'modes': MODES,
        'qualities': QUALITIES, 'node_source_mm': NODE_CAP, 'timeout_seconds': args.timeout,
        'scope': SCOPE, 'one_observation_per_process': True}
    write_tsv(raw/'manifest.tsv', [{'key':key, 'value':repr(value)} for key,value in config.items()])
    for family in FAMILIES:
        source, kind = source_case(family)
        source_path = raw / 'sources' / (family+'.bgsource')
        write_source(source, source_path)
        container_sha = sha(source_path)
        for label, backend, threads, device in MODES:
            process_id = family+'-'+label
            capture = raw / process_id
            capture.mkdir()
            output = capture / 'native'
            command = [str(executable), '--source', str(source_path), '--definition', '200',
                '--face', '900', '--output', str(output), '--node-source-mm', str(NODE_CAP),
                '--threads', str(threads)]
            if backend == 'p100':
                command += ['--backend','p100','--cuda-module',str(module),'--device',str(device)]
            for quality in QUALITIES:
                command += ['--positional-mm',str(quality)]
            prefix = {'process_id': process_id, 'family': family, 'mode': label,
                'backend': backend, 'threads': threads, 'device': '' if device is None else device}
            row = dict(prefix, observation=1, revision=args.revision,
                revision_provenance='provided_source_revision',
                executable=executable_relative, executable_sha256=executable_sha,
                cuda_module=module_relative if backend == 'p100' else '',
                cuda_module_sha256=module_sha if backend == 'p100' else '',
                quality_sequence_mm='0.01,0.005', node_source_mm=NODE_CAP,
                source_container_sha256=container_sha, source_metadata_digest=source.source_sha256,
                source_path=relative(source_path), artifact_path=relative(output),
                timeout_seconds=args.timeout, timing_scope='spawn_through_native_process_exit',
                reader_scope=SCOPE, reader_timing_scope='artifact_readers_and_hash_comparison_after_native_exit',
                cache_scope='new_native_process_and_session_OS_file_cache_uncontrolled')
            print('begin\t'+process_id, flush=True)
            started = time.perf_counter()
            try:
                completed = subprocess.run(command, capture_output=True, text=True,
                                           timeout=args.timeout, cwd=ROOT)
                wall = time.perf_counter()-started
                stdout, stderr, code = completed.stdout, completed.stderr, completed.returncode
                status = 'complete' if code == 0 else 'native_error'
            except subprocess.TimeoutExpired as error:
                wall = time.perf_counter()-started
                stdout, stderr, code = decode_text(error.stdout), decode_text(error.stderr), ''
                status = 'timeout_censored'
            row.update(status=status, returncode=code, process_exit_wall_seconds=f'{wall:.17g}',
                       reader_status='not_run', reader_seconds='', native_through_publication_seconds='')
            (capture/'stdout.txt').write_text(stdout)
            (capture/'stderr.txt').write_text(stderr)
            (capture/'command.txt').write_text(shlex.join(command)+'\n')
            if status != 'complete':
                # Native structured stage/code only; raw messages remain ignored.
                tokens = stderr.splitlines()[0].split('\t') if stderr else []
                if len(tokens) >= 3 and tokens[0] == 'error':
                    row['native_failure_stage'] = tokens[1]
                    row['native_failure_code'] = tokens[2]
                process_rows.append(row)
                print('end\t'+process_id+'\t'+status, flush=True)
                continue
            match = re.fullmatch(r'adaptive_trimmed_face\tcomplete\tqueries\t2\t'
                                 r'through_publication_seconds\t([^\n]+)\n?', stdout)
            assert match, 'missing native publication horizon'
            row['native_through_publication_seconds'] = match[1]
            original_rows = read_tsv(output/'queries.tsv')
            assert len(original_rows) == 2
            native_stages = read_tsv(output/'timings.tsv')
            reader_started = time.perf_counter()
            current_queries = []
            try:
                callback = None if kind == 'plane' else lambda use, uv: source_value(kind,*uv)
                for q, quality in enumerate(QUALITIES):
                    boundary_path, face_path = (output/f'query-{q}.bgacrv', output/f'query-{q}.bgatrm')
                    boundary, mesh = read_curves(boundary_path), read_face(face_path)
                    check_plane_curves(boundary, source, F(quality), F(NODE_CAP), 'plain', callback)
                    qualify_face(source, boundary, mesh, quality, NODE_CAP, kind)
                    signatures = {'boundary_artifact_sha256':sha(boundary_path),
                        'face_artifact_sha256':sha(face_path),
                        'boundary_decoded_sha256':decoded_sha(boundary),
                        'face_decoded_sha256':decoded_sha(mesh),
                        'face_geometry_topology_sha256':decoded_sha(face_geometry(mesh))}
                    identity = family,q
                    if label == 'cpu1':
                        baselines[identity] = signatures
                    baseline = baselines.get(identity)
                    comparison = {name+'_matches_cpu1': '' if baseline is None else
                                  int(value == baseline[name]) for name,value in signatures.items()}
                    native = original_rows[q]
                    assert int(native['query']) == q
                    assert native['backend'] == backend and int(native['threads']) == threads
                    assert float(native['positional_mm']) == quality
                    assert native['source_digest'] == source.source_sha256
                    current_queries.append(dict(prefix, observation=1,
                        cache_state='first_query_fresh_session' if q == 0 else 'second_query_retained_session',
                        source_container_sha256=container_sha,
                        source_metadata_digest=source.source_sha256,
                        native_query_path=relative(output/'queries.tsv'),
                        boundary_path=relative(boundary_path), face_path=relative(face_path),
                        reader_scope=SCOPE, reader_status='pass',
                        comparison_scope='full_artifact_bytes;full_decoded_records;canonical_UV_XYZ_oriented_facets',
                        **signatures, **comparison, **{'native_'+k:v for k,v in native.items()}))
            except Exception:
                row['reader_status'] = 'failed'
                row['reader_seconds'] = f'{time.perf_counter()-reader_started:.17g}'
                process_rows.append(row)
                write_tsv(raw/'processes-incomplete.tsv', process_rows)
                raise # no tracked aggregates claiming qualified data
            row['reader_status']='pass'
            row['reader_seconds']=f'{time.perf_counter()-reader_started:.17g}'
            query_rows.extend(current_queries)
            query_index = -1
            for ordinal, stage in enumerate(native_stages):
                if stage['scope'] == 'query.prepare':
                    query_index += 1
                cumulative = stage['scope'].startswith('through_')
                stage_rows.append(dict(prefix, observation=1, row=ordinal,
                    query='' if query_index < 0 or cumulative or stage['scope']=='session.close' else query_index,
                    scope_kind='cumulative_horizon' if cumulative else 'native_stage',
                    scope=stage['scope'], seconds=stage['seconds'],
                    native_timing_path=relative(output/'timings.tsv')))
            process_rows.append(row)
            write_tsv(raw/'processes-progress.tsv', process_rows)
            print('end\t'+process_id+'\tcomplete\t'+f'{wall:.9g}', flush=True)
    assert len(process_rows) == 28
    write_tsv(tracked/'processes.tsv', process_rows)
    write_tsv(tracked/'queries.tsv', query_rows)
    write_tsv(tracked/'stages.tsv', stage_rows)
    print('retained\t28 processes\t'+str(len(query_rows))+' successful queries', flush=True)


if __name__ == '__main__':
    main()
