#!/usr/bin/env python3
"""Complete NVIDIA reference attempts over explicit arms; NVIDIA keeps its fastest valid arm.

Arms are ingress arm x preparation x heal policy x worker count x worker
placement (papers/2026-10-17-heterogeneous-placement-measurement.md §7.11;
papers/2026-10-17-occt-ingress-translation-edits.md §5). Every arm runs in its
own caller process, launched already bound to one lane node. Each arm first
runs a calibration pass (prepare mode); its measured pass gets an LPT
assignment from measured per-definition headline CPU (a 24-worker arm uses
the 12-worker pinned pass of the same arm when there is one, since SMT
siblings contend). Measured passes run in a seeded random order. Before each,
G8 §7.10 settle checks classify the foreign load (host busy minus our own
tree); the run's own foreign CPU must also meet the bound. A loaded measured
pass is retried up to --quiet-tries times; an arm that never ran quiet is
labelled not_measured_quiet.

NVIDIA's time is the caller's nvidia_headline scope: NVIDIA's ingress (the
minimal OCCT STEP-to-.brep process) plus the worker makespan of NVIDIA's own
work, process start to exit. Our preparation of its input, our admission,
emit, display and measurement are listed per arm and excluded. A preflight
refuses harness problems before any arm; a caller refusal stops the campaign
as harness_refused. Our ingress or post-batch failures are labelled
ingress_refused or ours_failed, never NVIDIA failures. reference-outcomes.tsv
keeps one row per heal policy: its fastest valid calibrated arm measured
quiet, with any faster arm not measured quiet named beside it. Every row is
n=1: not a headline number (G8 §7.5 needs at least 10 fresh processes).
"""
import argparse
import csv
import itertools
import os
from pathlib import Path
import random
import subprocess
import sys
import tempfile
import time

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'examples')]
from render_om10_nvidia import (HARNESS_REFUSED,INGRESS_REFUSED,OURS_FAILED,INGRESS_ARMS,HarnessRefusal,digest,
                                exporter_declaration,exporter_shape_processing,read_tsv,refuse,require,tsv,matched_view_path)
from brepgpu.nvidia_reference import load_native_model_match
from qualify_om10_nvidia import qualified_owned_inputs
from measurement_lane import (CONTEXT_COLUMNS,Recorder,bound,delivered_clock,lane_cpus,load_class,measure_quietly,
                              node_lane,numactl,read_topology)


COLUMNS=('config','view','scope','n','status','complete_seconds','elapsed_seconds',
         'source_sha256','views_sha256','detail')
ARM_COLUMNS=('arm','try','tries','representative','finite_quality','ingress_arm','preparation','heal','workers','worker_placement','lane_node',
             'assignment_basis','calibration_source','curve_angle_degrees','surface_angle_degrees','load_class',
             'quiet_status','settle_samples','settle_foreign_cpu_percent','run_foreign_cpu_percent','env_pad_bytes',
             'caller_reexecuted','nvidia_headline_seconds','nvidia_headline_excluding_ingress_seconds',
             'nvidia_only_worker_makespan_seconds','nvidia_batch_wall_seconds','nvidia_ingress_seconds',
             'requires_our_preparation','required_input_preparation_seconds','input_preparation_as_run_seconds',
             'as_run_total_seconds','nvidia_failed_attempt_seconds','admission_seconds','emit_seconds',
             'operation_wall_seconds','context_id',*COLUMNS)
CALIBRATED='measured_definition_cpu_seconds'
FIXTURE=ROOT/'tests/fixtures/reference-source.step'
NOT_A_HEADLINE='n=1, not a headline (G8 §7.5 needs at least 10 fresh processes)'
# NVIDIA's arm is valid when its batch and our admission passed. OURS_FAILED
# (our display or presentation failed afterwards) keeps NVIDIA's time valid.
NVIDIA_VALID_OURS_FAILED='OURS_FAILED'


def classify_exit(code):
    """(status, scope) of a caller exit that is not a success."""
    return {HARNESS_REFUSED:('HARNESS_REFUSED','harness_refused'),
            INGRESS_REFUSED:('INGRESS_REFUSED','ingress_refused'),
            OURS_FAILED:(NVIDIA_VALID_OURS_FAILED,'ours_failed')}.get(code,('FAIL','failed_pipeline'))


def nvidia_valid(row,valid_status):
    """NVIDIA's arm is valid: its own status passed, or our later work failed after its batch,
    our admission and (finite mode) its geometry quality passed."""
    if row['status']==valid_status:return True
    return row['status']==NVIDIA_VALID_OURS_FAILED and row.get('finite_quality') in ('PASS','not_applicable')


