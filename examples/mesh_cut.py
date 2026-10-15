#!/usr/bin/env python3
"""Mesh an actual owned Boolean result, read its indexed buffers and ancestry."""
import argparse
from dataclasses import fields, is_dataclass
from fractions import Fraction as F
import hashlib
from pathlib import Path
import sys
import time

from brepgpu.body import BodySnapshot, Frame, make_body
from brepgpu.boolean import difference
from brepgpu.cut_meshing import tessellate
from brepgpu.cut_meshing_oracles import qualify_mesh
from mesh_body import (read_mesh, export_mesh_data, write_source, tsv, ratio,
                       check_binding as check_mesh_binding, same_output as same_mesh_output)

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ('python/brepgpu/__init__.py','python/brepgpu/body.py',
           'python/brepgpu/body_oracles.py','python/brepgpu/boolean.py',
           'python/brepgpu/boolean_oracles.py','python/brepgpu/intersections.py',
           'python/brepgpu/intersection_oracles.py','python/brepgpu/classification.py',
           'python/brepgpu/arrangements.py','python/brepgpu/arrangement_oracles.py',
           'python/brepgpu/meshing.py',
           'python/brepgpu/meshing_oracles.py','python/brepgpu/cut_meshing.py',
           'python/brepgpu/cut_meshing_oracles.py','tests/test_cut_meshing.py',
           'examples/mesh_body.py','examples/mesh_cut.py','benchmarks/cut_meshing/run.py')
CASES = (('coarse-unit',(0.,0.,0.),1.,F(1,2)),
         ('fine-unit',(0.,0.,0.),1.,F(1,1024)),
         ('translated-scale2',(-4.,2.,-1.),2.,F(1,4096)),
         ('subnormal-quarter',(float.fromhex('0x0.0000000000001p-1022'),0.,0.),.25,F(1,4096)))


def source_pins():
    return tuple((name,hashlib.sha256((ROOT/name).read_bytes()).hexdigest()) for name in SOURCES)


def require(condition,message):
    if not condition:raise AssertionError(message)


def tree(path,value):
    """Emit actual immutable records; retained source bodies remain references."""
    if type(value) is BodySnapshot:
        yield path,'body_ref',f'{value.body_id}:{value.revision}'
    elif type(value) is F:yield path,'rational',ratio(value)
    elif type(value) is float:yield path,'binary64',value.hex()
    elif value is None:yield path,'none',''
    elif type(value) in (bool,int,str):yield path,type(value).__name__,str(value)
    elif type(value) is bytes:yield path,'bytes_sha256',hashlib.sha256(value).hexdigest()
    elif type(value) is tuple:
        yield path,'tuple_length',len(value)
        for i,item in enumerate(value):yield from tree(f'{path}/{i}',item)
    elif is_dataclass(value):
        yield path,'record',type(value).__name__
        for field in fields(value):yield from tree(f'{path}/{field.name}',getattr(value,field.name))
    else:raise TypeError('unsupported actual cut source field '+path+': '+type(value).__name__)


def read_history(cut):
    """Follow every typed output origin to its actual original operand record."""
    checksum = 0
    relations = 0
    for record in cut.history.records:
        require(cut.history.from_input(record.source) is record,'forward history lookup lost record')
        source = cut.left if record.source.body_id == cut.left.body_id else cut.right
        require((record.source.body_id,record.source.revision)==(source.body_id,source.revision),
                'history escaped original operand namespace')
        table = {'vertex':'vertices','edge':'edges','coedge':'coedges','loop':'loops',
                 'face':'faces','chart':'charts'}[record.source.kind]
        original = next(x for x in getattr(source,table) if x.id == record.source.entity_id)
        for path,kind,value in (*tree("original",original),*tree("forward",record)):
            checksum += sum(map(ord,path+kind+str(value)))
        for target in (*record.images,*record.generated):
            require(record.source in cut.history.origin(target).parents,'forward/reverse history differs')
            relations += 1
    for origin in cut.history.origins:
        require(cut.history.origin(origin.target) is origin,'reverse history lookup lost record')
        require((origin.target.body_id,origin.target.revision)==(cut.solid.body_id,cut.solid.revision),
                'origin escaped actual result namespace')
        for parent in origin.parents:
            record = cut.history.from_input(parent)
            require(origin.target in (*record.images,*record.generated),'reverse/forward history differs')
        # Read every range/domain/event/source-map field, beyond entity counts.
        for path,kind,value in tree('origin',origin):
            checksum += sum(map(ord,path+kind+str(value)))
    return (('input_records',len(cut.history.records)),('output_origins',len(cut.history.origins)),
            ('forward_relations',relations),('history_checksum',checksum))


def check_binding(mesh,cut,tolerance,max_quarter_segments=256):
    check_mesh_binding(mesh,cut.solid,tolerance,max_quarter_segments)
    require(mesh.cut_result is cut and mesh.history is cut.history,'mesh replaced original cut/history')


def same_output(mesh,first):
    same_mesh_output(mesh,first)
    require(mesh.cut_result is first.cut_result and mesh.history is first.history,
            'repeated mesh replaced actual cut/history')


