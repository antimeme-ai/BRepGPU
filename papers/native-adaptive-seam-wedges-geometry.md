# Original-vertex seam wedges in the cached mesher

2026-10-17. Read-only geometry/design study after the retained carrier-bank unit.
No product changes, source repair, meshing run or new dependency. This proposes
one finite singular-cap unit for review; its API names below are proposals.
Private imported source paths, identifiers and coordinates remain in ignored
context. Geometry dispatch must use the checked conditions, never that inventory.

## What the source actually requires

The previous aggregate original-source study found three singular-envelope
candidates: two cones and a sphere. Each has one loop, THREE original uses and
TWO original vertices. One physical meridian edge occurs twice in opposite loop
directions; a regular-side closed edge connects its other endpoint to itself.
There is no original collapsed coedge. The same original vertex at the two
meridian endpoints is already physical authority; no coordinate weld, new source
edge or degenerate-edge union is needed for this profile.

A second read-only coefficient inspection matters to the finite admission. Only
one cone has both seam guides exactly constant in U. The other cone and the
sphere each have a rational guide with nonconstant U controls. This is genuine
stored algebra, not an evaluator-roundoff observation. Porting the old
constant-U-guide test would therefore leave two capability gaps. All these uses
are marked reconstructed; the guides can propose new selected UVs, but ORIGINAL
V/Ce-to-S(chosenUV) nodal guards and trace-to-actual-chord bounds must still pass.
No conclusion that the actual candidates pass those guards has been measured.

`source_uv.cpp::query_poles` creates a temporary degenerate connector on a copied
QUERY shape. `require_source_uv_uses` checks that its incoming/outgoing physical
uses remain consecutive and share the original vertex. It separately counts the
original uses, so that temporary connector is not an original BGSRC coedge.
The native design must preserve that distinction without calling the importer
or its OCCT geometry from production.

The current cached path cannot represent the case honestly yet:

* `adaptive_traces.cpp::prepare_trace_sites` copies one chosen joint UV to both
  incident occurrences and shares its carrier sample. That is right for an
  ordinary chart joint and erases a pole's two source sectors.
* `adaptive_regions.cpp::make_face_domain` requires equal UVs at every joint and
  constructs only ordinary selected-PL cycles. It has no construction-only
  collapsed connector or mapped cap.
* `source_cache.cpp::admit_analytic` rejects cone radius-zero and sphere pole
  rectangles, including trace-purpose caches. Trace purpose skips only the
  angular-width guard. Conversely `adaptive_numeric.hpp::bound_node_rn64` sets
  analytic `regularity_proved=1`, relying on that source admission.
* `FacetGenerator`, `RegionFacet`, actual-facet work and display currently describe
  only ordinary original-UV facets. A pole proxy cannot enter that path as if its
  dyadic UV were the true source locus or an ordinary normal locator.

## Reference reading and selected improvements

Pinned NVIDIA USD/BRep revision
`c9c5979f04cea8c38213004d4f04df0dcfb40d05`,
`source/SMLib/src/SmTess.cpp`:

* `sms_FillGapUVWithPolyEdge`, around1440–1595, explicitly distinguishes a pole
  vertex with two UV images. It inserts a polygon edge and keeps original-vertex
  back references. Its tolerance/double-back tests are upstream repair policy,
  not our singularity or error proof.
* `SmTess::MakePolygon`, comments/mechanism around1610–1760 and1871 onward,
  explicitly describes a sphere with missing singular segments and one pole
  vertex becoming two polygon vertices plus an extra polygon edge. It also
  separates seam physical identity from chart multiplicity. This supports a
  construction connector, not inventing an original source use.
* `TessellateEdge`, around3009–3066, and `Phase1SetupBrep`, around11118–11866,
  prepare shared physical edge schedules before the face polygons. Keep that
  ownership when a cap asks for meridian cuts.
* `Phase2CreateFacePolygons`, around12023–12131, emits cached inside quads and
  triangulates the remaining regions. Retain that primary architecture for the
  regular remainder; a cap is an explicit finite mapped region of the same face.

Pinned OCCT8.0.1 revision `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`:

* `ShapeAnalysis_Surface::IsDegenerated`, `DegeneratedValues` and
  `ProjectDegenerated` use an original 3D point, singular surface locus and nearby
  UV sector. Their distance grades are not exact source-pole identities.
* `ShapeFix_Wire::FixDegenerated` can insert/remove degenerate query edges. We
  retain original source inventory and emit explicit construction records instead.
* `BRepMesh_EdgeDiscret::Tessellate3d/Tessellate2d` preserves original endpoint
  vertices and supplies every pcurve occurrence from the same physical schedule.
