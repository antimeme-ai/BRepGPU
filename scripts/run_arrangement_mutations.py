#!/usr/bin/env python3
"""Focused assertion-sensitive arrangement mutants; Linux fleet execution only."""
import argparse
import csv
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
PRODUCT = Path('python/brepgpu/arrangements.py')
# Target tests are supplied by the qualified test unit after its interface settles.
SITES = (
    ('hole-omitted', 'tuple(holes[ordinal])', '()'),
    ('wrong-twin', 'twin=2*eid+1', 'twin=2*eid'),
    ('next-self-loop', 'next=word[(index+1)%len(word)]', 'next=word[index]'),
    ('boundary-parameter-reversal', 'source = ("boundary", coedge, original, t0, t1)',
     'source = ("boundary", coedge, original, t1, t0)'),
    ('wrong-circle-uv-side', '(piece.left_map if side == 0 else piece.right_map)',
     '(piece.right_map if side == 0 else piece.left_map)'),
    ('cut-crossing-omitted-cell', '(at-1, at) if at < len(axis) and axis[at] == value',
     '(at,) if at < len(axis) and axis[at] == value'),
    ('tangent-disks-admitted', ')) <= (circle.radius+previous.radius)**2:',
     ')) < (circle.radius+previous.radius)**2:'),
    ('use-orientation-lost', 'uses=tuple(Use(**draft) for draft in drafts)',
     'uses=tuple(Use(**dict(draft, orientation=1)) for draft in drafts)'),
)
PROTECTED = ('build/topology-release/libbrepgpu_topology.so',
             'build/pair-release/libbrepgpu.so', 'build/pair-release/libbrepgpu_cuda.so',
             'build/discovery-release/libbrepgpu.so', 'build/discovery-release/libbrepgpu_cuda.so')


def hashes():
    return tuple((name, hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in PROTECTED)


def main(args):
    if sys.platform != 'linux':
        raise RuntimeError('mutants run only on the fleet')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if args.output.exists():
        raise FileExistsError(args.output)
    targets = {}
    with args.targets.open() as stream:
        for row in csv.DictReader(stream, delimiter='\t'):
            targets[row['mutant']] = row['test']
    before = hashes()
    private = ROOT/'context'/'arrangements'/('mutants-'+str(time.time_ns()))
    private.mkdir(parents=True)
    rows = []
    for name, old, new in SITES:
        tree = private/name
        shutil.copytree(ROOT/'python', tree/'python', ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
        shutil.copytree(ROOT/'tests', tree/'tests', ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
        path = tree/PRODUCT
        source = path.read_text()
        if source.count(old) != 1:
            raise RuntimeError('mutation site does not resolve uniquely: '+name)
        env = dict(os.environ, PYTHONPATH=str(tree/'python')+os.pathsep+str(tree/'tests'),
                   PYTHONDONTWRITEBYTECODE='1')
        command = [sys.executable, '-m', 'unittest', '-v', targets[name]]
        for phase, contents in (('baseline',source), ('mutant',source.replace(old,new,1)), ('retry',source)):
            path.write_text(contents)
            started = time.perf_counter()
            run = subprocess.run(command, cwd=tree, env=env, text=True,
                                 stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
            elapsed = time.perf_counter()-started
            (tree/(phase+'.log')).write_text(run.stdout)
            if phase == 'mutant':
                if (run.returncode != 1 or 'FAIL:' not in run.stdout or
                        'AssertionError' not in run.stdout or 'ERROR:' in run.stdout or
                        not re.search(r'^FAILED \(failures=[1-9][0-9]*\)$', run.stdout, re.MULTILINE)):
                    print(run.stdout[-3500:], flush=True)
                    raise AssertionError('mutation lacks a direct geometry assertion kill: '+name)
                outcome = 'ASSERTION_KILLED'
            else:
                if run.returncode:
                    print(run.stdout[-3500:], flush=True)
                    raise AssertionError('healthy arrangement test failed: '+name+' '+phase)
                outcome = 'PASS'
            rows.append((name,phase,targets[name],elapsed,run.returncode,outcome))
        print(name, 'assertion killed; healthy retry', flush=True)
    if hashes() != before:
        raise AssertionError('protected native library changed')
    with args.output.open('w',newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(('mutant','phase','test','process_seconds','exit_code','result'))
        writer.writerows(rows)
    with args.output.with_name('protected-binaries.tsv').open('w',newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(('path','before_sha256','after_sha256'))
        writer.writerows((name,sha,sha) for name,sha in before)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--targets', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    main(parser.parse_args())
