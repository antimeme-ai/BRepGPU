#!/usr/bin/env python3
"""Three bounded actual extrusion meshes, indexed/history readers and exports."""
import argparse
from dataclasses import fields,is_dataclass
from fractions import Fraction as F
import hashlib
from pathlib import Path
import statistics
import sys
import time

from brepgpu.body import Frame
from brepgpu.extrusion import make_profile,extrude
from brepgpu.extrusion_meshing import tessellate
from brepgpu.extrusion_meshing_oracles import qualify_mesh
from mesh_body import read_mesh,export_mesh_data,same_output,tsv,ratio

ROOT=Path(__file__).resolve().parents[1]
SOURCES=('python/brepgpu/__init__.py','python/brepgpu/body.py','python/brepgpu/boolean.py',
         'python/brepgpu/meshing.py','python/brepgpu/meshing_oracles.py',
         'python/brepgpu/extrusion.py','python/brepgpu/extrusion_oracles.py',
         'python/brepgpu/extrusion_meshing.py','python/brepgpu/extrusion_meshing_oracles.py',
         'tests/test_extrusion_meshing.py','examples/mesh_body.py','examples/mesh_extrusion.py')
CASES={
    'triangle':(((-160,-128),(192,-96),(-32,192)),(0.,0.,0.),1.,F(3,4)),
    'chamfer':(((-160,-128),(64,-160),(192,-32),(128,128),(-32,192),(-192,32)),(2.,-3.,1.),.5,F(5,4)),
    'many16':(((176,0),(160,64),(128,128),(64,160),(0,176),(-64,160),(-128,128),(-160,64),
               (-176,0),(-160,-64),(-128,-128),(-64,-160),(0,-176),(64,-160),(128,-128),(160,-64)),
              (-2.,1.,.5),2.,F(3,2))}
TOLERANCE=F(1,1048576)


def pins():
    return tuple((p,hashlib.sha256((ROOT/p).read_bytes()).hexdigest()) for p in SOURCES)


def require(ok,message):
    if not ok:raise AssertionError(message)


def tree(path,value):
    if type(value) is F:yield path,'rational',ratio(value)
    elif type(value) is float:yield path,'binary64',value.hex()
    elif value is None:yield path,'none',''
    elif type(value) in (bool,int,str):yield path,type(value).__name__,str(value)
    elif type(value) is tuple:
        yield path,'tuple_length',len(value)
        for i,item in enumerate(value):yield from tree(f'{path}/{i}',item)
    elif is_dataclass(value):
        yield path,'record',type(value).__name__
        for field in fields(value):yield from tree(f'{path}/{field.name}',getattr(value,field.name))
    else:raise TypeError('unsupported source record '+path)


def read_history(feature):
    checksum=0;relations=0
    tables={'vertex':'vertices','edge':'edges','coedge':'coedges','loop':'loops','face':'faces','chart':'charts'}
    for record in feature.history.records:
        key=record.source
        require(feature.history.from_input(key) is record,'forward history lost identity')
        require((key.body_id,key.revision)==(feature.profile.body_id,feature.profile.revision),'foreign source namespace')
        original=next(r for r in getattr(feature.profile,tables[key.kind]) if r.id==key.entity_id)
        for path,kind,value in (*tree('source',original),*tree('forward',record)):
            checksum+=sum(map(ord,path+kind+str(value)))
        for target in (*record.images,*record.generated):
            require(key in feature.history.origin(target).parents,'forward origin differs');relations+=1
    for origin in feature.history.origins:
        require(feature.history.origin(origin.target) is origin,'reverse history lost identity')
        require((origin.target.body_id,origin.target.revision)==(feature.solid.body_id,feature.solid.revision),'foreign target namespace')
        for parent in origin.parents:
            record=feature.history.from_input(parent)
            require(origin.target in (*record.images,*record.generated),'reverse disposition differs')
        for path,kind,value in tree('origin',origin):checksum+=sum(map(ord,path+kind+str(value)))
    return (('input_records',len(feature.history.records)),('output_origins',len(feature.history.origins)),
            ('forward_relations',relations),('history_checksum',checksum))


def export(directory,mesh,reader,history,quality):
    directory.mkdir()
    feature=mesh.feature_result
    tsv(directory/'source-profile.tsv',('path','type','value'),tree('profile',feature.profile))
    def solid_rows():
        for name in ('body_id','revision','frame','height','vertices','curves','edges','coedges',
                     'loops','faces','charts','patches','volume'):
            yield from tree('solid/'+name,getattr(mesh.body,name))
    tsv(directory/'result.tsv',('path','type','value'),solid_rows())
    tsv(directory/'history-forward.tsv',('path','type','value'),tree('records',mesh.history.records))
    tsv(directory/'history-reverse.tsv',('path','type','value'),tree('origins',mesh.history.origins))
    tsv(directory/'history-reader.tsv',('metric','value'),history)
    export_mesh_data(directory,mesh,reader,quality,shape='convex extrusion',namespace='result',
                     boundary_note='Colors name result faces. Straight shared source edges; no periodic seams. Wire view has no hidden-line removal.')
    (directory/'RECIPE.md').write_text(
        '# Actual convex-profile extrusion mesh\n\n'
        f'{len(feature.profile.vertices)} original profile vertices, {ratio(feature.height)} local height; '
        f'{len(mesh.vertices)} shared output vertices and {len(mesh.triangles)} triangles. '
        f'World tolerance {ratio(mesh.tolerance)}; actual rounding/error bound {ratio(mesh.error_bound)}.\n\n'
        'The actual ExtrusionResult, original profile and complete typed history are retained. '
        'Exact planar faces need no geometric approximation. The cap mesh covers the actual '
        'trimmed profile polygon, and original coedges/UV maps name each occurrence. Source and '
        'result tables preserve original controls and Frames. The PLY and views use one shared '
        'rounded coordinate array; facet normals are not smoothed. No hidden-line removal, '
        'general triangulation/quality guarantee, GPU mesher or alpha qualification claim.\n')


