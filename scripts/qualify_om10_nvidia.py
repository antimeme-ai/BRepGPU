#!/usr/bin/env python3
"""Finite actual NVIDIA geometry admission, separate from benchmark walls."""
import argparse
import math
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'python'),str(ROOT/'examples')]
from brepgpu.imported_assembly import assemble
from brepgpu.imported_source import load_source
from brepgpu.reference_source import load_reference_source
from brepgpu.nvidia_reference import (load_definition,qualify_physical_definition,
                                      write_source_witnesses)
from render_om10_nvidia import digest,read_tsv,require,tsv


def qualified_owned_inputs(directory,source_sha256,tolerance):
    """Use the completed owned run's existing completion and exact input hashes."""
    directory=Path(directory)
    completion=tuple(row for row in read_tsv(directory/'timings.tsv')
                     if row['stage']=='complete_nonvisual_qualification')
    require(len(completion)==1 and math.isfinite(float(completion[0]['seconds'])) and
            float(completion[0]['seconds'])>=0,'owned nonvisual qualification is incomplete')
    inputs={r['field']:r['value'] for r in read_tsv(directory/'inputs.tsv')}
    require(inputs['source_sha256']==source_sha256,'owned qualification original source differs')
    require(float(inputs['tolerance_mm'])==tolerance,'owned qualified mesh tolerance differs')
    source_path,scene_path=directory/'source.bgsource',directory/'scene.bgmesh'
    require(digest(scene_path)==inputs['scene_sha256'] and
            digest(directory/'views.bgview')==inputs['views_sha256'],
            'completed owned qualification input bytes changed')
    original=load_source(source_path)
    require(original.source_sha256==source_sha256,'completed owned source snapshot differs')
    return original,source_path,scene_path


def query_outcome(summary,code,source_face_count,witness_count,tolerance):
    """Classify the existing helper result; missing answers are not quality FAILs."""
    try:
        fields={r['field']:r['value'] for r in read_tsv(summary)}
        status=fields['status'];fq=int(fields['forward_queries']);rq=int(fields['reverse_queries'])
        ff=int(fields['forward_failures']);rf=int(fields['reverse_failures']);sf=int(fields['forward_solver_failures'])
        selected=int(fields['selected_nv_triangles'])
        require(status in ('PASS','FAIL') and ((code==0)==(status=='PASS')) and code in (0,1),
                'helper exit/status inconsistent')
        require(int(fields['original_faces'])==source_face_count and selected>0 and
                fq==4*selected and rq==witness_count and 0<=sf<=ff<=fq and 0<=rf<=rq,
                'helper actual completed query counts inconsistent')
        require(float(fields['tolerance_mm'])==tolerance and float(fields['numerical_grade_mm'])==2.**-32,
                'helper requested tolerance/numerical grade differs')
        require(all(math.isfinite(float(fields[key])) and float(fields[key])>=0
                    for key in ('forward_max_mm','reverse_max_mm')),'helper numerical maximum invalid')
        require((status=='PASS')==(ff==0 and rf==0),'helper failure counts/status inconsistent')
        if sf:
            return 'SOURCE_QUERY_SOLVER_FAILURE',False,f'{sf} actual source-distance queries have no numerical answer'
        return ('PASS' if status=='PASS' else 'MEASURED_QUALITY_FAIL',True,
                'complete finite numerical outcome; see actual forward/reverse rows')
    except (OSError,ValueError,KeyError) as error:
        return 'SOURCE_QUERY_PREPARATION_OR_EXCEPTION',False,str(error)


