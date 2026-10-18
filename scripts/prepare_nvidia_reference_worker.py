"""Explicit reference preparation followed by one genuine SDK mesh batch.

Every original conversion is attempted. Geometrically admitted periodic
preparation runs only after a refusal. Plane-range preparation precedes all
SDK healing/tessellation; all failed work remains in the measured operation.
"""
import argparse
import csv
import hashlib
from pathlib import Path
import os
import resource
import select
import subprocess
import sys
import time



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


def cpu_seconds():
    """CPU of this process plus its reaped children; stage deltas are stage CPU."""
    own, children = resource.getrusage(resource.RUSAGE_SELF), resource.getrusage(resource.RUSAGE_CHILDREN)
    return own.ru_utime+own.ru_stime+children.ru_utime+children.ru_stime


# changed_input: 1 when the stage produced input bytes that differ from its own
# input (a real repair), 0 for a byte-identical passthrough, empty otherwise.
# conversion_seconds / process_start_seconds / process_exit_seconds, for the
# official converter only: the span of ConvertOcctFileToUsd (NVIDIA's work, as
# an in-process user runs it) and this converter process's start (launch to
# main) and exit (main end to reap), which are ours. Empty for other stages.
STAGE_COLUMNS = ('definition_id', 'stage', 'seconds', 'exit', 'cpu_seconds', 'changed_input',
                 'conversion_seconds', 'process_start_seconds', 'process_exit_seconds')
CONVERTER_STAGES = ('convert_original', 'convert_periodic_copy')


def converter_times(log, launch, reaped):
    """(conversion, start, exit) seconds from the converter's timing lines, or blanks."""
    values = {}
    for line in log.read_text(errors='replace').splitlines():
        key, _, value = line.partition('=')
        if key in ('main_start_monotonic', 'conversion_seconds', 'main_end_monotonic'):
            values[key] = float(value)
    if 'main_start_monotonic' not in values or 'main_end_monotonic' not in values:
        return '', '', ''
    return (values.get('conversion_seconds', ''), values['main_start_monotonic']-launch,
            reaped-values['main_end_monotonic'])


class ConverterServiceFailure(RuntimeError):
    """Our converter service failed twice on one job, or was handed a job it cannot frame: ours, never NVIDIA's.

    failed_seconds is the time the failed requests took."""

    def __init__(self, message, failed_seconds=0.):
        super().__init__(message)
        self.failed_seconds = failed_seconds


# Exit status of a preparation worker whose converter service failed: ours.
CONVERTER_SERVICE_FAILED = 5


