# Tolerant geometry and healing: what is true when an edge and its surfaces disagree

Research lane of the fast-mesher study (branch `research/fast-mesher-study`).
It answers two items recorded in
[the 2026-10-17 direction journal](../journal/2026-10-17-fast-mesher-direction.md):

- Real B-reps are inexact. Which carrier is the truth when an edge and its
  surfaces disagree? The operator's bar is "the one an eye would instantly know
  preserves the coherence of the solid."
- Why must healing happen in a single pass? Could the easy work mesh at once and
  the hard cases be deferred into a batched second pass?

This is gap #1 in [the CAD test-plan study](2026-10-17-mesher-test-plans-cad.md)
(§(3)). It extends §1.4 and §3 of
[the fast-tessellation literature](2026-10-17-fast-tessellation-literature.md) and
the source-meaning proposal in
[the whole-mesher geometry contract](2026-10-16-whole-mesher-geometry-contract-study.md) §1.
Nothing was built, run or meshed.

The sources, with their read depth, are listed in
[2026-10-17-tolerant-geometry-healing-sources.tsv](2026-10-17-tolerant-geometry-healing-sources.tsv).
PDFs are in ignored `papers/pdfs/`.

**How the reading was done.** Six sub-lanes read in parallel:

- (A) OCCT edge-level healing and BRepMesh tolerance use;
- (B) OCCT wire, face and shape healing, sewing and STEP import defaults;
- (C) tolerant modelling in Parasolid, ACIS and other kernels;
- (D) the healing industry and data-quality standards;
- (E) academic repair and meshing on imperfect CAD;
- (F) staging and deferral in kernels and meshers.

OCCT is OCCT 8.0.1 at `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`, in
`quarantine/new-iteration/occt`. In code citations, `SF/` stands for
`src/ModelingAlgorithms/TKShHealing/ShapeFix/`, `SA/` for `.../ShapeAnalysis/`
and `BRepMesh/` for `src/ModelingAlgorithms/TKMesh/BRepMesh/`. The shared
`quarantine/sources/occt` snapshot differs only in `BRepLib.cxx::UpdateEdgeTol`
(the type of an array holder; behaviour is the same), and its line numbers past
:498 are shifted by about 6.

I re-read these load-bearing passages against the extracted text:

- the ACIS "TCOEDGE contains the primary geometric definition" and TEDGE
  "purely for graphic visualization" passages;
- Parasolid sewing "one of which is arbitrarily chosen to lie on the primary fin";
- Parasolid trimmed import "averaging curve ends" and "no SP-curve … less than
  twice the supplied edge tolerance";
- Jackson "arbitrary, but fixed for each edge";
- Steinbrenner's single-owner rule and why averaging was rejected;
- JAMA PDQ 0.01 mm values;
- the PDES "closure" definition;
- the Zeiss/ACIS "about 3 times the maximum gap" rule;
- `BRepMesh_EdgeDiscret.cxx:57-63, 247-277`.

**Tags**

- **[src]**: a source establishes this, at the cited location.
- **[own]**: my derivation or recommendation. No source read establishes it.
- **(inferred)**: a reader's inference from code or text that it did not state
  directly.

---

## Decision-relevant findings in one page

1. **The two mature commercial kernels make the pcurve-on-surface the truth for
   an inexact edge. The 3D curve is deleted, or kept only for display.**
   - Parasolid: a tolerant edge "is represented by a collection of curves, in the
     parameter spaces of the faces connected to it". Its tolerance is a tube
     "centred on one of the parameter-space curves. The choice of curve is
     arbitrary, but fixed for each edge" (Jackson 1995 §3.1). When precision is
     set, "the 3-space curve attached to edge E1 is deleted" (Parasolid V35 FD
     §16.3.1). Nominal 3D curves "are NOT used by Parasolid to define an edge's
     shape" (FD ch. 19).
   - ACIS: "the TCOEDGE contains the primary geometric definition, while the
     TEDGE contains the secondary". The TEDGE curve "is used purely for graphic
     visualization" (ACIS R10 kernel ch. 6; TEDGE class page).
   - OCCT and openNURBS do the opposite. The 3D curve is the reference and sets
     the parameter. Pcurves are reparameterized to it, and tolerance is measured
     from it (`Approx_SameParameter.cxx:653-766`; `opennurbs_brep.h`).
   - [src]
