"""Read actual adaptive geometry outputs against independent source formulas."""
from fractions import Fraction as F
from functools import lru_cache
from pathlib import Path
import argparse
import subprocess
from native_mesher_point_trig_vectors import original_oracle,plus,times
original_oracle=lru_cache(maxsize=None)(original_oracle)


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--fixture',type=Path,required=True)
    p.add_argument('--source',type=Path,required=True)
    p.add_argument('--analytic-fixtures',type=Path)
    p.add_argument('--cuda-module',type=Path)
    p.add_argument('--device',type=int,default=0)
    a=p.parse_args()
    backend=[] if a.cuda_module is None else ["--cuda",str(a.cuda_module),str(a.device)]
    completed=subprocess.run([str(a.fixture),str(a.source),*backend],check=True,capture_output=True,text=True)
    counts={}
    for line in completed.stdout.splitlines():
        fields=line.split('\t');kind=fields[0]
        if kind=='adaptive-original-geometry':
            assert fields[1]=='PASS'
            continue
        counts[kind]=counts.get(kind,0)+1
        values=[F(float.fromhex(x)) for x in fields[1:] if x!='end']
        if kind=='point':
            u,v=values[:2]
            expected=(u,2*v/(1+v),2*u*v/(1+v))
            for j,truth in enumerate(expected):
                assert values[2+2*j]<=truth<=values[3+2*j],('source point',u,v,j)
            actual=values[8:11];departure=values[11]
            assert departure>=0 and sum((actual[j]-expected[j])**2 for j in range(3))<=departure**2
        elif kind=='precise':
            expected=(F(1,3),F(4,5),F(4,15))
            assert all(values[2*j]<=truth<=values[2*j+1] for j,truth in enumerate(expected))
        elif kind in ('leaf','leaf-child'):
            upper=values[0];corners=[values[1+i*3:4+i*3] for i in range(4)]
            for i in range(17):
                for j in range(17):
                    u,v=F(i,16),F(j,16)
                    original_v=v/2 if kind=="leaf-child" else v
                    expected=(u,2*original_v/(1+original_v),2*u*original_v/(1+original_v))
                    weights=((1-u)*(1-v),u*(1-v),(1-u)*v,u*v)
                    actual=[sum(weights[k]*corners[k][axis] for k in range(4)) for axis in range(3)]
                    assert sum((expected[k]-actual[k])**2 for k in range(3))<=upper**2
        elif kind=='trig':
            x=values[0];truth=original_oracle(x)
            assert values[1]<=truth[0]<=truth[1]<=values[2],('sin',x)
            assert values[3]<=truth[2]<=truth[3]<=values[4],('cos',x)
        elif kind=='tiny':
            assert values[0]<=F(1,1<<1075)<=values[1]
        elif kind=='twist-facet':
            shift,upper=values
            assert F(1,4)+shift<=upper<F(251,1000)+shift
            for i in range(33):
                for j in range(i+1):
                    u,v=F(i,32),F(j,32)
                    assert abs(u*v-v-shift)<=upper
        else:
            raise AssertionError(kind)
    assert counts=={'point':81,'precise':1,'leaf':1,'leaf-child':1,'trig':129,'tiny':1,'twist-facet':2},counts

    if a.analytic_fixtures:
        frame=(3.,-2.,4.,2.,0.,1.,.5,1.,0.,.25,.5,3.)
        def point(x):return (x,x)
        def scale(x,r):return times(x,point(r))
        def position(kind,u,v):
            factor=1. if kind<=5 else 1e200 if kind==6 else 1e-200
            f=tuple(F(x*factor) for x in frame)
            actual_kind=kind if kind<=5 else 1
            if actual_kind==1:
                return tuple(point(f[k]+u*f[3+k]+v*f[6+k]) for k in range(3))
            su0,su1,cu0,cu1=original_oracle(u)
            e=tuple(plus(scale((cu0,cu1),f[3+k]),scale((su0,su1),f[6+k])) for k in range(3))
            if actual_kind==2:radial,axial=point(F(2)),point(v)
            elif actual_kind==3:
                sa0,sa1,ca0,ca1=original_oracle(F(1,4))
                radial,axial=plus(point(F(2)),scale((sa0,sa1),v)),scale((ca0,ca1),v)
            else:
                sv0,sv1,cv0,cv1=original_oracle(v)
                minor=F(2) if actual_kind==4 else F(1,4)
                radial=scale((cv0,cv1),minor)
                if actual_kind==5:radial=plus(point(F(2)),radial)
                axial=scale((sv0,sv1),minor)
            return tuple(plus(point(f[k]),plus(times(radial,e[k]),scale(axial,f[9+k]))) for k in range(3))
        for kind in range(1,8):
            response=subprocess.run([str(a.fixture),str(a.analytic_fixtures/f'analytic-{kind}.bgsource'),str(kind),*backend],check=True,capture_output=True,text=True)
            rows=0
            for line in response.stdout.splitlines():
                f=line.split('\t');assert int(f[1])==kind
                q=[F(float.fromhex(x)) for x in f[2:]]
                if f[0]=='analytic':
                    rows+=1;u,v=q[:2];truth=position(kind,u,v)
                    for k in range(3):assert q[2+2*k]<=truth[k][0]<=truth[k][1]<=q[3+2*k],('analytic truth',kind,u,v,k)
                    actual=q[8:11];departure=q[11]
                    assert sum(max(abs(truth[k][0]-actual[k]),abs(truth[k][1]-actual[k]))**2 for k in range(3))<=departure**2
                else:
                    assert f[0]=='analytic-leaf';upper=q[0];corners=[q[1+3*k:4+3*k] for k in range(4)]
                    for i in range(9):
                        for j in range(9):
                            u,v=F(i,8),F(j,8);truth=position(kind,u,v)
                            weights=((1-u)*(1-v),u*(1-v),(1-u)*v,u*v)
                            actual=[sum(weights[k]*corners[k][axis] for k in range(4)) for axis in range(3)]
                            assert sum(max(abs(truth[k][0]-actual[k]),abs(truth[k][1]-actual[k]))**2 for k in range(3))<=upper**2,('analytic leaf',kind,u,v)
            assert rows==25
        fallback=subprocess.run([str(a.fixture),str(a.analytic_fixtures/'fallback-weights.bgsource'),'100'],check=True,capture_output=True,text=True)
        assert fallback.stdout.strip()=='selected-root-fallback\tPASS' 
        for kind,name in ((101,'u'),(102,'v'),(103,'elevated')):
            response=subprocess.run([str(a.fixture),str(a.analytic_fixtures/f'extrusion-{name}.bgsource'),str(kind),*backend],check=True,capture_output=True,text=True)
            row=response.stdout.strip().split('\t')
            assert row[0]=='extrusion-leaf' and int(row[1])==kind
            q=[F(float.fromhex(x)) for x in row[2:]]
            upper=q[0];corners=[q[1+3*k:4+3*k] for k in range(4)]
            for i in range(17):
                for j in range(17):
                    u,v=F(i,16),F(j,16)
                    t=v if kind==102 else u;linear=u if kind==102 else v
                    w=1+14*t-14*t*t
                    truth=((8*t-7*t*t)/w,linear,16*t*(1-t)/w)
                    weights=((1-u)*(1-v),u*(1-v),(1-u)*v,u*v)
                    actual=[sum(weights[k]*corners[k][axis] for k in range(4)) for axis in range(3)]
                    assert sum((actual[k]-truth[k])**2 for k in range(3))<=upper**2,('extrusion residual',kind,u,v)

    print('adaptive-independent-source\tPASS\t'+str(sum(counts.values()))+'\tanalytic-families\t'+str(7 if a.analytic_fixtures else 0))

if __name__=='__main__':
    main()
