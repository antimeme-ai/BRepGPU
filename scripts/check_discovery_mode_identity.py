#!/usr/bin/env python3
"""Capture/compare actual discovery slabs, metadata and inputs without JSON.

Run capture separately for each actual plugin in a fresh process. Geometry
qualification remains scripts/qualify_discovery.py; this client checks execution
identity and the imported focused callers' simple analytic/profile contracts.
"""
import argparse
from array import array
from contextlib import closing
import csv
import ctypes
import hashlib
import io
import math
from pathlib import Path
import sys
import struct

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "python"))
sys.path.insert(0, str(ROOT / "tests"))
import brepgpu as bg
from brepgpu.discovery_fixtures import discovery_cases
from test_discovery_modes import (
    SLABS, require, corpus_observations, focused_observations, smoke_observations)

INDEX_FIELDS = ("key", "kind", "count", "profile", "part", "file", "offset", "size", "element_size")
SOURCE_FILES = ("python/brepgpu/__init__.py", "python/brepgpu/fixtures.py",
                "python/brepgpu/projection_fixtures.py", "python/brepgpu/discovery_fixtures.py",
                "tests/test_discovery_modes.py", "scripts/check_discovery_mode_identity.py")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_table(path, fields, rows):
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fields, delimiter="\t")
        writer.writeheader(); writer.writerows(rows)


def named_record(record, exclude=(), prefix=""):
    """Serialize declared numeric fields, never ctypes object padding."""
    rows = []
    for name, field_type in type(record)._fields_:
        if name in exclude:
            continue
        value = getattr(record, name)
        items = enumerate(value) if issubclass(field_type, ctypes.Array) else ((None, value),)
        for index, item in items:
            key = prefix+name+(f"[{index}]" if index is not None else "")
            # Preserve floating metadata bits, including a signed frame zero.
            encoded = "double_bits:"+struct.pack("=d", item).hex() if isinstance(item, float) else str(int(item))
            rows.append((key, encoded))
    stream = io.StringIO(newline="")
    csv.writer(stream, delimiter="\t").writerows(rows)
    return stream.getvalue().encode("utf-8")


def option_parts(batch):
    if isinstance(batch, bg.DiscoveryBatch):
        hierarchy = bg.HierarchyOptionsDesc()
        hierarchy.depth = batch.hierarchy_options.depth
        hierarchy.max_nodes = batch.hierarchy_options.max_nodes
        hierarchy.flags = batch.hierarchy_options.mode
        options = bg.DiscoveryOptionsDesc()
        options.max_visits = batch.options.max_visits
        options.max_refinements = batch.options.max_refinements
        options.tolerance_squared = batch.options.tolerance_squared
        return (("hierarchy_options", named_record(hierarchy), 1), ("options", named_record(options), 1))
    if isinstance(batch, bg.ProjectionBatch):
        options = bg.ProjectionOptionsDesc()
        options.max_iterations = batch.options.max_iterations
        options.max_backtracks = batch.options.max_backtracks
        options.gradient_tolerance = batch.options.gradient_tolerance
        return (("options", named_record(options), 1),)
    return ()


def parts(observation):
    batch = observation.batch
    yield "data", batch.data.tobytes(), 8
    names = SLABS if observation.kind == "discovery" else \
        ("statuses", "flags", "iterations", "evaluation_counts") if observation.kind == "projection" else ("statuses",)
    for name in names:
        yield name, getattr(batch, name).tobytes(), 4
    # Named deterministic metadata; no uninitialized/padding bytes enter identity.
    yield "info_without_duration", named_record(batch.info, exclude=("kernel_ms",)), 1
    yield "descriptors", b"".join(named_record(descriptor, prefix=f"{i}.")
                                  for i, descriptor in enumerate(batch._descriptors)), 1
    yield "slots", batch._slots.tobytes(), 4
    yield "targets_or_uv", (batch._uv if observation.kind == "evaluator" else batch.targets).tobytes(), 8
    if observation.kind == "projection":
        yield "seeds", batch.seeds.tobytes(), 8
    yield from option_parts(batch)
    yield "input_controls", array("d", observation.controls).tobytes(), 8
    yield "input_slots", array("I", observation.slots).tobytes(), 4
    yield "input_targets_or_uv", array("d", observation.targets).tobytes(), 8
    yield "input_auxiliary", array("d", observation.auxiliary).tobytes(), 8
    yield "labels", "\n".join(observation.labels).encode("utf-8"), 1


