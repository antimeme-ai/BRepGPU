# Original geometry through complete cached components

2026-10-17. Read-only source-to-component study under BLACKBIRD. No product
implementation, meshing run, dependency adoption or new qualification campaign
was performed for this study. The aggregate inventory below comes from decoding
the retained original BGSRC001 snapshot on the fleet, not a geometry callback.
Private source bytes, identifying part names, coordinates and machine paths stay
in ignored context.

## Selected integration

The current cached surface and shared-curve mechanisms can be joined without
returning to the old mesher. Keep one original SourceStore, one physical boundary
authority per definition, bounded reusable face owners, the exact AssemblyStore,
and the SAME BGACRV/BGATRM results produced by those owners. An explicit component
artifact index pairs every complete face with its exact boundary snapshot and
original source identity. Adapt those values into the existing SceneView and
BGMESH01 display wire. Stored XYZ remains the geometry rendered.

The old NativeMesh publisher is not the shortest honest adapter: FaceResult and
FaceBoundarySnapshot retain GeometryStore, mesh_publish requires PreparedRun,
and old lineage enums describe operations that cached packets/band CDT did not
perform. Faking old Lawson lineage would lose rather than preserve semantics.
An artifact index linking existing complete semantic files is an external export,
not a new internal geometry model or replacement mesh wire. Publication must name
whether it covers one selected complete definition or the whole original assembly.

The reviewed design is [adaptive-component-unit.md](../docs/adaptive-component-unit.md).
Its pooled-assignment unit is accepted in scope. There is no numerical reason to
require an eager GeometryStore merely to obtain a renderer handle.

## Reference mechanisms and their limits

The exact acquired revisions and restoration records remain in QUARANTINE.md.
The following functions were read, rather than inferred from product descriptions.

| Reference | Actual mechanism used in this design |
| --- | --- |
| NVIDIA USD/BRep `c9c5979f04cea8c38213004d4f04df0dcfb40d05`, `source/SMLib/src/SmTess.cpp`, `TessellateEdge` around 3009–3066 and `Phase1SetupBrep` around 11118–11866 | Tessellate shared physical edges before creating face polygons. Edge and face ownership are separate stages. |
| Same file, `Phase2CreateFacePolygons`, 12023–12131; `AddQuadBoundaries` and `SmTessSrfCache::ImplantUVSegment`, readings recorded in native-adaptive-boundary-regions.md | Classify cached interior polygons against occurrence-specific UV trims, then construct remaining boundary regions. A face-only sampler does not replace shared edge scheduling. |
| OCCT 8.0.1 `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`, `BRepMesh_EdgeDiscret.cxx`, `process`, `Tessellate3d` 225–280, `Tessellate2d` 283 onward | Keep original endpoint vertex positions, obtain one 3D parameter chain, and populate every pcurve occurrence using its parameter provider. |
| Same revision, `BRepMesh_ModelPreProcessor.cxx`, `SeamEdgeAmplifier` 106–250 and `performInternal` 285 onward | A seam has distinct forward/reverse pcurve occurrences. Updating the 3D edge updates all pcurves and invalidates incident faces. |
| Same revision, `BRep_Tool.cxx`, `UVPoints` around 1060–1135 | Closed-surface edge orientation selects the appropriate pcurve endpoint pair; a single edge ID does not identify one UV occurrence. |

These references motivate ownership and staging. Their tolerance expansions,
sampling stops, minimum sizes and repair policies do not establish our original
source bounds. Existing cached mesher reports retain the applicable numerical
derivations: Filip/Magedson/Markot 1986 §2, Theorems 2/4 (with the Theorem 5
limitation); Balázs/Guthe/Klein 2004 §§5.2–7; Ma/Hewitt 2002 §§4.1–4.4. The current
actual-facet service is stronger than sampled flatness: it bounds the SAME emitted
affine facet against original source across leaf/span clips.

## Actual original-source inventory

The retained imported snapshot contains 107 definitions, 9,162 faces, 9,768 loops,
25,871 edges, 51,742 coedges, 17,129 vertices, 77,613 curves and 196 usage rows.
Definition face counts range from 4 to 1,112. Every coedge is marked reconstructed,
has parameter-map kind 1 and zero stored periodic shift. There are no declared
degenerate edges, zero spatial parameter intervals or missing spatial curves.
There are 608 distinct edges used twice on the SAME face. Their two chart
occurrences must survive even when their physical samples are shared.

| Carrier | Count |
| --- | ---: |
| Plane | 3,688 |
| Cylinder | 4,431 |
| Cone | 341 |
| Sphere | 18 |
| Torus | 49 |
| Rational surface | 635 |

The rational carrier degrees are 624 at (2,1), one at (3,1), and ten at (3,3).
The actual rational curves have degrees 1,2,3 or 5. This inventory does not narrow
the admitted general curve contract, which remains degree at most 32.

All reconstructed is meaningful: the existing selected-PL domain policy applies
to this source without claiming the original raw guide is authoritative. It does
not justify silently admitting curved authoritative sources under identity.
Mixed/curved authoritative coverage remains a generic capability and authored
fixture obligation, independent of whether this snapshot exercises it.

