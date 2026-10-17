"""Original three-use seam wedges for the bound-only cap numerical operation."""
from dataclasses import replace
from pathlib import Path
import math
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from brepgpu.imported_source import (Carrier, Coedge, Curve, Edge, Face, Loop,
                                    Vertex, write_source)
from native_mesher_pole_fixture import source_case


def cap_source(kind, warped=False):
    base = source_case(kind)
    sphere = kind != "cone"
    finite = (math.pi/2 if kind == "north" else -math.pi/2) if sphere else -1/math.sin(.5)
    far = finite + (-.5 if kind == "north" else .5)
    origin = (2.**25, -2.**24, 3.) if warped else (0., 0., 0.)
    x = (1., .25, 0.) if warped else (1., 0., 0.)
    y = (.125, 1., .25) if warped else (0., 1., 0.)
    z = (.25, .5, 128.) if warped else (0., 0., 1.)
    frame = (*origin, *x, *y, *z)
    def point(radial, axial):
        return tuple(origin[a]+radial*x[a]+axial*z[a] for a in range(3))
    radial = math.cos(far) if sphere else 1.+far*math.sin(.5)
    axial = math.sin(far) if sphere else far*math.cos(.5)
    pole = point(0., 1. if kind == "north" else -1. if sphere else finite*math.cos(.5))
    regular = point(radial, axial)
    lo, hi = sorted((finite, far))
    if sphere:
        cross = (x[1]*z[2]-x[2]*z[1], x[2]*z[0]-x[0]*z[2], x[0]*z[1]-x[1]*z[0])
        meridian = Curve(2000, 3, 2, (*origin,*x,*z,*cross), (1.,1.), (lo,hi))
    else:
        start = point(1.,0.)
        direction = tuple(math.sin(.5)*x[a]+math.cos(.5)*z[a] for a in range(3))
        meridian = Curve(2000, 3, 1, (*start,*direction,0.,0.,0.,0.,0.,0.), (0.,0.), (lo,hi))
    base_origin = tuple(origin[a]+axial*z[a] for a in range(3))
    circle = Curve(2001, 3, 2, (*base_origin,*x,*y,*z), (radial,radial), (0.,math.tau))
    left = Curve(3000, 2, 1, (0.,0.,0.,0.,1.,0.,0.,0.,0.,0.,0.,0.), (0.,0.), (lo,hi))
    bottom = Curve(3001, 2, 1, (0.,far,0.,1.,0.,0.,0.,0.,0.,0.,0.,0.),
                   (0.,0.), (0.,math.tau))
    # Nonconstant ORIGINAL U controls; later chosen-trace production must not
    # pretend this guide itself is constant-U or overwrite its source bytes.
    controls = tuple(value for i,du in enumerate((0.,.005,-.003,0.))
                     for value in (math.tau+du, lo+(hi-lo)*i/3, 1.))
    right = Curve(3002, 2, 5, (0.,)*12, (0.,0.), (lo,hi), 3,
                  (lo,hi), (4,4), controls)
    edges = (Edge(400,(300,301),2000,(finite,far),far>finite,True,False,0.),
             Edge(401,(301,301),2001,(0.,math.tau),True,True,False,0.))
    uses = (Coedge(500,400,800,1,3000,(lo,hi),reconstructed=True),
            Coedge(501,401,800,1,3001,(0.,math.tau),reconstructed=True),
            Coedge(502,400,800,-1,3002,(lo,hi),reconstructed=True))
    return replace(base, source_sha256=("ce" if warped else "cd")*32,
        vertices=(Vertex(300,pole,0.),Vertex(301,regular,0.)), edges=edges,coedges=uses,
        loops=(Loop(800,900,True,(500,501,502)),), faces=(Face(900,200,9,1,(800,),0.),),
        curves=(meridian,circle,left,bottom,right),
        carriers=(Carrier(9,4 if sphere else 3,frame,(1.,1. if sphere else .5),
                          (0.,math.tau,lo-.125,hi+.125)),),
        definitions=(replace(base.definitions[0],face_ids=(900,),edge_ids=(400,401),vertex_ids=(300,301)),))


def main():
    root = Path(sys.argv[1]); root.mkdir(parents=True, exist_ok=True)
    for kind in ("north","south","cone"):
        for warped in (False,True):
            write_source(cap_source(kind,warped),root/f"{kind}{'-warped' if warped else ''}.bgsource")


if __name__ == "__main__":
    main()
