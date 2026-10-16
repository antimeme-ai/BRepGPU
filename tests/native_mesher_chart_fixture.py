"""Authored geometric chart counterfamilies; original source bytes via writer."""
from dataclasses import replace
from pathlib import Path
import sys
import tempfile

import native_mesher_geometry_fixture as original
from brepgpu.imported_source import Carrier, SurfaceSpan, load_source, write_source


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
                         ("interior-fold", 0.), ("curved-projection-boundary", 0.)):
        x = (0., 1.5 if name == "interior-fold" else .75, 1.)
        net = tuple(value for i in range(3) for j in range(3)
                    for value in (x[i], .25 if name == "curved-projection-boundary"
                                  and i == 1 and j == 0 else j/2.,
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
        model=replace(source,vertices=vertices,curves=matching_curves,
                      carriers=(carrier,*source.carriers[1:]))
        write_source(model,destination/f"{name}.bgsource")
        if name=="warped-plane":
            use=model.coedges[0]
            mapped=replace(use,parameter_map_kind=2,
                           parameter_map=((0.,.25),(.5,.75),(1.,.5)))
            write_source(replace(model,coedges=(mapped,*model.coedges[1:])),
                         destination/"mapped-guide.bgsource")
            auth=tuple(replace(c,reconstructed=False) for c in model.coedges)
            write_source(replace(model,coedges=auth),destination/"authoritative-joint.bgsource")
            write_source(replace(model,coedges=(replace(mapped,reconstructed=False),*auth[1:])),
                         destination/"open-authoritative-joint.bgsource")
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
