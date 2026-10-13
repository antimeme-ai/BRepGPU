#!/usr/bin/env python3
"""Run public native-ABI cases; preserve samples and summarize their medians."""
import argparse
import csv
import io
from pathlib import Path
import statistics
import subprocess


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output-directory", type=Path, required=True)
    parser.add_argument("--repetitions", type=int, default=7)
    parser.add_argument("--counts", default="1,32,256,4096,65536,1048576")
    parser.add_argument("--cpu-threads", default="1,24")
    parser.add_argument("--cuda-devices", default="0,1")
    parser.add_argument("--orders", default="0,1,2")
    parser.add_argument("--patches", default="1,16")
    args = parser.parse_args()
    def integers(text):
        return [int(value) for value in text.split(",") if value]
    targets = [("cpu", 0, n) for n in integers(args.cpu_threads)]
    targets += [("cuda", device, 1) for device in integers(args.cuda_devices)]
    rows = []
    args.output_directory.mkdir(parents=True, exist_ok=True)
    for backend, device, threads in targets:
        for count in integers(args.counts):
            for order in integers(args.orders):
                for patches in integers(args.patches):
                    print(f"{backend} device={device} threads={threads} N={count} order={order} patches={patches}", flush=True)
                    result = subprocess.run([str(args.binary.resolve()), backend, str(device), str(threads), str(count),
                                             str(order), str(patches), str(args.repetitions)],
                                            text=True, capture_output=True, check=True)
                    batch = list(csv.DictReader(io.StringIO(result.stdout), delimiter="\t"))
                    if not batch:
                        raise RuntimeError("native benchmark produced no rows")
                    rows.extend(batch)
                    # Incremental artifact retains successful cases if a later
                    # case fails. The final summary exists only on full success.
                    write(args.output_directory / "samples.tsv", rows, list(rows[0]))
    groups = {}
    keys = ["backend", "device", "threads", "count", "order", "patches", "phase"]
    for row in rows:
        groups.setdefault(tuple(row[key] for key in keys), []).append(row)
    summary = []
    for key, batch in groups.items():
        timing = [float(row["ms"]) for row in batch]
        summary.append(dict(zip(keys, key), samples=len(batch), median_ms=statistics.median(timing),
                            min_ms=min(timing), max_ms=max(timing),
                            max_abs_error=max(float(row["max_abs_error"]) for row in batch)))
    write(args.output_directory / "summary.tsv", summary, list(summary[0]))


def write(path, rows, keys):
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, keys, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


if __name__ == "__main__":
    main()
