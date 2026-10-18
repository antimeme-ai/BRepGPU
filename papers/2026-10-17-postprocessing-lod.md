# Post-processing legality and LOD

Research lane M4-G18 of the fast-mesher campaign, 2026-10-17. Brief:
[docs/fast-mesher-campaign/briefs/M4-G18.md](../docs/fast-mesher-campaign/briefs/M4-G18.md),
read with [M4-common.md](../docs/fast-mesher-campaign/briefs/M4-common.md).
Stub: [lifecycle map](2026-10-17-geometry-lifecycle-map.md) §4.1 G18 (stages
S11, S12; S24 for sharpen-in-place). Bead `brepgpu-12s.19`. Nothing was built,
run, meshed or measured. Sources and read depth are in
[2026-10-17-postprocessing-lod-sources.tsv](2026-10-17-postprocessing-lod-sources.tsv).

Tags follow the lifecycle map: **[src]** a source establishes it; **[code]**
read in a repository at file:line (reference code at the revision pinned in
`papers/reference-catalog.tsv`); **[own]** this lane's derivation; **[inf]**
inferred from a source but not stated by it. Design section numbers refer to
the working draft `docs/FAST_MESHER_DESIGN.md`.

## 0. Answer

1. **One legality rule covers every in-core operation [own].** An operation on
   the mesh inside the mesher is legal only if every triangle it creates passes
   the predicates the face generator's own triangles must pass, evaluated by
   the same evaluator, and it changes no chain vertex, no B-rep vertex and no
   identity. The predicates are UV orientation, the face's density rule, the 3D
   orientation check (§5.3), and the topology condition for the operation.
   Under that rule the operation re-runs the generator's own checks on new
   triangles. It adds no second verification layer (BLACKBIRD tenet 5).
