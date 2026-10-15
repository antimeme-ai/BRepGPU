#!/usr/bin/env python3
"""Source-based NCSX study: owned fitted carriers, P100 first jets, actual image."""
import argparse
from array import array
import csv
from fractions import Fraction as F
import hashlib
import math
from pathlib import Path
import sys
import time

from brepgpu import Engine, Patch
from ncsx_display import render

ROOT=Path(__file__).resolve().parents[1]
SCALE=4.
COIL_SEGMENTS=64
PLASMA_PHI,PLASMA_THETA=36,24
HALF_WIDTH,HALF_DEPTH=.04,.06
DATA=ROOT/'data/ncsx'
SOURCES=('examples/render_ncsx.py','examples/ncsx_display.py','python/brepgpu/__init__.py',
         'data/ncsx/NCSX.dat','data/ncsx/input.NCSX_c09r00_halfTeslaTF','data/ncsx/LICENSE.simsopt')
EXPECTED=('2b857ca137184d0f4b5e7fa271c6cf946cf7fdfa127a19ec302d4f9731db57a7',
          '32b5609d02643d05b30c353ba5ab0a9b3ef6c486a8122f141a3c14e01748c956')


def require(condition,message):
    if not condition:raise RuntimeError(message)


def add(a,b):return tuple(x+y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def mul(a,k):return tuple(x*k for x in a)
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]


def unit(v):
    length=math.hypot(*v)
    require(math.isfinite(length) and length>1e-12,'degenerate source/native direction')
    return mul(v,1/length)


def load_data():
    files=(DATA/'NCSX.dat',DATA/'input.NCSX_c09r00_halfTeslaTF')
    require(tuple(hashlib.sha256(p.read_bytes()).hexdigest() for p in files)==EXPECTED,'source dataset hash mismatch')
    with files[0].open() as stream:coils=tuple(tuple(float(x) for x in row) for row in csv.reader(stream))
    require(len(coils)==31 and all(len(r)==18 and all(math.isfinite(x) for x in r) for r in coils),'coil source schema differs')
    require(all(coils[0][i]==0 for i in range(0,18,2)),'unexpected zero-mode sine terms')
    lines=[s.split() for s in files[1].read_text().splitlines() if s.strip() and not s.lstrip().startswith('#')]
    require(tuple(map(int,lines[0]))==(136,3,0),'FOCUS source header differs')
    modes=tuple((int(r[0]),int(r[1]),*(float(x) for x in r[2:])) for r in lines[1:])
    require(len(modes)==136 and all(len(r)==6 and all(math.isfinite(x) for x in r) for r in modes),'FOCUS source rows differ')
    return coils,modes


def centerline(rows,base,t,derivative=False):
    angle=2*math.pi*t
    answer=[]
    for axis in range(3):
        a=6*base+2*axis
        if derivative:
            answer.append(sum(m*(row[a]*math.cos(m*angle)-row[a+1]*math.sin(m*angle)) for m,row in enumerate(rows[1:],1)))
        else:
            answer.append(rows[0][a+1]+sum(row[a]*math.sin(m*angle)+row[a+1]*math.cos(m*angle) for m,row in enumerate(rows[1:],1)))
    return tuple(answer)


def symmetry(point,period,flip):
    a=2*math.pi*period/3;c,s=math.cos(a),math.sin(a)
    x,y,z=point;point=(c*x-s*y,s*x+c*y,z)
    return (point[0],-point[1],-point[2]) if flip else point


def plasma_point(modes,phi,theta):
    R=Z=0.
    for n,m,rc,rs,zc,zs in modes:
        phase=m*theta-3*n*phi;c,s=math.cos(phase),math.sin(phase)
        R+=rc*c+rs*s;Z+=zc*c+zs*s
    require(R>0,'nonpositive source cylindrical radius')
    return R*math.cos(phi),R*math.sin(phi),Z


def catmull(points,k):
    count=len(points);a,b,c,d=(points[(k+i)%count] for i in (-1,0,1,2))
    return b,add(b,mul(sub(c,a),1/6)),sub(c,mul(sub(d,b),1/6)),c


