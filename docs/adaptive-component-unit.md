# Cached source to complete components and assembly display

2026-10-17. Concrete integration design and dependency plan, accepted in scoped
[geometry](../papers/native-adaptive-component-integration-geometry.md) and
[runtime](../papers/2026-10-17-adaptive-component-runtime-review.md) review.
The operator requests closing the remaining native mesher dependencies and then
returning to the actual watch. This is one source-to-image design; it does not
select a new meshing algorithm or a tuning campaign.

## Grounding and choices

Read the accepted ADAPTIVE_MESHER_DESIGN/PLAN, current selected-face plans,
`adaptive_curves`, `adaptive_trimmed`, `adaptive_surface_cache`, `assembly`,
`contact_epoch`, `mesh_publish`, `scene_view` and `presentation` actual sources.
The pinned NVIDIA SmTess cache/trim functions and OCCT shared-edge/face staging
recorded in papers/native-adaptive-boundary-regions.md ground shared source
ownership, classification and connected band construction. Their floating
stopping/repair policies are not our geometry contract. The existing exact
assembly transport and contact mechanisms are owned, already exercised
components, not adopted libraries.

Source inspection finds the retained movement uses reconstructed domains
throughout. This does not select source-specific dispatch: the declared source
policy selects the existing reconstructed domain contract on any input. General
curved authoritative maps still need the designed B/T extension. Three finite
singular-envelope candidates have a repeated meridian and two distinct chart
occurrences of one original vertex, not a declared collapsed source coedge.
They need exact chart/quotient admission before publication; a carrier kind or
floating hint is not proof of a pole. Preserve all original uses.

Publish the existing complete BGACRV/BGATRM semantic artifacts, retained original
SourceStore and exact AssemblyStore, and adapt those SAME values directly to the
existing display SceneView/BGMESH01 wire. Do not force the old FaceResult /
PreparedRun / GeometryStore pipeline into this operation or fabricate Lawson
lineage for cached packets and band CDT. The newer face artifacts already retain
their actual source roots, packet paths, occurrence bindings and affine facets.
An explicit artifact index names complete definitions/faces and the exact paired
boundary snapshot for each face. Stored XYZ is copied, never resampled by this
adapter. This is a deliberate alternative to duplicating the old semantic
publisher, not permission to omit source topology from the export.

## Whole native operation

1. Decode one immutable original SourceStore. Select either an explicit complete
   definition or the full source assembly. Keep the full original snapshot in
   both cases; selected component output must name its scope and cannot report
   whole-assembly completion. No source-name/ID-dependent geometry policy.
2. Compile only requested curve/surface roots. Build original definition XYZ
   envelopes conservatively from positive rational source/control hulls and
   finite analytic descriptors. Reuse prepare_assembly's exact composed-transform
   norm and storage-error transport to obtain definition positional/node caps.
   Original source units metadata is not another spatial scale.
3. One AdaptiveBoundarySession owns each active definition's physical forest.
   All faces enter a common request horizon. A bounded pool of face owners runs
   faces to complete or needs-boundary, retaining immutable host results or
   requests. Only after ALL current faces have arrived, union/sort/deduplicate
   requested BoundaryKeys and refine the forest. Restart affected faces by their
   actual per-face boundary generations; retain unaffected host results.
4. A pool assignment has its own monotone owner epoch, distinct from the global
   quality query generation. It binds source, definition, face and boundary.
   Session exact-table ordinals cannot reset within the same quality generation.
   Old resident work/views cannot submit after reassignment, while already
   published immutable host result leases remain readable. Clear mutable face
   cache pages at a drained reassignment barrier as needed; immutable source
   roots, workers and device services stay open. A pool of K owners must not
   silently retain mutable caches for all source faces. Admit capacities against
   the actual RunResource; count source uploads and open/close costs.
5. Build the actual component contact input from stable physical keys and facet
   XYZ. Reuse ContactEpochOwner's BVH and exact filtered pair predicates. Source
   edge declarations come from retained BoundaryKeys, internal edges from one
   source-face generation. Coordinate any construction refinement through the
   same cache/band mechanisms; never privately move a shared boundary or accept
   forbidden contact because individual source bounds pass. Typed finite
   unresolved outcomes publish no complete component.
