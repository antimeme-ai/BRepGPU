"""Add maximal-degree original controls to the same geometry source cohort."""
from dataclasses import replace
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'python'))
from brepgpu.imported_source import load_source,write_source,SurfaceSpan,Curve,ZERO_FRAME

source=load_source(Path(sys.argv[1]))
carrier=next(c for c in source.carriers if c.id==9)
controls=tuple(x for i in range(4) for j in range(4)
               for x in (i/3.,j/3.,float(i==3)+float(j==3)+i*j/9.,1.))
carrier=replace(carrier,degrees=(3,3),control_shape=(4,4),
                knot_u=(0.,1.),mult_u=(4,4),knot_v=(0.,1.),mult_v=(4,4),
                controls=controls,spans=(SurfaceSpan((0.,1.,0.,1.),(3,3),controls),))
curves=list(source.curves)
for ident,degree,values,weights in ((50000,3,(0.,0.,0.,1.),(1.,)*4),
                                   (50001,3,(0.,2.,-1.,1.),(1.,)*4),
                                   (50002,32,(0.,)*32+(1.,),(1.,)*33),
                                   (50003,1,(0.,1.),(2.,1.))):
    h=tuple(x for value,w in zip(values,weights) for x in (value*w,value*w,w))
    curves.append(Curve(ident,2,4,ZERO_FRAME,(0.,0.),(0.,1.),degree,controls=h))
write_source(replace(source,carriers=tuple(carrier if c.id==9 else c for c in source.carriers),
                     curves=tuple(curves)),Path(sys.argv[2]))
