"""Explicit reference preparation followed by one genuine SDK mesh batch.

Every original conversion is attempted. Geometrically admitted periodic
preparation runs only after a refusal. Plane-range preparation precedes all
SDK healing/tessellation; all failed work remains in the measured operation.
"""
import argparse
import csv
import hashlib
from pathlib import Path
import subprocess
import sys
import time

from prepare_nvidia_plane_envelope import prepare


def require(value, message):
    if not value:
        raise ValueError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fields(path, key='field'):
    with path.open() as stream:
        reader = csv.DictReader(stream, delimiter='\t')
        require(reader.fieldnames == [key,'value'], 'unexpected preparation report schema')
        rows = list(reader)
    result = {r[key]:r['value'] for r in rows}
    require(len(result) == len(rows), 'duplicate preparation status field')
    return result


def tsv(path, header, rows):
    with path.open('w', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t')
        writer.writerow(header)
        writer.writerows(rows)


def main(args):
    jobs = [(int(id), Path(path).resolve(), int(count)) for id,path,count in args.definition]
    require(jobs and len({j[0] for j in jobs}) == len(jobs), 'empty/duplicate definition IDs')
    require(all(0 <= id < 2**64-1 and count > 0 for id,_,count in jobs), 'invalid source inventory')
    require(not args.preparation_directory.exists() and not args.output_directory.exists(),
            'preparation and mesh directories must be new')
    args.preparation_directory.mkdir(parents=True)
    start = time.perf_counter()
    stages, outcomes, prepared = [], [], []

    def run(command, log, definition, stage, timeout=120):
        began = time.perf_counter()
        code = -1
        try:
            with log.open('wb') as stream:
                result = subprocess.run(tuple(map(str,command)), stdout=stream,
                                        stderr=subprocess.STDOUT, timeout=timeout)
            code = result.returncode
            return code
        finally:
            stages.append((definition, stage, time.perf_counter()-began, code))
            tsv(args.preparation_directory/'stages.tsv',('definition_id','stage','seconds','exit'),stages)

    for id, original, original_faces in jobs:
        directory = args.preparation_directory/f'definition-{id}'
        directory.mkdir()
        stage, periodic = 'convert_original', False
        try:
            original_hash = digest(original)
            usd = directory/'original.usdc'
            code = run((args.converter,original,usd), directory/'original-conversion.log', id,stage)
            expected_faces = original_faces
            if code:
                stage, periodic = 'prepare_periodic_copy', True
                derivative = directory/'periodic'
                require(run((args.periodic_preparer,original,derivative),
                            directory/'periodic.log',id,stage) == 0,
                        'original conversion refused and periodic preparation failed')
                status = fields(derivative/'status.tsv', key='key')
                require(status['status'] == 'PASS' and int(status['original_faces']) == original_faces
                        and status['original_face_coverage'] == 'COMPLETE'
                        and status['all_prepared_faces_have_ancestry'] == '1',
                        'periodic preparation lacks complete original/derived face history')
                require((derivative/'source-history.tsv').is_file(), 'periodic face history missing')
                expected_faces = int(status['prepared_faces'])
                usd = directory/'periodic.usdc'
                stage = 'convert_periodic_copy'
                require(run((args.converter,derivative/'prepared.brep',usd),
                            directory/'prepared-conversion.log',id,stage) == 0,
                        'official conversion still refuses prepared periodic representation')
            stage = 'prepare_plane_envelopes'
            envelope = directory/'plane'
            envelope.mkdir()
            began = time.perf_counter()
            plane_exit = 1
            try:
                corrected = prepare(usd,envelope/'source',args.plane_helper)
                plane_exit = 0
            finally:
                stages.append((id,stage,time.perf_counter()-began,plane_exit))
            policy = fields(envelope/'preparation.tsv')
            require(int(policy['source_faces']) == expected_faces,
                    'converted USD did not retain every original/prepared face')
            require(digest(original) == original_hash, 'original BRep bytes changed')
            prepared.append((id,corrected,expected_faces))
            outcomes.append((id,'PASS',original_faces,expected_faces,int(periodic),
                             original_hash,digest(corrected),''))
        except Exception as error:
            outcomes.append((id,'FAIL',original_faces,0,int(periodic),'','',stage+': '+str(error)))
            print(f'definition {id} {stage}: {error}',file=sys.stderr,flush=True)
        tsv(args.preparation_directory/'definitions.tsv',
            ('definition_id','status','original_faces','prepared_faces','periodic_preparation',
             'original_brep_sha256','prepared_usd_sha256','message'),outcomes)
        tsv(args.preparation_directory/'stages.tsv',('definition_id','stage','seconds','exit'),stages)

    code = 1
    if prepared:
        command = [args.producer,'--heal',str(args.heal),'--chord-height',str(args.chord_height),
                   '--curve-angle',str(args.curve_angle),'--surface-angle',str(args.surface_angle),
                   '--output-directory',args.output_directory]
        for id,usd,_ in prepared:
            command.extend(('--usd-definition',str(id),usd))
        code = run(command,args.preparation_directory/'native-producer.log',0,
                   'genuine_sdk_prepared_batch',timeout=900)
        if not code:
            with (args.output_directory/'definitions.tsv').open() as stream:
                reports = list(csv.DictReader(stream,delimiter='\t'))
            require({int(r['definition_id']):int(r['input_faces']) for r in reports}
                    == {id:count for id,_,count in prepared},
                    'SDK imported face inventory differs from prepared USD')
    final_code = 0 if code == 0 and len(prepared) == len(jobs) else 1
    stages.append((0,'complete_worker',time.perf_counter()-start,final_code))
    tsv(args.preparation_directory/'stages.tsv',('definition_id','stage','seconds','exit'),stages)
    return final_code


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('preparation-directory','output-directory','converter','periodic-preparer',
                 'plane-helper','producer'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--definition',nargs=3,action='append',required=True,
                        metavar=('ID','ORIGINAL_BREP','ORIGINAL_FACES'))
    parser.add_argument('--heal',type=int,choices=(0,1),required=True)
    parser.add_argument('--chord-height',type=float,required=True)
    parser.add_argument('--curve-angle',type=float,required=True)
    parser.add_argument('--surface-angle',type=float,required=True)
    try:
        sys.exit(main(parser.parse_args()))
    except Exception as error:
        print(f'prepared reference worker failed: {error}',file=sys.stderr)
        sys.exit(1)
