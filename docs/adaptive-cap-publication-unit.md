# Retained cap maps through artifacts and display

2026-10-17. Root design/finite plan before body. Complete singular construction
and runtime are dependencies. This closes publication of their actual result;
it does not create another mesher, coverage index or numerical source authority.

## Grounding and actual data

Read accepted cap construction/runtime/geometry plans, actual
adaptive_cap_domain.hpp, adaptive_trim_types.hpp, adaptive_caller_io.cpp,
adaptive_trimmed_io.cpp and adaptive_scene.cpp. Read existing
scene_view.cpp::normal_flags and om10/math.hpp::source_evaluate's
ANALYTIC_NORMAL_LIMIT branch. Their owned source-normal convention derives
analytic Su cross Sv on a selected regular sheet: sphere pole limit and cone
angular-sector limit. Existing source-normal flags/API2 already express this;
no new renderer/kernel ABI or sampled physical pole is needed. Source and
reference grounding for the cap map/sector topology remains the pinned
USD/BRep MakePolygon and OCCT endpoint mechanisms in the construction plan;
they do not supply our error proof or artifact semantics.

## Versioned external records

BGACRV01 version3 adds the complete seam-wedge pool after existing version2
records. Ordinary snapshots without wedges continue writing version2. Each row
records ORIGINAL VertexID/LoopID, both original CoedgeIDs, both shared-site
indices, cap kind and four exact finite connector UV coordinates. Validate
source incidence/site bounds and serialize exact refs from snapshot.values.
No source coedge is created for the connector. Readers select the wire version
explicitly; old readers refuse version3 rather than discard the pool.

BGATRM01 version2 retains ordinary records and adds a typed cap extension. Use
version2 when the result has cap data, mapped facets, wedge bindings or independent
face/domain revision semantics. Existing ordinary version1 remains unchanged.
The extension retains full face_generation/domain_revision/assignment keys,
produced domain cap rows, angular-to-original-sector order, original wedge loop
identity, original pole boundary node, exact cut point plans/authorities, piece
maps and mapped facet-to-piece/source-role associations. Exact refs are written
by value from their ACTUAL owning table; no ref from domain.values is read as
construction.values. Remap only original root ordinals through the existing
complete selected-carrier closure. Physical point locators retain original
EdgeID/CoedgeID/exact original t/logical key, never a private surface point on
the original chain. Source occurrence bindings retain explicit wedge/sector tags.

Mapped coverage is the finite QUADRILATERAL (both connector UVs and both cut UVs),
with both connector corners mapping to SAME fixed physical pole Q. It is not
the construction site's locator-UV triangle. No ordinary packet association,
UV metric or triangle source bound is invented for it. Record finite connector,
piece cut, sheet, positive source-role permutation and current actual-Q site
roles explicitly. The SAME original atomic-chain/coverage reader consumes these
quad edge words alongside ordinary triangles; physical incidence still consumes
actual triangles and has no connector edge. Construction owner extends that
same reader, root owns wire writer/version dispatch, no second coverage checker.

## Source display adaptation

Accept mapped facets only with matching retained produced domain/piece/full face
key and exact source-role-to-facet permutation. Actual XYZ words remain copied
from the accepted construction. Cut corners use their ordinary original source
UV and normal. At the pole role, choose the exact midpoint of that piece's
increasing-U cut endpoints as cosmetic normal U; choose the exact midpoint of
its finite connector V endpoints as the finite evaluator locator. Convert those
exact values once with existing nearest-even conversion. This locator does not
move Q or assert ideal pole identity.

Set STORED_XYZ, COFACTOR_NORMAL, ANALYTIC_NORMAL_LIMIT, sheet sign and SPHERE_SOUTH
from the retained source-qualified descriptor; REVERSE_NORMAL and physical
triangle role swap apply original face orientation once. The existing consumer
computes the explicit limit normal while using STORED_XYZ for geometry. It still
rejects unrepresentable derivatives/normals. Source descriptor remains original
frame/radius; no CPU duplicate normal evaluator is added to preparation.

Append explicit optional mapped pole-normal provenance (piece and source role)
to AdaptiveSceneVertexSource. Keep construction original_uv refs unchanged;
the mapped descriptor explains the separate cosmetic locator. Include that
normal-sector identity in candidate dedup keys, so different cone sectors do
not collapse to one display corner merely because physical key/site matches.
Same source physical ID is retained across those corners. Facet provenance
already retains construction facet, hence its full finite map. Cut corners can
share ordinary display vertices when all actual/source-normal inputs match.
No mapped locator is passed to ordinary accuracy or source-coverage checks.

## Integration and finite RED/GREEN

Review plan before edits. Existing adaptive scene refusal for mapped facets and
ordinary wire's absent typed extension supply actual missing-operation RED once
the source-backed constructive fixture produces its first real mapped result.
Do not forge a successful cap solely to test publication. Implement external
versioned records first, extend SAME readers, then adapt scene. Require one
complete source-backed north/south/cone result through ordinary+cap runtime;
the existing original Fsrc/SAME-L and chain readers remain geometry truth.
CPU/P100 source-normal consumers check pole limits against independent original
derivative formulas, source orientation, sheet and chosen angular sector, along
with exact copied actual XYZ and shared physical IDs. One real assembled/exploded
singular-component image uses existing display/presentation, no alternate mesh.

Close with scoped native lifetime/static checks for added provenance/records,
actual-source peer review and one ordinary saved artifact compatibility read.
Do not repeat the108-row low-level numerical cohort. Root also stores immutable
FaceContinuation per pending face before pooled retirement, supplies it on resume,
and retains it also for complete faces: another face's physical refinement can
invalidate them later in the SAME query. Both complete and needs_boundary
capture immutable control/metric; regular faces return empty. Fresh query has
no prior history. This source-read finding was agreed with runtime/construction
before the coordinator edit. The existing ContactEdgeKind::face_cut carries
source-face owner/current independent face generation and cut interval; original
source edges retain their existing identity. Contact frontier identity uses
independent face_generation, with construction revision still qualifying cells.
Component contact edges retain physical provenance. Use the
existing contact horizon and actual used-node key helper; no coordinate weld or
new contact certifier. Fresh query starts without prior cut control.

After connected singular operations pass, the complete model caller may use the
quotient-capable bank with explicit produced empty-cap domains for regular faces.
Then run the actual complete watch, CPU/P100 and NVIDIA reference, retaining all
required original inventory and reporting any failure/censor. The whole-watch
render remains pending until those outcomes exist.