def eligible(rows,valid_status):
    """Representative, NVIDIA-valid, calibrated rows with a time: the only rows that may be named fastest."""
    return [r for r in rows if r['representative']=='1' and nvidia_valid(r,valid_status)
            and r['assignment_basis']==CALIBRATED and r['complete_seconds']!='']


def finite_outcome(code,quality,display_threads,log_name):
    """(status, scope, detail) of a finite-mode measured pass that ran its batch and admission (exit 0 or 5).

    NVIDIA's time counts only when its finite geometry quality is PASS; our
    later failure (exit 5) then keeps it as OURS_FAILED."""
    if quality!='PASS':
        return ('FAIL','failed_qualification','finite geometry admission FAIL; no matched-quality timing claim'+
                ('; our post-batch work also failed' if code==OURS_FAILED else ''))
    if code==OURS_FAILED:
        return (NVIDIA_VALID_OURS_FAILED,'ours_failed','NVIDIA batch, our admission and finite geometry quality PASS; '
                f'our post-batch work failed; see {log_name}-caller.log')
    return ('PASS','fresh_step_both_labeled_views',f'complete fresh source→both labeled views; NVIDIA CPU kernel + '
            f'shared owned CPU{display_threads} display; finite geometry admission passed separately')


def fastest_valid(rows,valid_status):
    """(fastest measured-quiet row, fastest not-measured-quiet row if faster than it)."""
    candidates=eligible(rows,valid_status)
    quiet=[r for r in candidates if r['quiet_status']=='measured_quiet']
    loud=[r for r in candidates if r['quiet_status']!='measured_quiet']
    best=min(quiet,key=lambda r:float(r['complete_seconds'])) if quiet else None
    faster=min(loud,key=lambda r:float(r['complete_seconds'])) if loud else None
    if best is not None and faster is not None and float(faster['complete_seconds'])>=float(best['complete_seconds']):
        faster=None
    return best,faster


def selected_outcomes(arm_rows,heals,valid_status):
    """reference-outcomes rows: per heal policy, its fastest valid calibrated arm measured quiet.

    A faster arm that was never measured quiet is named beside it, never
    silently dropped; with no quiet arm the fastest such arm is reported and
    labelled not measured quiet."""
    rows=[]
    for heal in heals:
        candidates=[r for r in arm_rows if r['heal']==str(heal) and r['representative']=='1']
        names=','.join(r['arm'] for r in candidates)
        base={k:candidates[0][k] for k in COLUMNS} if candidates else {k:'' for k in COLUMNS}
        best,faster=fastest_valid(candidates,valid_status)
        refused=[r for r in candidates if r['status']=='HARNESS_REFUSED']
        uncalibrated=[r for r in candidates if nvidia_valid(r,valid_status) and r['assignment_basis']!=CALIBRATED]
        ingress=[r for r in candidates if r['status']=='INGRESS_REFUSED']
        ours=[r for r in candidates if r['status']==NVIDIA_VALID_OURS_FAILED and not nvidia_valid(r,valid_status)]
        chosen=best or faster
        if refused or (chosen is None and uncalibrated):
            rows.append({**base,'config':f'heal{heal}','status':'HARNESS_REFUSED','scope':'harness_refused',
                         'complete_seconds':'','detail':'harness refused '+','.join(r['arm'] for r in refused)
                         if refused else 'valid arms ran uncalibrated; no fastest arm is named: '+
                         ','.join(r['arm'] for r in uncalibrated)})
        elif chosen is None and ours:
            rows.append({**base,'config':f'heal{heal}','status':NVIDIA_VALID_OURS_FAILED,'scope':'ours_failed',
                         'complete_seconds':'','detail':'our work failed before NVIDIA\'s time was established: '+
                         ','.join(r['arm'] for r in ours)})
        elif chosen is None and ingress:
            rows.append({**base,'config':f'heal{heal}','status':'INGRESS_REFUSED','scope':'ingress_refused',
                         'complete_seconds':'','detail':'our ingress failed before NVIDIA received input: '+
                         ','.join(r['arm'] for r in ingress)})
        elif chosen is None:
            failed=[r for r in candidates if r['scope']=='failed_qualification' and r['complete_seconds']!='']
            if failed:
                worst=min(failed,key=lambda r:float(r['complete_seconds']))
                rows.append({**{k:worst[k] for k in COLUMNS},'config':f'heal{heal}',
                             'detail':f'no valid arm among {names}; fastest admission failure {worst["arm"]}'})
            else:
                rows.append({**base,'config':f'heal{heal}','status':'FAIL','scope':'failed_pipeline',
                             'complete_seconds':'','detail':f'no valid arm among {names}; per-arm outcomes in arm-outcomes.tsv'})
        else:
            beside=(f'; faster arm {faster["arm"]} at {faster["complete_seconds"]} s was not measured quiet'
                    if best is not None and faster is not None else '')
            label='measured quiet' if best is not None else 'not measured quiet (no arm ran quiet)'
            rows.append({**{k:chosen[k] for k in COLUMNS},'config':f'heal{heal}',
                         'detail':f'fastest valid calibrated arm {chosen["arm"]}, {label}{beside}; '
                                  f'complete_seconds is the NVIDIA headline scope; {NOT_A_HEADLINE}; '+chosen['detail']})
    return rows


