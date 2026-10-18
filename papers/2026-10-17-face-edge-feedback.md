# Face-to-edge feedback and its termination

Research lane M4-G2 of the fast-mesher campaign, 2026-10-17, branch
`research/fast-mesher-study`. Briefs:
[M4-common](../docs/fast-mesher-campaign/briefs/M4-common.md) and
[M4-G2](../docs/fast-mesher-campaign/briefs/M4-G2.md); bead `brepgpu-12s.3`.
Stub: [lifecycle map §4.1 G2](2026-10-17-geometry-lifecycle-map.md) (S20, S7,
S9; contradiction X2). Nothing was built, run, meshed or measured. The lane
read reference code at pinned revisions in `quarantine/` and literature in
`papers/pdfs/`. Sources and read depth are in
[2026-10-17-face-edge-feedback-sources.tsv](2026-10-17-face-edge-feedback-sources.tsv).

Tags: **[src]** a source establishes it (file:line or section given);
**[code]** read in this repository; **[own]** this lane's derivation;
**[inf]** inferred from a source but not stated by it. Design section numbers
refer to `docs/FAST_MESHER_DESIGN.md` (working draft) as of this lane.

## 0. Findings and the proposed loop

1. **Every mature mesher fixes face trouble by re-splitting the shared edge, and
   every one stops on a count.** OCCT stops after 5 rounds, Gmsh after 10
   retries plus 3 refine-all passes, CGAL's feature protection after 29 loops,
   SmTess after 100 passes, and the retired owned path after 256 rounds.
   Netgen and truck have no feedback path. CGAL's count has been "9, 99, and
   now 29" over its history, and reaching it "might result in a crash"
   [src §1]. No system derives its stop from geometry. Parasolid's documented
   floor is a user length (`min_facet_width`), and its round limit is not
   documented.
2. **Checking for crossings and then splitting (OCCT, Gmsh) misses one failure
   class.** OCCT's checker only tests chord crossings
   (`BRepMesh_FaceChecker.cxx:136-194`). An inner loop that lies wholly
   inside the cap between an outer chord and its curve crosses nothing, so it
   is never reported. Yet that loop now lies outside the polygonal domain
   [own from code]. The checker also ignores crossings at angles below 5° and
   self-loops of area below π(2·deflection)² [src].
3. **The literature gives a termination argument that fits trim chains.** Pav
   and Walkington (2005) handle curved 2D input with no angle bound: local
   feature size relative to disjoint features, curve separation at shared
   endpoints, and lens containment (Lemma 1). Their result is finite
   termination by ball packing [src]. Ruppert and Shewchuk give the
   insertion-radius and packing template [src]. None of these papers treats a
   tangent corner (α = 0, a cusp), dyadic parameter splits, a requirement from
   the surface's own sizing (the band), or 3D folds. §2 derives what is
   missing.
4. **The lane's main result [own]:** state every boundary demand as a
   *hereditary predicate*. That means a pure function of one segment's own
   parameter interval and of the face's source geometry, which, once true,
   stays true for every sub-segment. The final chain is then the unique
   minimal dyadic refinement in which every segment passes the predicates in
   every face occurrence. It is identical at any thread count and in any
   order, and it needs no generations, no barriers and no round count. The
   chain is still re-split in response to faces. What changes is that the
   check runs as a closure stage before triangulation, not as a check-and-retry
   after it.
5. **Check-and-split rounds are order-dependent unless they run
   synchronously.** A conflict between chords A and B can be cleared by
   splitting either one. An asynchronous schedule therefore reaches different
   but equally valid fixpoints [own]. OCCT avoids this with synchronous
   rounds. Gmsh mutates shared curves from inside parallel face meshing, then
   serialises after round 2 because "self-intersections of 1D meshes are not
   thread safe" (`Generator.cpp:797-799`) [src]. Hereditary predicates remove
   the problem entirely.
6. **The healing paper's proposed stop rule needs correcting.** It says to stop
   "when the conflicting segments are shorter than the UV distance of the
   nearest non-adjacent loop feature" (tolerant-geometry-healing §7.4). The
   quantity that matters is the lens half-width, about ¼·ℓ·τ, which grows as
   ℓ²κ, compared with the separation. The proposed rule is too strong for
   straight pcurves, which never need splitting, and too weak at corners,
   where adjacent features meet [own §2.4].

### The loop in ten lines

1. Edges are sampled and resolved as in design §5.2 items 1–4 and M2's point
   solve. Sample keys are edge identity plus a dyadic path in the edge
   parameter.
2. For each face occurrence of each edge segment s, evaluate four hereditary
   predicates on its UV arc and its 3D arc. P1 flatness: UV tangent variation
   τ(s) ≤ τ₀. P2 separation: the lens half-width ½·ℓ_uv(s)·tan(τ(s)/2) is
   less than ½ of the arc's distance to every feature outside its corner
   balls. P3 corner: inside a corner ball the arc stays in its corner cone.
   P4 band: the 3D arc length is no more than the face's admissible boundary
   length from its own second-derivative bound.
3. Trigger: some predicate fails in some incident occurrence.
4. Action: split s at its dyadic parameter midpoint. The new sample takes
   M2's L1 point solve and the L3 blend, with blend weights from the curve
   parameter, so existing samples never move. It gets one UV per occurrence.
5. Recurse on each child until every predicate passes. Edges are independent,
   so this runs in parallel, and the result does not depend on order.
6. Invalidation: none inside the closure stage, because predicates are
   hereditary and no face consumes a chain before closure. A face meshed
   early (speculative or display) rebuilds only its band components that
   touch the split segments.
7. Stop: a segment passes. Or its required length L\*(s) falls below the
   floor (arithmetic in t and UV, or a broken premise: lfs = 0, α = 0 with no
   cusp certificate, unbounded κ or M). Or its depth exceeds the packing bound
   precomputed from L\*. There is no count.
8. Refusal: refuse only the face occurrence whose predicate cannot be met,
   with a record (§5.2). The chain stays valid for the other faces, and the
   part is marked incomplete under D-partial-output.
9. After triangulation, a residual detector looks for crossings, containment
   of other loops, 3D folds and boundary-triangle ε. It is not a feedback
   trigger. If it fires on input that met the premises, a bound was wrong, so
   it refuses the face and names the predicate.
10. Work estimate: ∫ ds/((1−q)·L\*(s)) per edge feeds design §5.5 and the
    output-size guard before any refinement runs.