* `BRepMesh_SphereRangeSplitter::GenerateSurfaceNodes` uses staggered angular
  samples from a deflection step. That is a useful layout contender, but neither
  a pointwise source-cap bound nor a substitute for the shared boundary authority.

The vector interpolation inequality comes from the acquired
Filip–Magedson–Markot1986 §2, Theorems2/4, DOI
`10.1016/0167-8396(86)90005-1`. Applying it to the cap's square parameter map,
and the finite-source correspondence below, is our derivation. The existing
owned `geometry_poles.cpp::bound_analytic_pole_fan` and its corrected-band review
already implement that derivation independently of GeometryStore. Reuse its
source formulas and proof, not old FaceEngine or eager geometry preparation.
The cap is a parameterized source-to-actual approximation; do not claim an exact
analytic pole identity from a binary64 endpoint or a universal singular atlas.

## Finite chosen-source profile

Select a sphere north/south or nonzero-angle cone, independent original frame
axes and nonzero source radius, one outer source loop and one shared-vertex seam
wedge. The two consecutive source uses at the wedge must share the SAME original
vertex and actual fixed XYZ words. In the initial profile they are the two
occurrences of one nondegenerate physical meridian edge, with the remaining
original use the regular side. This is checked source topology, not a face/use
count dispatch. Unsupported additional holes, interacting caps, nonmonotone
meridians or arbitrary polar trims remain named limitations of the first unit.

Retain both original use/site records at the wedge, their literal spatial t,
mapped raw s, raw guide curve/lift, chosen UV and ORIGINAL source-node bound.
Introduce a face-level `SeamWedge` record naming the source vertex, loop,
incoming/outgoing use endpoint occurrences, analytic pole kind and two selected
branch UV refs. A construction connector joins those two UV values; it does not
appear in SourceStore or the physical edge forest. Both occurrence endpoints
continue to use the original vertex's PhysicalNodeKey and actual Q.

For reconstructed meridians, propose constant U independently on each unwrapped
branch from a regular-side raw-guide endpoint (exact dyadic chosen coordinate).
Apply that constant U to the selected sites on the use, retaining their existing
guide-derived V proposals, except for any separately admitted source height
locator. Require exact monotone V in source traversal, distinct angular branch
order, positive reference-fan height and all selected coordinates in the SAME
retained assembly profile. Then execute unchanged original V/Ce-to-S(chosenUV)
node guards and whole selected carrier-trace-to-SAME-actual-chord bounds. The raw
guide is retained as proposal/source provenance. It is not asserted equal to the
selected constant-U trace. Failure does not spend delta, relax a grade, repair
the source or privately move Q; it refuses or asks the existing physical master
for a justified finer interval. A persistent source-node failure is unresolved.

This proposal handles a nonconstant reconstructed U guide without a new general
inverse. A guide-derived constant branch may fail legitimate geometry, so it is
not a universal admission rule. A later general inverse needs its own finite
branch/iteration policy. General authoritative curved uses require the real B/T
domain map; they cannot acquire this reconstructed policy. An authoritative cap
could later admit a checked exact identity profile with its explicit raw source
coverage, but that is not inferred here from a small boundary residual.

## One source cache, explicit singular capability

Keep assembly's finite original carrier rectangle unchanged. Original source
evaluation on a singular analytic rectangle is mathematically well-defined;
ordinary regular chart construction on that whole rectangle is a different
claim. Do not weaken `prepare_trim_source_cache` or relabel trace purpose.

Propose an explicit quotient-capable source factory/purpose. It retains the SAME
SourceStore, profile/generation/resource, original full envelope and deterministic
prepared angular panels, and records that analytic regularity must be checked
per retained ordinary node. It skips only the global regular-sheet exclusion for
supported cone/sphere source evaluation. Finite independent frames, finite
parameters/radii, supported angle arithmetic and all source envelopes remain
checked. Regular PatchSession must refuse this capability absent the matching
singular-domain operation; shared boundary work can use it for source samples
but cannot invent a cap merely from carrier kind.

For ordinary retained nodes, prove regularity using their ACTUAL parameter box:
cone `r0+v*sin(alpha)` excludes zero and `cos(alpha)` excludes zero; sphere
`cos(v)` excludes zero on the admitted single sheet. Compute those conditions
with the same interval/precise ladder and original frame. An undecided node is
split or bounded unresolved, never positional PASS with regularity assumed.
Analytic source samples at the pole remain legal; they are not ordinary facets.

Add a typed mapped-cap classification result naming the admitted cap, alongside
ordinary interior/boundary/outside. A leaf wholly in the cap is covered by that
mapped region, not a fabricated material-OUT label. Cut-straddling nodes follow
the same cache refinement and ordinary boundary-band construction. The complete
source partition is ordinary remainder plus explicit mapped cap. Existing
SurfaceBoundView/domain provenance must retain that same partition, including
all prepared roots even when no ordinary leaf survives in a cap region.

