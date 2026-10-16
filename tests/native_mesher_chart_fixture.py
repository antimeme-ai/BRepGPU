"""Authored geometric chart counterfamilies; original source bytes via writer."""
from dataclasses import replace
from pathlib import Path
import sys
import tempfile

import native_mesher_geometry_fixture as original
from brepgpu.imported_source import Carrier, SurfaceSpan, Curve, Edge, Coedge, Loop, Vertex, load_source, write_source


def main():
    destination = Path(sys.argv[1])
    destination.mkdir(parents=True, exist_ok=True)
    # Reuse the valid source graph/inventory, not an evaluator or expected mesh.
    with tempfile.TemporaryDirectory() as tmp:
        old_argv = sys.argv
        try:
            sys.argv = [__file__, str(Path(tmp) / "base.bgsource")]
            original.main()
        finally:
            sys.argv = old_argv
        source = load_source(Path(tmp) / "base.bgsource")
    # S(u,v)=(3u/2-u²/2,v,0): a regular planar rectangle whose UV
    # interpolation is curved. The correct physical-plane chart has zero height.
    x = (0., .75, 1.)
    vertices = tuple(replace(v, xyz=(v.xyz[0], v.xyz[1], 0.)) for v in source.vertices)
    boundary = (
        ((0., 0., 0.), (.75, 0., 0.), (1., 0., 0.)),
        ((1., 0., 0.), (1., .5, 0.), (1., 1., 0.)),
        ((1., 1., 0.), (.75, 1., 0.), (0., 1., 0.)),
        ((0., 1., 0.), (0., .5, 0.), (0., 0., 0.)),
    )
    curves = tuple(replace(c, degree=2,
                           controls=tuple(value for point in boundary[c.id-2000]
                                          for value in (*point, 1.)))
                   if 2000 <= c.id < 2004 else c for c in source.curves)
    for name, bubble in (("warped-plane", 0.), ("interior-bubble", 1.),
                         ("interior-fold", 0.), ("curved-projection-boundary", 0.),
                         ("parabolic-sheet",0.),("affine-plane",0.)):
        x = (0., 1.5 if name == "interior-fold" else .5 if name=="affine-plane" else .75, 1.)
        net = tuple(value for i in range(3) for j in range(3)
                    for value in (x[i], .25 if name == "curved-projection-boundary"
                                  and i == 1 and j == 0 else j/2.,
                                  (1. if i==2 else 0.) if name=="parabolic-sheet" else
                                  bubble/4. if i == j == 1 else 0., 1.))
        points = tuple(tuple(net[k:k+4]) for k in range(0, len(net), 4))
        boundary_points = ((points[0], points[3], points[6]),
                           (points[6], points[7], points[8]),
                           (points[8], points[5], points[2]),
                           (points[2], points[1], points[0]))
        matching_curves = tuple(replace(c, controls=tuple(value for point in
                                  boundary_points[c.id-2000] for value in point))
                                if 2000 <= c.id < 2004 else c for c in curves)
        carrier = Carrier(9, 6, source.carriers[0].frame, (0., 0.),
                          (0., .125, 0., .125), (2, 2),
                          (0., 1.), (3, 3), (0., 1.), (3, 3), (3, 3), net,
                          (SurfaceSpan((0., 1., 0., 1.), (2, 2), net),))
        corner_indices=(0,6,8,2)
        model_vertices=tuple(replace(v,xyz=points[corner_indices[i]][:3])
                             for i,v in enumerate(vertices))
        model=replace(source,vertices=model_vertices,curves=matching_curves,
                      carriers=(carrier,*source.carriers[1:]))
        write_source(model,destination/f"{name}.bgsource")
        if name=="affine-plane":
            delta=2.**-10
            cycle=((.5,.5),(.5+delta,.5),(.5,.5+delta),(.5,.5))
            prototype=model.curves[0]
            world=Curve(4000,3,4,prototype.frame,(0.,0.),(0.,1.),3,
                        controls=tuple(v for x,y in cycle for v in (x,y,0.,1.)))
            pcurve=Curve(4001,2,4,prototype.frame,(0.,0.),(0.,1.),3,
                         controls=tuple(v for x,y in cycle for v in (x,y,1.)))
            hole=replace(model,vertices=(*model.vertices,Vertex(304,(.5,.5,0.),0.)),
                edges=(*model.edges,Edge(404,(304,304),4000,(0.,1.),True,True,False,0.)),
                coedges=(*model.coedges,Coedge(504,404,801,1,4001,(0.,1.))),
                loops=(*model.loops,Loop(801,900,False,(504,))),
                faces=(replace(model.faces[0],loop_ids=(800,801)),),
                definitions=(replace(model.definitions[0],edge_ids=(*model.definitions[0].edge_ids,404),
                                      vertex_ids=(*model.definitions[0].vertex_ids,304)),),
                curves=(*model.curves,world,pcurve))
            write_source(hole,destination/"tiny-closed-cubic-hole.bgsource")
        if name=="warped-plane":
            exterior=replace(model.vertices[0],xyz=(-2.**-30,0.,0.))
            write_source(replace(model,vertices=(exterior,*model.vertices[1:])),destination/"plane-exterior-node.bgsource")
            use=model.coedges[0]
            mapped=replace(use,parameter_map_kind=2,
                           parameter_map=((0.,.25),(.5,.75),(1.,.5)))
            write_source(replace(model,coedges=(mapped,*model.coedges[1:])),
                         destination/"mapped-guide.bgsource")
            auth=tuple(replace(c,reconstructed=False) for c in model.coedges)
            write_source(replace(model,coedges=auth),destination/"authoritative-joint.bgsource")
            partitioned=replace(auth[0],parameter_map_kind=2,
                                parameter_map=((0.,0.),(.5,.25),(1.,1.)))
            write_source(replace(model,coedges=(partitioned,*auth[1:])),
                         destination/"authoritative-map-partition.bgsource")
            rational_pc=replace(next(c for c in model.curves if c.id==3000),degree=1,
                                controls=(0.,0.,2.,1.,0.,1.))
            rational_ce=replace(next(c for c in model.curves if c.id==2000),degree=2,
                                controls=(0.,0.,0.,4.,1.5,0.,0.,2.,1.,0.,0.,1.))
            rational_model=replace(model,coedges=auth,curves=tuple(
                rational_pc if c.id==3000 else rational_ce if c.id==2000 else c
                for c in model.curves))
            write_source(rational_model,destination/"authoritative-rational-node.bgsource")
            curved_pc=replace(next(c for c in model.curves if c.id==3000),degree=2,
                controls=(0.,0.,1.,.5,1./32.,1.,1.,0.,1.))
            curved_ce=replace(next(c for c in model.curves if c.id==2000),degree=2,
                controls=(0.,0.,0.,1.,.75,1./32.,0.,1.,1.,0.,0.,1.))
            curved_model=replace(model,coedges=auth,curves=tuple(
                curved_pc if c.id==3000 else curved_ce if c.id==2000 else c
                for c in model.curves))
            write_source(curved_model,destination/"authoritative-curved-trim.bgsource")
            write_source(replace(curved_model,coedges=(auth[0],*model.coedges[1:])),
                         destination/"mixed-curved-trim.bgsource")

            write_source(replace(model,coedges=(replace(mapped,reconstructed=False),*auth[1:])),
                         destination/"open-authoritative-joint.bgsource")
            for suffix,curve_scale,surface_scale in (("weighted",1.,1.),("weighted-scaled",8.,16.)):
                original_curve=next(c for c in model.curves if c.id==2000)
                weighted=replace(original_curve,controls=tuple(
                    value*curve_scale*(2. if i==1 else 1.)
                    for i in range(3) for value in original_curve.controls[4*i:4*i+4]))
                scaled_net=tuple(value*surface_scale for value in net)
                scaled_carrier=replace(carrier,controls=scaled_net,
                    spans=(SurfaceSpan((0.,1.,0.,1.),(2,2),scaled_net),))
                write_source(replace(model,curves=tuple(weighted if c.id==2000 else c for c in model.curves),
                                     carriers=(scaled_carrier,*model.carriers[1:])),
                             destination/f"{suffix}-trace.bgsource")
            unknown=replace(use,parameter_map_kind=3,
                            parameter_map=((0.,0.),(1.,1.)))
            write_source(replace(model,coedges=(unknown,*model.coedges[1:])),
                         destination/"unsupported-map.bgsource")
            shifted=replace(use,periodic_shift=(1,0))
            write_source(replace(model,coedges=(shifted,*model.coedges[1:])),
                         destination/"unsupported-nurbs-lift.bgsource")
            degenerate=replace(model.edges[0],degenerate=True)
            for suffix,angle in (("zero",0.),("uncertain",2.**-200)):
                cone=Carrier(9,3,carrier.frame,(1.,angle),(0.,1.,0.,1.))
                write_source(replace(model,edges=(degenerate,*model.edges[1:]),
                                     carriers=(cone,*model.carriers[1:])),
                             destination/f"cone-apex-{suffix}.bgsource")


if __name__ == "__main__":
    main()
