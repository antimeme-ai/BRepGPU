#!/usr/bin/env python3
"""Construct and consume a complete owned box-edge fillet on CPU/P100."""
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
from brepgpu.fillet import fillet_edge
from brepgpu.boolean import EntityKey
from brepgpu.fillet_sampling import prepare_grid,sample
from brepgpu.fillet_oracles import qualify_result,qualify_samples,qualify_topology
from brepgpu.adjacency import AdjacencyLibrary,PAIRED
from cut_bore import tsv,require,tree,graph_rows

ROOT=Path(__file__).resolve().parents[1]
SOURCES=('examples/fillet_box_edge.py','examples/cut_bore.py','python/brepgpu/fillet.py',
    'python/brepgpu/fillet_sampling.py','python/brepgpu/fillet_oracles.py',
    'python/brepgpu/boolean.py','python/brepgpu/boolean_sampling.py',
    'python/brepgpu/body.py','python/brepgpu/body_sampling.py','python/brepgpu/body_oracles.py',
    'python/brepgpu/adjacency.py','python/brepgpu/adjacency_oracles.py','python/brepgpu/__init__.py')
RECIPES={
    'small':(9,F(1,16),(0.,0.,0.),1.),
    'mid':(11,F(3,8),(2.,-3.,1.),.5),
    'large':(12,F(1,2),(-2.,1.,.5),2.)}


def rotate(x,y,q):return ((x,y),(-y,x),(-x,-y),(y,-x))[q]


def render(result,batch,out):
    """Actual owned line/quarter-arc boundaries and native surface points."""
    solid=result.solid;origin=solid.frame.origin;scale=solid.frame.scale
    def project(point):
        x,y,z=((float(p)-o)/scale for p,o in zip(point,origin))
        return 300+190*.76*(x-y),280+190*(.27*(x+y)-.9*z)
    parts=['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1050 570">',
        '<rect width="1050" height="570" fill="#101923"/>','<g font-family="sans-serif" fill="#e6eff7">',
        '<text x="28" y="38" font-size="24">BRepGPU · complete source-edge fillet</text>',
        f'<text x="28" y="64" font-size="14" fill="#a9bed0">CPU feature construction · {batch.backend} native consumers · selected source edge {result.edge.entity_id}</text>']
    points=[]
    for row in batch.rows:
        if row.native_status==-1 or sum(a*b for a,b in zip(row.normal,(1,1,1.4)))<=0:continue
        x,y=project(row.position);points.append((sum(row.position),x,y,row.face_id))
    for _,x,y,face in sorted(points):
        parts.append(f'<circle cx="{x:.3f}" cy="{y:.3f}" r="1.2" fill="{"#efc47e" if face==7 else "#8ab8cc"}" opacity=".6"/>')
    for edge in solid.edges:
        steps=32 if solid.curves[edge.curve_slot].kind=='quarter_arc' else 1
        points=[project(solid.edge_point(edge.id,F(i,steps))) for i in range(steps+1)]
        word=' '.join(('M' if i==0 else 'L')+f'{x:.3f},{y:.3f}' for i,(x,y) in enumerate(points))
        parts.append(f'<path d="{word}" stroke="{"#efc47e" if edge.id>=12 else "#62d7e7"}" stroke-width="1.7" fill="none"/>')
    labels=('One actual full source edge rounded',
        f'Exact local radius {result.radius}',
        '6 original plane carriers + 1 circular band',
        '10 vertices / 15 edges / 30 distinct uses',
        '2 complete tangent contact rails',
        '62 sources → 76 exact output origins',
        '33 retained / 24 modified / 19 generated',
        '5 deleted source entities retain ancestry')
    for i,label in enumerate(labels):
        parts.append(f'<text x="560" y="{144+35*i}" font-size="16" fill="{"#efc47e" if i>=4 else "#c4d5e3"}">{label}</text>')
    parts.extend(('<text x="28" y="525" font-size="13" fill="#a9bed0">Actual exact line/arc wireframe + admitted native samples; display study, no result mesher.</text>',
        '<text x="28" y="549" font-size="13" fill="#a9bed0">Gold: generated circular band / cap arcs / tangent rails. Both side joins are tangent; cap joins remain sharp.</text>','</g></svg>'))
    (out/'fillet.svg').write_text('\n'.join(parts)+'\n')