def build(rows,modes):
    patches=[];tiles=[];meta=[];offset=0
    def emit(degrees,controls,identity,grid,material):
        nonlocal offset
        payload=tuple(value for p in controls for value in (*mul(p,1/SCALE),1.))
        require(all(math.isfinite(x) for x in payload),'nonfinite fitted control')
        require(all(abs(payload[k+a])<=payload[k+3] for k in range(0,len(payload),4) for a in range(3)),'control outside native envelope')
        patches.append(Patch(*degrees,payload,scale=SCALE))
        nu,nv=grid;tiles.append((offset,nu,nv,material));offset+=nu*nv
        meta.append(identity)
    base_rails=[]
    for base in range(3):
        rails=[[] for _ in range(4)]
        for k in range(COIL_SEGMENTS):
            t=k/COIL_SEGMENTS;p=centerline(rows,base,t);T=unit(centerline(rows,base,t,True))
            rho=math.hypot(p[0],p[1]);require(rho>1e-8,'source centerline touches toroidal axis')
            toroidal=(-p[1]/rho,p[0]/rho,0.)
            projected=sub(toroidal,mul(T,dot(T,toroidal)))
            require(math.hypot(*projected)>1e-8,'projected section frame is degenerate')
            N=unit(projected);B=unit(cross(T,N))
            # Clockwise section perimeter viewed along tangent: outward du×dv.
            for rail,(w,d) in zip(rails,((HALF_WIDTH,HALF_DEPTH),(HALF_WIDTH,-HALF_DEPTH),(-HALF_WIDTH,-HALF_DEPTH),(-HALF_WIDTH,HALF_DEPTH))):
                rail.append(add(p,add(mul(N,w),mul(B,d))))
        base_rails.append(rails)
    component=0
    for period in range(3):
        for flip in (0,1):
            for base in range(3):
                rails=base_rails[base]
                for segment in range(COIL_SEGMENTS):
                    curves=[catmull(rail,segment) for rail in rails]
                    for side in range(4):
                        control=tuple(symmetry(curves[r][u],period,flip) for u in range(4) for r in (side,(side+1)%4))
                        emit((3,1),control,('coil',component,segment,side,base,period,flip),(5,3),base)
                component+=1
    points=tuple(tuple(plasma_point(modes,2*math.pi*i/PLASMA_PHI,2*math.pi*j/PLASMA_THETA) for j in range(PLASMA_THETA)) for i in range(PLASMA_PHI))
    matrix=((0.,1.,0.,0.),(-1/6,1.,1/6,0.),(0.,1/6,1.,-1/6),(0.,0.,1.,0.))
    for i in range(PLASMA_PHI):
        for j in range(PLASMA_THETA):
            controls=[]
            for u in range(4):
                for v in range(4):
                    controls.append(tuple(sum(matrix[u][a]*matrix[v][b]*points[(i+a-1)%PLASMA_PHI][(j+b-1)%PLASMA_THETA][axis] for a in range(4) for b in range(4)) for axis in range(3)))
            emit((3,3),controls,('plasma',18,i,j,-1,-1,-1),(5,5),3)
    return tuple(patches),tuple(tiles),tuple(meta)


def basis(degree,t,derivative):
    if derivative:
        lower=basis(degree-1,t,False) if degree else ()
        return tuple(degree*((lower[i-1] if i else F(0))-(lower[i] if i<degree else F(0))) for i in range(degree+1))
    return tuple(math.comb(degree,i)*t**i*(1-t)**(degree-i) for i in range(degree+1))


def qualify(patches,tiles,jets):
    maxima=[F(0)]*3
    basis_cache={}
    for patch,(start,nu,nv,_) in zip(patches,tiles,strict=True):
        p,q=patch.degree_u,patch.degree_v
        coefficients=tuple(F(x) for x in patch.controls)
        require(all(coefficients[4*k+3]==1 for k in range((p+1)*(q+1))),'polynomial oracle needs unit weights')
        for i in range(nu):
            for j in range(nv):
                u,v=F(i,nu-1),F(j,nv-1);k=start+i*nv+j
                for term,(du,dv) in enumerate(((0,0),(1,0),(0,1))):
                    key=p,q,u,v,du,dv
                    weights=basis_cache.get(key)
                    if weights is None:
                        bu,bv=basis(p,u,du),basis(q,v,dv)
                        weights=tuple(a*b for a in bu for b in bv);basis_cache[key]=weights
                    expected=tuple(sum((coefficients[4*m+axis]*w for m,w in enumerate(weights) if w),F(0)) for axis in range(3))
                    observed=jets.vector(term,k)
                    maxima[term]=max(maxima[term],*(abs(F(a)-b) for a,b in zip(observed,expected)))
    require(maxima[0]<=F(1,2**40) and max(maxima[1:])<=F(1,2**30),'native binary-control polynomial grade failed')
    return tuple(maxima)


