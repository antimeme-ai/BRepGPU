#!/usr/bin/env python3
"""Construct and consume a complete signed sharp-cylinder offset on CPU/P100."""
import argparse
import csv
from fractions import Fraction as F
import hashlib
import math
from pathlib import Path
import sys
import time

from brepgpu import Engine
from brepgpu.body import Frame,make_body
from brepgpu.offset import offset_solid
from brepgpu.offset_sampling import prepare_grid,sample
from brepgpu.offset_oracles import qualify_result,qualify_samples,qualify_topology
from brepgpu.adjacency import AdjacencyLibrary,PAIRED,SAME_FACE_PAIR
from cut_bore import tsv,require,tree,graph_rows

ROOT=Path(__file__).resolve().parents[1]
SOURCES=('examples/offset_cylinder.py','examples/cut_bore.py','python/brepgpu/offset.py',
    'python/brepgpu/offset_sampling.py','python/brepgpu/offset_oracles.py',
    'python/brepgpu/boolean.py','python/brepgpu/boolean_sampling.py',
    'python/brepgpu/body.py','python/brepgpu/body_sampling.py','python/brepgpu/body_oracles.py',
    'python/brepgpu/adjacency.py','python/brepgpu/adjacency_oracles.py','python/brepgpu/__init__.py')
RECIPES={'inset':(F(-1,8),(0.,0.,0.),1.),
    'outset':(F(1,8),(2.,-3.,1.),.5),
    'odd':(F(17,256),(-2.,1.,.5),2.)}


def render(result,batch,out):
    """Actual exact source/result curves with native output surface samples."""
    solid=result.solid;origin=solid.frame.origin;scale=solid.frame.scale
    def project(point):
        x,y,z=((float(p)-o)/scale for p,o in zip(point,origin))
        return 300+300*.76*(x-y),280+300*(.27*(x+y)-.9*z)
    parts=['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1050 570">',
        '<rect width="1050" height="570" fill="#101923"/>','<g font-family="sans-serif" fill="#e6eff7">',
        '<text x="28" y="38" font-size="24">BRepGPU · signed analytic cylinder offset</text>',
        f'<text x="28" y="64" font-size="14" fill="#a9bed0">CPU feature construction · {batch.backend} native consumers · sharp intersection joins</text>']
    for e in result.source.edges:
        steps=128 if e.id<3 else 1
        points=[]
        for i in range(steps+1):
            # The original body reader is binary64; overlay reflects its actual output.
            local=result.source.edge_point(e.id-1,F(i,steps))
            points.append(project(tuple(F(o)+F(scale)*F(x) for o,x in zip(origin,local))))
        word=' '.join(('M' if i==0 else 'L')+f'{x:.3f},{y:.3f}' for i,(x,y) in enumerate(points))
        parts.append(f'<path d="{word}" stroke="#758998" stroke-dasharray="5 4" stroke-width="1.2" fill="none"/>')
    points=[]
    for row in batch.rows:
        if row.native_status==-1 or sum(a*b for a,b in zip(row.normal,(1,1,1.4)))<=0:continue
        x,y=project(row.position);points.append((sum(row.position),x,y))
    for _,x,y in sorted(points):parts.append(f'<circle cx="{x:.3f}" cy="{y:.3f}" r="1.2" fill="#8ab8cc" opacity=".65"/>')
    for e in solid.edges:
        steps=128 if e.id<3 else 1
        points=[project(solid.edge_point(e.id,F(i,steps))) for i in range(steps+1)]
        word=' '.join(('M' if i==0 else 'L')+f'{x:.3f},{y:.3f}' for i,(x,y) in enumerate(points))
        parts.append(f'<path d="{word}" stroke="#efc47e" stroke-width="1.8" fill="none"/>')
    labels=(f'Exact signed local distance {result.distance}',
        'Actual source retained · fresh output namespace',
        '2 disks + 4 exact rational wall quarters',
        '2 ordinary rim pairs + 1 same-face seam pair',
        '23 original entities → 23 modified images',
        'Source normal feet expose carrier extensions',
        'Positive sharp offset differs from round dilation')
    for i,label in enumerate(labels):parts.append(f'<text x="530" y="{150+36*i}" font-size="15" fill="{"#efc47e" if i>=4 else "#c4d5e3"}">{label}</text>')
    parts.extend(('<text x="28" y="525" font-size="13" fill="#a9bed0">Gold: actual result boundaries. Dashed gray: original body-reader curves. Dots: admitted native samples.</text>',
        '<text x="28" y="549" font-size="13" fill="#a9bed0">Exact analytic feature + source maps; display study, no result mesher or GPU construction.</text>','</g></svg>'))
    (out/'offset.svg').write_text('\n'.join(parts)+'\n')


