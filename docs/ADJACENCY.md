# GPU edge-use adjacency and mate reader

ABI5 constructs edge-to-coedge CSR and consumes it through a mate/face-neighbor
reader on CPU or one P100. Input comes from owned immutable body snapshots or
explicit incidence records. This is topology construction and traversal, with
no body editing, radial order, geometric welding or Boolean operation.

## Python caller

```python
from brepgpu.body import make_body
from brepgpu.adjacency import pack_bodies, AdjacencyLibrary

body = make_body("cylinder")
source = pack_bodies((body,))
library = AdjacencyLibrary("build/topology-release/libbrepgpu_topology.so")
result = library.build(source, backend="cuda", device=0)

uses = result.edge_uses(2)  # both original cylinder seam occurrences
mate = result.mate(3)      # original coedge ID 6
neighbor = result.neighbor(3)  # wall face, also the use's own face
```

CPU placement uses `backend="cpu", threads=1` or `threads=24`; CPU device is
zero. CUDA requests a nonnegative device ordinal and has no silent fallback.
The constructor path can also come from `BREPGPU_TOPOLOGY_LIBRARY`.

`Source(instance, body_id, revision, entity_id)` is immutable. The containing
edge/face/coedge table supplies the entity kind; include that kind when storing
a source outside its table or reader context. Identity is never a numeric slot,
memory address or coordinate. Every batch instance names exactly one body and
revision. Repeating a snapshot in `pack_bodies` creates separate occurrences of
that same geometry, with separate slots and instance ordinals, rather than new
body namespaces or a combined solid.

`TopologyInput(edge_sources, face_sources, coedge_sources, coedge_edges,
coedge_faces, orientations, *, snapshots=())` accepts owned records for incidence
experiments. Slot/sign arrays accept integer iterables or little-endian bytes;
preparation validates counts, signs, reference ranges, typed-key uniqueness and
instance/body/revision correspondence. Generic records carry no claim of a
geometrically qualified B-rep. `pack_bodies` retains the original snapshots so
loop order, holes and UV occurrences remain available on CPU.

`input.permuted(edge_order=..., face_order=..., coedge_order=...)` takes original
slots at each new slot and rewrites every dependent edge/face reference. Keys
and original snapshot provenance remain unchanged. The accepted per-kind bound
is 2^22 records; all valid slots and prefixes fit below UINT32_MAX.

## Output and actual reader

The result retains `input` and owns immutable little-endian uint32 byte arrays:

| Array | Elements | Meaning |
| --- | ---: | --- |
| offsets | E+1 | Row ranges, first zero and final C |
| members | C | Original coedge slots, each exactly once in its edge row |
| kinds | E | Edge-use classification from the constructed CSR |
| mates | C | Other coedge slot, or UINT32_MAX |
| neighbors | C | Other use's face slot, or UINT32_MAX |

Row order is unspecified. It does not replace original directed loop order.
`edge_uses(edge_slot)` maps actual members to coedge sources; `edge_kind` reads
the actual kind. `mate(coedge_slot)` and `neighbor` read native output and resolve
typed source occurrences, returning `None` when no unique mate is supported.

| Kind | Condition | Reader |
| --- | --- | --- |
| 0 UNUSED | No uses | No entries |
| 1 BOUNDARY | One use | No mate |
| 2 PAIRED | Two opposed uses, different faces | Distinct mate and its face |
| 3 SAME_FACE_PAIR | Two opposed uses, same face | Distinct mate and same face |
| 4 ORIENTATION_CONFLICT | Two equally oriented uses | No mate |
| 5 MULTIPLE_USES | More than two uses | Complete membership, no unique mate |

Kind3 is a seam for the qualified cylinder/plate sources. Incidence alone does
not prove an arbitrary same-face pair is periodic. Closed ring edges have equal
endpoint vertices and still retain their distinct cap/wall uses. Higher degree
does not imply a radial ordering; that remains a separate construction problem.

