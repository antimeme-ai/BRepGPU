"""Source-backed local-domain requests, with the existing exact chain oracle."""
from dataclasses import replace
from fractions import Fraction as F
from pathlib import Path
from types import SimpleNamespace
import argparse
import subprocess
import tempfile
from native_adaptive_curves_caller import source, Curve, Edge, Coedge, Loop, Vertex, ZERO_FRAME, write_source
from face_domain_oracle import qualify_chain
from brepgpu.imported_oracles import _simple_domains


def tangent_source(inner=F(1), outer=F(2), a=F(-1,2), b=F(-1,4), reflected=False):
    base = source('plain')
    nets = []
    for r, t0, t1 in ((inner,a,F(0)), (outer,F(0),b)):
        nets.append(tuple((-2*r*x*y, r*(x+y), 1+x*y)
                          for x,y in ((t0,t0),(t0,t1),(t1,t1))))
    points = [(n[0][0]/n[0][2], n[0][1]/n[0][2]) for n in nets]
    end = (nets[1][-1][0]/nets[1][-1][2],nets[1][-1][1]/nets[1][-1][2])
    points.append(end)
    # Rigid reflection plus exact translation preserves the source graph while
    # reversing its orientation; ordinary loop adaptation must retain the word.
    def transform(x,y,w):
        return ((-x if reflected else x)+2*w,y+3*w,w)
    nets = [tuple(transform(*h) for h in net) for net in nets]
    nets.append((transform(*end,F(1)),transform(*points[0],F(1))))
    curves=[]
    for i,net in enumerate(nets):
        curves.extend((Curve(2000+i,3,4,ZERO_FRAME,(0.,0.),(0.,1.),len(net)-1,
                    controls=tuple(float(v) for h in net for v in (h[0],h[1],0,h[2]))),
            Curve(3000+i,2,4,ZERO_FRAME,(0.,0.),(0.,1.),len(net)-1,
                    controls=tuple(float(v) for h in net for v in h))))
    transformed=[transform(*p,F(1)) for p in points]
    vertices=tuple(Vertex(300+i,(float(p[0]),float(p[1]),0.),0.)
                   for i,p in enumerate(transformed))
    edges=tuple(Edge(400+i,(300+i,300+(i+1)%3),2000+i,(0.,1.),True,True,False,0.)
                for i in range(3))
    uses=tuple(Coedge(500+i,400+i,800,1,3000+i,(0.,1.),reconstructed=True) for i in range(3))
    return replace(base,vertices=vertices,edges=edges,coedges=uses,curves=tuple(curves),
        loops=(Loop(800,900,True,(500,501,502)),),
        definitions=(replace(base.definitions[0],edge_ids=(400,401,402),vertex_ids=(300,301,302)),),
        carriers=(replace(base.carriers[0],uv_bounds=(-4.,4.,-4.,4.)),))


def curved_line_source():
    base=tangent_source()
    c=F(353,500)
    nets=(((F(1),F(0),F(1)),(F(1),F(1),F(1)),(F(0),F(2),F(2))),
          ((F(0),F(1),F(1)),(c,c,F(1))),
          ((c,c,F(1)),(F(1),F(0),F(1))))
    curves=[]
    for i,net in enumerate(nets):
        curves.extend((Curve(2000+i,3,4,ZERO_FRAME,(0.,0.),(0.,1.),len(net)-1,
                    controls=tuple(float(v) for h in net for v in (h[0],h[1],0,h[2]))),
            Curve(3000+i,2,4,ZERO_FRAME,(0.,0.),(0.,1.),len(net)-1,
                    controls=tuple(float(v) for h in net for v in h))))
    vertices=tuple(Vertex(300+i,(*map(float,p),0.),0.)
                   for i,p in enumerate(((1,0),(0,1),(c,c))))
    return replace(base,vertices=vertices,curves=tuple(curves))


