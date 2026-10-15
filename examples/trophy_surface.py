#!/usr/bin/env python3
"""Orbital reliquary: actual native rational carriers, analytic checks, owned SVG."""
import argparse
from array import array
import csv
from fractions import Fraction as F
import hashlib
import math
from pathlib import Path
from brepgpu import Engine, Patch


def rotate(x, y, quadrant):
    for _ in range(quadrant):
        x, y = -y, x
    return x, y


def transform(p, axis):
    return (p[0], p[1], p[2]) if axis == 0 else (p[2], p[0], p[1]) if axis == 1 else (p[1], p[2], p[0])


def recipe():
    # All radii, translations and homogeneous controls are exact binary64.
    specs = [(F(1,2), F(3,64), axis, (F(0),)*3, axis) for axis in range(3)]
    specs += [(F(3,8), F(1,64), axis, (F(0),)*3, 3) for axis in range(3)]
    for axis in range(3):
        for sign in (-1,1):
            center = transform((F(sign,2), F(0), F(0)), axis)
            specs.append((F(1,8), F(1,32), (axis+1)%3, center, 4))
    specs.append((F(5,64), F(1,64), 0, (F(0),)*3, 1))
    return specs


def analytic_circle(t, quadrant):
    # Literal signed bases avoid sharing control-builder quadrant logic.
    c,s=(1-t*t)/(1+t*t),2*t/(1+t*t)
    dc,ds=-4*t/(1+t*t)**2,2*(1-t*t)/(1+t*t)**2
    return ((c,s,dc,ds),(-s,c,-ds,dc),(-c,-s,-dc,-ds),(s,-c,ds,-dc))[quadrant]


def truth(spec, qu, qv, u, v):
    R,r,axis,center,_ = spec
    cu,su,du,dus = analytic_circle(u,qu)
    cv,sv,dv,dvs = analytic_circle(v,qv)
    rho = R+r*cv
    # Explicit source-to-world basis is independent of transform().
    indices=((0,1,2),(2,0,1),(1,2,0))[axis]
    p=(rho*cu,rho*su,r*sv)
    pu=(rho*du,rho*dus,F(0)); pv=(r*dv*cu,r*dv*su,r*dvs)
    return (tuple(p[a]+center[k] for k,a in enumerate(indices)),
            tuple(pu[a] for a in indices),tuple(pv[a] for a in indices))


def patches_for(spec):
    R,r,axis,center,_ = spec
    base = ((F(1),F(0),F(1)),(F(1),F(1),F(1)),(F(0),F(2),F(2)))
    patches=[]
    for qu in range(4):
        for qv in range(4):
            controls=[]
            for au,bu,wu in base:
                au,bu=rotate(au,bu,qu)
                for av,bv,wv in base:
                    av,bv=rotate(av,bv,qv)
                    w=wu*wv
                    xyz=transform((au*(R*wv+r*av),bu*(R*wv+r*av),wu*r*bv),axis)
                    xyz=tuple(a+b*w for a,b in zip(xyz,center))
                    assert all(abs(a)<=w for a in xyz) and 1<=w<=256
                    controls.extend(float(a) for a in (*xyz,w))
            patches.append(Patch(2,2,controls))
    return patches


def view(p):
    x,y,z=p
    # Orthonormal camera rotations; these do not alter source geometry.
    ca,sa=math.cos(.62),math.sin(.62)
    cb,sb=math.cos(.74),math.sin(.74)
    x,y=ca*x-sa*y,sa*x+ca*y
    return x,cb*y-sb*z,sb*y+cb*z


def unit(p):
    d=math.sqrt(sum(x*x for x in p))
    if not d or not math.isfinite(d): raise RuntimeError('degenerate native normal')
    return tuple(x/d for x in p)


def cross(a,b):
    return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])


