"""Independent authored whole-operation families, without production evaluators."""
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'python'))
from brepgpu.imported_source import (ImportedSource,Product,SolidDefinition,Usage,Vertex,
    Edge,Coedge,Loop,Face,Carrier,Curve,SurfaceSpan,ZERO_FRAME,IDENTITY,write_source)


def make(path,kind,count,*,hint=None,units_mm=1.):
    vertices=[];edges=[];uses=[];loops=[];faces=[];carriers=[];curves=[]
    for index in range(count):
        shift=10000*index
        dx,dy=3.*(index%4),3.*(index//4)
        if kind=='c0':
            degree=1;us=(0.,.5,1.);zs=(0.,.25,0.);weights=(1.,1.,1.)
            knots=(0.,.5,1.);multiplicities=(2,1,2)
        else:
            degree=2;us=(0.,.5,1.);zs=(0.,1.,0.) if kind=='weighted' else (0.,0.,1.)
            weights=(1.,8.,1.) if kind=='weighted' else (1.,1.,1.)
            knots=(0.,1.);multiplicities=(3,3)
        net=tuple(v for u,z,w in zip(us,zs,weights) for y in (0.,1.)
                    for v in ((u+dx)*w,(y+dy)*w,z*w,w))
        if kind=='c0':
            spans=tuple(SurfaceSpan((i/2.,(i+1)/2.,0.,1.),(1,1),
                tuple(v for k in (i,i+1) for y in (0.,1.) for v in (us[k]+dx,y+dy,zs[k],1.))) for i in range(2))
        else:
            spans=(SurfaceSpan((0.,1.,0.,1.),(degree,1),net),)
        carriers.append(Carrier(9+shift,6,ZERO_FRAME,(0.,0.),hint or (0.,1.,0.,1.),
            (degree,1),knots,multiplicities,(0.,1.),(2,2),(3,2),net,spans))
        points=tuple(tuple(net[i:i+4]) for i in range(0,len(net),4))
        boundary=(tuple(points[i] for i in (0,2,4)),tuple(points[i] for i in (4,5)),
                  tuple(points[i] for i in (5,3,1)),tuple(points[i] for i in (1,0)))
        uv=((0.,0.),(1.,0.),(1.,1.),(0.,1.))
        corners=(points[0],points[4],points[5],points[1])
        for i,p in enumerate(corners):
            vertices.append(Vertex(300+shift+i,tuple(p[a]/p[3] for a in range(3)),0.))
            controls=tuple(v for point in boundary[i] for v in point)
            if i in (0,2) and kind=='c0':
                curves.append(Curve(2000+shift+i,3,5,ZERO_FRAME,(0.,0.),(0.,1.),1,
                    knots,multiplicities,controls))
            else:
                curves.append(Curve(2000+shift+i,3,4,ZERO_FRAME,(0.,0.),(0.,1.),
                    degree if i in (0,2) else 1,controls=controls))
            curves.append(Curve(3000+shift+i,2,4,ZERO_FRAME,(0.,0.),(0.,1.),1,
                controls=(*uv[i],1.,*uv[(i+1)%4],1.)))
            edges.append(Edge(400+shift+i,(300+shift+i,300+shift+(i+1)%4),
                2000+shift+i,(0.,1.),True,True,False,0.))
            uses.append(Coedge(500+shift+i,400+shift+i,800+shift,1,3000+shift+i,(0.,1.)))
        loops.append(Loop(800+shift,900+shift,True,tuple(500+shift+i for i in range(4))))
        faces.append(Face(900+shift,200,9+shift,1,(800+shift,),0.))
    translated=list(IDENTITY);translated[3]=16.;translated[11]=2.
    model=ImportedSource('ac'*32,units_mm,0.,100,'independent native whole-operation family',
        (Product(100,'authored assembly','root'),Product(101,kind,'part',200)),
        (SolidDefinition(200,101,tuple(f.id for f in faces),tuple(e.id for e in edges),tuple(v.id for v in vertices)),),
        (Usage(201,100,101,'original',IDENTITY),Usage(202,100,101,'translated',tuple(translated))),
        tuple(vertices),tuple(edges),tuple(uses),tuple(loops),tuple(faces),tuple(carriers),tuple(curves))
    write_source(model,path)


def main():
    destination=Path(sys.argv[1]);destination.mkdir(parents=True,exist_ok=True)
    for kind,count in (('polynomial',12),('weighted',1),('c0',1)):
        make(destination/f'{kind}.bgsource',kind,count)

if __name__=='__main__': main()
