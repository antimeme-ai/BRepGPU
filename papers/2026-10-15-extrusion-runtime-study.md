# Planar-profile extrusion: owned runtime and representation study

2026-10-15. Research only: read current body/CutSolid, Patch/native evaluator,
sampling and topology contracts, plus pinned OCCT/NVIDIA sweep interfaces and
implementations. No modeling/reference execution, build, install or product code.
The already available optional reference API is a comparison target, not a new
production dependency.

## Bounded useful operation

Recommend a real source profile→closed prism operation, using a strictly convex
CCW polygon with3..32 vertices, no holes, exact1/256-grid coordinates in
[-.75,.75]^2. Retain its ordered source vertices/edges/uses/loop/face/chart and
explicit source identity. Root's provisional fixed source carrier is the plane
XY=(-1+2u,-1+2v), Z=0 on[0,1]^2. This is preferable to fitting a bounding-box
carrier: original UV survives unchanged and no normalization quotient is needed.
Existing bounded Frame supplies world origin and power-of-two scale.

Extrude symmetrically to local Z=±h/2, with h on1/256 grid in[.25,2]. Two cap
carriers copy original XY control bits at the new Z values; each original edge
generates one bilinear side carrier from its actual endpoint XY and the two Z
levels. Every homogeneous weight is1 and every control lies in[-1,1]^3. All local
controls are exactly representable dyadics, including the1/512-height grid; no
refitting, knot insertion, geometric approximation or new native ABI is needed.

The polygon family adds nonrectangular geometry rather than reproducing a box or
the previous bore. A rectangle with an interior circle could reuse the existing
annular-cap/cylindrical-wall ideas, but requires the circle/seam/trim machinery
already exercised by the bore. Concave polygons, arbitrary curves, holes,
oblique/negative/infinite/drafted sweeps and generalized construction architecture
are separate work. A strict finite convex first family is useful without making
any of those a prerequisite.

## Exact input and geometry ownership

Convert accepted source coordinates to exact ratios before checking grid/envelope,
unique vertex identity, strict convex CCW order and all nonadjacent incidences.
Every nonincident vertex must lie strictly to the left of every oriented boundary
edge; adjacent endpoints alone have zero side test. This avoids self-crossing,
concavity, redundant collinear nodes and zero-length edges without a tolerance.
Bound the iterable at33 observations, before materializing arbitrary input. Reject
bool/unsupported numeric types, nonfinite values, off-grid data and invalid height.
Never silently reorder, weld, round onto the lattice or simplify source vertices.

The lattice is also numerically useful: distinct source vertices cannot approach
arbitrarily closely. Minimum possible edge length is at least1/256 and height
is at least1/4, preventing the vanishing wall normals admitted by unrestricted
binary64 endpoints. Cap derivative magnitudes are fixed by the original2×2
carrier. Direct emitted-position/normal grades still need actual independent
qualification; a local derivative error grade by itself is not a normal bound.

Construct a separate factory-owned profile/result type, not BodySnapshot or a
canonical shape label. Existing canonical sampling/classification/meshing/FaceRef
guards embed fixed families and must remain unchanged. Reuse immutable typed
Vertex/Curve/Edge/Coedge/Loop/Face/Chart meanings and Patch/native Lineage as they
fit; no need for a generalized body protocol or to retrofit the Boolean class.
Allocate source-profile and result namespaces through shared body._namespace.
Snapshot IDs are scoped identities, not persistent cross-session names.

For N source edges, the result has2N vertices,3N edges,6N coedges and N+2 each of
faces/loops/charts, one connected outward shell with Euler2. Source vertex IDs
define bottom/top copies and the shared vertical edges; source edge IDs define
bottom/top edges and side faces. Cap outer-loop words reverse on the bottom;
side maps follow source edge traversal in U and bottom→top height in V. For CCW
edge(dx,dy), side du×dv points right=(dy,-dx,0), the outward sense. Top/bottom cap
sense is +1/−1 respectively. Every physical edge has two opposite uses and vertex
links are cycles; no seam is manufactured for this polygon family.

Cap trim labels apply all exact polygon halfspaces to actual fixed-carrier XY;
carrier-square points outside the polygon remain explicit rejected candidates.
Wall trims are complete[0,1]^2 rectangles. World material membership combines
the same exact source halfspaces with the two closed height constraints, returning
every incident output face at edges/corners. Source cap UV, curve parameter and
face sense are separate maps; apply orientation once. Exact shoelace area and
height give volume, but volume/counts cannot replace whole-domain geometry,
boundary and embedded-manifold checks.

## Feature provenance, transaction and serialization

