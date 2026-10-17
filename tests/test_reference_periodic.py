#!/usr/bin/env python3
"""Actual OCCT periodic preparation; independent exact nominal-period history reader."""
import argparse
import csv
from fractions import Fraction
from pathlib import Path
import subprocess
import sys


def rows(path):
    with path.open(newline="") as stream:
        return list(csv.DictReader(stream, delimiter="\t"))


def rational(text):
    return Fraction.from_float(float(text))


def image(a, b, period, pieces):
    width = b - a
    assert width > 0 and (not period or width <= period)
    atoms = []
    for left, right in pieces:
        length = right - left
        assert length > 0
        if not period:
            atoms.append((left - a, right - a))
            continue
        assert length <= period
        start = (left - a) % period
        end = start + length
        if end <= period:
            atoms.append((start, end))
        else:
            atoms.extend(((start, period), (Fraction(0), end - period)))
    cursor = Fraction(0)
    for left, right in sorted(atoms):
        assert left == cursor and right <= width, (a, b, period, atoms)
        cursor = right
    assert cursor == width


def check(directory):
    status = {row["key"]: row["value"] for row in rows(directory / "status.tsv")}
    assert status["status"] == "PASS"
    assert status["original_face_coverage"] == "COMPLETE"
    assert status["all_prepared_faces_have_ancestry"] == "1"
    history = rows(directory / "source-history.tsv")
    faces = [row for row in history if row["role"] == "face"]
    assert len({row["source_index"] for row in faces}) == int(status["original_faces"])
    assert len({row["prepared_index"] for row in faces}) == int(status["prepared_faces"])
    groups = {}
    for row in history:
        if row["role"] == "edge":
            groups.setdefault(row["source_index"], []).append(row)
    for group in groups.values():
        first = group[0]
        a, b, period = map(rational, (first["source_first"], first["source_last"], first["period"]))
        if b > a:
            image(a, b, period, [(rational(row["first"]), rational(row["last"])) for row in group])
        word = sorted(group, key=lambda row: int(row["order"]))
        assert [int(row["order"]) for row in word] == list(range(len(word)))
        cursor = first["source_first_vertex"]
        for row in word:
            assert row["first_vertex"] == cursor
            assert int(row["direction"]) == 1
            cursor = row["last_vertex"]
        assert cursor == first["source_last_vertex"]
    for row in rows(directory / "tolerances.tsv"):
        if not row["role"].startswith("generated_"):
            assert rational(row["original_mm"]) == rational(row["prepared_mm"])
    assert (directory / "prepared.brep").stat().st_size > 0
    return int(status["prepared_faces"])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--tool", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--sdk", type=Path)
    parser.add_argument("--converter", type=Path)
    parser.add_argument("--plane-helper", type=Path)
    parser.add_argument("--allow-export-refusal", action="store_true", help="report successful SDK import/tessellation separately from existing unsupported raw polygon export")
    args = parser.parse_args()
    args.output.mkdir()
    source = args.output / "source"
    subprocess.run([str(args.fixture), str(source)], check=True)
    for number, original in enumerate(sorted(source.glob("*.brep")), 1):
        prepared = args.output / original.stem
        process = subprocess.run([str(args.tool), str(original), str(prepared)], capture_output=True, text=True)
        (args.output / (original.stem + ".log")).write_text(process.stdout + process.stderr)
        if original.stem.startswith("invalid-"):
            assert process.returncode != 0 and not (prepared / "prepared.brep").exists()
            continue
        assert process.returncode == 0, process.stdout + process.stderr
        count = check(prepared)
        if args.sdk:
            sdk_output = args.output / (original.stem + "-sdk")
            source_args = ["--definition", str(number), str(prepared / "prepared.brep")]
            if args.converter:
                assert args.plane_helper
                usd = prepared / "prepared.usdc"
                converted = subprocess.run([str(args.converter), str(prepared / "prepared.brep"), str(usd)], capture_output=True, text=True, timeout=180)
                (prepared / "conversion.log").write_text(converted.stdout + converted.stderr)
                assert converted.returncode == 0, converted.stdout + converted.stderr
                envelope = prepared / "plane-envelope"
                enclosed = subprocess.run([sys.executable, str(Path(__file__).resolve().parents[1] / "scripts/prepare_nvidia_plane_envelope.py"), "--input-usd", str(usd), "--output-directory", str(envelope), "--helper", str(args.plane_helper)], capture_output=True, text=True, timeout=180)
                (prepared / "plane-envelope.log").write_text(enclosed.stdout + enclosed.stderr)
                assert enclosed.returncode == 0, enclosed.stdout + enclosed.stderr
                source_args = ["--usd-definition", str(number), str(envelope / "corrected.usdc")]
            process = subprocess.run([str(args.sdk), "--heal", "0", "--chord-height", ".005",
                "--curve-angle", "25", "--surface-angle", "25",
                "--output-directory", str(sdk_output), *source_args], capture_output=True, text=True, timeout=180)
            (args.output / (original.stem + "-sdk.log")).write_text(process.stdout + process.stderr)
            summary, = rows(sdk_output / "definitions.tsv")
            assert int(summary["input_faces"]) == count
            if process.returncode:
                assert args.allow_export_refusal and summary["status"] == "FAIL"
                assert summary["message"].startswith("extract_f64_corner_mesh: mesh_export: unsupported nontriangle polygon;")
                assert "tessellate" in {row["stage"] for row in rows(sdk_output / "stages.tsv")}
                assert not rows(sdk_output / "failures.tsv")
                assert all(row["status"] == "1000" for row in rows(sdk_output / "imports.tsv"))
                assert not list(sdk_output.glob("*.bgrefmesh"))
                print(original.stem, "IMPORT_TESS_PASS EXPORT_UNSUPPORTED", count)
                continue
            assert summary["status"] == "PASS" and int(summary["triangles"]) > 0
            assert summary["input_manifold"] == summary["output_manifold"] == "1"
        print(original.stem, "PASS", count)
    # Output collision must not modify an existing directory.
    sentinel = args.output / "sentinel"
    sentinel.mkdir()
    (sentinel / "keep").write_text("unchanged")
    one = next(source.glob("*.brep"))
    collision = subprocess.run([str(args.tool), str(one), str(sentinel)], capture_output=True)
    assert collision.returncode != 0 and list(sentinel.iterdir()) == [sentinel / "keep"]
    print("periodic history/model PASS")


if __name__ == "__main__":
    main()
