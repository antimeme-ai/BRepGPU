#!/usr/bin/env python3
"""Construct an owned through-bore Boolean and consume its geometry/history."""
import argparse
import csv
from dataclasses import fields, is_dataclass
from fractions import Fraction as F
import hashlib
import math
from pathlib import Path
import sys
import time

from brepgpu import Engine
from brepgpu.body import BodySnapshot, Frame, make_body
from brepgpu.boolean import difference
from brepgpu.boolean_sampling import prepare_grid, sample
from brepgpu.boolean_oracles import qualify_result, qualify_samples, qualify_topology
from brepgpu.adjacency import AdjacencyLibrary, PAIRED, SAME_FACE_PAIR

ROOT=Path(__file__).resolve().parents[1]
SOURCES=('examples/cut_bore.py','python/brepgpu/boolean.py',
    'python/brepgpu/boolean_sampling.py','python/brepgpu/boolean_oracles.py',
    'python/brepgpu/body.py','python/brepgpu/body_sampling.py',
    'python/brepgpu/intersections.py','python/brepgpu/classification.py',
    'python/brepgpu/arrangements.py','python/brepgpu/adjacency.py',
    'python/brepgpu/__init__.py')
RECIPES={'unit':((0.,0.,0.),1.),'small':((2.,-3.,1.),.25),'large':((-4.,2.,-1.),2.)}
KINDS=(('vertex','vertices'),('edge','edges'),('coedge','coedges'),
       ('loop','loops'),('face','faces'),('chart','charts'))