def continuity(tiles,meta,jets):
    lookup={identity[:4]:tile for identity,tile in zip(meta,tiles,strict=True)}
    maximum=0.
    def compare(a,b):
        nonlocal maximum
        maximum=max(maximum,*(abs(x-y) for x,y in zip(jets.vector(0,a),jets.vector(0,b))))
    for identity,(start,nu,nv,_) in zip(meta,tiles,strict=True):
        kind,component,i,j=identity[:4]
        if kind=='coil':
            nxt=lookup[kind,component,(i+1)%COIL_SEGMENTS,j]
            adjacent=lookup[kind,component,i,(j+1)%4]
        else:
            nxt=lookup[kind,component,(i+1)%PLASMA_PHI,j]
            adjacent=lookup[kind,component,i,(j+1)%PLASMA_THETA]
        for v in range(nv):compare(start+(nu-1)*nv+v,nxt[0]+v)
        for u in range(nu):compare(start+u*nv+nv-1,adjacent[0]+u*adjacent[2])
    require(maximum<=2**-39,'fitted carrier seam position mismatch')
    return maximum


def source_fit(rows,modes,tiles,meta,jets):
    lookup={identity[:4]:tile for identity,tile in zip(meta,tiles,strict=True)}
    coil_error=plasma_error=0.;coil_checks=plasma_checks=0
    for identity,(start,nu,nv,_) in zip(meta,tiles,strict=True):
        kind,component,i,j,base,period,flip=identity
        if kind=='coil':
            if j:continue
            opposite=lookup[kind,component,i,2]
            for u in (1,2,3):
                a=jets.vector(0,start+u*nv+1);b=jets.vector(0,opposite[0]+u*opposite[2]+1)
                fitted=tuple(SCALE*(x+y)/2 for x,y in zip(a,b))
                original=symmetry(centerline(rows,base,(i+u/4)/COIL_SEGMENTS),period,flip)
                coil_error=max(coil_error,math.dist(fitted,original));coil_checks+=1
        else:
            for u in (1,2,3):
                for v in (1,2,3):
                    fitted=mul(jets.vector(0,start+u*nv+v),SCALE)
                    original=plasma_point(modes,2*math.pi*(i+u/4)/PLASMA_PHI,2*math.pi*(j+v/4)/PLASMA_THETA)
                    plasma_error=max(plasma_error,math.dist(fitted,original));plasma_checks+=1
    return coil_error,plasma_error,coil_checks,plasma_checks


