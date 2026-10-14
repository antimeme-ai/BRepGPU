# First body beachhead: snapshots and face samples

Design, 2026-10-14. Proposed until implementation/qualification are recorded.
This operation connects the existing patch evaluator to owned analytic bodies.
It establishes body, trim, chart and shared-boundary semantics without a new
native ABI, GPU topology builder, mesh, importer or Boolean operation.

## Task and support family

Construct one immutable owned `box`, `cylinder` or `plate` snapshot. Select
faces and a grid resolution; classify every candidate in its chart; evaluate
admitted UVs in one native first-jet batch; emit oriented unit normals, XYZ,
source identities and trim labels. Export all candidates, including rejected
ones, and a point-cloud PLY/SVG of the admitted samples. Measure the complete
caller with CPU1, CPU24 and each P100, separating first use and reused inputs.

Canonical body coordinates:

* Box: x/y in [-3/4,3/4], z in [-1/2,1/2].
* Cylinder: radius 1/4, z in [-1/2,1/2], including its two trimmed disk caps.
* Plate: the box with a radius-1/4 through-hole along the z axis. This is a
  constructed analytic fixture, not an implemented Boolean or imported part.

All carrier controls are exact dyadic homogeneous values in the existing
positive-weight/Cartesian evaluator envelope. Planes are degree (1,1).
Each cylinder quadrant is degree (2,1), built from the exact circle map
`c(s)=((1-s*s)/(1+s*s),2*s/(1+s*s))`, scaled by 1/4 and rotated by a
multiple of 90 degrees. Its V map is z=-1/2+v.

One body frame applies to every carrier and boundary: world position is
`origin + scale * body_position`. Origin components are finite and bounded
by 16; scale is a positive power of two in [1/4,4]. No rotation/general affine
frame is admitted. This restriction leaves ample margin inside the existing
conversion contract. Frames change no topology, chart coordinates or unit
normal direction. World/source conversion uses the existing `JetBatch.convert`.

## Owned body data

The first body layer is CPU Python using standard-library immutable native
values: frozen records, recursively immutable tuples and owned packed binary
tables. Geometry execution remains owned C++17/CUDA through the existing C ABI.
Python object addresses never define identity, and no mutable `array`/list is
retained inside a snapshot. This is an explicit first placement, not a claim
that CUDA reads body adjacency or that topology is constructed in C++ yet.

`BodySnapshot` owns shape, process-local body ID, revision=1, frame, vertices,
curves, edges, coedges, ordered loops, faces and native patch/chart descriptors.
Entity IDs are typed 64-bit integers assigned deterministically within that
body; the process allocates body namespaces monotonically and never recycles
them. Dense slots are uint32 and only meaningful for that snapshot. There is
one outward shell containing all oriented face uses. No editing, persistent
naming or cross-process import of dumped IDs is promised.

Canonical relationships follow [the topology architecture](TOPOLOGY.md):

* Vertex: ID and body-space position.
* Curve: line or closed piecewise rational circle, with explicit body geometry
  and canonical parameter t in [0,1]. Each circle has four spans. The spatial
  curve is continuous; uniform t is not promised to be angle or arc length.
* Edge: ID, endpoint vertex slots and curve slot. A circle edge begins and
  ends at the same designated vertex; its four geometry spans do not create
  additional body edges or vertices.
* Coedge: ID, edge/face/loop slots, orientation +/-1 and an owned UV map of the
  canonical edge parameter. Opposite traversal changes parameter order, not
  the edge identity. The two seam uses have distinct atlas locations.
* Loop: ID, face slot, outer/hole role and ordered coedge slots.
* Face: ID, supported carrier/trim kind, orientation +/-1, loop slots and
  patch/chart slots. A cylindrical wall remains ONE face with four charts.

Loop coedge order supplies canonical adjacency. Packed tables additionally
carry offsets/counts into loop-to-coedge, face-to-loop and face-to-chart slot
arrays. Edge-to-coedge and vertex-to-edge views are derived from the canonical
records, not independently edited graphs. Binary table formats use explicit
little-endian integers/binary64, no pointers or JSON. Their concrete field
layout will be documented with the module; no public C ABI is added.

## Charts, curves and boundary correspondence

Each plane has a normalized [0,1]^2 chart. Its affine map and trim kind are
owned: rectangle, rectangle minus open disk, or closed disk. Caps and the
plate top/bottom use the same carrier with different exact membership rules.
Plane orientation applies to normal direction and boundary traversal.

A cylindrical face has one rectangular lifted atlas `(a,v)` in [0,4]x[0,1].
Native chart k in {0,1,2,3} uses s=a-k. Its source UV rectangle is
[k,k+1]x[0,1] and its lineage retains body/revision/face/chart IDs.
Internal chart joins a=1,2,3 are not body boundaries. The periodic seam has
two coedge uses, at a=0 and a=4, referring to ONE spatial vertical edge.

A closed circle edge uses a=4t, selecting quadrant floor(a), with t=1 handled
as the last quadrant endpoint. Its planar coedge UV map is the exact expression
`((x(t)+half_width)/(2*half_width),
  (y(t)+half_width)/(2*half_width))`.
The representation stores original dyadic half-width/radius, not a rounded
coefficient such as 1/6. Cylinder coedges use (4t,0/1). Seam coedges use
(0/4,t). Spatial and UV curves thus have an explicit shared parameter.
Floating evaluations of these maps are approximations to those expressions;
they are not automatically exact-boundary UV witnesses.