Floating metadata inspection identifies broad angular boxes for 364 cylinders,
208 cones, 11 spheres and 25 tori near or beyond a full turn. Existing trim cache
panels address the regular angular extent; these counts are not admission results.
Seventeen sphere boxes appear strictly away from the poles; one is near a pole.
Two cone boxes cross/touch the radius-zero locus. These three singular-envelope
candidates require original-source interpretation, not a carrier-kind shortcut.
All 49 tori have a nonzero minor radius and major radius larger in magnitude.
The floating checks are diagnostics only; exact/enclosed production admission
still decides regularity, branch ownership and source allowances.

## The three singular candidates are source-vertex seam wedges

Each candidate has one loop of THREE original uses and TWO original vertices.
A meridian edge appears out and back, and a closed regular-side edge connects its
other endpoint to itself. The two cone candidates have two line occurrences and
one circle; the sphere candidate has three circle occurrences, including the
repeated meridian. There is no original collapsed coedge to name as quotient_use.

This is consistent with the existing importer, not permission to mutate its
inventory. In `native/source_uv/source_uv.cpp::query_poles` around line 218, the
copied OCCT QUERY shape receives a temporary degenerate connector between the
incoming/outgoing chart branches at one original vertex. `require_source_uv_uses`
around 95–111 checks the original neighboring uses and vertex ownership. That
query connector is separate from the original physical source.

The native capability therefore needs two explicit quotient anchors:
`declared_collapsed_use` and `shared_vertex_seam_wedge`. The latter retains the
original vertex, both adjacent endpoint occurrences and their distinct UV branches;
its symbolic chart connector is construction/coverage data, never an invented
source use. The same physical meridian chain supplies both seam branches and both
cut endpoints. Distinct chart sites may have the same PhysicalNodeKey and XYZ.

Reuse the reviewed analytic cap mathematics, not old face preparation: source
finite-to-ideal pole correspondence must cover the WHOLE rho band, with
`(1-rho)*|v_finite-v_pole|*sup ||S_v||` over the convex hull of finite pole, ideal
pole and both cuts. Add actual corner departures and the fan interpolation bound.
The original vertex need not equal the ideal analytic pole; its fixed departure
cannot be removed by shrinking private cuts or spending node_source allowance.
Binary64 pi/2 is not an exact analytic pole identity.

Radial cuts remain physical source-edge requests. Private angular cuts belong to
the face. A replacement cut requires progress on BOTH meridian endpoint gaps;
one source epoch may split only one endpoint, so retain the old cut/history while
requesting the other. Strict interior cuts avoid inserting private nodes on an
opposite original source edge. An admitted full-face empty remainder would instead
need actual opposite master-chain nodes and source BoundaryKeys.

Current trace source admission still rejects singular cone/sphere boxes. A future
cap service must separate original evaluator availability from regular prepared
surface panels; relabeling a trace-only cache as a regular patch is invalid.
Whether each candidate needs a cap rather than a smaller proved regular source
envelope is decided from original use/guide bounds and original guards, not hints.

## One definition horizon and a bounded face pool

`AdaptiveBoundarySession` already owns the definition forest and supports
`refine(expected_snapshot, keys)` after a complete consumer horizon. The missing
component coordinator must run ALL current faces to complete or first boundary
request, union/sort/deduplicate those requests, and then mutate that forest once.
Every affected face restarts from its new actual boundary generation. Unaffected
complete host results may remain. Old `meshing_run.cpp::boundary_horizon` around
473–536 is the owned protocol reference, not a reason to instantiate PreparedRun.

Current `FaceTrimSession::begin` requires a larger quality generation and resets
its table ordinal. `SurfaceCacheSession::begin` permits same-quality replacement
only for the same face with advancing face/boundary generations. Consequently a
pool cannot currently assign two faces in one quality query. Faking a new quality
for every face contradicts the shared boundary snapshot.

Add a monotone assignment epoch distinct from global quality. Exact-table identity
and work/view identity include the session/assignment; ordinals cannot alias live
tables within one quality generation. Validate a candidate assignment before
mutation. Reassignment drains operations and retires old mutable point/control/
node pages. A pool of K owners cannot retain mutable pages for every processed
face while claiming bounded residency. Immutable source pages, workers, device
services and published host result leases can remain.

An unrelated forest split can change global row ordinals while a retained face is
still valid. Pair each BGATRM with its EXACT captured BGACRV snapshot, deduplicated
by actual epoch. Alternatively remap into a final definition snapshot using stable
physical keys and original use/t; never reinterpret old indices in a new array.

Complete components also need the existing exact contact owner: local source
approximation bounds do not exclude forbidden inter-face intersections. Build its
inputs from stored facet XYZ and declared physical source adjacency, not coordinate
welding. A contact request must use the same face/cache or shared-boundary owner.
Explicit unresolved publishes no complete component.

## Original envelopes, assembly allowances and exact extraction

