#!/usr/bin/env python3
"""Run qualified native discovery comparisons; retain raw rows and work histograms."""
import argparse
import csv
import io
from pathlib import Path
import statistics
import subprocess


def write(path, rows):
    if not rows:
        return
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, list(rows[0]), delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output-directory", type=Path, required=True)
    parser.add_argument("--counts", default="1,256,4096,65536")
    parser.add_argument("--corpora", default="plane,diagonal,cylinder,axis,parabolic,twins,capped")
    parser.add_argument("--modes", default="0,1,2")
    parser.add_argument("--depths", default="8")
    parser.add_argument("--cpu-threads", default="1,24")
    parser.add_argument("--cuda-devices", default="0,1")
    parser.add_argument("--repetitions", type=int, default=7)
    parser.add_argument("--max-refinements", type=int, default=8)
    args = parser.parse_args()

    def integers(value):
        return [int(x) for x in value.split(",") if x]

    targets = [("cpu", 0, x) for x in integers(args.cpu_threads)]
    targets += [("cuda", x, 1) for x in integers(args.cuda_devices)]
    args.output_directory.mkdir(parents=True, exist_ok=False)
    (args.output_directory / "work").mkdir()
    rows = []
    binary = str(args.binary.resolve())
    for backend, device, threads in targets:
        for n in integers(args.counts):
            for corpus in args.corpora.split(","):
                for mode in integers(args.modes):
                    for depth in integers(args.depths):
                        print(f"{backend} device={device} threads={threads} N={n} corpus={corpus} mode={mode} depth={depth}", flush=True)
                        work = args.output_directory / "work" / f"{backend}-{device}-{threads}-{n}-{corpus}-{mode}-{depth}.tsv"
                        command = [binary, backend, str(device), str(threads), str(n), corpus,
                                   str(mode), str(depth), str(args.repetitions), str(args.max_refinements), str(work.resolve())]
                        result = subprocess.run(command, text=True, capture_output=True)
                        if result.returncode:
                            # Preserve the failed case's emitted rows; qualified
                            # performance stops at the first failed operation.
                            (args.output_directory / "failed-case.tsv").write_text(result.stdout)
                            raise RuntimeError(f"Native case failed ({backend}, {corpus}, mode {mode}, depth {depth}): {result.stderr}")
                        batch = list(csv.DictReader(io.StringIO(result.stdout), delimiter="\t"))
                        if not batch:
                            raise RuntimeError("native benchmark emitted no samples")
                        rows.extend(batch)
                        write(args.output_directory / "samples.tsv", rows)
    keys = ["backend", "device", "threads", "count", "corpus", "mode", "depth",
            "max_visits", "max_refinements", "tolerance_squared", "phase"]
    groups = {}
    for row in rows:
        groups.setdefault(tuple(row[k] for k in keys), []).append(row)
    summary = []
    for key, batch in groups.items():
        times = [float(row["ms"]) for row in batch]
        item = dict(zip(keys, key), samples=len(times), median_ms=statistics.median(times),
                    min_ms=min(times), max_ms=max(times))
        for column in rows[0]:
            if column not in keys + ["sample", "ms"]:
                values = [float(row[column]) for row in batch]
                item[column + "_min"] = min(values)
                item[column + "_max"] = max(values)
        summary.append(item)
    write(args.output_directory / "summary.tsv", summary)


if __name__ == "__main__":
    main()
