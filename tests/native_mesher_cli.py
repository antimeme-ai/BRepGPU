"""External complete-operation CLI checks. Python is only the test boundary."""
import argparse
import csv
import hashlib
from pathlib import Path
import struct
import subprocess
import time
import tempfile
import zlib


def png(path, width, height, geometry=False):
    data = path.read_bytes()
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    at, payload, ended = 8, bytearray(), False
    while at < len(data):
        size = struct.unpack_from(">I", data, at)[0]
        tag = data[at + 4:at + 8]
        content = data[at + 8:at + 8 + size]
        assert len(content) == size
        assert zlib.crc32(tag + content) & 0xffffffff == struct.unpack_from(">I", data, at + 8 + size)[0]
        if tag == b"IHDR":
            assert struct.unpack(">IIBBBBB", content) == (width, height, 8, 2, 0, 0, 0)
        elif tag == b"IDAT":
            payload.extend(content)
        elif tag == b"IEND":
            assert not content
            ended = True
        at += 12 + size
    assert ended and at == len(data)
    raw = zlib.decompress(payload)
    assert len(raw) == height * (width * 3 + 1)
    assert all(raw[y * (width * 3 + 1)] == 0 for y in range(height))
    assert len(set(raw)) > 8, "actual render/annotation contains no varied pixels"
    if geometry:
        # Assembled view has no occurrence leader labels. Exclude its65-row
        # caption band so colored annotation cannot stand in for geometry.
        body = {raw[y * (width * 3 + 1) + 1 + x * 3:
                    y * (width * 3 + 1) + 4 + x * 3]
                for y in range(70, height) for x in range(width)}
        assert len(body) > 1, "assembled image contains only background below caption"


def invoke(binary, options, success=True):
    started = time.perf_counter()
    done = subprocess.run([str(binary), *map(str, options)], text=True,
                          capture_output=True, timeout=90)
    elapsed = time.perf_counter() - started
    assert (done.returncode == 0) == success, (done.returncode, done.stdout, done.stderr)
    if success:
        assert "required_output_through_close_seconds\t" in done.stdout
        assert "publication_seconds\t" in done.stdout
    print("external_process_exit_seconds", elapsed, sep="\t")
    return done


def check(binary, fixtures, directory, bound, display):
    assert binary.is_file(), "native whole-run CLI does not exist"
    directory.mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix="run-", dir=directory))
    print("artifact_directory", directory, sep="\t")
    common = ["--source", fixtures / "parabolic-sheet.bgsource", "--positional-mm", ".05",
              "--node-source-mm", ".000001", "--width", "320", "--height", "240"]
    outputs = []
    for tag, threads, backend in [("cpu1", 1, "cpu"), ("cpu4", 4, "cpu")]:
        output = directory / tag
        invoke(binary, [*common, "--threads", threads, "--backend", backend, "--output", output])
        outputs.append(output)
    if bound and display:
        output = directory / "p100"
        invoke(binary, [*common, "--threads", 4, "--backend", "p100", "--output", output,
                        "--bound-module", bound, "--display-module", display])
        outputs.append(output)
    reference = (outputs[0] / "mesh.bgnmesh").read_bytes()
    assert reference[:8] == b"BGNMESH1"
    for output in outputs:
        assert (output / "mesh.bgnmesh").read_bytes() == reference
        assert (output / "scene.bgscene").read_bytes() == (outputs[0] / "scene.bgscene").read_bytes()
        assert (output / "views.bgview").read_bytes() == (outputs[0] / "views.bgview").read_bytes()
        for image in ("assembled.png", "exploded.png"):
            png(output / image, 320, 240, image == "assembled.png")
        rows = list(csv.DictReader((output / "timings.tsv").open(), delimiter="\t"))
        names = {row["name"] for row in rows}
        assert {"whole.prepublication", "display.close", "meshing.total"} <= names
        labels = list(csv.DictReader((output / "labels.tsv").open(), delimiter="\t"))
        assert labels and labels[0]["occurrence"] == "1" and labels[0]["product_name"]
    before = hashlib.sha256((outputs[0] / "mesh.bgnmesh").read_bytes()).digest()
    marker = outputs[0] / "keep.bin"
    marker.write_bytes(b"must survive publication refusal\x00")
    invoke(binary, [*common, "--threads", 1, "--output", outputs[0]], False)
    assert marker.read_bytes() == b"must survive publication refusal\x00"
    assert hashlib.sha256((outputs[0] / "mesh.bgnmesh").read_bytes()).digest() == before
    bad = directory / "invalid.bgsource"
    bad.write_bytes(b"not a source")
    failed = directory / "failed"
    invoke(binary, ["--source", bad, *common[2:], "--threads", 1, "--output", failed], False)
    assert not failed.exists() and not list(directory.glob("failed.tmp-*"))
    missing = directory / "missing-device"
    invoke(binary, [*common, "--threads", 1, "--backend", "p100", "--output", missing], False)
    assert not missing.exists()
    mesh_only = directory / "mesh-only"
    invoke(binary, [*common, "--threads", 1, "--output", mesh_only, "--mesh-only"])
    assert (mesh_only / "mesh.bgnmesh").read_bytes() == reference
    assert not list(mesh_only.glob("*.png"))
    if bound:
        device_mesh = directory / "p100-mesh-only"
        invoke(binary, [*common, "--threads", 1, "--backend", "p100", "--output", device_mesh,
                        "--bound-module", bound, "--mesh-only"])
        assert (device_mesh / "mesh.bgnmesh").read_bytes() == reference
        assert not list(device_mesh.glob("*.png"))
    print("PASS native complete CLI, publication, semantic equality and independent PNG decode")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    parser.add_argument("fixtures", type=Path)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--bound-module", type=Path)
    parser.add_argument("--display-module", type=Path)
    args = parser.parse_args()
    check(args.binary, args.fixtures, args.directory, args.bound_module, args.display_module)