2. **The operations that pass it are few:** interior splits; chain splits made
   only by the edge stage (G2's loop); flips of non-constraint interior edges;
   half-edge collapse of interior vertices onto an existing vertex; tangential
   relaxation of interior vertices in (u,v) with exact re-evaluation; and
   operations that change only identity or attributes. Everything that moves
   vertices off the exact target, touches a shared chain face-locally, or
   merges across faces is forbidden in-core.
3. **Default operation set for M5.** The generator (lattice, θ-slabs, CDT with
   its own Lawson flips) plus interior refinement and edge-stage chain splits.
   Flips, collapses and relaxation are arena flags, off by default, to be kept
   only if a measured quality target (G13's angle cap, G14's needle limits)
   cannot be met by refinement alone (H1, §7).
4. **LOD is re-tessellation at a coarser ε. It is not new machinery:** it is the
   mesher called with another ε, cached under the G7/G9 mesh key. The ladder
   ε_k = 4^k·ε matches one dyadic level per step. With dyadic adaptive
   sampling, coarse samples are a subset of fine samples. Mixed per-face
   levels then stay crack-free by chain identity, and sharpen-in-place gets
   ancestor maps for geomorphs [own, §5].
5. **Decimation is downstream only.** It measures error against our mesh, not
   the B-rep. QEM and Nanite report estimates, not bounds. Decimation can never
   be finer than its base mesh. Its one job that re-tessellation cannot do is
   going below the topology floor (about one triangle fan per face). That is a
   renderer stage outside the mesher.

### The table

Guarantees: **D** every point within ε of the repaired target (§4.2); **M**
manifold (each closed-shell edge has two uses); **O** oriented (UV positive and
3D agreement); **S** no new self-intersection; **E** shared-edge identity (one
chain per physical edge, one 3D position per ID). "Generator predicates" means
UV orientation by robust orient2d, the face's density rule on each new
triangle, and the 3D normal check at the UV centroid (§5.3).

| # | Operation | Verdict | D | M | O | S | E | Check that makes it legal |
|---|---|---|---|---|---|---|---|---|
| A1 | Split inside a face (point in a triangle or on a non-constraint edge), new vertex = target evaluator at that UV | **Allowed** | automatic under Waldron/r_min rules (containment); re-run for the sphere circumradius rule | always (Hoppe 1993 §4.2) | 3D check on children | as construction | untouched | 3D check; density rule only where it is not monotone under containment |
| A2 | Split of a chain at the exact curve parameter (dyadic path key), same ID in every incident face | **Allowed, edge stage only** | chord rule automatic; incident faces' affected band triangles re-checked | yes | yes, after re-triangulating incident bands | as construction | preserved: one key | G2 loop: all incident faces re-mesh their affected band together |
| A3 | Flip of a non-constraint interior edge (not chain, trim, θ-slab line, seam, lattice/cluster line) | **Allowed under check; arena flag** | re-check (r_min can grow); automatic on planes and inside one cylinder/cone slab | new diagonal must not exist (Hoppe 1993 §4.2; CGAL `is_flip_topologically_allowed`) | quad strictly convex in UV; 3D check | as construction | untouched | generator predicates on both new triangles; strictly decreasing quality potential; flip cap |
| A4 | Half-edge collapse of an interior vertex onto an adjacent existing vertex (no placement) | **Allowed under check; arena flag** | re-check every new fan triangle | Hoppe 1993 link conditions | every new triangle UV positive (preserves the star's UV union); 3D check | as construction | untouched; collapse history recorded (G16) | generator predicates on the fan; link condition |
| A5 | Tangential relaxation of an interior vertex in (u,v), position re-evaluated exactly | **Allowed under check; arena flag** | re-check the star | unchanged | new UV in the kernel of the link polygon (all star triangles positive); 3D check | as construction | ID kept | generator predicates on the star; fixed iteration count; Jacobi or canonical order |
| A6 | Canonical reorder, ID-based weld at export, normal and attribute evaluation, strip and cluster packing | **Allowed** | no geometry change | — | — | — | by ID | none needed |
| F1 | Flip of any constraint edge: chain, trim, seam, θ-slab line | **Forbidden** | slab line: breaks Δθ bound | — | — | — | breaks the chain | — |
| F2 | Face-local move, slide or removal of a chain vertex or B-rep vertex | **Forbidden** (chain resampling belongs to the edge stage) | moves the boundary off the resolved edge | — | — | — | breaks one-position-per-ID | — |
| F3 | Full-edge collapse with optimal (QEM) placement | **Forbidden** | vertex placed off the surface | — | — | — | — | — |
| F4 | Smoothing in 3D projected onto the mesh | **Forbidden** | error accumulates against the mesh, not the surface | — | — | — | — | — |
| F5 | Positional weld across faces; nearby-vertex merge | **Forbidden** | — | can pinch | — | — | aliases distinct IDs | — |
| F6 | Hole filling, overlap union, orientation repair, T-junction repair, deletion of degenerate triangles | **Forbidden** (each signals a generator defect or a refusal) | — | — | — | — | — | — |
| F7 | Cross-face simplification or aggregation (pair contraction with t > 0) | **Forbidden in-core** | — | creates non-manifold regions | — | — | merges faces | — |
| F8 | Lawson or quality flips on plain-epsilon predicates with no tie margin and no cap | **Forbidden** (may not terminate) | — | — | — | — | — | — |
| L1 | Re-tessellation at ε_k > ε (LOD, sharpen-in-place stages) | **In the mesher**: the same operation with another ε | within ε_k of the target | yes | yes | needs the G4 rule at ε_k | yes, per level; nested if dyadic | the mesher's own contract at ε_k |
| W1 | Decimation (QEM, Nanite-style cluster DAG, Manifold `Simplify`) | **Downstream** | ε + δ, only if δ is a bound | only some tools | penalty or reject | not guaranteed | only if face boundaries are locked | the consumer's |
| W2 | Global remeshing (fTetWild, MMG) | **Downstream** | envelope or hausd added | tool-dependent | — | — | lost | — |
| W3 | Convex decomposition (CoACD), PhysX cooking | **Downstream** | — | requires 2-manifold input | — | — | lost | — |
| W4 | Float32 narrowing and export welding | **Separate export stage** (ours, not the mesher; G3) | G3 | G3 | G3 | G3 | ID weld | G3's reader-equivalent oracle |
| W5 | Slicer and consumer repair (PrusaSlicer merge, Cura union, hole stitching) | **Downstream** (theirs) | — | — | — | — | — | — |

## 1. What binds this lane

- **D-features**: never drop features above ε; defeaturing is out of scope.
  This rules decimation out of the mesher: a QEM decimator drops whatever it
  scores cheapest. Re-tessellation at ε_k meshes every face; faces below ε_k
  go through G10's virtual collapse at that ε_k.
- **D-consumers**: PrusaSlicer, Cura, one glTF renderer, and fTetWild as the
  analysis stand-in; PhysX with physics. Each of these modifies the mesh it
  receives (lifecycle map S14), so it must be told what stays true (§6).
- **D-provisional**: provisional meshes are display only. A LOD mesh at ε_k is
  not provisional. It is final for the request ε_k. §5.4 keeps the two apart.
- **D-determinism**: identical output across thread counts on one backend.
  Every optional pass below runs in canonical key order inside one
  independent unit (a face component or a lattice cell).
- **The fidelity contract** (design §4): deviation against the repaired target
  (§4.2), topology (§4.3), orientation (§4.4), three-way status (§4.6),
  self-intersection by construction (§4.8, §5.6).

## 2. The legality principle, derived

Every in-core operation is local to one face occurrence's UV triangulation,
except A2, which is edge-stage work. Six facts follow [own]:

1. **Coverage.** A flip of a strictly convex quad, a split, a half-edge
   collapse whose new fan triangles are all UV-positive, and a vertex move that
   stays inside the kernel of its link polygon each keep the UV union of the
   affected triangles unchanged. The face therefore covers exactly the same
   trim domain. No new trim excursion appears, and per-face accounting
   (§4.5) is unchanged. The positivity tests are the standard ones: CGAL
   `collapse_would_invert_face` and `check_normals`
   (`PMP_Remeshing/.../remesh_impl.h:726, 2039`); Garland–Heckbert's "compare
   the normal of each neighboring face before and after" (1997 §4); Gmsh
   rejects a move that increases the star's parametric area
   (`meshGFaceOptimize.cpp:659-687`).
2. **Deviation under containment.** For a rule of Waldron form, e ≤
   ½·r_min(T)²·M_T (design §6.1), a triangle contained in an accepted triangle
   has r_min no larger and M no larger: any disk enclosing the parent encloses
   the child, and the span bounds over a subset are no larger. Splits (A1)
   therefore pass with no new density test. Flips, collapses and moves create
   triangles not contained in any old one, so they re-run the rule.
   Rules that are not monotone under containment, such as the sphere
   circumradius rule ρ² ≤ 2Rε − ε² (§5.3), are re-run on every new triangle.
   An obtuse child can have a larger circumradius than its parent.
3. **Analytic slabs.** A planar triangle whose vertices lie on a cylinder
   inside one θ-slab [θ₀, θ₀+Δθ] lies inside the circular segment of that slab.
   Its distance to the axis is at least R·cos(Δθ/2), so its deviation is at
   most the sagitta. Any re-triangulation inside one slab (flip, collapse,
   move along the ruling) keeps D with no recheck. The cone behaves the same
   with its largest radius. Planes have zero deviation under any operation.
   Crossing a slab line breaks the bound, so slab lines are constraints (F1).
4. **The target evaluator.** Inside a boundary band next to an R2–R4 edge, the
   target is the surface displaced by the recorded edge field (§4.2). "Exact
   re-evaluation" therefore means the evaluator the generator used,
   displacement included. An operation that evaluates the raw carrier there
   breaks D by up to the recorded displacement.
5. **Self-intersection adds no new case.** New triangles stay inside their face's
   ε-tube and pass the same orientation checks, so the inter-face 2ε argument
   (§5.6) and the intra-face argument are the generator's own. Whatever G4
   decides those arguments need applies unchanged to A1–A5.
6. **Identity.** A1–A5 never touch a chain vertex, a B-rep vertex, or a
   lattice vertex on a cluster seam. Chain identity (E) and the oracle
   mesh(K=1) ≡ mesh(K=k) (§5.4) hold if candidate edges exclude lattice lines
   and slab lines and each cell or band component is processed in canonical
   key order. A collapse removes an interior vertex's ID and records it in the
   collapse history (G16; lifecycle S23). A move keeps the ID and changes its
   UV and position.

Termination [src + own]. A1 and A2 are bounded by the density limits of §5.3.
Each A4 removes one vertex. A5 runs a fixed iteration count. A3 needs a
potential that strictly decreases, for example the sorted angle vector
compared lexicographically, plus a flip cap. The cap is derived from the edge
count, and reaching it is `valid_goal_unmet` (§4.6). With plain-double
in-circle tests, a tie margin with a proven roundoff bound is the minimum.
Earcut's `1e-13·s²` margin fixed an infinite flip loop
(`earcut/src/earcut.js:995-1006`). libigl issue 521 shows epsilon predicates
cycling an unbounded Lawson loop (mesher-test-plans-meshgen §E1).

## 3. What mature systems do

| System | Flips | Collapse | Smoothing | Deviation check | Constraint rule |
|---|---|---|---|---|---|
| CGAL isotropic remeshing | valence/shape; refused if normals fold, if degenerate, or if the new edge exists (`remesh_impl.h:958-967, 1367-1373`) | refused if it inverts a face (`:726`) | tangential relaxation | none against a surface; vertices "projected on the input surface mesh" by default (`remesh.h:184-194`); a `projection_functor` can substitute an exact projector | "A constrained edge can be split or collapsed, but not flipped, nor its endpoints moved by smoothing" (`remesh.h:99`) |
| Gmsh (BDS, face optimisation) | angle test then quality (`meshGFaceBDS.cpp:156-185`) | refused for points on model vertices or edges (`BDS.cpp:1157-1165`) | Laplacian in (u,v) with `gf->point(after)`; rejected if star parametric area grows or min quality drops (`meshGFaceOptimize.cpp:659-687, 830-835, 986`) | none beyond the surface evaluation | swap refused on edges classified on a model edge (`BDS.cpp:1035`) |
| Netgen surface optimisation | allowed only if new and old normals stay within 30° of the geometry normal (`improve2.cpp:111-127`) | fixed points: everything not `SURFACEPOINT`, plus locked points (`:628-636`) | moves re-projected with `ProjectPointGI` and rejected if badness rises (`smoothing2.cpp:1059-1067`) | normal deviation only | no swap on a segment (`improve2.cpp:289, 733`) |
| MMG (mmgs) | swap refused if the estimated Hausdorff of the new edge exceeds `hausd` (`swapar_s.c:250, 303`) | ridge and geometric tags respected (`colver_s.c:143-209`) | on ridges, along the ridge only (`movpt_s.c:564-645`) | estimate from a cubic Bézier built on vertex normals, not the CAD surface | no swap on edge, ridge or singular tags (`swapar_s.c:74`) |
| Manifold `Simplify` / `Decimate` | swaps only to remove degenerate configurations | collapse keeps a subset of original vertices; refused if a face folds or obtuse angles worsen (`edge_op.cpp:992-1050`) | — | cost accumulates per vertex against `tolerance²` (`edge_op.cpp:374-375, 424-425, 446`); "all surfaces will have moved by less than tolerance" relative to the input mesh (`manifold.cpp:378-384`) | — |
| Old owned mesher (retired) | strict Lawson after construction; pole pieces excluded; accepted only if the shape potential falls and both new cells pass the certified allowance (`native/mesher/face_engine.cpp:1181, 1192-1239`) | — | — | certified per cell (retired with certification) | Python twin: "fixed vertices and source constraints survive" (`python/brepgpu/imported_meshing.py:839-840`) |
| NVIDIA SmTess | — | — | a smoothing pass exists but does nothing at the default zero passes (nvidia-smtess-study, pipeline step 6, `:5904`) | — | — |
| Parasolid | — | — | — | `quality = improved` runs "extra checks", recommended only "for purposes other than visualisation" (FD §107.2.18) | facets "repeatedly refined" to meet tolerances (§107.2.7) |
| Hoppe 1993 / 1996 | swap legal iff the new edge is absent | link conditions (1993 §4.2); collapses that change discontinuity-curve topology are disallowed or penalised (1996 §4.5) | — | self-intersection check "would be costly", replaced by a dihedral heuristic (1993 §4.3.1) | boundaries are discontinuity curves (1996 §4.5) |
| Garland–Heckbert 1997 | — | pair contraction to the quadric optimum, which lies off the surface (§4, eq. 1); t > 0 aggregates and "will regularly create non-manifold regions" (§3.1) | — | quadric error = summed squared plane distance; the published evaluation measures average error (§6.1) | boundary edges kept by penalty-weighted perpendicular planes (§5) |

Reading across [src + own]:

- Every system forbids flipping constraint or feature edges and freezes
  geometric vertices. F1 and F2 are universal.
- No system checks deviation against the exact CAD surface during
  post-processing. Netgen and Gmsh evaluate the surface but check only
  normals or quality. MMG estimates from a reconstructed Bézier. The old
  owned mesher was the exception, using certified allowance. With
  certification excised, the generator's density rule takes that role
  ([own], §2.2). The literature read does not offer this rule: it is our
  derivation, falsifiable by the offline deviation oracle (§10).
- Mature CAD meshers keep post-processing optional and off for display. NVIDIA
  smooths zero passes by default; Parasolid's extra checks are for
  non-visualisation uses.
- Owen 1998 §5 separates smoothing (positions only) from clean-up
  (connectivity changes). Constrained Laplacian smoothing accepts a move
  "only if element quality is improved". This is the A5 pattern.

## 4. Question 1: guarantees per operation

The table in §0 is the answer. Points that need more than a cell:

- **A1 interior split.** A split can never change topological type (Hoppe
  1993 §4.2). D follows by containment (§2.2) for Waldron rules. The child's
  3D check is not inherited, because its UV centroid differs, so it is re-run.
  This is the operation behind G13's cap refinement and G14's sizing caps. A
  cap whose long edge lies on a chain cannot be fixed inside the face; it
  needs A2.
- **A2 chain split.** This is G2's face-to-edge feedback, not
  post-processing. It is listed so M5 states that chains change only through
  the edge stage. The new sample takes the curve parameter on the edge's
  dyadic path, so every incident face derives the same key, and all incident
  faces re-mesh their affected band in one step (Parasolid FD §107.2.16;
  Kumar–Manocha 1994 §6.1). Chain-vertex removal along a curve is
  topologically legal by Hoppe 1996 §4.5 (an edge with #sharp = 2 at both
  ends). It still belongs to the edge sampler choosing fewer samples, never
  to a face-local pass.
- **A3 flip.** Legal only on edges the generator did not constrain. That
  excludes trim chains, θ-slab lines, seams, quadtree and cluster lattice
  lines, and pole fans (the old mesher excluded pole pieces,
  `face_engine.cpp:1200`). Inside the CDT band, a flip pass after a Delaunay
  construction can only trade Delaunayhood for another target, such as G13's
  angle cap. Its value is a measurement (H1), not a given.
- **A4 collapse.** Legal for interior Steiner points only. Its use is
  removing a generator Steiner point that landed next to a chain and left a
  sliver. The target keeps its exact position. Optimal placement is F3.
- **A5 relaxation.** In (u,v), never in 3D. The UV move keeps the vertex on
  the target by evaluation. A 3D move would need a point inversion onto the
  surface, which is costly and can fail near poles, and the defaults of
  CGAL-style remeshers project onto the mesh instead (F4).
- **F5–F6.** Equal XYZ never aliases two nodes (`PhysicalNodeKey`,
  `native/mesher/mesh_values.hpp:21-35`). Holes, overlaps, flipped triangles
  and T-junctions are absent by construction. Repairing them would hide a
  generator defect, and repair "depends on the downstream use" (Campen,
  Attene and Kobbelt 2012), which the mesher does not know. Each is a
  refusal record or a bug.

## 5. Question 2: LOD by re-tessellation against decimation

### 5.1 Two ways to get a coarser mesh

| | Re-tessellation at ε_k | Decimation of the ε mesh |
|---|---|---|
| Deviation | within ε_k of the B-rep target, by the mesher's own contract | ≤ ε + δ, where δ is the decimator's deviation from our mesh, and only if δ is a bound (Manifold: yes, relative to the input mesh, `manifold.cpp:378-384`). QEM gives a summed squared plane distance (GH 1997 §5); Nanite's error is "an estimate", and getting it right is "the hardest part" (Karis 2021, mesh-simplify slides) |
| Normals | exact per-corner normals at every level | re-derived or interpolated; Nanite mixes position and attribute error with weights, "a complete heuristic hack" |
| Shared edges and watertightness | one chain per edge per level | decimators lock group boundaries to avoid cracks (Nanite) or penalise boundary planes (GH §5) |
| Face IDs and association | kept (S23 map per level) | survive only if face boundaries are locked; collapsed vertices stand for several entities |
| D-features | holds at ε_k | violated by design |
| Finer than the base mesh | yes: any ε | never: "the highest achievable visual precision … is bound to the original world-space error tolerance the highest-level LOD mesh has been built with" (Claux–Barthe 2014 §2) |
| Below the topology floor | no (§5.3) | yes |
| Industry carriers | AP242 `tessellated_shape_representation_with_accuracy_parameters`, "alternative tessellations of a part with different levels of detail", by chordal and angular deviation, untested in MBx-IF (CAx-IF 3D tessellated RP v1.1 §4.3); Parasolid parameterises faceting by chordal tolerance and contrasts coarser and finer facets for the same body (FD ch. 27, mass properties by faceting); AP242 also links tessellated faces to exact faces | renderer-internal (Nanite cluster DAG), Hoppe's progressive meshes |

For the renderer and for sharpen-in-place, re-tessellation wins on every row
except going below the floor [src + own].

### 5.2 The ladder, nesting and cost [own]

- **Scaling.** Along a curve the sagitta of a chord of length L is about
  L²κ/8, so edge samples grow like ε^(−1/2). On a face, the Waldron rule
  r² ≤ 2ε/M gives triangle count ∝ area/r² ∝ 1/ε. Quartering ε is one dyadic
  level: it doubles curved-edge samples and quadruples curved-face triangles.
- **Ladder.** ε_k = 4^k·ε. The curved part of a full pyramid costs at most
  1 + ¼ + ¹⁄₁₆ + … = 4⁄3 of the finest level. Planes with straight edges cost
  the same at every level (§5.3).
- **Nesting.** The edge sampler bisects dyadically on the edge's parameter
  path (`DefinitionBoundaryForest` keys, kept by design §8;
  parallel-growth-clusters §3). Its split test is "bound(segment) > ε_k", and
  the bound does not depend on ε. A segment split at ε_k is therefore also
  split at every ε_{k−1} < ε_k, so the coarse sample set is a subset of the
  fine one. The incident faces' admissible-length term falls monotonically
  with ε, so it keeps the property. The 2:1-balanced quadtree is nested by
  the same argument, provided balancing depends only on cell levels. Two
  things break nesting:
  - uniform closed-form counts, such as the circle sagitta count n in §6.1,
    unless n is rounded up to base·2^j (cost: up to 2× samples on those
    edges);
  - the CDT band, which re-triangulates a superset of points differently.
  Nesting is a property to verify (H3), not something to assume.
- **Pop bound.** Two levels both lie within their own ε of the same target,
  so switching from level k to level k−1 moves a surface point by at most
  ε_k + ε_{k−1} = 1.25·ε_k. That bounds the visible pop with no further
  measurement.
- **Cache.** A level is a mesh key (definition, ε in definition units) under
  G7/G9. LOD needs no state beyond the key.

### 5.3 The topology floor [own]

Re-tessellation meshes every face. Each face needs at least V − 2 + 2h
triangles for V boundary samples and h holes. Each straight edge needs one
segment, and each closed curved edge at least three samples, since two
samples make a two-segment degenerate loop. Above the floor, coarsening comes
only from curved edges and curved faces. The OM10 watch has 9162 faces; its
floor is at least 9162 triangles and likely several times that. The real
number is a measurement (H4). For a far view of a whole assembly below the
floor, only cross-face simplification or impostors reduce further. That is
downstream renderer work (W1), fed by our finest or coarsest level.

### 5.4 Mixed levels, cracks and sharpen-in-place

- **Mixed per-face levels.** A view-dependent renderer gives neighbouring
  faces different levels. Three published answers:
  - Guthe et al. 2005 §7.1 conceal cracks with line strips of width 2ε_img.
  - Claux–Barthe 2014 detect and fill crack pixels in image space or by ray
    casting.
  - Kumar–Manocha 1994 §4.5 tessellate one representation of each shared
    curve for both patches.
  The design is already the third answer: one chain per physical edge, sized
  by the maximum over incident faces (§5.2 item 1). A chain's level is the
  finest level among its incident faces. A coarse face takes a finer chain as
  CDT constraints. Watertightness holds by identity, with no concealment
  pass [own]. Changing one face's level re-meshes the incident faces' bands
  only on chains whose level changed. This is Parasolid's refined-edge rule
  (FD §107.2.16), and invalidation is G12's.
- **Sharpen-in-place** runs coarse to fine, so it can only be
  re-tessellation. Decimation needs the fine mesh first. With nesting, every
  fine chain and lattice vertex has an ancestor, which gives Hoppe's
  geomorphs (1996 §3.2) for those vertices. Band triangles have no ancestors
  and swap atomically per edge-closed region (lifecycle S24), with the pop
  bounded as in §5.2.
- **Provisional against LOD.** A provisional mesh comes from unsettled
  resolution (R3/R4 queues, speculative chains). It is display only
  (D-provisional). A LOD mesh is settled and final for ε_k: it can be emitted
  as a valid mesh for that request, and a physics consumer that asks for ε_k
  gets a final mesh. Both carry status and ε in their record. Only
  provisional meshes are excluded from physics.
- **Self-intersection at coarse levels.** The 2ε argument becomes 2ε_k, so
  more thin features fall under G4's rule at coarse levels. For a display
  level, the rule should tighten tolerance locally rather than refuse
  [own; G4 owns the rule].

## 6. Question 3: what stays downstream, and what we tell consumers

Downstream, outside the mesher:

- decimation below the floor and renderer cluster LOD (W1);
- global remeshing for analysis: fTetWild's default envelope is 1/1000 of the
  diagonal, MMG defaults `hausd` 0.01 and a 45° ridge angle (lifecycle S14)
  (W2);
- convex decomposition and physics cooking (W3);
- consumer repair: PrusaSlicer merges and closes gaps, Cura unions overlaps
  by default (S14) (W5).

Float32 narrowing and welding for export are our stage but not the mesher's;
G3 owns their legality (W4).

Draft statement to consumers [own]:

> Every triangle BRepGPU emits lies within the requested ε of the repaired
> B-rep target. Faces meet along shared edges by vertex identity, every
> triangle agrees with its face's orientation, and the mesh is manifold except
> where the source is, which is reported. No in-mesher operation moved,
> merged or removed geometry without checking it against the exact surface.
> Any later operation (decimation, remeshing, export narrowing, slicer repair,
> physics cooking) works from our triangles, not the B-rep. Its deviation from
> the B-rep is at most ε plus its own deviation from our mesh, and only when
> that operation bounds its deviation; quadric decimators estimate it. Face
> identity survives such an operation only if it keeps face boundaries
> locked. A coarser level of detail is a mesh at a larger ε, labelled with
> that ε. In 3MF it is a `lowres` alternative, which a production printer
> must reject when it is the only representation (3MF Production Extension
> 1.2 §4.2.2.1).

## 7. Hypotheses, falsifiers and the measurements the literature does not give

| ID | Hypothesis | Falsifier | Measurement |
|---|---|---|---|
| H1 | Interior refinement (A1, plus A2 where caps touch chains) meets G13's angle cap and G14's needle limits within 10% of the triangle count of A1 + A3/A4/A5 | the flag set with flips or relaxation reaches the targets with >10% fewer triangles or less time on a corpus class | arena: flags A3, A4, A5 off/on, stratified by class |
| H2 | Re-running the generator predicates on post-op triangles keeps the offline two-sided deviation oracle (§10) at pass on every input | any post-op triangle fails the deviation oracle while its generator predicate passed | offline oracle on the arena corpus with each flag on |
| H3 | Dyadic adaptive sampling nests: the sample keys at ε_k are a subset of those at ε_{k−1} on every edge and lattice | one edge or cell whose coarse keys are not a subset of the fine keys | direct key-set comparison across the ladder (an exit-status oracle) |
| H4 | The topology floor is far below the finest count on curved-heavy parts and close to it on prismatic parts | — (descriptive) | triangle counts per level ε·4^k, k = 0..4, per corpus class; the watch's floor |
| H5 | Re-tessellation at ε_1 is faster than decimating the ε mesh to the same triangle count | decimation (Manifold `Simplify` as stand-in, not adopted) is faster at matched count | fleet timing, both on the same inputs |
| H6 | Mixed-level faces stay watertight with no crack pass | an edge-use count ≠ 2 in a mixed-level closed shell | topology oracle over random per-face level assignments |

The literature read does not say:

- whether deviation-checked flips or relaxation against an exact CAD surface
  pay for themselves in a fast mesher (H1);
- what the topology floor is on real corpora (H4);
- how visible band pops are in sharpen-in-place. That is a render question
  for the G13 renderer, judged by reflection lines (Guthe et al. 2006).

## 8. What M5 should write

- **Operations in the mesher:** the generator (lattice, θ-slabs, CDT with its
  Lawson flips under robust predicates and a cap); interior refinement (A1);
  edge-stage chain splits (A2, G2); ID-only operations (A6). Flips, collapses
  and relaxation (A3–A5) are arena flags, default off, legal only under §2.
- **Forbidden in-core:** F1–F8, with reasons as in the table.
- **LOD:** the mesher at ε_k = 4^k·ε, keyed by ε, with nested sampling (round
  closed-form counts to base·2^j behind a flag for H3), chain level = finest
  incident level, and atomic band swaps for sharpen-in-place.
- **Downstream:** W1–W3 and W5, with the §6 consumer statement; W4 is G3's.

## 9. Disagreements among sources

- **What "legal" covers.**
  - Hoppe 1993 §4.2 defines legality as topological type only.
  - CGAL, Gmsh and Netgen add normal or quality checks; MMG adds an estimated
    Hausdorff check.
  - The old owned mesher added certified deviation.
  - Resolution: topological legality is necessary but not sufficient for a
    B-rep-bound mesh, because D and E are part of the contract. We use the
    union of the topology condition and the generator predicates, which is
    the cheapest check that bears directly on the claim.
- **Projection target.**
  - CGAL projects onto the input mesh by default (`remesh.h:194`).
  - Gmsh and Netgen evaluate the CAD surface.
  - Resolution: the CAD surface, through UV, because the contract is against
    the target, not against our own triangles.
- **Crack handling under LOD.**
  - Guthe et al. and Claux–Barthe conceal or fill cracks at render time.
  - Kumar–Manocha and Parasolid share boundary tessellation.
  - Resolution: shared chains. Concealment yields a picture, not a mesh, and
    the same mesh must serve export and physics.

## For SHOPPING_LIST.md

Not fetched; cited only second-hand or as leads:

- Frey P.J., Borouchaki H., "Geometric surface mesh optimization", Computing
  and Visualization in Science 1(3), 1998 (paywalled). This is the closest
  published treatment of deviation-controlled surface optimisation, and the
  basis of MMG's `hausd` checks.
- Chhugani J., Kumar S., "View-dependent adaptive tessellation of spline
  surfaces", ACM I3D 2001 (paywalled; cited by Guthe 2005 for shared-boundary
  crack avoidance).
- Klein R., Liebich G., Straßer W., "Mesh reduction with error control", IEEE
  Visualization 1996 (paywalled). Hausdorff-bounded decimation, needed
  before claiming a bounded decimator exists beyond Manifold's.
- Canann S., Tristano J., Staten M., "An approach to combined Laplacian and
  optimization-based smoothing", IMR 1998 (cited by Owen 1998 §5.1).
- Hoppe H. et al., technical report [6] of Hoppe 1993, with the proof of the
  collapse legality conditions (University of Washington TR 92-01-01); the
  conditions themselves are read from Hoppe 1993 §4.2.

## Acquisition and scrub notes

- One PDF fetched: Hoppe et al. 1993, `papers/pdfs/hoppe-et-al-1993-mesh-optimization.pdf`
  (2.7 MB, open on the author's site). Everything else was already held.
- The 3MF Production Extension 1.2 text was read on the consortium's GitHub
  repository; nothing was stored.
- Nothing was built or run; no file other than this paper and its sources TSV
  was written; nothing was committed. Paths are repository-relative, and no
  hostnames or workstation paths appear.