This avoids a second cut-dependent source bank, an eager GeometryStore and
recomputing assembly allowances at each radial cut. Source roots are immutable;
cut revisions change the domain/classification and face generation. A simpler
evaluation-only bank plus a new regular bank for every cut duplicates root
closures, point ownership and cache lifetime and is not selected. The exact
source admission/API fields need runtime review before product edits.

## Whole-cap source correspondence and actual L

Write `u(xi)=u0+xi*du`, a finite chosen connector
`vf(xi)=(1-xi)*v0+xi*v1`, a linear cut
`g(xi)=(1-xi)*c0+xi*c1`, and `0<=xi,rho<=1`. Require the reference width
`g-vf` to have a fixed nonzero sign. The chosen source fan is

    Fsrc(xi,rho) = S(u(xi), (1-rho)*vf(xi)+rho*g(xi)).

It covers the selected UV trapezoid once before identifying its connector with
the original physical joint. That identification is an APPROXIMATION with an
explicit bound, not an exact assertion that all S(u,vf) equal the vertex.
The ideal comparison fan uses the symbolic pole `vp` instead of `vf`:

    Fideal(xi,rho) = S(u(xi), (1-rho)*vp+rho*g(xi)).
    L(xi,rho) = (1-rho)*Qpole + rho*((1-xi)*Qleft+xi*Qright).

L is exactly the emitted physical triangle in fan coordinates. No source
resampling at virtual points changes it. At rho0, every xi maps to the SAME
fixed original Qpole. With `dg=c1-c0` (height difference, not derivative in U),
the ideal derivatives are

    F_xixi = du^2*S_uu + 2*du*rho*dg*S_uv + (rho*dg)^2*S_vv,
    F_rhorho = (g-vp)^2*S_vv.

The interpolation bound on the square is

    Eideal <= sup||F_xixi||/8 + sup||F_rhorho||/8
              + max(actual four-corner departures).

Both ideal corners at rho0 use Qpole. The ideal/source difference is pointwise

    ||Fsrc-Fideal|| <= (1-rho)*|vf-vp|*sup||S_v||.

The S_v supremum MUST cover the convex V hull of finite connector, ideal pole
and BOTH cuts for that whole angular piece. A derivative bound confined to the
near-pole strip is invalid on a general nonorthogonal frame; that exact defect
was fixed in the old owned primitive. Require `Eideal+Eband<=local_tau` for this
actual mapped facet. Original node-source delta is a separate admission and is
not an output-approximation budget. An original vertex merely near the ideal
pole may pass or fail its positional floor; do not set it to the ideal point.

Perpiece angular widths are at most a conservative pi/2 lower bound, with a
deterministic source-derived base partition. The finite whole-map bound does
not require declaring stored endpoint angles an exact full period. Topology and
actual nonzero facets still need the original occurrence/edge-chain checks and
existing complete-component contact mechanism; no injective global chart is
claimed across an analytic pole.

Use an RN directed/padded common cap-bound kernel on CPU/P100, precomputing the
fixed source frame/angle/pole intervals once. Reuse fixed128/192/256 or the
owned exact analytic primitive only for unresolved same-task comparisons.
This is not old point-by-point rational-trig preparation. The producer retains
actual fixed-pole departure separately from changing cut-corner departure.
After an actual cap bound fails, a fixed source-profile upper that cannot fit
can produce named unresolved; a loose upper is never a lower-bound claim that
the source is impossible. Original V cannot be numerically promoted.

## Shared cuts, progress and result transport

Choose radial endpoints only from the shared meridian master's settled chain.
The two seam occurrences use the SAME t and physical Q but different branch UV.
For this repeated-edge profile a source split normally supplies both cuts at
once. Keep the general both-endpoint history guard: do not call a one-sided
replacement contracted until both symbolic parameter gaps are at most3/4 their
previous values. Until then retain the old cut and request the missing source
side. Radial requests go through the definition's complete boundary horizon.

Initial cuts must be strict interior master samples. This is a source ownership
obligation, independent of epsilon, and avoids private angular sites on the
opposite original shared boundary. Private angular cut sites are face-owned,
sampled through the same source point authority. They are shared exactly by the
regular remainder and cap. Angular failure increments a face-cut revision;
radial failure requests the actual source BoundaryKey. Never send face_cut keys
into the physical source forest. An empty ordinary remainder needs the actual
opposite master chain and source keys; it is a separate admitted case, not a
reason to privately replace that original edge.

Proposed immutable rows, qualified by retained source/boundary/face/profile keys:

