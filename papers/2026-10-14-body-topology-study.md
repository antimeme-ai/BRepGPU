# Minimal owned body snapshot and sampling consumer

Design advice,2026-10-14; no product implementation or native execution.
An immutable Python body layer with owned packed bytes fits this beachhead.
The existing native evaluator consumes copied patch descriptors/controls and
query arrays; it does not need a new body ABI to sample these three parts.
CPU topology, exact trim decisions and result provenance remain explicit costs.
The body is a constructed analytic fixture, not an imported model or an owned
Boolean result.

## Smallest representation

Use factory-only canonical constructors. Frozen records must contain recursively
immutable tuples/bytes rather than mutable arrays. Authoritative topology is
the vertex/edge/coedge/loop/face/chart records; packed adjacency and binary tables
are built once from those records, not independently edited. Store typed64-bit
entity IDs, uint32 snapshot slots, offset/count adjacency and homogeneous FP64
control records. Body namespace/revision and chart identity accompany every
slot. Runtime handles belong to a separate uploaded sampler, not the body.

The initial geometry has lines, one closed piecewise circle per rim, affine
planes and one four-chart cylinder carrier. A curve has canonical t∈[0,1]; an
edge references its endpoint vertices and curve; each coedge references the
same edge plus face/loop, traversal sign and explicit UV map. A closed edge
may have the same start/end vertex. Count its two endpoint incidences separately
when deriving vertex valence. Do not create physical edges at rational span
boundaries or derive identities from coordinates/object addresses.

Construct only one closed outward shell; no incremental editing, general
nonmanifold import, allocator migration or persistent cross-session naming.
Packed bytes need a versioned little-endian schema with explicit lengths/types,
but dumping them does not promise cross-process entity-ID restoration. The
concrete layout belongs in the reviewed implementation plan/module.

## Canonical geometry and frame

Use box x/y±3/4,z±1/2; cylinder radius1/4,z±1/2; plate the same box with
the radius1/4 axial through-hole. All homogeneous surface controls can be
exact dyadics in the delivered evaluator envelope. For one circular quadrant,

`C(s)=((1-s²)/(1+s²),2s/(1+s²))`.

Its degree2 homogeneous Bernstein controls are (1,0,1),(1,1,1),(0,2,2)
for Ax,Ay,W. Radius scaling, quarter-turn signed coordinate permutations and
linear z extrusion preserve dyadics and positive weights1/1/2. This avoids an
irrational sqrt(2) weight approximation. Each quarter is a native(2,1) patch;
planes use(1,1).

Use one common frame `world=origin+scale*body` for every face and curve. The
proposed finite bounded origin and power-of-two scale restriction is a useful
first scope; no arbitrary rotations/affine metrics are needed. Existing
JetBatch.convert handles this uniform frame. Unit-normal direction is unchanged
by positive uniform scale. Frame and chart maps must be retained by prepared
inputs and output records, not supplied anew at result interpretation.

## One periodic face, multiple numerical charts

The wall is one face, with lifted atlas a∈[0,4],v∈[0,1]. Native chart k uses
s=a−k. Source rectangles [k,k+1]×[0,1] express that mapping while native
query UV stays in[0,1]². Internal joins a=1/2/3 are not trim boundaries.
The one seam edge has two distinct coedges on the same wall at a=0 and4.
Keep those atlas locations distinct even though they describe the same spatial
line. A closed rim edge maps a=4t across four geometry pieces.

The parameterization is piecewise smooth: |C′(0)|=2 and |C′(1)|=1. Local
derivatives therefore differ in speed at quarter joins. They are local-chart/
one-sided jets, not derivatives of a globally smooth a coordinate. Normalized
geometric normals agree. Choose endpoint chart ownership deterministically;
retaining duplicate chart-grid samples is valid for this point consumer.

For a planar cap/plate carrier with half-width h, the exact pcurve expression
is `u=(x(t)+h)/(2h),v=(y(t)+h)/(2h)`. Store h,r and the expression, rather than
rounded coefficients. If a rational UV net is useful later, homogeneous
controls `(Ax+hW,Ay+hW,2hW)` retain that expression exactly. At plate h=3/4,
weights become1.5/1.5/3; the disk-cap version can be commonly rescaled to fit
the native positive-weight envelope. This is an option, not another required
GPU interface.

Opposite coedge traversal evaluates canonical t=1−τ. It reverses order without
duplicating or changing the spatial edge identity. Geometric correspondence
is proved over the analytic expressions; rounded floating UV evaluation has
a separate mapping/evaluator allowance. A rounded pcurve UV is not automatically
an exact-boundary witness on the original binary-input planar patch.

## Counts and orientation

