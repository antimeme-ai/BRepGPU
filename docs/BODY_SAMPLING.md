# Body snapshots and face sampling

The first body consumer constructs owned analytic box, cylinder and one-hole
plate snapshots. It retains faces, shared edges/coedges, holes and a periodic
seam, then samples admitted chart UVs through the existing native first-jet
evaluator. This is CPU body construction/classification and normal consumption
around C++/CUDA geometry execution; no new native ABI or dependency is needed.
[Qualification and complete-caller measurements](../results/body-2026-10-14/REPORT.md)
are delivered on CPU1/CPU24 and both P100s.

## Run the actual caller

Build/select the core and optional plugin as in [iteration1](ITERATION_1.md).
The Python adapter is standard-library only:

```sh
export PYTHONPATH="$PWD/python"
export BREPGPU_LIBRARY="$PWD/build/discovery-release/libbrepgpu.so"
python3 examples/sample_body.py --shape plate --backend cuda --device 0 \
  --resolution 33 --output-directory results/local-body
```

CPU uses `--backend cpu --threads 1` or `--threads 24`. Other shapes are box
and cylinder. Resolution is 2..257. `--origin x y z --scale s` chooses the
admitted common frame: finite origins in [-16,16], positive power-of-two scale
in [1/4,4]. `--face-ids` selects canonical face IDs; an empty selection produces
an empty batch. Duplicate/unknown IDs fail before query allocation. Output
directories must be fresh. `--library-path`/`--plugin-path` provide explicit
library selection without publishing those private paths into artifacts.

The plate is a constructed analytic recipe, not the output of an owned Boolean.
All three fixtures use z in [-1/2,1/2]; box/plate x/y extent is +/-3/4, and the
cylinder/hole radius is 1/4. Face order is top,bottom,front,back,right,left for
box/plate, followed by the plate wall; cylinder uses top,bottom,wall.

## Native and Python API

```python
from brepgpu import Engine
from brepgpu.body import make_body
from brepgpu.body_sampling import BodySampler, prepare_grid

body = make_body("plate")
request = prepare_grid(body, 33)
with Engine(backend="cpu", threads=1) as engine:
    with BodySampler(engine, body) as sampler:
        first = sampler.sample(request)
        second = sampler.sample(request)  # Reuses geometry and classified grid.
# first/second retain owned rows and source identity after native cleanup.
```

`BodySampler` borrows the Engine and owns one uploaded PatchSet. Each call
creates fresh query/result storage, downloads XYZ/first derivatives, checks
actual source UV/slot bits and complete lineage, converts to the common world
frame and computes oriented normals. Admitted native or normal failures raise;
no successful partial batch or automatic CPU fallback is published.

`SampleRequest(body, candidates)` supports explicit frozen
`Candidate(patch_slot,u,v)` rows. Candidate slots are snapshot-local; metadata
derives from the source charts. Requests bind to the exact immutable snapshot;
another body/frame cannot reuse their masks or dense slots. The maximum row
count is 660,490 and is enforced during iterable copying.

Body and output values are recursively immutable tuples/bytes/frozen records.
Factory creation is the only supported body input. Packed record layouts are
documented in [body.py](../python/brepgpu/body.py). Exported IDs name a
process-local body/revision/kind/entity; the table dumps are not a general
cross-session import format or persistent naming system.

## Trims, charts and normals

Each cylindrical wall is ONE face with four rational quarter charts. The
lifted atlas is a in [0,4], v in [0,1]; native chart k uses s=a-k. Quarter joins
have no invented body edge. The seam has two occurrences, a=0 and a=4, of ONE
vertical spatial edge. Top/bottom rings each use a single closed edge with four
geometry spans. Its parameter is piecewise rational, not angle/arc length;
derivatives at quarter joins are local-chart/one-sided. Normals remain per
face/chart, without smoothing at sharp body edges.

The classifier operates on the exact submitted binary64 UV, using integer
circle predicates. Labels are 0 rejected, 1 interior, 2 exact boundary and
3 invalid. Interior/boundary rows are evaluated; other rows retain NaN XYZ and
normal with native_status=-1. Counts include repeated face/chart boundaries.

The plate's hole curve has an exact symbolic spatial-to-UV map, but none of
its circular UV points is dyadic. Rounded UVs obtained from that map retain
their actual inside/outside label; they are not declared exact boundary points.
Cylinder caps do have exact cardinal UV boundary witnesses. This distinction
is necessary before shared-edge meshing, but this caller emits no triangles.

## Artifacts and timing

The caller emits all-row samples.tsv, admitted point-cloud points.ply,
body-samples.svg, packed body tables, original patch/control bits, frame/source
records and independent exact analytic qualification. SVG curves are CPU-owned
reference boundaries; its points and normal arrows come from actual evaluated
rows, thinned only for display. No chord, embedding or watertight mesh claim
comes from these point files.

Cold sampling wall starts before body/request preparation and includes native
setup, output and cleanup. Qualification/export have separate phases; actual
export-inclusive wall is also retained, excluding the final timing-file write.
Internal phases name query packing/upload, native evaluation, Python
download/provenance, host conversion and normal/scatter. Their independent
medians must not be added into total latency.

The timing driver runs a small matrix serially:

```sh
python3 benchmarks/body/run.py --output-directory results/local-body-benchmark
```

It measures three bodies, resolutions17/65, CPU1/CPU24/P1000/P1001, one cold
observation, an excluded warmup and five reused calls. The primary
actual_reused_caller_ms wraps the returned API call; internal reused_wall_ms
ends through child cleanup. Reused calls retain the prepared classified grid,
Engine and geometry, but include fresh queries, output and CPU normal work.
First exact qualification and export are outside that repeated timer.

CUDA query input is 20 bytes/evaluated row and first-jet/status output is76.
Only geometry uploads:16 bytes/native patch plus homogeneous controls. Body
adjacency stays on CPU. Source descriptors/UV, conversion and Python allocation
add host memory work. These are same-host RAM/VRAM transfers, not server network
traffic. CPU24 changes evaluator workers, not Python stages.

[Design](BODY_SAMPLING_DESIGN.md) · [Reviewed plan](BODY_SAMPLING_PLAN.md) ·
[Breadth-first theme map](ACCELERATION_ROADMAP.md#breadth-first-delivery-policy).
