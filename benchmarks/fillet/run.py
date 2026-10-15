#!/usr/bin/env python3
"""Fresh-process source→filleted-solid→consumed CPU/P100 costs."""
import argparse
import csv
import hashlib
import os
from pathlib import Path
import statistics
import subprocess
import sys
import time


def tsv(path,header,rows):
    with path.open('w',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(header);writer.writerows(rows)


def execute(args):
    if sys.platform!='linux':raise RuntimeError('benchmarks execute on fleet')
    if not 1<=args.repeats<=7 or not 0<=args.warmups<=2:raise ValueError('invalid repeat budget')
    out=args.output_directory;out.mkdir(parents=True,exist_ok=False)
    pin=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();raw=[];summary=[]
    environment=dict(os.environ,PYTHONPATH='python')
    for recipe in ('small','mid','large'):
        for backend,device,threads in (('cpu',0,1),('cpu',0,24),('cuda',0,1),('cuda',1,1)):
            cells=[]
            for repetition in range(args.warmups+args.repeats):
                case=out/f'{recipe}-{backend}-device{device}-threads{threads}-r{repetition}'
                command=[sys.executable,'examples/fillet_box_edge.py','--recipe',recipe,'--backend',backend,
                    '--device',str(device),'--threads',str(threads),'--grid',str(args.grid),
                    '--library-path',str(args.library_path),'--topology-library',str(args.topology_library),
                    '--output-directory',str(case)]
                started=time.perf_counter_ns();result=subprocess.run(command,env=environment,capture_output=True,text=True,timeout=180)
                process_ms=(time.perf_counter_ns()-started)/1e6
                if result.returncode:raise RuntimeError(f'caller failed {case.name}: {result.stdout}\n{result.stderr}')
                costs=dict(csv.reader((case/'costs.tsv').open(),delimiter='\t'))
                keys=('complete_source_to_consumed_cleanup','complete_factory_to_consumed_cleanup','source_creation_outside',
                      'cpu_fillet','sample_and_consume','adjacency_build','independent_qualification')
                values=tuple(float(costs[k]) for k in keys);warmup=int(repetition<args.warmups)
                raw.append((recipe,backend,device,threads,repetition,warmup,*values,process_ms))
                if not warmup:cells.append(values)
                print(case.name,result.stdout.strip(),flush=True)
            summary.append((recipe,backend,device,threads,len(cells),statistics.median(c[0] for c in cells),
                min(c[0] for c in cells),max(c[0] for c in cells),*(statistics.median(c[i] for c in cells) for i in range(1,6))))
    tsv(out/'raw.tsv',('recipe','backend','device','threads','repeat','warmup','complete_ms','factory_complete_ms','source_create_ms',
        'cpu_fillet_ms','sample_consume_ms','adjacency_ms','qualification_ms','child_process_ms'),raw)
    tsv(out/'summary.tsv',('recipe','backend','device','threads','samples','median_complete_ms','min_complete_ms','max_complete_ms',
        'median_factory_complete_ms','median_source_create_ms','median_cpu_fillet_ms','median_sample_consume_ms','median_adjacency_ms'),summary)
    if hashlib.sha256(Path(__file__).read_bytes()).hexdigest()!=pin:raise RuntimeError('controller changed')
    tsv(out/'sources.tsv',('path','sha256'),(('benchmarks/fillet/run.py',pin),))
    tsv(out/'scope.tsv',('field','value'),(('useful_timer','prepared source→CPUfeature→geometry/history/request/Engine/native consumers/cleanup/actual adjacency readers'),
        ('factory_complete','also includes source creation from already-owned Frame'),
        ('outside_useful_timer','module imports/process launch/independent qualification/artifact/SVG/reference'),
        ('child_process_ms','all child work plus process launch/wait'),('grid',args.grid),('warmups',args.warmups),('repeats',args.repeats),
        ('interpretation','exploratory unpinned freshprocess small features; no GPU construction or general CAD speedup')))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--grid',type=int,default=17);p.add_argument('--warmups',type=int,default=1);p.add_argument('--repeats',type=int,default=3)
    p.add_argument('--library-path',type=Path,default=Path('build/pair-release/libbrepgpu.so'))
    p.add_argument('--topology-library',type=Path,default=Path('build/topology-release/libbrepgpu_topology.so'))
    p.add_argument('--output-directory',type=Path,required=True);execute(p.parse_args())