def render(output, specs, points, normals, n, backend_label):
    palette=((.16,.68,.72),(.87,.45,.20),(.77,.76,.48),(.28,.49,.62),(.64,.31,.39))
    polys=[]
    light=unit((-.4,-.5,1))
    for ti,spec in enumerate(specs):
        color=palette[spec[4]]
        for patch in range(16):
            start=(ti*16+patch)*n*n
            for i in range(n-1):
                for j in range(n-1):
                    ids=[start+i*n+j,start+(i+1)*n+j,start+(i+1)*n+j+1,start+i*n+j+1]
                    vertices=[view(points[k]) for k in ids]
                    normal=unit(view(tuple(sum(normals[k][a] for k in ids) for a in range(3))))
                    diffuse=abs(sum(a*b for a,b in zip(normal,light)))
                    highlight=abs(normal[2])**30
                    rim=(1-abs(normal[2]))**3
                    rgb=[min(255,round(255*(c*(.23+.70*diffuse)+.28*highlight+.09*rim))) for c in color]
                    fill='#'+''.join(f'{c:02x}' for c in rgb)
                    xy=' '.join(f'{900+900*x:.2f},{795-900*y:.2f}' for x,y,z in vertices)
                    polys.append((sum(p[2] for p in vertices)/4,xy,fill))
    lines=['<svg xmlns="http://www.w3.org/2000/svg" width="1800" height="1500" viewBox="0 0 1800 1500">',
           '<defs><radialGradient id="bg"><stop stop-color="#182a36"/><stop offset="1" stop-color="#070e18"/></radialGradient></defs>',
           '<rect width="1800" height="1500" fill="url(#bg)"/>',
           '<g fill="none" stroke="#7b9ca9" opacity=".12">']
    for radius in (220,440,660): lines.append(f'<circle cx="900" cy="795" r="{radius}"/>')
    lines += ['<path d="M90 795H1710M900 195V1395"/></g>',
              '<g font-family="sans-serif"><text x="90" y="80" fill="#8ba4b7" font-size="20" letter-spacing="5">BRepGPU / STUDIES IN RATIONAL GEOMETRY / 01</text>',
              '<text x="88" y="152" fill="#f0e6cf" font-size="62" letter-spacing="2">ORBITAL RELIQUARY</text>',
              f'<text x="91" y="197" fill="#a3b6c5" font-size="23">Thirteen interleaved carriers. One FP64 patch batch · {backend_label}.</text></g>']
    lines += [f'<polygon points="{xy}" fill="{fill}" stroke="{fill}" stroke-width=".35"/>' for _,xy,fill in sorted(polys)]
    lines += ['<g font-family="monospace" font-size="19" fill="#a3b6c5">',
              '<text x="90" y="1402">208 RATIONAL QUADRATIC PATCHES / 60,112 POSITION + FIRST-JET SAMPLES</text>',
              '<text x="90" y="1440">Native derivative lighting · surface assembly · sampled display, no fusion or chord bound</text></g></svg>']
    output.write_text('\n'.join(lines)+'\n')


