# Complete analytic face sections

Intersect a closed planar face with a finite cylindrical wall in the owned
box/cylinder/one-hole-plate corpus. Construction is exact CPU Python algebra;
the existing owned C++/CUDA evaluator consumes supplied curve samples on both
surfaces. No general SSI, GPU construction, topology mutation or Boolean is
implemented by this operation. [Design](INTERSECTION_DESIGN.md),
[plan](INTERSECTION_PLAN.md), [issue20](https://github.com/antimeme-ai/BRepGPU/issues/20).

```python
from brepgpu import Engine
from brepgpu.body import Frame, make_body
from brepgpu.intersection_sampling import prepare_samples, IntersectionSampler
from brepgpu.intersection_oracles import qualify_batch, recipe_counts

plane = make_body("box", Frame((0., 0., -.125), .25))
wall = make_body("cylinder")
result = plane.intersect_faces(1, wall, 3)   # Four corner arcs at z=0
recipe_counts(result, "clipped")           # Literal complete-count oracle
request = prepare_samples(result, 65)
with Engine(backend="cuda", device=0) as engine:
    with IntersectionSampler(engine, result) as sampler:
        batch = sampler.sample(request)
qualify_batch(batch, request)               # Whole maps/graph + actual sources
```

The explicit face API is `intersect_faces(FaceRef(left_body,left_face_id),
FaceRef(right_body,right_face_id))` in brepgpu.intersections. Both operand
orders work. Select exactly one face with kind rectangle/disk/rectangle_hole
and one wall with kind cylinder. Frames translate and scale by positive powers
of two in the existing admitted range; axes stay aligned. Plane-plane,
wall-wall, imported/tilted/spline families raise UnsupportedIntersection.
Supported empty results are distinct from unsupported or arithmetic failure.

Supported Python execution is round-to-nearest/ties-even with gradual underflow,
as qualified on CPython3.12/Linux and the native backends. Arbitrary FTZ/DAZ or
other rounding environments are outside this scope; float-to-Fraction decoding
and float equality retain this precondition. CUDA sampling targets the existing
qualified P100/CUDA12.8.1 binaries; exact construction needs no CUDA compiler.

## Exact geometry and occurrences

IntersectionSet is factory-only and recursively immutable. It retains exact
left/right FaceRefs, components, pieces and events. IDs are dense zero-based
within this result, not addresses or persistent names. Source identities remain
the body's process namespace/revision/face/chart values. No GPU topology table
or pointer graph is uploaded by the consumer.

Components have kind circle/arc/segment/point, closed, carrier-tangent, and
ordered piece_ids. Piece fields include component_id, span_id, original left/
right chart slots, lo/hi, spatial_map/left_map/right_map and boundary_constraints.
Each ExactMap exposes ascending-power Scalar coefficients in numerator tuples
and its positive common denominator, and returns exact values at the common
parameter. Scalars expose Fraction a/b and integer d for a+b√d; float conversion
rounds with exact comparisons, and bracket() returns adjacent enclosing floats.

Horizontal pieces use the wall's rational quarter parameter s in[0,1]; this is
not angle or arclength. Vertical generators use common world height. Spatial
and both normalized-native-UV maps refer to the same point for the whole piece,
with source wall atlas lift quadrant+u. A circle is one closed component with
four pieces unless additional trim events split a chart. Crossing a seam keeps
one arc; a cardinal/seam generator may have two chart occurrences sharing one
physical span. Count grouped spans once when reading physical graph degree.
Events retain every endpoint, constraint, atlas join and occurrence incidence.

Finite domains and trims are CLOSED: disk rims and hole boundaries are included.
Exact quadratic cell decomposition retains all components, trim tangencies and
boundary-coincident curves. A carrier-tangent generator is a segment if it has
positive height. An isolated contact is a point with no fabricated edge/self-loop.
Named events, rather than approximate XYZ welding, establish connectivity.
Canonical direction does not assign future Boolean coedge orientation.

## Sampling and failure

prepare_samples(result,resolution) accepts integers2..4097 and at most2^19
occurrence candidates. Every non-point piece contributes that many candidates;
each point occurrence contributes one. Endpoints remain exact algebraic values.
Rational spans use rational interior spacing; irrational spans use rational
values in an inward interval, with at most4096 dyadic subdivisions when needed.
SamplingPrecisionError fails preparation without damaging complete geometry.
No uniform spatial approximation or mesh embedding bound follows from this grid.

IntersectionSampler borrows Engine and owns the selected faces' PatchSet.
Requests bind the exact result object; an equivalent separately constructed
result is a different request domain. One order-zero native query contains2*N
rows, interleaved left/right. Batch.count isN and evaluated_count is2*N.
Rows retain common parameter, location, component/span/piece IDs, original
source slots, rounded UVs, exact reference point, both world positions/statuses.

The producer compares actual native descriptors and slot/UV bytes, checks all
statuses, then grades both positions against the exact curve and each other
componentwise at2^-36. SamplingArithmeticError fails the whole batch; no partial
successful output is published. Exact endpoints on a trim need not round to UVs
classified BOUNDARY. Rounded samples never decide exact topology or membership.

Actual source_descriptors/native_slots/native_uv are immutable binary copies
from the returned native source buffers; dense_sources maps each native slot to
(side0or1,original_slot). Empty results submit no query and carry no fabricated
native provenance. Explicit BaseException guards consume children, preserve a
primary failure and attach secondary cleanup failures. Close failure on success
fails the operation. Geometry and returned batches remain readable after close.

## Truth and costs

qualify_result proves whole-piece identities against ORIGINAL homogeneous
controls/frames, exact closed domains/trims, event incidence and ordered physical
span connectivity. It does not prove complete component counts for every
unknown configuration. recipe_counts proves those counts and exact endpoints
for the two literal caller recipes. qualify_batch(batch,request) additionally
binds actual row order/parameters to submitted candidates, checks actual native
metadata and independently grades positions/rounding. Its one-argument form
proves submitted-row validity only. Direct tests cover further complete literals,
subnormal near-tangency, unsupported input and resource/failure paths.

Run the actual caller with PYTHONPATH=python:

```sh
python3 examples/intersect_faces.py --recipe clipped --resolution 65 \
  --backend cuda --device 0 --library-path build/pair-release/libbrepgpu.so \
  --output-directory results/my-section
```

It consumes all four chart-pair bits, groups one face pair, constructs a complete
face result and exports exact coefficients/events/request, actual native source
buffers, two-sided samples, source/binary manifests and an owned SVG. Its whole
wall includes all non-timing artifacts; final timing-file/console writes are
outside that span. This complete scene contains only the selected supported pair.
An incomplete broad-phase prefix cannot certify general scene intersections.

benchmarks/intersections/run.py compares two recipes at65/4097 samples per
piece on CPU1/CPU24/P1000/P1001. Fresh construction/request/engine/setup and final
cleanup are separate from five reused calls on retained values/Engine/PatchSet.
Main trials disable events; evaluate_ms is a synchronous host span including
native allocation/launch/evaluation, not a kernel-only timer. Gather, query,
download/provenance copies, world conversion/scatter, exact model grades and
child cleanup remain measured host costs. External sample wall includes final
batch publication. Initial independent qualification is outside reused timing.

For these recipes geometry payload is976 bytes (896 controls+80 compact device
descriptors) once on CUDA. Each two-sided candidate uploads40 bytes and downloads
56 position/status bytes. The136*5 source descriptors and40*N source slot/UV
bytes are host metadata, not CUDA transfer. The actual caller also uploads240
pair-index bytes and downloads4 mask+240 resident-bound bytes. These are host/
device movements, not server/network transfers. Topology upload is zero.

Read the [results](../results/intersections-2026-10-14/REPORT.md) for actual
qualification, versions, measured ranges and scope. Round-two general SSI,
arrangements, approximation and GPU construction ideas are retained in
[research notes](../papers/2026-10-14-intersection-round2-notes.md) and the
[return pass](RETURN_PASS.md). GPU session/residency/integration polish stays
after the broad-pass big rocks.