### The stop rule and its argument

Each predicate becomes true once the segment's UV arc length satisfies
ℓ(s) ≤ L\*(s), where

L\*(s) = min( τ₀/κ̄, √(2·lfs̲/κ̄), α_v/(4κ̄) inside corner balls,
2√(2ε_b/M̄)·(3D→UV) ).

Here κ̄ bounds the UV curvature on the arc, lfs̲ bounds the local feature size
of the face's trim complex from below, α_v is the corner angle, and M̄ bounds
the surface's second derivative on the band. All are properties of the
source; none depends on how other edges are sampled.

A segment splits only when it fails, and failing implies ℓ > L\*. A child keeps
at least (1−q) of its parent's arc length, where q = ρ/(1+ρ) and ρ bounds the
speed ratio of the parameterisation. So no arc shorter than (1−q)·L\*_min is
ever created. That bounds samples per edge by ℓ_edge/((1−q)·L\*_min) + 1, and
by ∫ ds/((1−q)L\*(s)) when graded, since lfs is 1-Lipschitz. Depth is bounded
by log_{1/q}(ℓ_edge/L\*_min) + 1. This is the Ruppert/Shewchuk packing
argument (Shewchuk 2002 Theorem 4) carried to trim chains through Pav and
Walkington's curve lfs and lens lemma.

When a premise fails, L\*_min is 0 and the face refuses at the floor. Iterating
in that case would be wrong. The full statement and its premises are in §2.

## 1. How mature meshers feed face failure back into shared edges (question 1)

### 1.1 Summary table

| System, revision | Trigger | Action and invalidation set | Stop | Determinism |
|---|---|---|---|---|
| OCCT 8.0.1 `b8f597c6` | Proper UV crossing between discretised wire segments of one face (any wires, self included), excluding crossings under 5° and self-loops of area < π(2δ)² | Whole edge re-sampled at max(δ/3, Confusion); every face with a pcurve on that edge re-checked | 5 rounds; survivors `SelfIntersectingWire\|Failure` | Synchronous rounds; per-face checks pure; set union of edges |
| Gmsh `b4e84a43` | Model-edge segment not recoverable in the initial 2D triangulation (1D mesh intersects); or invalid elements (3D normal check) after 10 swap/smooth passes | Split the offending segment at its parameter midpoint; un-mesh every face on that curve (`PENDING`); retry this face. Invalid elements: split *all* bounding edges | `MaxRetries` = 10 retries and outer passes; refine-all at most 3 | Parallel in-place mutation of shared curves; serialised after pass 2 |
| Netgen `31e85f81` | Face mesher fails | None to edges: retry the face in plane space, then give up | 1 fallback | n/a |
| Parasolid V35 (FD) | Facet mesh needs refinement; inner loops close to outer boundary | Topology matching splits facets in the neighbour mesh; incremental auto method re-facets "any faces adjacent to an edge that has been refined" | Min facet width (user length); round limit not documented | Not documented |
| SmTess `c9c5979f` | Crossing or mis-nested linearised trims | Split shared B-rep edges; rebuild *every* face | 100 passes | Serial, order-dependent |
| CGAL Mesh_3 `28811b67` | Non-adjacent protecting balls intersect; approximation too large | Shrink both balls to min(d/2.1, r); repopulate edges | 29 loops (history 9, 99, 29); `minimal_size` floor | Jacobi-style: sizes collected, then applied |
| SMESH `83f8bf48` | none found | Failed sub-shape re-arms on event | — | — |
| truck `88ed0052` | Constraint cannot be added | Constraint silently skipped | — | — |
| Owned (retired) | Face returns `needs_boundary` from a failed certificate | Sorted, unique interval requests; refine; reset faces whose boundary generation moved | 256 rounds; watch used 3 | Synchronous generations, canonical order |

### 1.2 OCCT BRepMesh (8.0.1, `quarantine/new-iteration/occt`) [src]

The stage order is `BuildModel → DiscretizeEdges → HealModel →
PreProcessModel → DiscretizeFaces → PostProcessModel`
(`IMeshTools_MeshBuilder.cxx:39-110`). HealModel is the feedback stage, and it
runs *before* any face is triangulated.

- **Trigger.** `BRepMesh_FaceChecker::Perform` (`BRepMesh_FaceChecker.cxx:224-300`)
  builds one box tree per wire over its UV segments, then tests each segment
  against the segments of the same and later wires with
  `BRepMesh_GeomTool::IntSegSeg` (:136-194). Only a proper `Cross` counts.
  Crossings at angles below `MaxTangentAngle` = 5° are ignored (:26, :155).
  For self-wire pairs, a crossing that closes a loop of area below
  `π·(2·deflection)²` is ignored (:105, :160-186; the 2× factor carries the
  comment "in order to fit regressions", :256). A face whose single wire has
  two two-point edges is also flagged (`BRepMesh_ModelHealer.cxx:258-279`).
- **Pre-repair.** `fixFaceBoundaries` snaps the end UVs of consecutive edges to
  each other with no distance threshold. A wire it cannot connect becomes
  `OpenWire`, and the face becomes `Outdated` (:292-357, :440-515).
- **Action.** `EdgeAmplifier` clears the edge, sets its deflection to
  `max(δ/3, Confusion)`, and re-tessellates the 3D curve and every pcurve. It
  adds one point if a face's outer wire has two edges or fewer (:40-90).
  `MinSize` is forced to `Confusion` during healing (:142).
- **Invalidation.** `popEdgesToUpdate` pools all flagged edges from all faces
  (:216-230). Every face with a pcurve on an amplified edge is re-checked
  (:189-207).
- **Stop.** `while (aAmpIt++ < aIterNb && popEdgesToUpdate(...))` with
  `aIterNb = 5` (:179-183). Faces that still fail get
  `SelfIntersectingWire` and `Failure` (:155-163), and `FaceDiscret` skips them
  (`BRepMesh_FaceDiscret.cxx:102`). Five rounds multiply deflection by 3⁻⁵ =
  1/243 at most. The cap is a count, and the factor it implies is arbitrary.
- **After triangulation there is no feedback.** Frontier recovery makes two
  passes and fills gaps with "makeshift triangles" (`BRepMesh_Delaun.cxx:944-1046`).
  Deflection-control refinement (11 passes) skips frontier links
  (`BRepMesh_DelaunayDeflectionControlMeshAlgo.hxx:85-88, 301-304`), so
  interior refinement never splits a shared edge. A face that fails here gets
  `Failure` (`BRepMesh_BaseMeshAlgo.cxx:253`).
