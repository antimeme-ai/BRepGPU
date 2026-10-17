# Adaptive component display adapter

2026-10-17. Parent selected the direct actual-component-to-SceneView connection.
Read docs/adaptive-component-unit.md and current ComponentResult/FaceTrimResult,
AssemblyStore, scene_view, presentation, source evaluator/display flags and
lazy original rational source-net extraction. No production code written.

Saved DESIGN and finite subplan docs/adaptive-scene-unit.md for root and numerical
peer review. Chosen seams: shared packed SceneBuffers, retained exact original
UV/source result provenance, bitwise actual XYZ copy, lazy USED original rational
span extraction, incident one-sided source-normal root at C0 knots, original
assembly/material semantics and SceneView presentation overload. Existing CPU/
CUDA renderer and image writer remain the display implementation. Root owns the
whole-source caller and publication. Asked root for a shared component_site_key
helper so contact/display use the same physical identity rule.

Component oracle driver ownership passed to runtime peer for narrow actual CLI
invocation/refusal integration; its source builders/readers remain unchanged.
No new native runs, geometry or dependencies in this design step.

## Implementation and finite direct closure

Parent accepted the DESIGN/subplan; numerical peer accepted the exact original
span/one-sided facet sector convention. Before the public header/body, the real
source→AdaptiveAssembly→ComponentMesher→make_adaptive_scene fixture failed to
compile because adaptive_scene.hpp was absent. Fleet log:
context/adaptive-scene/missing-operation-red.log. No stub producer.

Implemented shared SceneBuffers, with old NativeSceneData retaining its same
mesh/geometry fields and forwarding view storage. Added the borrowed SceneView
presentation overload; the existing NativeScene overload forwards. Fixed the
initial compile mismatch BorrowedRecords.empty() to its actual size() API.
AdaptiveScene retains exact UV refs/result leases and copies actual Q words;
root's shared ComponentSiteSlot helper supplies physical identity. Original
analytic descriptors and USED original exact rational root nets feed cosmetic
source normals. At knot corners, indexed neighboring original spans and exact
positive intersection with the SAME actual facet select a real one-sided sector.
Normal failures belong to actual CPU/P100 source_evaluate and abort complete
caller publication; preparation deliberately does not duplicate that evaluation.

Independent reader tests/native_adaptive_scene.py reuses the existing complete
face/source-curve oracle, then matches display facet roles, stored XYZ, original
UV normal locators, source controls, physical IDs, source-normal directions,
original assembly matrices and appearance buffers. Native fixture additionally
checks exact retained UV refs and bitwise Q/UV copies through provenance. CPU and
both actual P100 display consumers emit their actual jets/projected placements;
authored analytic/weighted/C0 derivatives and exact affine placement/cofactors
supply the expected results, not backend equality.

The proposed strong shear/reflection styled source was refused BEFORE native
geometry by the existing Python source writer and native source.cpp655–663
proper near-rigid Usage contract. Parent approved replacing that input with a
proper quarter-turn+translation, distinct occurrence styles and an explicit face
style. Source admission was not widened. Existing direct renderer affine checks
retain their separate programmatic-input scope. A reader list/tuple degree
comparison typo was corrected against the saved weighted output; no native
weighted query was repeated for it.

All five fixed profiles passed CPU and both-P100 display consumer readers:
box12 facets, cylinder64, weighted76, C0400 and styled reused box12/two instances.
C0 retains two original normal sides at a shared exact knot corner; crossing
facets preserve their original actual affine geometry. An additional CPU box
query and the P1000 box query emitted real assembled/exploded 320x320 PNGs through
the existing rasterizer/PNG writer. Local preview of that actual CPU image shows
the expected box. Meshing in this adapter fixture stays the already qualified CPU
operation; this cohort establishes the native display placement, not new GPU
meshing or a speed claim. All GPU work finished and devices were released.

The new old-path compatibility binary read an existing saved a.bgnmesh and passed
the old native source-scene test. ASan/UBSan/leak runs passed only C0 and styled
representative new-buffer/root/provenance lifetimes. Selected native tidy passed
all four affected translation units; its one unchecked optional finding at the
root-side fallback was changed to an explicit throwing guard, then the affected
source was rerun GREEN. No suppressed user-code finding or extra numerical suite.

Pinned display cohort fixture SHA256:
638758523d4f01c3ff0ce51308ab943114bc731514908cbbab5dd3a4dd8aa2d4.
Pinned existing owned display module SHA256:
e87cbf89099f9785029fa4c52aa0012308e444f82e6363a45d8435cf2d007b36.
The final source adds finite material-slot/mapped-facet admission and explicit
optional failure guards; those do not change the admitted cohort. Final ASan
representatives and selected source lint use that guarded version. Raw logs and
actual artifacts remain context/adaptive-scene/ on fleet. Isolated builds:
build/adaptive-scene-release and build/adaptive-scene-asan.

Numerical peer actual-source review ACCEPT covers original root selection,
positive facet sector, exact original net extraction, provenance and orientation.
Parent actual-source review found only the now-corrected normal refusal wording.
No commits/pushes from this lane; final source window handed to parent.
