#!/usr/bin/env python3
"""Fresh STEP→NVIDIA CPU tessellation→both shared CPU displays.

Reference only. Run on the fleet after the owned complete-source rendering gate.
Qualification is separate; similarly named tolerance settings do not establish
equal quality. Actual failures keep their reports and never receive a substitute.
"""
import argparse
import csv
import hashlib
import os
from pathlib import Path
import shutil
import signal
import struct
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'python'),str(ROOT/'examples'),str(ROOT/'scripts')]
from brepgpu.imported_assembly import assemble
from brepgpu.nvidia_reference import (load_definition, write_assembly, load_native_model_match,
                                     qualify_physical_definition, IncompleteSourceDefinition)
from om10_scene import PALETTE, authored_material, labeled_svg, visible_anchors, write_png
from render_om10 import callouts
from measurement_lane import (Lane, Recorder, bound, ensure_bound, format_cpus, lane_cpus,
                              node_lane, numactl, prepare_ingress_cache, process_binding, read_topology,
                              reexecuted, settle, shared_libraries)


# NVIDIA reads only .brep, so every NVIDIA arm starts from an OCCT STEP read
# (papers/2026-10-17-occt-ingress-translation-edits.md §5). The value is the
# shape-processing flag of the minimal NVIDIA ingress (native/nvidia_ingress).
INGRESS_ARMS={
    # Today's arm and default: OCCT translation without ShapeProcess. Not
    # NVIDIA's strongest input (curved faces without pcurves, invalid solids).
    'occt-no-shape-process':'none',
    # OCCT's default STEP read (FixShape on): the candidate strongest arm.
    # Its processing time is NVIDIA import time.
    'occt-default-read':'default',
    # Labelled slot: our projected pcurves give NVIDIA our ingress work. It
    # awaits an operator decision and is refused until then.
    'owned-projected-pcurves':None}
# Shape-processing declarations our XDE exporter may make for each arm, current
# first. Exporters since B2-ingress declare translation-only (OCCT translation
# without ShapeProcess, with G1's translation-layer edits named); older ones
# declared none, which is accepted and recorded as such.
EXPORTER_DECLARATIONS={'occt-no-shape-process':('translation-only','none'),'occt-default-read':('default',)}
# The mode our XDE exporter runs in for each arm (brepgpu-reference-source
# --shape-processing): the same OCCT read as the arm's minimal ingress, so its
# face map, placements and styles describe the input NVIDIA receives.
EXPORTER_MODES={'occt-no-shape-process':'translation-only','occt-default-read':'default'}


def exporter_declaration(arm,declared,exporter):
    """Refuse an exporter whose declaration does not fit the arm; name its kind."""
    accepted=EXPORTER_DECLARATIONS.get(arm,())
    refuse(declared in accepted,f'ingress arm {arm} needs an exporter that declares shape-processing='
                                f'{"|".join(accepted) or "(none runnable)"}; {exporter} declares shape-processing={declared}')
    return 'current' if declared==accepted[0] else f'older exporter ({declared})'


# Calibration must match on every field, compared by type. Numbers compare
# as numbers; hashes and names compare exactly.
CALIBRATION_FIELDS={'source_sha256':str,'heal':int,'chord_height_mm':float,'curve_angle_degrees':float,
                    'surface_angle_degrees':float,'reference_preparation':str,'ingress_arm':str,
                    'workers':int,'worker_placement':str}
# Exit statuses that are never NVIDIA outcomes. 3: a harness or input-
# admission problem (harness_refused). 4: our ingress tooling failed before
# NVIDIA received input (ingress_refused). 5: our own work after a valid NVIDIA
# batch and admission failed, such as display or presentation (ours_failed);
# NVIDIA's validity rests on its batch and our admission only. 1: NVIDIA or
# its admitted output failed.
HARNESS_REFUSED,INGRESS_REFUSED,OURS_FAILED=3,4,5
# A preparation worker's exit when its converter service failed (ours).
CONVERTER_SERVICE_FAILED=5


class HarnessRefusal(ValueError):
    """The harness, its inputs or its settings are wrong; NVIDIA was not judged."""


class IngressRefusal(RuntimeError):
    """Our STEP ingress tooling failed; NVIDIA received no input."""


class OursFailure(RuntimeError):
    """Our work after a valid NVIDIA batch and admission failed."""


def refuse(value,message):
    if not value:raise HarnessRefusal(message)


def require(value,message):
    if not value:raise ValueError(message)


PHYSICAL_COLUMNS=('definition_id','status','vertices','edges','triangles','euler','message')


def incomplete_source(matched,source_hash):
    """The native source's definitions that are incomplete at the source (a face refused,
    D-partial-output), read before NVIDIA's ingress; () without a native source. Ours.
    The legacy matched mode carries the adapter's source too (its face comparison), so
    it is read there as well: a definition the adapter leaves incomplete never reaches
    NVIDIA in either mode, and no NVIDIA outcome on it is scored."""
    if not ((Path(matched)/'model.tsv').is_file() or (Path(matched)/'source.bgsource').is_file()):return (),None
    from brepgpu.imported_source import load_source,DEFINITION_COMPLETE
    try:source=load_source(Path(matched)/'source.bgsource')
    except (ValueError,OSError) as error:raise HarnessRefusal(f'matched input admission: {error}') from error
    refuse(source.source_sha256==source_hash,'matched native source digest differs from the STEP')
    return tuple(d.id for d in source.definitions if d.status!=DEFINITION_COMPLETE),source


def decode_reference_mesh(path,definition_id):
    """Our wrapper's .bgrefmesh of NVIDIA's mesh (review round 4): (mesh, None) when it
    decodes; (None, message) when the fault is in the mesh NVIDIA produced
    (ReferenceMeshContent: NVIDIA's outcome, a FAIL row); any other decode error is in our
    producer wrapper's container (OursFailure, exit 5), never scored against NVIDIA."""
    from brepgpu.nvidia_reference import ReferenceMeshContent
    try:
        return load_definition(path,expected_id=definition_id),None
    except ReferenceMeshContent as error:
        return None,f'NVIDIA mesh content: {error}'
    except Exception as error:
        raise OursFailure(f'our producer wrapper\'s mesh of definition {definition_id} does not decode '
                          f'({type(error).__name__}: {error}); not an NVIDIA outcome') from error


def physical_row(mesh,native_match,definition_id):
    """One physical.tsv row: our admission of NVIDIA's mesh. A definition incomplete at the
    source is NOT_EVALUATED (ours), never FAIL against NVIDIA. Only an admission verdict on
    NVIDIA's mesh (AdmissionVerdict) is a FAIL; any other exception is an error in our
    admission code, ours (exit 5; review round 3 ruling), never scored against NVIDIA."""
    from brepgpu.imported_oracles import AdmissionVerdict
    try:
        if native_match is not None:
            stats=qualify_physical_definition(mesh,native_match.source,native_match.source.definition(definition_id))
        else:
            from brepgpu.imported_oracles import qualify_physical_solid
            stats=qualify_physical_solid(dict(zip(mesh.physical_keys,mesh.points)),
                tuple(tuple(mesh.physical_keys[i] for i in t) for t in mesh.triangles))
        return (definition_id,'PASS',stats['vertices'],stats['edges'],stats['triangles'],stats['euler'],'')
    except IncompleteSourceDefinition as error:
        return (definition_id,'NOT_EVALUATED',0,0,0,0,f'{error}; ours, not an NVIDIA outcome')
    except AdmissionVerdict as error:
        return (definition_id,'FAIL',0,0,0,0,str(error))
    except Exception as error:
        raise OursFailure(f'our physical admission of definition {definition_id} failed in our code '
                          f'({type(error).__name__}: {error}); not an NVIDIA outcome') from error