class ConverterService:
    """One long-lived official converter per preparation worker (convert --serve FD).

    An NVIDIA user converts in-process: one context and plugin setup, and the
    first conversion's one-time cost, once per worker. Replies arrive on a
    dedicated pipe, never on stdout, which OCCT and USD may write to. A job's
    conversion span is NVIDIA's; the request round trip beyond it, and each
    server's start, setup and exit, are ours. Time the server spends waiting
    while our own preparation runs is listed as idle, not charged. A server
    that dies, breaks its pipe or times out is replaced and the job retried
    once; a second failure raises ConverterServiceFailure. Each launch is
    recorded in converter-process.tsv."""

    COLUMNS = ('launch_monotonic', 'main_start_monotonic', 'ready_monotonic', 'close_monotonic',
               'main_end_monotonic', 'reaped_monotonic', 'jobs', 'job_round_trip_seconds', 'failed_request_seconds',
               'ended')

    def __init__(self, executable, directory, timeout=120):
        self.executable, self.directory, self.timeout = executable, directory, timeout
        self.process, self.replies, self.buffer, self.launches = None, None, b'', []

    def _readline(self, deadline):
        while b'\n' not in self.buffer:
            remaining = deadline-time.monotonic()
            if remaining <= 0 or not select.select([self.replies], [], [], remaining)[0]:
                raise TimeoutError('converter did not answer in time')
            chunk = os.read(self.replies, 65536)
            if not chunk:
                raise EOFError('converter exited')
            self.buffer += chunk
        line, _, self.buffer = self.buffer.partition(b'\n')
        return line.decode(errors='replace')

    def _record(self):
        tsv(self.directory/'converter-process.tsv', self.COLUMNS,
            ([format(v, '.17g') if isinstance(v, float) else v for v in (r[c] for c in self.COLUMNS)]
             for r in self.launches))

    def _start(self):
        launch = time.monotonic()
        read, write = os.pipe()
        log = (self.directory/f'converter-{len(self.launches)}.log').open('wb')
        try:
            self.process = subprocess.Popen((str(self.executable), '--serve', str(write)), stdin=subprocess.PIPE,
                                            stdout=log, stderr=subprocess.STDOUT, pass_fds=(write,))
        finally:
            os.close(write)
            log.close()
        self.replies, self.buffer = read, b''
        self.current = dict(launch_monotonic=launch, main_start_monotonic='', ready_monotonic='',
                            close_monotonic='', main_end_monotonic='', reaped_monotonic='', jobs=0,
                            job_round_trip_seconds=0., failed_request_seconds=0., ended='')
        self.launches.append(self.current)
        deadline = time.monotonic()+self.timeout
        while True:
            line = self._readline(deadline)
            if line.startswith('main_start_monotonic='):
                self.current['main_start_monotonic'] = float(line.partition('=')[2])
            if line == 'ready':
                self.current['ready_monotonic'] = time.monotonic()
                return

    def _stop(self, kill=False, ended='closed'):
        if self.process is None:
            return
        self.current['close_monotonic'] = time.monotonic()
        try:
            if kill:
                self.process.kill()
            else:
                self.process.stdin.close()
                deadline = time.monotonic()+self.timeout
                while True:
                    line = self._readline(deadline)
                    if line.startswith('main_end_monotonic='):
                        self.current['main_end_monotonic'] = float(line.partition('=')[2])
        except (EOFError, TimeoutError, OSError, ValueError):
            pass
        if self.process.poll() is None:
            self.process.kill()
        self.process.wait()
        try:
            self.process.stdin.close()
        except (OSError, ValueError):
            pass
        os.close(self.replies)
        self.current['reaped_monotonic'], self.current['ended'] = time.monotonic(), ended
        self.process = self.replies = None
        self._record()

    def _request(self, source, target):
        """One request; raises on a dead, broken or silent server."""
        if self.process is None:
            self._start()
        self.process.stdin.write(f'{source}\t{target}\n'.encode())
        self.process.stdin.flush()
        deadline = time.monotonic()+self.timeout
        while True:
            line = self._readline(deadline)
            if line.startswith('job\t'):
                break
        _, status, span, message = (line.split('\t', 3)+['', '', ''])[:4]
        return (0 if status == '0' else 1), (float(span) if span else ''), message

    def convert(self, source, target):
        """(exit 0 converted | 1 NVIDIA's refusal, conversion span or '', round trip, message, failed-request seconds).

        A failed request (dead server, broken pipe, timeout) is ours: its time
        is returned for one listed scope of ours, the server is replaced and
        the job retried once; a second failure raises ConverterServiceFailure."""
        for path in (str(source), str(target)):
            if '\t' in path or '\n' in path or '\r' in path:
                raise ConverterServiceFailure(f'job path cannot be framed: {path!r}')
        failed = 0.
        for attempt in (1, 2):
            began = time.monotonic()
            try:
                code, span, message = self._request(source, target)
            except (EOFError, TimeoutError, OSError, ValueError) as error:
                elapsed = time.monotonic()-began
                failed += elapsed
                if self.process is not None:
                    self.current['failed_request_seconds'] += elapsed
                self._stop(kill=True, ended=f'failed: {type(error).__name__}')
                if attempt == 2:
                    raise ConverterServiceFailure(f'converter service failed twice on {source}: {error}', failed)
                continue
            round_trip = time.monotonic()-began
            self.current['jobs'] += 1
            self.current['job_round_trip_seconds'] += round_trip
            return code, span, round_trip, message, failed

    def close(self):
        self._stop()


