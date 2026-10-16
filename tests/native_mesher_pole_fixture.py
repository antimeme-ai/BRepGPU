"""Original analytic pole sectors, including declared distinct-ID endpoints."""
from pathlib import Path
import math
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from brepgpu.imported_source import (
    ImportedSource, Product, SolidDefinition, Usage, Vertex, Edge, Coedge, Loop,
    Face, Carrier, Curve, IDENTITY, write_source,
)

FRAME=(0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.)

def source_case(kind, distinct=False, authoritative=False, reversed_face=False):
    sphere=kind!="cone"
    north=kind=="north"
    alpha=.5
    finite=math.pi/2 if north else -math.pi/2 if sphere else -1/math.sin(alpha)
    far=1. if north else -1. if sphere else finite+1.
    vlo,vhi=(far,finite) if north else (finite,far)
    us=(0.,1.,1.,0.)
    vs=(vlo,vlo,vhi,vhi)
    pole_indices=(2,3) if north else (0,1)
    ids=[300,301,302,303]
    if not distinct:
        ids[pole_indices[1]]=ids[pole_indices[0]]
    def point(u,v,pole=False):
        if pole:
            return (0.,0.,1. if north else -1. if sphere else finite*math.cos(alpha))
        radial=math.cos(v) if sphere else 1.+v*math.sin(alpha)
        return (radial*math.cos(u),radial*math.sin(u),math.sin(v) if sphere else v*math.cos(alpha))
    vertices={}
    for i in range(4):
        xyz=point(us[i],vs[i],i in pole_indices)
        if distinct and i==pole_indices[1]:xyz=(2.**-30,xyz[1],xyz[2])
        vertices[ids[i]]=Vertex(ids[i],xyz,0.)
    curves=[];edges=[];uses=[]
    for i in range(4):
        j=(i+1)%4
        collapsed=i==(2 if north else 0)
        if i in (0,2):
            a,b=us[i],us[j]
            pframe=(0.,vs[i],0.,1.,0.,0.,0.,0.,0.,0.,0.,0.)
            if not collapsed:
                radius=math.cos(vs[i]) if sphere else 1.+vs[i]*math.sin(alpha)
                height=math.sin(vs[i]) if sphere else vs[i]*math.cos(alpha)
                frame=(0.,0.,height,*FRAME[3:])
                curves.append(Curve(2000+i,3,2,frame,(radius,radius),(0.,1.)))
        else:
            a,b=vs[i],vs[j]
            pframe=(us[i],0.,0.,0.,1.,0.,0.,0.,0.,0.,0.,0.)
            radial=(math.cos(us[i]),math.sin(us[i]),0.)
            if sphere:
                frame=(0.,0.,0.,*radial,0.,0.,1.,-radial[1],radial[0],0.)
                curves.append(Curve(2000+i,3,2,frame,(1.,1.),(min(a,b),max(a,b))))
            else:
                direction=(radial[0]*math.sin(alpha),radial[1]*math.sin(alpha),math.cos(alpha))
                frame=(*radial,*direction,0.,0.,0.,0.,0.,0.)
                curves.append(Curve(2000+i,3,1,frame,(0.,0.),(min(a,b),max(a,b))))
        curves.append(Curve(3000+i,2,1,pframe,(0.,0.),(min(a,b),max(a,b))))
        edges.append(Edge(400+i,(ids[i],ids[j]),0 if collapsed else 2000+i,
                          (0.,0.) if collapsed else (a,b),a<b,True,collapsed,0.))
        uses.append(Coedge(500+i,400+i,800,1,3000+i,(min(a,b),max(a,b)),
                            reconstructed=not authoritative))
    return ImportedSource("ab"*32,1.,0.,100,"analytic quotient source family",
        (Product(100,"root","root"),Product(101,kind,kind,200)),
        (SolidDefinition(200,101,(900,),tuple(e.id for e in edges),tuple(sorted(vertices))),),
        (Usage(201,100,101,"sector",IDENTITY),),tuple(vertices[k] for k in sorted(vertices)),
        tuple(edges),tuple(uses),(Loop(800,900,True,(500,501,502,503)),),
        (Face(900,200,9,-1 if reversed_face else 1,(800,),0.),),
        (Carrier(9,4 if sphere else 3,FRAME,(1.,1. if sphere else alpha),(0.,1.,vlo,vhi)),),
        tuple(curves))

def joint_source_case():
    """A regular face sorts before a cap sharing one declared alias component."""
    from dataclasses import replace
    source=source_case("north",distinct=True)
    vertex={v.id:v for v in source.vertices}
    vertex[304]=Vertex(304,(-1.,0.,1.),0.)
    vertex[305]=Vertex(305,(0.,-1.,1.),0.)
    corners=(303,304,305)
    uv=((0.,0.),(1.,0.),(0.,1.))
    edges=list(source.edges);uses=list(source.coedges);curves=list(source.curves)
    for i in range(3):
        j=(i+1)%3
        a,b=vertex[corners[i]].xyz,vertex[corners[j]].xyz
        direction=tuple(b[k]-a[k] for k in range(3))
        curves.append(Curve(2100+i,3,1,(*a,*direction,0.,0.,0.,0.,0.,0.),(0.,0.),(0.,1.)))
        delta=(uv[j][0]-uv[i][0],uv[j][1]-uv[i][1])
        curves.append(Curve(3100+i,2,1,(*uv[i],0.,*delta,0.,0.,0.,0.,0.,0.,0.),(0.,0.),(0.,1.)))
        edges.append(Edge(410+i,(corners[i],corners[j]),2100+i,(0.,1.),True,True,False,0.))
        uses.append(Coedge(510+i,410+i,801,1,3100+i,(0.,1.),reconstructed=True))
    definition=replace(source.definitions[0],face_ids=(899,900),
        edge_ids=tuple(e.id for e in edges),vertex_ids=tuple(sorted(vertex)))
    return replace(source,definitions=(definition,),vertices=tuple(vertex[k] for k in sorted(vertex)),
        edges=tuple(edges),coedges=tuple(uses),curves=tuple(curves),
        loops=(*source.loops,Loop(801,899,True,(510,511,512))),
        faces=(*source.faces,Face(899,200,10,1,(801,),0.)),
        carriers=(*source.carriers,Carrier(10,1,(0.,0.,1.,-1.,0.,0.,0.,-1.,0.,0.,0.,1.),(0.,0.),(0.,1.,0.,1.))))

def main():
    root=Path(sys.argv[1]);root.mkdir(parents=True,exist_ok=True)
    write_source(joint_source_case(),root/"north-joint.bgsource")
    for kind in ("north","south","cone"):
        for distinct,authoritative in ((False,False),(True,False),(False,True)):
            suffix="-distinct" if distinct else "-authoritative" if authoritative else ""
            write_source(source_case(kind,distinct,authoritative),root/f"{kind}{suffix}.bgsource")
        write_source(source_case(kind,reversed_face=True),root/f"{kind}-reversed.bgsource")

if __name__=="__main__":main()