def capture(args):
    require(sys.byteorder == "little" and array("d").itemsize == 8 and array("I").itemsize == 4,
            "capture requires the qualified native widths/byte order")
    binaries = {name: getattr(args, name).resolve(strict=True) for name in ("library", "plugin", "benchmark")}
    before = {name: digest(path) for name, path in binaries.items()}
    args.output.mkdir(parents=True, exist_ok=False)
    arguments = dict(backend="cuda", device=args.device,
                     library_path=str(binaries["library"]), plugin_path=str(binaries["plugin"]))
    index, durations, keys = [], [], set()
    corpus_rows = 0
    for factory in (corpus_observations, focused_observations, smoke_observations):
        with closing(factory(arguments)) as observations:
            for observation in observations:
                require(observation.key not in keys, "duplicate capture case key")
                keys.add(observation.key)
                filename = f"payload-{len(keys):06d}.bin"
                offset = 0
                with (args.output / filename).open("xb") as stream:
                    for name, payload, element_size in parts(observation):
                        stream.write(payload)
                        index.append(dict(key=observation.key, kind=observation.kind,
                                          count=observation.batch.count, profile=int(observation.profile),
                                          part=name, file=filename, offset=offset, size=len(payload),
                                          element_size=element_size))
                        offset += len(payload)
                info = observation.batch.info
                durations.append(dict(key=observation.key, count=observation.batch.count,
                                      profile=int(observation.profile), flags=info.flags,
                                      kernel_ms=repr(info.kernel_ms)))
                if observation.key.startswith("corpus/"):
                    corpus_rows += observation.batch.count
    require(corpus_rows == 3*len(discovery_cases()) and corpus_rows > 0,
            "incomplete existing-corpus capture")
    require(all(digest(path) == before[name] for name, path in binaries.items()),
            "selected binary changed during capture")
    metadata = dict(format="brepgpu_mode_identity_1", device=str(args.device), byteorder=sys.byteorder,
                    double_size="8", integer_size="4", corpus_rows=str(corpus_rows),
                    record_count=str(len(keys)), label=args.label,
                    core_sha256=before["library"], plugin_sha256=before["plugin"],
                    benchmark_sha256=before["benchmark"])
    for path in SOURCE_FILES:
        metadata["source:"+path] = digest(ROOT / path)
    write_table(args.output / "index.tsv", INDEX_FIELDS, index)
    write_table(args.output / "durations.tsv", ("key", "count", "profile", "flags", "kernel_ms"), durations)
    # No private paths are emitted; actual plugin identity deliberately differs.
    write_table(args.output / "metadata.tsv", ("key", "value"),
                (dict(key=key, value=value) for key, value in metadata.items()))
    print(f"Captured {corpus_rows} corpus rows and {len(keys)} total batches; plugin {before['plugin']}")