def tsv(path, header, rows):
    with path.open('w', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t')
        writer.writerow(header)
        writer.writerows(rows)


def main(args):
    # Imported here: it needs the package's usdpy bindings, which timing helpers
    # such as converter_times do not.
    from prepare_nvidia_plane_envelope import prepare
    jobs = [(int(id), Path(path).resolve(), int(count)) for id,path,count in args.definition]
    require(jobs and len({j[0] for j in jobs}) == len(jobs), 'empty/duplicate definition IDs')
    require(all(0 <= id < 2**64-1 and count > 0 for id,_,count in jobs), 'invalid source inventory')
    require(not args.preparation_directory.exists() and not args.output_directory.exists(),
            'preparation and mesh directories must be new')
    args.preparation_directory.mkdir(parents=True)
    start, start_cpu = time.perf_counter(), cpu_seconds()
    stages, outcomes, prepared = [], [], []
    converter = ConverterService(args.converter, args.preparation_directory)
    service_failed = False

    def convert(source, target, log, definition, stage):
        try:
            code, span, round_trip, message, failed = converter.convert(source, target)
        except ConverterServiceFailure as error:
            # The failed request time, once, in one scope of ours; then our failure propagates.
            stages.append((definition, 'converter_service_failure', error.failed_seconds, -1, '', '', '', '', ''))
            tsv(args.preparation_directory/'stages.tsv',STAGE_COLUMNS,stages)
            raise
        log.write_text(message+'\n')
        if failed:
            # The failed request time, once, in one scope of ours.
            stages.append((definition, 'converter_service_failure', failed, -1, '', '', '', '', ''))
        # CPU of the live server is not in this process's rusage; the span stands in.
        stages.append((definition, stage, round_trip, code, '' if span == '' else span, '',
                       span, '', ''))
        tsv(args.preparation_directory/'stages.tsv',STAGE_COLUMNS,stages)
        return code

    def run(command, log, definition, stage, timeout=120):
        began, cpu, launch = time.perf_counter(), cpu_seconds(), time.monotonic()
        code = -1
        try:
            with log.open('wb') as stream:
                result = subprocess.run(tuple(map(str,command)), stdout=stream,
                                        stderr=subprocess.STDOUT, timeout=timeout)
            code = result.returncode
            return code
        finally:
            reaped = time.monotonic()
            times = converter_times(log, launch, reaped) if stage in CONVERTER_STAGES else ('', '', '')
            stages.append((definition, stage, time.perf_counter()-began, code, cpu_seconds()-cpu,
                           1 if stage == 'prepare_periodic_copy' and code == 0 else '', *times))
            tsv(args.preparation_directory/'stages.tsv',STAGE_COLUMNS,stages)

    for id, original, original_faces in jobs:
        directory = args.preparation_directory/f'definition-{id}'
        directory.mkdir()
        stage, periodic = 'convert_original', False
        try:
            original_hash = digest(original)
            usd = directory/'original.usdc'
            code = convert(original, usd, directory/'original-conversion.log', id, stage)
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
                require(convert(derivative/'prepared.brep', usd, directory/'prepared-conversion.log', id, stage) == 0,
                        'official conversion still refuses prepared periodic representation')
            stage = 'prepare_plane_envelopes'
            envelope = directory/'plane'
            envelope.mkdir()
            began, cpu = time.perf_counter(), cpu_seconds()
            plane_exit = 1
            try:
                corrected = prepare(usd,envelope/'source',args.plane_helper)
                plane_exit = 0
            finally:
                stages.append((id,stage,time.perf_counter()-began,plane_exit,cpu_seconds()-cpu,'','','',''))
            policy = fields(envelope/'preparation.tsv')
            changed = int(policy['input_sha256'] != policy['output_sha256'])
            stages[-1] = (*stages[-1][:5], changed, *stages[-1][6:])
            require(int(policy['source_faces']) == expected_faces,
                    'converted USD did not retain every original/prepared face')
            require(digest(original) == original_hash, 'original BRep bytes changed')
            prepared.append((id,corrected,expected_faces))
            outcomes.append((id,'PASS',original_faces,expected_faces,int(periodic),
                             original_hash,digest(corrected),''))
        except ConverterServiceFailure as error:
            # Ours: no NVIDIA refusal is inferred and no repair is attempted.
            outcomes.append((id,'FAIL',original_faces,0,int(periodic),'','','ours: '+str(error)))
            print(f'definition {id} {stage}: ours: {error}',file=sys.stderr,flush=True)
            service_failed = True
        except Exception as error:
            outcomes.append((id,'FAIL',original_faces,0,int(periodic),'','',stage+': '+str(error)))
            print(f'definition {id} {stage}: {error}',file=sys.stderr,flush=True)
        tsv(args.preparation_directory/'definitions.tsv',
            ('definition_id','status','original_faces','prepared_faces','periodic_preparation',
             'original_brep_sha256','prepared_usd_sha256','message'),outcomes)
        tsv(args.preparation_directory/'stages.tsv',STAGE_COLUMNS,stages)
        if service_failed:
            break

    converter.close()
    if service_failed:
        stages.append((0,'complete_worker',time.perf_counter()-start,CONVERTER_SERVICE_FAILED,
                       cpu_seconds()-start_cpu,'','','',''))
        tsv(args.preparation_directory/'stages.tsv',STAGE_COLUMNS,stages)
        return CONVERTER_SERVICE_FAILED
    code = 1
    if prepared:
        command = [args.producer,'--heal',str(args.heal),'--chord-height',str(args.chord_height),
                   '--curve-angle',str(args.curve_angle),'--surface-angle',str(args.surface_angle),
                   '--output-directory',args.output_directory]
        for id,usd,_ in prepared:
            command.extend(('--usd-definition',str(id),usd))
        launch = time.monotonic()
        code = run(command,args.preparation_directory/'native-producer.log',0,
                   'genuine_sdk_prepared_batch',timeout=900)
        # Launch and reap on CLOCK_MONOTONIC, the producer's own clock, so the
        # caller can split the producer process into start, main and exit.
        tsv(args.preparation_directory/'producer-process.tsv',('launch_monotonic','reaped_monotonic'),
            ((format(launch,'.17g'),format(time.monotonic(),'.17g')),))
        if not code:
            with (args.output_directory/'definitions.tsv').open() as stream:
                reports = list(csv.DictReader(stream,delimiter='\t'))
            require({int(r['definition_id']):int(r['input_faces']) for r in reports}
                    == {id:count for id,_,count in prepared},
                    'SDK imported face inventory differs from prepared USD')
    final_code = 0 if code == 0 and len(prepared) == len(jobs) else 1
    stages.append((0,'complete_worker',time.perf_counter()-start,final_code,cpu_seconds()-start_cpu,'','','',''))
    tsv(args.preparation_directory/'stages.tsv',STAGE_COLUMNS,stages)
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