def headline(dataset):
    """Scope seconds from a caller's scopes.tsv."""
    return {r['scope']:float(r['seconds']) for r in read_tsv(Path(dataset)/'scopes.tsv')}


def calibrations(paths):
    """Explicit calibration caller outputs keyed by their arm settings."""
    found={}
    for path in paths:
        refuse((Path(path)/'metadata.tsv').is_file(),f'calibration {path} has no metadata')
        fields={r['field']:r['value'] for r in read_tsv(Path(path)/'metadata.tsv')}
        refuse(fields.get('caller_complete')=='1',f'calibration {path} did not complete')
        key=(fields['ingress_arm'],fields['reference_preparation'],int(fields['heal']),int(fields['workers']),
             fields['worker_placement'])
        refuse(key not in found,f'two calibrations for {key}')
        found[key]=(Path(path),fields)
    return found


def context_row(path,context_id):
    """The last context row written for `context_id`."""
    with Path(path).open(newline='',encoding='utf-8') as stream:
        found=[r for r in csv.DictReader(stream,delimiter='\t') if r['context_id']==context_id]
    return found[-1] if found else None


def caller_settle(dataset):
    """The settle samples the caller took immediately before NVIDIA's timed region."""
    path=Path(dataset)/'settle.tsv'
    if not path.is_file():return [],float('inf')
    rows=read_tsv(path)
    samples=[dict(load1=float(r['load1']),foreign=float(r['foreign_cpu_percent']),own=float(r['own_cpu_percent']),
                  host=float(r['host_cpu_percent']),gpu_foreign=int(r['gpu_foreign'])) for r in rows]
    return samples,(float(rows[0]['window_foreign_cpu_percent']) if rows else float('inf'))


def nvidia_region_foreign(dataset):
    """Foreign CPU during NVIDIA's own timed region: the larger of the caller's nvidia_ingress and
    nvidia_batch context rows, not the whole caller (our XDE read, admission and display excluded)."""
    path=Path(dataset)/'context.tsv'
    if not path.is_file():return float('inf')
    values=[]
    for context_id in ('nvidia_ingress','nvidia_batch'):
        row=context_row(path,context_id)
        if row is None or row['foreign_cpu_percent']=='error':return float('inf')
        values.append(float(row['foreign_cpu_percent']))
    return max(values)


