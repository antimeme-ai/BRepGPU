"""Read actual directed-region construction with the shared exact chain oracle."""
import argparse
from collections import defaultdict
from fractions import Fraction
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
from face_domain_oracle import qualify_chain
from native_adaptive_trimmed_caller import curved_source, write_source


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--fixture',required=True,type=Path)
    parser.add_argument('--source',type=Path)
    parser.add_argument('--storage-only',action='store_true')
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='adaptive-regions-') as directory:
        path=args.source or Path(directory)/'saddle-hole.bgsource'
        if args.source is None:
            write_source(curved_source('saddle',(((Fraction(251,512),Fraction(251,512)),
                (Fraction(251,512),Fraction(253,512)),(Fraction(253,512),Fraction(253,512)),
                (Fraction(253,512),Fraction(251,512))),)),path)
        process=subprocess.run([str(args.fixture),'--storage-only' if args.storage_only else str(path)],
                               capture_output=True,text=True,timeout=60)
        if process.returncode:
            raise RuntimeError(f'actual region fixture exit {process.returncode}: {process.stderr}')
        output=process.stdout
        if args.storage_only:
            print(process.stderr,end='')
    cases=[]
    for line in output.splitlines():
        row=line.split('\t')
        if row[0]=='case':
            case=SimpleNamespace(name=row[1],points={},loops=[],triangles=[],components=[])
            cases.append(case)
        elif row[0]=='point': case.points[int(row[1])]=tuple(map(Fraction,row[2:4]))
        elif row[0]=='loop': case.loops.append(tuple(map(int,row[1:])))
        elif row[0]=='triangle':
            case.triangles.append(tuple(map(int,row[1:4])))
            case.components.append(int(row[4]))
        else: raise AssertionError('unknown region fixture row')
    assert {case.name for case in cases}==({'storage'} if args.storage_only else
        {'rational','anchor','disconnected','nested','face'})
    for case in cases:
        positives=sum(sum(case.points[a][0]*case.points[b][1]-case.points[a][1]*case.points[b][0]
                          for a,b in zip(loop,loop[1:]+loop[:1]))>0 for loop in case.loops)
        _,edges=qualify_chain(case,case.triangles,required_sites=case.points,
                             positive_cycles=positives)
        # Actual dual connectivity must match labels, including nested bands.
        incident=defaultdict(list)
        for i,t in enumerate(case.triangles):
            for a,b in zip(t,t[1:]+t[:1]): incident[min(a,b),max(a,b)].append(i)
        neighbors=defaultdict(set)
        for members in incident.values():
            if len(members)==2:
                a,b=members; neighbors[a].add(b); neighbors[b].add(a)
        unseen=set(range(len(case.triangles))); groups=[]
        while unseen:
            pending=[min(unseen)]; group=set()
            while pending:
                i=pending.pop()
                if i in group: continue
                group.add(i); pending.extend(neighbors[i]-group)
            unseen-=group; groups.append(group)
        assert len(groups)==positives
        labels=[]
        for group in groups:
            values={case.components[i] for i in group}; assert len(values)==1
            labels.extend(values)
        assert len(set(labels))==len(groups)
    print(f'directed_region_exact_chains={len(cases)} PASS')

if __name__=='__main__': main()