- **Determinism [own from code].** Each face check is a pure function of
  the current discretisation. Edges are amplified independently, and the
  edge set is a union. So the result does not depend on thread count, despite
  pointer-hashed maps.
- **What it misses [own from code].** Containment of a whole loop inside a
  chord's cap produces no crossing. Crossings below 5° and small self-loops
  pass by design.

### 1.3 Gmsh (`quarantine/sources/gmsh`, catalog revision `b4e84a43`) [src]

- **Trigger 1: unrecoverable boundary.** `recoverBoundaryEdges`
  (`meshGFace.cpp:1160-1243`) forces each model-edge segment into the
  initial triangulation. Segments that cannot be recovered because the 1D
  mesh intersects go into `edgesNotRecovered`, and the function returns
  `Retry` (:1204-1240). The periodic generator does the same (:2032-2118).
- **Action.** `remeshUnrecoveredEdges` (:86-200 and :202-300) un-meshes
  "model faces adjacent to the model edge" (status `PENDING`, `deMeshGFace`).
  It then inserts a vertex at the parameter midpoint `t = (t1+t2)/2` of the
  offending segment, with size `lc = (lc1+lc2)/2`. The face retries
  recursively while `RECUR_ITER < mesh.maxRetries` (:1517-1520, :2118-2121).
- **Trigger 2: invalid elements.** After refinement, elements whose 3D
  validity (`BDS_Face_Validity` against the surface normal) is ≤ 0 survive 10
  swap and smooth passes. The face is then marked `FAILED`
  (`meshGFaceBDS.cpp:927-985`). The generator re-queues it with
  `refineAllEdges` at most 3 times (`meshGFace.cpp:2158-2173`). The comment
  there reads: "refining the bounding curves does not always help, e.g. near
  the apex of a cone, where the circumference always ends up smaller than the
  mesh size: stop after a few attempts, as each one doubles the number of
  nodes".
- **Outer loop.** `Mesh2D` re-meshes all `PENDING` faces in passes. It runs
  the first passes in parallel, then sets `nthreads = 1` once
  `nIter > 2` ("self-intersections of 1D meshes are not thread safe"), and
  breaks at `nIter > maxRetries` (`Generator.cpp:755-801`). The 1D loop has
  the same shape (`Generator.cpp:569-610`).
- **Default.** `Mesh.MaxRetries = 10`, "also controls maximum number of
  recursive subdivisions when self-intersecting 1D meshes are detected"
  (`common/DefaultOptions.h:1430-1433`).
- **Determinism.** Per-face random perturbation is now seeded by (seed, face
  tag, retry) (`meshGFace.cpp:55-84`). The periodic path serialises its
  shared-curve mutation when more than one thread runs (:2060-2068). The
  non-periodic path calls `remeshUnrecoveredEdges` from inside parallel face
  meshing in passes 0–2 with no guard [own from code]: it writes
  `ge->lines` of a shared curve and un-meshes the neighbours. That is a race in
  Gmsh, not a defect of ours. A final `renumberMeshCanonically`
  (`Generator.cpp:806`) normalises numbering, not geometry.

### 1.4 Netgen (`31e85f81`) [src]

`NetgenGeometry::MeshSurface` meshes faces in order and counts failures
(`basegeom.cpp:875-1090`). `OCCGeometry::MeshFace` tries `PARAMETERSPACE` and
then `PLANESPACE`, and otherwise sets `facemeshstatus = -1`
(`occgeom.cpp:304-327`). Any failure skips surface optimisation for the whole
mesh (`basegeom.cpp:1081-1086`). Edge density comes only in advance, from
`Analyse`: curvature, segments per edge and close-edge `RestrictLocalH`
(`basegeom.cpp:360-484`). No face sends anything back to an edge.

### 1.5 Parasolid (V35 Functional Description) [src]

- §107.2.2.1 (p. 1809–1810): "Parasolid checks whether the face contains any
  inner loops that lie very close to the outer face boundary. If it does,
  Parasolid reduces its chordal tolerance locally so that the boundary chords
  do not intersect any inner loops." With geometry matching this leaves
  "small triangular gaps".
- §107.2.2.2: topology matching (the default) "checks that the boundary
  vertices on a facet mesh always match with vertices on the boundaries of
  neighboring meshes (possibly splitting facets in the neighboring facet mesh
  in order to meet this condition)". This is face-to-neighbour feedback that
  adds vertices without re-meshing the neighbour.
- §107.2.16 (p. 1825–1827): with incremental faceting, "the current
  tessellation … does not satisfy the required faceting controls … The usual
  consequence of this is that additional facet vertices are added to model
  edges". With the auto method, "any faces adjacent to an edge that has been
  refined will be automatically refaceted". With the attribute method and
  refinement off (the default), a refinement error is reported for each face
  on that edge and those faces are not faceted.
- §107.2.12 (p. 1820) and §107.2.7 (p. 1815): facets are split recursively
  "until the criteria are satisfied", but stop at `min_facet_width`
  "regardless of the other criteria". That is a length floor, not a count.
- The FD gives no round limit for edge refinement.

### 1.6 SmTess (NVIDIA usd-brep `c9c5979f`) [src]

The NVIDIA SmTess study covers this in detail (§Phase 1). Faces first split
their shared edges to their surface-cache leaf size (`SplitFaceEdgesToUVSize`,
`SmTess.cpp:3218-3541`), with one rebuild of the UV trims on failure
(:11582-11637). `CheckPolygonIntersections` and `CheckPolygonNesting` then
split shared edges through `TrySplitSegment`. Any split forces every face to be
rebuilt, up to 100 passes (:11765-11831). The code comment admits that only the
faces on a split edge need rebuilding. Faces are processed serially, and
splits made by earlier faces are visible to later ones, so the result depends
on face order [inf].

### 1.7 CGAL Mesh_3 feature protection (`28811b67`) [src]

This is the closest analogue to a geometric stop. `refine_balls`
(`Protect_edges_sizing_field.h:1720-1838`) runs a loop:

- When two non-adjacent protecting balls intersect, or the approximation is
  too large, both radii shrink to `min(d/2.1, r)`.
- New sizes are collected per vertex as a minimum, then applied, which makes
  each round commutative.