def tsv(path,header,rows):
    with path.open('w',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(header);writer.writerows(rows)


def main(args):
    require(sys.platform=='linux','product geometry executes on the fleet')
    started=previous=time.perf_counter_ns();timings=[]
    def phase(name):
        nonlocal previous
        now=time.perf_counter_ns();timings.append((name,(now-previous)/1e6));previous=now
    out=args.output_directory;out.mkdir(parents=True,exist_ok=False)
    pins=tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES)
    rows,modes=load_data();patches,tiles,meta=build(rows,modes)
    phase('source_pins_load_and_fit')
    slots=array('I',(slot for slot,(_,nu,nv,_) in enumerate(tiles) for _ in range(nu*nv)))
    uv=array('d',(value for _,nu,nv,_ in tiles for i in range(nu) for j in range(nv) for value in (i/(nu-1),j/(nv-1))))
    phase('query_prepare');native_started=time.perf_counter_ns()
    with Engine(backend=args.backend,device=args.device,threads=args.threads) as engine:
        core_path=Path(engine._lib._name).resolve()
        native_paths=(core_path,)+( (core_path.with_name('libbrepgpu_cuda.so'),) if args.backend=='cuda' else () )
        native_pins=tuple((path.name,hashlib.sha256(path.read_bytes()).hexdigest()) for path in native_paths)
        phase('engine_setup_and_native_pins')
        with engine.create_patch_set(patches) as ps:
            phase('patch_setup')
            with engine.create_query_set(ps,slots,uv) as qs:
                phase('query_setup')
                with engine.evaluate(qs,order=1) as result:
                    phase('evaluate_sync_wrapper')
                    jets=result.download();phase('download_and_sources')
                phase('result_cleanup')
            phase('query_cleanup')
        phase('patch_cleanup')
    phase('engine_cleanup');native_ms=(time.perf_counter_ns()-native_started)/1e6
    require(jets.info.backend==int(args.backend=='cuda') and jets.info.patch_count==len(patches), 'native observed backend/patch ownership mismatch')
    require(jets.info.terms==3 and len(jets.data)==9*len(slots),'native first-jet extent mismatch')
    require(jets.count==len(slots) and jets.order==1 and jets._slots==slots and jets._uv==uv,'native result query binding mismatch')
    require(len(jets._descriptors)==len(patches),'native patch count mismatch')
    offset=0
    for descriptor,patch in zip(jets._descriptors,patches,strict=True):
        count=len(patch.controls)//4
        require((descriptor.degree_u,descriptor.degree_v,descriptor.control_offset,descriptor.control_count,descriptor.flags)==(patch.degree_u,patch.degree_v,offset,count,2)
                and tuple(descriptor.uv)==patch.uv and tuple(descriptor.origin)==patch.origin and descriptor.scale==SCALE,'native descriptor mismatch')
        offset+=count
    jets.require_ok();require(all(math.isfinite(x) for x in jets.data),'nonfinite native output')
    phase('native_binding_status_finite_checks')
    maxima=qualify(patches,tiles,jets);phase('exact_arithmetic_qualification')
    seam=continuity(tiles,meta,jets);phase('seam_checks')
    coil_fit,plasma_fit,coil_checks,plasma_checks=source_fit(rows,modes,tiles,meta,jets);phase('source_fit_diagnostics')
    points=[mul(jets.vector(0,k),SCALE) for k in range(jets.count)]
    normals=[unit(cross(jets.vector(1,k),jets.vector(2,k))) for k in range(jets.count)]
    phase('display_point_normal_prepare')
    tsv(out/'controls.tsv',('patch','control','X_hex','Y_hex','Z_hex','W_hex'),((slot,k,*(x.hex() for x in patch.controls[4*k:4*k+4])) for slot,patch in enumerate(patches) for k in range(len(patch.controls)//4)))
    tsv(out/'patches.tsv',('patch','kind','component','segment_or_phi','side_or_theta','base','period','flip','query_start','nu','nv','material','degree_u','degree_v'),((slot,*identity,*tile,patch.degree_u,patch.degree_v) for slot,(identity,tile,patch) in enumerate(zip(meta,tiles,patches,strict=True))))
    tsv(out/'qualification.tsv',('check','maximum','limit_or_units','samples'),(('position',float(maxima[0]),2**-40,jets.count),('du',float(maxima[1]),2**-30,jets.count),('dv',float(maxima[2]),2**-30,jets.count),('seam_position',seam,2**-39,'all neighboring patch boundary samples'),('source_coil_centerline_fit',coil_fit,'original coordinate units; sampled diagnostic only',coil_checks),('source_plasma_fit',plasma_fit,'original coordinate units; sampled diagnostic only',plasma_checks)))
    tsv(out/'recipe.tsv',('key','value'),(('source_revision','81436b4f7c655a4099eadadb168b7cc27923e555'),('coil_fourier_order',30),('coil_instances',18),('field_periods',3),('coil_segments',COIL_SEGMENTS),('pack_half_width_source_units',HALF_WIDTH),('pack_half_depth_source_units',HALF_DEPTH),('section_frame','projected toroidal direction; periodic source tangent'),('plasma_phi_segments',PLASMA_PHI),('plasma_theta_segments',PLASMA_THETA),('plasma_component','FOCUS plasma boundary; not vacuum vessel'),('source_length_units','not declared by raw files'),('native_scale',SCALE),('backend',args.backend),('observed_native_backend_tag',jets.info.backend),('device',args.device),('patches',len(patches)),('queries',jets.count),('native_components_per_query',9),('native_status','all BG_OK; finite'),('raw_jets_layout','little-endian f64; component-major Sxyz,Suxyz,Svxyz; local normalized'),('query_rule','patch-major,u-major/v-fast; i/(nu-1),j/(nv-1)')))
    tsv(out/'sources.tsv',('path','sha256'),pins)
    tsv(out/'native-libraries.tsv',('file','sha256'),native_pins)
    (out/'jets.f64').write_bytes(jets.data.tobytes())
    phase('artifact_tables_and_raw_jets')
    render(out/'ncsx.svg',points,normals,tiles,dict(patches=len(patches),samples=jets.count,backend=args.backend,device=args.device))
    phase('cpu_display_render_and_writes')
    require(pins==tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES),'source changed during geometry/render generation')
    require(native_pins==tuple((path.name,hashlib.sha256(path.read_bytes()).hexdigest()) for path in native_paths),'native library changed during scene generation')
    phase('final_source_native_pin_checks')
    total_ms=(time.perf_counter_ns()-started)/1e6
    timings.extend((('native_setup_to_cleanup_wall',native_ms),('full_qualified_caller_wall',total_ms)))
    tsv(out/'costs.tsv',('phase','wall_ms'),timings)
    tsv(out/'placement.tsv',('field','value'),(('requested_backend',args.backend),('requested_device',args.device),('requested_threads',args.threads),('observed_native_backend_tag',jets.info.backend)))
    print(f'full_qualified_caller_ms={total_ms:.6f} native_setup_to_cleanup_ms={native_ms:.6f}',flush=True)
    print(f'patches={len(patches)} samples={jets.count} local_errors={[float(e) for e in maxima]} seam={seam} coil_fit={coil_fit} plasma_fit={plasma_fit}',flush=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--backend',choices=('cpu','cuda'),default='cuda')
    parser.add_argument('--device',type=int,default=0)
    parser.add_argument('--threads',type=int,default=1)
    parser.add_argument('--output-directory',type=Path,required=True)
    main(parser.parse_args())