def preflight(args,arms,preparations,worker_arms,placements,source_hash):
    """Refuse every harness problem before any arm runs."""
    try:
        refuse(sys.platform=='linux','reference benchmark executes on the fleet')
        numactl()
        topology=read_topology();lane=node_lane(topology,args.lane_node)
        for workers in worker_arms:
            refuse(set(lane_cpus(topology,args.lane_node,workers))<=set(lane.cpus),
                   f'{workers} workers need CPUs of node {args.lane_node} outside this process affinity')
        recorder=Recorder(args.output/'context.tsv',lane,'cold',(('driver','benchmark_om10_nvidia'),))
        delivered_clock(lane.cpus[:1],recorder.probe)
    except HarnessRefusal:raise
    except InterruptedError:raise
    except (ValueError,OSError,subprocess.SubprocessError) as error:raise HarnessRefusal(str(error)) from error
    exporters={}
    for item in args.exporter:
        arm,_,path=item.partition('=')
        refuse(arm in INGRESS_ARMS and path,f'--exporter takes ARM=PATH with ARM in {",".join(INGRESS_ARMS)}')
        exporters[arm]=Path(path)
    exporters.setdefault('occt-no-shape-process',ROOT/'build/om10-reference-source/brepgpu-reference-source')
    needed=[('producer',args.producer),('display',args.display),('package library',args.package/'lib'),
            ('minimal NVIDIA ingress',args.nvidia_ingress)]
    if args.admission=='finite':needed.append(('distance query',args.query))
    if 'periodic-plane' in preparations:
        needed+=[('converter',args.converter),('periodic preparer',args.periodic_preparer),('plane helper',args.plane_helper)]
    missing=[f'{name} {path}' for name,path in needed if not path.exists()]
    refuse(not missing,'missing harness inputs: '+', '.join(missing))
    from brepgpu.reference_source import load_reference_source
    for arm in arms:
        refuse(INGRESS_ARMS[arm] is not None,f'{arm} gives NVIDIA our ingress work and awaits an operator decision')
        refuse(arm in exporters and exporters[arm].is_file(),f'ingress arm {arm} has no built exporter; pass --exporter {arm}=PATH')
        with tempfile.TemporaryDirectory(dir=args.output) as directory:
            result=subprocess.run(bound((exporters[arm],FIXTURE,Path(directory)/'source'),lane),capture_output=True)
            refuse(result.returncode==0,f'exporter for {arm} fails on the authored fixture')
            declared=exporter_shape_processing(load_reference_source(Path(directory)/'source/source.bgocct'))
            result=subprocess.run(bound((args.nvidia_ingress,'--shape-processing',INGRESS_ARMS[arm],FIXTURE,
                                         Path(directory)/'nvidia-input'),lane),capture_output=True)
            refuse(result.returncode==0,f'minimal NVIDIA ingress for {arm} fails on the authored fixture')
        exporter_declaration(arm,declared,exporters[arm])
    explicit=calibrations(args.calibration)
    wanted={(a,p,h,w,c) for a,p,h,w,c in itertools.product(arms,preparations,(0,1),worker_arms,placements)}
    for key,(path,fields) in explicit.items():
        refuse(key in wanted,f'calibration {path} matches no requested arm')
        refuse(fields['source_sha256']==source_hash,f'calibration {path} is from other source bytes')
        refuse(float(fields['chord_height_mm'])==.005,f'calibration {path} chord differs')
        for name,value in (('curve_angle_degrees',args.curve_angle),('surface_angle_degrees',args.surface_angle)):
            refuse(value is None or float(fields[name])==value,f'calibration {path} {name} differs')
    return lane,recorder,exporters,{k:v[0] for k,v in explicit.items()}