def export_cut(directory,mesh,reader,history_read,quality):
    directory.mkdir()
    cut=mesh.cut_result;body=mesh.body
    for label,operand in (('left-box',cut.left),('right-cutter',cut.right)):
        target=directory/label;target.mkdir();write_source(target,operand)
    def result_graph():
        for name in ('body_id','revision','frame','vertices','curves','edges','coedges','loops',
                     'faces','charts','patches','radius','wall_band','volume'):
            yield from tree('result/'+name,getattr(body,name))
    tsv(directory/'result-graph.tsv',('path','type','value'),result_graph())
    for filename,name in (('sections','sections'),('arrangements','arrangements'),('selections','selections')):
        tsv(directory/(filename+'.tsv'),('path','type','value'),tree(name,getattr(cut,name)))
    tsv(directory/'history-forward.tsv',('path','type','value'),tree('records',cut.history.records))
    tsv(directory/'history-reverse.tsv',('path','type','value'),tree('origins',cut.history.origins))
    tsv(directory/'history-reader.tsv',('metric','value'),history_read)
    tsv(directory/'recipe.tsv',('field','value'),
        (('operation','actual owned box-minus-through-cylinder'),('placement','exact CPU serial'),
         ('left_body_id',cut.left.body_id),('left_revision',cut.left.revision),
         ('right_body_id',cut.right.body_id),('right_revision',cut.right.revision),
         ('result_body_id',body.body_id),('result_revision',body.revision),
         *((f'origin_{axis}_hex',x.hex()) for axis,x in zip('xyz',body.frame.origin)),
         ('box_scale_hex',body.frame.scale.hex()),('cutter_scale_hex',cut.right.frame.scale.hex()),
         ('max_quarter_segments',256),('tolerance',ratio(mesh.tolerance)),
         ('local_radius',ratio(body.radius)),('raw_wall_v0',ratio(body.wall_band[0])),
         ('raw_wall_v1',ratio(body.wall_band[1])),('native_calls',0),('host_device_transfer_bytes',0)))
    export_mesh_data(directory,mesh,reader,quality,shape='through-bore result',namespace='result')
    (directory/'RECIPE.md').write_text(
        '# Actual through-bore Boolean result mesh\n\n'
        f'{len(mesh.vertices)} shared vertices, {len(mesh.triangles)} oriented triangles; '
        f'{mesh.quarter_segments} segments per original rational circle quarter. '
        f'World tolerance {ratio(mesh.tolerance)}; total bound {ratio(mesh.error_bound)}.\n\n'
        'The mesher retains the actual CutResult, its CutSolid, original operands and complete '
        'typed history. Named operand directories preserve their original factory packed tables; '
        'result-graph.tsv preserves actual output records and original carrier controls/frames. '
        'No factory plate or fabricated packed result supplies this mesh.\n\n'
        'Mesh vertices retain exact world positions and one shared rounded binary64 array. '
        'PLY contains those actual indexed coordinates and geometric facet normals. The SVG '
        'shows actual top triangles and an all-face wire view without hidden-line removal. '
        'The bore remains open; gold marks one physical seam with two atlas occurrences.\n\n'
        'corners.tsv contains chart-local U and original cutter V[1/4,3/4]; boundary-uses.tsv '
        'retains original carrier atlas UV and intrinsic result-edge parameters. Original cutter '
        'seam t[1/4,3/4] ancestry is retained separately in history-reverse.tsv. Circular chords '
        'include a thin strip inside the analytic hole; the two-sided error bound covers this '
        'discrepancy. Not all triangle interiors are exactly material-IN.\n\n'
        'Exact CPU construction; no native/GPU mesher or device transfer. External community '
        'qualification remains incomplete and required before alpha.\n')


def main(args):
    require(sys.platform=='linux','product callers run on the fleet')
    name,origin,scale,tolerance=next(case for case in CASES if case[0]==args.case)
    pins=source_pins()
    start=time.perf_counter_ns()
    left=make_body('box',Frame(origin,scale));right=make_body('cylinder',Frame(origin,2*scale))
    prepared=time.perf_counter_ns()
    cut=difference(left,right)
    constructed=time.perf_counter_ns()
    mesh=tessellate(cut,tolerance=tolerance,max_quarter_segments=256)
    built=time.perf_counter_ns()
    reader=read_mesh(mesh)
    indexed=time.perf_counter_ns()
    history_read=read_history(cut)
    consumed=time.perf_counter_ns()
    require(cut.left is left and cut.right is right,'Boolean replaced original operands')
    check_binding(mesh,cut,tolerance)
    quality=qualify_mesh(mesh)
    qualified=time.perf_counter_ns()
    args.output_directory.mkdir(parents=True,exist_ok=False)
    export_cut(args.output_directory/'mesh',mesh,reader,history_read,quality)
    exported=time.perf_counter_ns()
    tsv(args.output_directory/'timings.tsv',('scope','milliseconds'),
        (('operand_prepare',(prepared-start)/1e6),('cpu_boolean',(constructed-prepared)/1e6),
         ('mesh_construct',(built-constructed)/1e6),('indexed_reader',(indexed-built)/1e6),
         ('history_reader',(consumed-indexed)/1e6),('prepared_result_to_consumed',(consumed-constructed)/1e6),
         ('independent_qualification',(qualified-consumed)/1e6),('filesystem_export',(exported-qualified)/1e6),
         ('prepare_through_export_including_qualification',(exported-start)/1e6)))
    require(source_pins()==pins,'caller source changed during operation')
    tsv(args.output_directory/'sources.tsv',('path','sha256'),pins)
    print(f'{name}: vertices={len(mesh.vertices)} triangles={len(mesh.triangles)} '
          f'n={mesh.quarter_segments} complete_ms={(consumed-constructed)/1e6:.3f}',flush=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',choices=tuple(case[0] for case in CASES),default='fine-unit')
    parser.add_argument('--output-directory',type=Path,required=True)
    main(parser.parse_args())
