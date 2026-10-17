"""Literal original-source curve families, independent of producer extraction."""
from pathlib import Path
from dataclasses import replace
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'python'))
from brepgpu.imported_source import Curve,ZERO_FRAME,load_source,write_source
base=load_source(sys.argv[1])
frame=(.25,-.5,.125, 1.,.25,0., .5,1.,.25, 0.,0.,1.)
curves=[
 Curve(6000,3,4,ZERO_FRAME,(0.,0.),(0.,1.),32,controls=tuple(x for i in range(33) for x in (i/32., (i%5)/8., (i%7)/16., 1.))),
 Curve(6001,3,5,ZERO_FRAME,(0.,0.),(0.,1.),2,(-1.,0.,.5,1.,2.),(1,2,1,2,1),tuple(x for i in range(4) for x in (i/4.,i*i/16.,-i/8.,1.))),
 Curve(6002,3,4,ZERO_FRAME,(0.,0.),(0.,1.),3,controls=(0.,0.,0.,1., 1.,2.,0.,2., 2.,-1.,1.,1., 4.,0.,2.,2.)),
 Curve(6003,3,2,frame,(2.,2.),(0.,1.)),
 Curve(6004,3,3,frame,(2.,.5),(-1.,1.)),
]
write_source(replace(base,curves=base.curves+tuple(curves)),sys.argv[2])
