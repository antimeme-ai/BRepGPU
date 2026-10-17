# Native source display adapter review

Reviewed actual `native/mesher/scene_view.hpp/.cpp`, the affected
`native/om10/{types,math,scene}.hpp` source-profile/material changes,
`tests/native_mesher_scene.cpp` and `tests/native_mesher_display_api.cpp`
against `docs/native-mesher-scene-unit.md`. Read-only review; no execution or
additional test campaign was performed by this reviewer.

The adapter copies physical-node XYZ and physical identity, retains source
normal locators/carrier roots separately, and keeps final FACETS role order.
Source orientation becomes one normal-reversal flag; no second facet reversal
was found. Candidate sorting keeps face/root/UV/flags in its identity and dense
corner remapping preserves each facet's original ordered roles. Rational
controls and original roots feed cosmetic normals rather than output positions.
Declared pole limits derive regular-sheet signs from exact retained cut
parameters; source trig enclosures refine uncertain signs. Sphere north/south,
cone radial sheet and instance cofactor transport are consistent with the
selected formulas.

Occurrence appearance is one-based in `Instance.reserved`, with zero absent.
Only source-profile triangles may inherit via UINT32_MAX. Explicit face styles
remain explicit triangle materials; otherwise the placed occurrence material is
resolved into Prepared.material. CPU/CUDA use the same setup code. Legacy
profile rejects the new fields. Header/record byte layouts remain unchanged,
and the writer emits explicit little-endian scalar fields.

Two constructor/writer contract findings were sent to the owner:

1. `make_scene` did not yet invoke the file decoder's complete record validation.
   Direct `scene_vertex_profile` checked flags/material refs only. Derived
   rounded rational spans that collapse, invalid source display frames, or
   collapsed stored triangles could return a NativeScene accepted by a direct
   route while the emitted file is rejected. Factor the finite borrowed
   SceneView record validator for the factory/file boundary; do not duplicate
   geometry qualification or copy the whole scene merely to validate it.
2. `write_views` checked finite doubles/count narrowing but did not enforce the
   existing Request dimensions/reserved fields/flags/alpha/scale/camera/color
   rules used by `load_requests`. A zero-width request could be emitted then
   refused by the reader. Share that finite Request contract at the writer and
   file reader (and applicable direct boundary).

Pending owner fixes and focused reread; other reviewed associations/formulas
have no concrete finding.

## Finding resolution

Root delegated the finite boundary unit back to this owner. The actual code now
shares `validation.hpp` record/placement checks across make_scene, file decode
and both direct factories. It shares one Request validator across reader,
writer and CPU/CUDA render; malformed requests leave output untouched. Shared
finite vector/profile helpers are in `vector_math.hpp`, independently of source
evaluation/raster code. PMR scratch contains only IDs, and native construction
charges its caller Budget. The original record/wire formats remain unchanged.

The topology owner independently reviewed the actual validator and accepted
pointer/count guards, complete partitions, index-before-dereference ordering,
canonical placements, source/legacy material handling and Budget context. The
original scene association/formula review has no other concrete blocker.
Execution scope/results are in journal/2026-10-16-native-mesher-scene-review.md.