* `SeamWedge`: original vertex, loop, two endpoint occurrences, pole kind,
  finite connector UV4 and raw use-site refs. No fictitious quotient_use.
* `MappedCap`: wedge index, original evaluator root, fixed Qpole/site,
  angular-piece range, source/ideal V enclosures, regular sheet sign,
  exact bound precision/profile and cut history/revision.
* `MappedCapPiece`: cut sites/actual Qs, UV4, face-cut key, source-map roles,
  positive chart role permutation, angular/radial/corner/band bounds.
* `CapCutControl`: immutable exact table and old cut refs, angular level,
  revision, pending radial sides. No Budget-affine Rational escapes.

`SharedCurveSnapshot` retains all original sites/uses and the wedge occurrence
pair. `FaceDomainLease` retains the chosen reference connector/cap map and
ordinary construction loops. SourceOccurrenceBinding must not erase either
original branch when both bind one physical pole. Ordinary active topology and
mapped cap topology are separate roles of the SAME face result. Add an explicit
mapped-cap facet generator; a connectivity proxy/UV never enters ordinary metric,
surface leaf error, ordinary physical-zero rejection or ordinary normal lookup.
Every emitted physical cap triangle must nevertheless have nonzero exact stored
XYZ area. Shared cuts have opposite incidence, and original face orientation is
applied once at publication. Semantic export retains source facts even when they
are inactive in ordinary CDT.

Display needs an explicit pole-corner descriptor with north/south or cone radial
sheet/sector, alongside the actual source map and original vertex key. The
existing renderer source-normal profile already has symbolic pole/radial-sheet
normal machinery; adapt the actual retained descriptor rather than inferring a
pole from rounded proxy UV. Stored XYZ is always the emitted Q. Assembly B0 needs
no ideal-apex extension: its original box+epsilon transport already covers every
accepted actual L under the full source-map bound, as derived in the completed
adaptive assembly unit.

## Alternatives and finite delivery order

Moving seam joint UVs to one branch loses the reference domain and does not
solve the singular physical facets. Generic unwrapped rectangular subdivision
can refine forever at a nonregular boundary and emit zero-area triangles.
Rotated sphere charts/stereographic coordinates can be excellent general atlas
choices but need new inverse/source-domain maps, seam transports and cone handling
and do not reuse the reviewed common cap map as directly. Query-only imported
degenerate edges would falsify original inventory. A private perface physical
meridian would violate shared authority. The selected mapped cap is finite,
source-checked and compatible with the existing cached ordinary remainder.

After design review, sequence one cohesive implementation plan:

1. Literal source-cache capability and wedge snapshot schema with runtime and
   construction owners; ordinary regularity must change with factory admission.
   Add genuine source-backed RED for two distinct joint UVs without a collapsed
   original use, and for a variable-U reconstructed guide.
2. Source/trace wedge admission, branch-constant chosen proposals, unchanged
   original nodal/chord guards and fixed-profile retention. No cap result until
   the complete original trace horizon is settled.
3. Source cap plan/cut control and common CPU/P100 map-bound kernel using the
   reviewed whole-rho formula. Root adds a typed mapped-cap CUDA operation using
   the existing session/poll/allocator; no second device owner.
4. Same cached ordinary remainder, shared cut schedule, explicit fan installation
   and physical-key/contact/export integration. Component horizon routes source
   and face-cut revisions before publication. Display consumes the same map.
5. One direct original-output oracle extends the existing component/face reader:
   preserve all three source uses and literal edge chains, all UV branches and
   SAME Q, exact ordinary/cap interface chain cancellation, complete mapped
   coverage, nonzero physical facets and no new original coedge. For cap error,
   evaluate ORIGINAL analytic source on the retained Fsrc map and subtract SAME
   actual L using independent Fraction Taylor interval subdivision in xi/rho;
   this attacks the whole source-to-actual residual without reusing the production
   Hessian formula. Independent point witnesses with nonorthogonal frame/large Z
   attack the earlier wrong near-pole-band estimate. Interior samples alone are
   diagnostic, not the whole-map oracle.
6. Preselect north, south and cone authored seam wedges, reversed orientation,
   nonlinear reconstructed U proposals, finite source/pole offset and a fixed
   original-Q floor that refuses. One coincident different source vertex must
   not acquire wedge identity. Exercise one radial and one angular request through
   the actual complete component, plus one source-profile refusal without output.
   CPU/P100 bytes are a determinism check, not the geometry oracle. Bound host
   sanitizers and a single direct device memory check to this new fault class.

No production geometry callback, dependency, full imported mesh, larger work cap
or extra qualification campaign is selected by this study. The concrete open
review choice is the exact quotient-capable source-cache/type seam; the source
and pointwise cap mathematics above do not depend on a private mutable cache.