| Body | Vertices | Edges | Faces | Loops | Coedges |
| --- | ---: | ---: | ---: | ---: | ---: |
| Box | 8 | 12 | 6 | 6 | 24 |
| Closed cylinder | 2 | 3 | 3 | 3 | 6 |
| One-hole plate | 10 | 15 | 7 | 9 | 30 |

Every edge has two opposed uses; the seam's two uses belong to the same face.
With a CCW spatial rim direction and upward seam direction, outer-wall loop
order is bottom-rim forward, seam at a4 upward, top-rim backward, seam at a0
downward. Top disk rim is forward; bottom disk rim backward. The plate cavity
wall reverses its wall loop and normal. Plate top hole loop is backward and
bottom hole loop forward; rectangle outer loops follow their face orientation.

The cap and seam-cut wall faces of the cylinder are disk cells, giving
V−E+F=2. The plate has two planar faces with a hole. Its cell contribution
is χ(face)=1−holes, so χ(shell)=10−15+(7−2)=0, the genus1 through-hole shell.
Naive V−E+F would give2 and miss both holes. Loop closure uses oriented vertex
occurrences plus chart transitions, not raw UV coordinate equality across
different charts or coordinate welding.

## Consumer semantics and qualification

Exact CPU trim classification applies to submitted binary64 UV, independently
of GPU XYZ. Rectangles use domain/endpoints; disks/holes use the exact rational
sign of x²+y²−r² under the planar affine map. Keep interior, boundary, rejected
and invalid distinct. Plate normalized UV circle radius1/6 has no dyadic exact
boundary points: rounded rim UVs must be classified by their actual bits rather
than assigned boundary labels from intent. The owned symbolic edge still
defines the exact hole boundary.

A prepared grid/gather map retains the exact immutable snapshot, including
frame/chart/patch ordering. A sampler rejects another snapshot even when counts
and slots match. Candidate face/chart labels derive from authoritative chart
records. Upload patches once; gather admitted UVs into one first-jet batch;
download/convert; form oriented normalized cross products; scatter to candidate
order with explicit not-evaluated sentinels for rejected/invalid rows. Failed
native statuses or invalid normal norms fail publication, never become trim
rejection. Output immutable values remain readable after native cleanup.

Direct topology oracles check counts, two-use incidence, closed loops, holes,
seam occurrences and complete analytic edge/pcurve correspondence. Sampling
oracles evaluate original binary UV with exact rational primitive maps and
known outward normals; compare frame/world outputs with declared finite-corpus
grades. Wrong-snapshot same-size inputs, closed-edge endpoint counting,
chart joins, rounded trim UVs and empty selection are concrete fault attacks.
Point clouds are not meshes/chord/embedding certificates. Measure CPU trim/
normal/scatter, first use, uploads, evaluation, allocating provenance download,
conversion, exact qualification and export alongside directly measured caller
wall intervals. No modeling/GPU-topology speedup follows from this operation.

## Retained source basis

- The owned [adapter](../python/brepgpu/__init__.py) exposes frozen Patch/Lineage,
  copied patch/query creation, first-jet vectors and uniform source/world
  conversion. The [quarter-cylinder fixture](../python/brepgpu/fixtures.py#L152)
  already uses the exact rational controls above.
- [Topology architecture](../docs/TOPOLOGY.md) and the
  [pinned source study](2026-10-13-acceleration-roadmap-study.md) separate stable
  identities, slots, face uses and chart/curve correspondence. SMLib Python
  ordered loops/classification are reference seams; they do not supply a
  complete control-net/pcurve importer.
- Pinned OCCT b8f597c677811d1f9f4d8a97f5ae2825c0353a42
  [BRep_CurveOnClosedSurface.hxx](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKBRep/BRep/BRep_CurveOnClosedSurface.hxx)
  explicitly represents two pcurves on one closed surface. Its
  [BRep_TEdge.hxx](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKBRep/BRep/BRep_TEdge.hxx)
  distinguishes SameParameter and SameRange. Both retained headers were read;
  their object/pointer model is not adopted.

Recommendation: proceed with the narrow immutable body/trim/chart consumer;
retain its records for later meshing, arrangements and GPU adjacency. General
import, SSI, history editing and topology migration are not hidden prerequisites.

## Body sampling design disposition

Independently reviewed docs/BODY_SAMPLING_DESIGN.md. Accepted with no remaining
topology blocker after explicit prepared-input snapshot binding, carrier-type
wording, counts/Euler and local-chart derivative clarifications were integrated.
The analytic trim predicate, one-face periodic atlas, closed shared ring edges,
opposed loop uses, common frame and failure/lifetime scope are coherent.
Factory-only construction and existing ABI1 placement fit this narrow unit.
No implementation or qualification result is claimed; ready for plan review.
