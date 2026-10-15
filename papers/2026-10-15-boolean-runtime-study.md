# Box-minus-through-cylinder: runtime and owned representation

2026-10-15. Source-only study of the roadmap, immutable body tables, exact SSI/
arrangements/classification, sampler, adjacency and mesher. No implementation,
dependency, build or product execution. Recommendation: construct one actual CPU
Boolean result with source history and a narrow sampling reader through existing
native patches; keep canonical contracts intact.

## Concrete family and input admission

Accept actual factory BodySnapshot operands of shapes box and cylinder, common
Z axis and initially identical XY center. Treat original float fields as exact
ratios before every geometric decision. Cutter lower Z must be strictly below box
lower Z and upper Z strictly above box upper Z. Radius is strictly inside all
four box sides. Reject tangent, coplanar-ended, partial-depth, oblique and invalid
families explicitly, with no fallback canonical fixture.

Current box has half-width 3*sB/4 and height sB; cylinder has radius sC/4 and
height sC. Strict containment/throughness imply sB<sC<3*sB. Existing Frame scales
are dyadic, so the admitted strict family necessarily has **sC=2*sB**, subject to
their [.25,4] bounds. The resulting hole radius is sB/2, not the existing plate's
sB/4. Cutter center Z can vary within |zC-zB|<sB/2. This is a useful direct case
that cannot be replaced by make_body('plate'). Off-center XY is geometrically
available to existing sections/arrangements, but a broader first family needs
explicit emitted-control rounding budgets; do not smuggle that change in through
a generalized shape label. A bounded off-center design is also possible if it
owns those numerical rules rather than relying on canonical sampler assumptions.

Construct the two actual cap/wall sections using original FaceRef inputs. Retain
their exact events, four source chart pieces and both UV maps. Build each cap's
actual arrangement and select rectangle-minus-open-disk material. Assemble box
side faces and the reversed, Z-clipped cutter wall from those selected boundaries.
One physical circle per cap has one anchor and two opposite face uses; four
source chart pieces do not become four feature edges. A wall seam has two uses
on the same wall. Result topology is plate-isomorphic, but its computed geometry,
identities and history come from these operands and operation intermediates.

## Preserve existing canonical type boundaries

Existing BodySnapshot is factory-only, with shape tags box/cylinder/plate and
BGbody01 packed header. The following consumers encode those exact families:

| Consumer | Existing admission/assumption | First Boolean integration |
| --- | --- | --- |
| BodySampler / SampleRequest | isinstance(BodySnapshot), snapshot.classify and face orientation | Separate result request/sampling adapter with exact result trims; existing Engine/Patch batches |
| Material classify_point | exact BodySnapshot type, fixed radius/width constraints | Result-owned exact membership with every supporting result face |
| FaceRef / SSI / arrangements | exact canonical BodySnapshot inputs | Use original operand FaceRefs for construction; do not coerce result back into inputs |
| Mesher | canonical type/shape, centered rings and fixed UV maps | Explicitly unsupported on new result until a complete result meshing unit is selected |
| pack_bodies | BodySnapshot references retained for source resolution | Build existing public TopologyInput directly, keeping result separately |
| Packed body header | fixed shape enums and canonical geometry interpretation | Separate result serialization identifier; never emit canonical plate tag |

Use a **separate factory-only immutable result body**, not a BodySnapshot
subclass: subclassing would accidentally satisfy permissive sampling/meshing
guards while violating their fixed ratios. Reuse typed Vertex/Edge/Coedge/Loop/
Face/Chart records where their meanings agree. Existing Curve('circle',(r,z))
and circle_plane(r,half_width) maps can represent the centered unequal radius;
it is classify/meshing admission, not those data values alone, that is canonical.
Keep every result-specific interpretation in the owned operation module/adapter.
Do not globally generalize current body APIs as a prerequisite.

Result has fresh shared body._namespace identity, revision 1 and slot+1 typed
entity IDs. For centered strict-through construction the expected counts are
10 vertices,15 edges,30 coedges,9 loops,7 faces,10 charts, one outward shell.
Those counts alone do not prove topology: check each shared boundary, ordered
loop, two-use edge incidence, opposite directions, seam multiplicity, face
orientation and closure. Hole loops mean ordinary cell Euler V-E+F needs the
appropriate face Euler contributions; do not infer genus from a naïve seven-disk
face assumption. No coordinate welding or borrowed object pointers define IDs.