- A ball whose new size falls below `minimal_size` becomes "special" and stops
  refining. That is a geometric floor.
- The loop is also capped at `refine_balls_max_nb_of_loops = 29`. The comment
  says the constant "has had different values in the Git history: 9, 99, and
  now 29" (:96-106), and on exit it warns "It might result in a crash"
  (:1840-1844).

The theory behind protecting balls (Cheng–Dey–Ramos) bounds ball radii below
by lfs. The code adds a count because its floor and the theory do not meet in
practice [inf].

### 1.8 Others

- **SMESH**: per-sub-shape compute states. A failed face re-arms only on
  `SUBMESH_COMPUTED` or a hypothesis change (healing paper §6.1,
  `SMESH_subMesh.cxx:1363-1900`), with no edge re-split [src, secondary].
- **truck**: `insert_to` drops points that fail insertion and skips constraints
  that `can_add_constraint` rejects (`truck-meshalgo/src/tessellation/triangulation.rs:494-530`).
  That is a silent accept with no feedback [src].
- **Owned retired path**:
  - Faces return `needs_boundary` with physical interval requests. Requests
    are sorted and deduplicated, the boundary is refined, and faces whose
    boundary generation moved are reset
    (`native/mesher/adaptive_component.cpp:440-531`).
  - The rounds are capped at `maximum_boundary_rounds = 256`
    (`adaptive_component.hpp:13`), and the watch used 3.
  - Requests came from failed certificates, which have been excised.
  - Synchronous generations with canonical request order made it
    deterministic [code].

### 1.9 What the systems agree on, and where they differ

- They agree that an edge is sampled once and shared. Trouble found by a face
  splits the edge, and every face on that edge is checked again or re-meshed.
  All of them stop on a count.
- The trigger sets differ:
  - crossing only (OCCT);
  - crossing plus 3D invalidity (Gmsh);
  - crossing plus nesting (SmTess);
  - ball overlap plus approximation (CGAL);
  - a loop close to the outer boundary (Parasolid).
  No system read tests containment without crossing, except SmTess's nesting
  check and Parasolid's inner-loop proximity.
- The split rule differs: the whole edge at δ/3 (OCCT), the offending segment
  at its parameter midpoint (Gmsh, SmTess), or the radius set from the
  distance (CGAL).

## 2. Termination (question 2)

### 2.1 What the sources establish

- **Ruppert, and Shewchuk 2002 §3.2–3.3** [src]:
  - lfs(p) is the radius of the smallest disk centred at p that meets two
    non-incident features. It is 1-Lipschitz (Lemma 2).
  - Each inserted vertex's insertion radius is bounded below by its parent's
    radius times a factor, or by its lfs (Lemma 3).
  - When no cycle in the flow graph has a product below 1 (input angles ≥ 60°,
    ratio bound ≥ √2), no edge shorter than the minimum lfs is created, and the
    algorithm "will run out of places to put vertices" (Theorem 4).
  - Small input angles break this. Ruppert's algorithm "sometimes fails to
    terminate", and concentric shells or the "terminator" (§6) restore
    termination at the cost of angle quality near the small angle.
- **Pav and Walkington 2005** [src]:
  - The input is a piecewise regular curve complex (Def. 1). lfs is measured
    to "two mutually disjoint features" and "has a positive minimum value"
    (Def. 2, Note 1).
  - Curve separation σ, for curves sharing an endpoint, is the minimum of
    |z₁−z₂|/max(|z₁−x|, |z₂−x|) over points sufficiently far from x
    (Def. 3).
  - Total variation of the tangent is τ ≤ π/3, "achieved by splitting curves
    in a preprocessing step" (Def. 4, after Boivin and Ollivier-Gooch).
  - Lens containment (Lemma 1): a curve with variation < τ satisfies
    ∠xzy ≥ π − τ, so it lies in the diametral lens of its chord.
  - Theorem 1: lfs(p) ≤ C₁|p−q| for curve midpoints, which gives termination
    by ball packing.
  - σ > 0 is a premise.
- **Amenta, Bern and Eppstein 1998** [src]:
  - For a smooth curve, LFS is the distance to the medial axis. A disk of
    diameter ≤ LFS meets the curve in one arc (Corollary 3).
  - With r < 1 sampling, adjacent samples are Delaunay neighbours
    (Theorem 12), and the polygon is within (r²/2)·LFS of the curve
    (Theorem 13).
  - This covers smooth closed curves only. Corners make the medial-axis LFS
    zero, which is why the PSLG-style lfs above is the one to use.
- **Spielman, Teng and Üngör 2002** [src, abstract and introduction]: parallel
  refinement by independent sets needs O(log²(L/s)) rounds, or O(log(L/s)) for
  quasi-uniform meshes, where L/s is the ratio of domain size to smallest
  output edge. This is the round count to expect from any synchronous
  check-and-split scheme.
- **Chew 1993** [src, abstract and §1]: guaranteed quality on curved surfaces,
  with region boundaries respected.

### 2.2 What they do not give

1. Our chains are refined for boundary topology, not for triangle quality.
   The quality machinery (circumcentres, encroachment) does not apply. The
   band is triangulated afterwards by CDT (design §5.3).
2. Splits are at the dyadic parameter midpoint, so that keys are shared across
   faces. Pav and Walkington split at the curve point on the chord's bisector
   (Corollaries 1–2) or under their Assumption 1.
3. A chain is shared between faces with *different* UV charts. One 3D edge has
   one UV arc per face occurrence, each with its own κ, lfs and corners.
4. A cusp (α = 0) is excluded by σ > 0.
5. Requirements from the surface (band ε, 3D fold) are outside a 2D theory.
   Gmsh's cone-apex comment shows that splitting edges does not converge when
   the trouble is not on the edge.

### 2.3 Derivation [own]

**Setting.**

- Face F has a trim complex in its UV chart. Its points are the vertex UVs of
  the occurrence. Its curves are the UV arcs of each edge occurrence: the UV
  preimage on F's surface of the resolved 3D edge, which is the pcurve where
  it agrees and the foot-point curve otherwise.
- A periodic face uses the lifted chart that matches its seam convention. The
  two seam occurrences are distinct curves, one period apart.
- A pole edge is a UV segment curve.
- Each edge e carries a dyadic tree of parameter intervals. The chain is a
  cut of that tree.