def table(path, fields):
    with path.open(newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        require(tuple(reader.fieldnames or ()) == tuple(fields), "unexpected table schema: "+path.name)
        rows = list(reader)
        require(all(None not in row and None not in row.values() for row in rows), "malformed table row")
        return rows


def load(directory):
    metadata = {}
    for row in table(directory / "metadata.tsv", ("key", "value")):
        require(row["key"] not in metadata, "duplicate metadata key")
        metadata[row["key"]] = row["value"]
    require(metadata.get("format") == "brepgpu_mode_identity_1"
            and metadata.get("byteorder") == "little"
            and metadata.get("double_size") == "8" and metadata.get("integer_size") == "4",
            "wrong capture format/native environment")
    rows = table(directory / "index.tsv", INDEX_FIELDS)
    require(rows, "empty identity capture")
    seen, positions, payloads, cases = set(), {}, {}, {}
    for row in rows:
        key = (row["key"], row["part"])
        require(key not in seen, "duplicate indexed payload")
        seen.add(key)
        filename = row["file"]
        require(Path(filename).name == filename and filename.startswith("payload-")
                and filename.endswith(".bin"), "invalid payload filename")
        offset, size, element = (int(row[name]) for name in ("offset", "size", "element_size"))
        require(offset == positions.get(filename, 0) and size >= 0 and element in (1, 4, 8)
                and size % element == 0, "invalid/overlapping payload extent")
        if filename not in payloads:
            payloads[filename] = (directory / filename).read_bytes()
        data = payloads[filename]
        require(offset+size <= len(data), "short payload file")
        positions[filename] = offset+size
        require(int(row["count"]) >= 0 and row["profile"] in ("0", "1"), "invalid count/profile key")
        description = (row["kind"], row["count"], row["profile"])
        require(cases.setdefault(row["key"], description) == description, "inconsistent case metadata")
    require(all(positions[name] == len(data) for name, data in payloads.items()), "unindexed trailing payload")
    require(int(metadata["record_count"]) == len(cases), "record count differs")
    corpus_rows = sum(int(count) for key, (_, count, _) in cases.items() if key.startswith("corpus/"))
    require(corpus_rows == int(metadata["corpus_rows"]) and corpus_rows > 0, "corpus count differs")
    duration_keys = set()
    for row in table(directory / "durations.tsv", ("key", "count", "profile", "flags", "kernel_ms")):
        require(row["key"] not in duration_keys and row["key"] in cases, "wrong/duplicate duration case")
        duration_keys.add(row["key"])
        require(cases[row["key"]][1:] == (row["count"], row["profile"]), "duration case conditions differ")
        measured = row["profile"] == "1" and int(row["count"]) > 0
        duration = float(row["kernel_ms"])
        require(int(row["flags"]) == int(measured), "incorrect stored profile flags")
        require((math.isfinite(duration) and duration >= 0) if measured else math.isnan(duration),
                "incorrect stored profile duration")
    require(duration_keys == set(cases), "missing duration/profile evidence")
    return metadata, rows, payloads


def compare(args):
    old_meta, old_index, old_files = load(args.baseline)
    new_meta, new_index, new_files = load(args.candidate)
    # Plugin and human variant label are provenance, not equality conditions.
    ignored = {"plugin_sha256", "label"}
    require({k: v for k, v in old_meta.items() if k not in ignored}
            == {k: v for k, v in new_meta.items() if k not in ignored},
            "core/benchmark/source/device/input-set metadata differ")
    require(old_index == new_index, "case/part/shape/condition index differs")
    for row in old_index:
        offset, size = int(row["offset"]), int(row["size"])
        old = old_files[row["file"]][offset:offset+size]
        new = new_files[row["file"]][offset:offset+size]
        if old != new:
            byte = next(i for i, (a, b) in enumerate(zip(old, new)) if a != b)
            element = byte // int(row["element_size"])
            where = f"element {element}"
            if row["part"] == "data" and int(row["count"]):
                where = f"field {element//int(row['count'])}, row {element%int(row['count'])}"
            elif row["part"] in SLABS:
                where = f"row {element}"
            raise AssertionError(f"raw identity differs: {row['key']}, {row['part']}, {where}, byte {byte}")
    print(f"PASS: {old_meta['corpus_rows']} corpus rows; {old_meta['record_count']} batches; raw buffers and deterministic metadata identical")
    print(f"Baseline plugin {old_meta['plugin_sha256']}; candidate plugin {new_meta['plugin_sha256']}")


def main():
    parser = argparse.ArgumentParser(__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    collect = commands.add_parser("capture")
    for name in ("library", "plugin", "benchmark"):
        collect.add_argument("--"+name, type=Path, required=True)
    collect.add_argument("--device", type=int, choices=(0, 1), required=True)
    collect.add_argument("--label", choices=("dynamic", "specialized"), required=True)
    collect.add_argument("--output", type=Path, required=True)
    check = commands.add_parser("compare")
    check.add_argument("--baseline", type=Path, required=True)
    check.add_argument("--candidate", type=Path, required=True)
    args = parser.parse_args()
    try:
        (capture if args.command == "capture" else compare)(args)
    except Exception as error:
        print(f"Identity {args.command} FAILED: {type(error).__name__}: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