6. After every face/contact settles, export all complete face artifacts with
   their EXACT retained BGACRV snapshot. Deduplicate snapshots by actual epoch;
   do not pair old face row ordinals with the latest boundary pool. The index
   includes original IDs, source digest, quality/policy, snapshot and face paths.
   Full assembly publication requires complete original inventories. Product
   internals remain native typed values; text indices/timings are external
   exports, not an IPC or internal geometry representation.
7. Produce the existing display buffers. Original analytic frames/radii and
   lazy exact extraction of each USED original rational root provide source
   normals; reuse surface_detail::original_net_exact, not restricted numerical
   leaves or all-source eager geometry compilation. Copy actual face XYZ and
   orient facets/normals using original face orientation. Boundary PhysicalNodeKey
   determines physical identity, never coordinate equality. Distinct chart seam
   corners may share one physical node. Reuse existing assembly/material/BOM,
   presentation, CPU/CUDA display and image writers.
8. Measure decode/compilation/envelopes/assembly, boundary, face, contacts,
   publication/display preparation, device work, transfers, output and close.
   Do not add overlapping scopes into a fictitious total. Cold process walls
   and original-source reader/foreign reference walls remain distinct.

## Finite execution order

First close the three already selected full-caller family ports and caller
refusals; their plans define the finite checks. In parallel implement the pooled
assignment invariant below after review. Then implement the complete definition
coordinator, original assembly allowance producer, direct display adapter and
the explicit source-vertex wedge capability if exact admission requires it.

Before product implementation of each unit: actual-source grounded subplan,
direct RED, review. Actual-source adversarial code review and affected tests
follow implementation. Use authored adjacent faces with one shared edge, a
closed box, a curved seam shell, reused transformed definitions and mismatched
source/unsupported domains as independent component families. The readers must
check original source incidence and actual affine geometry, then match display
XYZ/UV/source/placement to those SAME artifacts. Existing source/geometry and
exact-chain readers are extended, not wrapped in a second certifier.

Run a complete imported component on CPU and P100 with original-source checks
and unchanged quality before a whole movement trial. Retain failures and costs.
Then run the complete movement, assembled and exploded labels, CPU/P100 plus
the actual NVIDIA CPU reference. Do not omit failed definitions, produce a
partial success, resume the old Python mesher, or silently loosen quality.
Stop qualification once the relevant claims have been answered and return to
delivery. External alpha suites and post-watch README reference backfill remain.

## First implementation unit: reusable face assignments

The current FaceTrimSession.begin requires a strictly larger quality generation;
SurfaceCacheSession permits same-quality replacement only for its SAME face.
These prevent a bounded worker/device owner from handling several faces in one
definition query. Per-face device setup and one permanently live owner per face
are both rejected as defaults.

Select an explicit monotonically increasing assignment epoch for pooled use,
while preserving the default single-face API's existing query progression.
The actual boundary snapshot still names the global quality generation. Exact
table keys retain owner/session identity and unique ordinals across assignments;
surface candidate/view/facet work additionally binds the current assignment.
Validate the entire candidate assignment before mutation. A failed preflight
must leave an idle owner usable. A successful reassignment requires all resident
operations released/drained and cannot lower global quality/source generations.
   Same-face boundary continuation remains its existing strictly advancing path.

Expose only the small native API needed for assignment/retirement. Keep point,
node and control storage bounded: on a drained different-face assignment, retire
prior mutable query pages and retain only reusable immutable source/service
state, or reuse the same carrier pages within a measured explicit bound.
Old host result leases retain copied values; no mutable cache offset is their
semantic identity. Device bank/stream lifetime survives page retirement.

Direct fixture: two different original faces in ONE quality generation through
ONE owner, then a later quality; keep the first host result across reassignment
and close. Demand real complete faces from the existing source/chain oracle.
Submit an old view/request after reassignment and require precise refusal. Try
invalid/nonmonotone assignment and prove the same owner can still begin valid
work. Exercise more distinct assignments than configured resident-page capacity
would otherwise retain; admission must remain bounded. CPU1/parallel CPU and
P100 use the same identities. Scoped memory/device checks target this NEW page
retirement and lifetime behavior, not a rerun of all numerical qualification.