def execute(args):
    require(sys.platform=='linux','product geometry executes on fleet')
    require(2<=args.grid<=65,'caller grid must be2..65')
    pins=tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES)
    native=(Path(args.library_path).resolve(),Path(args.topology_library).resolve())
    if args.backend=='cuda':native+=(native[0].with_name('libbrepgpu_cuda.so'),)
    before=tuple((p.name,hashlib.sha256(p.read_bytes()).hexdigest()) for p in native)
    distance,origin,scale=RECIPES[args.recipe];frame=Frame(origin,scale)
    factory_start=time.perf_counter_ns();source=make_body('cylinder',frame)
    started=previous=time.perf_counter_ns();source_ms=(started-factory_start)/1e6;stages=[]
    def phase(name):
        nonlocal previous
        now=time.perf_counter_ns();stages.append((name,(now-previous)/1e6));previous=now
    result=offset_solid(source,distance);solid=result.solid
    require(result.source is source and solid.source is source and result.distance==distance and solid.distance==distance
        and result.join==solid.join=='intersection','offset replaced requested source/distance/join')
    phase('cpu_offset');R=F(1,4)+distance;H=F(1,2)+distance
    local=((0,0,0),(R,0,0),(0,0,H),(R,0,H),(R,0,-H),(F(1,4),0,F(1,2)),
        (F(3,5)*R,F(4,5)*R,0),(F(3,5)*R,F(4,5)*R,H),
        (R+F(1,1<<40),0,0),(0,0,H+F(1,1<<40)))
    material=[]
    for p in local:
        world=tuple(F(o)+F(scale)*F(x) for o,x in zip(origin,p));answer=solid.classify_point(world)
        material.append((world,answer.label,answer.boundary_faces))
    parameters=(F(0),F(1,7),F(1,2),F(6,7),F(1))
    edges=tuple((e.id,t,solid.edge_point(e.id,t)) for e in solid.edges for t in parameters)
    uv=tuple((c.id,t,solid.coedge_uv(c.id,t)) for c in solid.coedges for t in (F(0),F(1,7),F(1)))
    require(all(result.history.from_input(r.source) is r for r in result.history.records),'forward history lookup differs')
    require(all(result.history.origin(o.target) is o for o in result.history.origins),'reverse history lookup differs')
    history_maps=[]
    for entry in result.history.origins:
        if entry.target.kind=='vertex':probes=((F(0),F(0)),)
        elif entry.target.kind in ('edge','coedge'):probes=tuple((t,F(0)) for t in parameters)
        else:
            a,b,c,d=entry.source_uv.domain;probes=((a+(b-a)/3,c+(d-c)*F(2,5)),)
        for u,v in probes:
            source_t=None if entry.source_t is None else entry.source_t[0]+entry.source_t[1]*u+entry.source_t[2]*v
            source_uv=None if entry.source_uv is None else entry.source_uv.evaluate(u,v)
            history_maps.append((entry.target.kind,entry.target.entity_id,u,v,source_t,source_uv))
    feet=[]
    for slot in range(6):
        probes=((F(1,2),F(1,2)),(F(1),F(1,2)),(F(3,4),F(3,4))) if slot<2 else ((F(0),F(0)),(F(1,7),F(1,2)),(F(1),F(1)))
        for u,v in probes:
            foot=solid.source_foot(slot,u,v)
            require(foot.solid is solid and foot.chart_id==slot+1 and (foot.u,foot.v)==(u,v),'source-foot query ownership differs')
            require((foot.source_face.body_id,foot.source_face.revision,foot.source_face.kind,foot.source_face.entity_id)==(source.body_id,source.revision,'face',solid.charts[slot].face_slot+1)
                and (foot.source_chart.body_id,foot.source_chart.revision,foot.source_chart.kind,foot.source_chart.entity_id)==(source.body_id,source.revision,'chart',slot+1),'source-foot original owner differs')
            require(foot.distance==distance*F(scale) and sum(n*n for n in foot.normal)==1
                and foot.point==tuple(p+foot.distance*n for p,n in zip(foot.source_point,foot.normal)),'actual signed source-foot identity differs')
            feet.append(foot)
    phase('material_boundary_history_consume')
    request=prepare_grid(result,args.grid);phase('request_prepare')
    with Engine(backend=args.backend,device=args.device,threads=args.threads,library_path=args.library_path) as engine:
        phase('engine_setup');batch=sample(engine,request)
        checksum=sum(sum(row.position)+sum(row.normal) for row in batch.rows if row.native_status!=-1)
        require(math.isfinite(checksum),'nonfinite consumed native checksum');phase('sample_and_consume')
    phase('engine_cleanup')
    topology=solid.topology_input();library=AdjacencyLibrary(args.topology_library);phase('topology_pack_library_load')
    adjacency=library.build(topology,backend=args.backend,device=args.device,threads=args.threads);phase('adjacency_build')
    kinds=tuple(adjacency.edge_kind(i) for i in range(3))
    require(kinds==(PAIRED,PAIRED,SAME_FACE_PAIR),'actual rim/seam pair kinds differ')
    erows=tuple((i,kind,tuple(v.entity_id for v in adjacency.edge_uses(i))) for i,kind in enumerate(kinds))
    crows=tuple((i,adjacency.mate(i).entity_id,adjacency.neighbor(i).entity_id) for i in range(6))
    phase('actual_adjacency_reader');complete=(time.perf_counter_ns()-started)/1e6
    factory_complete=(time.perf_counter_ns()-factory_start)/1e6
    qstart=time.perf_counter_ns();quality={}
    for prefix,values in (('result',qualify_result(result)),('samples',qualify_samples(batch)),('topology',qualify_topology(result,adjacency))):
        quality.update((prefix+'_'+name,value) for name,value in values.items())
    qualification=(time.perf_counter_ns()-qstart)/1e6
    require(tuple((p.name,hashlib.sha256(p.read_bytes()).hexdigest()) for p in native)==before,'native library changed during call')
    out=args.output_directory;out.mkdir(parents=True,exist_ok=False);astart=time.perf_counter_ns()
    tsv(out/'recipe.tsv',('field','value'),(('recipe',args.recipe),('distance',distance),('join',result.join),('source_body',source.body_id),('source_revision',source.revision),
        ('output_body',solid.body_id),('output_revision',solid.revision),('scale',scale.hex()),*((f'origin_{i}',o.hex()) for i,o in enumerate(origin)),
        ('radius',R),('half_height',H),('world_volume_rational',solid.volume.rational),('world_volume_pi_coefficient',solid.volume.pi),
        ('observed_backend',batch.backend),('observed_device',batch.device),('candidate_count',batch.count),('admitted_count',batch.evaluated_count),('checksum',checksum)))
    tsv(out/'source-vertices.tsv',('id','x','y','z'),((v.id,*v.position) for v in source.vertices))
    tsv(out/'sources.tsv',('path','sha256'),pins);tsv(out/'native-libraries.tsv',('name','sha256'),before)
    tsv(out/'quality.tsv',('field','value'),sorted(quality.items()))
    tsv(out/'graph.tsv',('field','type','value'),(*graph_rows('source',source),*graph_rows('output',solid)))
    tsv(out/'history-forward.tsv',('field','type','value'),tree('history',result.history.records))
    tsv(out/'history-reverse.tsv',('field','type','value'),tree('origins',result.history.origins))
    tsv(out/'history-map-reads.tsv',('kind','id','u','v','source_t','source_uv'),history_maps)
    tsv(out/'source-foot.tsv',('chart','u','v','source_face','source_chart','source_u','source_v','source_x','source_y','source_z','x','y','z','nx','ny','nz','distance','in_source_square','source_trim'),
        ((f.chart_id,f.u,f.v,f.source_face.entity_id,f.source_chart.entity_id,*f.source_uv,*f.source_point,*f.point,*f.normal,f.distance,f.in_source_square,f.source_trim) for f in feet))
    tsv(out/'material.tsv',('x','y','z','label','faces'),((*p,label,faces) for p,label,faces in material))
    tsv(out/'edge-points.tsv',('edge_id','t','x','y','z'),((id,t,*p) for id,t,p in edges))
    tsv(out/'coedge-uv.tsv',('coedge_id','t','u','v'),((id,t,*p) for id,t,p in uv))
    tsv(out/'samples.tsv',('index','slot','face','chart','u','v','trim','native_status','x','y','z','nx','ny','nz'),
        ((r.index,r.patch_slot,r.face_id,r.chart_id,r.u,r.v,r.trim,r.native_status,*r.position,*r.normal) for r in batch.rows))
    for name in ('coedge_edges','coedge_faces','orientations'):(out/(name+('.i32le' if name=='orientations' else '.u32le'))).write_bytes(getattr(topology,name))
    for name in ('offsets','members','kinds','mates','neighbors'):(out/(name+'.u32le')).write_bytes(getattr(adjacency,name))
    tsv(out/'adjacency-edges.tsv',('slot','kind','coedges'),erows);tsv(out/'adjacency-uses.tsv',('slot','mate','neighbor'),crows)
    render(result,batch,out)
    stages.extend(('construction/'+name,value) for name,value in result.timings)
    stages.extend(('sampling/'+name,value) for name,value in batch.timings)
    stages.extend(('adjacency/'+name,value) for name,value in adjacency.timings)
    stages.extend((('source_creation_outside',source_ms),('complete_source_to_consumed_cleanup',complete),
        ('complete_factory_to_consumed_cleanup',factory_complete),('independent_qualification',qualification),('artifact_writes',(time.perf_counter_ns()-astart)/1e6)))
    tsv(out/'costs.tsv',('phase','wall_ms'),stages)
    require(pins==tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES),'owned source changed during call')
    print(f'{args.recipe} {batch.backend}:{batch.device} distance={distance} samples={batch.count} admitted={batch.evaluated_count} complete_ms={complete:.6f} qualification_ms={qualification:.6f}',flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--recipe',choices=RECIPES,default='outset')
    p.add_argument('--backend',choices=('cpu','cuda'),default='cpu');p.add_argument('--device',type=int,default=0)
    p.add_argument('--threads',type=int,default=1);p.add_argument('--grid',type=int,default=17)
    p.add_argument('--library-path',default='build/pair-release/libbrepgpu.so');p.add_argument('--topology-library',default='build/topology-release/libbrepgpu_topology.so')
    p.add_argument('--output-directory',type=Path,required=True);execute(p.parse_args())