def digest(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        while block:=f.read(1<<20):h.update(block)
    return h.hexdigest()


def tsv(path,header,rows):
    with Path(path).open('w',newline='',encoding='utf-8') as f:
        writer=csv.writer(f,delimiter='\t');writer.writerow(header);writer.writerows(rows)


def read_tsv(path):
    with Path(path).open(newline='',encoding='utf-8') as f:
        return tuple(csv.DictReader(f,delimiter='\t'))


def matched_views(path):
    data=path.read_bytes();row=struct.Struct('<4I19dII2d')
    require(len(data)==16+2*row.size and data[:16]==struct.pack('<8sII',b'BGVIEW01',1,2),
            'matched source must contain exactly two actual views')
    from om10_scene import View
    views=[]
    for i in range(2):
        r=row.unpack_from(data,16+i*row.size)
        require(r[4]==float(i) and r[23]==0,'matched views must render alpha0 thenalpha1')
        views.append(View(r[0],r[1],r[2],r[4],r[5:14],r[14:17],r[17:19],r[19],r[20:23],r[23]))
    return data,tuple(views)


def matched_view_path(path,query=0):
    return path/f'query-{query}/views.bgview' if (path/'model.tsv').is_file() else path/'views.bgview'


def legacy_face_comparison(path,reference_source,source_sha256):
    """The legacy matched mode's face comparison with the STEP source adapter (review
    round 2 ruling: the one the native match has, load_native_model_match): the matched
    directory's adapter source (source.bgsource, read from the same STEP) and our fresh
    reference must give every face the same carrier, side and style."""
    from brepgpu.imported_source import load_source
    native=Path(path)/'source.bgsource'
    require(native.is_file(),'legacy matched inputs lack the adapter source (source.bgsource) for the face comparison')
    source=load_source(native)
    require(source.source_sha256==source_sha256,'legacy matched adapter source digest differs from the STEP')
    expected={f.id:f for f in reference_source.faces}
    require(len(expected)==len(source.faces) and
            all(f.id in expected and (f.carrier_id,f.orientation,f.style_id)==
                (expected[f.id].carrier_id,expected[f.id].orientation,expected[f.id].style_id) for f in source.faces),
            'fresh reference original face metadata differs from the adapter source')


def matched_inputs(path,assembly,source_sha256,chord_height,query=0,reference_source=None):
    """Actual shared authored inputs; never recompute cameras from foreign bounds."""
    if (path/'model.tsv').is_file():
        match=load_native_model_match(path,query,chord_height,source_sha256=source_sha256)
        require(tuple((o.id,o.path,o.product_id,o.definition_id) for o in match.assembly.occurrences)==
                tuple((o.id,o.path,o.product_id,o.definition_id) for o in assembly.occurrences),
                'fresh original occurrence identity differs from native model')
        data,views=matched_views(match.views_path)
        return data,views,match.offsets
    metadata={row['field']:row['value'] for row in read_tsv(path/'metadata.tsv')}
    require(metadata.get('source_sha256')==source_sha256,
            'matched original source SHA differs from fresh reference source')
    if reference_source is not None:legacy_face_comparison(path,reference_source,source_sha256)
    require(float(metadata['tolerance_mm'])==chord_height,
            'requested reference chord height differs from owned mesh target')
    data,views=matched_views(path/'views.bgview')
    expected=read_tsv(path/'bom.tsv')
    require(len(expected)==len(assembly.occurrences),'fresh reference occurrence count differs')
    for row,occurrence in zip(expected,assembly.occurrences):
        require((int(row['occurrence_id']),row['source_usage_path'],int(row['product_id']),int(row['solid_id']))==
                (occurrence.id,'/'.join(map(str,occurrence.path)),occurrence.product_id,occurrence.definition_id),
                'fresh original occurrence identity differs from matched source')
        require(tuple(float(row[f'matrix_{i}']) for i in range(12))==occurrence.transform,
                'fresh original occurrence placement differs from matched source')
    offsets={}
    for row in read_tsv(path/'explosion.tsv'):
        id=int(row['occurrence_id']);require(id not in offsets,'duplicate authored explosion')
        offsets[id]=tuple(float(row[f'world_offset_{axis}_mm']) for axis in 'xyz')
    require(set(offsets)=={o.id for o in assembly.occurrences},'authored explosion incomplete')
    return data,tuple(views),tuple(offsets[o.id] for o in assembly.occurrences)


def reference_description(path):
    """Override only the pinned owned-only helper's fixed exported SVG desc."""
    old='<desc>Source: Association openmovement, OM10 MAN VA01.3. Owned tessellation, illustrative palette, lighting, authored explosion and labels by BRepGPU.</desc>'
    new='<desc>Source: Association openmovement, OM10 MAN VA01.3. Genuine NVIDIA USD/BRep CPU tessellation; shared owned CPU display, illustrative palette, lighting, authored explosion and labels by BRepGPU. Actual geometry admission is reported separately.</desc>'
    data=path.read_text(encoding='utf-8')
    require(data.count(old)==1,'shared presentation description changed; reference override requires review')
    path.write_text(data.replace(old,new),encoding='utf-8')


def assignments(definitions,count,weights=None):
    """Deterministic largest-first (LPT) assignment of definitions to workers.

    G8 §7.11 weights each definition by NVIDIA's own measured cost from a
    calibration run. Without one, the weight is source face count, which the
    caller records as an uncalibrated basis."""
    weight=(lambda d:weights[d.id]) if weights is not None else (lambda d:len(d.faces))
    bins=[[] for _ in range(min(count,len(definitions)))];loads=[0]*len(bins)
    for d in sorted(definitions,key=lambda d:(-weight(d),d.id)):
        i=min(range(len(bins)),key=lambda i:(loads[i],i))
        bins[i].append(d);loads[i]+=weight(d)
    return tuple(tuple(group) for group in bins)


# Who owns each measured producer stage (G9 §0.2 rule 3; B2-harness reviews).
# nvidia: NVIDIA's converter, import/heal, tessellation, its rectangle
#   triangulation call, and (derived below) its object teardown, context
#   setup, process start and exit. ours: prepared-USD copy, f64 extraction,
#   mesh write, report bookkeeping. Preparation stages are classified per row.
PRODUCER_STAGES={'convert_occt_to_usd':'nvidia','import_usd_brep_heal':'nvidia','tessellate':'nvidia',
                 'copy_prepared_usd':'ours','extract_f64_corner_mesh':'ours','write_genuine_mesh':'ours'}
NESTED_NVIDIA='official_rectangle_triangulation_nested_in_extract'
DEFINITION_TIME_COLUMNS=('definition_id','worker','status','measured_seconds','measured_cpu_seconds',
    'nvidia_seconds','nvidia_teardown_seconds','nvidia_failed_attempt_seconds','required_input_seconds',
    'input_preparation_as_run_seconds','ours_converter_process_seconds','ours_output_seconds','weight_cpu_seconds',
    'tessellate_seconds')
WORKER_TIME_COLUMNS=('worker','producer_process_wall_seconds','exec_to_main_seconds','exec_to_main_from_stat_seconds',
    'main_body_seconds','after_main_return_seconds','context_setup_seconds','bookkeeping_seconds','nvidia_seconds',
    'required_input_seconds','nvidia_failed_attempt_seconds','ours_converter_service_seconds',
    'converter_service_idle_seconds')


def _seconds(value):
    return float(value) if value not in ('',None) else 0.


def _preparation_owners(row):
    """[(owner, wall seconds, CPU seconds)] for one preparation stage row.

    An NVIDIA user converts in-process, as the producer does. The preparation
    worker therefore keeps one long-lived converter per worker, so a process's
    first-conversion setup is paid once per worker, as in-process use pays it;
    only each ConvertOcctFileToUsd span is NVIDIA's (its wall stands in for
    CPU, single-threaded), the request round trip beyond it is ours, and the
    server's start, setup and exit are ours per worker (converter-process.tsv).
    A one-process-per-definition converter row is split the same way. A
    refused first conversion, before our repair, is NVIDIA's attempt but never
    part of its headline. A converter without timing lines is charged whole to
    NVIDIA, which can only overstate NVIDIA's time."""
    stage,code=row['stage'],row['exit']
    wall,cpu=_seconds(row['seconds']),_seconds(row.get('cpu_seconds'))
    if stage=='converter_service_failure':
        # A dead, broken or silent converter server: ours, charged once here.
        return [('ours_converter_process',wall,0.)]
    if stage in ('convert_original','convert_periodic_copy'):
        owner='nvidia' if code=='0' else 'nvidia_failed_attempt'
        if row.get('conversion_seconds') in ('',None):return [(owner,wall,cpu)]
        span=_seconds(row['conversion_seconds'])
        return [(owner,span,span),('ours_converter_process',max(0.,wall-span),max(0.,cpu-span))]
    if stage in ('prepare_periodic_copy','prepare_plane_envelopes'):
        # Ours. Charged as required only where it changed NVIDIA's input bytes.
        return [('required_input' if row.get('changed_input')=='1' else 'ours_passthrough',wall,cpu)]
    return []


def definition_times(output,groups,prepared_policy,process_spans):
    """Per-definition and per-worker cost of one run, split by owner.

    A definition's NVIDIA teardown is its definition_complete time minus its
    measured stages (Brep/PolyBrep destruction after the write). A worker's
    producer process is split on CLOCK_MONOTONIC: start is launch (fork) to
    main, exit is main's end to reap. process_spans gives each worker's
    (launch, reap) times from the caller; in prepared mode the preparation
    worker launches the producer and records them itself. weight_cpu_seconds is the
    headline's own per-definition CPU: NVIDIA stages, teardown and the
    input-changing preparation, from process CPU (the nested rectangle call
    has no CPU column; its wall stands in, single-threaded)."""
    reports={}
    for i,_ in enumerate(groups):
        path=output/f'worker-{i:02d}/definitions.tsv'
        if path.is_file():reports.update({int(r['definition_id']):r['status'] for r in read_tsv(path)})
    definitions,workers=[],[]
    for i,group in enumerate(groups):
        producer={};worker={}
        path=output/f'worker-{i:02d}/stages.tsv'
        for r in read_tsv(path) if path.is_file() else ():
            id=int(r['definition_id']);wall,cpu=_seconds(r['seconds']),_seconds(r.get('process_cpu_seconds'))
            if id==0 and r['stage'] in ('process_exec_to_main_from_stat','process_main_start_monotonic','main_body',
                                        'process_main_end_monotonic','worker_context_plugin_setup'):
                worker[r['stage']]=wall
            else:producer.setdefault(id,{})[r['stage']]=(wall,cpu)
        preparation={};batch=None
        path=output/f'preparation-worker-{i:02d}/stages.tsv'
        for r in read_tsv(path) if prepared_policy and path.is_file() else ():
            if r['stage']=='genuine_sdk_prepared_batch':batch=_seconds(r['seconds'])
            elif r['stage']!='complete_worker':preparation.setdefault(int(r['definition_id']),[]).append(r)
        totals={'nvidia':0.,'required_input':0.,'nvidia_failed_attempt':0.};complete_sum=0.
        for d in group:
            stages=producer.get(d.id,{})
            if 'definition_complete_through_object_teardown' not in stages:continue
            complete,complete_cpu=stages['definition_complete_through_object_teardown']
            complete_sum+=complete
            nvidia=ours=measured_stages=measured_cpu=0.;tessellate=''
            for stage,(wall,cpu) in stages.items():
                owner=PRODUCER_STAGES.get(stage)
                if owner is None:continue
                measured_stages+=wall;measured_cpu+=cpu
                if owner=='nvidia':nvidia+=wall
                else:ours+=wall
                if stage=='tessellate':tessellate=format(wall,'.17g')
            nested=stages.get(NESTED_NVIDIA,(0.,0.))[0]
            nvidia+=nested;ours-=nested
            teardown=max(0.,complete-measured_stages)
            nvidia_cpu=sum(stages[s][1] for s in stages if PRODUCER_STAGES.get(s)=='nvidia')+nested+max(0.,complete_cpu-measured_cpu)
            owned={'nvidia':0.,'required_input':0.,'nvidia_failed_attempt':0.,'ours_passthrough':0.,
                   'ours_converter_process':0.}
            owned_cpu=dict(owned);prep_wall=prep_cpu=0.
            for r in preparation.get(d.id,()):
                for owner,wall,cpu in _preparation_owners(r):
                    owned[owner]+=wall;owned_cpu[owner]+=cpu;prep_wall+=wall;prep_cpu+=cpu
            nvidia_total=nvidia+teardown+owned['nvidia']
            for key,value in (('nvidia',nvidia_total),('required_input',owned['required_input']),
                              ('nvidia_failed_attempt',owned['nvidia_failed_attempt'])):totals[key]+=value
            definitions.append(dict(definition_id=d.id,worker=i,status=reports.get(d.id,'MISSING'),
                measured_seconds=format(complete+prep_wall,'.17g'),measured_cpu_seconds=format(complete_cpu+prep_cpu,'.17g'),
                nvidia_seconds=format(nvidia_total,'.17g'),nvidia_teardown_seconds=format(teardown,'.17g'),
                nvidia_failed_attempt_seconds=format(owned['nvidia_failed_attempt'],'.17g'),
                required_input_seconds=format(owned['required_input'],'.17g'),
                input_preparation_as_run_seconds=format(owned['required_input']+owned['ours_passthrough'],'.17g'),
                ours_converter_process_seconds=format(owned['ours_converter_process'],'.17g'),
                ours_output_seconds=format(ours,'.17g'),
                weight_cpu_seconds=format(nvidia_cpu+owned_cpu['nvidia']+owned_cpu['required_input'],'.17g'),
                tessellate_seconds=tessellate))
        if not worker:continue
        span=process_spans.get(i)
        path=output/f'preparation-worker-{i:02d}/producer-process.tsv'
        if prepared_policy and path.is_file():
            r=read_tsv(path)[0];span=(float(r['launch_monotonic']),float(r['reaped_monotonic']))
        setup=worker.get('worker_context_plugin_setup',0.)
        main_start,main_end=worker.get('process_main_start_monotonic'),worker.get('process_main_end_monotonic')
        if span is not None and main_start is not None and main_end is not None:
            wall,start,body,after=span[1]-span[0],main_start-span[0],main_end-main_start,span[1]-main_end
        else:
            wall,start,body,after=None,0.,worker.get('main_body',0.),0.
        bookkeeping=max(0.,body-setup-complete_sum)
        # Each converter server: start through ready and close request through
        # reap are ours; the time it waits while our own preparation runs is
        # idle, listed but not charged; failed requests are charged in their
        # own stage rows.
        service=idle=0.
        path=output/f'preparation-worker-{i:02d}/converter-process.tsv'
        for r in read_tsv(path) if prepared_policy and path.is_file() else ():
            if not (r['launch_monotonic'] and r['ready_monotonic'] and r['close_monotonic'] and r['reaped_monotonic']):
                continue
            launch,ready,close,reaped=(float(r[k]) for k in ('launch_monotonic','ready_monotonic','close_monotonic',
                                                             'reaped_monotonic'))
            service+=max(0.,ready-launch)+max(0.,reaped-close)
            idle+=max(0.,close-ready-float(r['job_round_trip_seconds'])-float(r['failed_request_seconds']))
        workers.append(dict(worker=i,ours_converter_service_seconds=format(service,'.17g'),
            converter_service_idle_seconds=format(idle,'.17g'),producer_process_wall_seconds='' if wall is None else format(wall,'.17g'),
            exec_to_main_seconds=format(start,'.17g'),
            exec_to_main_from_stat_seconds=format(worker.get('process_exec_to_main_from_stat',0.),'.17g'),
            main_body_seconds=format(body,'.17g'),
            after_main_return_seconds=format(after,'.17g'),context_setup_seconds=format(setup,'.17g'),
            bookkeeping_seconds=format(bookkeeping,'.17g'),
            nvidia_seconds=format(start+setup+after+totals['nvidia'],'.17g'),
            required_input_seconds=format(totals['required_input'],'.17g'),
            nvidia_failed_attempt_seconds=format(totals['nvidia_failed_attempt'],'.17g')))
    return definitions,workers


def nvidia_scopes(definitions,workers,ingress_wall,batch_wall):
    """Headline and listed scopes from per-definition and per-worker owners.

    Workers run in parallel, so a batch scope is the makespan over workers.
    The headline is NVIDIA's ingress (the minimal OCCT STEP-to-.brep process,
    timed the same way as owned ingress) plus the makespan of NVIDIA's own
    work per worker: process start, context setup, convert/import/
    tessellate, teardown and exit. Input preparation of ours that NVIDIA
    needed is listed and flagged, never added; the as-run total is listed
    beside it. Failed NVIDIA attempts before our repair are listed apart."""
    def total(rows,key):
        return sum(float(r[key]) for r in rows)
    nvidia=max((float(w['nvidia_seconds']) for w in workers),default=0.)
    with_input=max((float(w['nvidia_seconds'])+float(w['required_input_seconds']) for w in workers),default=0.)
    required=total(definitions,'required_input_seconds')
    flag='; requires our preparation' if required>0 else ''
    return [
        ('nvidia_ingress_process_wall','nvidia',ingress_wall,'minimal STEP->.brep process, fork to exit',True),
        ('nvidia_exec_to_main_sum','nvidia',total(workers,'exec_to_main_seconds'),'producer start, sum over workers',False),
        ('nvidia_worker_context_setup_sum','nvidia',total(workers,'context_setup_seconds'),'sum over workers',False),
        ('nvidia_teardown_sum','nvidia',total(definitions,'nvidia_teardown_seconds'),'object teardown, sum over definitions',False),
        ('nvidia_after_main_return_sum','nvidia',total(workers,'after_main_return_seconds'),'producer exit, sum over workers',False),
        ('nvidia_stage_sum','nvidia',total(definitions,'nvidia_seconds'),'convert/import/tessellate/teardown, sum over definitions',False),
        ('nvidia_only_worker_makespan','nvidia',nvidia,'makespan of NVIDIA-owned worker time'+flag,True),
        ('nvidia_failed_attempt_sum','nvidia_failed_attempt',total(definitions,'nvidia_failed_attempt_seconds'),
         'NVIDIA conversions refused before our repair; never in the headline',False),
        ('required_input_preparation_sum','ours_required_input',required,'our preparation that changed NVIDIA input',False),
        ('input_preparation_as_run_sum','ours',total(definitions,'input_preparation_as_run_seconds'),
         'all our preparation as run, passthroughs included',False),
        ('nvidia_with_required_input_worker_makespan','as_run',with_input,'makespan of NVIDIA-owned time plus required preparation',False),
        ('ours_output_extract_write_sum','ours',total(definitions,'ours_output_seconds'),'copy, extraction and writes',False),
        ('ours_converter_process_overhead_sum','ours',
         total(definitions,'ours_converter_process_seconds')+total(workers,'ours_converter_service_seconds'),
         'converter request round trips beyond the conversion span, failed requests, and each converter server '
         'start, setup and exit; NVIDIA converts in-process',False),
        ('converter_service_idle_sum','ours_idle_listed',total(workers,'converter_service_idle_seconds'),
         'converter servers waiting while our own preparation runs; listed, charged to no one',False),
        ('producer_bookkeeping_sum','ours',total(workers,'bookkeeping_seconds'),'producer report writes and loop overhead',False),
        ('nvidia_batch_wall','as_run',batch_wall,'measured batch wall, all workers through exit',False),
        ('as_run_total','as_run',ingress_wall+batch_wall,'NVIDIA ingress plus measured batch wall',False),
        ('nvidia_headline','headline',ingress_wall+nvidia,'NVIDIA ingress + NVIDIA-only makespan'+flag,False),
        ('nvidia_headline_excluding_ingress','headline',nvidia,'NVIDIA-only makespan'+flag,False)]


SCOPE_COLUMNS=('scope','owner','seconds','basis','in_headline')


def write_scopes(output,rows):
    tsv(output/'scopes.tsv',SCOPE_COLUMNS,((n,o,format(s,'.17g'),b,int(h)) for n,o,s,b,h in rows))


def calibration_weights(directory,source,expected,pins):
    """Per-definition headline CPU seconds from an earlier, complete run of this caller.

    The calibration must come from the same source bytes, NVIDIA settings,
    worker count, worker placement and runtime pins, with one allowance: a
    24-worker arm may calibrate from the 12-worker pinned run, whose per-
    definition CPU is uncontended by SMT siblings. Every worker must have
    exited 0, the run must have written its final metadata, and every
    definition must be present, PASS and cost finite positive CPU."""
    directory=Path(directory)
    refuse((directory/'metadata.tsv').is_file(),f'calibration {directory} has no metadata')
    metadata={r['field']:r['value'] for r in read_tsv(directory/'metadata.tsv')}
    refuse(metadata.get('caller_complete')=='1','calibration run did not complete; its final metadata is missing')
    for field,kind in CALIBRATION_FIELDS.items():
        refuse(field in metadata,f'calibration metadata lacks {field}')
    source_kind='own_settings'
    if (int(metadata['workers']),metadata['worker_placement'])==(12,'pinned') and int(expected['workers'])==24:
        source_kind='w12_pinned_for_w24'
    for field,kind in CALIBRATION_FIELDS.items():
        if source_kind=='w12_pinned_for_w24' and field in ('workers','worker_placement'):continue
        try:same=kind(metadata[field])==kind(expected[field])
        except ValueError:same=False
        refuse(same,f'calibration {field} {metadata[field]} differs from this run {expected[field]}')
    exits=read_tsv(directory/'worker-exits.tsv') if (directory/'worker-exits.tsv').is_file() else ()
    refuse(exits and all(r['exit_code']=='0' for r in exits),'calibration run has a worker that did not exit 0')
    recorded=tuple((r['component'],r['sha256']) for r in read_tsv(directory/'runtime-pins.tsv'))
    refuse(recorded==pins,'calibration runtime pins differ from this run')
    rows=read_tsv(directory/'definition-times.tsv')
    try:weights={int(r['definition_id']):float(r['weight_cpu_seconds']) for r in rows}
    except (KeyError,ValueError):raise HarnessRefusal('calibration definition times lack headline CPU weights')
    refuse(len(weights)==len(rows),'duplicate calibration definition')
    refuse(set(weights)=={d.id for d in source.definitions},'calibration does not cover exactly this source\'s definitions')
    refuse(all(r['status']=='PASS' for r in rows),'calibration contains a definition that did not PASS')
    refuse(all(0<w<float('inf') for w in weights.values()),'calibration CPU seconds must be finite and positive')
    return weights,digest(directory/'definition-times.tsv'),source_kind


def exporter_shape_processing(source):
    """The shape-processing policy the exporter declares in its revision text."""
    values=[t.strip().partition('=')[2] for t in source.adapter_revision.split(';')
            if t.strip().startswith('shape-processing=')]
    refuse(len(values)==1,'reference exporter declares no single shape-processing policy')
    return values[0]


def exporter_raw_corrections(source):
    """Raw-entity corrections our XDE exporter declares ('raw-entity-corrections=...'); none if undeclared."""
    values=[t.strip().partition('=')[2] for t in source.adapter_revision.split(';')
            if t.strip().startswith('raw-entity-corrections=')]
    return values[0] if values else 'none'


def describe_brep(tool,path):
    """Topology counts, pcurve presence and vertex points of one .brep, from the minimal ingress tool."""
    result=subprocess.run((str(tool),'--describe',str(path)),capture_output=True,text=True)
    counts,points={},[]
    for line in result.stdout.splitlines():
        parts=line.split('\t')
        if parts[0]=='vertex':points.append(tuple(map(float,parts[1:4])))
        elif len(parts)==2:counts[parts[0]]=int(parts[1])
    return counts,points


def geometry_diff(tool,pairs,path):
    """Write a geometry diff for differing NVIDIA/XDE file pairs: counts, pcurve presence, max vertex move."""
    rows=[]
    for id,nvidia,ours in pairs:
        (a,pa),(b,pb)=describe_brep(tool,nvidia),describe_brep(tool,ours)
        move=(format(max(max(abs(x-y) for x,y in zip(p,q)) for p,q in zip(pa,pb)),'.17g')
              if len(pa)==len(pb) and pa else 'vertex counts differ')
        rows.append((id,*(f'{a.get(k,"")}/{b.get(k,"")}' for k in
                     ('faces','edges','vertices','face_edge_uses','face_edge_uses_with_pcurve')),move))
    tsv(path,('definition_id','faces_nvidia/ours','edges_nvidia/ours','vertices_nvidia/ours',
              'face_edge_uses_nvidia/ours','with_pcurve_nvidia/ours','max_vertex_coordinate_difference'),rows)


def corrected_definitions(xde_directory):
    """Definitions whose .brep our translation-only export corrected (face sides S1, S2;
    source/records.tsv reference_face_orientation_raw rows, ids 'definition face')."""
    path=Path(xde_directory)/'records.tsv'
    if not path.is_file():return set()
    rows=[line.split('\t') for line in path.read_text().splitlines()[1:]]
    return {int(r[2].split()[0]) for r in rows if len(r)>2 and r[0]=='reference_face_orientation_raw'}


def check_nvidia_input(directory,arm,source,xde_directory,tool):
    """Judge the minimal NVIDIA ingress output; refuse it as ingress (exit 4), never as NVIDIA.

    Its reported shape-processing must be the arm's (no operations for the
    translation-only arm, some for the default read); its solids must be
    exactly the source definitions, each one identity-located, forward-
    oriented SOLID, as our XDE export enforces.

    Translation-only arm (dispatcher ruling, B2-reader-export review round 1):
    a definition whose .brep our export corrected (S1, S2) is NVIDIA's input as
    our corrected file, declared, once our uncorrected translation of it (our
    export's uncorrected/) is byte-identical to the minimal ingress's, else the
    harness refuses (exit 3; review round 2); every other definition's minimal-ingress
    file must be byte-identical to our XDE export of it (both readers run the
    same OCCT translation; the OCCT libraries of both are pinned). Placement
    corrections (P1, P2) change usages, not .brep bytes, so they exempt no
    definition. On a difference a geometry diff is written before refusing
    (exit 4).

    Default-read arm: both reads must report the same OCCT operations, and
    every file must be byte-identical to our default export; otherwise the
    harness refuses (exit 3): the face map would not describe NVIDIA's input.

    Returns (identical count, how the byte check applied, the definitions whose
    NVIDIA input is our corrected file)."""
    policy={r['field']:r['value'] for r in read_tsv(directory/'policy.tsv')}
    wanted=INGRESS_ARMS[arm]
    if policy.get('shape_processing')!=wanted:
        raise IngressRefusal(f'minimal ingress ran shape-processing={policy.get("shape_processing")}, arm needs {wanted}')
    operations=int(policy.get('operations','-1'))
    if (wanted=='none' and (policy.get('flags_supplied')!='1' or operations!=0)) or (wanted=='default' and operations<=0):
        raise IngressRefusal(f'minimal ingress shape-processing flags {policy} do not match arm {arm}')
    rows=read_tsv(directory/'definitions.tsv')
    files={int(r['definition_id']):r for r in rows}
    if len(files)!=len(rows) or set(files)!={d.id for d in source.definitions}:
        raise IngressRefusal('minimal NVIDIA ingress solids differ from the source definitions')
    wrong=[i for i,r in files.items() if (r.get('shape_type'),r.get('identity_location'),r.get('orientation'))!=
           ('solid','1','forward')]
    if wrong:
        raise IngressRefusal(f'minimal NVIDIA ingress roots are not identity-located forward solids: {sorted(wrong)}')
    pairs=[(d.id,directory/files[d.id]['filename'],xde_directory/d.filename) for d in source.definitions]
    differing=[(i,a,b) for i,a,b in pairs if a.read_bytes()!=b.read_bytes()]
    identical=len(pairs)-len(differing)
    declared=exporter_shape_processing(source)
    if wanted!='none':
        ours=[t.strip().partition('=')[2] for t in source.adapter_revision.split(';')
              if t.strip().startswith('shape-process-operations=')]
        refuse(ours==[policy.get('operation_bits')],
               f'minimal ingress ran shape-process operations {policy.get("operation_bits")}, our XDE export {ours}')
        if differing:
            geometry_diff(tool,differing,directory/'nvidia-input-diff.tsv')
        refuse(not differing,f'default-read NVIDIA input differs from our default export for {len(differing)} '
                             'definitions (see nvidia-input/nvidia-input-diff.tsv): our face map would not describe it')
        return identical,'applied: default read, every definition byte-identical',set()
    refuse(declared in ('translation-only','none'),f'translation-only arm with an exporter declaring {declared}')
    corrected=corrected_definitions(xde_directory)&{d.id for d in source.definitions}
    # Review round 2 ruling: before a correction is applied, our uncorrected
    # translation of that definition (written beside our export) must be
    # byte-identical to the minimal ingress's, so the only bytes that differ
    # from NVIDIA's own translation are the declared correction.
    filenames={d.id:d.filename for d in source.definitions}
    for i in sorted(corrected):
        ours=Path(xde_directory)/'uncorrected'/filenames[i]
        refuse(ours.is_file() and ours.read_bytes()==(directory/files[i]['filename']).read_bytes(),
               f'our uncorrected translation of corrected definition {i} differs from the minimal ingress\'s '
               '(or is missing): the correction would not be the only difference from NVIDIA\'s input')
    unexplained=[(i,a,b) for i,a,b in differing if i not in corrected]
    if unexplained:
        geometry_diff(tool,unexplained,directory/'nvidia-input-diff.tsv')
        raise IngressRefusal(f'translation-only NVIDIA input differs from our XDE export for {len(unexplained)} '
                             'definitions we did not correct; see nvidia-input/nvidia-input-diff.tsv')
    return identical,(f'applied per definition; corrected definitions {sorted(corrected)} take our .brep' if corrected
                      else 'applied'),corrected


def pin_paths_for(args,prepared_policy,probe):
    paths=(("reference_source_executable",args.exporter),("nvidia_ingress_executable",args.nvidia_ingress),
        ("nvidia_producer_executable",args.producer),("shared_cpu_display_executable",args.display),
        *((f'package/lib/{name}',args.package/'lib'/name) for name in
          ('libOCCT_BREP_IMPORT.a','libBREP_STAGING.a','libBREP_USD_DATA.so','libSM_API_USD.so',
           'libBREP_SM_USD.so','libUSD_BREP_UTILS.so','libSM_API.so','libSMLib.so')),
        *((f'owned/{name}',ROOT/name) for name in ('examples/render_om10_nvidia.py',
           'examples/om10_scene.py','examples/render_om10.py',
           'python/brepgpu/imported_assembly.py','python/brepgpu/imported_source.py',
           'python/brepgpu/nvidia_reference.py','python/brepgpu/reference_source.py',
           'native/om10_nvidia/main.cpp','native/om10_nvidia/raw_mesh.hpp',
           'native/om10_nvidia/raw_mesh.cpp','native/reference_source/main.cpp','native/nvidia_ingress/main.cpp',
           'native/om10_reference/raw.hpp','native/om10_reference/main.cpp','native/om10_reference/cpu.cpp',
           'scripts/measurement_lane.py')),('delivered_clock_executable',probe),
        # The byte-identity premise: both readers' OCCT libraries, pinned.
        *((f'occt/{role}/{Path(library).name}',Path(library)) for role,tool in
          (('xde_exporter',args.exporter),('nvidia_ingress',args.nvidia_ingress)) for library in shared_libraries(tool)))
    if prepared_policy:
        paths=(*paths,('nvidia_converter_executable',args.converter),
            ('periodic_preparation_executable',args.periodic_preparer),
            ('plane_envelope_executable',args.plane_helper),
            *((f'owned/{name}',ROOT/name) for name in (
                'scripts/prepare_nvidia_reference_worker.py','scripts/prepare_nvidia_plane_envelope.py',
                'native/om10_nvidia/convert.cpp','native/om10_nvidia/plane_envelope.cpp',
                'native/reference_periodic/prepare.cpp','native/reference_periodic/main.cpp')))
    return paths


def run_main(args):
    refuse(sys.platform=='linux','reference product execution belongs on the fleet')
    refuse(1<=args.workers<=24,'reference worker count must be1..24')
    refuse(1<=args.display_threads<=256,'reference display thread count must be1..256')
    refuse(args.chord_height>0 and args.chord_height<float('inf'),'finite positive reference chord required')
    prepared_policy=args.reference_preparation=='periodic-plane'
    curve_angle=(25. if prepared_policy else 0.) if args.curve_angle is None else args.curve_angle
    surface_angle=(25. if prepared_policy else 0.) if args.surface_angle is None else args.surface_angle
    refuse(all(0<=x<float('inf') for x in (curve_angle,surface_angle)),
           'finite nonnegative reference angles required')
    refuse(INGRESS_ARMS[args.ingress_arm] is not None,
           f'ingress arm {args.ingress_arm} gives NVIDIA our own projected pcurves; it is a labelled arm '
           'that awaits an operator decision and is not runnable')
    # G8 §7.7/§7.11: the caller and everything it starts live on one node,
    # memory bound there; each pinned NVIDIA worker gets one logical CPU of it.
    try:
        numactl()
        topology=read_topology();caller_lane=node_lane(topology,args.lane_node)
        refuse(set(lane_cpus(topology,args.lane_node,args.workers))<=set(caller_lane.cpus),
               f'{args.workers} workers need CPUs of node {args.lane_node} outside this process affinity')
        ensure_bound(caller_lane)
        args.output.mkdir(parents=True,exist_ok=False)
        labels=(('caller','render_om10_nvidia'),('ingress_arm',args.ingress_arm),('heal',args.heal),
                ('workers',args.workers),('worker_placement',args.worker_placement),
                ('curve_angle_degrees',curve_angle),('surface_angle_degrees',surface_angle),
                ('reference_preparation',args.reference_preparation))
        recorder=Recorder(args.output/'context.tsv',caller_lane,'cold',labels)
    except HarnessRefusal:raise
    except ValueError as error:raise HarnessRefusal(str(error)) from error
    # Pin hashing (two ~58 MB package archives among them) stays outside the
    # caller's timed bracket.
    started=time.perf_counter()
    pin_paths=pin_paths_for(args,prepared_policy,recorder.probe)
    refuse(all(Path(path).is_file() for _,path in pin_paths),
           'missing harness inputs: '+', '.join(str(path) for _,path in pin_paths if not Path(path).is_file()))
    pins=tuple((name,digest(path)) for name,path in pin_paths)
    tsv(args.output/'runtime-pins.tsv',('component','sha256'),pins)
    pin_seconds=time.perf_counter()-started
    try:
        with recorder.timing('caller'):
            reference_run(args,recorder,topology,caller_lane,prepared_policy,curve_angle,surface_angle,pins)
        started=time.perf_counter()
        refuse(tuple((name,digest(path)) for name,path in pin_paths)==pins,
               'reference executable/library/source changed during caller')
        pin_seconds+=time.perf_counter()-started
    finally:
        # Measurement cost, never NVIDIA's time.
        tsv(args.output/'measurement-overhead.tsv',('scope','seconds'),
            (('context_capture_in_caller_process',format(recorder.capture_seconds,'.17g')),
             ('runtime_pin_hashing_outside_caller_bracket',format(pin_seconds,'.17g'))))


def wait_all(children):
    """The CLOCK_MONOTONIC time each child was reaped."""
    import threading
    ends={}
    def wait(i,process):
        process.wait();ends[i]=time.monotonic()
    threads=[threading.Thread(target=wait,args=(i,process),daemon=True) for i,process,_ in children]
    for thread in threads:thread.start()
    for thread in threads:thread.join()
    return ends


def reference_run(args,recorder,topology,caller_lane,prepared_policy,curve_angle,surface_angle,pins):
    from brepgpu.reference_source import load_reference_source
    capture_before=recorder.capture_seconds
    begin=time.perf_counter();times=[];source_hash=digest(args.step)
    base_env=dict(os.environ,CUDA_VISIBLE_DEVICES='',PXR_WORK_THREAD_LIMIT='1',OMP_NUM_THREADS='1',
             OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
    env=dict(base_env,
             OMNISOLID_PLUGIN_PATH=str(args.package.resolve()/'omniSolid/resources'))
    # Package RPATH and its matching transitive dependencies stay isolated from
    # the separate installed Python wheel. No inherited wheel library override.
    env['LD_LIBRARY_PATH']=os.pathsep.join(str(args.package.resolve()/p) for p in ('lib','extraLibs'))
    if prepared_policy:
        env['PYTHONPATH']=str(args.package.resolve()/'usdpy')
    def run(command,log,environment):
        with log.open('wb') as stream:
            child=subprocess.Popen(tuple(map(str,command)),env=environment,stdout=stream,
                                   stderr=subprocess.STDOUT,start_new_session=True)
            try:
                child.wait()
            finally:
                if child.poll() is None:
                    try:os.killpg(child.pid,signal.SIGTERM)
                    except ProcessLookupError:pass
                    child.wait()
            return child
    residency={}
    # Our XDE read first: products, placements, styles and the face map our
    # display and admission use. Our cost, never NVIDIA's. It runs before the
    # settle checks so NVIDIA's timed region (ingress, then batch) follows them
    # directly, as an owned run's does.
    residency['before_ours_xde_export']=prepare_ingress_cache(args.step,(args.exporter,))
    with recorder.timing('ours_xde_export'):
        start=time.perf_counter()
        result=run(bound((args.exporter,'--shape-processing',EXPORTER_MODES[args.ingress_arm],args.step,args.output/'source'),
                         caller_lane),args.output/'source-export.log',base_env)
        xde_wall=time.perf_counter()-start
        times.append(('ours_xde_manifest_export_process_wall',xde_wall,'ours_xde_export'))
    if result.returncode:raise IngressRefusal('our XDE source export failed; see source-export.log')
    if args.settle_checks:
        samples,window_foreign=settle(args.settle_checks,args.settle_interval)
        tsv(args.output/'settle.tsv',('check','load1','foreign_cpu_percent','own_cpu_percent','host_cpu_percent',
                                      'gpu_foreign','window_foreign_cpu_percent'),
            ((i,*(format(x[k],'.3f') for k in ('load1','foreign','own','host')),x['gpu_foreign'],
              format(window_foreign,'.3f')) for i,x in enumerate(samples)))
    # G8 §7.1, shared policy with owned runs: the STEP file's pages are dropped
    # immediately before each ingress and residency is recorded (libraries
    # recorded, not dropped).
    # A source definition incomplete at ingress (a face refused at the source) is
    # our refusal: NVIDIA receives no input, and no physical row can read FAIL.
    incomplete,native_source=incomplete_source(args.matched,source_hash)
    if incomplete:
        tsv(args.output/'physical.tsv',PHYSICAL_COLUMNS,
            ((d.id,'NOT_EVALUATED',0,0,0,0,'definition incomplete at the source (a face refused); NVIDIA not run'
              if d.id in incomplete else 'NVIDIA not run: the source has incomplete definitions')
             for d in native_source.definitions))
        raise IngressRefusal(f'source definitions {list(incomplete)} are incomplete at the source (faces refused, '
                             'D-partial-output); NVIDIA receives no input')
    residency['before_nvidia_ingress']=prepare_ingress_cache(args.step,(args.nvidia_ingress,))
    # NVIDIA's ingress: the minimal STEP->.brep an OCCT user runs, timed fork
    # to exit like owned ingress. Its files are the input NVIDIA consumes.
    with recorder.timing('nvidia_ingress'):
        start=time.perf_counter()
        result=run(bound((args.nvidia_ingress,'--shape-processing',INGRESS_ARMS[args.ingress_arm],args.step,
                          args.output/'nvidia-input'),caller_lane),args.output/'nvidia-ingress.log',base_env)
        ingress_wall=time.perf_counter()-start
        times.append(('nvidia_ingress_process_wall',ingress_wall,'nvidia_ingress'))
    if result.returncode:raise IngressRefusal('NVIDIA minimal STEP ingress failed; see nvidia-ingress.log')
    tsv(args.output/'page-cache.tsv',('moment','file','resident_fraction'),
        ((moment,name,format(value,'.4f')) for moment,files in residency.items() for name,value in files.items()))
    # Our checks, outside both timed ingress processes.
    source=load_reference_source(args.output/'source/source.bgocct');assembly=assemble(source)
    refuse(source.source_sha256==source_hash and digest(args.step)==source_hash,
           'source bytes changed during fresh original transfer')
    declared=exporter_shape_processing(source)
    declaration=exporter_declaration(args.ingress_arm,declared,args.exporter)
    identical,byte_check,corrected=check_nvidia_input(args.output/'nvidia-input',args.ingress_arm,source,args.output/'source',
                                            args.nvidia_ingress)
    nvidia_inputs={int(r['definition_id']):args.output/'nvidia-input'/r['filename']
                   for r in read_tsv(args.output/'nvidia-input/definitions.tsv')}
    # A definition our translation-only export corrected (S1, S2) reaches NVIDIA
    # as our corrected .brep, declared in metadata; placements never reach
    # NVIDIA (it meshes each definition alone; our display places them from
    # the reference or native source, whose usages are read raw).
    for id in corrected:nvidia_inputs[id]=args.output/'source'/source.definition(id).filename
    if not args.allow_fixture:
        refuse((len(source.definitions),len(source.faces),len(assembly.occurrences))==(107,9162,166),
               'reference must consume complete original107/9162/166 movement')
    native_match=None
    try:
        if (args.matched/'model.tsv').is_file():
            native_match=load_native_model_match(args.matched,args.matched_query,args.chord_height,
                source_sha256=source_hash,reference_source=source)
            assembly=native_match.assembly
            request,views=matched_views(native_match.views_path)
            offsets=native_match.offsets
            shutil.copyfile(native_match.labels_path,args.output/'labels.tsv')
            tsv(args.output/'bom.tsv',('occurrence_id','source_usage_path','product_id','solid_id',
                *(f'matrix_{i}' for i in range(12))),
                ((o.id,'/'.join(map(str,o.path)),o.product_id,o.definition_id,*o.transform)
                 for o in assembly.occurrences))
            tsv(args.output/'explosion.tsv',('occurrence_id','world_offset_x_mm','world_offset_y_mm','world_offset_z_mm'),
                ((o.id,*offset) for o,offset in zip(assembly.occurrences,offsets)))
        else:
            request,views,offsets=matched_inputs(args.matched,assembly,source_hash,args.chord_height,
                                                 reference_source=source)
            for name in ('bom.tsv','explosion.tsv'):
                shutil.copyfile(args.matched/name,args.output/name)
    except InterruptedError:raise
    except (ValueError,OSError,KeyError) as error:
        raise HarnessRefusal(f'matched input admission: {error}') from error
    refuse(native_match is not None or args.allow_fixture or not prepared_policy,
           'prepared watch reference requires the original native source for source-Euler admission')
    (args.output/'views.bgview').write_bytes(request)
    settings=dict(source_sha256=source_hash,heal=args.heal,chord_height_mm=args.chord_height,
                  curve_angle_degrees=curve_angle,surface_angle_degrees=surface_angle,
                  reference_preparation=args.reference_preparation,ingress_arm=args.ingress_arm,
                  workers=args.workers,worker_placement=args.worker_placement)
    # Retain fresh complete input identity even when an official worker refuses.
    tsv(args.output/'metadata.tsv',('field','value'),
        (*settings.items(),('views_sha256',hashlib.sha256(request).hexdigest()),
         ('source_definitions',len(source.definitions)),
         ('source_faces',len(source.faces)),('occurrences',len(assembly.occurrences)),
         ('input_admission','complete_native_model_inventory' if native_match else 'legacy_matched_inputs'),
         ('exporter_shape_processing',declared),('exporter_raw_entity_corrections',exporter_raw_corrections(source)),
         ('nvidia_input_ours_corrected',','.join(map(str,sorted(corrected))) or 'none'),
         ('scope','fresh reference preparation; producer completion not yet established')))
    weights=None;calibration_sha256='';calibration_source='none'
    if args.calibration is not None:
        weights,calibration_sha256,calibration_source=calibration_weights(args.calibration,source,settings,pins)
    basis='measured_definition_cpu_seconds' if weights else 'source_face_count_uncalibrated'
    groups=assignments(source.definitions,args.workers,weights)
    worker_cpus=lane_cpus(topology,args.lane_node,len(groups))
    pinned=args.worker_placement=='pinned'
    def worker_lane(i):
        # Pinned: one logical CPU each. Floating: the scheduler places every
        # worker within the same W-CPU set. Memory stays bound either way.
        return Lane(args.lane_node,(worker_cpus[i],) if pinned else worker_cpus)
    tsv(args.output/'assignments.tsv',('worker','cpus','source_definition_id','source_face_count','weight','basis'),
        ((i,format_cpus(worker_lane(i).cpus),d.id,len(d.faces),format(weights[d.id],'.17g') if weights else len(d.faces),basis)
         for i,group in enumerate(groups) for d in group))
    with recorder.timing('nvidia_batch',Lane(args.lane_node,worker_cpus),
                         (('assignment_basis',basis),('worker_cpus',format_cpus(worker_cpus)))):
        start=time.perf_counter();children=[];launched={}
        try:
            for i,group in enumerate(groups):
                if prepared_policy:
                    command=[sys.executable,str(ROOT/'scripts/prepare_nvidia_reference_worker.py'),
                        '--preparation-directory',str(args.output/f'preparation-worker-{i:02d}'),
                        '--output-directory',str(args.output/f'worker-{i:02d}'),
                        '--producer',str(args.producer.resolve()),'--converter',str(args.converter.resolve()),
                        '--periodic-preparer',str(args.periodic_preparer.resolve()),
                        '--plane-helper',str(args.plane_helper.resolve()),
                        '--heal',str(args.heal),'--chord-height',str(args.chord_height),
                        '--curve-angle',str(curve_angle),'--surface-angle',str(surface_angle)]
                    for d in group:
                        # The faces of NVIDIA's input .brep: the original faces under
                        # translation-only, what OCCT's processing made of them under
                        # the default read (check_nvidia_input holds the two equal).
                        command.extend(('--definition',str(d.id),str(nvidia_inputs[d.id].resolve()),
                                        str(d.translated_face_count)))
                else:
                    command=[str(args.producer),'--heal',str(args.heal),'--chord-height',str(args.chord_height),
                             '--output-directory',str(args.output/f'worker-{i:02d}')]
                    if args.curve_angle is not None or args.surface_angle is not None:
                        command.extend(('--curve-angle',str(curve_angle),'--surface-angle',str(surface_angle)))
                    for d in group:command.extend(('--definition',str(d.id),str(nvidia_inputs[d.id])))
                command=bound(command,worker_lane(i))
                log=(args.output/f'worker-{i:02d}.log').open('wb')
                launched[i]=time.monotonic()
                try:process=subprocess.Popen(command,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
                except BaseException:log.close();raise
                children.append((i,process,log))
            ends=wait_all(children)
            statuses=[(i,process.returncode) for i,process,_ in children]
        finally:
            for _,process,log in children:
                if process.poll() is None:
                    try:os.killpg(process.pid,signal.SIGTERM)
                    except ProcessLookupError:pass
            for _,process,log in children:
                if process.poll() is None:process.wait()
                if not log.closed:log.close()
            elapsed=time.perf_counter()-start
    times.append(('nvidia_batch_through_all_worker_exits_wall',elapsed,'nvidia_batch'))
    tsv(args.output/'worker-exits.tsv',('worker','exit_code'),statuses)
    process_spans={i:(launched[i],ends[i]) for i in ends}
    definition_rows,worker_rows=definition_times(args.output,groups,prepared_policy,process_spans)
    tsv(args.output/'definition-times.tsv',DEFINITION_TIME_COLUMNS,
        ([r[k] for k in DEFINITION_TIME_COLUMNS] for r in definition_rows))
    tsv(args.output/'worker-times.tsv',WORKER_TIME_COLUMNS,([r[k] for k in WORKER_TIME_COLUMNS] for r in worker_rows))
    scopes=nvidia_scopes(definition_rows,worker_rows,ingress_wall,elapsed)
    scopes.append(('ours_xde_manifest_export_wall','ours',xde_wall,'our XDE read for display and admission',False))
    write_scopes(args.output,scopes)
    if any(code==CONVERTER_SERVICE_FAILED for _,code in statuses) and prepared_policy:
        # Our converter service failed twice on one job: ours, never an NVIDIA refusal.
        raise OursFailure('our converter service failed twice on one job; see preparation-worker-*/definitions.tsv')
    reports=[]
    for i,_,_ in children:
        path=args.output/f'worker-{i:02d}/definitions.tsv'
        if path.is_file():reports.extend(read_tsv(path))
    if reports:tsv(args.output/'definitions.tsv',tuple(reports[0]),(tuple(r[k] for k in reports[0]) for r in reports))
    if prepared_policy:
        prep_reports=[]
        for i,_,_ in children:
            path=args.output/f'preparation-worker-{i:02d}/definitions.tsv'
            if path.is_file():prep_reports.extend(read_tsv(path))
        if prep_reports:
            tsv(args.output/'preparation-definitions.tsv',tuple(prep_reports[0]),
                (tuple(r[k] for k in prep_reports[0]) for r in prep_reports))
        require(len(prep_reports)==len(source.definitions) and
                {int(r['definition_id']) for r in prep_reports}=={d.id for d in source.definitions}
                and all(r['status']=='PASS' for r in prep_reports),'reference preparation incomplete')
    require(all(code==0 for _,code in statuses),'NVIDIA strict worker failure; complete definition reports retained')
    require(len(reports)==len(source.definitions) and {int(r['definition_id']) for r in reports}==
            {d.id for d in source.definitions} and all(r['status']=='PASS' for r in reports),
            'NVIDIA batch omitted/duplicated/failed original definition')
    locations={d.id:args.output/f'worker-{i:02d}'/f'definition-{d.id}.bgrefmesh'
               for i,group in enumerate(groups) for d in group}
    with recorder.timing('admission'):
        # Our physical-shell admission of NVIDIA's output: its own scope,
        # outside NVIDIA's time, run for every preparation.
        start=time.perf_counter()
        decoded=[(d.id,*decode_reference_mesh(locations[d.id],d.id)) for d in source.definitions]
        decode=time.perf_counter()-start
        start=time.perf_counter();physical=[]
        kind='shell_and_original_source_euler' if native_match is not None else 'shell_only'
        for id,mesh,fault in decoded:
            physical.append((id,'FAIL',0,0,0,0,fault) if fault else physical_row(mesh,native_match,id))
        meshes=tuple(mesh for _,mesh,_ in decoded)
        admission=time.perf_counter()-start
        times.append(('all_definition_physical_shell_source_euler_admission',admission,'admission'))
    tsv(args.output/'physical.tsv',PHYSICAL_COLUMNS,physical)
    scopes.append(('physical_admission','admission',admission,'our admission of NVIDIA output; never NVIDIA time',False))
    write_scopes(args.output,scopes)
    if any(r[1]=='NOT_EVALUATED' for r in physical):
        raise IngressRefusal('a source definition is incomplete at the source; physical admission not evaluated')
    require(all(r[1]=='PASS' for r in physical),'reference physical shell/source-Euler admission failed')
    # NVIDIA's geometry identity, written with its admission so a driver can
    # re-qualify it even when our later work fails.
    tsv(args.output/'geometry-pins.tsv',('source_definition_id','sha256'),
        ((d.id,digest(locations[d.id])) for d in source.definitions))
    # NVIDIA's batch and our admission passed: NVIDIA's arm is valid. A
    # failure from here on is ours (exit 5), except a harness integrity
    # refusal or a termination.
    try:
        finish_reference(args,recorder,caller_lane,base_env,env,run,source,assembly,offsets,views,request,
                         locations,meshes,decode,scopes,times,settings,declared,declaration,kind,native_match,basis,
                         calibration_sha256,calibration_source,worker_cpus,pinned,identical,byte_check,groups,capture_before,
                         begin,source_hash)
    except (HarnessRefusal,InterruptedError):raise
    except Exception as error:
        raise OursFailure(f'{type(error).__name__}: {error}') from error


def finish_reference(args,recorder,caller_lane,base_env,env,run,source,assembly,offsets,views,request,locations,
                     meshes,decode,scopes,times,settings,declared,declaration,kind,native_match,basis,calibration_sha256,
                     calibration_source,worker_cpus,pinned,identical,byte_check,groups,capture_before,begin,source_hash):
    with recorder.timing('emit'):
        start=time.perf_counter()
        products={p.id:p for p in source.products}
        material_map={d.id:authored_material(products[d.product_id].name) for d in source.definitions}
        materials=tuple((rgb,roughness) for _,rgb,roughness in PALETTE)
        write_assembly(args.output/'scene.bgrefmesh',meshes,assembly,offsets,materials,material_map)
        emit=decode+time.perf_counter()-start
        times.append(('genuine_mesh_decode_scene_pack_write',emit,'emit'))
    scopes.append(('emit_decode_scene_pack_write','emit',emit,'G9 §0.2 rule 3 emit stage; same owned emitter for both producers',False))
    native_seconds=None
    if args.mode=='render':
        with recorder.timing('display'):
            start=time.perf_counter()
            result=run(bound((args.display,'--mesh',args.output/'scene.bgrefmesh','--views',args.output/'views.bgview',
                        '--threads',str(args.display_threads),'--output-directory',args.output/'native'),caller_lane),
                       args.output/'display.log',base_env)
            native_seconds=time.perf_counter()-start
        times.append(('shared_owned_cpu_display_process_wall',native_seconds,'display'))
        scopes.append(('shared_display_process_wall','display',native_seconds,'owned CPU display of both views',False))
        write_scopes(args.output,scopes)
        require(result.returncode==0,'shared owned CPU display failed; see display.log')
        for i,(name,view) in enumerate(zip(('assembled','exploded'),views)):
            with recorder.timing(f'{name}_view'):
                start=time.perf_counter();prefix=args.output/'native'/f'frame-{i:06d}'
                png=args.output/f'{i:03d}-{name}.png'
                write_png(png,view.width,view.height,prefix.with_suffix('.rgba').read_bytes())
                anchors=visible_anchors(prefix.with_suffix('.instance'),view.width,view.height,
                                        valid_ids=tuple(o.id for o in assembly.occurrences))
                labels,rows=callouts(assembly,anchors,view.width,view.height)
                tsv(args.output/f'{i:03d}-{name}-callouts.tsv',('label','visible_anchor_occurrence_id',
                    'source_group_occurrence_ids','anchor_x','anchor_y','label_x','label_y'),rows)
                labeled_svg(args.output/f'{i:03d}-{name}.svg',png,view,title='OM10 / '+name.upper(),
                    subtitle=f'NVIDIA USD/BRep CPU import+tessellation · heal={args.heal} · shared owned CPU{args.display_threads} display',
                    counts=(('source definitions',len(source.definitions)),('source faces',len(source.faces)),
                            ('placed parts',len(assembly.occurrences))),
                    timings=(('CPU display process, both views',f'{native_seconds:.3f} s'),
                             ('requested chord height',f'{args.chord_height:g} mm'),
                             ('actual geometry admission','reported separately')),
                    callouts=labels,
                    attribution='Association openmovement · OM10 MAN VA01.3 · CC BY-SA 3.0 · NVIDIA reference mesh; BRepGPU display and labels')
                reference_description(args.output/f'{i:03d}-{name}.svg')
                seconds=time.perf_counter()-start
                times.append((f'{name}_png_svg_labels_write',seconds,f'{name}_view'))
            scopes.append((f'{name}_png_svg_labels_write','display',seconds,'our presentation',False))
    refuse(digest(args.step)==source_hash,'original source changed during reference caller')
    headline=next(s for n,_,s,_,_ in scopes if n=='nvidia_headline')
    required=next(s for n,_,s,_,_ in scopes if n=='required_input_preparation_sum')
    metadata=(*settings.items(),('views_sha256',hashlib.sha256(request).hexdigest()),
        ('geometry','genuine NVIDIA definition-local f64 tessellation, official rectangle polygon triangulation and retained original oriented per-corner normals'),
        ('polygon_triangulation','official SDK TriangulatePlanar for exact axis-extruded rectangles; per-definition counts in worker polygon-export.tsv'),
        ('scope','fresh STEP through '+('both labeled views' if args.mode=='render' else 'NVIDIA mesh only')),
        ('physical_shell_source_euler','PASS'),('physical_admission_kind',kind),
        ('independent_distance_qualification','not_performed_in_caller'),
        ('allow_partial',False),('exporter_shape_processing',declared),('exporter_declaration',declaration),
        ('nvidia_input','minimal STEPControl_Reader ingress; per definition, files byte-identical to our XDE export: '
                        f'{identical}/{len(source.definitions)}'),
        ('nvidia_input_identical_to_xde',identical),('nvidia_input_byte_check',byte_check),
        # The completed run's metadata keeps what the input judgement found.
        ('exporter_raw_entity_corrections',exporter_raw_corrections(source)),
        ('nvidia_input_ours_corrected',','.join(map(str,sorted(corrected_definitions(args.output/'source')&
                                                              {d.id for d in source.definitions}))) or 'none'),
        ('occt_libraries_shared_by_both_readers',int(shared_libraries(args.exporter)==shared_libraries(args.nvidia_ingress))),('lane_node',args.lane_node),
        ('caller_cpus',format_cpus(caller_lane.cpus)),('caller_memory_policy',process_binding()[1]),
        ('caller_reexecuted',int(reexecuted())),
        ('worker_cpus',format_cpus(worker_cpus)),
        ('worker_binding',('one logical CPU per worker' if pinned else 'every worker floats within the worker CPU set')+
                          ', memory bound to lane node'),
        ('assignment_basis',basis),('calibration_source',calibration_source),
        ('calibration_definition_times_sha256',calibration_sha256),
        ('nvidia_headline_seconds',format(headline,'.17g')),
        ('headline_requires_our_preparation',int(required>0)),
        ('display_threads',args.display_threads),('source_definitions',len(source.definitions)),('source_faces',len(source.faces)),
        ('occurrences',len(assembly.occurrences)),('requested_quality_is_measured_admission',False),
        ('input_admission','complete_native_model_inventory' if native_match else 'legacy_matched_inputs'),
        ('available_cpu_affinity',','.join(map(str,sorted(os.sched_getaffinity(0))))),
        *((key,env[key]) for key in ('PXR_WORK_THREAD_LIMIT','OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS')),
        ('caller_complete',1))
    tsv(args.output/'metadata.tsv',('field','value'),metadata)
    times.append(('caller_through_outputs_before_timing_write_and_interpreter_exit',time.perf_counter()-begin,'caller'))
    times.append(('context_capture_nested_in_caller_wall',recorder.capture_seconds-capture_before,'caller'))
    scopes.append(('context_capture_nested_in_caller','measurement',recorder.capture_seconds-capture_before,
                   'context rows; never NVIDIA time',False))
    write_scopes(args.output,scopes)
    tsv(args.output/'timings.tsv',('stage','seconds','scope','context'),
        ((name,seconds,'actual nested wall; not additive; outer driver includes metadata/timing writes and exit',context)
         for name,seconds,context in times))
    print('NVIDIA reference '+args.mode+' complete; geometry admission remains separate')


def main(args):
    # TERM must enter the same child/process-group cleanup as ordinary errors.
    # This also covers source export and display subprocesses outside the batch.
    terminating=False
    def terminate(signum,frame):
        nonlocal terminating
        if terminating:return
        terminating=True
        raise InterruptedError('reference caller terminated')
    previous=signal.signal(signal.SIGTERM,terminate)
    try:
        return run_main(args)
    finally:
        signal.signal(signal.SIGTERM,previous)


def parser():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--step',type=Path,required=True);p.add_argument('--matched',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--heal',type=int,choices=(0,1),required=True)
    p.add_argument('--mode',choices=('prepare','render'),default='prepare')
    p.add_argument('--matched-query',type=int,default=0);p.add_argument('--display-threads',type=int,default=24);p.add_argument('--workers',type=int,default=24);p.add_argument('--chord-height',type=float,default=.005)
    p.add_argument('--package',type=Path,default=ROOT/'quarantine/nvidia-usd-brep-1.0.0-linux')
    p.add_argument('--exporter',type=Path,default=ROOT/'build/om10-reference-source/brepgpu-reference-source',
                   help='our XDE exporter: products, placements and face map for display and admission (our cost); '
                        'run with --shape-processing set by the ingress arm')
    p.add_argument('--nvidia-ingress',type=Path,default=ROOT/'build/nvidia-ingress/brepgpu-nvidia-ingress',
                   help='minimal STEP->.brep ingress whose files NVIDIA consumes (NVIDIA ingress cost)')
    p.add_argument('--producer',type=Path,default=ROOT/'build/om10-nvidia/brepgpu-om10-nvidia')
    p.add_argument('--reference-preparation',choices=('none','periodic-plane'),default='none')
    p.add_argument('--curve-angle',type=float);p.add_argument('--surface-angle',type=float)
    p.add_argument('--converter',type=Path,default=ROOT/'build/nvidia-reference-recovery/brepgpu-nvidia-convert')
    p.add_argument('--periodic-preparer',type=Path,default=ROOT/'build/reference-periodic/brepgpu-reference-periodic')
    p.add_argument('--plane-helper',type=Path,default=ROOT/'build/nvidia-reference-recovery/brepgpu-nvidia-plane-envelope')
    p.add_argument('--display',type=Path,default=ROOT/'build/om10-reference/brepgpu-om10-reference-display')
    p.add_argument('--allow-fixture',action='store_true',help='explicit small authored integration fixture; never actualwatch benchmark')
    p.add_argument('--lane-node',type=int,default=0,
                   help='NUMA node of the lane: caller, exporter, display and workers run on its CPUs with memory bound to it')
    p.add_argument('--settle-checks',type=int,default=0,
                   help='G8 §7.10 settle checks run immediately before NVIDIA\'s timed region (settle.tsv); 0 skips')
    p.add_argument('--settle-interval',type=float,default=60.)
    p.add_argument('--worker-placement',choices=('pinned','floating'),default='pinned',
                   help='pinned: one logical CPU per worker; floating: workers share the W-CPU set')
    p.add_argument('--calibration',type=Path,
                   help='earlier complete caller output with the same source, NVIDIA settings, worker count, placement '
                        'and pins (a 24-worker arm may use the 12-worker pinned run); workers get an LPT assignment by '
                        'its per-definition headline CPU')
    p.add_argument('--ingress-arm',choices=tuple(INGRESS_ARMS),default='occt-no-shape-process',
                   help='OCCT STEP read that produces the .brep NVIDIA consumes. Default is today\'s arm, OCCT translation '
                        'without ShapeProcess, which is not the candidate strongest input; occt-default-read needs an '
                        'exporter that declares shape-processing=default; owned-projected-pcurves is a labelled arm '
                        'awaiting an operator decision')
    return p


if __name__=='__main__':
    try:main(parser().parse_args())
    except HarnessRefusal as error:
        print(f'NVIDIA reference harness refused: {error}',file=sys.stderr);sys.exit(HARNESS_REFUSED)
    except IngressRefusal as error:
        print(f'NVIDIA reference ingress refused (ours): {error}',file=sys.stderr);sys.exit(INGRESS_REFUSED)
    except OursFailure as error:
        print(f'NVIDIA reference: our work failed (ours, not an NVIDIA outcome): {error}',file=sys.stderr)
        sys.exit(OURS_FAILED)
    except Exception as error:
        print(f'NVIDIA reference failed: {error}',file=sys.stderr);sys.exit(1)