- For a segment s (a tree node) and an occurrence of e in F:
  - ℓ(s) is the UV arc length;
  - τ(s) is the total variation of the UV tangent angle on the arc;
  - ℓ₃(s) is the 3D arc length;
  - lfs̲(s) is the minimum over the arc of lfs, measured to features disjoint
    from e. Pav–Walkington's lfs excludes e itself and its endpoint
    vertices.

**Premises (checked, never assumed).**

- (A1) Each UV arc is C², regular, and has curvature bounded by κ̄ on every
  sub-arc. The speed ratio of the parameterisation over each segment is at
  most ρ.
- (A2) The trim complex is valid: arcs meet only at shared vertices, so
  lfs̲ > 0.
- (A3) At every vertex v the corner angle α_v between the two UV tangents is
  positive, or the cusp certificate below holds.
- (A4) For P4, the band's second-derivative bound M̄ is finite.

**Predicates on (s, F)** [own]. Each is hereditary: if s passes, every
descendant passes.

- **P1 flatness:** τ(s) ≤ τ₀ (τ₀ ≤ π/3). τ only shrinks on sub-arcs.
- **P2 separation:** w̄(s) := ½·ℓ(s)·tan(τ(s)/2) < ½·D(s), where D(s) is a
  lower bound on the distance from arc(s) to every feature of F other than
  e's own arc and the parts of features inside s's corner balls (P3).
  - w̄ bounds the lens half-width: Lemma 1 lens of chord L ≤ ℓ with
    angle π−τ, whose sagitta is (L/2)·tan(τ/2).
  - w̄ is monotone under subdivision, because ℓ and τ both shrink, and D only
    grows on sub-arcs.
- **P3 corner:** for each endpoint vertex v of e in F with corner angle α_v,
  let the corner ball be B_v = B(v, R_v). R_v is the largest radius at which
  the tangent variations of the two incident arcs from v satisfy
  τ₁(R_v) + τ₂(R_v) < α_v/4. Inside B_v, each arc lies in a cone about its
  tangent at v of half-angle τ(R_v) (mean-value argument). The two cones meet
  only at v.
  - A segment inside B_v passes when its lens stays in its cone. That holds
    when w̄(s) < dist(s, v)·sin(α_v/4), and for the segment touching v when
    τ(s) < α_v/4. The lens half-angle at an endpoint is τ, and the chord
    direction is within τ of the tangent at v.
  - A segment that crosses ∂B_v is split until it is inside or outside.
  - All three conditions shrink with the segment.
- **P4 band:** ℓ₃(s) ≤ L_adm(F, s) = 2·√(2ε_b/M̄(F, tube(s))). M̄ bounds the
  surface's second directional derivative over the tube of geodesic radius
  ℓ₃(s) around the arc. ε_b is the band's share of ε (design §6.1 Waldron
  rule; a boundary triangle has r_min ≥ ℓ₃/2). The tube shrinks with the
  segment.

**Lemma 1 (sufficiency) [own].** Suppose every leaf of the chain passes P1–P3
in F. Then:

- the polygonal loops of F are simple;
- they nest exactly as the curved loops do;
- every curve point of another loop lies outside every chord's cap.

*Argument.*

- Each leaf's arc and chord lie in its lens (Lemma 1 of Pav–Walkington, with
  the lens convex as an intersection of two disks). So any chord crossing, or
  any other loop's point inside a cap, puts two lenses in contact.
- Non-incident leaves: their lenses are within w̄ of their arcs. P2 gives
  w̄ᵢ + w̄ⱼ < ½Dᵢ + ½Dⱼ ≤ dist(arcᵢ, arcⱼ), so the lenses are disjoint.
- Leaves of the same arc: with τ ≤ π/3 the arc is monotone along its chord
  direction, so its polyline is monotone and cannot cross itself.
- Incident leaves inside a corner ball lie in disjoint cones by P3.
- Hence no crossing and no containment. The region swept as each arc is
  straightened to its chord stays inside that arc's lens, which no other
  feature meets, so loop nesting is preserved.

P4 adds that a boundary triangle can meet ε_b at an apex placed about ℓ₃/2
away. With design §5.3's 3D orientation check this pre-empts Gmsh's invalid-element
branch, except at singularities, where (A4) fails and the face refuses. The
design handles poles with fans.

**Lemma 2 (termination and bounds) [own].** Let L\*(s) be the arc length below
which each predicate is guaranteed:

- P1: ℓ ≤ τ₀/κ̄.
- P2: τ ≤ κ̄ℓ, so w̄ ≤ ½ℓ·tan(κ̄ℓ/2) ≈ κ̄ℓ²/4 < ½D, giving
  ℓ < √(2D/κ̄). This is infinite for κ̄ = 0, so straight UV arcs never split.
- P3: ℓ < α_v/(4κ̄) for the segment at v. Inside the ball,
  ℓ ≲ √(4·dist(s,v)·sin(α_v/4)/κ̄).
- P4: ℓ₃ ≤ L_adm.

Under (A1)–(A4), L\* has a positive minimum L\*_min on every edge. A segment is
split only if it fails, which implies ℓ > L\*. By (A1), each child keeps a
fraction of its parent's arc between 1−q and q, with q = ρ/(1+ρ). Therefore:

- no arc shorter than (1−q)·L\*_min is ever created;
- each occurrence's chain has at most ℓ_edge/((1−q)L\*_min) + 1 samples, and
  the graded form is ∫ ds/((1−q)L\*(s)), using lfs's 1-Lipschitz property
  (Shewchuk Lemma 2; Pav–Walkington Note 1);
- tree depth is at most ⌈log_{1/q}(ℓ_edge/L\*_min)⌉ + 1.

The chain is the union over occurrences. Its count is at most the sum of the
per-occurrence bounds, and in practice the maximum, because the same dyadic
nodes satisfy all occurrences.

**The floor (what replaces a count).** A segment stops without passing in four
cases.

1. **Arithmetic.** The midpoint parameter is not representable apart from the
   endpoints (interval width < c·ulp(t)), or a child chord in UV is shorter
   than the predicate floor at which `direct_cdt`'s fixed-point inputs separate
   points. Doubles bound depth by about 52.
2. **Premise broken.**
   - D(s) is measured at or below the arc separation floor: the arcs of
     different features touch or cross, so the source trim is self-intersecting
     or loops touch (A2 fails).
   - κ̄ or M̄ is unbounded (A1 or A4 fails).
   - α_v = 0 and the cusp certificate fails (A3 fails).
