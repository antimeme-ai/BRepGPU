# Conservative carrier-pair discovery: semantics and identity

Proposed beachhead,2026-10-14. Start with immutable indices of the existing
canonical body charts, conservative common-world boxes and a paginated packed
overlap bitset. It supplies possible surface pairs to a later intersection
consumer; it does not compute intersections, contacts, overlap regions or
Boolean topology. No product code/native execution accompanied this study.

## What each bit means

A one bit means the two closed conservative AABBs overlap, so the carrier pair
survives broad phase. It is not an intersection witness. A zero bit certifies
disjoint entire admitted untrimmed carrier rectangles, hence also disjoint
trimmed subsets. Do not tighten boxes from a finite mesh or sampled trim
membership. Extra candidates caused by holes, tilted planes, chart overlap or
loose control hulls are correct. Closed comparisons must retain equality:
exclude only when one upper endpoint is strictly below the other's lower on
some axis. Zero-width plane/curve/point boxes are valid, not empty volumes.

Keep all Cartesian pairs, including self/adjacent chart pairs if supplied.
Same body/face IDs or shared edges do not justify an undocumented bit clear.
A cylindrical face has four numerical charts but one face identity; output
must preserve both facts. To derive a face-level exclusion, every admitted
chart-pair combination for that face pair must be covered and cleared. A page
or a selected source subset cannot establish whole-body exclusion.

## Common-world enclosure is a prerequisite

For positive homogeneous weights, the rational rectangle lies in the Cartesian
control hull. Local root bounds follow from enclosing every exact Ak/W.
Transform those bounds under the actual common frame, not rounded evaluated
points or independently normalized local coordinates. For positive scale σ,

`world_lo = down(origin + σ*local_lo)`

`world_hi = up(origin + σ*local_hi)`.

Every primitive arithmetic operation needs its enclosure rule; one final ULP
does not account for arbitrary earlier rounding. Current bodies admit bounded
finite origins and power-of-two positive scales; that restriction is sufficient
for this first operation. Retain the frame as source metadata and qualify the
world endpoints directly against exact binary-rational control ratios/frame
maps. Do not clip transformed boxes to the local[-1,1] envelope. Lifted source
UV rectangles such as[3,4] describe charts; native carrier domains remain the
full local[0,1]² and must not be accidentally cropped.

Reuse existing qualified local bounding machinery where it simplifies the
implementation, but avoid uploading a deep hierarchy for a root-box Cartesian
test. Downloadable actual world boxes make direct enclosure qualification
possible. Index construction refuses invalid/unsupported sources or arithmetic
failure transactionally; never omit them and publish an apparently complete
index. Generic Patch input is optional later scope: existing patch creation
alone does not replace numeric degree/weight/control/frame admission for an
all-domain bound. Canonical BodySnapshot input keeps the first unit bounded.

## Page coverage and deterministic identity

Let left/right counts be NL/NR and total=NL*NR, admitted with checked arithmetic.
Pair ordinal q has i=q//NR,j=q%NR. A page beginning at start covers
`[start,start+count)`, where count=min(max_pairs,total-start). Stored local bit k
belongs to q=start+k. Define one byte/word's bit order explicitly; initialize
unused tail bits to zero and do not interpret padding as processed pairs.

Return immutable left/right index identities, total, covered start/count,
next_start and unresolved remaining suffix with the bytes. start>0 means
the earlier[0,start) interval is outside this receipt; reaching total from a
nonzero start is not proof of whole-product coverage. A caller can establish
complete coverage only from matching-source nonoverlapping pages whose union
is[0,total). Zero max_pairs may return empty coverage with the same cursor;
document this or reject it explicitly. Empty indices require no division by0.
All count/offset/byte extents are checked before allocation or publication.

An unprocessed suffix is not encoded as disjoint zero bits. Buffer size is
ceil(count/8), computed without overflowing count+7. One worker owns each
packed byte/word, or an equivalent explicit ballot scheme; no shared non-atomic
bit writes, atomic append capacity, arbitrary truncation or sorting-dependent
survivor order is required. This page contract avoids claiming completeness
from a candidate queue that filled before traversal finished.

Source records retain `(body_id,revision,entity_kind,face_id,chart_id)`, source
patch slot and common frame. Dense index slots additionally belong to one
immutable index version. Pages retain both indices and reject accidental
replacement by equal-sized sources. If repeated source occurrences are allowed,
their index occurrence slots remain distinct; do not deduplicate by coordinates.
Row-major operand order is the query convention, not a future spatial-event
identity: SSI must separately normalize its branch/event provenance when sharing
an edge across opposed uses.

## Useful scope and direct checks

The flat Cartesian baseline is O(NL*NR), not a BVH claim. A single box/cylinder
pair has only a few dozen chart comparisons and will likely expose overhead;
publish that crossover honestly. Material first use is a multi-part scene or
face shortlist that preserves source IDs and excludes distant carriers before
narrow-phase work. Use many independently posed canonical bodies with nearby,
far, touching and coincident configurations. Clearly report repeated geometry,
scene size, preparation/index upload, page download/decoding and reuse costs.
This is useful scheduling data without claiming exact collision or SSI savings
before an actual downstream operation consumes it.

Direct qualification should check every actual emitted box against exact
original controls/world maps, then each covered bit against the closed-box rule.
Independent analytic intersection fixtures additionally attack false exclusions.
Required cases include distinct local frames with identical world geometry,
large translations and different scales, exact face/edge/point touching,
nextafter gaps, planes/singletons, holes that cause false positives, four-chart
seams, swapped operands, duplicate source occurrences if admitted, foreign
same-size indices, empty products and pages crossing row/byte/word boundaries.
Adjacent pages must reproduce the full output without gaps/duplicates; padding
and capped coverage must remain explicit. CPU parity and rendered boxes do not
certify enclosure or completeness.

## Retained references and recommendation

- [Bounds oracle study](2026-10-14-bounds-oracle-study.md) derives positive-weight
  control-hull containment and exclusion/coverage rules; it identifies source
  pitfalls involving sampled bounds and zero-width boxes.
- [Intersection/topology study](2026-10-13-gpu-topology.md) separates conservative
  pair discovery from complete branch discovery, paired UV/spatial curves and
  shared-event topology. Mesh resolution does not establish curved completeness.
- Pinned OCCT b8f597c677811d1f9f4d8a97f5ae2825c0353a42
  [BOPDS_Iterator.cxx](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingAlgorithms/TKBO/BOPDS/BOPDS_Iterator.cxx)
  selects bounding-box pairs, then performs source/type filtering and stable
  ordering. Its [Bnd_Box.cxx](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/FoundationClasses/TKMath/Bnd/Bnd_Box.cxx)
  distinguishes box separation and transformed boxes. These retained source
  portions were read; their tolerance/arithmetic and object format are not
  automatically our certificate or an adopted dependency.
- [Body topology study](2026-10-14-body-topology-study.md) and
  [current body source](../python/brepgpu/body.py) provide explicit immutable
  graph/chart/frame provenance for a narrow first input path.

Proceed with the paginated carrier overlap service and a scene shortlist caller.
Defer BVHs, tighter bounds, trim-based culling, generic import and SSI to their
own beachheads/return pass. Whole-index admission, common-world enclosure and
explicit page coverage are necessary first-pass semantics, not optional
generalized robustness.
