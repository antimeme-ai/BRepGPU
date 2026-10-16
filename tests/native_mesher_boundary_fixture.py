"""Literal shared-feature source; writer is never part of native meshing."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from brepgpu.imported_source import (
    ImportedSource, Product, SolidDefinition, Vertex, Edge, Coedge, Loop, Face,
    Carrier, Curve, ZERO_FRAME, write_source,
)

def main():
    xyz = ((0.,0.,0.), (1.,0.,0.), (1.,1.,0.), (0.,1.,0.))
    vertices = tuple(Vertex(300+i,p,0.) for i,p in enumerate(xyz))
    edges = tuple(Edge(400+i,(300+i,300+(i+1)%4),600+i,
                       (1.,-1.) if i == 0 else (0.,1.),
                       False, False, False, 0.) for i in range(4))
    curves = []
    for i in range(4):
        a,b = xyz[i],xyz[(i+1)%4]
        origin = tuple((x+y)/2 for x,y in zip(a,b)) if i == 0 else a
        direction = tuple((x-y)/2 for x,y in zip(a,b)) if i == 0 else tuple(y-x for x,y in zip(a,b))
        frame = (*origin,*direction,0.,0.,0.,0.,0.,0.)
        curves.extend((Curve(600+i,3,1,frame,(0.,0.),(-2.,2.)),
                       Curve(700+i,2,1,ZERO_FRAME,(0.,0.),(-2.,2.))))
    uses,loops,faces = [],[],[]
    for face in range(3):
        order = range(4) if face != 1 else (3,2,1,0)
        ids = []
        for i in order:
            identity = 500+4*face+i
            ids.append(identity)
            mapping = ((1.,1.),(.5,.5),(-1.,-1.)) if i == 0 else ((0.,0.),(1.,1.))
            uses.append(Coedge(identity,400+i,800+face,-1 if face == 1 else 1,
                               700+i,(-1.,1.),(0,0),False,2,mapping))
        loops.append(Loop(800+face,900+face,True,tuple(ids)))
        faces.append(Face(900+face,200 if face < 2 else 201,1000,1,(800+face,),0.))
    source = ImportedSource("12"*32,1.,0.,100,"boundary epoch fixture",
        (Product(100,"assembly","root"),Product(101,"shared A","A",200),
         Product(102,"shared B","B",201)),
        (SolidDefinition(200,101,(900,901),tuple(e.id for e in edges),tuple(v.id for v in vertices)),
         SolidDefinition(201,102,(902,),tuple(e.id for e in edges),tuple(v.id for v in vertices))),
        (),vertices,edges,tuple(uses),tuple(loops),tuple(faces),
        (Carrier(1000,1,ZERO_FRAME,(0.,0.),(-2.,2.,-2.,2.)),),tuple(curves),(),())
    write_source(source,sys.argv[1])

if __name__ == "__main__":
    main()