## Geometry, source maps and actual consumers

For the coaxial family, reuse the six actual box plane carriers and construct
four exact rational quarter-wall carriers in the box frame at radius 1/2 and
z=±1/2. Weight pattern remains 1,1,2; native envelope fits without a new ABI or
kernel. Give every native patch the **new result** face/chart Lineage and retain
its original source mapping separately. Wall longitudinal source mapping is an
explicit affine interval into the original cutter's parameter; reversal is a
face-orientation property, not a silent flip of intrinsic du/dv.

Result membership is exact: inside all six closed box constraints and outside
the open cutter radial interior. A point on a hole cap rim belongs to both cap
and inward wall; return every incident result face. Trim classification on cap
charts uses the actual emitted affine map and actual radius; wall charts have
the cut body's Z extent. All validation/query values bind to the exact result
snapshot and source request, not a geometry-equal replacement.

A minimal complete consumer can perform bounded result trim admission, gather
actual native slots/UV, evaluate first jets on CPU/P100, download and restore
world coordinates, orient normals by result faces, scatter explicit rejected/
invalid rows, and consume complete position/normal/source rows. Keep required
status and returned descriptor/query/Lineage checks in its useful wall. A result
graph reader should traverse loops and read all physical shared boundary mappings,
not merely checksum counts. Existing TopologyInput accepts explicit typed Source
tables with snapshots=(); retain the actual result in the caller and resolve its
identity there. This reuses synchronous ABI5 if adjacency is part of the selected
caller; no pack_bodies widening or native binary change is required.

## History, ownership and failure

Publish operation result, retained operands, exact sections/arrangements and
typed source→output relations together. Distinguish retained geometry, modified
trimmed descendants, generated intersection entities and removed input entities.
Box top/bottom faces become modified hole-cap faces; four box sides retain their
geometric/trim meaning. Cutter wall becomes the clipped inward wall; cutter caps
and their original rim edges do not survive. New cap rings are generated from
both incident input faces. The cutter seam's clipped segment has an explicit
original edge parameter interval; it is not a fabricated intersection feature.
Every output needs a source relation, with intentional anchor/chart artifacts
identified as such. An input can generate entities and be removed, whereas
“removed” must not also claim a surviving modified descendant. OCCT's public
history model is a useful reference for these distinctions, not a chosen library
or a promise of its exact naming conventions.

Operands and intermediates are immutable; construct mutable local arrays only
inside the operation and publish one recursively immutable result after all
checks. Unsupported admission, geometric contradiction, failed material/loop
selection, allocation or native consumer error must not mutate operands or
publish partial solids/history. Fresh namespace allocation may leave a gap after
failure; no identity is ever reused. A consumer failure preserves the already
valid CPU result and closes every native child without masking the primary error.
No GPU is needed for construction or exact decisions; CPU placement is valid.

Measure operand preparation separately, then actual admission, sections,
arrangements/material selection, carrier/topology/history assembly, packing and
immutable publication. Complete useful costs additionally include bounded request
preparation, context, uploads, evaluation, download, frame conversion, normals,
graph/history reading and cleanup. Independent material/boundary/topology/history
qualification and SVG/export have explicit outside timers. Distinguish cold from
retained Engine/patch scopes if both are measured; no retained session optimization
or speedup assumption belongs in this breadth unit. An actual result visual may
show native samples/trim boundaries, without claiming an unimplemented general
mesher. Applicable external Boolean qualification remains an alpha requirement.

References: [OCCT Boolean specification](https://occt3d.com/dev/doc/overview/html/specification__boolean_operations.html),
[OCCT history reference](https://occt3d.com/dev/doc/refman/html/class_b_rep_tools___history.html)
and pinned implementation sources in `boolean-runtime-sources.tsv` and
`boolean-runtime.bib`. Existing owned code supplies the runnable integration
constraints; no optional reference runtime probe is needed to establish them.

## Selected design supersedes the carrier construction option

The reviewed `docs/BOOLEAN_DESIGN.md` selects identical XYZ origins and sC=2*sB,
with **unchanged actual cutter wall control/UV/frame bits**, new output Lineage,
and retained original wall V band [.25,.75]. This supersedes the earlier optional
reconstructed box-frame wall recommendation above. No carrier clipping/refitting
or local-coordinate rounding is needed; per-patch frames remain explicit, wall
derivatives retain original cutter parameters, and the new result sampler owns
the trim-band request. Existing canonical consumers remain unchanged.
