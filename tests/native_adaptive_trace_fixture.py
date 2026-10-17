"""Authored source variants for trace mapping/joint/source-guard fault classes."""
from dataclasses import replace
from pathlib import Path
import math
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'python'))
from brepgpu.imported_source import (load_source, write_source, ImportedSource,
    Product, SolidDefinition, Vertex, Edge, Coedge, Loop, Face, Carrier, Curve,
    ZERO_FRAME, IDENTITY)


def analytic_source(kind):
    # Literal original iso curves on a dyadic, nonorthogonal frame. The bad
    # reconstructed guide has the same endpoints and a different interior clock.
    frame = (0., 0., 0., 1., .25, 0., 0., 1., .125, .125, 0., 1.)
    origin, x, y, z = (frame[i:i+3] for i in (0, 3, 6, 9))
    radius, other = (-1., 0.) if kind == 4 else (-2., -.5) if kind == 5 else (1., .4)
    lo, hi = .25, .75
    def add(a, b): return tuple(i+j for i, j in zip(a, b))
    def scale(a, t): return tuple(i*t for i in a)
    def radial(v):
        return radius if kind == 2 else radius+v*math.sin(other) if kind == 3 else radius*math.cos(v) if kind == 4 else radius+other*math.cos(v)
    def axial(v):
        return v if kind == 2 else v*math.cos(other) if kind == 3 else radius*math.sin(v) if kind == 4 else other*math.sin(v)
    def direction(u): return add(scale(x, math.cos(u)), scale(y, math.sin(u)))
    def point(u, v): return add(origin, add(scale(direction(u), radial(v)), scale(z, axial(v))))
    uv = ((lo, lo), (hi, lo), (hi, hi), (lo, hi))
    curves, edges, uses = [], [], []
    for i in range(4):
        varying_u = i % 2 == 0
        fixed = uv[i][1 if varying_u else 0]
        if varying_u:
            center = add(origin, scale(z, axial(fixed)))
            cframe = (*center, *x, *y, 0., 0., 0.)
            curve = Curve(2000+i, 3, 2, cframe, (radial(fixed),)*2, (lo, hi))
        elif kind in (2, 3):
            e = direction(fixed)
            center = add(origin, scale(e, radius))
            d = z if kind == 2 else add(scale(e, math.sin(other)), scale(z, math.cos(other)))
            curve = Curve(2000+i, 3, 1, (*center, *d, 0., 0., 0., 0., 0., 0.), (0., 0.), (lo, hi))
        else:
            e = direction(fixed)
            center = origin if kind == 4 else add(origin, scale(e, radius))
            r = radius if kind == 4 else other
            curve = Curve(2000+i, 3, 2, (*center, *e, *z, 0., 0., 0.), (r, r), (lo, hi))
        a = (lo, fixed) if varying_u else (fixed, lo)
        b = (hi, fixed) if varying_u else (fixed, hi)
        controls = (*a, 1., *a, 1., *b, 1.) if i == 0 else (*a, 1., *b, 1.)
        degree = 2 if i == 0 else 1
        guide = Curve(3000+i, 2, 5, ZERO_FRAME, (0., 0.), (lo, hi), degree,
                      (lo, hi), (degree+1, degree+1), controls)
        va, vb = (i, (i+1)%4) if i < 2 else ((i+1)%4, i)
        edges.append(Edge(400+i, (300+va, 300+vb), 2000+i, (lo, hi), True, True, False, 0.))
        uses.append(Coedge(500+i, 400+i, 800, 1 if i < 2 else -1, 3000+i, (lo, hi), reconstructed=True))
        curves.extend((curve, guide))
    return ImportedSource('ab'*32, 1., 0., 100, 'analytic reconstructed inverse proposals',
        (Product(100, 'root', 'root', 200),),
        (SolidDefinition(200, 100, (900,), tuple(e.id for e in edges), (300, 301, 302, 303)),), (),
        tuple(Vertex(300+i, point(*p), 0.) for i, p in enumerate(uv)), tuple(edges), tuple(uses),
        (Loop(800, 900, True, tuple(u.id for u in uses)),), (Face(900, 200, 9, 1, (800,), 0.),),
        (Carrier(9, kind, frame, (radius, other), (lo, hi, lo, hi)),), tuple(curves))


