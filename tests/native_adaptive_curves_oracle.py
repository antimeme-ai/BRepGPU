"""Read actual CPU/P100 numerical output against original source coefficients.

Raw Cox basis and exact polynomial interpolation here are independent of the
producer's Cox-in-Bernstein extraction and GPU restriction implementation.
"""
from decimal import Decimal,localcontext
from fractions import Fraction as F
from functools import lru_cache
from math import comb
from pathlib import Path
import subprocess,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'python'))
from brepgpu.imported_source import load_source

def dec(x):
 x=F(x);return Decimal(x.numerator)/Decimal(x.denominator)
def norm(v):return sum(dec(x)**2 for x in v).sqrt()
def trig(t):
 t=dec(t);s=t;c=Decimal(1);a=t;b=Decimal(1)
 for i in range(1,100):
  a*=-t*t/Decimal((2*i)*(2*i+1));b*=-t*t/Decimal((2*i-1)*(2*i));s+=a;c+=b
 return s,c
def homogeneous(c,t):
 p=c.degree;stride=c.dimension+1;controls=[list(map(F,c.controls[i:i+stride])) for i in range(0,len(c.controls),stride)]
 if c.kind==4:
  row=controls
  for _ in range(p):row=[[a*(1-t)+b*t for a,b in zip(x,y)] for x,y in zip(row,row[1:])]
  return row[0]
 knots=tuple(F(k) for k,m in zip(c.knots,c.multiplicities) for _ in range(m))
 @lru_cache(None)
 def basis(i,d):
  if d==0:return F(knots[i]<t<=knots[i+1]) if t==knots[-p-1] else F(knots[i]<=t<knots[i+1])
  out=F(0)
  if knots[i+d]!=knots[i]:out+=(t-knots[i])/(knots[i+d]-knots[i])*basis(i,d-1)
  if knots[i+d+1]!=knots[i+1]:out+=(knots[i+d+1]-t)/(knots[i+d+1]-knots[i+1])*basis(i+1,d-1)
  return out
 return [sum(basis(i,p)*row[a] for i,row in enumerate(controls)) for a in range(stride)]
def value(c,t):
 if c.kind>=4:
  h=homogeneous(c,t);return [dec(x/h[-1]) for x in h[:-1]]+[Decimal(0)]*(3-c.dimension)
 o=c.frame[:3];x=c.frame[3:6];y=c.frame[6:9]
 if c.kind==1:return [dec(a)+dec(t)*dec(b) for a,b in zip(o,x)]
 s,co=trig(t);rx,ry=c.radii
 return [dec(a)+dec(rx)*dec(b)*co+dec(ry)*dec(d)*s for a,b,d in zip(o,x,y)]
def interpolate(values):
 n=len(values)-1;out=[F(0)]*(n+1)
 for i,v in enumerate(values):
  poly=[F(1)];den=F(1);xi=F(i,n) if n else F(0)
  for j in range(n+1):
   if i==j:continue
   xj=F(j,n);den*=xi-xj;new=[F(0)]*(len(poly)+1)
   for k,a in enumerate(poly):new[k]-=xj*a;new[k+1]+=a
   poly=new
  for k,a in enumerate(poly):out[k]+=v*a/den
 return [sum(out[k]*F(comb(i,k),comb(n,k)) for k in range(i+1)) for i in range(n+1)]
def restricted(c,a,b):
 p=c.degree
 return list(zip(*(interpolate([homogeneous(c,a+(b-a)*F(i,p))[j] for i in range(p+1)]) for j in range(c.dimension+1)))) if p else [homogeneous(c,a)]
def hullbound(h,q):
 p=len(h)-1;d=p+1;wmin=min(row[-1] for row in h);upper=Decimal(0)
 for i in range(d+1):
  r=[]
  for axis in range(3):
   n=F(0);wl=F(0)
   for j in (i-1,i):
    if 0<=j<=p:
     weight=F(comb(p,j),comb(d,i))
     n+=weight*(h[j][axis] if axis<len(h[j])-1 else 0)
     wl+=weight*h[j][-1]*q[(i-j)*3+axis]
   r.append((n-wl)/wmin)
  upper=max(upper,norm(r))
 return upper