def main(args):
    require(sys.platform=='linux','reference qualification executes on the fleet')
    require(1<=args.workers<=24,'finite query worker limit must be1..24')
    args.output.mkdir(parents=True,exist_ok=False);begin=time.perf_counter()
    source=load_reference_source(args.dataset/'source/source.bgocct')
    metadata={r['field']:r['value'] for r in read_tsv(args.dataset/'metadata.tsv')}
    tolerance=float(metadata['chord_height_mm'])
    require(math.isfinite(tolerance) and tolerance>0,'reference requested tolerance invalid')
    original,owned_source,owned_scene=qualified_owned_inputs(args.owned_qualification,source.source_sha256,tolerance)
    require(source.source_sha256==original.source_sha256,'reference/qualified source bytes differ')
    require(source.units_mm==original.units_mm==1.,'reference/qualified millimetre units differ')
    require(source.products==original.products and source.usages==original.usages and
            source.styles==original.styles,'fresh reference original graph/style metadata differs')
    require(source.root_product_id==original.root_product_id,'fresh reference root differs')
    definitions={d.id:d for d in original.definitions}
    require({d.id for d in source.definitions}==set(definitions),'reference original definitions incomplete')
    for d in source.definitions:
        expected=definitions[d.id]
        require(d.product_id==expected.product_id and set(d.face_ids)==set(expected.face_ids),
                f'reference definition #{d.id} original binding/face inventory differs')
        for face in d.faces:
            other=original.face(face.id)
            require((face.carrier_id,face.orientation,face.style_id)==
                    (other.carrier_id,other.orientation,other.style_id),
                    f'reference original face #{face.id} metadata differs')
    require(assemble(source)==assemble(original),'fresh complete original occurrences/placements differ')
    require(metadata['source_sha256']==source.source_sha256,'reference caller/source hash differs')
    require(metadata['views_sha256']==digest(args.dataset/'views.bgview'),'reference requested views changed')
    paths={}
    for path in args.dataset.glob('worker-*/*.bgrefmesh'):
        mesh=load_definition(path)
        require(mesh.id not in paths,'duplicate genuine reference definition output')
        paths[mesh.id]=path
    require(set(paths)==set(definitions),'actual genuine NVIDIA mesh inventory incomplete')
    pinned={int(r['source_definition_id']):r['sha256'] for r in read_tsv(args.dataset/'geometry-pins.tsv')}
    require(set(pinned)==set(paths) and all(digest(paths[id])==pinned[id] for id in paths),
            'actual NVIDIA geometry changed after caller')
    reports={int(r['definition_id']):r for r in read_tsv(args.dataset/'definitions.tsv')}
    require(set(reports)==set(paths),'actual NVIDIA perdefinition report incomplete')
    physical=[];failed=[]
    for d in source.definitions:
        try:
            mesh=load_definition(paths[d.id],expected_id=d.id)
            report=reports[d.id]
            require(report['status']=='PASS' and set(mesh.face_ids)==set(range(int(report['input_faces']))),
                    'actual imported NVface coverage incomplete')
            stats=qualify_physical_definition(mesh,original,definitions[d.id])
            physical.append((d.id,'PASS',stats['vertices'],stats['edges'],stats['triangles'],stats['euler'],''))
        except Exception as error:
            physical.append((d.id,'FAIL',0,0,0,0,str(error)));failed.append(d.id)
    tsv(args.output/'physical.tsv',('definition_id','status','vertices','edges','triangles','euler','detail'),physical)
    witness_counts=write_source_witnesses(owned_scene,args.output/'witnesses',source.definitions)
    tsv(args.output/'witness-counts.tsv',('definition_id','count'),witness_counts)
    query_definitions=(max(source.definitions,key=lambda d:(len(d.faces),-d.id)),) if args.pilot_only else source.definitions
    if args.pilot_definition is not None:
        require(args.pilot_only,'pilot-definition requires pilot-only')
        query_definitions=(source.definition(args.pilot_definition),)
    children=[];exit_rows=[];query_begin=time.perf_counter()
    # Bounded batches keep at most24 independent resident source definitions.
    for start in range(0,len(query_definitions),args.workers):
        try:
            for d in query_definitions[start:start+args.workers]:
                command=(str(args.query),'--source',str(args.dataset/'source'/d.filename),
                    '--mesh',str(paths[d.id]),'--witnesses',str(args.output/'witnesses'/f'definition-{d.id}.bgprobe'),
                    '--tolerance',str(tolerance),'--output-directory',str(args.output/f'query-{d.id}'))
                log=(args.output/f'query-{d.id}.log').open('wb');stamp=time.perf_counter()
                try:p=subprocess.Popen(command,stdout=log,stderr=subprocess.STDOUT,
                                       env=dict(os.environ,CUDA_VISIBLE_DEVICES=''))
                except BaseException:log.close();raise
                children.append((d.id,p,log,stamp))
            for id,p,log,stamp in children:
                code=p.wait();log.close();exit_rows.append((id,code,time.perf_counter()-stamp))
                if code:failed.append(id)
        finally:
            for _,p,_,_ in children:
                if p.poll() is None:p.terminate()
            for _,p,log,_ in children:
                if p.poll() is None:p.wait()
                if not log.closed:log.close()
            children=[]
    tsv(args.output/'query-exits.tsv',('definition_id','exit_code','launch_to_wait_return_upper_bound_seconds'),exit_rows)
    completed_queries=0;query_outcomes=[];witness_count_map=dict(witness_counts)
    for id,code,_ in exit_rows:
        classification,complete,detail=query_outcome(args.output/f'query-{id}/summary.tsv',code,
            len(definitions[id].face_ids),witness_count_map[id],tolerance)
        if complete:completed_queries+=1
        else:failed.append(id)
        query_outcomes.append((id,classification,complete,detail))
    tsv(args.output/'query-outcomes.tsv',('definition_id','outcome','complete_numerical_outcome','detail'),query_outcomes)
    status='FAIL' if failed else 'PILOT_ONLY' if args.pilot_only else 'PASS'
    summary=(('status',status),('source_sha256',source.source_sha256),
        ('owned_source_sha256',digest(owned_source)),('owned_scene_sha256',digest(owned_scene)),
        ('views_sha256',metadata['views_sha256']),('requested_tolerance_mm',tolerance),
        ('source_definitions',len(source.definitions)),('source_faces',len(source.faces)),
        ('source_occurrences',len(assemble(source).occurrences)),('requested_query_definitions',len(query_definitions)),
        ('queried_definitions',completed_queries),
        ('query_capability_status','COMPLETE' if completed_queries==len(query_definitions) else 'INCOMPLETE'),
        ('query_batch_wall_seconds',time.perf_counter()-query_begin),
        ('complete_qualification_wall_seconds',time.perf_counter()-begin),
        ('failed_definition_ids',','.join(map(str,sorted(set(failed))))),
        ('scope','finite original translated boundary/mesh coverage probes + physical shell/source inventory; no universal distance certificate'))
    tsv(args.output/'summary.tsv',('field','value'),summary)
    require(not failed,'reference geometry admission incomplete or failed; see physical.tsv and query-outcomes.tsv')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dataset',type=Path,required=True);p.add_argument('--owned-qualification',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--query',type=Path,default=ROOT/'build/om10-reference-geometry/brepgpu-om10-reference-geometry')
    p.add_argument('--workers',type=int,default=24)
    group=p.add_mutually_exclusive_group(required=True)
    group.add_argument('--pilot-only',action='store_true');group.add_argument('--full',action='store_true')
    p.add_argument('--pilot-definition',type=int)
    try:main(p.parse_args())
    except Exception as error:print(f'NVIDIA qualification failed: {error}',file=sys.stderr);sys.exit(1)
