"""Bounded external original-source -> actual patch observation; no mesher code."""
import argparse
import csv
import hashlib
from pathlib import Path
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'tests'))
from native_adaptive_caller import make_bowl, read_patch, table


def run(args):
    args.destination.mkdir(parents=True, exist_ok=False)
    source = args.source or args.destination/'coupled-quadratic.bgsource'
    if args.source is None:
        make_bowl(source)
    identity = ((name, hashlib.sha256(path.read_bytes()).hexdigest())
                for name, path in (('caller', args.executable), ('cuda_module', args.cuda_module),
                                   ('source', source)))
    (args.destination/'identity.tsv').write_text(
        'artifact\tsha256\n'+''.join(f'{name}\t{digest}\n' for name, digest in identity))
    qualities = (1e-5,) if args.transfer_comparison or args.parallel_baseline else (1e-4, 1e-5)
    if args.parallel_baseline:
        assert args.reference_artifact is not None, 'parallel baseline requires checked reference'
        modes = (('cpu4', 4, None),)
    elif args.transfer_comparison:
        modes = (('cpu1', 1, None), ('p1000', 4, 0))
    else:
        modes = (('cpu1', 1, None), ('cpu4', 4, None), ('p1000', 4, 0), ('p1001', 4, 1))
    observations = []
    summary_path = args.destination/'observations.tsv'
    with summary_path.open('w', newline='') as stream:
        writer = csv.DictWriter(stream, delimiter='\t', fieldnames=(
            'epsilon_mm', 'backend', 'status', 'process_seconds', 'query_cold_seconds',
            'query_warm_seconds', 'output_cold_seconds', 'output_warm_seconds',
            'source_decode_seconds', 'source_compile_seconds', 'session_create_seconds',
            'device_open_seconds', 'close_seconds', 'through_close_seconds', 'points', 'facets',
            'aggregate_run_peak_reserved_bytes', 'session_host_peak_upper_bytes', 'device_peak_bytes', 'uploaded_bytes', 'downloaded_bytes',
            'warm_uploaded_bytes', 'warm_downloaded_bytes', 'exact_fallbacks_cold',
            'exact_fallbacks_warm', 'actual_facet_failures_cold', 'actual_facet_failures_warm',
            'device_points_cold', 'device_bounds_cold', 'device_restrictions_cold', 'sha256'))
        writer.writeheader()
        for quality in qualities:
            reference = args.reference_artifact.read_bytes() if args.reference_artifact else None
            for backend, threads, device in modes:
                label = f'{quality:g}-{backend}'
                output = args.destination/label
                command = [str(args.executable), '--source', str(source), '--carrier', '9',
                           '--output', str(output), '--threads', str(threads),
                           '--memory-mib', '2048', '--max-nodes', '262144', '--max-depth', '64',
                           '--positional-mm', str(quality), '--positional-mm', str(quality)]
                if device is not None:
                    command += ['--backend', 'p100', '--device', str(device),
                                '--cuda-module', str(args.cuda_module)]
                (args.destination/f'{label}.command').write_text('\n'.join(command)+'\n')
                started = time.perf_counter()
                row = {'epsilon_mm': quality, 'backend': backend}
                try:
                    completed = subprocess.run(command, capture_output=True, timeout=120, check=False)
                    row['status'] = 'complete' if completed.returncode == 0 else f'exit{completed.returncode}'
                    stdout, stderr = completed.stdout, completed.stderr
                except subprocess.TimeoutExpired as failure:
                    row['status'] = 'censored120s'
                    stdout, stderr = failure.stdout or b'', failure.stderr or b''
                row['process_seconds'] = time.perf_counter()-started
                (args.destination/f'{label}.stdout').write_bytes(stdout)
                (args.destination/f'{label}.stderr').write_bytes(stderr)
                if row['status'] == 'complete':
                    try:
                        cold = output/'query-0.bgapch'
                        raw = read_patch(cold, 'bowl', quality) if reference is None else cold.read_bytes()
                        if reference is None:
                            reference = raw
                        assert raw == reference, 'backend changed actual semantic bytes'
                        assert (output/'query-1.bgapch').read_bytes() == reference, 'warm changed actual semantic bytes'
                        query = table(output/'queries.tsv')
                        assert len(query) == 2
                        times = table(output/'timings.tsv')
                        def elapsed(scope):
                            return [entry['seconds'] for entry in times if entry['scope'] == scope]
                        row.update(query_cold_seconds=elapsed('query.construct_bound_packets')[0],
                                   query_warm_seconds=elapsed('query.construct_bound_packets')[1],
                                   output_cold_seconds=elapsed('output.patch')[0],
                                   output_warm_seconds=elapsed('output.patch')[1],
                                   source_decode_seconds=elapsed('source.decode')[0],
                                   source_compile_seconds=elapsed('source.compile')[0],
                                   session_create_seconds=elapsed('session.create')[0],
                                   device_open_seconds=elapsed('device.open')[0],
                                   close_seconds=elapsed('session.close')[0],
                                   through_close_seconds=elapsed('through_session_close')[0],
                                   points=query[0]['points'], facets=query[0]['facets'],
                                   aggregate_run_peak_reserved_bytes=query[-1].get('run_peak_including_device_reservations_bytes', ''),
                                   session_host_peak_upper_bytes=query[-1].get('session_host_peak_upper_bytes',
                                                                          query[-1].get('host_peak_bytes', '')),
                                   device_peak_bytes=query[-1]['device_peak_bytes'],
                                   uploaded_bytes=query[0]['uploaded_bytes'],
                                   downloaded_bytes=query[0]['downloaded_bytes'],
                                   warm_uploaded_bytes=int(query[1]['uploaded_bytes'])-int(query[0]['uploaded_bytes']),
                                   warm_downloaded_bytes=int(query[1]['downloaded_bytes'])-int(query[0]['downloaded_bytes']),
                                   exact_fallbacks_cold=query[0]['exact_fallbacks'],
                                   exact_fallbacks_warm=int(query[1]['exact_fallbacks'])-int(query[0]['exact_fallbacks']),
                                   actual_facet_failures_cold=query[0]['actual_facet_failures'],
                                   actual_facet_failures_warm=int(query[1]['actual_facet_failures'])-int(query[0]['actual_facet_failures']),
                                   device_points_cold=query[0]['device_points'],
                                   device_bounds_cold=query[0]['device_bounds'],
                                   device_restrictions_cold=query[0]['device_restrictions'],
                                   sha256=hashlib.sha256(reference).hexdigest())
                    except KeyError as failure:
                        row['status'] = f'metadata_failure:{failure}'
                    except (AssertionError, IndexError, ValueError) as failure:
                        row['status'] = f'oracle_failure:{failure}'
                writer.writerow(row)
                stream.flush()
                observations.append(row)
                print(row, flush=True)
    return 0 if all(row['status'] == 'complete' for row in observations) else 1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--cuda-module', type=Path, required=True)
    parser.add_argument('--destination', type=Path, required=True)
    parser.add_argument('--source', type=Path, help='reuse the identical authored source asset')
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--transfer-comparison', action='store_true',
                      help='only affected fine1e-5 CPU1/P1000 comparison; two processes')
    mode.add_argument('--parallel-baseline', action='store_true',
                      help='only affected fine1e-5 CPU4 baseline; one process')
    parser.add_argument('--reference-artifact', type=Path,
                        help='compare bytes to already independently checked actual artifact')
    args = parser.parse_args()
    return run(args)


if __name__ == '__main__':
    raise SystemExit(main())