def main(args):
    require(sys.platform=='linux','product callers execute on the fleet')
    output=args.output_directory;output.mkdir(parents=True,exist_ok=False)
    source_pins=pins();raw=[];single=[];counts=[]
    names=tuple(CASES) if args.case is None else (args.case,)
    for name in names:
        points,origin,scale,height=CASES[name]
        started=time.perf_counter_ns()
        profile=make_profile(tuple(tuple(F(x,256) for x in p) for p in points),Frame(origin,scale))
        prepared=time.perf_counter_ns();feature=extrude(profile,height)
        constructed=time.perf_counter_ns()
        first=first_reader=first_history=quality=None
        for phase,index in (('first',0),*(('repeat',i) for i in range(3))):
            begin=time.perf_counter_ns();mesh=tessellate(feature,tolerance=TOLERANCE)
            built=time.perf_counter_ns();reader=read_mesh(mesh)
            indexed=time.perf_counter_ns();history=read_history(feature)
            consumed=time.perf_counter_ns()
            require(mesh.feature_result is feature and mesh.body is feature.solid and mesh.history is feature.history,
                    'mesh lost actual feature/solid/history')
            require(mesh.tolerance==TOLERANCE,'mesh lost exact request')
            checked=time.perf_counter_ns()
            if first is None:
                first,first_reader,first_history=mesh,reader,history
                quality=qualify_mesh(mesh)
            else:
                same_output(mesh,first)
                require(mesh.feature_result is first.feature_result,'repeat replaced original feature')
                require(history==first_history and reader.metrics==first_reader.metrics,'repeat reader differs')
                require(all(getattr(reader,k).tobytes()==getattr(first_reader,k).tobytes()
                            for k in ('xyz','indices','face_ids','normals')),'repeat indexed buffers differ')
            end=time.perf_counter_ns()
            raw.append((name,phase,index,(built-begin)/1e6,(indexed-built)/1e6,(consumed-indexed)/1e6,
                        (consumed-begin)/1e6,(end-checked)/1e6,'PASS'))
        export_start=time.perf_counter_ns();export(output/name,first,first_reader,first_history,quality)
        export_ms=(time.perf_counter_ns()-export_start)/1e6
        single.append((name,(prepared-started)/1e6,(constructed-prepared)/1e6,export_ms))
        counts.append((name,len(profile.vertices),len(first.vertices),len(first.triangles),
                       len(first.edge_polylines),len(first.boundary_uses),ratio(TOLERANCE),ratio(first.error_bound)))
        print(f'{name}: {len(first.vertices)} vertices/{len(first.triangles)} triangles; first qualified, three repeat correspondences',flush=True)
    tsv(output/'raw.tsv',('case','phase','sample','mesh_ms','indexed_reader_ms','history_reader_ms',
                        'complete_ms','outside_qualification_or_correspondence_ms','result'),raw)
    summary=[]
    for name in names:
        for index,scope in ((3,'mesh'),(4,'indexed_reader'),(5,'history_reader'),(6,'complete')):
            values=[r[index] for r in raw if r[0]==name and r[1]=='repeat']
            summary.append((name,scope,statistics.median(values),min(values),max(values)))
    tsv(output/'summary.tsv',('case','scope','median_ms','min_ms','max_ms'),summary)
    tsv(output/'single.tsv',('case','profile_prepare_ms','cpu_extrusion_ms','filesystem_export_ms'),single)
    tsv(output/'counts.tsv',('case','profile_vertices','mesh_vertices','triangles','physical_edges','coedge_uses',
                           'world_tolerance','error_bound'),counts)
    require(pins()==source_pins,'caller source changed during serial work')
    tsv(output/'sources.tsv',('path','sha256'),source_pins)
    (output/'SCOPE.md').write_text(
        '# Actual extrusion-result CPU mesh scopes\n\n'
        'Each selected case prepares one original profile and ExtrusionResult, then produces '
        'one first mesh and three repeated fresh meshes. First output is excluded from medians. '
        'Complete wall wraps meshing, actual indexed buffers/record reading and typed original '
        'profile history traversal. First independent qualification and later full-output '
        'correspondence are outside that wall and named by phase. Profile preparation, original '
        'feature construction and first-output filesystem export are separate observations. '
        'Separate phase medians need not add. Imports, final summary/source writes and process '
        'shutdown are excluded. CPU-only, zero native/CUDA calls or device transfers; no speedup '
        'baseline or hardware claim. This is one bounded run, not a certification campaign.\n')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',choices=tuple(CASES))
    parser.add_argument('--output-directory',required=True,type=Path)
    main(parser.parse_args())