Reuse `prepare_assembly`, whose `DefinitionEnvelopeView` is already a borrowed
original-source bbox. Its exact composed transforms, conservative norm factor
kappa, stored-transform error eta and minimum per-definition allowances are the
right transport. Do not scale coordinates again by original units metadata.
The nodal contract remains original V/Ce versus original S(chosenUV), plus original
V–Ce endpoint guards. Constructed Q departure is positional error, not free delta.

Produce B0 from ORIGINAL vertices, spatial curves and complete finite source
carrier domains. Positive rational homogeneous controls give a conservative XYZ
convex hull only after division by each actual positive weight. Analytic bounds
use literal original frame columns/radii and finite nonperiodic coordinates;
global angular bounds are often stronger operationally than exact trig extrema
for this compile-once purpose. Nonorthogonal frames remain legal. Include original
pre-cap source and any admitted ideal-cap/output extent needed by transport.
Display uv_bounds hints are not rational natural-domain authority. Finite analytic
domains must include the original occurrence/guide envelope used by preparation;
expanding that envelope after assembly transport requires recomputing allowances.

Three extraction alternatives were compared. Eager `compile_geometry` is correct
owned original extraction, but preparing all source curve roots solely for display
duplicates the new cache and the old measured whole-source preparation cost was
material. A separate newly implemented exact evaluator would duplicate source
truth. Select the existing owned exact extraction functions with requested,
immutable memoized pages: `surface_detail::original_net_exact` reconstructs an
original rational span lazily from SourceStore and existing extraction. Reuse
the corresponding curve services for exact values/hulls/line images. This is a
cache of the same original algebra, not old meshing callbacks. A narrowed eager
requested-closure extraction remains a reasonable alternative if measurement
shows it cheaper; no subset timing is claimed by this study.

Curved authoritative domains require the actual reviewed B/T mechanism adapted
to these common source services. Keep ambient joint fans and monotone tapered
strips, source descriptors, support polygons and inverse displacement queries.
For each emitted facet K, query inverse T then B supports, bound original carrier
transport over the indexed endpoint hull and spend `E_domain(K)` exactly once
before bounding original S against the SAME emitted affine L. A boundary maximum
is not an interior domain map. The old `geometry_domain` algebra is worth adapting;
its GeometryStore/WorkingChart container dependencies are not mathematical
requirements. This extension must not block meaningful reconstructed integration,
but whole-source support claims must state the exact admitted policy.

## Display and one direct source-to-image oracle

Extract common owned SceneView buffer storage from the old NativeScene wrapper.
The new builder retains original source, AssemblyStore and component result leases.
Copy actual Q into display vertices. For analytic normals copy original frames and
radii; for rational normals memoize exact USED original roots and perform the
existing checked binary64 display conversion. Restricted numerical leaf controls
are not source-normal metadata. At C0 knots choose the admitted corner/one-sided
root policy explicitly; the canonical source point owner alone does not establish
a unique normal. Pole north/south or radial-sheet normals need their actual cap
descriptor. Apply original face orientation once and instance cofactor once.

Reuse the existing original-output geometry/edge-chain reader. Extend it for full
component source inventory and occurrence incidence, then compare SceneView's
stored positions, original normal roots and exact placements to those SAME
semantic artifacts. Do not add an independent second geometry certifier around
the current one. Finite authored fault classes are:

* Two adjacent curved faces, with a late request from the last processed face;
  an earlier complete incident face must restart while an unrelated face stays.
* More same-quality face assignments than resident pool capacity, stale view
  refusal, retained first host result after owner close and out-of-order jobs.
* A closed box, periodic cylindrical seam, rational C0 face and nonorthogonal
  analytic frame; distinct UV seam occurrences share identical physical Q.
* Both explicit collapsed-edge caps and implicit three-use seam wedges, with a
  finite source-to-ideal offset and an irreducible fixed-Q floor.
* Mixed authoritative/reconstructed bowed trims: identity refusal until an actual
  B/T lease supplies the interior map and its SAME-facet allowance charge.
* Reused definitions under shear/nonuniform scale and nested transforms; original
  world tolerances include kappa/eta, assembly/source units are not applied twice.

Then execute one complete imported definition with unchanged quality on CPU and
P100, followed by the whole source assembly and existing assembled/exploded
renderer views. The NVIDIA reference uses its original-source input/export path;
its timings and images remain comparison results, not production callbacks.
Measure decode, source compilation/envelopes/assembly, boundary, face/contact,
publication, transfers, render and end-to-end walls separately. Neither a fixed
triangle count nor transactions per second defines this geometry performance
target. The first complete outcome must expose time, memory, refinement and
remaining typed refusals without hiding failed definitions.

## Review disposition

The root component design and first pooled-assignment unit are accepted in scope.
The exact paired snapshot export fixes the concrete stale-index hazard. The next
geometry unit should own original definition envelopes and AssemblyStore transport
with the finite source profile above. It should not also take on B/T maps, a pole
atlas or renderer storage. Those have coherent independent owners and depend on
the same retained source/allowance interfaces.