def main():
    base = load_source(sys.argv[1])
    out = Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    def save(name, source):
        write_source(source, out / f'{name}.bgsource')
    for kind in (2, 3, 4, 5):
        analytic = analytic_source(kind)
        save(f'locator-{kind}', analytic)
        if kind == 3:
            save('locator-authoritative', replace(analytic, coedges=tuple(replace(u, reconstructed=False) for u in analytic.coedges)))
            save('locator-mismatch', replace(analytic, vertices=tuple(replace(v, xyz=(v.xyz[0], v.xyz[1], v.xyz[2]+.125)) for v in analytic.vertices)))
    edge = replace(base.edges[0], vertex_ids=base.edges[0].vertex_ids[::-1],
                   parameter_range=(1., 0.), curve_same_sense=False, same_parameter=False)
    use = replace(base.coedges[0], sense=-1, parameter_map_kind=2,
                  parameter_map=((1.,1.),(.5,.25),(0.,0.)))
    curves = tuple(replace(c, kind=5, degree=1, knots=(0.,.25,1.),
                           multiplicities=(2,1,2), controls=(0.,0.,1., .5,0.,1., 1.,0.,1.))
                   if c.id == 3000 else c for c in base.curves)
    mapped = replace(base, edges=(edge, *base.edges[1:]),
                     coedges=(use, *base.coedges[1:]), curves=curves)
    save('maps', mapped)
    save('map3', replace(mapped, coedges=(replace(use, parameter_map_kind=3), *mapped.coedges[1:])))
    reconstructed = replace(base,coedges=tuple(replace(u,reconstructed=True) for u in base.coedges))
    tiny = 2.**-40
    proposed = tuple(replace(c,controls=(1.+tiny,0.,1.,1.+tiny,1.,1.))
                     if c.id == 3001 else c for c in reconstructed.curves)
    save('natural-proposal',replace(reconstructed,curves=proposed))
    def shifted_controls(curve):
        values = list(curve.controls)
        stride = curve.dimension+1
        for i in range(0,len(values),stride): values[i] += .125*values[i+stride-1]
        return replace(curve,controls=tuple(values))
    save('natural-sourceguard',replace(reconstructed,
        vertices=tuple(replace(v,xyz=(v.xyz[0]+.125,*v.xyz[1:])) for v in reconstructed.vertices),
        curves=tuple(shifted_controls(c) for c in reconstructed.curves)))
    auth = tuple(replace(u, reconstructed=False) for u in base.coedges)
    save('auth',replace(base,coedges=auth))
    outside=tuple(replace(c,kind=4,degree=3,knots=(),multiplicities=(),
                          controls=(1.,0.,1., 1.,4.,1., 1.,4.,1., 1.,1.,1.))
                  if c.id==3001 else c for c in base.curves)
    save('authoutside',replace(base,coedges=auth,curves=outside))
    curves = tuple(replace(c, controls=(.125,*c.controls[1:])) if c.id==3000 else c
                   for c in base.curves)
    save('authjoint', replace(base, coedges=auth, curves=curves))
    unit = (0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.)
    plane = Carrier(9,1,unit,(0.,0.),(0.,1.,0.,1.))
    save('sourceguard', replace(base, carriers=(plane,*base.carriers[1:])))
    uv = ((0.,0.),(1.,0.),(1.,1.),(0.,1.))
    curves=[]
    for i in range(4):
        a,b=uv[i],uv[(i+1)%4]
        controls = ((0.,0.,0.,1.)*3+(1.,0.,0.,1.)) if i==0 else (*a,0.,1.,*b,0.,1.)
        curves.append(Curve(2000+i,3,4,ZERO_FRAME,(0.,0.),(0.,1.),3 if i==0 else 1,
                            controls=controls))
        curves.append(Curve(3000+i,2,4,ZERO_FRAME,(0.,0.),(0.,1.),1,
                            controls=(*a,1.,*b,1.)))
    speed=replace(base, vertices=tuple(Vertex(300+i,(*p,0.),0.) for i,p in enumerate(uv)),
                  carriers=(plane,*base.carriers[1:]),curves=tuple(curves))
    save('speed',speed)
    for name, offset in (('plane-foot-outside', -2.**-45), ('plane-foot-mismatch', -.125)):
        def translated_spatial(curve):
            if curve.dimension != 3:
                return curve
            values = list(curve.controls)
            for i in range(0, len(values), 4):
                values[i] += offset*values[i+3]
            return replace(curve, controls=tuple(values))
        save(name, replace(speed,
            vertices=tuple(replace(v, xyz=(v.xyz[0]+offset,*v.xyz[1:])) for v in speed.vertices),
            curves=tuple(translated_spatial(c) for c in speed.curves)))
    # Same complete UV line image, a quadratic raw clock and cubic spatial
    # clock. Whole-use image equality must avoid same-t flatten refinement.
    authspeed=tuple(replace(c,degree=2,controls=(0.,0.,1.,0.,0.,1.,1.,0.,1.))
                    if c.id==3000 else c for c in speed.curves)
    save('authspeed',replace(speed,curves=authspeed,
         coedges=tuple(replace(u,reconstructed=False) for u in speed.coedges)))
    bad=Curve(2000,3,5,ZERO_FRAME,(0.,0.),(0.,1.),1,(0.,.5,1.),(2,1,2),
              (0.,0.,0.,1., .5,0.,1.,1., 1.,0.,0.,1.))
    save('sourceguard',replace(speed,curves=(bad,*speed.curves[1:])))
    tau = 6.283185307179586
    # One physical vertical edge occurs twice in a literal UV rectangle.
    edges=(Edge(400,(300,300),2000,(0.,tau),True,True,False,0.),
           Edge(401,(301,301),2001,(0.,tau),True,True,False,0.),
           Edge(402,(300,301),2002,(0.,1.),False,True,False,0.))
    circle = lambda i,z: Curve(i,3,2,(0.,0.,z,1.,0.,0.,0.,1.,0.,0.,0.,1.),
                               (1.,1.),(0.,tau))
    guide = lambda i,o,x,domain: Curve(i,2,1,(*o,0.,*x,0.,0.,0.,0.,0.,0.,0.),
                                      (0.,0.),domain)
    curves=(circle(2000,0.),circle(2001,1.),Curve(2002,3,1,
            (1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,0.,0.),(0.,0.),(0.,1.)),
            guide(3000,(0.,0.),(1.,0.),(0.,tau)),
            guide(3001,(tau,0.),(0.,1.),(0.,1.)),
            guide(3002,(0.,1.),(1.,0.),(0.,tau)),
            guide(3003,(0.,0.),(0.,1.),(0.,1.)))
    uses=(Coedge(500,400,800,1,3000,(0.,tau)),Coedge(501,402,800,1,3001,(0.,1.)),
          Coedge(502,401,800,-1,3002,(0.,tau)),Coedge(503,402,800,-1,3003,(0.,1.)))
    seam=ImportedSource('ab'*32,1.,0.,100,'literal seam trace fixture',
        (Product(100,'root','root',200),),(SolidDefinition(200,100,(900,),(400,401,402),(300,301)),),(),
        (Vertex(300,(1.,0.,0.),0.),Vertex(301,(1.,0.,1.),0.)),edges,uses,
        (Loop(800,900,True,(500,501,502,503)),),(Face(900,200,9,1,(800,),0.),),
        (Carrier(9,2,unit,(1.,0.),(0.,tau,0.,1.)),),curves)
    save('seam',seam)
    shifted = tuple(replace(c,frame=(0.,0.,0.,0.,1.,0.,0.,0.,0.,0.,0.,0.)) if c.id==3001 else c
                    for c in curves)
    save('lift',replace(seam,curves=shifted,
                        coedges=(uses[0],replace(uses[1],periodic_shift=(1,0)),*uses[2:])))
    # Runtime integration needs an actually curved rational boundary, not a
    # dummy restriction count on analytic circles that have no control net.
    h3 = (0.,0.,0.,1., .2,-.2,0.,1., .8,-.2,0.,1., 1.,0.,0.,1.)
    h2 = (0.,0.,1., .2,-.2,1., .8,-.2,1., 1.,0.,1.)
    bent = tuple(replace(c, degree=3, controls=h3) if c.id==2000 else
                 replace(c, degree=3, controls=h2) if c.id==3000 else c
                 for c in speed.curves)
    save('runtime-curved', replace(speed, curves=bent))
    wire = Edge(404,(300,301),4000,(0.,1.),True,True,False,0.)
    wirecurve = Curve(4000,3,1,(0.,0.,.125,1.,0.,0.,0.,0.,0.,0.,0.,0.),(0.,0.),(0.,1.))
    save('runtime-wire-sourceguard',replace(speed,edges=(*speed.edges,wire),
        curves=(*speed.curves,wirecurve),definitions=(replace(speed.definitions[0],
        edge_ids=(*speed.definitions[0].edge_ids,404)),)))
    equality = tuple(replace(c,degree=2,controls=(0.,0.,0.,1.,.5,1.,0.,1.,1.,0.,0.,1.)) if c.id==2000 else
                     replace(c,degree=2,controls=(0.,0.,1.,.5,1.,1.,1.,0.,1.)) if c.id==3000 else c
                     for c in speed.curves)
    save('runtime-equality',replace(speed,curves=equality))
    print('trace_source_variants=23')


if __name__ == '__main__':
    main()
