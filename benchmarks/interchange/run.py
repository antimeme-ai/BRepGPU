#!/usr/bin/env python3
"""Fresh-process complete input-to-consumed-carrier measurements; exploratory."""
import argparse
import csv
import hashlib
import os
from pathlib import Path
import statistics
import subprocess
import sys
import time


def tsv(path,fields,rows):
    with path.open('w',newline='') as f:
        w=csv.writer(f,delimiter='\t',lineterminator='\n');w.writerow(fields);w.writerows(rows)


def run(args):
    if sys.platform!='linux':raise RuntimeError('benchmarks execute on fleet')
    if not 1<=args.repeats<=7 or not 0<=args.warmups<=2:raise ValueError('invalid repetition budget')
    out=args.output_directory;out.mkdir(parents=True,exist_ok=False)
    controller_hash=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    rows=[];summary=[]
    environment=dict(os.environ,PYTHONPATH='python')
    placements=(('cpu',0,1),('cpu',0,24),('cuda',0,1),('cuda',1,1))
    for backend,device,threads in placements:
        complete=[]
        for repetition in range(args.warmups+args.repeats):
            case=out/f'{backend}-device{device}-threads{threads}-r{repetition}'
            command=[sys.executable,'examples/import_carriers.py','--input',str(args.input),'--backend',backend,
                '--device',str(device),'--threads',str(threads),'--grid',str(args.grid),'--library-path',str(args.library_path),
                '--output-directory',str(case)]
            started=time.perf_counter_ns()
            result=subprocess.run(command,env=environment,capture_output=True,text=True,timeout=180)
            process_ms=(time.perf_counter_ns()-started)/1e6
            if result.returncode:raise RuntimeError(f'caller failed {case.name}: {result.stdout}\n{result.stderr}')
            costs=dict(csv.reader((case/'costs.tsv').open(),delimiter='\t'))
            value=float(costs['complete_file_to_consumed_cleanup'])
            if repetition>=args.warmups:complete.append(value)
            rows.append((backend,device,threads,repetition,int(repetition<args.warmups),value,float(costs['independent_qualification']),process_ms))
            print(case.name,result.stdout.strip(),flush=True)
        summary.append((backend,device,threads,len(complete),statistics.median(complete),min(complete),max(complete)))
    tsv(out/'raw.tsv',('backend','device','threads','repeat','warmup','complete_ms','qualification_ms','child_process_ms'),rows)
    tsv(out/'summary.tsv',('backend','device','threads','samples','median_complete_ms','min_complete_ms','max_complete_ms'),summary)
    if hashlib.sha256(Path(__file__).read_bytes()).hexdigest()!=controller_hash:raise RuntimeError('campaign controller source changed')
    tsv(out/'sources.tsv',('path','sha256'),(('benchmarks/interchange/run.py',controller_hash),))
    tsv(out/'scope.tsv',('field','value'),(('useful_caller','capped read/parse/convert/query prep/context/uploads/eval/download/required validation/source conversion/checksum/cleanup'),
        ('outside_useful_timer','module import/process launch, exact independent qualification, artifact writes, reference acquisition and scalar queries'),
        ('child_process_ms','includes all child work plus launch and wait; reported separately'),('grid',args.grid),('warmups',args.warmups),('repeats',args.repeats),
        ('interpretation','exploratory unpinned whole input-consumer latency; no clock isolation; no general CAD speedup')))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input',type=Path,required=True)
    p.add_argument('--grid',type=int,default=17)
    p.add_argument('--warmups',type=int,default=1)
    p.add_argument('--repeats',type=int,default=3)
    p.add_argument('--library-path',type=Path,default=Path('build/pair-release/libbrepgpu.so'))
    p.add_argument('--output-directory',type=Path,required=True)
    run(p.parse_args())