3. **Packing bound exceeded.** Depth goes past the bound computed from the
   measured L\*. The geometry model was wrong (for example a κ̄ bound that does
   not hold), so the segment refuses and names which bound.
4. **Output-size guard.** The predicted count from Lemma 2 exceeds the request's
   budget (design §5.5). This is refused before any refinement runs.

Each case produces a refusal record (§5.2). None is a silent accept.

**Cusp certificate (α_v = 0) [own; the literature excludes it].**

- Take two arcs leaving v with a common tangent and curvatures κ₁ ≠ κ₂ on the
  same side. They separate as |κ₁−κ₂|r²/2 at distance r. The first chords from
  v are straight and distinct, at an angle of about |κ₁−κ₂|ℓ/2, so they meet
  only at v.
- In the frame y = κᵢx²/2, the polygon formed by the two chords is the thin
  wedge between them. It is simple and has the same topology as the crescent.
- For later segments at distance r ≥ ℓ_first, P2 applies with
  D ≈ |κ₁−κ₂|r²/2. The resulting grading ℓ(r) ≲ r·√(2|κ₁−κ₂|/κ̄) is
  geometric, so the count per decade of r is finite. The innermost segment is
  never split for the cusp, because its chords already certify.
- The certificate needs |κ₁−κ₂| to be measurable above the floor. If the arcs
  osculate to the evaluated order, so that they coincide near v, the face
  refuses with "trim curves tangent-coincident at vertex".

Lens disjointness alone cannot certify a cusp: the first lenses always overlap
near v. P3 therefore needs this special case.

### 2.4 Why the healing paper's stop rule needs correcting [own]

Tolerant-geometry-healing §7.4 item 5 proposes: stop when the conflicting
segments are shorter than the UV distance to the nearest non-adjacent loop
feature. Two problems:

- **Too strong.** A segment's chord deviates from its arc by its lens width,
  about ¼·ℓ·τ, which is 0 for a straight pcurve. Comparing ℓ with D demands
  ℓ < D everywhere a loop runs close to another. P2 needs only ℓ < √(2D/κ̄), which is unbounded for a straight
  arc (κ̄ = 0), so the proposed rule over-refines lines and large-radius
  arcs without limit. For example,
  two straight parallel slot sides 0.01 mm apart would get samples every
  0.01 mm along the whole slot.
- **Too weak at corners.** Adjacent features meeting at a small angle are
  "adjacent", so the rule never compares them, yet their chords can cross a
  few segments away from the vertex. P3 covers this case.

Replace the rule with P2 and P3. The healing paper's other point stands:
refuse, never cap silently.

### 2.5 ε does not set the topological floor [own]

Stopping at a length tied to ε, as Parasolid's `min_facet_width` does in
spirit, would either emit crossing loops or refuse valid faces whose loops are
closer than ε in UV-mapped 3D. The topology must be right at any ε. A neck
thinner than 2ε in 3D is a thin-feature case for design §5.6. That planning
may tighten local tolerance or refuse. The closure stage carries out whatever
it decides and does not make the decision itself. Without such a decision, the
sample count near a neck of width D grows only as ℓ_neck·√(κ̄/D), and the
output-size guard bounds it.

## 3. Determinism under parallel scheduling (question 3)

**Theorem (uniqueness) [own].** Suppose every predicate is a pure function of:

- the segment's dyadic interval;
- the face's source geometry and resolved arcs;
- the request (ε, flags);

and none depends on how other edges are sampled. Then the set of dyadic nodes
split in the final chain is

N = { n : every proper ancestor of n fails some predicate in some
occurrence }.

This is a pointwise definition, so N is the same for every evaluation order,
schedule, thread count, face order and occurrence permutation. Heredity makes
N closed under ancestors, so N is a tree whose leaves pass.

Consequences:

- **No schedule generations and no barriers.** Each edge closes independently.
  "Requests" from different occurrences merge by maximum required depth per
  node. That merge is a commutative, associative and idempotent join, the kind
  of operation Blelloch et al. 2012 use for internal determinism [src].
- **Canonical order of re-splits** follows from the keys: (edge id, dyadic
  path). Sample IDs are issued in key order (design §5.5 canonical order).
- **What is recomputed for a split.** Per new sample:
  - M2's L1 point solve;
  - the L3 blend;
  - one UV per occurrence (pcurve or seeded projection);
  - the predicates for the two children in each occurrence.

  Nothing else.
- **Requirement on M2 [own].** M2 §5.3 L3 spreads each vertex correction as
  (1−s)Δ₀ + sΔ₁ with s the "normalised arclength". If s is taken from the
  *polyline*, an inserted sample moves every other sample on the edge. Take s
  from the curve parameter, or from the source curve's arc length, so that
  existing positions do not change when samples are inserted. Otherwise
  closure stops being local, and incremental-equals-fresh fails.
- **Floating point.** Predicates must not depend on reduction order. Minimum
  is order-independent in IEEE arithmetic, and sums are not. Arc length and
  variation come from fixed-order quadrature per segment. The lfs lower bound
  is a minimum over BVH candidates.
- **Backends (D-determinism).** A chain is computed once, by the edge's owner,
  on one backend. Faces meshed on other backends consume it. Near a threshold,
  CPU and GPU could otherwise split differently and break shared identity. This
  answers the lifecycle map S17 contradiction for the edge layer.

**Why check-and-split rounds are not order-free [own].**

- A crossing between chords A and B is cleared by splitting either one. The
  check is not hereditary: after B is split, A may no longer be implicated.
- Under asynchronous scheduling, the fixpoint therefore depends on timing.
  Both outcomes are valid but they differ, which violates D-determinism.
- Synchronous rounds (OCCT, the owned path) fix this, at the cost of a barrier
  per round over the dirty set.
- If the check-and-split variant is kept as an arena arm, it must run as
  synchronous rounds per definition with the requests from round k merged
  before round k+1. Spielman–Teng–Üngör's O(log²(L/s)) round bound is the
  expectation [src].

**Cost of making predicates pure.**

- D(s) must be a lower bound computed from *source* arcs, not from the current
  chords of other edges.
- Mechanism [own]:
  - each feature carries its own dyadic lens hierarchy, built from its curve
    bounds and independent of the chain;
  - the query descends a neighbour's hierarchy only until its lens width falls
    below a fixed fraction of the distance being certified;
  - the first level is a box tree over lens boxes, which is OCCT's
    `FaceChecker` cost.
