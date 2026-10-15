#!/usr/bin/env python3
"""Fresh-process complete owned through-bore construction and consumption costs."""
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
    if sys.platform!='linux':raise RuntimeError('benchmark executes on fleet')
    if not 1<=args.repeats<=7 or not 0<=args.warmups<=2:raise ValueError('invalid repetition budget')
    out=args.output_directory;out.mkdir(parents=True,exist_ok=False)
    controller_hash=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    raw=[];summary=[];environment=dict(os.environ,PYTHONPATH='python')
    for recipe in ('unit','small','large'):
        for backend,device,threads in (('cpu',0,1),('cpu',0,24),('cuda',0,1),('cuda',1,1)):
            values=[];construction=[];sampling=[];topology=[]
            for repetition in range(args.warmups+args.repeats):
                case=out/f'{recipe}-{backend}-device{device}-threads{threads}-r{repetition}'
                command=[sys.executable,'examples/cut_bore.py','--recipe',recipe,'--backend',backend,
                    '--device',str(device),'--threads',str(threads),'--grid',str(args.grid),
                    '--library-path',str(args.library_path),'--topology-library',str(args.topology_library),
                    '--output-directory',str(case)]
                start=time.perf_counter_ns()
                result=subprocess.run(command,env=environment,capture_output=True,text=True,timeout=180)
                process_ms=(time.perf_counter_ns()-start)/1e6
                if result.returncode:raise RuntimeError(f'caller failed {case.name}: {result.stdout}\n{result.stderr}')
                costs=dict(csv.reader((case/'costs.tsv').open(),delimiter='\t'))
                complete=float(costs['complete_cut_to_consumed_cleanup']);build=float(costs['cpu_difference'])
                sample=float(costs['sample_and_consume']);adj=float(costs['adjacency_build'])
                warmup=int(repetition<args.warmups)
                if not warmup:
                    values.append(complete);construction.append(build);sampling.append(sample);topology.append(adj)
                raw.append((recipe,backend,device,threads,repetition,warmup,complete,build,sample,adj,float(costs['independent_qualification']),process_ms))
                print(case.name,result.stdout.strip(),flush=True)
            summary.append((recipe,backend,device,threads,len(values),statistics.median(values),min(values),max(values),
                            statistics.median(construction),statistics.median(sampling),statistics.median(topology)))
    tsv(out/'raw.tsv',('recipe','backend','device','threads','repeat','warmup','complete_ms','cpu_difference_ms','sample_consume_ms','adjacency_ms','qualification_ms','child_process_ms'),raw)
    tsv(out/'summary.tsv',('recipe','backend','device','threads','samples','median_complete_ms','min_complete_ms','max_complete_ms','median_cpu_difference_ms','median_sample_consume_ms','median_adjacency_ms'),summary)
    if hashlib.sha256(Path(__file__).read_bytes()).hexdigest()!=controller_hash:raise RuntimeError('controller source changed')
    tsv(out/'sources.tsv',('path','sha256'),(('benchmarks/boolean/run.py',controller_hash),))
    tsv(out/'scope.tsv',('field','value'),(('useful_timer','CPU cut/material/boundary/history reads/request/context/sampling/cleanup/topology library/adjacency/actual mates and neighbors'),
        ('outside_useful_timer','operand preparation/module imports/process launch/independent qualification/artifact writes/reference experiment'),
        ('child_process_ms','includes all child work plus process launch/wait'),('grid',args.grid),('warmups',args.warmups),('repeats',args.repeats),
        ('interpretation','exploratory fresh-process small operation; no GPU Boolean construction or full CAD speedup; clocks unpinned')))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--grid',type=int,default=17)
    p.add_argument('--warmups',type=int,default=1)
    p.add_argument('--repeats',type=int,default=3)
    p.add_argument('--library-path',type=Path,default=Path('build/pair-release/libbrepgpu.so'))
    p.add_argument('--topology-library',type=Path,default=Path('build/topology-release/libbrepgpu_topology.so'))
    p.add_argument('--output-directory',type=Path,required=True)
    execute(p.parse_args())