Retain the exact input profile and full six-kind profile→solid provenance.
Profile vertices support two copied endpoint vertices plus swept vertical edges;
profile edges support two translated boundary edges and swept side faces.
Original uses/loops/face/chart have explicit bottom/top descendants, source UV
identity and cap orientation. Generated side uses/loops/charts retain their
actual generating edge/vertex/profile support. Every output entity needs an
origin and every source an explicit disposition; do not infer history by matching
coordinates. Centered extrusion moves both caps away from source Z0, so a
“retained unchanged base” convention would be false for this selected operation.
Reuse generic typed key/record meanings where appropriate; operation-owned
assembly/validation defines the actual history, not another kernel's conventions.

Validate admission, source maps, complete loop/face/chart ownership, opposed edge
incidence, connected shell, vertex links, complete exact material domains and
forward/reverse provenance before one immutable publication. Local candidate
arrays never escape. Late construction failures preserve source/profile/prior
results; namespace gaps are acceptable. A later consumer failure publishes no
partial batch and preserves the already valid result. No native handle belongs
to the CPU construction. Export typed native records/TSV/binary with a distinct
profile/result identifier, not canonical BGbody01 shape tags or internal JSON.

## Existing CPU/P100 consumers and honest costs

A narrow operation-specific adapter suffices: copy actual binary64 Candidate UV
before exact trim classification, retain result/request identity, gather admitted
slots/parameters, evaluate first jets with current Engine, check complete returned
counts/order/statuses/descriptors/Lineage/backend/query bits, convert using the
actual per-Patch frame, orient normals, scatter all rows and consume geometry.
Exact Fraction classify queries remain distinct from their rounded native
neighbors. Borrow Engine; close patch/query/result children on every outcome,
preserving the primary exception. Optional profiling fields obey the existing
flags0/NaN versus timing-valid contract.

For up to34 charts, choose a fixed complete-request cap (for example1,048,576)
and check chart_count*resolution^2 before generating grids. Do not reuse the
canonical ten-chart cap as an accidental new-family bound. A direct
TopologyInput with typed source IDs and snapshots=() can use existing synchronous
ABI5, retaining the actual feature result in its caller. Read actual mates and
neighbor faces. No body API widening, mesh integration or native rebuild is
needed for this useful geometry/topology consumer.

Measure profile preparation separately, then admission/exact geometry/topology/
history/validation/publication construction and complete actual graph/history/
material reads, request setup, native context/packing/upload/evaluation/download,
frame/normal/scatter/reader work and cleanup. Qualification, reference experiment,
exports/SVG and process startup are separate; retain full child-process wall.
Nested phase rows overlap enclosing timers and are not additive medians. The
previous bore campaign already showed fresh geometry/topology setup can dominate;
do not assume a P100 gain or start the deferred residency/tuning lane. Use real
triangle/pentagon/more-sided source profiles and record actual topology/query
counts; merely changing world scale is not a larger problem. An actual wireframe
plus native-sample visual is valid without claiming a general feature mesher.

## Primary reference implementation and optional experiment

Pinned OCCT MakePrism explicitly distinguishes generating shape from sweep and
exposes first/last/generated shapes. Its implementation delegates to translation
sweep topology; its input check may build missing3D curves, so even a const-looking
reference entry is not a promise of an entirely untouched input. Its Copy and
Canonize options, tolerance-based zero-vector rejection and infinite-prism support
are reference behavior, not selected owned API requirements.

Pinned NVIDIA SmApiCreateLinearSweep copies source curves before passing them to
the consuming primitive routine, sets output null first and uses cleanup ownership
for the new body. Public Python linear_sweep(curves,direction,distance,
cap_ends=True) documents inputs unchanged, direction magnitude ignored and distance
as sweep length. This public function was present in the previously inspected
installed module; no current extrusion execution has been performed. A future
isolated experiment can create actual source-profile boundary line segments at
the lower world plane and call direction(0,0,1), distance=h*Frame.scale, capped.
Retain curve handles for before/after checks. The optional reference may round
exact world endpoints on construction: publish those actual endpoint bits/error,
or choose dyadic translations with exactly representable benchmark endpoints.
Compare returned manifold status, analytic/owned volume, finite complete bounds
and deterministic source-edge/face-interior boundary witnesses. Foreign topology
counts/history are not owned identity truth, and absent material/history adapters
remain explicit. No reference dependency adoption or alpha-corpus waiver follows.

Sources: [OCCT MakePrism](https://occt3d.com/dev/doc/refman/html/class_b_rep_prim_a_p_i___make_prism.html),
[NVIDIA Python API](https://nvidia-omniverse.github.io/usd-brep/latest/docs/python.html),
and pinned source paths/hashes in `extrusion-runtime-sources.tsv`;
`extrusion-runtime.bib` records the primary bibliography.