def tsv(path,header,rows):
    with path.open('w',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n')
        writer.writerow(header);writer.writerows(rows)


def require(value,message):
    if not value:raise AssertionError(message)


def tree(path,value):
    """TSV field paths for native immutable records; owning inputs stay references."""
    if type(value) is BodySnapshot:
        yield path,'body_ref',f'{value.body_id}:{value.revision}'
    elif isinstance(value,F):yield path,'rational',str(value)
    elif isinstance(value,float):yield path,'binary64',value.hex()
    elif value is None:yield path,'none',''
    elif isinstance(value,(bool,int,str)):yield path,type(value).__name__,str(value)
    elif isinstance(value,bytes):yield path,'bytes_sha256',hashlib.sha256(value).hexdigest()
    elif isinstance(value,(tuple,list)):
        yield path,'tuple_length',len(value)
        for i,item in enumerate(value):yield from tree(f'{path}/{i}',item)
    elif is_dataclass(value):
        yield path,'record',type(value).__name__
        for field in fields(value):yield from tree(f'{path}/{field.name}',getattr(value,field.name))
    else:raise TypeError('unsupported artifact field '+path+': '+type(value).__name__)


def graph_rows(label,body):
    yield from tree(label+'/frame',body.frame)
    for name in ('vertices','curves','edges','coedges','loops','faces','charts'):
        yield from tree(label+'/'+name,getattr(body,name))



def render(result,batch,out):
    """Display actual native samples and exact boundary curves; no result mesher."""
    solid=result.solid;origin=solid.frame.origin;scale=solid.frame.scale
    def project(point):
        x,y,z=((float(p)-o)/scale for p,o in zip(point,origin))
        return 300+235*(x-y)*.76,270+235*((x+y)*.27-z*.9)
    parts=['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 980 520">',
        '<rect width="980" height="520" fill="#101923"/>',
        '<g font-family="sans-serif" fill="#e6eff7">',
        '<text x="28" y="38" font-size="24">BRepGPU · owned through-bore Boolean</text>',
        f'<text x="28" y="64" font-size="14" fill="#a9bed0">CPU construction · {batch.backend} sampling/adjacency consumer · original source carriers</text>']
    points=[]
    for row in batch.rows:
        if row.native_status==-1:continue
        if sum(a*b for a,b in zip(row.normal,(1,1,1.4)))<=0:continue
        x,y=project(row.position)
        points.append((row.position[0]+row.position[1]+row.position[2],x,y,row.face_id))
    for _,x,y,face in sorted(points):
        color='#5ecfe2' if face==7 else '#7699b4'
        parts.append(f'<circle cx="{x:.3f}" cy="{y:.3f}" r="1.35" fill="{color}" opacity=".55"/>')
    for edge in solid.edges:
        curve=solid.curves[edge.curve_slot]
        count=128 if curve.kind=='circle' else 1
        points=[project(solid.edge_point(edge.id,F(k,count))) for k in range(count+1)]
        path=' '.join(('M' if k==0 else 'L')+f'{x:.3f},{y:.3f}' for k,(x,y) in enumerate(points))
        color='#62d7e7' if curve.kind=='circle' else '#efc47e' if edge.id==15 else '#95aabe'
        parts.append(f'<path d="{path}" stroke="{color}" fill="none" stroke-width="1.8" opacity=".85"/>')
    lines=(('ONE CLOSED GENUS-ONE SHELL', '#62d7e7'),
           ('7 faces / 15 edges / 30 distinct uses','#e6eff7'),
           ('Two annular caps + reversed cutter wall','#a9bed0'),
           ('COMPLETE INPUT → OUTPUT HISTORY','#efc47e'),
           ('85 source entities / 81 output origins','#e6eff7'),
           ('58 retained · 13 modified · 14 deleted','#a9bed0'),
           ('10 generated from exact source events','#a9bed0'))
    for i,(text,color) in enumerate(lines):
        y=150+36*i+(20 if i>=3 else 0)
        parts.append(f'<text x="574" y="{y}" font-size="15" fill="{color}">{text}</text>')
    parts.extend(('<text x="28" y="478" font-size="13" fill="#a9bed0">Exact source-derived boundary wireframe + admitted native surface samples; display study, no result tessellation.</text>',
                  '<text x="28" y="500" font-size="13" fill="#a9bed0">Cyan: generated rims / inward wall. Gold: cropped source seam with two distinct same-face uses.</text>',
                  '</g></svg>'))
    (out/'through-bore.svg').write_text('\n'.join(parts)+'\n')


def execute(args):
    require(sys.platform=='linux','product geometry executes on the fleet')
    require(2<=args.grid<=65,'caller grid must be in 2..65')
    pins=tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES)
    planned_native=(Path(args.library_path).resolve(),Path(args.topology_library).resolve())
    if args.backend=='cuda':planned_native+=(planned_native[0].with_name('libbrepgpu_cuda.so'),)
    native_before=tuple((path.name,hashlib.sha256(path.read_bytes()).hexdigest()) for path in planned_native)
    started=time.perf_counter_ns()
    origin,scale=RECIPES[args.recipe]
    left=make_body('box',Frame(origin,scale))
    right=make_body('cylinder',Frame(origin,2*scale))
    operand_ms=(time.perf_counter_ns()-started)/1e6
    started=time.perf_counter_ns();previous=started;stages=[]
    def phase(name):
        nonlocal previous
        now=time.perf_counter_ns();stages.append((name,(now-previous)/1e6));previous=now
    result=difference(left,right);solid=result.solid
    require(result.left is left and result.right is right,'cut replaced original operands')
    phase('cpu_difference')
    # Actual callers read queries/history, rather than only count records.
    local_probes=((F(0),F(0),F(0)),(F(5,8),F(0),F(0)),(F(1,2),F(0),F(0)),
                  (F(1,2),F(0),F(1,2)),(F(3,4),F(3,4),F(1,2)),(F(1),F(0),F(0)))
    material=[]
    for local in local_probes:
        point=tuple(F(o)+F(scale)*x for o,x in zip(origin,local))
        answer=solid.classify_point(point)
        material.append((point,answer.label,answer.boundary_faces))
    parameters=(F(0),F(1,8),F(1,4),F(1,2),F(3,4),F(1))
    boundary=tuple((edge.id,t,solid.edge_point(edge.id,t)) for edge in solid.edges for t in parameters)
    uv=tuple((coedge.id,t,solid.coedge_uv(coedge.id,t)) for coedge in solid.coedges for t in parameters)
    require(all(result.history.from_input(row.source)==row for row in result.history.records),'history forward lookup differs')
    require(all(result.history.origin(row.target)==row for row in result.history.origins),'history reverse lookup differs')
    phase('material_boundary_history_consume')
    request=prepare_grid(result,args.grid)
    phase('request_prepare')
    with Engine(backend=args.backend,device=args.device,threads=args.threads,library_path=args.library_path) as engine:
        core=Path(engine._lib._name).resolve()
        phase('engine_setup')
        batch=sample(engine,request)
        require(batch.result is result and batch.request is request,'sampling owner differs')
        require(batch.backend==args.backend and batch.device==args.device,'sampling placement differs')
        checksum=math.fsum(x for row in batch.rows if row.native_status!=-1 for x in (*row.position,*row.normal))
        require(math.isfinite(checksum),'nonfinite consumed sample checksum')
        phase('sample_and_consume')
    phase('engine_cleanup')
    topology=solid.topology_input()
    library=AdjacencyLibrary(args.topology_library)
    phase('topology_pack_library_load')
    adjacency=library.build(topology,backend=args.backend,device=args.device,threads=args.threads)
    phase('adjacency_build')
    edge_rows=[];use_rows=[]
    for slot,source in enumerate(topology.edge_sources):
        uses=adjacency.edge_uses(slot);kind=adjacency.edge_kind(slot)
        require(kind in (PAIRED,SAME_FACE_PAIR) and len(uses)==2,'closed cut edge lacks opposed pair')
        edge_rows.append((slot,source.entity_id,kind,','.join(str(use.entity_id) for use in uses)))
    for slot,source in enumerate(topology.coedge_sources):
        mate=adjacency.mate(slot);neighbor=adjacency.neighbor(slot)
        require(mate is not None and neighbor is not None,'closed cut has missing mate/neighbor')
        require((mate.body_id,mate.revision)==(solid.body_id,solid.revision),'mate escaped output namespace')
        require((neighbor.body_id,neighbor.revision)==(solid.body_id,solid.revision),'neighbor escaped output namespace')
        use_rows.append((slot,source.entity_id,mate.entity_id,neighbor.entity_id))
    phase('actual_adjacency_reader')
    complete_ms=(time.perf_counter_ns()-started)/1e6
    qualification_start=time.perf_counter_ns()
    quality={**{'result_'+k:v for k,v in qualify_result(result).items()},
             **{'samples_'+k:v for k,v in qualify_samples(batch).items()},
             **{'topology_'+k:v for k,v in qualify_topology(result,adjacency).items()}}
    qualification_ms=(time.perf_counter_ns()-qualification_start)/1e6
    native_paths=(core,Path(args.topology_library))
    if args.backend=='cuda':native_paths+=(core.with_name('libbrepgpu_cuda.so'),)
    require(tuple(path.resolve() for path in native_paths)==planned_native,'loaded native paths differ from pinned request')
    native_pins=tuple((path.name,hashlib.sha256(path.read_bytes()).hexdigest()) for path in native_paths)
    require(native_pins==native_before,'native library changed during operation')
    out=args.output_directory;out.mkdir(parents=True,exist_ok=False)
    artifact_start=time.perf_counter_ns()
    tsv(out/'recipe.tsv',('field','value'),(('recipe',args.recipe),('backend',args.backend),('device',args.device),
        ('threads',args.threads),('grid',args.grid),('left_body',left.body_id),('right_body',right.body_id),('output_body',solid.body_id),
        *((f'origin_{axis}_hex',x.hex()) for axis,x in zip('xyz',origin)),('scale_hex',scale.hex()),
        ('volume_rational',solid.volume.rational),('volume_pi',solid.volume.pi),
        ('observed_backend',batch.backend),('observed_device',batch.device),('samples',batch.count),
        ('evaluated_samples',batch.evaluated_count),('checksum',checksum),('constructor','owned CPU exact through-bore'),
        ('native_consumers','first jets/oriented normals and edge-use adjacency'),('general_result_mesher','not implemented')))
    tsv(out/'sources.tsv',('path','sha256'),pins)
    tsv(out/'native-libraries.tsv',('file','sha256'),native_pins)
    tsv(out/'quality.tsv',('field','value'),sorted(quality.items()))
    def graph():
        for label,body in (('left',left),('right',right),('output',solid)):
            yield from graph_rows(label,body)
    tsv(out/'graph.tsv',('path','type','value'),graph())
    tsv(out/'operand-tables.tsv',('operand','table','bytes','sha256'),
        ((label,name,len(data),hashlib.sha256(data).hexdigest())
         for label,body in (('left',left),('right',right)) for name,data in body.packed_tables))
    for name in ('coedge_edges','coedge_faces','orientations'):
        (out/(name+('.i32le' if name=='orientations' else '.u32le'))).write_bytes(getattr(topology,name))
    for name in ('offsets','members','kinds','mates','neighbors'):
        (out/(name+'.u32le')).write_bytes(getattr(adjacency,name))
    tsv(out/'sections.tsv',('path','type','value'),tree('sections',result.sections))
    tsv(out/'arrangements.tsv',('path','type','value'),tree('arrangements',result.arrangements))
    tsv(out/'selections.tsv',('path','type','value'),tree('selections',result.selections))
    tsv(out/'history-forward.tsv',('path','type','value'),tree('records',result.history.records))
    tsv(out/'history-reverse.tsv',('path','type','value'),tree('origins',result.history.origins))
    tsv(out/'material.tsv',('probe','x','y','z','label','boundary_faces'),
        ((i,*point,label,','.join(map(str,faces))) for i,(point,label,faces) in enumerate(material)))
    tsv(out/'edge-points.tsv',('edge_id','t','x','y','z'),((eid,t,*p) for eid,t,p in boundary))
    tsv(out/'coedge-uv.tsv',('coedge_id','t','u','v'),((cid,t,*p) for cid,t,p in uv))
    tsv(out/'samples.tsv',('index','slot','face','chart','u_hex','v_hex','trim','native_status','x','y','z','nx','ny','nz'),
        ((row.index,row.patch_slot,row.face_id,row.chart_id,row.u.hex(),row.v.hex(),row.trim,row.native_status,*row.position,*row.normal) for row in batch.rows))
    tsv(out/'adjacency-edges.tsv',('slot','edge_id','kind','coedge_ids'),edge_rows)
    tsv(out/'adjacency-uses.tsv',('slot','coedge_id','mate_id','neighbor_face_id'),use_rows)
    render(result,batch,out)
    tsv(out/'costs.tsv',('phase','wall_ms'),(('operand_preparation_outside',operand_ms),*stages,
        *((f'construction/{name}',value) for name,value in result.timings),
        *((f'sampling/{name}',value) for name,value in batch.timings),
        *((f'adjacency/{name}',value) for name,value in adjacency.timings),
        ('complete_cut_to_consumed_cleanup',complete_ms),('independent_qualification',qualification_ms),
        ('artifact_writes_except_costs', (time.perf_counter_ns()-artifact_start)/1e6)))
    require(pins==tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES),'owned source changed during caller')
    print(f'{args.recipe} {args.backend}:{args.device} samples={batch.count} evaluated={batch.evaluated_count} complete_ms={complete_ms:.6f} qualification_ms={qualification_ms:.6f}',flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--recipe',choices=tuple(RECIPES),default='unit')
    p.add_argument('--backend',choices=('cpu','cuda'),default='cpu')
    p.add_argument('--device',type=int,default=0)
    p.add_argument('--threads',type=int,default=1)
    p.add_argument('--grid',type=int,default=17)
    p.add_argument('--library-path',type=Path,default=Path('build/pair-release/libbrepgpu.so'))
    p.add_argument('--topology-library',type=Path,default=Path('build/topology-release/libbrepgpu_topology.so'))
    p.add_argument('--output-directory',type=Path,required=True)
    execute(p.parse_args())