def execute(args):
    require(sys.platform=='linux','product geometry executes on fleet')
    require(2<=args.grid<=65,'caller grid must be2..65')
    pins=tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES)
    native=(Path(args.library_path).resolve(),Path(args.topology_library).resolve())
    if args.backend=='cuda':native+=(native[0].with_name('libbrepgpu_cuda.so'),)
    before=tuple((p.name,hashlib.sha256(p.read_bytes()).hexdigest()) for p in native)
    edge_id,radius,origin,scale=RECIPES[args.recipe]
    frame=Frame(origin,scale)
    factory_start=time.perf_counter_ns()
    source=make_body('box',frame)
    started=previous=time.perf_counter_ns();source_ms=(started-factory_start)/1e6;stages=[]
    def phase(name):
        nonlocal previous
        now=time.perf_counter_ns();stages.append((name,(now-previous)/1e6));previous=now
    selected=EntityKey(source.body_id,source.revision,'edge',edge_id)
    result=fillet_edge(source,selected,radius);solid=result.solid
    require(result.source is source and solid.source is source and result.edge==selected,'fillet replaced original source/selection')
    phase('cpu_fillet')
    q=(edge_id-7)%4;c=F(3,4)-radius;cx,cy=rotate(c,c,q)
    def section(x,y,z=F(0)):
        dx,dy=rotate(x,y,q);return cx+radius*dx,cy+radius*dy,z
    corner=rotate(F(3,4),F(3,4),q)
    local=[(F(0),F(0),F(0)),(*corner,F(0)),(cx,cy,F(0)),section(-1,0),
           section(1,0),section(0,1),section(F(3,5),F(4,5)),
           section(F(3,5),F(4,5),F(-1,2)),section(F(3,5),F(4,5),F(1,2)),
           section(1,0,F(-1,2)),section(0,1,F(1,2))]
    material=[]
    for p in local:
        world=tuple(F(o)+F(scale)*x for o,x in zip(origin,p))
        answer=solid.classify_point(world);material.append((world,answer.label,answer.boundary_faces))
    parameters=(F(0),F(1,7),F(1,2),F(6,7),F(1))
    edges=tuple((e.id,t,solid.edge_point(e.id,t)) for e in solid.edges for t in parameters)
    uv=tuple((c.id,t,solid.coedge_uv(c.id,t)) for c in solid.coedges for t in (F(0),F(1,7),F(1)))
    require(all(result.history.from_input(r.source)==r for r in result.history.records),'forward history lookup differs')
    require(all(result.history.origin(o.target)==o for o in result.history.origins),'reverse history lookup differs')
    history_maps=[]
    for entry in result.history.origins:
        if entry.target.kind=='vertex':probes=((F(0),F(0)),)
        elif entry.target.kind in ('edge','coedge'):probes=tuple((t,F(0)) for t in parameters)
        else:probes=((F(1,3),F(2,5)),)
        for u,v in probes:
            source_t=None if entry.source_t is None else entry.source_t[0]+entry.source_t[1]*u+entry.source_t[2]*v
            source_uv=None if entry.source_uv is None else entry.source_uv.evaluate(u,v)
            source_label=None
            if source_uv is not None:
                owner=entry.source_uv_owner
                slot=owner.entity_id-1 if owner.kind=='chart' else source.faces[owner.entity_id-1].chart_slots[0]
                source_label=source.classify(slot,*source_uv)
                require(source_label in (1,2),'emitted source UV does not read its original plane domain')
            history_maps.append((entry.target.kind,entry.target.entity_id,entry.role,u,v,
                entry.source_t_relation,entry.source_t_owner,source_t,entry.source_uv_owner,
                *(('', '') if source_uv is None else source_uv),entry.orientation,source_label))
    contact_reads=[]
    for contact in result.contacts:
        uses=[use for use in solid.coedges if use.edge_slot==contact.edge.entity_id-1]
        require(len(uses)==2,'contact lacks two actual output occurrences')
        side=next(use for use in uses if use.face_slot==contact.source_face.entity_id-1)
        band=next(use for use in uses if use.face_slot==contact.blend_face.entity_id-1)
        for t in parameters:
            point=solid.edge_point(contact.edge.entity_id,t)
            uv_source=contact.source_uv.evaluate(t,F(0));uv_band=contact.blend_uv.evaluate(t,F(0))
            require(solid.coedge_uv(side.id,t)==uv_source and solid.coedge_uv(band.id,t)==uv_band,'contact maps differ from actual incident UV readers')
            source_t=contact.source_t[0]+contact.source_t[1]*t
            contact_reads.append((contact.edge.entity_id,contact.source_face.entity_id,contact.source_coedge.entity_id,
                contact.blend_face.entity_id,contact.blend_u,t,*point,*uv_source,*uv_band,source_t,*contact.normal))
    phase('material_boundary_history_consume')
    request=prepare_grid(result,args.grid);phase('request_prepare')
    with Engine(backend=args.backend,device=args.device,threads=args.threads,library_path=args.library_path) as engine:
        require(Path(engine._lib._name).resolve()==native[0],'loaded native path differs')
        phase('engine_setup');batch=sample(engine,request)
        require(batch.result is result and batch.request is request,'native request/result owner differs')
        require((batch.backend,batch.device)==(args.backend,args.device),'native placement differs')
        checksum=math.fsum(x for r in batch.rows if r.native_status!=-1 for x in (*r.position,*r.normal))
        require(math.isfinite(checksum),'nonfinite consumed native output');phase('sample_and_consume')
    phase('engine_cleanup')
    packed=solid.topology_input();library=AdjacencyLibrary(args.topology_library);phase('topology_pack_library_load')
    adjacency=library.build(packed,backend=args.backend,device=args.device,threads=args.threads);phase('adjacency_build')
    edge_rows=[];use_rows=[]
    for slot,key in enumerate(packed.edge_sources):
        uses=adjacency.edge_uses(slot);kind=adjacency.edge_kind(slot)
        require(kind==PAIRED and len(uses)==2,'fillet edge lacks ordinary opposed pair')
        edge_rows.append((slot,key.entity_id,kind,','.join(str(u.entity_id) for u in uses)))
    for slot,key in enumerate(packed.coedge_sources):
        mate=adjacency.mate(slot);neighbor=adjacency.neighbor(slot)
        require(mate is not None and neighbor is not None,'missing actual mate/neighbor')
        require((mate.body_id,mate.revision)==(solid.body_id,solid.revision) and
                (neighbor.body_id,neighbor.revision)==(solid.body_id,solid.revision),'native reader escaped result namespace')
        use_rows.append((slot,key.entity_id,mate.entity_id,neighbor.entity_id))
    phase('actual_adjacency_reader')
    end=time.perf_counter_ns();complete_ms=(end-started)/1e6;factory_ms=(end-factory_start)/1e6
    start=time.perf_counter_ns()
    quality={}
    for prefix,values in (('result',qualify_result(result)),
                          ('samples',qualify_samples(batch)),('topology',qualify_topology(result,adjacency))):
        quality.update((prefix+'_'+k,v) for k,v in values.items())
    qualification_ms=(time.perf_counter_ns()-start)/1e6
    require(before==tuple((p.name,hashlib.sha256(p.read_bytes()).hexdigest()) for p in native),'native file changed')
    out=args.output_directory;out.mkdir(parents=True,exist_ok=False);start=time.perf_counter_ns()
    tsv(out/'recipe.tsv',('field','value'),(('recipe',args.recipe),('backend',args.backend),('device',args.device),
        ('threads',args.threads),('grid',args.grid),('source_body',source.body_id),('output_body',solid.body_id),('source_edge',edge_id),('radius',radius),
        *((f'origin_{axis}_hex',o.hex()) for axis,o in zip('xyz',origin)),('scale_hex',scale.hex()),
        ('world_volume_rational',solid.volume.rational),('world_volume_pi_coefficient',solid.volume.pi),('observed_backend',batch.backend),('observed_device',batch.device),
        ('samples',batch.count),('evaluated_samples',batch.evaluated_count),('checksum',checksum),('construction','owned CPU complete constant-radius box-edge fillet'),
        ('native_consumers','first jets/oriented normals and adjacency'),('mesher','not implemented')))
    tsv(out/'source-vertices.tsv',('vertex_id','x','y','z'),((v.id,*v.position) for v in source.vertices))
    tsv(out/'sources.tsv',('path','sha256'),pins);tsv(out/'native-libraries.tsv',('file','sha256'),before)
    tsv(out/'quality.tsv',('field','value'),sorted(quality.items()))
    def graph():
        yield from graph_rows('source',source);yield from graph_rows('output',solid)
    tsv(out/'graph.tsv',('path','type','value'),graph())
    tsv(out/'history-forward.tsv',('path','type','value'),tree('records',result.history.records))
    tsv(out/'history-reverse.tsv',('path','type','value'),tree('origins',result.history.origins))
    tsv(out/'history-map-reads.tsv',('kind','id','role','u','v','source_t_relation','source_t_owner','source_t','source_uv_owner','source_u','source_v','orientation','source_trim'),history_maps)
    tsv(out/'contacts.tsv',('path','type','value'),tree('contacts',result.contacts))
    tsv(out/'contact-map-reads.tsv',('edge','source_face','source_coedge','blend_face','blend_u','t','x','y','z','source_u','source_v','blend_u_read','blend_v','source_t','nx','ny','nz'),contact_reads)
    tsv(out/'material.tsv',('probe','x','y','z','label','boundary_faces'),((i,*p,label,','.join(map(str,faces))) for i,(p,label,faces) in enumerate(material)))
    tsv(out/'edge-points.tsv',('edge_id','t','x','y','z'),((eid,t,*p) for eid,t,p in edges))
    tsv(out/'coedge-uv.tsv',('coedge_id','t','u','v'),((cid,t,*p) for cid,t,p in uv))
    tsv(out/'samples.tsv',('index','slot','face','chart','u_hex','v_hex','trim','native_status','x','y','z','nx','ny','nz'),
        ((r.index,r.patch_slot,r.face_id,r.chart_id,r.u.hex(),r.v.hex(),r.trim,r.native_status,*r.position,*r.normal) for r in batch.rows))
    tsv(out/'adjacency-edges.tsv',('slot','edge_id','kind','coedge_ids'),edge_rows)
    tsv(out/'adjacency-uses.tsv',('slot','coedge_id','mate_id','neighbor_face_id'),use_rows)
    for name in ('coedge_edges','coedge_faces','orientations'):
        (out/(name+('.i32le' if name=='orientations' else '.u32le'))).write_bytes(getattr(packed,name))
    for name in ('offsets','members','kinds','mates','neighbors'):(out/(name+'.u32le')).write_bytes(getattr(adjacency,name))
    render(result,batch,out)
    tsv(out/'costs.tsv',('phase','wall_ms'),(('source_creation_outside',source_ms),*stages,
        *((f'construction/{k}',v) for k,v in result.timings),*((f'sampling/{k}',v) for k,v in batch.timings),
        *((f'adjacency/{k}',v) for k,v in adjacency.timings),('complete_source_to_consumed_cleanup',complete_ms),
        ('complete_factory_to_consumed_cleanup',factory_ms),('independent_qualification',qualification_ms),
        ('artifact_writes_except_costs',(time.perf_counter_ns()-start)/1e6)))
    require(pins==tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES),'owned source changed')
    print(f'{args.recipe} {args.backend}:{args.device} source_edge={edge_id} radius={radius} samples={batch.count} admitted={batch.evaluated_count} complete_ms={complete_ms:.6f} qualification_ms={qualification_ms:.6f}',flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--recipe',choices=tuple(RECIPES),default='small')
    p.add_argument('--backend',choices=('cpu','cuda'),default='cpu');p.add_argument('--device',type=int,default=0)
    p.add_argument('--threads',type=int,default=1);p.add_argument('--grid',type=int,default=17)
    p.add_argument('--library-path',type=Path,default=Path('build/pair-release/libbrepgpu.so'))
    p.add_argument('--topology-library',type=Path,default=Path('build/topology-release/libbrepgpu_topology.so'))
    p.add_argument('--output-directory',type=Path,required=True);execute(p.parse_args())