CPU1 uses direct integer count/fill; CPU24 uses atomic count/fill and bounded
worker groups. CPU prefix scan is serial. CUDA uses owned histogram, recursive
256-lane exclusive scan, atomic fill and post-fill reader kernels on one stream.
Partial scan tiles pad zeros and keep every lane in the barriers. Construction
and the degree-two reader perform O(E+C) work, including high-degree inputs.

The separate synchronous library keeps old ABI1–4 resource lifetimes unchanged.
Each call allocates, uploads, constructs, reads, downloads and attempts cleanup.
Repeated calls reuse prepared host input and initialized runtime only. Results
remain usable after the call and library object; no device pointers escape.
Allocation pools, resident topology and session integration are deferred.

## ABI and failure

[brepgpu_topology.h](../include/brepgpu_topology.h) declares `bg5_abi_info` and
`bg5_adjacency`, a 48-byte descriptor, 24-byte ABI-info and 96-byte timings.
Payload capacities are exact element counts. Structure byte capacities are
minimum capacities with canonical descriptor size. Information/timings are
output-only. Optional diagnostic storage uses the existing 288-byte ABI1 shape.

Pointers must name valid aligned storage stable for the synchronous call.
Structural range/alias admission precedes diagnostic writes; unsafe diagnostic
destinations receive no write. All outputs and structures must be separate from
input and each other. Input arrays may overlap. Preflight rejection leaves
payload destinations untouched. Runtime failure can partly modify them: discard
all outputs. Python raises and publishes no partial result. Failed cleanup or
device restoration prevents success; primary errors retain bounded secondary
diagnostics. Runtime cleanup failure may prevent reclamation.

`result.timings` records synchronized host spans for admission, CUDA context,
allocation, upload, count, scan, fill, reader, download, cleanup and native total.
These include launch/synchronization overhead and are not kernel-only times.
Wrapper allocation/native-call/publication/wall spans are separate. Publication
times binary freezing and result fields; final timing-tuple assembly is covered
by external build/caller wall. Actual full reader traversal and independent
qualification have separate costs in the campaign.

CUDA payload upload is **12C bytes** (edge slots, face slots, signs). Payload
download is **8E+12C+4 bytes** (CSR, kinds, mates, neighbors). Typed identity,
geometry, loops and UV maps have zero device upload in this operation. These
are host/device payload bytes, excluding kernel-launch metadata, scratch traffic
and driver staging; they are not network transfers.

## Reproduction

Build only the new targets, in a new build tree, using the isolated CUDA12
compiler/root documented in [CUDA_TOOLCHAIN.md](CUDA_TOOLCHAIN.md):

```sh
cmake -S . -B build/topology-release -DCMAKE_BUILD_TYPE=Release \
  -DBREPGPU_ENABLE_CUDA=ON \
  -DCMAKE_CUDA_COMPILER="$cad_cuda_root/bin/nvcc" \
  -DCUDAToolkit_ROOT="$cad_cuda_root"
cmake --build build/topology-release \
  --target brepgpu_topology brepgpu_topology_native_checks -j4
PYTHONPATH=python python3 examples/build_adjacency.py \
  --recipe cylinder --order permuted --backend cuda --device 0 \
  --library-path build/topology-release/libbrepgpu_topology.so \
  --output-directory context/adjacency-caller
```

Native targets use sm_60 real code. A CPU-only build omits CUDA and reports
unsupported for CUDA placement. Product execution and mutants run on the fleet.
The caller exports actual binary inputs/outputs, typed sources, original edges,
faces, directed loops/UV occurrences and an incidence SVG. Independent truth
uses literal body loops plus a source-relation model, not CPU/GPU parity.

The [reviewed design](ADJACENCY_DESIGN.md), [plan](ADJACENCY_PLAN.md), research
and journals retain algorithm choices, adversarial findings and their resolutions.
