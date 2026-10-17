# Complete original-definition output oracle

2026-10-17. Test-only extension of the existing selected-face original-source
reader. Root owns ComponentMesher, CLI/export and CMake. This unit owns
`tests/native_adaptive_component.py`, the optional original-face selector in
`tests/native_adaptive_trimmed_caller.py`, and this plan/journal. Coordinate any
extension of root's `tests/native_adaptive_component.cpp` before editing it.

## Grounding and claim

The coordinator plan cites NVIDIA USD/BRep c9c5979f04cea8c38213004d4f04df0dcfb40d05
`SmTess::TessellateEdge`, `Phase1SetupBrep`, `Phase2CreateFacePolygons`, and OCCT
8.0.1 b8f597c677811d1f9f4d8a97f5ae2825c0353a42
`EdgeDiscret::process/Tessellate2d`, `ModelPreProcessor::SeamEdgeAmplifier`, and
`FaceDiscret::performInternal/process`: one spatial edge chain supports separate
chart occurrences. Our direct oracle checks that relation on actual complete
native output. It reuses `qualify_face`/`qualify_chain` and original carrier
formulas; it adds no second triangulation or triangle-contact checker.

## Fixed sources, selected before results

1. Unit closed box: eight original vertices, twelve original axis edges, six
   independent original plane carriers/faces, twenty-four original coedge uses.
   Every spatial edge has two opposed original uses. One chart swaps its two
   basis axes and has source face orientation -1, preserving the outward physical
   use word; this exercises normalized-domain reversal and one physical flip.
2. Existing `cylinder_source()` seam component: two original circles and one
   vertical spatial edge used twice by the same cylindrical face at distinct
   literal unwrapped U values. Preserve chart occurrences and shared physical
   keys; use the existing rigorous cylinder facet/chord reader.
3. A disconnected definition containing an ordinary plane square and a genuine
   unit-circle trimmed plane (both stored circle radii equal one). The closed
   edge repeats its endpoint vertex. A coarse positional allowance accepts its
   initial chord error, but the selected loop is not representable until the
   construction requests shared master subdivisions. Require an actual boundary
   round/request and a retained unaffected square face, without fixing counts.
4. One explicit coincident pair of independent original plane faces: separate
   physical identities at identical XYZ. Expect the existing contact owner to
   refuse the component and publish no complete output. This does not add a
   second contact classifier.

## Reader changes

`qualify_face` and `original_boundary` accept an optional ORIGINAL face row;
default remains the first face. They receive the complete SourceStore and the
exact BGACRV captured by that BGATRM, never a filtered product source. Each
BGACRV source-use inventory must equal its definition's original inventory;
BGATRM selected-face uses and bindings must match every original selected-face
use occurrence.

The component index must name every original face of the explicit definition
exactly once, in canonical FaceID order, with paired artifact paths. Each face
then passes the SAME positive-UV/atomic-chain/Euler/whole-original-facet reader.
The BGATRM root inventory must contain only its selected original carrier's
complete root closure, including unused original spans; grid, anchor and packet
source indices must address this remapped closure. Six box carriers attack
accidental whole-bank leakage.

Extend the SAME directed atomic incidence accounting to physical keys: apply
source face orientation once; original boundary sites use captured BGACRV keys,
generated sites remain face-qualified. No coordinate welding. The closed box's
physical frontier must cancel completely; open-component frontier equals the
independently authored original singly-used edge chains. Seam chart sites stay
distinct while sharing actual physical keys/XYZ and opposed source traversal.

An older retained face snapshot may contain stale unrelated edge rows. Compare
only actually referenced selected-face incident source chains to the final
boundary snapshot, including exact parameters, keys, construction revisions and
XYZ. Do not demand unrelated older rows equal current ones. Contact counters
must describe the complete actual final facet horizon; the existing service is
the sole contact mechanism. Host results must remain readable after owner close.

## Execution order and stopping rule

Root captured the actual missing-ComponentMesher link RED before body work at
`context/adaptive-component/missing-body-red.log`. First author source builders
and selected-face reader changes; use root's real native fixture, not a mock
component. On fleet only, run closed box CPU1 first, then periodic and genuine
boundary-request cases CPU1. Report actual product failures before edits or
matched placements. Root approves the bounded CPU4/both-P100/history closure
after the first complete original-source outputs pass. Reuse existing numerical
and device qualification. No watch run, geometry search, new dependency, second
coverage mechanism, renderer or broad sanitizer campaign is introduced.