Outer-cylinder wall orientation is +1; the plate inner wall is -1. Top caps
have +z outward normals, bottom caps -z. All directed loop traversals follow
the corresponding oriented face. Circle and seam identities are shared with
incident face uses, rather than welded by coordinate proximity.

## Exact trim classification and candidate records

Classification is about the submitted binary64 UV value, independent of the
rounded XYZ returned by the evaluator. Labels: REJECTED=0, INTERIOR=1,
BOUNDARY=2, INVALID=3. Nonfinite/out-of-[0,1] local UV is INVALID. Samples on
an admitted boundary are evaluated; REJECTED/INVALID samples are not.

Plane rectangular boundaries are exact u/v=0 or 1. For disks/holes write
u=un/ud, v=vn/vd from exact integer ratios. With half-width/radius=1 for
cylinder caps or 3 for the plate, the sign of

`ratio^2*((2*un-ud)^2*vd^2 + (2*vn-vd)^2*ud^2) - ud^2*vd^2`

decides exact disk location. No tolerance band or rounded squared distance is
used. Closed disk keeps negative/zero; plate removes negative and keeps zero.
Plate outer-rectangle boundaries are also BOUNDARY. Wall t=0/1 and the true
seam at chart0 s=0/chart3 s=1 are BOUNDARY; internal quadrant joins are INTERIOR
unless on top/bottom. Boundary rows may repeat across chart/face grids; this
caller is not a unique-point sampler or shared-edge mesh.

Candidate grid order is selected faces in snapshot order, their charts in
atlas order, then u-major/v-fast. Resolution is an integer in [2,257]; empty
face selection is supported and produces an empty batch. Face selection rejects
unknown or duplicate IDs. Samples preserve local UV, chart/face IDs, dense
patch slot, body ID/revision and original candidate row. The sampler also
accepts an explicitly prepared candidate sequence for adversarial/caller uses.

The grid and gather map are immutable reusable CPU inputs. Admitted rows are
gathered into native patch slots/UV once per operation; first-jet output uses
the admitted count as its SoA stride. Scatter restores original candidate order.
Rejected/invalid rows contain NaN XYZ and normals and an explicit not-evaluated
native status sentinel; no failed native result is mislabeled as trim rejection.

## Execution, failures and ownership

A sampler binds one snapshot to one engine and retains one native patch set.
Existing native resource/context rules apply. Explicit close is idempotent;
use after sampler/engine close fails. Construction failure closes any created
native resource. Result/query handles use existing context managers even if
download, conversion or normal preparation raises. No hidden CPU fallback.

Every admitted native query must succeed. Any failed status raises with native
diagnostics and prevents publication of a successful sample batch. Positions
and derivatives are converted once; normals are
`orientation * cross(Su,Sv)/hypot(cross(Su,Sv))`.
A nonfinite/zero norm or output raises ArithmeticError, rather than publishing
a fabricated normal. The admitted analytic family has no degenerate chart.
No general arbitrary-control normal solver or tangent rank guarantee is added.

Output batches own immutable row values and lineage. Input/caller mutations
cannot alter the snapshot or prepared request. Results remain readable after
sampler/engine close and do not retain a dependency on live native handles.
Grid preparation validates count/selection before allocating candidate buffers.
The maximum chart count is 10, giving at most 660,490 candidate rows.

## Direct qualification and value

Independent qualification uses exact rational analytic maps, not production
curve evaluation, classification or GPU/CPU parity as truth. Check each original
binary64 UV against exact trim membership; XYZ against exact rational geometry;
normals against the known outward analytic direction; returned source identities
against the originating face/chart. Corpus error targets: world XYZ <=2^-36
componentwise, normal direction <=2^-30 componentwise. These are checked
finite-corpus targets, not a universal floating-point error theorem.

Topology checks cover expected counts, typed references, opposed two-use edges,
ordered loop closure, correct outer/hole orientation, seam repeated face uses
and complete analytic spatial-edge/UV correspondence. Check cardinal boundary
points, nextafter neighbors, chart joins/seam, rounded arc UVs, empty/selected
faces, invalid inputs, immutable ownership and frame transforms. Body topology
is established for these constructed fixtures, not inferred from point samples.
PLY and SVG are point outputs, not tessellation, embedding or chord certificates.

Timing separates snapshot/request preparation, engine first use, patch upload,
native query creation (including Python packing/upload), evaluation, download
(including provenance copies), conversion, normal/scatter construction and
cleanup. Reused-caller wall includes query creation through child cleanup with
snapshot, prepared grid, engine and uploaded patches retained. Export and exact
qualification are timed separately; actual exported-caller wall through those
steps is also reported. Cold caller includes preparation and first use/cleanup.
Intervals are host wall measurements; no claim of pure device time.

Measure a small fixed matrix: all three bodies at resolutions 17 and 65,
CPU1/CPU24/P1000/P1001, one excluded warmup and five reused samples per case.
Publish raw samples, ranges, first-use observation and all labels/qualification
outcomes. Run cases serially after copies/qualification complete. Report transfer
payloads as actual uploaded evaluated rows (20 bytes/query) and order1 results
(76 bytes/query), separately from host source metadata/body tables. This does
not accelerate reference-kernel Booleans or establish full CAD speedup.

## Research inputs

The retained topology/reference source studies motivate explicit identity,
coedge occurrences, seam maps and separate carrier/trim data. New narrow studies
are recorded under papers/2026-10-14-body-*-study.md. No library dependency is
selected; no general extraction or assembly problem is hidden inside this task.
