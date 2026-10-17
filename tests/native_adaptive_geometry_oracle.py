"""Read actual adaptive geometry outputs against independent source formulas."""
from fractions import Fraction as F
from functools import lru_cache
from pathlib import Path
import argparse
import subprocess
from math import comb,isqrt
from collections import defaultdict,Counter


def trimmed_check(source_path, output, require_cancellation=False):
    """Original Cox values + exact nodal interpolation, not tensor blossom."""
    import sys
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'python'))
    from brepgpu.imported_source import load_source
    original=load_source(source_path)
    carrier=next(c for c in original.carriers if c.id==9)

    def sqrt_upper(square):
        if not square:return F(0)
        bits=256+max(0,(square.denominator.bit_length()-square.numerator.bit_length()+1)//2)
        n=isqrt((square.numerator<<(2*bits))//square.denominator)
        if F(n*n,1<<(2*bits))<square:n+=1
        return F(n,1<<bits)

    def analytic_position(u,v):
        frame=tuple(F(x) for x in carrier.frame)
        if carrier.kind==1:
            return tuple((frame[a]+u*frame[3+a]+v*frame[6+a],)*2 for a in range(3))
        su0,su1,cu0,cu1=original_oracle(u)
        e=[plus(times((cu0,cu1),(frame[3+a],)*2),times((su0,su1),(frame[6+a],)*2)) for a in range(3)]
        if carrier.kind==2:radial,axial=(F(carrier.radii[0]),)*2,(v,v)
        elif carrier.kind==3:
            sa0,sa1,ca0,ca1=original_oracle(F(carrier.radii[1]))
            radial=plus((F(carrier.radii[0]),)*2,times((v,v),(sa0,sa1)))
            axial=times((v,v),(ca0,ca1))
        else:
            sv0,sv1,cv0,cv1=original_oracle(v)
            radius=F(carrier.radii[0] if carrier.kind==4 else carrier.radii[1])
            radial=times((radius,radius),(cv0,cv1))
            if carrier.kind==5:radial=plus((F(carrier.radii[0]),)*2,radial)
            axial=times((radius,radius),(sv0,sv1))
        return tuple(plus((frame[a],)*2,plus(times(radial,e[a]),times(axial,(frame[9+a],)*2))) for a in range(3))

    def prove_analytic(uv,actual,upper,depth=0):
        departure=F(0)
        for point,q in zip(uv,actual):
            truth=analytic_position(*point)
            square=sum(max(abs(truth[a][0]-q[a]),abs(truth[a][1]-q[a]))**2 for a in range(3))
            departure=max(departure,sqrt_upper(square))
        hu=max(q[0] for q in uv)-min(q[0] for q in uv)
        hv=max(q[1] for q in uv)-min(q[1] for q in uv)
        a=sqrt_upper(sum(F(x)**2 for x in carrier.frame[3:9]))
        f=sqrt_upper(sum(F(x)**2 for x in carrier.frame[3:12]))
        r=abs(F(carrier.radii[0]));muu=muv=mvv=F(0)
        if carrier.kind==2:muu=r*a
        elif carrier.kind==3:
            slo,shi,_,_=original_oracle(F(carrier.radii[1]));s=max(abs(slo),abs(shi))
            radius=r+max(abs(q[1]) for q in uv)*s
            muu,muv=radius*a,s*a
        elif carrier.kind==4:muu,muv,mvv=r*a,r*a,r*f
        elif carrier.kind==5:
            minor=abs(F(carrier.radii[1]));muu,muv,mvv=(r+minor)*a,minor*a,minor*f
        bound=(muu*hu**2+2*muv*hu*hv+mvv*hv**2)/8+departure
        if bound<=upper:return
        assert depth<12,('independent analytic whole-triangle bound',carrier.kind,bound,upper)
        miduv=[tuple((uv[i][a]+uv[(i+1)%3][a])/2 for a in range(2)) for i in range(3)]
        midq=[tuple((actual[i][a]+actual[(i+1)%3][a])/2 for a in range(3)) for i in range(3)]
        for i in range(3):
            prove_analytic([uv[i],miduv[i],miduv[(i+2)%3]],
                           [actual[i],midq[i],midq[(i+2)%3]],upper,depth+1)
        prove_analytic(miduv,midq,upper,depth+1)

    def basis(knots, mult, degree, count, t, hint):
        knots=tuple(F(k) for k,m in zip(knots,mult) for _ in range(m))
        @lru_cache(None)
        def b(i,p):
            if p==0:
                return F(knots[i]<t<=knots[i+1]) if t==knots[count] or hint<t else F(knots[i]<=t<knots[i+1])
            out=F(0)
            if knots[i+p]!=knots[i]:out+=(t-knots[i])/(knots[i+p]-knots[i])*b(i,p-1)
            if knots[i+p+1]!=knots[i+1]:out+=(knots[i+p+1]-t)/(knots[i+p+1]-knots[i+1])*b(i+1,p-1)
            return out
        return tuple(b(i,degree) for i in range(count))

    def h(u,v,hint):
        p,q=carrier.degrees;nu,nv=carrier.control_shape
        bu=basis(carrier.knot_u,carrier.mult_u,p,nu,u,hint[0])
        bv=basis(carrier.knot_v,carrier.mult_v,q,nv,v,hint[1])
        return tuple(sum(bu[i]*bv[j]*F(carrier.controls[(i*nv+j)*4+a])
                         for i in range(nu) for j in range(nv)) for a in range(4))

    count=0;precise_count=0;parents={};chains=defaultdict(lambda:defaultdict(Counter));piece_counts=Counter();cancellation=False
    def field(x):
        if x.startswith('r:'):
            n,d=x[2:].split('/');return F(int(n,16),int(d,16))
        return F(float.fromhex(x))
    def determinant(a,b,c):
        return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    def add_chain(parent,uv,weight):
        for a,b in zip(uv,uv[1:]+uv[:1]):
            dx,dy=b[0]-a[0],b[1]-a[1]
            assert dx or dy
            if dx:
                slope=dy/dx;line=('x',slope,a[1]-slope*a[0]);lo,hi=a[0],b[0]
            else:line=('y',a[0]);lo,hi=a[1],b[1]
            if lo>hi:lo,hi=hi,lo;sign=-weight
            else:sign=weight
            chains[parent][line][lo]+=sign;chains[parent][line][hi]-=sign
    for line in output.splitlines():
        fields=line.split('\t');kind=fields[0]
        if kind=='trim-work-parent':
            ident=int(fields[1]);assert ident not in parents
            values=list(map(field,fields[2:]));uv=list(zip(values[:6:2],values[1:6:2]))
            actual=[values[6+i*3:9+i*3] for i in range(3)]
            assert len(values)==15 and determinant(*uv)>0
            parents[ident]=(uv,actual);add_chain(ident,uv,-1);continue
        if kind=='trim-work-piece':
            parent=int(fields[1]);int(fields[2]);upper=field(fields[3]);values=list(map(field,fields[4:]))
            uv=list(zip(values[:6:2],values[1:6:2]));actual=[values[6+i*3:9+i*3] for i in range(3)]
            assert len(values)==51 and parent in parents and determinant(*uv)>0
            packed=values[15:]
            pu,pq=parents[parent];area=determinant(*pu)
            for vertex,(p,q) in enumerate(zip(uv,actual)):
                weights=[determinant(pu[1],pu[2],p)/area,determinant(pu[2],pu[0],p)/area,
                         determinant(pu[0],pu[1],p)/area]
                assert min(weights)>=0 and sum(weights)==1
                assert all(q[a]==sum(weights[i]*pq[i][a] for i in range(3)) for a in range(3)), 'virtual point changed SAME actual L'
                for axis in range(3):
                    terms=[weights[i]*pq[i][axis] for i in range(3)]
                    truth=sum(terms)
                    cancellation=cancellation or min(terms)<0<max(terms)
                    for route in range(2):
                        at=route*18+vertex*6+axis*2
                        assert packed[at]<=truth<=packed[at+1],('packed SAME-L interval misses original parent affine map',route,parent,vertex,axis)
            add_chain(parent,uv,1);piece_counts[parent]+=1
            xs=None
        else:xs=list(map(field,fields[1:]))
        if kind=='trim-line-image':
            direction,*values=xs
            curve=next(c for c in original.curves if c.id==3000)
            a=tuple(F(x)/F(curve.controls[2]) for x in curve.controls[:2])
            b=tuple(F(x)/F(curve.controls[-1]) for x in curve.controls[-3:-1])
            if direction<0:a,b=b,a
            cov=tuple(b[i]-a[i] for i in range(2))
            scalars=tuple(sum(cov[i]*p[i] for i in range(2)) for p in (a,b))
            assert values==[*a,*b,*cov,*scalars] and scalars[1]>scalars[0]
            continue
        if kind=='trim-precise-plane':
            tau,certified,upper,*values=xs
            assert carrier.kind==1 and tau>=0 and certified in (0,1)
            uv=list(zip(values[:6:2],values[1:6:2]));actual=[values[6+i*3:9+i*3] for i in range(3)]
            maximum=max(sum((analytic_position(*uv[i])[a][0]-actual[i][a])**2 for a in range(3)) for i in range(3))
            assert bool(certified)==(maximum<=tau*tau) and maximum<=upper*upper
            precise_count+=1;count+=1;continue
        if kind=='trim-skinny-facet':
            shift,upper=xs
            assert F(1,256)-shift<=upper<F(1,250)-shift, (kind,xs)
            count+=1;continue
        if kind!='trim-work-piece':
            assert kind=='trim-source-triangle',kind
            shift,upper,*values=xs
            uv=list(zip(values[:6:2],values[1:6:2]));actual=[values[6+i*3:9+i*3] for i in range(3)]
        if carrier.kind!=6:
            prove_analytic(uv,actual,upper)
            count+=1;continue
        degree=sum(carrier.degrees)+1
        powers=[(degree-i-j,i,j) for i in range(degree+1) for j in range(degree+1-i)]
        interior=tuple(sum(p[a] for p in uv)/3 for a in range(2))
        matrix=[]
        for exponents in powers:
            weights=tuple(F(k,degree) for k in exponents)
            u,v=(sum(weights[i]*uv[i][a] for i in range(3)) for a in range(2))
            n=h(u,v,interior);linear=[sum(weights[i]*actual[i][a] for i in range(3)) for a in range(3)]
            row=[F(comb(degree,e[0])*comb(degree-e[0],e[1]))*
                 weights[0]**e[0]*weights[1]**e[1]*weights[2]**e[2] for e in powers]
            row += [n[a]-n[3]*linear[a] for a in range(3)]+[n[3]]
            matrix.append(row)
        size=len(matrix)
        for column in range(size):
            pivot=next(i for i in range(column,size) if matrix[i][column])
            matrix[column],matrix[pivot]=matrix[pivot],matrix[column]
            divisor=matrix[column][column]
            matrix[column]=[x/divisor for x in matrix[column]]
            for i in range(size):
                if i==column:continue
                factor=matrix[i][column]
                if factor:matrix[i]=[x-factor*y for x,y in zip(matrix[i],matrix[column])]
        coefficients=[row[size:] for row in matrix]
        wmin=min(row[3] for row in coefficients)
        assert wmin>0
        assert all(sum(x*x for x in row[:3])<=upper*upper*wmin*wmin for row in coefficients),(
            'independent original-source whole-triangle residual',upper)
        count+=1
    if parents:
        assert set(piece_counts)==set(parents)
        assert max(piece_counts.values())>1,'no cross-leaf actual facet'
        assert all(not delta for line in chains.values() for events in line.values() for delta in events.values()), 'virtual positive-piece chain does not equal original facet'
        if require_cancellation:assert cancellation,'signed original source did not exercise cancelling SAME-L terms'
    else:assert count==4+precise_count,count
    return count
from native_mesher_point_trig_vectors import original_oracle,plus,times
original_oracle=lru_cache(maxsize=None)(original_oracle)


def cap_check(source_path, output):
    """Bound ORIGINAL Fsrc-SAME L by independent Taylor/power coefficients.

    The production operation uses directional derivatives. Here exact power
    arithmetic, Taylor remainder and tensor Bernstein subdivision bound the
    complete (xi,rho) square, including the collapsed physical lower edge.
    """
    import sys
    from math import factorial
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'python'))
    from brepgpu.imported_source import load_source
    original=load_source(source_path)
    carrier=original.carriers[0]
    frame=tuple(map(F,carrier.frame));radius=F(carrier.radii[0])
    assert len(original.coedges)==3 and len(original.edges)==2
    assert not any(e.degenerate for e in original.edges)

    def field(x):
        if x.startswith('r:'):
            n,d=x[2:].split('/');return F(int(n,16),int(d,16))
        return F(float.fromhex(x))
    def add(a,b):
        p=dict(a[0])
        for k,v in b[0].items():p[k]=p.get(k,F(0))+v
        return {k:v for k,v in p.items() if v},a[1]+b[1]
    def scale(a,s):return {k:v*s for k,v in a[0].items() if v*s},a[1]*abs(s)
    def mul(a,b):
        p=defaultdict(F)
        for (i,j),v in a[0].items():
            for (k,l),w in b[0].items():p[i+k,j+l]+=v*w
        error=a[1]*sum(map(abs,b[0].values()))+b[1]*sum(map(abs,a[0].values()))+a[1]*b[1]
        return dict(p),error
    def constant(x):return ({(0,0):x} if x else {}),F(0)
    def small_interval(lo,hi):
        # Keep the exact oracle's thousands-bit transcendental denominator out
        # of polynomial arithmetic. The rounded dyadic coefficient error stays.
        bits=160;mid=(lo+hi)/2;n=(mid.numerator<<bits)//mid.denominator
        chosen=F(n,1<<bits)
        return chosen,max(abs(lo-chosen),abs(hi-chosen))
    def trig_poly(argument,lo,hi,sine):
        center=(lo+hi)/2;delta=add(argument,constant(-center));width=(hi-lo)/2
        slo,shi,clo,chi=original_oracle(center)
        derivatives=((slo,shi),(clo,chi),(-shi,-slo),(-chi,-clo))
        if not sine:derivatives=derivatives[1:]+derivatives[:1]
        answer=constant(F(0));power=constant(F(1));error=F(0);order=8
        for k in range(order+1):
            value,uncertainty=small_interval(*derivatives[k%4])
            answer=add(answer,scale(power,value/F(factorial(k))))
            error+=uncertainty*width**k/F(factorial(k))
            power=mul(power,delta)
        return answer[0],answer[1]+error+width**(order+1)/F(factorial(order+1))
    def scalar_trig(x,sine):
        slo,shi,clo,chi=original_oracle(x)
        midpoint,error=small_interval(*((slo,shi) if sine else (clo,chi)))
        return constant(midpoint)[0],error
    def split(net,axis):
        n=len(net);m=len(net[0]);left=[[F(0)]*m for _ in range(n)];right=[[F(0)]*m for _ in range(n)]
        for line in range(m if axis==0 else n):
            work=[net[i][line] for i in range(n)] if axis==0 else list(net[line])
            degree=len(work)-1
            for level in range(degree+1):
                if axis==0:left[level][line],right[degree-level][line]=work[0],work[-1]
                else:left[line][level],right[line][degree-level]=work[0],work[-1]
                work=[(a+b)/2 for a,b in zip(work,work[1:])]
        return left,right
    def prove(residuals,upper):
        p=max(i for polynomial,_ in residuals for i,j in polynomial)
        q=max(j for polynomial,_ in residuals for i,j in polynomial)
        nets=[]
        for polynomial,error in residuals:
            net=[[sum(v*F(comb(I,i),comb(p,i))*F(comb(J,j),comb(q,j))
                      for (i,j),v in polynomial.items() if i<=I and j<=J)
                  for J in range(q+1)] for I in range(p+1)]
            nets.append((net,error))
        pending=[(nets,0)];pieces=0
        while pending:
            rows,depth=pending.pop();pieces+=1
            assert pieces<=1023,'bounded cap oracle subdivision exhausted'
            maximum=max(sum((abs(net[I][J])+error)**2 for net,error in rows)
                        for I in range(p+1) for J in range(q+1))
            if maximum<=upper*upper:continue
            assert depth<9,('original whole-rho cap residual exceeds emitted upper',maximum,upper*upper)
            children=[[],[]]
            for net,error in rows:
                a,b=split(net,depth%2);children[0].append((a,error));children[1].append((b,error))
            pending.extend((child,depth+1) for child in children)

    source_record=None;counts=Counter();band_witness=False
    for line in output.splitlines():
        fields=line.split('\t');kind=fields[0];counts[kind]+=1
        if kind in ('cap-native','cap-progress-uncertain'):
            assert fields[1]=='PASS';continue
        if kind=='cap-source':
            capkind,panels,stage=map(int,fields[1:4]);values=list(map(field,fields[4:]))
            assert panels>=4 and stage in (0,1,2,3,4)
            if capkind<2:
                # Obtain pi/2 independently through atan-free Taylor oracle:
                # the point generator records a binary64 pole V; its cosine
                # enclosure alone does not certify exact pole identity. The
                # independent trig oracle exports a rigorous pi enclosure.
                from native_mesher_point_trig_vectors import pi_interval
                pi=pi_interval(256)
                pole=(pi[0]/2,pi[1]/2) if capkind==0 else (-pi[1]/2,-pi[0]/2)
                ideal=[(frame[a]+(radius if capkind==0 else -radius)*frame[9+a],)*2 for a in range(3)]
            else:
                s0,s1,c0,c1=original_oracle(F(carrier.radii[1]))
                pole=tuple(sorted((-radius/s0,-radius/s1)))
                axial=times(pole,(c0,c1));ideal=[plus((frame[a],)*2,times(axial,(frame[9+a],)*2)) for a in range(3)]
            assert values[0]<=pole[0]<=pole[1]<=values[1]
            assert all(values[2+2*a]<=ideal[a][0]<=ideal[a][1]<=values[3+2*a] for a in range(3))
            source_record=capkind,pole,ideal;continue
        assert source_record is not None
        capkind,pole,ideal=source_record
        if kind=='cap-regularity':
            rn,precise=map(int,fields[1:3]);u0,u1,v0,v1=map(field,fields[3:])
            if carrier.kind==4:
                _,_,a,b=original_oracle(v0);_,_,c,d=original_oracle(v1)
                regular=(a>0 and c>0) or (b<0 and d<0)
                regular=regular and not (v0<=pole[0] and pole[1]<=v1)
            else:
                s0,s1,_,_=original_oracle(F(carrier.radii[1]))
                r0=plus((radius,)*2,times((v0,)*2,(s0,s1)))
                r1=plus((radius,)*2,times((v1,)*2,(s0,s1)))
                regular=(r0[0]>0 and r1[0]>0) or (r0[1]<0 and r1[1]<0)
            assert rn==precise==int(regular);continue
        if kind=='cap-progress':
            states=list(map(int,fields[1:3]));values=list(map(field,fields[3:]));old,new=values[:4],values[4:]
            assert states==[0,0]
            for i in (1,3):
                side=1 if old[i]>pole[1] else -1
                residual=tuple(sorted(side*(4*new[i]-3*old[i]-p) for p in pole))
                assert residual[1]<=0
            continue
        assert kind in ('cap-bound','cap-precise'),kind
        scenario=int(fields[1]);values=list(map(field,fields[2:18]))
        upper=values[0];u0,g0,u1,g1,vf0,vf1=values[1:7]
        actual=[values[7+3*i:10+3*i] for i in range(3)]
        components=list(map(field,fields[18:24]));sheet=int(fields[24]);roles=list(map(int,fields[25:]))
        assert tuple(actual[0])==tuple(map(F,original.vertices[0].xyz))
        assert (g0-vf0)*(g1-vf1)>0 and roles==([0,2,1] if g0>vf0 else [0,1,2])
        side=1 if g0>pole[1] else -1
        expected=(-side if capkind==0 else side) if capkind<2 else side
        assert sheet==expected
        U=({(0,0):u0,(1,0):u1-u0},F(0))
        V=({(0,0):vf0,(1,0):vf1-vf0,(0,1):g0-vf0,(1,1):g1-g0-vf1+vf0},F(0))
        su=trig_poly(U,u0,u1,True);cu=trig_poly(U,u0,u1,False)
        if carrier.kind==4:
            lo,hi=min(vf0,vf1,g0,g1),max(vf0,vf1,g0,g1)
            radial=scale(trig_poly(V,lo,hi,False),radius)
            axial=scale(trig_poly(V,lo,hi,True),radius)
        else:
            radial=add(constant(radius),mul(V,scalar_trig(F(carrier.radii[1]),True)))
            axial=mul(V,scalar_trig(F(carrier.radii[1]),False))
        residuals=[]
        for a in range(3):
            harmonic=add(scale(cu,frame[3+a]),scale(su,frame[6+a]))
            result=add(mul(radial,harmonic),scale(axial,frame[9+a]))
            # Subtract constant origin and Q algebraically before enclosures.
            linear={(0,0):frame[a]-actual[0][a],(0,1):actual[0][a]-actual[1][a],
                    (1,1):actual[1][a]-actual[2][a]}
            residuals.append(add(result,(linear,F(0))))
        prove(residuals,upper)
        if kind=='cap-bound':
            position,band,angular,radial_bound,corner,pole_departure=components
            assert upper>=position+band and position>=angular+radial_bound+corner
            assert pole_departure<=corner
            # The finite-v -> ideal-v connector must be bounded over WHOLE rho.
            # A large nonorthogonal Z makes its interior departure visible.
            if scenario==1 and carrier.kind==4 and abs(frame[11])>100:
                rho=F(1,2);u=u1;finite_v=vf1
                v=finite_v*(1-rho)+g1*rho
                vp=tuple(p*(1-rho)+g1*rho for p in pole)
                su0,su1,cu0,cu1=original_oracle(u)
                sv0,sv1,cv0,cv1=original_oracle(v)
                p0=original_oracle(vp[0]);p1=original_oracle(vp[1])
                ds=(sv0-max(p0[1],p1[1]),sv1-min(p0[0],p1[0]))
                dc=(cv0-max(p0[3],p1[3]),cv1-min(p0[2],p1[2]))
                difference=[]
                for a in range(3):
                    h=plus(times((cu0,cu1),(frame[3+a],)*2),times((su0,su1),(frame[6+a],)*2))
                    difference.append(times((radius,)*2,plus(times(dc,h),times(ds,(frame[9+a],)*2))))
                lower=sum((min(abs(a),abs(b)) if a*b>0 else F(0))**2 for a,b in difference)
                # Old near-pole-strip Sv omits the large Z contribution.
                gap=max(abs(finite_v-p) for p in pole)
                old_sv=sum(abs(z) for z in frame[3:9])+gap*sum(abs(z) for z in frame[9:12])
                band_witness=band_witness or lower>(gap*old_sv)**2
    assert counts['cap-bound']==4 and counts['cap-precise']==2 and counts['cap-regularity']==2
    assert counts['cap-progress']==counts['cap-native']==counts['cap-source']==1
    if carrier.kind==4 and abs(frame[11])>100:assert band_witness,'whole-rho band attack not exercised'
    return counts['cap-bound']+counts['cap-precise']


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--fixture',type=Path)
    p.add_argument('--source',type=Path,required=True)
    p.add_argument('--analytic-fixtures',type=Path)
    p.add_argument('--cuda-module',type=Path)
    p.add_argument('--device',type=int,default=0)
    p.add_argument('--trimmed-fixture',type=Path)
    p.add_argument('--cap-fixture',type=Path)
    a=p.parse_args()
    if not a.fixture and not a.trimmed_fixture and not a.cap_fixture:
        p.error('--fixture, --trimmed-fixture or --cap-fixture is required')
    backend=[] if a.cuda_module is None else ["--cuda",str(a.cuda_module),str(a.device)]
    if a.cap_fixture:
        cap=subprocess.run([str(a.cap_fixture),str(a.source),*backend],check=True,capture_output=True,text=True)
        n=cap_check(a.source,cap.stdout)
        print('original-cap-geometry-reader\tPASS\t'+str(n))
    if a.trimmed_fixture:
        trimmed=subprocess.run([str(a.trimmed_fixture),str(a.source),*backend],check=True,capture_output=True,text=True)
        n=trimmed_check(a.source,trimmed.stdout)
        print('original-trimmed-geometry-reader\tPASS\t'+str(n))
        if 'trim-work-parent\t' in trimmed.stdout:
            # SAME work fixture/oracle, original source translated before native
            # decode. Negative parent Q and cancelling affine terms are real
            # source geometry, not a forged view or numerical packet.
            from dataclasses import replace
            import tempfile
            from brepgpu.imported_source import load_source,write_source
            original=load_source(a.source)
            shift=(-.5,-.5,-.5)
            def controls(values):
                result=list(values)
                for at in range(0,len(result),4):
                    for axis in range(3):result[at+axis]+=shift[axis]*result[at+3]
                return tuple(result)
            def curve(c):
                if c.dimension!=3:return c
                if c.kind in (4,5):return replace(c,controls=controls(c.controls))
                frame=list(c.frame)
                for axis in range(3):frame[axis]+=shift[axis]
                return replace(c,frame=tuple(frame))
            signed=replace(original,
                vertices=tuple(replace(v,xyz=tuple(v.xyz[a]+shift[a] for a in range(3))) for v in original.vertices),
                curves=tuple(curve(c) for c in original.curves),
                carriers=tuple(replace(c,controls=controls(c.controls)) if c.kind==6 else c for c in original.carriers))
            with tempfile.TemporaryDirectory(prefix='adaptive-work-signed-') as directory:
                signed_path=Path(directory)/'signed.bgsource'
                write_source(signed,signed_path)
                captured=subprocess.run([str(a.trimmed_fixture),str(signed_path)],check=True,capture_output=True,text=True)
                n=trimmed_check(signed_path,captured.stdout,require_cancellation=True)
                print('original-trimmed-signed-work-reader\tPASS\t'+str(n))
    if not a.fixture:
        return
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