def check(source,output):
 curves={c.id:c for c in source.curves};count=0
 for line in output.splitlines():
  fields=line.split('\t');kind=fields[0]
  if kind=='result':assert fields[1]=='PASS';continue
  ident=int(fields[1]);xs=[F(float(x)) for x in fields[2:]]
  if kind=='sample':
   t=xs[0];truth=value(curves[ident],t);ivs=xs[1:7];q=xs[7:10];dep=dec(xs[10])
   for a in range(3):assert dec(ivs[2*a])<=truth[a]<=dec(ivs[2*a+1]),(kind,ident,t,a,truth[a],ivs)
   assert sum((truth[a]-dec(q[a]))**2 for a in range(3)).sqrt()<=dep,(kind,ident,'departure')
   if ident==6000 and q[0]!=F(-1,8):assert dep<Decimal('1e-10'),(kind,ident,'degree32 interval dependency',dep)
  elif kind=='chord':
   a,b=xs[:2];q=xs[2:8];upper=dec(xs[8]);c=curves[ident]
   if c.kind>=4:
    independent=hullbound(restricted(c,a,b),q)
    assert independent<=upper,(kind,ident,a,b,independent,upper)
   else:
    v0=value(c,a);v1=value(c,b);departure=max(sum((v0[i]-dec(q[i]))**2 for i in range(3)).sqrt(),sum((v1[i]-dec(q[3+i]))**2 for i in range(3)).sqrt())
    if c.kind==1:independent=departure
    else:
     # Exact 2-column Gram eigenvalue, not producer's outward row-sum bound.
     x=[dec(v)*dec(c.radii[0]) for v in c.frame[3:6]];y=[dec(v)*dec(c.radii[1]) for v in c.frame[6:9]]
     aa=sum(v*v for v in x);bb=sum(v*v for v in y);ab=sum(v*w for v,w in zip(x,y));m2=((aa+bb+((aa-bb)**2+4*ab*ab).sqrt())/2).sqrt()
     independent=m2*dec(b-a)**2/8+departure
    assert independent<=upper,(kind,ident,independent,upper)
  elif kind=='raw':
   a,b,upper=xs;c=curves[ident];h=restricted(c,1-a,1-b)
   q=[base+delta*t for t in (a,b) for base,delta in ((F(-1,4),F(5,2)),(F(1,8),F(5,8)),(F(0),F(0)))]
   assert hullbound(h,q)<=dec(upper),(kind,ident,hullbound(h,q),dec(upper))
  elif kind=='trace':
   a,b,upper=xs;h=[]
   for j in range(3):
    t=a+(b-a)*F(j,2);u=v=F(1,8)+F(3,4)*t
    h.append([u*(1+v),2*v,2*u*v,1+v] if ident==9 else [u,v,u*v,F(1)])
   h=list(zip(*(interpolate([row[j] for row in h]) for j in range(4))))
   q=[base+delta*t for t in (a,b) for base,delta in ((F(-1,4),F(3,2)),(F(1,8),F(5,8)),(F(0),F(1)))]
   assert hullbound(h,q)<=dec(upper),(kind,ident,hullbound(h,q),dec(upper))
  else:raise AssertionError(line)
  count+=1
 assert count>=70,count
 return count

def main():
 executable,sourcefile,*extra=sys.argv[1:];source=load_source(sourcefile)
 with localcontext() as ctx:
  ctx.prec=100
  run=subprocess.run([executable,sourcefile,*extra],text=True,capture_output=True,timeout=60)
  if run.returncode:raise RuntimeError(run.stderr)
  n=check(source,run.stdout)
 print(f'original-curve-reader\tPASS\t{n}\t'+('cpu' if not extra else 'gpu'+extra[-1]))
if __name__=='__main__':main()
