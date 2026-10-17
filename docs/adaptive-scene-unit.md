# Actual cached component output to native display

2026-10-17. DESIGN and bounded implementation subplan, pending review. Own
`adaptive_scene.hpp/.cpp`, shared packed scene storage and the narrow
`scene_view`/`presentation` seams. Root owns whole-source caller/publication,
CMake and renderer-module integration. No product edits before review.

## Sources and selected mechanism

Read `adaptive-component-unit.md`, actual `adaptive_component`/`adaptive_trimmed`
result leases, `assembly.cpp::prepare_assembly`, `scene_view.cpp::SceneBuilder`,
`presentation.cpp::make_presentation`, `om10/math.hpp::evaluate/project`, and
`om10/validation.hpp::validate_scene_view`. The existing source-profile
BGMESH01/API2 records already copy STORED_XYZ and transform normals by matrix
cofactors, permit occurrence-inherited material, and select one original
rational root per display corner. Reuse those actual mechanisms, not the old
NativeMesh/GeometryStore semantic publisher. This follows the shared source
edge/per-occurrence chart separation grounded in the pinned NVIDIA/OCCT functions
recorded in adaptive-component-coordinator-unit.md; no upstream claim is made
about our exact source/normal or assembly bounds.

Rational normal roots come only from
`adaptive_geometry.cpp::surface_detail::original_net_exact`: lazy original
Bezier extraction of each USED original span. Numerical leaf nets, approximate
source cache intervals and eager whole-source GeometryStore are not inputs.
Original analytic frames/radii feed the existing finite display descriptors.
The original source and complete semantic artifacts remain retained separately
from the cosmetic display coefficients.

## Literal API and storage

Extract `SceneBuffers` into `scene_buffers.hpp`: retained RunResource and the
existing pmr vectors definitions/instances/vertices/triangles/carriers/materials/
controls/vertex_refs/primitive_refs, constructor(resource), `view() const`.
NativeSceneData retains mesh/geometry and derives from that storage; its current
public buffer fields, make_scene and write_scene behavior stay source-compatible.
This is one packed buffer owner shared by both producers.

In namespace `bgmesh::adaptive`:

* `enum class SceneScope { complete_assembly, selected_components };`
* `AdaptiveSceneRequest { AssemblyStore assembly; span<const ComponentResult>
  components; SceneScope scope=complete_assembly; ScenePresentation presentation; }`.
* `AdaptiveSceneData : SceneBuffers` retains SourceStore, AssemblyStore, copied
  ComponentResult leases, scope, canonical original definition slots and original
  instance indices. A flat provenance row per display vertex names component,
  face, construction site, selected original root (or analytic carrier), and the
  two ORIGINAL exact UV refs. No Budget-affine Rational escapes preparation.
* `using AdaptiveScene=shared_ptr<const AdaptiveSceneData>;`
* `make_adaptive_scene(Budget&, const shared_ptr<RunResource>&,
  const AdaptiveSceneRequest&) -> AdaptiveScene`.

Default scope requires exactly every definition used by the original AssemblyStore
and every original face of each such definition. Explicit selected_components
retains exactly the supplied complete definitions and their original placed
occurrences, recording that subset; it never claims full assembly. Reject absent
placements rather than inventing an identity instance. Preserve each original
instance's assembly index+1 as its display ID even in a subset. Source/assembly/
component stores and resources must match, components must be unique and one
quality generation, and local component caps must fit retained assembly caps.
All face/site/facet and source-root references are checked before dereference.

Root supplies one shared `component_site_key` helper for the existing boundary
versus generated-face-site identity rule. Both contact input and display consume
that rule. Sort once by physical key, verify equal-key actual XYZ words and assign
scene-local physical IDs. Distinct equal-coordinate source keys stay distinct.

## Geometry, roots and orientation

Iterate original definitions/faces and actual facets in their canonical result
order. Copy each construction site's actualXYZ WORDS unchanged. Display UV is the
correct nearest-even binary64 conversion of its original exact coordinate, while
the exact refs and owning construction lease remain in provenance. UV conversion
is a normal locator, never a new mesh position or a tolerance charge.

For analytic carriers reuse original frame/radius display projection and the
existing STORED_XYZ|COFACTOR_NORMAL flags. For an original rational surface, find
original spans containing the EXACT corner UV, not a prepared leaf or its point
sampler's canonical owner. Interior points have one span. At an original knot,
choose an original closed span whose intersection with THIS actual facet's UV
triangle has positive area. If the centroid direction selects one such side use
it; otherwise use canonical span(U,V) order among the qualifying sides. Convexity
of the triangle and span box means positive-area intersection containing the
corner supplies a one-sided facet-interior source sector. Exact small triangle/
rectangle clipping is needed only at ambiguous knot corners, not every corner.

