#!/usr/bin/env python3
"""Two predeclared actual complete reference attempts, no fastest-policy choice."""
import argparse
from pathlib import Path
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'examples')]
from render_om10_nvidia import digest,read_tsv,require,tsv,matched_view_path
from brepgpu.nvidia_reference import load_native_model_match
from qualify_om10_nvidia import qualified_owned_inputs


COLUMNS=('config','view','scope','n','status','complete_seconds','elapsed_seconds',
         'source_sha256','views_sha256','detail')


def main(args):
    require(sys.platform=='linux','reference benchmark executes on the fleet')
    require(1<=args.workers<=24 and 1<=args.display_threads<=256,'invalid reference worker/display width')
    require(all(x is None or 0<=x<float('inf') for x in (args.curve_angle,args.surface_angle)),
            'reference angles must be finite and nonnegative')
    args.output.mkdir(parents=True,exist_ok=False)
    source_hash=digest(args.step)
    view_path=matched_view_path(args.matched,args.matched_query)
    views_hash=digest(view_path)
    if (args.matched/'model.tsv').is_file():
        load_native_model_match(args.matched,args.matched_query,.005,source_sha256=source_hash)
        owned_input=args.matched
    else:
        require(args.owned_qualification is not None,'legacy matched input requires owned qualification')
        qualified_owned_inputs(args.owned_qualification,source_hash,.005)
        owned_input=args.owned_qualification
    def execute(command,log):
        start=time.perf_counter()
        with log.open('wb') as stream:
            result=subprocess.run(tuple(map(str,command)),stdout=stream,stderr=subprocess.STDOUT)
        return result.returncode,time.perf_counter()-start
    def caller(heal,dataset,mode):
        command=(sys.executable,ROOT/'examples/render_om10_nvidia.py','--step',args.step,
            '--matched',args.matched,'--output',dataset,'--heal',str(heal),'--mode',mode,
            '--workers',str(args.workers),'--chord-height','.005','--matched-query',str(args.matched_query),
            '--display-threads',str(args.display_threads),'--exporter',args.exporter,'--producer',args.producer,
            '--display',args.display,'--package',args.package,
            '--reference-preparation',args.reference_preparation,
            '--converter',args.converter,'--periodic-preparer',args.periodic_preparer,
            '--plane-helper',args.plane_helper)
        for option,value in (('--curve-angle',args.curve_angle),('--surface-angle',args.surface_angle)):
            if value is not None:command+= (option,str(value))
        return command
    def admission(dataset,output):
        return (sys.executable,ROOT/'scripts/qualify_om10_nvidia.py','--dataset',dataset,
            '--owned-qualification',owned_input,'--output',output,'--matched-query',str(args.matched_query),
            '--full','--workers',str(args.workers),'--query',args.query)
    def geometry_pins(dataset):
        rows=read_tsv(dataset/'geometry-pins.tsv')
        return tuple((r['source_definition_id'],r['sha256']) for r in rows)
    if args.admission=='transport':
        require(args.reference_preparation=='periodic-plane',
                'development transport mode requires explicit prepared-source physical admission')
        from brepgpu.reference_source import load_reference_source
        outcomes=[]
        for heal in (0,1):
            config=f'heal{heal}';dataset=args.output/config
            code,elapsed=execute(caller(heal,dataset,'render'),args.output/f'{config}-caller.log')
            status,scope,value,detail='FAIL','failed_pipeline','',f'caller exit{code}; retained reports and logs'
            if code==0:
                metadata={r['field']:r['value'] for r in read_tsv(dataset/'metadata.tsv')}
                source=load_reference_source(dataset/'source/source.bgocct')
                physical=read_tsv(dataset/'physical.tsv')
                require(source.source_sha256==source_hash and metadata['source_sha256']==source_hash
                        and metadata['views_sha256']==views_hash,
                        'completed transport source/camera differs')
                require((len(source.definitions),len(source.faces),int(metadata['occurrences']))==(107,9162,166)
                        and len(physical)==107
                        and {int(r['definition_id']) for r in physical}=={d.id for d in source.definitions}
                        and all(r['status']=='PASS' for r in physical)
                        and metadata['physical_shell_source_euler']=='PASS',
                        'unqualified completion still requires full original inventory and physical shells')
                status,scope,value='COMPLETE_UNQUALIFIED','fresh_source_nominal_request_both_views',format(elapsed,'.17g')
                detail=('actual complete source-to-both-images process; original inventory and physical/source-Euler PASS; '
                        'independent distance qualification unavailable/not performed; '
                        f'nominal chord_mm={metadata["chord_height_mm"]}, '
                        f'curve_angle_degrees={metadata["curve_angle_degrees"]}, '
                        f'surface_angle_degrees={metadata["surface_angle_degrees"]}, '
                        f'preparation={metadata["reference_preparation"]}, heal={metadata["heal"]}; '
                        'no matched-quality speedup claim')
            require(digest(args.step)==source_hash and digest(view_path)==views_hash,
                    'original source/views changed during transport attempt')
            outcomes.append((config,'both',scope,1,status,value,format(elapsed,'.17g'),source_hash,views_hash,detail))
            tsv(args.output/'reference-outcomes.tsv',COLUMNS,outcomes)
        return
    preparations={};preparation_rows=[]
    # Complete BOTH nonvisual reference outcomes before any actual NV watch image.
    for heal in (0,1):
        config=f'heal{heal}';dataset=args.output/(config+'-prepare');begin=time.perf_counter()
        code,seconds=execute(caller(heal,dataset,'prepare'),args.output/f'{config}-prepare.log')
        complete_outcome=False;check_code=None;quality=None
        if code==0:
            qout=args.output/(config+'-pre-admission')
            check_code,_=execute(admission(dataset,qout),args.output/f'{config}-pre-admission.log')
            if (qout/'summary.tsv').is_file():
                fields={r['field']:r['value'] for r in read_tsv(qout/'summary.tsv')}
                quality=fields.get('status')
                complete_outcome=(quality in ('PASS','FAIL') and
                    fields.get('queried_definitions')==fields.get('source_definitions')=='107' and
                    fields.get('source_faces')=='9162' and fields.get('source_occurrences')=='166' and
                    fields.get('requested_tolerance_mm')=='0.005' and
                    fields.get('query_capability_status')=='COMPLETE' and
                    fields.get('source_sha256')==source_hash and fields.get('views_sha256')==views_hash and
                    ((check_code==0)==(quality=='PASS')))
        elapsed=time.perf_counter()-begin
        preparations[heal]=(dataset,code,check_code,complete_outcome,quality,elapsed)
        preparation_rows.append((config,code,'' if check_code is None else check_code,
                                 complete_outcome,quality or 'NOT_COMPLETED',format(elapsed,'.17g')))
        tsv(args.output/'preparation-outcomes.tsv',('config','prepare_exit','admission_exit',
            'complete_nonvisual_reference_outcome','admission_status','elapsed_seconds'),preparation_rows)
    outcomes=[]
    for heal in (0,1):
        config=f'heal{heal}';dataset=args.output/config;begin=time.perf_counter()
        prepared,prepare_code,check_code,completed,quality,prepare_elapsed=preparations[heal]
        if prepare_code or not completed:
            status='FAIL';scope='failed_pipeline' if prepare_code else 'failed_qualification';value=''
            detail=(f'no reference images: nonvisual preparation exit{prepare_code}, admission exit{check_code}; '
                    f'see {config}-prepare and {config}-pre-admission; elapsed is actual nonvisual attempt')
            elapsed=prepare_elapsed
        else:
            # A complete finite-quality FAIL may produce an explicitly failed
            # all-parts diagnostic image. Missing-part/pipeline failures cannot.
            code,complete=execute(caller(heal,dataset,'render'),args.output/f'{config}-caller.log')
            if code:
                status,scope,value,detail='FAIL','failed_pipeline','',f'fresh caller exit{code}; see {config}-caller.log and retained perdefinition reports'
            else:
                if geometry_pins(dataset)!=geometry_pins(prepared):
                    qout=args.output/f'{config}-changed-geometry-admission'
                    check_code,_=execute(admission(dataset,qout),args.output/f'{config}-changed-geometry-admission.log')
                    quality='PASS' if check_code==0 else 'FAIL'
                if quality=='PASS':
                    status,scope,detail='PASS','fresh_step_both_labeled_views',f'actual complete fresh source→both labeled views; NVIDIA CPU kernel + shared owned CPU{args.display_threads} display; finite geometry admission passed separately'
                else:
                    status,scope,detail='FAIL','failed_qualification','actual complete all-parts diagnostic render; finite geometry admission FAIL; no matched-quality timing claim'
                value=format(complete,'.17g')
            elapsed=time.perf_counter()-begin
        require(digest(args.step)==source_hash and digest(view_path)==views_hash,
                'original source or actual matched views changed during reference attempt')
        outcomes.append((config,'both',scope,1,status,value,format(elapsed,'.17g'),source_hash,views_hash,detail))
        # Retain the first actual outcome even if the second attempt is interrupted.
        tsv(args.output/'reference-outcomes.tsv',COLUMNS,outcomes)
    require(len(outcomes)==2,'both predeclared reference configurations required')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--step',type=Path,required=True);p.add_argument('--matched',type=Path,required=True)
    p.add_argument('--owned-qualification',type=Path)
    p.add_argument('--matched-query',type=int,default=0)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--workers',type=int,default=24);p.add_argument('--display-threads',type=int,default=24)
    p.add_argument('--exporter',type=Path,default=ROOT/'build/om10-reference-source/brepgpu-reference-source')
    p.add_argument('--producer',type=Path,default=ROOT/'build/om10-nvidia/brepgpu-om10-nvidia')
    p.add_argument('--display',type=Path,default=ROOT/'build/om10-reference/brepgpu-om10-reference-display')
    p.add_argument('--package',type=Path,default=ROOT/'quarantine/nvidia-usd-brep-1.0.0-linux')
    p.add_argument('--query',type=Path,default=ROOT/'build/om10-reference-geometry/brepgpu-om10-reference-geometry')
    p.add_argument('--admission',choices=('finite','transport'),default='finite',
                   help='transport permits complete development images without distance qualification; never quality PASS')
    p.add_argument('--reference-preparation',choices=('none','periodic-plane'),default='none')
    p.add_argument('--curve-angle',type=float);p.add_argument('--surface-angle',type=float)
    p.add_argument('--converter',type=Path,default=ROOT/'build/nvidia-reference-recovery/brepgpu-nvidia-convert')
    p.add_argument('--periodic-preparer',type=Path,default=ROOT/'build/reference-periodic/brepgpu-reference-periodic')
    p.add_argument('--plane-helper',type=Path,default=ROOT/'build/nvidia-reference-recovery/brepgpu-nvidia-plane-envelope')
    try:main(p.parse_args())
    except Exception as error:print(f'NVIDIA benchmark failed: {error}',file=sys.stderr);sys.exit(1)