- Most queries resolve at the first level. That is a hypothesis (H3).

## 4. Interactions (question 4)

**Growth clusters** (parallel-growth-clusters §3–5):

- C1 lattice clusters: the lattice is built after closure. Cluster seams end at
  the lattice front, never on trims, so closure does not affect them.
  mesh(K=1) ≡ mesh(K=k) is unaffected.
- C2 band components and separators: computed after closure. Separators end on
  existing trim samples, and closure only adds samples. A band component that
  touches no split segment is unchanged.
- C4 lattice-aligned iso-line edges: closure splits are dyadic, so a split at
  depth d on an aligned edge stays on lattice level d. The adjacent lattice
  must refine to that level, and the 2:1 balance pass handles that inside each
  face. Closure therefore runs before lattice levels are fixed.

**Background healer and staged resolution** (design §7; M2 §5):

- Closure needs final arcs, so it runs after an edge's position class settles.
  A move made later (R3 Newton; M2's transverse leap of up to ¼ local feature
  size) changes the arc, and closure for that edge re-runs from scratch.
  Heredity covers refinement, not moved geometry.
- The lfs in P2 is the *UV trim lfs of each face*. It is a different quantity
  from M2's 3D feature size, so M2's ¼-lfs leap bound gives no closure
  guarantee.
- Dependency: the closure of edge e in face F reads the arcs of F's other
  edges, through D(s). The face pending counter therefore covers two hops. F
  can be meshed when its own edges are resolved, and the closure of each of its
  edges is final once every face incident to each of its edges has its edges
  resolved.
  - The default is to wait.
  - The speculative variant (design §7) may mesh earlier. A split arriving
    later then rebuilds only the band components that touch the split
    segments. Interior lattice cells are kept unless the edge is C4-aligned.
- New closure samples on R3 or R4 edges become deferred items, batched by kind
  like any other.
- D-provisional: a provisional mesh, on unsettled arcs or unclosed chains, is
  display-only and keeps its status. Physics reads only final meshes.

**Caching and incremental remeshing** (lifecycle map S16–S17):

- A face's content key includes its boundary-edge sample digests.
- An edge's chain depends on every face incident to it, and through D(s) on the
  features its lfs queries touched. Record those dependencies from the query.
- The invalidation radius after an edit is therefore the faces on changed
  edges, plus the faces on any edge whose closure query read a changed
  feature. This is Parasolid's "faces adjacent to an edge that has been
  refined" plus the read set.
- Pure predicates make fresh and incremental closure identical, which the
  design §10 metamorphic oracle needs.

**Coordination policy for M5 §5.2–5.3.**

- The whole-mesher study names two policies. Policy 1 is settle and protect a
  sufficient schedule, where "a failed boundary-cell bound must produce a
  global edge request". Policy 2 is a coupled queue with schedule generations
  and an open termination question.
- The answer [own]: policy 1, with "sufficient" defined as the hereditary
  predicates P1–P4 over every incident occurrence. The chain is frozen after
  closure.
- Policy 2 reduces to policy 1 once its requests are hereditary. Without that
  it needs synchronous generations to be deterministic. Keep it only as the
  arena comparison arm.

## 5. Authored families and the refusal record (question 5)

### 5.1 Families

Each family has a known answer. "Pass" means the residual detector stays
silent, the shared-edge identity oracle holds across thread counts {1, 2, 12,
24} and shuffled face order, and the predicted bound from Lemma 2 is not
exceeded.

| # | Family (parameters swept) | Branch forced | Expected |
|---|---|---|---|
| F1 | Planar disk radius R with an off-centre circular hole; gap g to the outer circle; ε sets the outer chord coarse. Sweep g/sagitta from 10 down to 0.01 | P2 crossing (outer chord into hole) | Outer edge splits only near the hole; count ≈ Lemma 2 |
| F1b | As F1, but a small hole lies wholly inside the cap between an outer chord and its arc | P2 containment without crossing (OCCT `FaceChecker` misses this) | Split; detector silent; arm B (OCCT-style) fails the nesting oracle |
| F2 | B-spline surface with clustered knots or extreme weights, so a 3D-straight edge has an S-bent UV pcurve with a neck | P1 (UV variation) then P2 | Splits in that face's chart only; the other face's chain gains samples, harmlessly |
| F3 | Two inner circles gap g, both small against the face | P2 between inner loops | Splits on both circles near the gap |
| F4 | Crescent: face between internally tangent circles (cusp vertex), Δκ swept to 0 | Cusp certificate; Δκ→0 refusal | Graded chains; refuse "tangent-coincident" when Δκ is below the floor |
| F5 | Wedge corner with α from 60° down to 0.1° between straight or curved edges | P3 corner ball | Splits only for curved edges; count ∝ 1/α near v |
| F6 | Saddle z = κxy on the unit square (straight boundary lines) next to plane faces | P4 band: straight edge with no curve rule; the face needs ℓ₃ ≤ 2√(2ε/M̄) | Line edges split; the plane neighbour gets the samples |
| F7 | Cone face trimmed by a curve passing a distance d from the apex, d → 0; then through the apex with no vertex | P1 + P4 near the singularity; (A4) failure | Graded refinement for d > 0; refusal at d = 0 unless the trim has an apex vertex (pole fan) |
| F8 | Source trim whose pcurves cross; inner loop touching the outer loop at a point with no vertex | (A2) failure | Face refused with crossing UV and edges; the rest of the part meshes |
| F9 | 3D neck thinner than 2ε inside one face (two loops close in 3D) | P2 at small D; §5.6 interaction; output-size guard | Follows the §5.6 decision; count within Lemma 2 or refused up front |
| F10 | Trim loop crossing the seam; multi-turn faces; shifted periodic ranges | Lifted-chart lfs; two seam occurrences as distinct curves | No false crossings across the period; seam chains identical in both occurrences |
| F11 | Non-manifold edge (three faces) and same-face seam (two occurrences), each occurrence wanting a different depth | Max-depth merge over occurrences | Identical chain for every occurrence permutation |
| F12 | Strip of k faces, each with an inner loop near the shared edge on alternating sides | Cascade (arm B rounds) against no cascade (closure) | Closure: no generations. Arm B: rounds ≤ k; compare samples |
| F13 | Negative control: a flag halves the D(s) lower bound's safety factor, or turns off P2 | Residual detector, refusal path | The detector fires and refuses with the predicate named; the run exits non-zero in the oracle harness |
| F14 | Parameter speed ratio ρ swept to 10³ (nonuniform knots on a straight 3D line) | q in Lemma 2 | Count within the bound; validates the (1−q) factor |

