"""Independent analytic original-source coefficient families for adaptive kernels."""
from dataclasses import replace
from pathlib import Path
import sys
import tempfile
import native_mesher_geometry_fixture as base
from native_mesher_run_fixture import make
from brepgpu.imported_source import Carrier,SurfaceSpan,load_source,write_source


def main():
    destination=Path(sys.argv[1]);destination.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory() as temporary:
        previous=sys.argv
        try:
            sys.argv=[__file__,str(Path(temporary)/'base.bgsource')]
            base.main()
        finally:
            sys.argv=previous
        source=load_source(Path(temporary)/'base.bgsource')
    weights=(1e-300,1.,1e-300)
    controls=tuple(value for i in range(3) for j in range(2)
                   for value in (weights[i]*i/2.,weights[i]*j,0.,weights[i]))
    fallback=Carrier(9,6,source.carriers[0].frame,(0.,0.),(0.,1.,0.,1.),
                     (2,1),(0.,1.),(3,3),(0.,1.),(2,2),(3,2),controls,
                     (SurfaceSpan((0.,1.,0.,1.),(2,1),controls),))
    write_source(replace(source,carriers=(fallback,)),destination/'fallback-weights.bgsource')
    with tempfile.TemporaryDirectory() as temporary:
        authored=Path(temporary)/'weighted.bgsource';make(authored,'weighted',1)
        weighted=load_source(authored)
    write_source(weighted,destination/'extrusion-u.bgsource')
    c=weighted.carriers[0]
    transposed=tuple(value for j in range(2) for i in range(3)
                     for value in c.controls[(i*2+j)*4:(i*2+j+1)*4])
    swapped=replace(c,degrees=(1,2),control_shape=(2,3),
                    knot_u=c.knot_v,knot_v=c.knot_u,mult_u=c.mult_v,mult_v=c.mult_u,
                    controls=transposed,spans=(SurfaceSpan((0.,1.,0.,1.),(1,2),transposed),))
    write_source(replace(weighted,carriers=(swapped,)),destination/'extrusion-v.bgsource')
    elevated=tuple(value for i in range(3) for j in range(3)
                   for value in ((0.,.5,1.)[i]*(1.,8.,1.)[i],
                                 j/2.*(1.,8.,1.)[i],(0.,1.,0.)[i]*(1.,8.,1.)[i],(1.,8.,1.)[i]))
    c2=replace(c,degrees=(2,2),control_shape=(3,3),knot_v=(0.,1.),mult_v=(3,3),
               controls=elevated,spans=(SurfaceSpan((0.,1.,0.,1.),(2,2),elevated),))
    write_source(replace(weighted,carriers=(c2,)),destination/'extrusion-elevated.bgsource')
    ordinary=(3.,-2.,4.,2.,0.,1.,.5,1.,0.,.25,.5,3.)
    for kind in range(1,8):
        scale=1. if kind<=5 else 1e200 if kind==6 else 1e-200
        frame=ordinary if kind<=5 else tuple(x*scale for x in ordinary)
        actual_kind=kind if kind<=5 else 1
        carrier=Carrier(9,actual_kind,frame,(2.,.25),(0.,1.,0.,1.))
        write_source(replace(source,carriers=(carrier,)),destination/f'analytic-{kind}.bgsource')

if __name__=='__main__':
    main()