def main():
    parser=argparse.ArgumentParser(__doc__)
    parser.add_argument('--backend',choices=('cpu','cuda'),default='cuda')
    parser.add_argument('--device',type=int,default=0)
    parser.add_argument('--output-directory',type=Path,required=True)
    args=parser.parse_args()
    specs=recipe(); n=17
    patches=[p for spec in specs for p in patches_for(spec)]
    slots=array('I',(slot for slot in range(len(patches)) for _ in range(n*n)))
    uv=array('d',(x for _ in patches for i in range(n) for j in range(n) for x in (i/(n-1),j/(n-1))))
    with Engine(backend=args.backend,device=args.device) as engine:
        with engine.create_patch_set(patches) as ps:
            with engine.create_query_set(ps,slots,uv) as qs:
                with engine.evaluate(qs,order=1) as rs:
                    jets=rs.download()
    if jets.count != len(slots) or jets.order != 1 or jets._slots != slots or jets._uv != uv:
        raise RuntimeError('returned query binding mismatch')
    if len(jets._descriptors) != len(patches): raise RuntimeError('patch binding mismatch')
    for slot,(d,p) in enumerate(zip(jets._descriptors,patches)):
        if ((d.degree_u,d.degree_v,d.control_count) != (p.degree_u,p.degree_v,len(p.controls)//4)
                or d.control_offset != slot*9 or d.flags != 2 or tuple(d.uv) != p.uv or tuple(d.origin) != p.origin or d.scale != p.scale):
            raise RuntimeError('returned patch descriptor mismatch')
    jets.require_ok()
    if not all(math.isfinite(v) for v in jets.data): raise RuntimeError('nonfinite successful jet')
    points=[jets.vector(0,k) for k in range(len(slots))]
    normals=[unit(cross(jets.vector(1,k),jets.vector(2,k))) for k in range(len(slots))]
    max_errors=[F(0)]*3
    for k,slot in enumerate(slots):
        ti,patch=divmod(slot,16); qu,qv=divmod(patch,4)
        expected=truth(specs[ti],qu,qv,F(uv[2*k]),F(uv[2*k+1]))
        for term in range(3):
            error=max(abs(F(a)-b) for a,b in zip(jets.vector(term,k),expected[term]))
            max_errors[term]=max(max_errors[term],error)
    if max_errors[0]>F(1,2**40) or max(max_errors[1:])>F(1,2**30):
        raise RuntimeError('analytic sample qualification failed')
    # Every chart seam is represented twice: equal position, oriented unit normals.
    seam_error=normal_error=0.
    for ti in range(len(specs)):
        for qu in range(4):
            for qv in range(4):
                start=(ti*16+qu*4+qv)*n*n
                for j in range(n):
                    for a,b in ((start+(n-1)*n+j,(ti*16+((qu+1)%4)*4+qv)*n*n+j),
                                (start+j*n+n-1,(ti*16+qu*4+(qv+1)%4)*n*n+j*n)):
                        seam_error=max(seam_error,*(abs(x-y) for x,y in zip(points[a],points[b])))
                        normal_error=max(normal_error,*(abs(x-y) for x,y in zip(normals[a],normals[b])))
    if seam_error>2**-40 or normal_error>2**-30: raise RuntimeError('carrier seam mismatch')
    out=args.output_directory;out.mkdir(parents=True,exist_ok=True)
    def tsv(name,header,rows):
        with (out/name).open('w',newline='') as f:
            w=csv.writer(f,delimiter='\t',lineterminator='\n');w.writerow(header);w.writerows(rows)
    tsv('controls.tsv',('patch','control','X_hex','Y_hex','Z_hex','W_hex'),
        ((slot,k,*(x.hex() for x in p.controls[k*4:k*4+4])) for slot,p in enumerate(patches) for k in range(9)))
    tsv('assembly.tsv',('torus','R','r','axis','cx','cy','cz','palette'),
        ((i,R,r,axis,*c,color) for i,(R,r,axis,c,color) in enumerate(specs)))
    # Query rule is patch-major, u-major/v-fast dyadic 17x17; retain raw native outputs.
    (out/'jets.f64').write_bytes(jets.data.tobytes())
    tsv('qualification.tsv',('check','maximum_error','limit'),
        [('position',float(max_errors[0]),2**-40),('du',float(max_errors[1]),2**-30),('dv',float(max_errors[2]),2**-30),('seam_position',seam_error,2**-40),('seam_unit_normal',normal_error,2**-30)])
    tsv('sources.tsv',('path','sha256'),((path,hashlib.sha256(Path(path).read_bytes()).hexdigest()) for path in ('examples/trophy_surface.py','python/brepgpu/__init__.py')))
    render(out/'orbital.svg',specs,points,normals,n, f'CUDA device {args.device}' if args.backend=='cuda' else 'native CPU')
    (out/'REPORT.md').write_text(f'''# Orbital reliquary\n\nActual {args.backend} device {args.device} evaluation: {len(specs)} independent torus\ncarriers, {len(patches)} degree-(2,2) patches, {len(slots):,} samples, first jets.\nAll statuses succeeded; all outputs finite. Every sampled position and both\nfirst derivatives were compared against independent exact Fraction torus maps\nat the submitted binary64 parameters. See qualification.tsv for absolute errors\nand existing native grades. Closed chart seams also agree in position and\noriented unit normal; raw seam parameter speeds intentionally differ.\n\nThe assembly is a visual arrangement of separate carriers, not a fused B-rep.\nSVG quadrilaterals are display samples without a chord or trim guarantee.\nLighting uses actual returned derivatives; painter ordering can have visibility\nartifacts where carriers overlap. No performance claim is made.\n\ncontrols.tsv preserves homogeneous binary64 inputs; assembly.tsv defines the\nanalytic recipe. Query order: patch-major, u-major/v-fast, 17×17, parameters\ni/16,j/16 including endpoints. jets.f64 is native little-endian binary64,\nstructure-of-arrays: Sx,Sy,Sz,Sux,Suy,Suz,Svx,Svy,Svz, each {len(slots)} values.\nStatus success is checked before writing. Source hashes are in sources.tsv.\n\nReproduce from repository root with PYTHONPATH=python and BREPGPU_LIBRARY set\nto an existing qualified native library:\n\n```sh\npython3 examples/trophy_surface.py --backend cuda --device 0 \\\n  --output-directory results/local-trophies/orbital\n```\n''')
    print(f'patches={len(patches)} samples={len(slots)} errors={[float(e) for e in max_errors]} seam={seam_error} normals={normal_error}')

if __name__=='__main__': main()