These extend design §9.4's families ("near-tangent trims", "trim loops crossing
the seam", "apex without vertex", "N-holes") with the four that only this loop
forces: F1b, F4, F6 and F12.

### 5.2 Refusal record

One record per face occurrence that cannot close. The other faces on the edge
keep the chain and mesh normally (D-partial-output). Fields:

- face id, edge id, occurrence (sense, seam side), definition and occurrence ids;
- segment key (edge id + dyadic path), its t interval, UV and 3D endpoints;
- predicate (P1, P2, P3, P4 or residual detector) and the stop case (§2.3:
  arithmetic, premise A1–A4, packing bound, output-size guard);
- measured quantities: ℓ, ℓ₃, τ, w̄, D(s) with the feature that bounded it
  (edge or vertex id and its segment key), α_v, κ̄, M̄, L\*, the floor value and
  the predicted and observed depth;
- for a crossing or touching, the UV and 3D witness point;
- status `refused` for the face and `incomplete` for the part. Where the lfs
  query could not resolve above the floor, the status is `indeterminate`
  (lifecycle map X7), never `valid`.

## 6. Proposals for M5, with hypotheses

Adopt the closure stage between design §5.2 (edge resolution) and §5.3 (face
meshing). The design's "an edge chain is final only when items 1–4 are
settled" gains item 5: closure passed in every incident occurrence. This
replaces the healing paper's post-hoc loop.

Arena flags (two-level, so the 2^(8−2) design is undisturbed):

- `boundary_closure`: hereditary closure (A) or OCCT-style synchronous
  check-and-split rounds (B), with crossing plus nesting checks and splits
  limited by the §2.3 floors, not a count.
- `split_point`: dyadic parameter midpoint, or curve point on the chord
  bisector (Pav–Walkington Corollary 2) snapped to the nearest dyadic key. A
  follow-up level only.
- τ₀: π/3 or π/6. A follow-up.

| # | Hypothesis | Measurement | Falsifier |
|---|---|---|---|
| H1 | Arm A's chain sets are identical across thread counts and face orders | Edge-sample key sets on corpus + F11, threads {1,2,12,24}, 8 shuffles | Any difference |
| H2 | P1–P4 are sufficient: the residual detector never fires on inputs meeting A1–A4 | Detector counts on corpus + F1–F14 | Any firing on a premise-valid input |
| H3 | Closure is cheap: ≤ 5% of edge-stage time; ≤ 1% of segments need a lens-hierarchy descent | Per-stage timings with lane context (common brief) | Above either bound |
| H4 | Lemma 2 bounds the count: closure samples per edge ≤ the predicted ∫ds/((1−q)L\*) | Predicted against measured per edge | Any edge above its prediction (a bound is wrong) |
| H5 | A adds no more than 1.5× the samples B adds on F1–F3, F12 | Sample counts per family | A > 1.5× B; then reduce conservatism in w̄ or D |
| H6 | B needs more than one round on some real inputs, so its barrier is real cost | Round histogram, corpus | B always finishes in one round (then A's advantage is determinism alone) |
| H7 | Most face occurrences need no closure split | Fraction per corpus class | Too large a fraction to call closure an exception (say > 10% of faces): profile which predicate dominates |

## 7. What the literature does not say

Each item is followed by the measurement or authored family that settles it.

- No system read derives its feedback stop from geometry. All of them stop on
  counts whose values have no stated derivation (H6 measures typical rounds).
- No termination proof covers cusps (α = 0) in curved trim complexes.
  Pav–Walkington require σ > 0; Shewchuk's terminator handles small positive
  angles. The cusp certificate here is a derivation to test on F4.
- No source gives an a-priori rule for chord sufficiency across several UV
  charts of one 3D edge. P1–P4 per occurrence with a max-depth merge is our
  derivation (F2, F11).
- Parasolid's round limit and its definition of "very close" inner loops are
  not documented in the FD (shopping list).
- No published measurement exists of how often face-to-edge feedback fires on
  real CAD, or of its cost share. Measure with H3, H6 and H7 on the corpus.
- Boivin and Ollivier-Gooch 2002, the source of the τ ≤ π/3 preprocessing and
  of the π/3 input-angle analysis, was not obtained. It is cited here only
  through Pav–Walkington.

## 8. Defects, operator questions

- **Defects in our code:** none found by this lane's reading. The retired
  path's 256-round count (`native/mesher/adaptive_component.hpp:13`) is
  retired with it. Two upstream items are noted for the record, not for
  beads:
  - Gmsh calls `remeshUnrecoveredEdges` without a guard inside parallel
    passes 0–2 (`meshGFace.cpp:1226-1228`).
  - OCCT's `FaceChecker` cannot see loop containment.
- **Operator questions:** none. The branches this loop needs are settled by
  D-partial-output (refuse the face, mesh the rest), D-determinism and
  D-provisional. The thin-neck policy belongs to design §5.6 and its lane.

## For SHOPPING_LIST.md

- Boivin C., Ollivier-Gooch C., "Guaranteed-quality triangular mesh generation
  for domains with curved boundaries", IJNME 55(10):1185–1213, 2002. Wiley,
  paywalled. Wanted for the curved-boundary termination argument and the
  τ ≤ π/3 preprocessing.
- Cheng S.-W., Dey T.K., Shewchuk J.R., *Delaunay Mesh Generation*, CRC 2012
  (book). Wanted for the piecewise-smooth-complex protection chapters.
- Cheng S.-W., Dey T.K., Ramos E.A., "Delaunay refinement for piecewise smooth
  complexes", DCG 2010 / SODA 2007. The author page served HTML, not the PDF,
  at fetch time. Retry.
- Miller G.L., Pav S.E., Walkington N.J., "When and why Ruppert's algorithm
  works", IMR 2003. Not fetched; open copies may exist.
- Parasolid PK reference pages for `PK_TOPOL_facet_2` incremental refinement:
  round limit and the definition of the inner-loop proximity test. This is
  already on the list as "Parasolid PK pages"; add these two questions to it.