Duplicate display vertices when selected original span/face/normal orientation
differs. Thus a C0 boundary shared by two facets gets the two original one-sided
source normals; a facet that crosses a knot keeps its actual affine geometry and
selects a real incident source side at an ambiguous corner. No global smoothness
or exact normal over a knot-crossing facet is claimed. The current renderer
already evaluates that chosen root at a closed endpoint. Original source knots
are binary64, so nearest rounding of an interior exact UV remains within its
closed original span. Failure to find a qualifying source sector refuses display.

Extract each used original root once, convert its original homogeneous
coefficients to binary64 once, check finite/positive display weights and append
tag6 descriptor/controls in U-major/V-fast order. These are cosmetic normals;
the stored actual XYZ remains authoritative. No source re-evaluation moves Q.
Current SourceCache surface profile and existing renderer both admit degrees
through3. The adapter checks finite source descriptors, weights and locators;
the actual CPU/P100 `source_evaluate` stage detects zero/unrepresentable source
normals. Such consumer failure aborts required display and complete caller
publication. Do not duplicate that evaluation on CPU during adapter preparation.
Singular pole/quotient normals need actual future retained cap records; this
adapter does not infer a pole from carrier kind or rounded UV.

Construction facets are positive ORIGINAL-UV roles. Swap roles1/2 exactly once
when original face.orientation<0; also set REVERSE_NORMAL for that face. The
renderer uses cofactor instance transport, including shear and negative
determinants. Boundary seams keep distinct chart corners with the same physical
ID. Sort/unique display corner candidates by physical ID, original source face,
chosen original root, exact UV identity and normal flags; prefix-fill triangle
indices with dense remap arrays. No heap map per triangle/vertex.

## Assembly, materials and presentation

Copy stored AssemblyStore matrices unchanged; do not compose/round them again.
Use its original product/path/style association and existing precedence. One
material table contains default gray(.7,.7,.7), roughness.5 and original styles
that have color. Explicit original face style selects a triangle material;
otherwise use INHERIT_OCCURRENCE_MATERIAL and instance's 1-based resolved override.
Do not duplicate geometry for differently styled occurrences. Original BOM/path
data remains in AssemblyStore for root's existing external BOM writer.

Add `make_presentation(Budget&, const bgom10::SceneView&,width,height,strength)`;
the old NativeScene overload validates non-null then forwards. Compute the SAME
stored-Q boxes, assembly placement, camera, explosion and label numbering through
that one implementation. Existing `write_scene(SceneView)`, write_views,
annotate_frame, PNG and CPU/CUDA API2 consumers remain the actual display path.
Presentation offsets are separate from assembly matrices/geometry. Root may
adapt its private NativeScene-specific Display wrapper to retain AdaptiveScene
and borrow SceneView; this unit adds no renderer or image dependency.

## Direct RED, implementation order and finite checks

1. Root review this whole unit and interface. Add genuine source-backed fixture
   calling make_adaptive_scene before the body; observe missing operation RED.
2. Extract SceneBuffers and SceneView presentation overload with the old producer
   forwarding unchanged. Implement actual adaptive adapter, guarded source/root
   lookup and lazy root extraction. Root registers target linking adaptive
   component/geometry/assembly plus existing scene/presentation libraries.
3. Independent external reader consumes SAME saved BGACRV/BGATRM/source and
   BGMESH01 arrays, extending the component reader rather than a new mesh oracle.
   Check every display triangle's source face and exact actual corner XYZ against
   actual facets under the one orientation permutation; every source UV locator
   equals correctly rounded original exact UV, and provenance retains exact refs.
   Physical seam IDs must agree while corner UVs differ. Every output definition,
   occurrence and material matches original source/AssemblyStore, with no extras.
4. Fixed direct cases: existing closed box with reversed chart; existing cylinder
   seam; original weighted/C0 surface (including a C0 knot corner) for original
   root controls and one-sided normals; repeated definition under proper quarter-turn/translation
   with distinct occurrence styles and one explicit face style. Use authored
   original power/analytic derivative formulas for source-normal directions,
   not CPU/CUDA equality as the normal oracle. Original whole-facet accuracy and
   chain coverage remain the existing component/face reader's sole mechanism.
   Amendment accepted before the styled source run: original SourceStore
   `source.cpp:655–663` and Python source writer admit only proper near-rigid
   Usage matrices. Strong shear/reflection is outside that source profile and
   was rejected during fixture writing; preserve that contract. Existing direct
   renderer affine/cofactor checks retain their separate programmatic-input
   claim, not a whole-source placement claim from this adapter.
5. Existing CPU API2 consumes these same buffers; both P100 consumers compare
   actual used XYZ and projected normal directions against those source formulas.
   A small real assembled/exploded render exercises existing presentation and
   retained buffers; pixels are a display outcome, not a geometry oracle.
   Scope host lifetime/sanitizer checks to the new buffers/provenance/shared
   storage, and one old scene fixture to the extraction compatibility. Parent
   approves the bounded runs before execution. No watch trial or new campaign.

Any source mismatch, missing complete component, stale source key or unrepresentable
display coefficient aborts preparation. A source-normal or rendering failure in
the actual display consumer aborts the required display. Root publishes only
after all required semantic and display outputs complete.
