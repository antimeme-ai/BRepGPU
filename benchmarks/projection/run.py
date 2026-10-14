#!/usr/bin/env python3
"""Run qualified native projection cases and retain samples/median timings."""
import argparse
import csv
import io
from pathlib import Path
import statistics
import subprocess


def write(path, rows):
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, list(rows[0]), delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output-directory", type=Path, required=True)
    parser.add_argument("--counts", default="1,32,256,4096,65536,1048576")
    parser.add_argument("--corpora", default="plane,tilted,cylinder,parabolic,mixed,adverse")
    parser.add_argument("--cpu-threads", default="1,24")
    parser.add_argument("--cuda-devices", default="0,1")
    parser.add_argument("--repetitions", type=int, default=7)
    args = parser.parse_args()
    def integers(text):
        return [int(x) for x in text.split(",") if x]
    targets = [("cpu", 0, n) for n in integers(args.cpu_threads)]
    targets += [("cuda", device, 1) for device in integers(args.cuda_devices)]
    rows = []
    args.output_directory.mkdir(parents=True, exist_ok=True)
    (args.output_directory / "work").mkdir(exist_ok=True)
    for backend, device, threads in targets:
        for count in integers(args.counts):
            for corpus in args.corpora.split(","):
                print(f"{backend} device={device} threads={threads} N={count} corpus={corpus}", flush=True)
                work_path = args.output_directory / "work" / f"{backend}-{device}-{threads}-{count}-{corpus}.tsv"
                result = subprocess.run([str(args.binary.resolve()), backend, str(device), str(threads), str(count), corpus, str(args.repetitions), str(work_path.resolve())], text=True, capture_output=True, check=True)
                batch = list(csv.DictReader(io.StringIO(result.stdout), delimiter="\t"))
                if not batch:
                    raise RuntimeError("native benchmark emitted no samples")
                rows.extend(batch)
                write(args.output_directory / "samples.tsv", rows)
    keys = ["backend", "device", "threads", "count", "corpus", "max_iterations", "max_backtracks", "tolerance", "phase"]
    groups = {}
    for row in rows:
        groups.setdefault(tuple(row[key] for key in keys), []).append(row)
    summaries = []
    for key, batch in groups.items():
        times = [float(row["ms"]) for row in batch]
        # Keep visible ranges of geometry/counter/status results alongside time.
        summary = dict(zip(keys, key), samples=len(batch), median_ms=statistics.median(times), min_ms=min(times), max_ms=max(times))
        for column in rows[0]:
            if column not in keys + ["sample", "ms"]:
                values = [float(row[column]) for row in batch]
                summary[column + "_min"] = min(values)
                summary[column + "_max"] = max(values)
        summaries.append(summary)
    write(args.output_directory / "summary.tsv", summaries)


if __name__ == "__main__":
    main()