def straight_crossing():
    base=source('plain'); points=((0.,0.),(1.,1.),(0.,1.),(1.,0.))
    curves=[]
    for i,a in enumerate(points):
        b=points[(i+1)%4]
        curves.extend((Curve(2000+i,3,4,ZERO_FRAME,(0.,0.),(0.,1.),1,
                             controls=(*a,0.,1.,*b,0.,1.)),
                       Curve(3000+i,2,4,ZERO_FRAME,(0.,0.),(0.,1.),1,
                             controls=(*a,1.,*b,1.))))
    return replace(base,curves=tuple(curves),vertices=tuple(
        Vertex(300+i,(*p,0.),0.) for i,p in enumerate(points)))


def check(output):
    case=SimpleNamespace(points={},loops=[],triangles=[])
    for line in output.splitlines():
        row=line.split('\t')
        if row[0]=='case': assert row[1]=='recovery'
        elif row[0]=='point': case.points[int(row[1])]=tuple(map(F,row[2:4]))
        elif row[0]=='loop': case.loops.append(tuple(map(int,row[1:])))
        elif row[0]=='triangle': case.triangles.append(tuple(map(int,row[1:4])))
        else: raise AssertionError(row)
    _simple_domains(tuple(tuple(case.points[i] for i in loop) for loop in case.loops),(True,))
    qualify_chain(case,case.triangles,required_sites=case.points,positive_cycles=1)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--fixture',required=True,type=Path)
    parser.add_argument('--one',action='store_true')
    parser.add_argument('--source',type=Path)
    parser.add_argument('--expected-refined-edge',type=int)
    args=parser.parse_args()
    variants=((F(1),F(2),F(-1,2),F(-1,4),False),
              (F(1),F(5,4),F(-1,2),F(-3,4),False),
              (F(1),F(4),F(-3,4),F(-1,4),True))
    if args.source:
        for initial in (0,1,2):
            run=subprocess.run([str(args.fixture),'--domain-recovery',str(args.source),str(initial)],
                               capture_output=True,text=True,timeout=120)
            print(run.stderr,end='')
            if run.returncode: raise RuntimeError(f'actual source schedule {initial}: exit {run.returncode}')
            requests=[line for line in run.stderr.splitlines() if line.startswith('domain_request')]
            assert requests, 'actual crossing witness was not exercised'
            if args.expected_refined_edge is not None:
                assert all(f'edge={args.expected_refined_edge}' in line for line in requests)
            check(run.stdout)
        print('actual_source_domain_local_recovery PASS')
        return
    with tempfile.TemporaryDirectory(prefix='native-domain-recovery-') as directory:
        for i,variant in enumerate(variants[:1] if args.one else variants):
            path=Path(directory)/f'{i}.bgsource';write_source(tangent_source(*variant),path)
            run=subprocess.run([str(args.fixture),'--domain-recovery',str(path)],
                               capture_output=True,text=True,timeout=120)
            print(run.stderr,end='')
            if run.returncode: raise RuntimeError(f'domain variant {i}: exit {run.returncode}')
            check(run.stdout)
        path=Path(directory)/'curved-line.bgsource';write_source(curved_line_source(),path)
        run=subprocess.run([str(args.fixture),'--domain-recovery',str(path)],
                           capture_output=True,text=True,timeout=120)
        print(run.stderr,end='')
        if run.returncode: raise RuntimeError('curved versus straight source refused')
        requests=[line for line in run.stderr.splitlines() if line.startswith('domain_request')]
        assert all('edge=400' in line for line in requests)
        check(run.stdout)
        path=Path(directory)/'straight-crossing.bgsource';write_source(straight_crossing(),path)
        run=subprocess.run([str(args.fixture),'--domain-recovery',str(path)],
                           capture_output=True,text=True,timeout=120)
        print(run.stderr,end='')
        assert run.returncode != 0 and not run.stdout
        assert ('selected face graph lacks local source refinement evidence' in run.stderr or
                'shared curve split depth exhausted' in run.stderr)
    print('source_domain_local_recovery PASS')
if __name__=='__main__':main()