2. **The kernels and healers decide case by case, by the angle between the two
   surfaces at the edge.**
   - Where the surfaces cross cleanly (transversally), they recompute the exact
     edge by re-intersecting the surfaces. This is ACIS healing's sharp-edge
     solver, Parasolid `PK_EDGE_reset_precision_2`, and VDA 4955 "always form
     edges as intersection or projection curves".
   - Where the surfaces meet tangentially, re-intersection is ill-posed. They keep
     a tolerant edge on one face's representation and move or perturb the other
     face. Jackson §2.3 puts it this way: "In some cases the surfaces can be
     extended and re-intersected to impose any desired accuracy, but in the case of
     surfaces which meet smoothly this may be impossible."
   - ACIS breaks ties toward keeping the lower-curvature surface fixed ("cylinders
     are snapped to planes rather than planes snapped to cylinders"). For a spline
     tangency ACIS keeps "the isoparametric edge of one of the surfaces" and
     modifies the other (ACIS R10 Healing pp. 13-16).
   - CATIA freezes planar and canonical surfaces and moves the freeform ones.
   - [src]
   - This gives one closed-form test per edge sample [own, §7.2]. Linearize the two
     surfaces at the sample. Let d₁ and d₂ be the stored point's distances to the
     surfaces and θ the angle between their normals. The smallest move to a point
     on both surfaces is
     `|x| = sqrt(d₁² + d₂² − 2 d₁ d₂ cos θ) / sin θ`.
     Re-intersect when |x| fits the budget. Otherwise the edge is "tangent" for
     this purpose and one face's representation must own it.
3. **Every system resolves disagreement by giving the entity a tolerance region
   (a tube or ball), not by averaging geometry.** The same rules recur:
   - Vertex tolerance ≥ edge tolerance ≥ face tolerance.
   - Two entities coincide when they are within the sum of their tolerances.
   - A join takes the larger tolerance.
   - A merge produces the smallest region enclosing both inputs. Sources: Jackson
     §3.1-4.4; Segal 1990 §4.3; OCCT `ShapeBuild_Vertex::CombineVertex`; sewing
     `ComputeToleranceVertex`.
   - Averaging appears only for vertices built from unsewn data. Parasolid's
     trimmed import puts the vertex at the average of the curve ends
     (FD §90.3.2.3), and OCCT sewing glues nodes at their unweighted centroid.
   - Steinbrenner et al. considered averaging edge curves and rejected it. It
     doubles the projections and destroys the compact form of the curve (IMR 2000 §2.3).
   - [src]
4. **Watertight meshers on imperfect CAD all choose one representative curve per
   shared edge, mesh it once, and push the mismatch onto the other face.**
   - Steinbrenner: "all of the error due to surface mismatch appears on the surface
     mesh on which the edge curve is not defined".
   - Henshaw: "choose one edge-curve to define the boundary segment for both
     patches".
   - Kumar–Manocha assign each shared boundary an owner.
   - OCCT BRepMesh: the 3D curve for edges flagged SameParameter, otherwise the
     **first face's** curve-on-surface. End nodes are overwritten with the stored
     vertex points (`BRepMesh_EdgeDiscret.cxx:57-63, 247, 277`). OCCT never checks
     that `S(uv)` matches the shared node.
   - This is exactly our edge-first shared-ID design. What the literature does not
     settle is *which* representative to choose. Parasolid says "arbitrary". OCCT
     takes whichever comes first in its traversal. openNURBS
     `CombineCoincidentEdges` keeps the edge with the smaller tolerance and breaks
     ties by iso trim, lower degree, then fewer spans.
   - [src]
5. **Stored tolerances are bookkeeping, not measurements. OCCT turns them into
   coarser meshes.**
   - OCCT `BRepLib::SameParameter` may *lower* a tolerance (`BRepLib.cxx:1731`).
   - `ShapeFix_Edge::FixSameParameter` sets the SameParameter flag to true
     unconditionally and inflates the tolerance to cover any deviation
     (`ShapeFix_Edge.cxx:872-929`).
   - On STEP import, `FixConnected` closes vertex gaps up to MaxTol ≥ 1.0 model
     units by tolerance alone, without moving geometry
     (`STEPControl_ActorRead.cxx:2384`; `ShapeFix_Wire.cxx:1485`).
   - Sewing defaults to an infinite MaxTolerance and forces SameParameter=true when
     it fails (`BRepBuilderAPI_Sewing.cxx:2123, 1176`).
   - BRepMesh then floors a face's interior deflection at
     `2 × max(face, edge, vertex tolerances in the face)` (`BRepMesh_Deflection.cxx:148`;
     `BRepMesh_ShapeTool.cxx:71-79`). One vertex with tolerance 6.77 coarsens the
     whole face. That is the A319 case.
   - [src] Consequence [own]: measure the discrepancy ourselves, and never let
     stored tolerance set interior density.
6. **The industry's numbers.**
   - **Data quality.** The JAMA/JAPIA PDQ V5.1 recommended values map onto
     ISO 10303-59: face gap, edge-to-surface gap, vertex gap and loop gap
     ≤ 0.01 mm; tiny edge ≥ 0.01 mm; tiny face ≥ 1e-4 mm²; smooth ≤ 0.01°;
     crease ≥ 0.2°. Isuzu and Nissan applied these in 2025 (Nissan relaxes tiny
     elements to 0.005 and creases to 3°).
   - **Exchange validation.** CAx-IF assumes "a model accuracy of 0.02 mm". Its
     production thresholds are volume/area 0.5% and centroid 0.02 mm + 0.1% of the
     diagonal. It counts an edge as "sharp" at a normal jump ≥ 1.0°.
   - **Kernel scale limits.**
     - Jackson: an edge shorter than 4× tolerance cannot be split.
     - Parasolid import: no SP-curve shorter than 2× edge tolerance.
     - ACIS local operations make an edge tolerant only below 100 × SPAresabs.
     - ACIS spline heal tolerance = `min(10, min(face_box/10, max(3·gap, 3·deviation)))`.
     - Steinbrenner: no split within 10 × EndTol of an existing endpoint.
   - [src]
7. **Small features collapse in topology only, from the bottom dimension up.**
   - Jackson imprints vertex–vertex, then vertex–edge, then edge–edge, then the
     face cases.
   - Sheffer's virtual topology works "lowest entities upward". A sliver face is
     collapsed symmetrically, not merged into one neighbour.
   - Steinbrenner: a sub-tolerance edge "reduce[s] to singularities".
   - Huang 2023: collapse a short edge only when it dangles, and reconcile a
     vertex at the lower-dimensional intersection.
   - OCCT Wireframe merges a small edge into the neighbour with the smaller
     tangent angle, but with no continuity check.
   - [src]
8. **No mature system heals in a single pass, and none runs a free-running
   background healer either.**
   - Each one measures, then fixes in a fixed order. SMLib alternates cache and fix
     steps "very order dependent". Mezentsev–Woehler verify, then fix
     automatically, then hand leftovers to user-assisted repair.
   - Or each one samples shared edges first and retries in bounded batched rounds.
     OCCT ModelHealer pools offending edges from all faces, re-samples them at
     deflection/3 and re-checks every incident face, up to 5 rounds. Gmsh keeps a
     PENDING flag with `MaxRetries=10`; a broken shared curve un-meshes every
     adjacent face.
   - SMLib parks unfixable faces on a `NotYet` list. Steinbrenner builds all
     topology from geometry "before any grid point" and leaves the remainder to an
     explicit second pass with doubled tolerance. Shephard 1998 meshes at target
     size first, then flags the bad elements and fixes them locally.
   - The invariant common to all of them: **shared-edge data is final before any
     incident face consumes it, and a change to a shared edge invalidates every
     incident face.**
   - [src]
9. **Up-front easy/hard classification already exists in production.**
   - OCCT chooses the algorithm by surface type (`BRepMesh_MeshAlgoFactory.cxx:64-116`).
   - Gmsh routes periodic surfaces separately.
   - Mezentsev–Woehler separate "independent" errors from "badly meshable
     geometries".
   - Cubit uses mesh-size-relative small-feature tests, plus a shipped ML predictor
     of quality per entity (Owen 2019).
   - PDQ checkers are themselves classifiers.
   - Feature-adaptive subdivision and DiagSplit route regular and irregular patches
     to separate GPU paths.
   - [src]
10. **Recommendation, short form (§7) [own].**
    - Classify each edge cheaply from the samples we already take for
      tessellation, and store its measured discrepancy.
    - Commit exact and near-exact edges at once on the 3D curve.
    - Re-intersect transverse inexact edges in one batched two-surface Newton pass.
    - Give tangent inexact edges to an owner face chosen by rule (§7.2).
    - Rebuild missing pcurves by projecting our own shared samples.
    - Collapse sub-tolerance features virtually.
    - Refuse gross cases per face, explicitly.
    - Faces whose edges are all committed mesh immediately. Faces that touch a
      deferred edge wait for the batch. None of this changes the source.

---

## 1. Tolerant modelling in commercial kernels

### 1.1 Parasolid

**Jackson 1995** (read in full). Every vertex, edge and face carries a tolerance.

**Representation**:
- A vertex is a sphere. The default tolerance is 5.0e-9 m (§2.3, p. 248).
- An accurate edge has one 3D curve. A tolerant edge has one pcurve per incident
  face and no 3D curve.
- Its tolerance region is a tube of radius equal to the tolerance, "centred on
  one of the parameter-space curves. The choice of curve is arbitrary, but fixed
  for each edge". All the edge's pcurves must lie inside that tube.
- Pcurve ends must lie inside the vertex sphere. The curves bounding a face "need
  not meet exactly end-to-end" (§3.1, p. 249).
- Vertex tolerance ≥ the tolerance of every connected edge ≥ that of every
  connected face (§3.2).

**Decision rules**:
- "tolerances are associated with topology, not geometry. Points, curves and
  surfaces are treated as 'exact'" (§3.3).
- Coincidence uses the **sum** of tolerances.
- When a split or face-add collides with existing tolerances, "the local
  tolerances are increased, and redundant topology removed, until a consistent
  model can be created" (§3.3).
- Imprinting compares low dimensions first (V–V, V–E, E–E, V–F, E–F, F–F). A
  vertex that splits an edge inherits the edge's tolerance (§4.3).
- Joins use the larger tolerance (§4.4).
- "It is not possible to split a short edge, of length less than 4 times the
  tolerance" (§2.3).
- When two new vertices coincide, they merge and the degenerate thin face is
  removed (§4.3, Fig. 5).
- Sewing is the edge-matching half of the Boolean. An unclosed gap is closed by
  "assigning larger tolerances to the edges and re-sewing" (§6).
- **Limit:** tolerances grow under repeated operations, and containing that
  "seems not to be achievable in all cases" (§7.3). [src]

**XT format reference (2006, checked against V35 2022), model-structure
sections.**
- A fin's curve "is only non-null if the fin's edge is tolerant, in which case
  every fin of that edge will reference a trimmed SP-curve".
- The fin curve "must not deviate by more than the edge tolerance from curves on
  other fins of the edge, and its ends must be within vertex tolerance".
- The edge's own curve "will be null if the edge has a tolerance".
- An accurate edge's curve lies in its faces' surfaces "to within modeller
  resolution".
- An accurate edge with tolerant vertices uses a trimmed curve whose ends sit on
  the basis curve and within vertex tolerance (p. 46).
- Face tolerance is "not used" (p. 90).
- Intersection curves are stored exactly: the two surfaces, a chart of points
  with parameters on both, and the limits (pp. 42-45).
- Default linear precision is 1e-8, in a size box of 1000. [src]

**Parasolid V35 Functional Description** (§16.3, ch. 19, §30.3, ch. 46, §90; read).

*Setting precision*
- After an edge gets local precision it "is represented geometrically by two
  SP-curves attached to its fins. The 3-space curve … is deleted."
- Vertices are raised to the edge precision.
- Edges are split at G1 discontinuities of the surface and at parameter
  singularities (§16.3.1).
- Generated SP-curves are non-rational quadratic curves (§90.3.1.2).

*Removing precision.* `PK_EDGE_reset_precision_2` "computes the position of the
new curve by finding the intersection of the underlying surfaces … may not always
be successful if the adjacent surfaces are tangent or nearly tangent". Its
options: accept any curve within session precision on both surfaces, prefer the
true intersection, or require the true intersection (§16.3.1.4).

*Import repair* (§90.4.6). `PK_EDGE_repair` measures the distance from each edge
curve to its adjacent surfaces and sets tolerances, *or* computes an accurate
curve by intersecting the faces. Non-tangent edges can then be made accurate
again, or "the edges may be left with local tolerances".

*Trimmed import* (§90.3.2.3).
- SP-curves need not meet: "Points for vertices are computed by Parasolid by
  averaging curve ends … P is the average centroid point computed using end of C1
  and start of C2. The tolerance is the maximum distance of C1 or C2 away from P."
- "No SP-curve should have a length less than twice the supplied edge tolerance."
- SP-curves must not approach within the sum of tolerances of SP-curves they do
  not share a vertex with.
- Faces that lie wholly inside the tolerance band are rejected.

*Sewing* (`PK_BODY_sew_bodies`, ch. 46).
- "fusing edge topologies together and applying a tolerance that engulfs the
  geometries of the edges that were so combined (one of which is arbitrarily
  chosen to lie on the primary fin)".
- It never picks a tolerance below any existing tolerance on the fused edges.
- Vertex tolerances are minimised.
- With `treat_as_manifold`, only the two closest edges match.
- A match must be bounded by vertices.
- Incremental sewing runs from small gap bounds to large ones.
- Measured gap = edge deviation − the sum of the edge precisions.

*Check faults* (§30.3). These include `bad_spcurve` ("SP-curves of edge not
within tolerance"), `bad_vertex` and `vertices_touch`. [src]

**Unresolved wording conflict.** Jackson describes a tube whose *radius* is the
tolerance. Parasolid's Nominal Geometry chapter says a pipe whose *diameter* is
the precision. XT and ACIS define tolerance by maximum pairwise deviation. Treat
tolerance as a bound on pairwise fin deviation. [src; conflict noted by lane C]

### 1.2 ACIS

**Kernel manual ch. 6 (R10), DocR21 wiki, TEDGE/TCOEDGE/TVERTEX pages** (read).

*Primary geometry and tolerances*
- For an exact edge, "the EDGE contains the primary geometric definition and the
  COEDGE the secondary". For a tolerant edge, "the TCOEDGE contains the primary
  geometric definition, while the TEDGE contains the secondary".
- A TCOEDGE must carry a pcurve, even on analytic surfaces. It has a lazily
  derived 3D curve.
- Edge tolerance = "the maximum distance between any two equiparametric
  positions on any of its tolerant coedges". A single-coedge tolerant edge has
  tolerance 0.
- Vertex tolerance = the maximum distance from the vertex to each coedge end.
- Coincidence is tested at max(tol, SPAresabs).
- Loops with tolerant entities need not close within SPAresabs.
- Tested up to "about 10 percent of the model size".

*When ACIS makes an edge tolerant* (pp. 5-7)
- Local operations make an edge tolerant only "when the tolerance will be less
  than 100.0 * SPAresabs".
- A plane–cone edge that is tangent within tolerance but is not a line becomes
  tolerant, and a straight line is substituted.
- "Non-tangent tolerant edges … are reverted to ordinary edges by using the
  precise geometry from the intersection of the edge's adjacent surfaces."
- If a vertex cannot be solved, it is made tolerant and the operation is retried.

*Tolerize APIs*
- `api_tolerize_entity` tolerizes gaps above SPAresabs, subject to a `max_gap`.
- If an edge is shorter than its vertices' gaps, the result has "overlapping
  tvertices".
- `api_check_edge_errors` needs sphere edges split at the poles. [src]

**Healing component manual (R10 ch. 1, 33 pp., read in full).**

*Phases.* Preprocess, simplify, stitch, then build geometry. Geometry building
runs five ordered solvers: analytic, isospline, sharp-edge, generic spline, then
wrap-up. Each phase has analyze, calculate and fix stages.

*How each kind of junction is fixed*
- **Transverse:** extend the surfaces and recompute the intersection.
- **Analytic tangent:** snap surfaces (translate, rotate, scale) with a graph
  solver. Cycles are broken "by giving preference to surfaces of higher curvature
  (e.g., cylinders are snapped to planes rather than planes snapped to
  cylinders)".
- **Isoparametric spline tangency:** "the new edge geometry is defined as the
  isoparametric edge of one of the surfaces, and the adjoining surface is modified
  to pass through this edge curve".
- **Sharp edges:** surface–surface intersection. Open edges are projected.
  Vertices come from curve–curve intersection or projection.
- **Wrap-up:** "recalculates coedge geometry (pcurves…) for all coedges". During
  healing, pcurves are derived *after* the 3D geometry is fixed.

*Stitch tolerance*
- Logarithmic steps from 1e-5 up to a maximum set by body box size: 1e-4 below a
  box of 0.01, rising a decade at a time to 1.0 above 10.
- Each step uses `min(current, min(min_edge_length/edge_factor, face_box/box_factor))`.

*Geometry-building tolerances*
- Analytic translation and scale: 1e-6 to 0.01 by box size. Rotation: 1e-3 rad.
- Spline edge tolerance = `min(10, min(min_face_box/10, max(3·stitch_gap, 3·deviation)))`.
- Vertex tolerance = the maximum of its incident edges' tolerances.

*Fallback.* Edges that cannot be healed become TEDGEs (`api_hh_make_tolerant`). [src]

**Modern stitching (STITCHAPI page).**
- Tolerant stitching "can apply the minimum tolerance necessary". If exact
  stitching would succeed, it creates nothing tolerant.
- Set `max_stitch_tol` "smaller than the minimum feature size and greater than
  the maximum gap". Edges smaller than it may be removed.
- `api_tighten_gaps` defines an edge gap as the distance from the curve to *all*
  of its adjoining surfaces. [src]

**Zeiss Calypso note on ACIS healing (2002, read).**
- Stitching starts small and rises step by step until every face stitches at one
  tolerance.
- Presets: simplify 0.010 mm; stitch from 1e-5 mm up to 1 unit; geometry build
  0.10 mm.
- The geometry-build tolerance should be "around 3 times the maximum gap size".
- Loosening simplification can widen gaps. [src]

### 1.3 Other kernels and CAD systems

**openNURBS** (header and source at `eb92af3`, read).
- Edge tolerance is "the maximum distance from the edge's 3d curve to any
  surface".
- Trim tolerance is defined from the 3D curve's closest surface points.
- Vertex tolerance = 1.001 × the maximum distance to the 3D-curve ends and to
  S(trim ends).
- `CombineCoincidentEdges` keeps the edge with the smaller tolerance. Ties at 0
  go to an iso trim, then the lower degree, then fewer spans.
- Rhino's own edge-tolerance computation is in closed `TL_Brep`.
- Rhino `RebuildEdges` "restores original 3-D surface edges that have been forced
  away from the surface", which makes the surface plus trim the regeneration
  authority. This is a secondary reading through a WebFetch summary.
- [src]

**SolidWorks Import Diagnostics (2011).**
- "Repair faulty faces first … Repair gaps last."
- Heal Gap adjusts the surrounding faces or knits a new face into the gap.
- SolidWorks treats gaps ≤ 1e-5 mm as coincident. Above that it loosens its
  tolerance, up to 1 mm, and retries (JAMA CAD V4.1 p. 62).
- [src]

**CATIA V5 healing.**
- Merging distance defaults to 0.001 mm; larger gaps "are not processed".
- Distance objective is 0.001-0.1 mm; tangency objective 0.5°.
- "Freeze Planar/Canonic elements": analytic surfaces stay fixed and freeform
  surfaces deform.
- [src]

**PTC Creo GeomIntegrityCHECK defaults.**
- Position < 0.02 mm, tangent < 0.1°, curvature < 10%.
- Tiny elements > 0.02 mm; vertex-to-edge and vertex-to-face < 0.02 mm.
- Minimum curvature radius 0.5 mm.
- [src]

### 1.4 Semantics papers

- **Segal 1990** (read in full).
  - **Consistency:** "every intersection among an arbitrary set of features is
    represented explicitly in the connectivities, and conversely". Inexactly, an
    intersection means the tolerance regions overlap, with ε = εᵢ + εⱼ (§3.2).
  - **Merging:** a merged region must contain both originals and be minimal, the
    smallest sphere or cylinder that encloses them. After a vertex merge, edges
    sharing both endpoints merge and triangular contours collapse (§4.3).
  - **Backtracking:** each feature keeps a "near" value, the distance to the
    closest feature already judged non-intersecting. If a merged tolerance exceeds
    it, the algorithm restarts.
  - **Failure mode:** cascading growth reduces a cube to a single vertex after 11
    Boolean iterations (§5). [src]
- **Qi, Shapiro & Stewart 2005** (§1-2).
  - "Single-set" semantics perturbs faces to fit exactly, by at most the largest
    boundary discrepancy already present.
  - "Class-of-sets" semantics instead keeps a thick δ-cover boundary.
- **Qi & Shapiro 2006.** A tolerant model is valid if some exact solid exists
  inside the zones, but that "is not practically verifiable". Healing "alter[s]
  the original geometric data". [src]
- **Hoffmann 2001.** Nearness is not transitive. "Every incidence test benefits
  from working with the primary geometric data." [src]
- **Sakkalis, Shen & Patrikalakis 2001** (method). Gives box-cover conditions
  under which the thickened solid is homeomorphic to the intended one. This is a
  formal statement of "coherent". [src]
- **Marussig & Hughes 2017** §3.1.6 (method+results).
  - An intersection yields three independent fits: the 3D curve and both trims.
    The 3D curve "does not lie on either of the intersecting surfaces", and the
    trim images "do not coincide, neither with each other, nor with Ĉ".
  - ACIS stores a 3D B-spline, Parasolid sorted intersection points, and C3D no
    3D curve.
  - Inside/outside needs the parameter-space trim; with only the model-space
    curve it is "ill-defined" (§3.2.1).
  - ACIS defaults: 1e-6 for point comparison, 1e-3 for approximation. [src]

---

## 2. OCCT ShapeFix, ShapeAnalysis, BRepLib and BRepMesh, from the code

### 2.1 Edge level: SameParameter, SameRange, pcurve and 3D-curve construction

**`BRepLib::SameParameter(edge, tol, newTol, useOld)`** (`BRepLib.cxx:1251-1736`).

*Preconditions and measurement*
- Returns if the edge is already flagged SameParameter. **Does nothing without a
  3D curve** (:1256, :1263-1266). The 3D curve is never modified.
- Per pcurve (both, on a seam): SameRange first (`GeomLib::SameRange`), then
  `ComputeTol` with NCONTROL=22, i.e. 23 equal parameters comparing `C3d(u)` with
  `S(pcurve(u))` (:1070-1188).
  - A UV point more than 1% of the range outside the domain contributes
    `overshoot/Resolution`.
  - If fewer than 10% of the samples have squared distance ≥ 1, those samples are
    dropped as outliers.
  - Result = 1.5 × the maximum distance, floored at 1e-7. Above 1e10 the pcurve is
    abandoned.
- C0 B-spline pcurves are converted to C1 first. The conversion is kept only if
  the error does not grow. An arc-length re-approximation is used when the knot
  ratio exceeds 10 and the error exceeds max(1e-3, tol) (:1404-1621).

*Reparameterization* (`Approx_SameParameter`)
- 22 initial samples, at most 1000 (`.hxx:161-162`).
- Each non-matching sample `COS(t)` is projected onto the **3D curve**: local
  Newton (30 iterations), then a global `Extrema_ExtPC`. The projected parameter
  must be monotone (:653-766).
- If more than 30% of the projections are lost, it gives up (:353-373).
- Otherwise it fits a cubic B-spline to the parameter map and composes it with
  the pcurve (degree ≤ 11, ≤ 1000 segments). It accepts when
  `TolReached < 250 × besttol` and refines otherwise. If that is exhausted, it
  keeps whichever of the original and the approximation is better (:318-543).
- **The UV trace is reparameterized, never moved** (inferred from the
  composition).
- A new pcurve is adopted only if its 44-interval error is no larger than the old
  error (`BRepLib.cxx:1651`).
- After a failure, the edge is still accepted if `edgeTol + max(PrecCurve,
  PrecSurface) ≥ error` (:1704-1712).

*Write-back*
- On success: `TE->Tolerance(max(maxdist, 1e-7))`. This is a **direct set that
  can lower** the tolerance (:1723-1734).
- Vertices only grow (`UpdateVertex`, :1222-1234).

**`BRepLib::BuildCurve3d`** (:301-455).
- If any pcurve lies on a plane: an exact lift.
- Otherwise it approximates `S(pcurve)` from **the first stored pcurve only**
  (:384-404). Defaults: tolerance 1e-5, C1, degree 14, `30 + knots` segments.
- The recorded tolerance is `max(edgeTol, requested)`. The line that would record
  the measured deviation is commented out as a "Patch" (:434-435).

**`BRepLib::UpdateTolerances`** (:1744-1962). This is the **only** place that
enforces vertex ≥ edge ≥ face, and only when it is called:
- Edge tolerance ≥ the maximum face tolerance.
- Vertex tolerance ≥ max(incident edge tolerances, distance to every
  representation evaluated at the vertex parameter) + 2ε.
- With `verifyFaceTolerance`, face tolerances are reset to a box-relative 1e-7 to
  4e-7, capped at 0.99. [src]

**`ShapeFix_Wire::FixEdgeCurves` order** (`ShapeFix_Wire.cxx:600-1030`):
1. FixReversed2d
2. FixRemovePCurve (off)
3. FixAddPCurve. Projects the 3D curve, and splits at surface singularities.
4. Over-degenerate re-projection, when the wire's 2D sum is within 10% of the U
   range.
5. FixRemoveCurve3d (off)
6. FixAddCurve3d. Builds the 3D curve from the first pcurve.
7. FixSeam (off)
8. FixShifted
9. FixSameParameter. A pcurve that fails the range check is removed and
   re-projected first.
10. FixVertexTolerance

[src]

**`ShapeFix_Edge::FixAddPCurve`** (`ShapeFix_Edge.cxx:470-614`) and
**`ShapeConstruct_ProjectCurveOnSurface`**.
- No pcurve is stored on a `Geom_Plane`; `BRep_Tool::CurveOnPlane` projects one
  on the fly.
- A 3D curve is required.
- Precision = `prec` if positive, else the edge tolerance.
- Seam pcurves are copied and shifted by the period.
- **The new pcurve is stored with tolerance 0**; the later FixSameParameter
  measures it.
- Projection tries, in order (:708-775):
  1. analytic projection onto a plane (or a trimmed or offset plane);
  2. full `ProjLib` for uneven B-splines (speed ratio > 1500);
  3. a straight UV line, if every sample reproduces within tolerance;
  4. point-by-point `NextValueOfUV`, seeded from the previous UV, with pole
     snapping within vertex tolerance and period unwrap at jumps > U/2;
  5. interpolation through all points, at tolerance `preci/(100N)`.
- Sample count: 23, raised for B-splines in steps of 22 to `knotSpans·(degree+1)`.
- `approximatePCurve` has no caller, so it always interpolates. [src]

**`ShapeFix_Edge::FixSameParameter`** (:798-936).
- Runs `BRepLib::SameParameter` on a copy.
- **Then sets SP=true on the original unconditionally** (:872).
- Measures the deviation with `ShapeAnalysis_Edge::CheckSameParameter`: 23
  points, through `BRepLib_ValidateEdge`, ×1.00001.
- Keeps the variant (original or BRepLib) with the smaller tolerance.
- Grows the edge and vertices to cover the deviation. **No MaxTolerance cap.**
- Net effect: every non-SP edge comes out SP. Where reparameterization did not
  help, the tolerance has absorbed the error. [src]

**Vertex reconciliation.**

| Function | Rule | Location |
|---|---|---|
| `ShapeAnalysis_Edge::CheckVertexTolerance` | tolerance = `max(1.0000001 × max distance to C3d ends and to every S(pcurve end), edgeTol)` | :656-657 |
| `FixRemovePCurve`, `FixRemoveCurve3d` | delete a representation whose ends miss the vertices; **vertices beat representations** | `ShapeFix_Edge.cxx:87-113` |
| `ShapeFix::FixVertexPosition` (opt-in) | group incident 3D-curve ends that lie within the target; put a new vertex at their running average with tolerance 1e-7; split outliers onto their own vertices; **the 3D curve wins**, pcurves are untouched | `ShapeFix.cxx:565-789` |
| `ShapeBuild_Vertex::CombineVertex` | smallest sphere enclosing both tolerance balls, biased toward the larger ball, ×1.0001 | `ShapeBuild_Vertex.cxx:38-72` |

[src]

**`BRepCheck_Edge`.**
- Reference = the 3D curve if present, else the first pcurve-on-surface.
- 23 points.
- Pass if `edgeTol + max(PrecCurve, PrecSurface) > dist`. The Prec terms are
  non-zero only for ellipses and cones.
- `BRepCheck_Vertex` does not require vertex ≥ edge.
- `BRep_Tool::Tolerance` returns max(stored, 1e-7).
- [src]

### 2.2 Wire, face and shape level

**Mode flags.** `NeedFix(Flag, need)` returns `Flag<0 ? need : Flag>0`
(`ShapeFix_Root.lxx:72-75`). The default -1 therefore means "use this fix's own
trigger". `LimitTolerance` exists, but **no ShapeFix fix calls it**, so
MaxTolerance acts only as an accept/reject gate inside particular fixes. [src]

**`ShapeFix_Shape::Perform`** (`ShapeFix_Shape.cxx:83-303`).
- Dispatches by type. Topology modification is forced on only when the top-level
  shape is a FACE or WIRE. It stays off for SOLID and SHELL inputs, and STEP also
  sets it to 0.
- Ends with `ShapeFix::SameParameter` and a per-edge `FixVertexTolerance`.
- **Result: healing depends on how the shape is presented.** [src]

**`ShapeFix_Wire::Perform` order** (:297-483). The face calls it in two passes
with different fixes masked (`ShapeFix_Face.cxx:380-385, 524-531`):
1. FixReorder
2. FixSmall (only in topology mode)
3. FixConnected
4. FixEdgeCurves
5. FixDegenerated
6. FixNotchedEdges
7. FixTails (off)
8. FixSelfIntersection, then reorder
9. FixLacking
10. FixVertexTolerance on every edge

FixGaps3d and FixGaps2d run only from ShapeFix_Wireframe. [src]

**Thresholds in the individual wire fixes.**

*FixSmall* (`SA/ShapeAnalysis_Wire.cxx:765-835`; `SF/ShapeFix_Wire.cxx:1404-1472`)
- An edge is small if its endpoints are within ε **and** the curve midpoint is
  within ε of both.
- ε = MinTolerance, which is 1e-7 on STEP import.
- The small edge is dropped and its neighbours' vertices merged by CombineVertex.
  Neighbour curves are not touched.

*FixConnected* (`SA/ShapeAnalysis_Wire.cxx:693-761`) grades the gap between
consecutive vertices:

| Gap | Status |
|---|---|
| ≤ gp::Resolution | DONE1 (reuse one vertex) |
| ≤ Precision | DONE2 |
| ≤ MaxTolerance | DONE3 |
| otherwise | FAIL |

DONE2 and DONE3 are closed by CombineVertex. **No curve moves; tolerance alone
covers the gap.**

*FixDegenerated* (`SA/ShapeAnalysis_Wire.cxx:896-1112`;
`SA/ShapeAnalysis_Surface.cxx:549-575`)
- A missing pole edge is detected when the 2D extent is **more than 10×** the 3D
  extent (resolution-normalized) and the gap lies within 2× the vertex tolerance
  of a surface singularity.
- It inserts a degenerated edge along a 2D line.

*FixSelfIntersection* (`SF/ShapeFix_Wire.cxx:2708-3265`)
- A self-crossing pcurve: try cutting out the loop; otherwise grow the nearer
  vertex to 1.001 × the distance, if that is below MaxTol.
- Adjacent edges that cross: first grow the edge tolerances to the local chord
  deviation (10 samples). Otherwise, in topology mode, cut both edges at the
  crossing and move the vertex there.

*FixLacking* (`SF/ShapeFix_Wire.cxx:3617-3973`). Applies when the vertex is shared
in 3D but the 2D gap is ≥ `2·max(Ures, Vres)(tol)`. The ladder:
1. Bend both pcurves to meet at the 2D midpoint.
2. Else, if the 3D equivalent of the gap is < Precision, grow the vertex
   tolerance.
3. Else, in topology mode, add a 3D edge if the 3D distance² exceeds
   1.25·(t₁+t₂)².
4. Else, below MaxTol, bend or grow.
5. Else add a degenerated edge.

*FixGaps3d* (`ShapeFix_Wire_1.cxx:204-206, 425-470`)
- Converts the curve to a BSpline and moves its **end pole to the mean of the
  two vertex points**.
- Resets tolerances to Confusion.
- Pcurves are left stale until the next SameParameter.

[src]

**`ShapeFix_Face`** (:345-781). The run order is:
1. AutoCorrectPrecision: precision = `min(prec, LeastEdgeSize/2)`.
2. Wire pass 1.
3. FixPeriodicDegenerated.
4. FixMissingSeam: needs the open-ness of the wires to be within 10% of the
   period.
5. Per resulting face:
   - wire pass 2, plus loop-wire splitting;
   - FixWiresTwoCoincEdges;
   - FixIntersectingWires;
   - FixOrientation: the outer wire is the one that contains the others under a
     2D classifier;
   - FixAddNaturalBound;
   - FixSplitFace.
6. FixSmallAreaWire is **off by default**, except for a top-level FACE. When on,
   it drops a wire with area < 0.5·perimeter·Prec. [src]

**Small edges and small faces.**

*`ShapeFix_Wireframe::FixSmallEdges`* (:726-1150): **the closest thing OCCT has to
an explicit collapse heuristic.**
- Ang1 is the tangent angle between the previous edge and the small edge; Ang2
  between the small edge and the next.
- A neighbour qualifies only if it has the same face set and seam flag as the
  small edge.
- If both qualify, the small edge merges into the neighbour with the **smaller
  tangent angle**. Ties go to the previous edge.
- `ShapeConstruct::JoinCurves` moves the shared end poles to their midpoint and
  concatenates the curves.
- There is no continuity check, and `LimitAngle` is off by default.

*`ShapeFix_FixSmallFace`*
- **Spot face** (bounding box and every edge within tol): collapse to the
  **unweighted mean** of its vertices.
- **Strip face** (two non-small edges that project onto each other at 11 samples
  within tol): replace by the **first** edge's 3D curve, with end vertices moved
  to the pair midpoints.
- Code defects:
  - The second vertex pair gets the full distance d rather than d/2 (:488-489).
  - The F2 pcurve is commented out (:629-634).
- Runs only through the `FixFaceSize` operator, never by default. [src]

### 2.3 BRepBuilderAPI_Sewing: which edge wins

*Defaults:* tolerance 1e-6, `MinTolerance = tol·1e-4`, **`MaxTolerance =
Precision::Infinite()`** (`BRepBuilderAPI_Sewing.cxx:2105-2133`).

*Stages* (:2188-2343):
1. Face analysis. A small edge, `2·max|C(t)−mid| ≤ MinTol` over 5 samples,
   becomes degenerate. Its vertices glue at the unweighted mean.
2. Free boundaries.
3. Vertex assembling. Cell-filter candidates within `tol` are found.
   - A node never merges with another node on the same face's wire.
   - The merged node sits at the **unweighted centroid**, with tolerance
     `max(|c−v|+tol(v))`.
   - This repeats until stable, so merges are transitive.
4. Cutting.
5. Merging.
6. Edge processing.
7. SameParameter on every edge.

*Candidate edges*
- `EvaluateDistances`: 8 samples at **equal parameter**, compared both forward
  and reversed. A match needs `dist < tol`, and at least half of the samples must
  fall inside the reference's ends.
- Manifold mode needs **mutual nearest neighbours**, checked recursively
  (:2007-2052).

*Merged-edge geometry* (`SameParameterEdge`, :662-1186)
1. **The longer edge (by arc length) is the master**: its 3D curve, range and
   tolerance survive.
2. The other edge's pcurves are linearly re-ranged and attached, then
   `BRepLib::SameParameter` runs.
3. If that fails or exceeds tol, the other edge is tried as master, and the
   lower-tolerance result is kept.
4. If both fail, it forces a sampled tolerance and sets SP=true anyway.

There is a defect in that fallback: `maxTol` is assigned per pcurve, not
accumulated, so only the last pcurve counts (:1146, :1160).

*Merged-vertex position* (`ComputeToleranceVertex`, :576-660)
- Two vertices: smallest enclosing sphere.
- Three vertices: centroid.

[src]

### 2.4 Default STEP import healing

*Precision settings*
- Base precision = the file's `uncertainty_measure_with_unit` × the length
  factor. The fallback is `read.precision.val`, which the reader's statics set to
  1e-3 (`Interface_StaticStandards.cxx:33-47`).
- **MaxTol = max(precision, `read.maxprecision.val`=1.0)**
  (`STEPControl_ActorRead.cxx:2370-2384`).
- MaxTol clamps tolerances only in "Forced" mode.

*Per edge, during translation* (`XSAlgo_ShapeProcessor.cxx:344-505`)
- `CheckPCurve` drops a pcurve that spans more than 6/8 of the surface range
  ("Why exactly 6/8? No idea"), or whose ends miss the 3D ends by more than the
  precision.
- It then runs FixSameParameter. If the resulting tolerance exceeds
  `min(1, 2·prec)`, or the edge is not SameRange, it re-projects and keeps the
  better result (verified at :449).

*After translation*
- `FromSTEP.exec.op : FixShape` runs with `ModifyTopologyMode=0`,
  `FixEdgeSameParameterMode=0` and `MinTolerance3d=1e-7`.
- Consequences:
  - FixSmall is off.
  - FixConnected closes gaps up to ≥ 1.0 by tolerance alone.
  - Shape-level `ShapeFix::SameParameter` still runs.
  - No sewing runs.

[src]

The OCCT STEP user guide gives the pcurve-recompute trigger as `max(1.0, 2 ×
base precision)`. The code reads `min(1, 2·prec)`. Lane D read the guide and lane
B read the code; **the code governs.** [src, discrepancy noted]

### 2.5 How BRepMesh consumes tolerances and chooses boundary geometry

*Edge positions* (`BRepMesh_EdgeDiscret.cxx:57-63`, verified)
- **3D positions** come from `BRepAdaptor_Curve(edge)`, the 3D curve, when the
  edge is SameParameter.
- Otherwise they come from the curve-on-surface of **the first pcurve's face**.
- **End nodes are overwritten with `BRep_Tool::Pnt(vertex)`** (:247, :277,
  verified).
- One polyline per edge is shared by every face, through `myDEdgeMap`
  (`BRepMesh_ShapeVisitor.cxx:47-53`).

*Edge UVs* (`BRepMesh_EdgeParameterProvider.hxx:65-140`)
- SameParameter edges: `pcurve(t)`.
- Others: the 3D node is projected onto that face's curve-on-surface at 1e-9.
  The result must be monotone.
- **There is no check that `S(uv)` reproduces the shared node.**

*Edges without a 3D curve* are marked degenerated (`BRepMesh_ShapeTool.cxx:110-116`),
so they emit only their end vertices (inferred).

*Deflection floors* (`BRepMesh_Deflection.cxx:67-154`)
- Edge deflection ≥ the vertex-to-curve-end distance.
- Face interior deflection ≥ `2 × MaxFaceTolerance`, where `MaxFaceTolerance` =
  max(face, every edge, every vertex tolerance in the face). The exception is
  `ForceFaceDeflection`.

*ModelHealer* (`BRepMesh_ModelHealer.cxx:173-515`)
- Snaps consecutive edges' end UVs to the closest neighbour pair **with no
  distance threshold**.
- Flags wires that cannot be connected as OpenWire.
- Re-samples self-intersecting edges at deflection/3 for up to 5 rounds before
  marking Failure (§6.1).

[src]

### 2.6 What OCCT gets wrong or leaves to luck

- **Tolerance growth has no bound.** No fix calls `LimitTolerance`, and sewing's
  MaxTolerance is infinite. [src]
- **"Healed" is a flag, not a measurement.** SameParameter is forced true
  (`ShapeFix_Edge.cxx:872`; sewing :1176). [src]
- **Results depend on traversal order.**
  - Backward wire scans (`ShapeFix_Wire.cxx:546, 567`).
  - Sewing's reference is whichever bound is visited first (:3798, :4381).
  - Strip faces take the "first" edge.
  - BRepMesh non-SP edges take the "first" face.
  - BuildCurve3d uses the "first" pcurve.

  [src]
- **Distances are compared at equal parameter, not at nearest point**, in sewing
  (8 samples) and in fallbacks (23). Edges with different parameterizations then
  look far apart, or close, by accident. [src]
- **Pcurve and 3D changes interleave without a consistency check.** FixGaps moves
  3D poles and leaves the pcurves stale. [src]
- **Arbitrary constants:**
  - 23 samples
  - 0.1 rad notch angle
  - 1.25 lacking-edge factor
  - 10× degenerate-edge ratio
  - 0.9π back-step angle
  - 6/8 pcurve span
  - 10% period window

  [src]
- **Stored tolerance drives mesh density.** [src]

---

## 3. The healing industry and data-quality standards

### 3.1 Standardized criteria and values

| Criterion (ISO 10303-59 measure) | Source | Value | Meaning |
|---|---|---|---|
| G-SH-LG `gap_between_faces_related_to_an_edge` / `gap_between_pcurves_related_to_an_edge` | JAMA V5.1 §3.2.19 | ≤ 0.01 mm | Maximum face-to-face gap along a shared edge, measured from the 3D edge or from the stored pcurves |
| G-FA-EG `gap_between_edge_and_base_surface` | JAMA V5.1 §3.2.15 | ≤ 0.01 mm | Edge off its face's surface |
| G-FA-VG | JAMA V5.1 §8.1 | ≤ 0.01 mm | Vertex off its edge or surface |
| G-LO-LG | JAMA V5.1 §3.2.13 | ≤ 0.01 mm | Endpoint gap between adjacent edges of a loop |
| G-ED-TI | JAMA V5.1 §3.2.12 | ≥ 0.01 mm | Minimum edge length |
| G-FA-TI | JAMA V5.1 | ≥ 1e-4 mm² | Minimum face area |
| G-FA-NA | JAMA V5.1 §3.2.17 | L = 0.01 mm | Narrow face: the whole face lies within L/2 of its boundary. The guideline says manufacturing should use larger L |
| G-SU-DC | JAMA V5.1 §3.2.8 | L0 = 1e-5, L1 = 0.01 mm | Flag near-degenerate boundaries in [L0, L1]. Fully collapsed boundaries are excluded on purpose, as CAD systems special-case them |
| Self-intersection (CU, LO, SH) | JAMA V5.1 | d < 0.01 mm with arc separation > 0.01·α | Two points close in space but far apart along the entity |
| G-SH-NT | JAMA V5.1 §3.2.28 | smooth ≤ 0.01°; crease ≥ 0.2° | Normal jump across an edge. 0.01-0.2° is an ambiguous band (inferred) |
| G-SH-NM | JAMA V5.1 §3.2.30 | 0 | No edge used by ≥ 3 faces |
| O-CM-AP | JAMA V5.1 §4.2.1.3 | 0.01 mm / 0.01° / 0.01 mm, absolute | Sender and receiver must agree on system accuracy |
| Applied, Isuzu (NX Check-Mate) | JAMA 2025 p. 2 | 0.01 mm gaps and tiny elements; 0.5° tangency | Production |
| Applied, Nissan | JAMA 2025 p. 11 | gaps ≤ 0.01; tiny ≥ 0.005; tangency ≤ 3° | Production |
| Creo VDA checker | PTC docs | 0.02 mm / 0.1° / 10%; radius ≥ 0.5 mm | Tool defaults |
| GVP production OK | CAx-IF 4.6 Fig. 13 | volume, area 0.5%; centroid 0.02 mm + 0.1% of diagonal (size > 20 mm) | Exchange acceptance |
| GVP testing bands | CAx-IF 4.6 §4.13.1 | 1% / 10%; 1 / 5 mm; 0.1% / 1% of diagonal | Green / yellow / red. "Assuming a model accuracy of 0.02 mm, the minimum model size is 20 mm" |
| Cloud-of-points sharp edge | CAx-IF 4.6 §6.2 | 1.0° | Non-manifold edge, or normal jump ≥ 1°: "a 1.0 degree limit seems to be practical" |
| STEP "closure" uncertainty | PDES AP203 RP §2.7.1 | the largest gap expected | "the largest gap anticipated between elements that should be deemed coincident". A larger gap is an error |

[src]

**VDA 4955 v4.1** (the German version of SASIG PDQ V2.1, read). It gives criteria
and repair advice, with numbers left to company standards.

- **Loop gap:** re-trim the existing curve ends to meet. Do not insert a tiny
  bridge curve, which risks unwanted loops (p. 40).
- **Edge off surface:** "always form edges as intersection or projection
  curves". Re-project in a more accurate system (p. 43).
- **Vertex off curve:** project the vertex onto the curve or face where possible,
  else rebuild (p. 43).
- **Face gap:** a gap acceptable at today's tolerance can lose topology in
  another system, or trigger automatic healing that creates tiny elements. Fix by
  rebuilding the faces on a common boundary curve (p. 48).

[src]

**PDES AP203 Recommended Practices (1998).**
- The receiver "shall use" the closure uncertainty for error checking.
- App. H.1.1: the "Receiving system chooses whether to use the 3D curves or
  recalculate them."
- App. H.1.8: tangency breaks inside a face must appear as topology.

[src]

### 3.2 CADfix, the ACIS presets and other vendors

- **Butlin & Stops 1996** (IMR5, read). CADfix's founding paper.
  - It lists 18 kinds of geometric and topological "insanity".
  - Every check depends on a tolerance and on local judgement, so each find/fix is
    vetted by the operator.
  - It proposes batch "laundry" macros.
- **CADfix release notes.** DX13 (2023, p. 12) offers a tolerance scheme of
  "auto (default), fixed or user defined", plus an absolute mode. The auto default
  scales with model size (inferred). 12/SP2 (2021) reports mass-property deviation
  at each wizard stage, with user pass/fail thresholds.
- **Machine Design (2000), secondary.** CADfix import merges points and edges
  within a tolerance it suggests, and its repair stage "pulls gaps together".
- **CADCEUS** deletes, on import, faces uniformly ≤ 0.005 mm wide (JAMA CAD V4.1
  p. 38).
- **Pro/ENGINEER** reads gaps below its accuracy as zero and gaps above it as
  separate surfaces.
- **Several systems** use an internal 1e-5 mm and refuse to build below 1e-4 mm
  (JAMA CAD V4.1 p. 62).
- **HOOPS Exchange** sewing takes a maximum tolerance in file units.
- **Datasmith/CoreTech (snippet)** sews at 100 × the geometric tolerance.

[src]

### 3.3 Cost and evidence

- **RTI for NIST, Planning Report 99-1.**
  - US automotive supply-chain interoperability costs at least about $1B per
    year.
  - OEMs spend $248M per year correcting or recreating data. One OEM averaged
    4.9 h of rework per exchange across more than 450,000 exchanges.
  - Differing "scale and closure tolerances" is a named cause (§3.1.2, §5.1.2).
- **NIST CTC-05 validation (2015).** Flagged tessellated vertices "up to .009
  inch off of the precise geometry", and a solid that arrived as an open shell.

[src]

---

## 4. Academic repair and meshing on imperfect CAD

- **Steinbrenner, Wyman & Chawner 2000** (IMR9, Pointwise Gridgen, read in full).
  Builds watertight topology without healing the surfaces.
  - **Edge curves first.** Every surface boundary gets its own edge curves. They
    are kept separate even where the surfaces abut perfectly.
  - **Split.** Edge curves are split repeatedly at any endpoint within EndTol,
    using Newton for the closest point, with box and octant culling. A split
    within Fac·EndTol (Fac = 10, "obtained empirically") of another endpoint is
    refused, to stop cascades of tiny edges (Fig. 4).
  - **Merge.** Edge-curve pairs within CrvTol are merged, "replacing all instances
    of one in the surface mesh definition with the other", using a sparse sample
    comparison.
  - **Iterate.** Both steps repeat until one full pass makes no change. "This
    means that mismatches in the CAD surfaces have effectively been reconciled
    without modifying any of the surfaces."
  - **Grid points come last.** Grid points are assigned only after topology, "so
    that the splitting/merging procedure is based purely on geometry".
  - **Who carries the error.** Boundary points are projected onto the non-owner
    surface for meshing and then restored. "All of the error due to surface
    mismatch appears on the surface mesh on which the edge curve is not defined."
    They rejected averaging: it "would increase the number of projections
    required … The sparse form of the edge curve representation … would also be
    lost."
  - **Small curves.** Curves shorter than the tolerance "reduce to singularities"
    and are removed.
  - **Limits.** All gaps must be reconcilable at one tolerance: "the largest
    mismatches in the model must be smaller than the smallest surface or feature".
    Otherwise "reduce the tolerances by a factor of 2 or more, and try again", and
    merge the leftovers interactively (§2.2).

  [src]
- **Henshaw 2001** (LLNL, read in full). Implements the Steinbrenner merge for
  IGES patch sets.
  - Uses exact edge curves where possible; otherwise it fits the trim by 2D
    arclength and curvature.
  - Matching checks the endpoints and the midpoint.
  - "Choose one edge-curve to define the boundary segment for both patches. By
    merging edge curves we effectively remove any gaps or overlaps."
  - Trims are rebuilt from the true edge curves. Each patch gets a UV CDT with no
    new boundary nodes.
  - A boundary node with no (u,v) is projected. If the round-trip 3D point
    disagrees, it is projected onto the patch's boundary edge instead.
  - The rejected alternative, mesh first and stitch afterwards, failed on
    overlaps and thin regions.

  [src]
- **Petersson & Chand 2001** (LLNL, read in full).
  - In one engine, 56 of 357 surfaces were wrong, "the majority … caused by
    incorrect trimming curves".
  - Trim validity requires:
    - closure;
    - positive signed area;
    - staying inside the unit square;
    - no self-intersection, tested on a coarsened control polygon.
  - A trim is flagged narrow when area/arclength < 0.001 (normalized).
  - A curve may be forced closed only if its endpoint gap is within tolerance.
  - Missing trims come from projecting a neighbour's edge or from SSI.

  [src]
- **Sheffer, Blacker, Clements & Bercovier 2000** (virtual topology, read). The
  method edits topology only and never geometry.
  - **Misplaced edges** (edge curves off the face): make "a virtual subset face
    with the edges as defined and a host of the existing face. The edges need not
    be on the host geometry, yet the surface definition will be preserved in the
    generated mesh". The edge is the boundary and the surface is the interior,
    within a tolerance gate.
  - **Mismatched or duplicate entities:** proceed "from the lowest entities
    upward, correcting mismatched vertices, then edges and finally faces", into one
    interpolant.
  - **T-junction:** project the vertex onto the edge, split there, and connect.
  - **Sliver face:** collapse it symmetrically. Draw parasite edges from its
    centre and merge each piece into its adjacent neighbour. Merging the whole
    sliver into one neighbour "will often lead to an asymmetric structure".
  - **Result:** an IGES sedan at tolerance 0.6 against a native 1e-6: "fixes 99%
    of the inconsistencies".

  [src]
- **Sheffer 2001** (face clustering, method). Greedy priority-queue contraction
  with veto indices:
  - dihedral angle;
  - **directed** area ratio, so a small region is attracted to a large one;
  - shared-boundary ratio;
  - curvature.

  [src]
- **Huang, Chen, Shen & Wang 2023** (B-rep Boolean repair, method).
  - Per-edge tolerance `t = k·deg(f₁)·deg(f₂)·deg(e)·t₀`.
  - Move disconnected intersection-edge ends to the lower-dimensional intersection
    `f₀ ∩ e₁₂`: "the dimension of the edge-face intersection is lower than that of
    the face-face intersection, which means lower errors".
  - Collapse a sub-tolerance edge only if it dangles and its bounding edges meet
    within tolerance.
  - Prefer an existing corner vertex to new near-copies.

  [src]
- **Bischoff & Kobbelt 2005** (read in full).
  - Repair only the cells where two patch IDs come within γ, and keep the input
    tessellation everywhere else.
  - Fill in minimally, by topology-preserving erosion of ⌈2γ⌉ layers.
  - Put each reconstructed vertex at the QEF minimizer of the supporting planes,
    which lands it on the feature.

  [src]
- **Bischoff, Pavic & Kobbelt 2005.**
  - Gap closing is *defined* by ρ-cylinders on boundary edges and ρ-spheres on
    boundary vertices.
  - "We cannot derive ρ from the input geometry."

  [src]
- **Barequet & Sharir 1995.**
  - Border matches vote at tolerance ε.
  - Choosing a consistent match set is NP-hard, so matches are accepted greedily
    with an orientation-consistent union-find.
  - Chains are stitched by advancing the side whose next triangle has the
    smaller perimeter.
  - Parameters for parts of 2-20 cm: s = 0.1 mm, ε = 0.5 mm.
  - A wrong ε creates "new surfaces far beyond the intention of the model
    designer".

  [src]
- **Bøhn & Wozny 1992.**
  - Unpairable edges become lamina boundaries.
  - Unambiguous loops are extracted first, and shells are merged "whenever it is
    an option".
  - Hole lids are built by removing the smallest-angle vertex first.

  [src]
- **Surveys.**
  - Ju 2009: local, mesh-based repair suits CAD; volumetric repair is robust but
    loses detail.
  - González-Lluch et al. 2017, quoting Gu et al. 2001:
    - *accuracy errors* are "one vertex that falls outside a tolerance volume
      which surrounds the intersection between the edges that should share it";
    - *structure errors* are a separate class;
    - a small gap has two borders, a large gap three or more.
  - Campen, Attene & Kobbelt 2012: "start from closest pairs to resolve
    ambiguities"; overlaps are negative gaps.

  [src]

---

## 5. Decision heuristics, by question

Each rule cites its source. The ordered procedure built from them is in §7.

### 5.1 The 3D curve and the pcurves disagree: which wins?

| Situation | Rule | Source |
|---|---|---|
| Edge flagged exact, 3D curve within resolution of both surfaces | The 3D curve is the edge | XT p. 22; ACIS ch. 6 |
| Inexact edge in a tolerant kernel | Each face's pcurve, lifted through its surface, is that face's boundary. One fixed pcurve centres the tube. The 3D curve is display-only or deleted | Jackson §3.1; FD §16.3.1; ACIS TEDGE |
| Inexact edge in OCCT or openNURBS | The 3D curve sets position and parameter. Pcurves are reparameterized, never moved, and tolerance absorbs the rest | `Approx_SameParameter`; `ShapeFix_Edge.cxx:872-929`; openNURBS header |
| Surfaces transverse, gap fixable | Re-intersect the adjacent surfaces. The result replaces the stored geometry | ACIS heal p. 13; FD §16.3.1.4, §90.4.6; VDA 4955 p. 43 |
| Surfaces tangent or near-tangent | Re-intersection may fail. Keep the tolerant edge, or keep one surface's iso edge and modify the other | Jackson §2.3; FD §16.3.1.4; ACIS heal p. 16 |
| Tangent cycle among analytic surfaces | The lower-curvature surface stays and the higher-curvature one moves. Planes and canonicals are frozen | ACIS heal p. 14; CATIA local healing |
| Two coincident edges compared | Keep the lower tolerance. Ties go to iso, lower degree, fewer spans | openNURBS `CombineCoincidentEdges` |
| Sewn pair | Keep the longer edge's 3D curve, unless the other gives a lower tolerance | OCCT sewing :680-745, :1091-1113 |
| Sewn pair, Parasolid | Arbitrary choice, under a tolerance that engulfs both | FD §46.2.4 |
| Meshing a merged shared edge | One owner surface carries the edge; the other face absorbs the mismatch by projection. Averaging is rejected | Steinbrenner §2.3; Henshaw §1-2 |
| Misplaced edge (edge curve off its surface) | The edge defines the boundary and the host surface defines the interior | Sheffer 2000 §5.1 |

Classification itself needs the parameter-space trim (Marussig & Hughes §3.2.1).
Whichever 3D position wins, each face still needs UVs that are consistent with
its own trim domain. [src]

### 5.2 Missing pcurves (Fusion STEP and similar)

- **The 3D curve is then the only truth.** Pcurves are made by projecting it.
  OCCT tries, in order:
  1. analytic projection onto planes;
  2. `ProjLib` for uneven B-splines;
  3. a straight UV line;
  4. seeded point-by-point projection with pole snapping and period unwrap;
  5. interpolation.

  Petersson projects a neighbour's edge or uses SSI. VDA says "projection
  curves". [src]
- **Risks the code shows** [src]:
  - The new pcurve is stored at tolerance 0 and only measured later.
  - Branch continuity on periodic surfaces depends on seeding.
  - BRepMesh degenerates an edge that has no 3D curve if healing is skipped.
- Our cheaper route [own]: project *our own shared samples* onto each incident
  surface, seeded and continued in order. That yields the face UVs directly, with
  no intermediate pcurve fit. This is OCCT's point-by-point route
  (`approxPCurve`'s `NextValueOfUV` seeding) without the interpolation step.

### 5.3 Missing 3D curve

- Lift a planar pcurve exactly; otherwise approximate `S(pcurve)` **from the first
  pcurve** (OCCT `BuildCurve3d`). [src]
- Pick deliberately instead [own]:
  - transverse faces → their intersection;
  - otherwise → the owner pcurve chosen by the §7.2 owner rule.

### 5.4 Vertex reconciliation

| Rule | Source |
|---|---|
| Vertex tolerance ≥ every incident edge tolerance; pcurve ends need only land inside the sphere | Jackson §3.1; FD §16.3.2 |
| Vertex tolerance = maximum distance to every representation's end | OCCT `CheckVertexTolerance`; ACIS TVERTEX; openNURBS ×1.001 |
| Representations that miss the vertex are removed (vertices win) | `ShapeFix_Edge.cxx:87-113` |
| Vertex position from unsewn data = average of the curve ends | FD §90.3.2.3 |
| Merge = smallest sphere enclosing both tolerance balls | Segal §4.3; `CombineVertex`; sewing `ComputeToleranceVertex` |
| Opt-in repositioning = average of agreeing 3D-curve ends; outliers are split onto their own vertices | `ShapeFix::FixVertexPosition` |
| Reconcile at the lower-dimensional intersection; prefer an existing corner | Huang 2023 §4.3.1, §4.3.3 |
| Vertex off curve: project onto the curve or face where possible | VDA 4955 p. 43 |
| Mesh end nodes = the stored vertex point | `BRepMesh_EdgeDiscret.cxx:247, 277` |

[src]

### 5.5 Gaps and overlaps between faces

- **Small gaps** (shared topology, inexact geometry) are closed by tolerance
  (OCCT, Parasolid) or by re-intersection (ACIS, VDA, CADfix "pulls gaps
  together"). [src]
- **Unsewn faces.** Sew: split and merge edges from small tolerances to large
  ones. Accept only mutual nearest neighbours. Require vertex-bounded matches and
  prefer matches longer than the gap.
  - Steinbrenner §2.2; Parasolid FD §46.2; OCCT sewing; ACIS stitch steps; Zeiss
    note. [src]
- **Global tolerance condition.** One tolerance works only when the largest gap is
  smaller than the smallest feature (Steinbrenner §2.2; ACIS STITCHAPI
  `max_stitch_tol`). [src]
- **Large gaps** are filled by a patch, whether transfinite, knitted or
  DP-triangulated (Petersson §4.2; SolidWorks Heal Gap; Barequet §6.2), or sent
  to the user. [src]

### 5.6 Small edges and sliver faces

**Small edges:**
- An edge shorter than 4× tolerance cannot be split (Jackson). No SP-curve should
  be shorter than 2× tolerance (FD §90.3.2.3). Do not split within 10× EndTol of
  an endpoint (Steinbrenner). [src]
- A Parasolid "short edge" is one whose every position lies within a vertex
  bubble (`PK_EDGE_optimise`). An OCCT small edge has both ends and its midpoint
  within ε. [src]
- Ways to collapse:

  | Method | Source |
  |---|---|
  | Into a singularity | Steinbrenner |
  | Into the neighbour with the smaller tangent angle | OCCT Wireframe |
  | Only if dangling, onto the edge–edge intersection | Huang |

  [src]

**Sliver faces:**
- **Spot face:** collapse to the mean of its vertices (OCCT). [src]
- **Strip face:** replace it by one edge (OCCT keeps the first edge's 3D curve).
  Sheffer instead collapses it symmetrically. [src]
- **Narrow face:** flagged when the whole face lies within L/2 of its boundary
  (JAMA). [src]
- **Small feature:** anything below about 25-30% of the target mesh size (Beall
  2003). Cubit uses 0.25·h for curves, and h² or the hydraulic radius for
  surfaces. [src]
- CADCEUS deletes faces of width ≤ 0.005 mm on import. [src]

### 5.7 What makes the result "obviously coherent"

These are the principles the sources appeal to, mapped to what an eye checks.

| Principle | Source | What the eye checks |
|---|---|---|
| **Closure.** Connectivity ⇔ approximate intersection, in both directions: every near-contact is represented in topology, and every topological contact is geometrically near | Segal §3; Bøhn & Wozny "merge shells whenever it is an option"; Sakkalis homeomorphism conditions | No cracks, no slits, no faces poking through each other |
| **Minimal displacement, confined to the defect.** Leave everything away from the defect unchanged | Bischoff & Kobbelt §3.3; Ju 2009 §4.1; Qi–Shapiro–Stewart "no larger than the largest discrepancy already existing" | The repair cannot be seen away from the edge |
| **Trust the more reliable entity.** | | Planes stay flat; cylinders stay round |
| — the lower-dimensional intersection | Huang | |
| — the lower-curvature or analytic surface | ACIS; CATIA | |
| — the lower-tolerance edge | openNURBS | |
| — the larger region (attraction from small to large) | Sheffer 2001 | |
| — vertices over representations | OCCT | |
| **Larger tolerance governs a join, and tolerance never decreases under sewing** | Jackson §4.4; FD §46.2.4 | No overconfident merges |
| **Continuity class preserved.** Smooth stays smooth (≤ 0.01° smooth, ≥ 0.2° crease, 1° sharp); tangency breaks become topology | JAMA G-SH-NT; CAx-IF §6.2; PDES H.1.8 | Shading: a G1 join rendered with a crease, or a crease rendered smooth, is the most visible artifact (inferred) |
| **Symmetry in collapse.** A sliver splits between its neighbours rather than being swallowed by one | Sheffer 2000 §4.2 | No lopsided kinks |
| **Order independence.** OCCT fails it; Segal's backtracking and Barequet's orientation union-find aim at it | §2.6 | Same input gives the same look across runs and threads |

[src]

---

## 6. Staging and deferral

### 6.1 How mature systems stage healing and meshing

**OCCT BRepMesh** (code). The stage chain is
`IMeshTools_MeshBuilder::Perform`: BuildModel → DiscretizeEdges → HealModel →
PreProcess → DiscretizeFaces → PostProcess (`IMeshTools_MeshBuilder.cxx:52-62`).

*Classification before anything is meshed* (`BRepMesh_ShapeVisitor.cxx:64-112`)
- A face whose outer wire cannot be ordered gets Failure and is never meshed.
- A bad inner wire is skipped, and the face gets UnorientedWire.

*Edge discretization*
- Parallel over edges, each sampled once.
- An existing polygon within ±ratio of the requested deflection is Reused;
  otherwise it is Outdated.

*ModelHealer: the deferred, batched second pass*
- It pools the offending edges from all faces (`popEdgesToUpdate`, :216-230).
- Each pooled edge is re-sampled in parallel at max(deflection/3, Confusion).
- Every face with a pcurve on an amplified edge is re-checked.
- At most 5 rounds; survivors get SelfIntersectingWire|Failure (:173-212,
  :155-164).
- **This is a dirty set over shared edges with a fixed number of rounds.**

*Face meshing*
- FaceDiscret skips Failure and Reused faces.
- The algorithm is chosen by surface type.

[src]

**SMLib / NVIDIA usd-brep** (code).
- *Heal gate.* Healing runs at import when the stored healer version is older
  than `SM_HEALER_VERSION` (=10). Its policy is to make the database valid "so
  that the rest of SMLib can be written assuming the database they are working
  upon is valid" (`SmHealData.cpp:1499-1501`).
- *Stage order.* `HealTgts` alternates Cache_* (measure, classify, list problems)
  and Fix_* steps in a fixed, "very order dependent" order (:1871-1877).
- *New topology.* New topology is brought up to the current stage by re-running
  earlier stages on the new objects only (:1860-1870).
- *Quarantine.* A face returning `SM_ERR_NOTYET_HEAL_FACE` moves to
  `m_sNotYetFaceProps`, "faces with probs not yet fixable", and is skipped
  (:5062-5070, :650).
- *Tessellation.* SmTess tessellates every shared edge first. A failing face
  gets one retry after rebuilding its UV trims "to include measured gaps"; then
  it is recorded failed and the loop moves on (`SmTess.cpp:11542-11637`).
- *Known issue.* `heal_brep` reports success when its stages fail
  (`KNOWN_ISSUES.md:16-20`).

[src]

**Gmsh** (code).
- *1D.* Every edge starts PENDING. A fixed-point OpenMP loop runs until none
  remain or `MaxRetries` (10) is reached (`Generator.cpp:569-610`).
- *2D, same loop.* "iter == 2 is for meshing re-parametrized surfaces; after
  that, we serialize" (:759-801).
- *Shared-curve conflict.* If 1D segments intersect, every face adjacent to that
  model curve is un-meshed and re-queued, with a midpoint inserted
  (`meshGFace.cpp:201-224`).
- *Failed face.* A failed face gets all its edges refined, at most 3 times
  (:2158-2171).
- *Compounds.* Compound leaders wait for all members, then reparameterize the
  union and remesh (`GFace.cpp:1836-1937, 2021-2038`).
- *Healing.* `_healShape` is off by default. When enabled it runs: degenerated,
  small edges and gaps, spot/strip faces, sew, make solids
  (`GModelIO_OCC.cpp:6036-6305`).

[src]

**Netgen** (code).
- `HealGeometry` is explicit and never called automatically.
- A face tries parameter space first, then plane space, then gives up
  (`occgeom.cpp:313-320`).
- Any failed face cancels surface optimization for the whole mesh
  (`basegeom.cpp:1081-1091`).

[src]

**SALOME SMESH** (code). Each sub-shape has a compute state machine:
NOT_READY, READY_TO_COMPUTE, COMPUTE_OK, FAILED_TO_COMPUTE.
- Errors are typed (BAD_INPUT_MESH, BAD_SHAPE, WARNING, …).
- A failure does not stop siblings.
- A failed entity re-arms on SUBMESH_COMPUTED or a hypothesis change.
- Each dimension finishes before the next starts (`SMESH_subMesh.cxx:1363-1900`;
  `SMESH_Gen.cxx:219-256, 401-404`).

[src]

**Literature.**
- **Mezentsev & Woehler 1999.**
  - Repair is a cyclic module before meshing: verify (tag entities with error
    attributes) → automatic repair of *independent* errors → user-assisted repair
    → removal of *badly meshable geometries* (BMGs).
  - "Unresolved cases are contained in the CAD model in the form of attributes".
  - BMGs are detected by a medial-axis test plus edge-count and area tests, and
    merged into "Super Elements". [src]
- **Steinbrenner 2000:** topology first, grid points afterwards, then an explicit
  interactive second pass for leftovers (§4 above). [src]
- **Shephard, Beall & O'Bara 1998.**
  - Mesh at the user's sizes, refusing to grade toward small features.
  - Then "identify and flag mesh entities that need to be deleted" and fix them by
    local modification.
  - Most small features "can be determined from the surface mesh"; the rest are
    caught after volume meshing. [src]
- **Beall, Walsh & Shephard 2003.**
  - Translation and healing "introduce slivers to resolve gaps".
  - Features below about 25-30% of the target size are "ignored in the initial
    mesh and accounted for … in each stage of the mesh adaptivity process". [src]
- **Escalation filters.**
  - Pion & Fabri: interval filters escalate to exact evaluation only on
    `unsafe_comparison`. A kept DAG allows a later exact recompute.
  - Shewchuk 1997: escalation happens only when forward error analysis says the
    cheap sign cannot be trusted. [src]
- **GPU tessellation.**
  - Feature-adaptive subdivision sends regular faces to hardware tessellation and
    irregular ones to precomputed tables.
  - DiagSplit decides dice or split only from information both neighbours share.
  - Patney & Owens split Reyes into an irregular Bound/Split queue and a regular
    Dice stage.
  - Claux 2014 tessellates first, detects cracks in image space and fixes only
    those pixels. [src]

### 6.2 Patterns and up-front classification

The staging patterns these systems share [src]:

1. Sample shared edges once.
2. Check faces.
3. Re-sample only the offending edges, in pooled batched rounds, and re-check
   every incident face.
4. Cap the number of rounds, then mark Failure and continue.
5. Relax an edge's target to a measured source gap rather than fail it (OCCT
   edge deflection ≥ the vertex-to-curve-end distance).
6. Reuse prior results within a deflection ratio.
7. Quarantine unsupported entities on a list; do not block.
8. Measure/fix stages run in a fixed order, so later stages may assume earlier
   classes are gone.
9. Catch-up for newly created topology.
10. A per-entity state machine with typed errors and dependency re-arming.
11. A low tolerance first; leftovers go to an explicit second pass with a doubled
    tolerance.

Prior art for up-front easy/hard classification [src]:

| Classifier | Source |
|---|---|
| Algorithm by surface type | OCCT `BRepMesh_MeshAlgoFactory.cxx:64-116`; Gmsh `meshGFace.cpp:2404-2441` |
| Wire-order pre-rejection; SameRange/degenerate flag cleanup | OCCT |
| 36-code per-sub-shape validity map with a topology-only mode | OCCT `BRepCheck_Status.hxx:20-58` |
| Staged gap/degeneracy/seam classifiers and a NotYet class | SMLib |
| Independent errors vs BMGs, with a medial-axis detector | Mezentsev–Woehler |
| Sub-tolerance curves → singularities | Steinbrenner |
| Small = below 25-30% of target size | Beall 2003 |
| Thresholds tied to mesh size | Cubit Power Tool (0.25·h curves; h² or hydraulic radius for surfaces) and ITEM (e = 0.1·h) |
| Per-entity ML quality prediction, ranked worst-first | Owen, Shead & Martin 2019; Cubit 17.08 |
| Hand-built meshing-difficulty function | White, Saigal & Owen 2005 (abstract) |
| Edge classes by face-use count (1 free, 2 shared, ≥ 3 non-manifold) | HyperMesh; Pointwise |
| Regular-face test | Nießner 2012 |
| PDQ checks themselves | JAMA, VDA |

---

## 7. Implications for BRepGPU

Everything in this section is **[own]** unless it cites a source. It is a design
input for the tribunal, not a selected design.

### 7.1 What "the truth" means for us

The literature's one consistent answer is this: there is no global truth carrier.
There is a per-edge decision, and the edge's measured discrepancy is retained.
Parasolid and ACIS settle it by making each face's own boundary
(pcurve-on-surface) primary. That guarantees every face's boundary lies exactly on
its own surface, and lets the faces disagree with each other inside a tube.

Our edge-first shared IDs force one position per sample, so we cannot let the
faces disagree in 3D the way a tolerant kernel does. The shared position must be
chosen, and the mismatch then lands in each face's boundary triangles. The
procedure below chooses that position so that:

- it lies on **both** surfaces where that is well-conditioned;
- it lies on the more trusted surface where it is not;
- it is never silently moved by more than a stated budget.

This revises the proposal in
[the geometry contract study](2026-10-16-whole-mesher-geometry-contract-study.md)
§1. That proposal reads: "retain original spatial edges/vertices as physical
skeleton authority". The kernels read here do not treat the 3D curve as the
authority for inexact edges. The OM10 evidence already in hand points the same
way:

- face 165590's foreign pcurve sat 2.47 µm off;
- the 3D curve sat 0.14 µm from the cylinder;
- `papers/2026-10-15-om10-tolerant-incidence-reading.md`.

That evidence argues for **measuring each representation against the surfaces and
picking per sample**, not for a fixed authority. The source is never modified.
Every choice is recorded with its displacement.

**Budget notation:**

| Symbol | Meaning |
|---|---|
| ε | Requested deflection (world units) |
| η = ε/16 | "Exact" band: noise we ignore |
| β = ε/2 | Displacement we may introduce silently, with the move recorded |
| G | Gross limit: refuse above it; default min(4·ε, ¼·the local feature size) |

These are proposals for the tribunal. No source sets them for a mesher. The
industry anchors are:

- 0.01 mm data quality and 0.02 mm assumed model accuracy (JAMA, CAx-IF);
- the 4× and 2× short-edge rules (Jackson; Parasolid FD);
- ACIS's 3 × gap.

### 7.2 Ordered decision procedure

**Stage 0. Ingress (per entity, batched).**
- Refuse non-finite values.
- Record the stored tolerances as metadata only.
- Classify each surface: plane, other analytic, freeform.
- Classify each edge by face-use count (1, 2, or ≥ 3, plus seams).
- **Stored tolerance never sets density** (contrast OCCT
  `BRepMesh_Deflection.cxx:148`). It is used only as a search radius for identity
  questions and as a hint for G. A stored tolerance far above the measured
  discrepancy is reported.

**Stage 1. Sample and measure each edge (batched over all edges, CPU SIMD or GPU).**

1. Choose the sample count from the 3D curve and from the incident faces'
   requirements, as in the [literature](2026-10-17-fast-tessellation-literature.md)
   §1.4. If there is no 3D curve, use the lifted pcurves.
2. At each sample tᵢ, evaluate `P = C(tᵢ)`.
3. For each incident face f, find a UV and its distance to the surface:
   - If the edge is flagged SameParameter and the range matches, use
     `u_f = p_f(tᵢ)`.
   - Otherwise, or if `S_f(u_f)` is more than η from P, project P onto S_f.
     Seed from `p_f` or from the previous sample. Require monotone order along
     the pcurve, and lift across the period.
   - Record `d_f = |P − S_f(u_f)|` and the surface normal n_f.

   This is the measurement OCCT does at 23 points
   (`ShapeAnalysis_Edge::CheckSameParameter`). We do it at every tessellation
   sample, which is denser and costs nothing extra, because the samples are being
   evaluated anyway.
4. With two incident faces, compute at each sample
   `θ = ∠(n₁, n₂)` and the minimum move to a point on both surfaces under
   linearization:

   `|x| = sqrt(d₁² + d₂² − 2·d₁·d₂·cos θ) / sin θ`.

   Derivation: the minimum-norm x satisfying `n₁·x = −d₁` and `n₂·x = −d₂` lies in
   span(n₁, n₂). |x| grows like 1/sin θ, which is the conditioning the kernels
   describe in words: re-intersection "may not always be successful if the
   adjacent surfaces are tangent or nearly tangent" (FD §16.3.1.4).
5. Record per edge: max d_f, max |x|, min θ, and whether any projection failed or
   came out non-monotone.

**Stage 2. Classify and resolve each edge.** Apply the rules in this order; the
first one that matches wins.

| Rule | Condition | Shared position | Face UVs | Justification |
|---|---|---|---|---|
| R1 exact | max d_f ≤ η | `C(tᵢ)` (the 3D curve) | `p_f(tᵢ)` or the projection | The kernels agree on accurate edges (XT p. 22). This is the common case and needs no decision |
| R2 near-exact | max d_f ≤ β | `C(tᵢ)` | The projections from stage 1 | Minimal displacement: the boundary moves by at most β, and the move is recorded. This is OCCT's de facto behaviour, but with measurement in place of tolerance inflation |
| R3 transverse | max d_f > β, and max \|x\| ≤ β at every sample (forces θ well away from 0) | Two-surface Newton from P, seeded by (u₁, u₂), converging to a point on S₁ ∩ S₂ | Read directly from the Newton solution | ACIS sharp-edge solver, Parasolid reset_precision, VDA "edges formed as intersection". A point on both surfaces closes the solid exactly, and transversality makes it well-conditioned |
| R4 tangent | max d_f > β, and \|x\| > β somewhere (near-tangent) | On the **owner** face: `S_o(u_o)`, where u_o is the owner's pcurve (if its lift is within β of P) or the projection of P | Owner: u_o. Non-owner: the projection of the owner position | Jackson §2.3: smooth joins cannot be re-intersected. Steinbrenner: one owner, the other face absorbs the mismatch. The non-owner's boundary triangles carry `d = dist(S_o(u_o), S_non)`, which is recorded |
| R5 single-face (free or lamina edge) | One incident face | max d_f ≤ β → `C(tᵢ)`. Otherwise `S_f(proj)`: the face's own surface wins | Projection | Nothing to stitch to. Keep the face exactly on its surface (Sheffer "misplaced edges": surface for the interior, edge for the boundary, within a gate) |
| R6 gross | max d_f > G, or a projection failed or came out non-monotone, or (for R3) Newton did not converge within a geometry-derived iteration bound | Refuse the edge. Every face that uses it is refused with a record. The part is marked incomplete; neighbouring faces still mesh | — | No silent tolerance inflation (contrast §2.6). Explicit refusal per the journal's default |

**Owner rule for R4,** in order:
1. If exactly one surface is analytic, the analytic surface owns. With two
   analytic surfaces, the lower-curvature one owns. The plane is frozen and the
   cylinder moves (ACIS heal p. 14; CATIA "Freeze Planar/Canonic").
2. Otherwise, the face whose stored pcurve lift lies nearest the 3D curve
   (smallest max d_f) owns. This is minimal displacement from the designer's
   recorded edge.
3. Otherwise, the larger face area owns: small is attracted to large (Sheffer 2001).
4. Otherwise, the lower face ID, for determinism.

Rule 1 matches the kernels and the eye: a plane with a ridge is visible, a
blend with a 5 µm step is not.

**Missing representations:**
- **No pcurve** (Fusion STEP): stage 1 projects every sample anyway, so R1 to R6
  apply unchanged. Missing pcurves cost one seeded projection per sample. Seams
  and poles need the period lift and pole snap: snap when within vertex tolerance
  of a singularity (OCCT `approxPCurve`).
- **No 3D curve:** take P from the owner rule (rules 2 to 4 skip the "nearest the
  3D curve" test). If the faces are transverse, refine by R3.

**Continuity class (for normals).**
- At each sample record θ. If θ ≤ 0.01° throughout (JAMA "smooth"), the edge is
  G1, and its shared vertices may carry one shared shading normal.
- If θ ≥ 1° anywhere (CAx-IF "sharp"), use per-face normals.
- Between those, treat as ambiguous: follow the source's own regularity if it is
  encoded (OCCT `EncodeRegularity`, the STEP `edge_curve` same_sense and
  regularity data). Otherwise use per-face normals.

This keeps smooth edges smooth and creases creased in shading, which is what an
eye checks first.

**Stage 3. Resolve vertices (after their edges are classified; before faces).**

Rules in order:
- **V1.** If the stored vertex point is within β of every incident edge's
  resolved end position, the vertex ID takes the stored point. Edge chains end at
  it, as in OCCT (`BRepMesh_EdgeDiscret.cxx:247`).
- **V2.** If ≥ 3 incident surfaces are pairwise transverse, solve the three-surface
  point S₁ ∩ S₂ ∩ S₃ by Newton, seeded from the stored point. Accept it if it is
  within G. This is the lower-dimensional intersection, the best-conditioned
  point (Huang §4.3.1, "prefer existing corner" §4.3.3).
- **V3.** Otherwise, take the centre of the smallest ball enclosing the resolved
  edge ends (Segal §4.3; `CombineVertex`). Do not average: averaging is pulled
  toward clusters of ends, the enclosing ball is not.
- **V4.** If the enclosing radius exceeds G, refuse, as in R6.

Each incident edge's last segment absorbs the end move, and that move is
recorded. If the move exceeds β, the edge is re-checked with its new endpoint.

**Small features (virtual, topology-only).**
- An edge whose every sample lies within its end vertices' β balls (the
  Parasolid "short edge" definition, applied with β) collapses to one vertex ID.
  Every incident face loop drops it.
- An edge shorter than 4·β (Jackson) emits no interior samples.
- A face whose interior lies entirely within β of its boundary (the JAMA G-FA-NA
  definition, with L = 2β) is meshed from its boundary vertices alone, with no
  interior points, so it stays consistent by construction.
- If such a face is narrower than η, collapse it virtually and symmetrically
  (Sheffer 2000 §4.2): its two long boundaries map onto one shared vertex chain.
  This is a topology edit to the *mesh*, recorded per face, never to the source.
- Collapse runs bottom-up: vertices, then edges, then faces (Jackson §4.3;
  Sheffer §5.1).

**Unsewn sheets** (faces with lamina edges that ought to meet) are **not** healed
implicitly. Sewing is a separate, operator-visible stage. If added, it should
follow Steinbrenner/Parasolid: split and merge from small tolerances to large,
mutual nearest neighbours, vertex-bounded matches, and a recorded tolerance.

### 7.3 Fit with edge-first shared vertex IDs

- **Each edge chain is final before any incident face consumes it.** A chain is
  its sample count, its vertex IDs, one 3D position per ID, and one UV per
  (ID, face occurrence), including both seam uses. Every system read obeys this
  (§6.1). The rules above produce it per edge, independently of the faces, so the
  stage is embarrassingly parallel over edges.
- Positions R1–R4 lie on the owner surface or on both. UVs are per occurrence.
  So:
  - each face's boundary vertices satisfy `S_f(uv) = position` exactly for R3,
    for the owner in R4, and within η for R1;
  - the residual on the non-owner side (R2 ≤ β, R4 recorded) sits in boundary
    triangles only;
  - interior density stays set by ε, unlike OCCT's 2× floor.
- **Fixing the sample count before positions settle.** The sample count comes from
  the 3D curve and the face requirements, which are known before classification.
  If an edge moves by at most β under R2–R4, its sample count stays valid, because
  the chord-error budget changes only by the move, which is already in the ledger.
  So IDs can be issued in stage 1 and positions finalized in stage 2.

### 7.4 What runs now, what is deferred, and what must be settled first

**Immediate pass (the "easy" majority, expected to be most edges on clean CAD; a
hypothesis to measure on the ABC sample):**
- R1 and R2 edges, V1 vertices.
- Faces whose edges are all R1 or R2 and whose vertices are all V1 are meshed at
  once.

**Deferred batched pass:**

| Queue | Work | Notes |
|---|---|---|
| R3 | Two-surface Newton per sample | Uniform SoA work, GPU- and SIMD-friendly. This is the operator's "hyper-efficient batched second pass" |
| V2 | Three-surface Newton | |
| R4 | Owner selection plus projection | |
| Missing-pcurve projections | Seeded continuation along each edge | Continuation is sequential within an edge, so the batch runs over edges |
| Small-feature collapse | Virtual collapse | |

- Faces that touch any deferred entity wait. When an edge resolves, decrement its
  incident faces' pending counters. A face meshes when its counter reaches zero.
  This is SMESH's dependency re-arming and Gmsh's PENDING, without the retry
  storm.
- Batches flush when a queue fills a SIMD/warp-sized batch or when the immediate
  pass drains, whichever comes first.

**What must be settled before an incident face consumes an edge:**
1. The chain's IDs and sample count.
2. Both end vertex IDs, resolved by V1–V4.
3. The 3D position of every ID.
4. The UV of every (ID, face occurrence), monotone along the edge and lifted
   consistently with the face's period and seam convention.
5. For the face as a whole: its boundary loops in UV must not self-intersect at
   its resolution.

Item 5 is OCCT ModelHealer's check. It runs per face after the face's edges
settle. A failure there re-samples the offending edges (pooled across faces, as in
`popEdgesToUpdate`), which invalidates every incident face (OCCT, Gmsh). That loop
is bounded by geometry: stop when the conflicting segments are shorter than the
UV distance of the nearest non-adjacent loop feature. Then refuse, never cap
silently (test-plan finding 3).

**Speculative variant (not in any source).** Let a face mesh against a provisional
chain from R2 positions while R3/R4 resolve. Patch the boundary positions
afterwards, keeping the IDs and the connectivity. This is valid only if the UV
moves leave every boundary-adjacent triangle positively oriented in UV. Check
that per triangle, and re-mesh the face otherwise. It buys latency, not work.
Whether it pays is a measurement question; the counter scheme above is the
default.

### 7.5 Consequences for the oracle and the spec

- **The dense-deviation oracle is defined per face** (the test plan's §(3) gap 1
  asks for this):
  - distance from the face's triangles to **that face's own surface** S_f, over
    its trim domain;
  - the face's boundary taken as the resolved shared chain, so the
    surface-to-mesh direction is measured within the domain the chain bounds.

  The allowed deviation is ε in the interior. On boundary triangles it is ε plus
  the recorded per-edge displacement (≤ β for R2; the recorded value for the R4
  non-owner side).
- **Edges get their own check:** chain-to-carrier distance against the carrier
  each rule names:

  | Rule | Carrier |
  |---|---|
  | R1, R2 | The 3D curve |
  | R3 | S₁ ∩ S₂ |
  | R4 | The owner's lifted pcurve |

- **Per-edge output records** feed the per-face accounting the test plan requires:
  - class R1–R6;
  - max d_f, max |x|, min θ;
  - the owner face;
  - vertex moves.
- **New authored families** (the test plan's §(3) gap 1 asks for this):
  - edge-to-surface offsets swept from 1e-6 to 1e-2 against ε;
  - dihedral angles swept from 0° to 90° across the R3/R4 boundary;
  - pcurve-free variants;
  - vertices off their curves;
  - a plane–cylinder tangent join with a deliberate step (owner rule 1);
  - slivers of width η/2, η and 2β.
  - Each family gets a known resolution by construction.

### 7.6 Questions for the operator

1. **Budgets.** η, β and G are proposals. Should the source discrepancy count
   inside ε (a tighter boundary budget) or be reported on top of ε, which is what
   §7.5 assumes? The manufacturing default of ε itself is still open; industry
   data quality sits at 0.01 mm.
2. **Moving the boundary.** R3 can move the shared boundary off the stored 3D
   curve by up to β, onto the true intersection. That contradicts the earlier
   "spatial edges are skeleton authority" proposal (geometry contract study §1).
   The kernels read here side with R3. Is that acceptable?
3. **Owner rule 1.** Analytic surfaces own over freeform ones. Agreed as the
   default?
4. **Sewing.** Should sewing of unsewn sheets enter the mesher's scope, as an
   explicit stage, or stay out?

---

## Acquisition notes

- 45 new PDFs were acquired and checked with `file`. Three duplicates written by
  two lanes were removed (Jackson 1995, Segal 1990, Sakkalis 2001). Rows,
  hashes and read depth are in the sources TSV.
- **Converted from PostScript.** Mezentsev–Woehler, Steinbrenner 2000,
  Shephard 1998 and Butlin–Stops were fetched as `.ps.gz` from Wayback copies of
  imr.sandia.gov and converted with local Ghostscript. Their PDF hashes depend on
  the Ghostscript build. The `.ps.gz` sha256 values:

  | File | sha256 |
  |---|---|
  | mezentsev | d94901873a60c4e88021ed0ee7e596b9df9880ea061a1c0613b8921d8cae4cbd |
  | steinbrenner00 | e4d31ef0a72398f4d84d0df42d2e30f7f139c5d02d6f2f961b7c431029d3a5c0 |
  | shephard98 | dbfbd41e56f9587ecc0dc643478b075dd5836da383902113c6f677fa0089f1fd |
  | butlin | 97c30a80304dbd7ea32dd7ec34bd5f988d4973654e4496c903bd90a3af51c9a6 |

- **Fetched through archive.org.** The Parasolid V35 Functional Description and
  XT reference came from an archive.org copy of q-solid.com, and the ACIS wiki
  pages from 2010-2015 snapshots. The live doc.spatial.com now needs a login.
- **Not acquired (for SHOPPING_LIST.md):**
  - **Standards:** SASIG PDQ V3.0 Attachment C "recommended values";
    ISO 10303-59:2022; ISO/PAS 26183.
  - **Kernel reference pages:** Parasolid PK reference pages (`PK_EDGE_repair`,
    `PK_EDGE_optimise`, sew options); the ACIS R10 healing function reference and
    the "Healing Tolerances"/"Tightening Gaps" articles.
  - **Vendor documentation:** the CADfix user guide; Elysium CADdoctor and
    Q-Checker defaults; Pointwise Assemble Models docs (Cloudflare 403); Simmetrix
    tolerance white papers.
  - **Papers:**
    - Steinbrenner et al. 2001 "Procedural CAD model edge tolerance
      negotiation" (Engineering with Computers);
    - Xiao, Alliez, Busé & Rineau 2021 "Delaunay meshing and repairing of NURBS
      models" (HAL bot wall);
    - Kahlesz, Balázs & Klein 2002 (sewing for tessellation);
    - Barequet & Kumar 1997;
    - Gu, Chase et al. 2001 (JCISE);
    - Chong, Kumar & Lee 2007;
    - Busaryev, Dey & Levine 2009;
    - Fang, Brüderlin & Zhu 1993;
    - Attene, Campen & Kobbelt 2013 (CSUR).
  - Highest value: Xiao 2021 (meshing *through* trim defects), Steinbrenner 2001,
    Kahlesz 2002.