def main(args):
    arms=tuple(args.ingress_arms.split(','))
    refuse(arms and len(set(arms))==len(arms) and all(a in INGRESS_ARMS for a in arms),
           'ingress arms must be distinct names from '+','.join(INGRESS_ARMS))
    preparations=tuple(args.preparation_arms.split(','))
    refuse(preparations and len(set(preparations))==len(preparations) and
           all(p in ('none','periodic-plane') for p in preparations),'preparation arms: none, periodic-plane')
    worker_arms=tuple(int(w) for w in args.worker_arms.split(','))
    refuse(worker_arms and len(set(worker_arms))==len(worker_arms) and all(1<=w<=24 for w in worker_arms),
           'worker arms must be distinct counts 1..24')
    placements=tuple(args.placement_arms.split(','))
    refuse(placements and len(set(placements))==len(placements) and all(p in ('pinned','floating') for p in placements),
           'placement arms: pinned, floating')
    refuse(1<=args.qualification_workers<=24 and 1<=args.display_threads<=256,'invalid qualification/display width')
    refuse(all(x is None or 0<=x<float('inf') for x in (args.curve_angle,args.surface_angle)),
           'reference angles must be finite and nonnegative')
    refuse(args.quiet_checks>=1 and args.quiet_interval>=0 and args.quiet_tries>=1,
           'at least one quiet check and one try are required')
    args.output.mkdir(parents=True,exist_ok=False)
    source_hash=digest(args.step)
    lane,recorder,exporters,explicit=preflight(args,arms,preparations,worker_arms,placements,source_hash)
    view_path=matched_view_path(args.matched,args.matched_query)
    views_hash=digest(view_path)
    owned_input=None
    try:
        if (args.matched/'model.tsv').is_file():
            load_native_model_match(args.matched,args.matched_query,.005,source_sha256=source_hash)
            owned_input=args.matched
        elif args.admission=='finite':
            refuse(args.owned_qualification is not None,'legacy matched input requires owned qualification')
            qualified_owned_inputs(args.owned_qualification,source_hash,.005)
            owned_input=args.owned_qualification
    except (HarnessRefusal,InterruptedError):raise
    except (ValueError,OSError,KeyError) as error:raise HarnessRefusal(f'matched input admission: {error}') from error
    seed=args.seed if args.seed is not None else int.from_bytes(os.urandom(8),'little')
    rng=random.Random(seed)
    configs=list(itertools.product(arms,preparations,(0,1),worker_arms,placements))
    calibration_order=rng.sample(configs,len(configs));measured_order=rng.sample(configs,len(configs))
    pads={c:rng.randrange(0,4097) for c in configs}
    def name(arm,preparation,heal,workers,placement):
        return f'{arm}-{preparation}-heal{heal}-w{workers}-{placement}'
    tsv(args.output/'run-plan.tsv',('field','value'),
        (('seed',seed),('calibration_order',','.join(name(*c) for c in calibration_order)),
         ('measured_order',','.join(name(*c) for c in measured_order)),
         ('quiet_checks',args.quiet_checks),('quiet_interval_seconds',args.quiet_interval),
         ('quiet_tries',args.quiet_tries),
         ('env_pad_bytes',';'.join(f'{name(*c)}={pads[c]}' for c in configs))))
    def execute(command,log,context_id,labels,config=None):
        environment=dict(os.environ)
        if config is not None:environment['BREPGPU_ENV_PAD']='x'*pads[config]
        with recorder.timing(context_id,labels=labels):
            start=time.perf_counter()
            with log.open('wb') as stream:
                result=subprocess.run(bound(command,lane),stdout=stream,stderr=subprocess.STDOUT,env=environment)
            elapsed=time.perf_counter()-start
        return result.returncode,elapsed
    def caller(config,dataset,mode,calibration,settle=False):
        arm,preparation,heal,workers,placement=config
        command=(sys.executable,'-B',ROOT/'examples/render_om10_nvidia.py','--step',args.step,
            '--matched',args.matched,'--output',dataset,'--heal',str(heal),'--mode',mode,
            '--workers',str(workers),'--worker-placement',placement,'--chord-height','.005',
            '--matched-query',str(args.matched_query),
            '--display-threads',str(args.display_threads),'--exporter',exporters[arm],
            '--nvidia-ingress',args.nvidia_ingress,'--producer',args.producer,
            '--display',args.display,'--package',args.package,
            '--reference-preparation',preparation,
            '--converter',args.converter,'--periodic-preparer',args.periodic_preparer,
            '--plane-helper',args.plane_helper,'--lane-node',str(args.lane_node),'--ingress-arm',arm)
        if calibration is not None:command+=('--calibration',calibration)
        if settle:command+=('--settle-checks',str(args.quiet_checks),'--settle-interval',str(args.quiet_interval))
        if args.allow_fixture:command+=('--allow-fixture',)
        for option,value in (('--curve-angle',args.curve_angle),('--surface-angle',args.surface_angle)):
            if value is not None:command+= (option,str(value))
        return command
    def labels(config):
        arm,preparation,heal,workers,placement=config
        return (('ingress_arm',arm),('preparation',preparation),('heal',heal),('workers',workers),
                ('worker_placement',placement))
    def admission(dataset,output):
        return (sys.executable,ROOT/'scripts/qualify_om10_nvidia.py','--dataset',dataset,
            '--owned-qualification',owned_input,'--output',output,'--matched-query',str(args.matched_query),
            '--full','--workers',str(args.qualification_workers),'--query',args.query)
    def geometry_pins(dataset):
        rows=read_tsv(dataset/'geometry-pins.tsv')
        return tuple((r['source_definition_id'],r['sha256']) for r in rows)
    arm_rows=[]
    valid_status='COMPLETE_UNQUALIFIED' if args.admission=='transport' else 'PASS'
    def finish():
        tsv(args.output/'arm-outcomes.tsv',ARM_COLUMNS,([r[k] for k in ARM_COLUMNS] for r in arm_rows))
        selected=selected_outcomes(arm_rows,(0,1),valid_status)
        tsv(args.output/'reference-outcomes.tsv',COLUMNS,([r[k] for k in COLUMNS] for r in selected))
        best,faster=fastest_valid(arm_rows,valid_status)
        tsv(args.output/'fastest-arm.tsv',('selection',*ARM_COLUMNS),
            ((role,*[row[k] for k in ARM_COLUMNS]) for role,row in
             (('fastest_measured_quiet',best),('faster_not_measured_quiet',faster)) if row is not None))
    def record(config,dataset,context_id,outcome,attempt=1,representative=True,load='not_measured',quiet='',
               samples='',settle_foreign='',run_foreign='',finite_quality=''):
        arm,preparation,heal,workers,placement=config
        fields=({r['field']:r['value'] for r in read_tsv(dataset/'metadata.tsv')}
                if (dataset/'metadata.tsv').is_file() else {})
        bases={r['basis'] for r in read_tsv(dataset/'assignments.tsv')} if (dataset/'assignments.tsv').is_file() else set()
        scopes=headline(dataset) if (dataset/'scopes.tsv').is_file() else {}
        def scope(key):
            return format(scopes[key],'.17g') if key in scopes else ''
        row=dict(zip(COLUMNS,outcome))
        if row['status'] in (valid_status,'FAIL',NVIDIA_VALID_OURS_FAILED) and row['complete_seconds']!='':
            row['complete_seconds']=scope('nvidia_headline')
        arm_rows.append({'arm':name(*config),'try':str(attempt),'tries':'1','finite_quality':finite_quality,
            'representative':'1' if representative else '0',
            'ingress_arm':arm,'preparation':preparation,'heal':str(heal),
            'workers':str(workers),'worker_placement':placement,'lane_node':str(args.lane_node),
            'assignment_basis':bases.pop() if len(bases)==1 else 'not_reached',
            'calibration_source':fields.get('calibration_source',''),
            'curve_angle_degrees':fields.get('curve_angle_degrees',''),
            'surface_angle_degrees':fields.get('surface_angle_degrees',''),
            'load_class':load,'quiet_status':quiet,'settle_samples':samples,
            'settle_foreign_cpu_percent':settle_foreign,'run_foreign_cpu_percent':run_foreign,
            'env_pad_bytes':str(pads[config]),'caller_reexecuted':fields.get('caller_reexecuted',''),
            'nvidia_headline_seconds':scope('nvidia_headline'),
            'nvidia_headline_excluding_ingress_seconds':scope('nvidia_headline_excluding_ingress'),
            'nvidia_only_worker_makespan_seconds':scope('nvidia_only_worker_makespan'),
            'nvidia_batch_wall_seconds':scope('nvidia_batch_wall'),
            'nvidia_ingress_seconds':scope('nvidia_ingress_process_wall'),
            'requires_our_preparation':fields.get('headline_requires_our_preparation',''),
            'required_input_preparation_seconds':scope('required_input_preparation_sum'),
            'input_preparation_as_run_seconds':scope('input_preparation_as_run_sum'),
            'as_run_total_seconds':scope('as_run_total'),
            'nvidia_failed_attempt_seconds':scope('nvidia_failed_attempt_sum'),
            'admission_seconds':scope('physical_admission'),'emit_seconds':scope('emit_decode_scene_pack_write'),
            'operation_wall_seconds':row['elapsed_seconds'],'context_id':context_id,**row})
        finish()
    def refused(config,dataset,context_id,code,elapsed):
        record(config,dataset,context_id,(name(*config),'both','harness_refused',1,'HARNESS_REFUSED','',
               format(elapsed,'.17g'),source_hash,views_hash,
               f'caller refused (exit {code}); a harness or input problem, not an NVIDIA outcome; see logs'))
        raise HarnessRefusal(f'caller refused arm {name(*config)}; campaign stopped')
    # Calibration passes: each arm's own uncalibrated run, unless an explicit
    # calibration for that arm was given.
    calibration_passes={}
    for config in calibration_order:
        config_name=name(*config);dataset=args.output/(config_name+'-prepare');begin=time.perf_counter()
        code,_=execute(caller(config,dataset,'prepare',explicit.get(config)),args.output/f'{config_name}-prepare.log',
                       config_name+'-prepare',labels(config),config)
        if code==HARNESS_REFUSED:refused(config,dataset,config_name+'-prepare',code,time.perf_counter()-begin)
        check_code=None;quality=None;complete_outcome=code==0
        if code==0 and args.admission=='finite':
            qout=args.output/(config_name+'-pre-admission')
            check_code,_=execute(admission(dataset,qout),args.output/f'{config_name}-pre-admission.log',
                                 config_name+'-pre-admission',labels(config))
            complete_outcome=False
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
        calibration_passes[config]=(dataset,code,check_code,complete_outcome,quality,time.perf_counter()-begin)
    tsv(args.output/'preparation-outcomes.tsv',('config','prepare_exit','admission_exit',
        'complete_nonvisual_reference_outcome','admission_status','elapsed_seconds'),
        ((name(*c),v[1],'' if v[2] is None else v[2],v[3],v[4] or 'NOT_COMPLETED',format(v[5],'.17g'))
         for c,v in calibration_passes.items()))
    def calibration_for(config):
        """Explicit calibration, else the 12-worker pinned pass for a 24-worker arm, else the arm's own pass."""
        if config in explicit:return explicit[config]
        arm,preparation,heal,workers,placement=config
        w12=(arm,preparation,heal,12,'pinned')
        if workers==24 and w12 in calibration_passes and calibration_passes[w12][1]==0:
            return calibration_passes[w12][0]
        return calibration_passes[config][0]
    from brepgpu.reference_source import load_reference_source
    def measured(config,config_name,dataset,prepared,attempt):
        begin=time.perf_counter()
        # The caller settles immediately before NVIDIA's timed region (after
        # our XDE read), as an owned run settles before its own command.
        code,complete=execute(caller(config,dataset,'render',calibration_for(config),settle=True),
                              args.output/f'{dataset.name}-caller.log',dataset.name,labels(config),config)
        if code==HARNESS_REFUSED:refused(config,dataset,dataset.name,code,time.perf_counter()-begin)
        samples,settle_foreign=caller_settle(dataset)
        run_foreign=nvidia_region_foreign(dataset)
        value='';check_code=None;quality=calibration_passes[config][4];finite_quality='not_applicable' 
        nvidia_admitted=((dataset/'physical.tsv').is_file() and (dataset/'geometry-pins.tsv').is_file() and
                         all(r['status']=='PASS' for r in read_tsv(dataset/'physical.tsv')))
        if code and code!=OURS_FAILED:
            status,scope=classify_exit(code)
            detail=f'measured caller exit{code}; see {dataset.name}-caller.log'
        elif code==OURS_FAILED and not nvidia_admitted:
            # Our work failed before NVIDIA's batch and admission completed: no NVIDIA time.
            status,scope=classify_exit(code);finite_quality='not_established'
            detail=f'our work failed before NVIDIA\'s batch and admission completed; see {dataset.name}-caller.log'
        elif args.admission=='transport':
            metadata={r['field']:r['value'] for r in read_tsv(dataset/'metadata.tsv')} if code==0 else {}
            source=load_reference_source(dataset/'source/source.bgocct')
            physical=read_tsv(dataset/'physical.tsv')
            inventory=(len(source.definitions),len(source.faces)) if args.allow_fixture else (107,9162)
            require(source.source_sha256==source_hash,'completed transport source differs')
            require((len(source.definitions),len(source.faces))==inventory and len(physical)==inventory[0]
                    and {int(r['definition_id']) for r in physical}=={d.id for d in source.definitions}
                    and all(r['status']=='PASS' for r in physical),
                    'unqualified completion still requires full original inventory and physical shells')
            value='headline'
            if code==OURS_FAILED:
                status,scope=classify_exit(code)
                detail=('NVIDIA batch and our physical admission PASS; our display or presentation failed afterwards; '
                        f'see {dataset.name}-caller.log')
            else:
                require(metadata['views_sha256']==views_hash and
                        (args.allow_fixture or int(metadata['occurrences'])==166),
                        'completed transport camera or occurrences differ')
                status,scope=valid_status,'fresh_source_nominal_request_both_views'
                detail=('complete source-to-both-images process; inventory and physical admission PASS '
                        f'({metadata["physical_admission_kind"]}); independent distance qualification not performed; '
                        f'nominal chord_mm={metadata["chord_height_mm"]}, angles {metadata["curve_angle_degrees"]}/'
                        f'{metadata["surface_angle_degrees"]}, preparation={metadata["reference_preparation"]}, '
                        f'heal={metadata["heal"]}, ingress_arm={metadata["ingress_arm"]}, workers={metadata["workers"]} '
                        f'{metadata["worker_binding"]}; no matched-quality speedup claim')
        else:
            # NVIDIA's geometry pins are written with its admission, so an arm
            # whose later work failed (exit 5) is re-qualified the same way.
            if geometry_pins(dataset)!=geometry_pins(prepared):
                qout=args.output/f'{dataset.name}-changed-geometry-admission'
                check_code,_=execute(admission(dataset,qout),args.output/f'{dataset.name}-changed-geometry-admission.log',
                                     dataset.name+'-changed-geometry-admission',labels(config))
                quality='PASS' if check_code==0 else 'FAIL'
            status,scope,detail=finite_outcome(code,quality,args.display_threads,dataset.name)
            value='headline';finite_quality=quality
        elapsed=time.perf_counter()-begin
        require(digest(args.step)==source_hash and digest(view_path)==views_hash,
                'original source or actual matched views changed during reference attempt')
        record(config,dataset,dataset.name,(config_name,'both',scope,1,status,value,format(elapsed,'.17g'),
               source_hash,views_hash,detail+f'; operation wall {complete:.6f}s'),attempt,False,
               load_class(samples,settle_foreign,run_foreign),'',
               ';'.join(f'{s["foreign"]:.3f}/{s["own"]:.3f}/{s["load1"]:.2f}/{s["gpu_foreign"]}' for s in samples),
               format(settle_foreign,'.3f'),format(run_foreign,'.3f'),finite_quality)
        return len(arm_rows)-1,run_foreign,status in (valid_status,NVIDIA_VALID_OURS_FAILED),samples,settle_foreign
    for config in measured_order:
        config_name=name(*config)
        prepared,prepare_code,check_code,completed,quality,prepare_elapsed=calibration_passes[config]
        if prepare_code or not completed:
            status,scope=classify_exit(prepare_code) if prepare_code else ('FAIL','failed_qualification')
            record(config,prepared,config_name+'-prepare',
                   (config_name,'both',scope,1,status,'',format(prepare_elapsed,'.17g'),source_hash,views_hash,
                    f'calibration pass exit{prepare_code}, admission exit{check_code}; no measured pass; '
                    f'see {config_name}-prepare'),finite_quality='not_established')
            continue
        # The shared quiet gate and retry rule (measurement_lane.measure_quietly),
        # the same one owned runs use: the first quiet try represents the arm,
        # else its first try, labelled not measured quiet.
        def attempt(index):
            dataset=args.output/(config_name if index==1 else f'{config_name}-try{index}')
            return measured(config,config_name,dataset,prepared,index)
        for entry in measure_quietly(attempt,args.quiet_checks,args.quiet_interval,args.quiet_tries,
                                     settle_in_attempt=True):
            row=arm_rows[entry['result']]
            row['load_class']=entry['load_class'];row['tries']=str(entry['tries'])
            row['representative']='1' if entry['representative'] else '0'
            row['quiet_status']=('measured_quiet' if entry['quiet_status']=='measured_quiet'
                                 else f'arm {config_name} not measured quiet')
        finish()
    finish()


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--step',type=Path,required=True);p.add_argument('--matched',type=Path,required=True)
    p.add_argument('--owned-qualification',type=Path)
    p.add_argument('--matched-query',type=int,default=0)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--ingress-arms',required=True,
                   help='comma list of NVIDIA ingress arms: occt-default-read (OCCT default STEP read, the candidate '
                        'strongest ingress), occt-no-shape-process (today\'s arm), owned-projected-pcurves '
                        '(labelled; awaits an operator decision, refused)')
    p.add_argument('--exporter',action='append',default=[],metavar='ARM=PATH',
                   help='our XDE exporter for an ingress arm; occt-no-shape-process defaults to build/om10-reference-source')
    p.add_argument('--nvidia-ingress',type=Path,default=ROOT/'build/nvidia-ingress/brepgpu-nvidia-ingress')
    p.add_argument('--preparation-arms',default='none,periodic-plane')
    p.add_argument('--worker-arms',default='12,24',help='NVIDIA worker counts, each its own arm (G8 §7.11)')
    p.add_argument('--placement-arms',default='pinned,floating',
                   help='pinned: one logical CPU per worker; floating: workers share the W-CPU set')
    p.add_argument('--lane-node',type=int,default=0,help='NUMA node every arm runs on')
    p.add_argument('--calibration',type=Path,action='append',default=[],
                   help='explicit complete caller output used instead of an arm\'s own calibration pass; repeatable')
    p.add_argument('--seed',type=int,help='arm-order and environment-padding seed; random and recorded if omitted')
    p.add_argument('--quiet-checks',type=int,default=3)
    p.add_argument('--quiet-interval',type=float,default=60.,help='seconds between quiet checks (G8 §7.10)')
    p.add_argument('--quiet-tries',type=int,default=3,help='measured passes per arm until one runs quiet')
    p.add_argument('--qualification-workers',type=int,default=24)
    p.add_argument('--display-threads',type=int,default=24)
    p.add_argument('--producer',type=Path,default=ROOT/'build/om10-nvidia/brepgpu-om10-nvidia')
    p.add_argument('--display',type=Path,default=ROOT/'build/om10-reference/brepgpu-om10-reference-display')
    p.add_argument('--package',type=Path,default=ROOT/'quarantine/nvidia-usd-brep-1.0.0-linux')
    p.add_argument('--query',type=Path,default=ROOT/'build/om10-reference-geometry/brepgpu-om10-reference-geometry')
    p.add_argument('--admission',choices=('finite','transport'),default='finite',
                   help='transport permits complete development images without distance qualification; never quality PASS')
    p.add_argument('--curve-angle',type=float);p.add_argument('--surface-angle',type=float)
    p.add_argument('--converter',type=Path,default=ROOT/'build/nvidia-reference-recovery/brepgpu-nvidia-convert')
    p.add_argument('--periodic-preparer',type=Path,default=ROOT/'build/reference-periodic/brepgpu-reference-periodic')
    p.add_argument('--plane-helper',type=Path,default=ROOT/'build/nvidia-reference-recovery/brepgpu-nvidia-plane-envelope')
    p.add_argument('--allow-fixture',action='store_true',
                   help='explicit small authored integration fixture: inventory from the exported source; never a watch benchmark')
    try:main(p.parse_args())
    except HarnessRefusal as error:print(f'NVIDIA benchmark harness refused: {error}',file=sys.stderr);sys.exit(HARNESS_REFUSED)
    except Exception as error:print(f'NVIDIA benchmark failed: {error}',file=sys.stderr);sys.exit(1)
