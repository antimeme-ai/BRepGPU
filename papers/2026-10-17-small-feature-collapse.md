# Small-feature collapse: what the β-ball and boundary-only rules guarantee

Research lane M4-G10 of the fast-mesher campaign, 2026-10-17. Brief:
[docs/fast-mesher-campaign/briefs/M4-G10.md](../docs/fast-mesher-campaign/briefs/M4-G10.md)
with [M4-common.md](../docs/fast-mesher-campaign/briefs/M4-common.md). Bead
`brepgpu-12s.11`. Stub: lifecycle map §4.1 G10 (stage S6, contradiction X14).
Sources, read depth and hashes:
[2026-10-17-small-feature-collapse-sources.tsv](2026-10-17-small-feature-collapse-sources.tsv).

No mesher, Boolean or NVIDIA run. One pure-arithmetic Python probe checked four
derivations on the fleet host (§9). It is not a timing.

Tags: **[src]** a source states it (section given); **[own]** this lane's
derivation; **[inf]** inferred from a source but not stated by it; **[draft]**
the working-draft design or the healing paper's §7.2 procedure, which the draft
copies.

## 0. Answer on one page

The draft has three rules plus an implied fourth (design §5.2 item 5; healing
paper §7.2 "Small features"):

- **E, edge collapse.** An edge whose every sample lies within its end
  vertices' β balls becomes one vertex.
- **F, boundary-only face.** A face whose every point lies within β of its
  boundary is meshed from its boundary vertices, with no interior points.
- **S, sliver collapse.** A face narrower than η is collapsed symmetrically: its
  two long boundaries become one shared chain.
- **V, vertex merge.** The draft does not say where the merged vertex goes.

Defaults: β = ε/2, η = ε/16.

**None of the rules, as written, keeps every emitted triangle within ε.** Each
has a counterexample, and each can be fixed with a small, cheap change:

| Rule | As written | Counterexample (fleet-checked) | Sound version | Bound |
|---|---|---|---|---|
| E | Per-edge test on samples; triangles at the merged vertex sized as usual | (a) A chain of N edges, each 1.5β long, around a polygon passes the test edge by edge. Collapsing all of them deletes a hole of radius 15β (N = 64) or 122β (N = 512). (b) An arc of radius ρ with only its end samples has its apex outside both β balls once ρ > β/√2. (c) A merged vertex d off a convex face's surface, on a triangle sized to the full ε, gives 1.09ε at d = β/2 and 1.19ε at d = β. | Test each **cluster**, not each edge: the connected set of short edges collapses only if all of its curve geometry (not just its samples) fits in a ball of radius r_c. Triangles that use the merged vertex are sized to ε − d_f. | Triangle t of face f at merged vertex p*: max dist ≤ e(t) + d_f, with d_f = dist(p*, T_f) ≤ r_c. Collapsed geometry lies within r_c of p*. |
| F | The width test (whole face within β of its boundary) by itself licenses boundary-only meshing | A strip on z = a x² + k x y, 3D-narrow (premise holds exactly), edges sampled by the curve rule, gives 1.05ε / 1.22ε / 1.35ε for k = 1 / 5 / 10 mm⁻¹ | Boundary-only is a work-saving default, not a licence. Every boundary-only triangle still passes the face's own density test (§5.3: every emitted edge is sized by the local admissible length). Failures get interior points as usual. A priori sufficient: the whole face fits in a scaled-chart disk of radius R_adm | ≤ ε by the face density rule (Waldron). Width alone gives no bound. |
| S | Symmetric collapse at width ≤ η | No counterexample to fidelity at η = ε/16. The merged chain moves each neighbour by ≤ w/2 ≤ ε/32, but that move is unbudgeted. A sliver whose two long sides border the **same** face pinches it. | Same budget reserve as E (ε − w/2 at the shared chain). The two sides must be distinct faces, or the sliver is absorbed into its one neighbour. Owner or symmetric dispersal by M2's trust order. | max dist ≤ e(t) + w/2 (symmetric) or e(t) + w (one neighbour absorbs it) |
| V | Unstated | A Shephard-style survivor (merge into one given vertex) moves the other faces by up to 2r_c. A merge across shells welds parts. | Put the merged vertex at the smallest-enclosing-ball centre of the cluster (V3), or at the multi-surface point (V2) when it is transverse and its two-sided distance passes. Merge only along topology inside one shell. | d_f ≤ r_c (V3); d_f = 0 with collapsed-geometry distance ≤ \|p_V2 − c\| + r_c (V2); up to 2r_c (one survivor) |

Here e(t) is the triangle's own chord bound, ½·r_min(t)²·M from the density
rules (fast-tessellation §1). T_f is the face's target: its trimmed surface,
displaced near resolved edges as in design §4.2.

**Six decisions follow.**

1. **The collapse radius is charged against ε, unlike β in R2.** Reconciliation
   moves (R2–R4) move the target (operator decision: error is measured against
   the repaired solid). A collapse is a mesh-only simplification inside the
   fidelity contract (lifecycle S6 [own L4]), so its displacement is charged.
   The draft uses one symbol, β, for both. Split them: r_c is the collapse
   radius, with reserve ε − d_f; β stays the R2 gate. That changes the arena's
   "healing thresholds" flag. **Operator question 1** confirms the charging
   rule.
2. **Correctness never depends on collapse.** A feature below ε can always be
   meshed faithfully: more and smaller triangles, all within ε by the ordinary
   density rules. Collapse is an optimisation, for triangle count, needle and
   cap shapes, and near-degenerate CDT input. So a failed validity check reverts
   that cluster and meshes it faithfully. It never refuses a face. That answers
   Owen 2019's lesson: 35% of Cubit's small-feature operations failed (98.6% for
   virtual collapse curve), and every failure needs a fallback.
3. **Dimension and topology checks are local and cheap** (§3):
   - cluster radius;
   - contiguity in every loop (Dey et al. 1999's link condition for 1-complexes);
   - simple UV loops after the merged vertex gets its per-face UV;
   - an Euler characteristic per shell before and after;
   - no merge across shells;
   - a 3D orientation check on triangles at the merged vertex (Garland–Heckbert's
     flip test).

   A face may collapse to a point only if all of it lies within the cluster
   ball. A face may collapse to an edge only through S.
4. **History is one flat record per cluster**, built from union-find
   components. Components do not depend on the order in which edges are
   visited, so the record satisfies D-determinism. Shephard 1998's tree is only
   needed for sequential operators. Face accounting (design §4.5) gains a third
   disposition, collapsed into vertex v or chain c, besides meshed and refused.
5. **Symmetric dispersal halves the worst displacement** (w/2 against w). That
   is the quantitative case for Sheffer's operator. At η = ε/16 the eye cannot
   see either choice. So follow M2's trust order (plane frozen) when the tiers
   differ, and split symmetrically on equal tiers. That is M2's `tangent_split`
   flag reused.
6. **Two draft clauses are unsound** as written (§2.4):
   - "Lines need no interior samples" (design §5.2.1), on ruled or twisted
     incident faces. This is the mechanism behind the F counterexample.
   - "An edge shorter than 4β emits no interior samples" (healing §7.2), if
     "shorter" means chord length. Measured by arc length it holds: sag ≤ 2β = ε.

**Operator question 2 (stopped):** may a sub-ε collapse change topology? The
cases are filling a through-pinhole (a genus change), closing a sub-ε gap into
contact, and absorbing an island. D-features says never drop features above ε
and that defeaturing is out of scope. It does not say whether a sub-ε change
that alters genus is a dropped feature or defeaturing. The rule set below
refuses topology changes by default (the Euler check) and makes this a flag. It
does not settle the question.

## 1. The rules as written, and the quantities used

**Draft text.** The draft (design §5.2 item 5) reads: "edges inside their end
vertices' β balls become one vertex; faces lying within β of their boundary mesh
from boundary vertices alone; virtual, bottom-up, never in the source." The
healing paper §7.2 adds four things:

- the Parasolid short-edge definition applied with β;
- Jackson's "edge shorter than 4β emits no interior samples";
- the JAMA G-FA-NA narrow-face definition with L = 2β;
- symmetric collapse of faces narrower than η (Sheffer 2000 §4.2), bottom-up:
  vertices, then edges, then faces.

The stage runs after edge and vertex resolution (items 1–4), so it reads
**resolved** positions.

**Quantities used in this paper.**

- ε: the caller's chord tolerance.
- T_f: face f's target, its trimmed carrier displaced near resolved edges
  (design §4.2). Distance to T_f is measured to the trimmed face, not to the
  untrimmed carrier. Otherwise a merged vertex placed on an extended carrier,
  outside the trim, would pass while adding material (§2.1, V2).
- e(t): the triangle's own interpolation bound, ½·r_min(t)²·M in the face's
  scaled chart. Waldron Thm 4.1 via fast-tessellation §1, §1.3.
- K: a collapse cluster, meaning a connected set of short edges and their
  vertices. r_K is the radius of the smallest ball that holds all of K's curve
  geometry, with centre c_K. p* is K's merged vertex.
- d_f = dist(p*, T_f) for each face f that uses a vertex of K.

## 2. Q1: a bound or a counterexample per rule

### 2.1 E and V: edge collapse and the merged vertex

**Bound [own].** Let face f use p*, and let t be any emitted triangle of f. Write
t's vertices as x_i = S_f(u_i) + δ_i, where δ_i is the vertex's offset from its
target. Interior and ordinary boundary vertices have δ = 0; p* has |δ| = d_f.
A point of t is x = Σλ_i x_i = Σλ_i S_f(u_i) + Σλ_i δ_i. The first sum lies
within e(t) of S_f(Σλ_i u_i) by Waldron. So

  dist(x, T_f) ≤ e(t) + λ_{p*}(x)·d_f ≤ e(t) + d_f.

**Where d_f comes from.** Every face that uses p* contains at least one of K's
original vertices, and those vertices lie on the face's target. So
d_f ≤ |p* − v| for that vertex v:

- **V3:** p* = c_K, the smallest-enclosing-ball centre (healing §7.2 V3, Segal).
  Then d_f ≤ r_K. This placement minimises the worst displacement over all
  incident faces, because the minimax point of a set is its enclosing-ball
  centre. Averaging is pulled toward clusters of points; the ball centre is not.
- **One survivor:** p* is one of K's vertices, as in Shephard 1998 §5.1
  "collapse edge: given an edge and one of its vertices". Then d_f ≤ diam K ≤ 2r_K.
- **V2:** p* is the point on all incident targets when they are pairwise
  transverse. Then d_f = 0. But the collapsed geometry now lies up to
  |p_V2 − c_K| + r_K from p*: on a chamfered cube corner of side s, the
  restored sharp corner lies s/√6 off the chamfer. So V2 is accepted only if
  that two-sided distance passes the same r_c test.

**Surface-to-mesh direction.** Every point of K's collapsed curves lies within
r_K of c_K. The two-sided oracle in design §10 therefore sees at most r_K of
missing geometry.

**Three counterexamples to the rule as written.**

- **(a) Per-edge test.** The test does not compose. A connected run of N edges,
  each 1.5β long, around a regular polygon passes edge by edge, yet the
  cluster's radius is 1.96β (N = 8), 15.3β (N = 64) and 122β (N = 512) (probe
  C4). Collapsing the run deletes a hole far above ε, which D-features forbids.
  Faceted or stitched imports make exactly these runs (Shephard 1998 §6: stitching
  "introduces a number of very short model edges"). So the criterion must apply
  to each cluster: K collapses only if r_K ≤ r_c, otherwise none of it does.
  All-or-nothing keeps the result independent of edge order (§4).
- **(b) Sample test.** It misses curve geometry between samples. A semicircular
  arc of radius ρ between a and b has sagitta ρ, so when ρ ≤ ε the curve rule
  needs only its two end samples. Its apex is ρ√2 from both ends: outside both β
  balls once ρ > β/√2 (probe C3: ρ = 0.9β puts the apex at 1.27β). The test must
  bound the curve itself: max over samples of |s_i − c_K| plus the curve's chord
  bound at that sampling, or a control-polygon hull.
- **(c) No budget reserve.** With triangles at p* sized to the full ε, e(t) + d_f
  can reach ε + d_f. Probe C2: on a sphere, an equilateral triangle whose worst
  point is exactly ε off; move one vertex inward by d:

  | Sphere radius | d | Worst distance |
  |---|---|---|
  | 1 mm | β/2 | 1.090ε |
  | 1 mm | β | 1.194ε |
  | 0.1 mm | β/2 | 1.086ε |
  | 0.1 mm | β | 1.186ε |

  The first-order prediction ε + d/3 is 1.083ε and 1.167ε. The extra comes from
  the worst point moving toward p*.

  Faces that use both a and b (the two faces of the collapsed edge) have a small
  d_f: the chord-midpoint sag, about |ab|²κ/8. Faces that use only one end
  (for example a face that the short edge leaves transversally) have d_f up to
  |ab|/2.

**Sound rule E′ [own].**

1. Find clusters as connected components of edges whose curve lies within the
   ball test, evaluated on resolved positions.
2. Accept a cluster only if r_K ≤ r_c, measured on the curve geometry.
3. Place p* by V2 if transverse and the two-sided distance passes, otherwise
   by V3.
4. Size the triangles that use p* in face f to e(t) ≤ ε − λ_{p*}·d_f. The simple
   form is ε − d_f.

Only triangles with p* as a vertex carry the reserve. The rest of the face is
unchanged.

**Effect on the last segment of a surviving edge.** A surviving edge that ends
at K absorbs the move in its last segment (healing §7.2). That segment's
deviation from its curve is bounded by its chord error plus d. The same reserve
applies to it.

### 2.2 F: boundary-only meshing of a narrow face

**What the width premise gives [own].** The premise is JAMA G-FA-NA (v5.1
§3.2.17: "from every point on the face, the face's boundary loop lies within
L/2", recommended L = 0.01 mm) with L = 2β. It bounds one direction only:

- **Surface to mesh:** every face point lies within β of a boundary point, and
  boundary points are on the mesh. So that distance is ≤ β plus the boundary
  chord.
- **Mesh to surface:** the premise says nothing about the reverse direction.
  A boundary-only triangle is a flat span between boundary samples, and its
  error depends on its r_min, not on the face's width.

**Counterexample (probe C1).** Take the surface z = a x² + k x y with
a = 0.5 mm⁻¹, ε = 0.005 mm, and the strip between y = 0 and y = w:

- Both long boundaries are parabolas. The curve rule samples them at sagitta ε,
  so the spacing is h = 2√(ε/a) = 0.2 mm and the samples on the two sides line
  up.
- w is set so that the 3D strip is exactly 2β wide: the premise holds and is
  measured to the boundary at β.
- The boundary-only triangles are right triangles (x0, 0), (x0 + h, 0),
  (x0 + h, w).
- Their vertical error, derived in closed form, is (x − x0)(x0 + h − x)(a + k·w/h).
  Its maximum is a·h²/4 + k·w·h/4 = ε + k·w·h/4. The curvature term and the twist
  term have the same sign, so they add.
- The probe found true Euclidean (foot-point) maxima of 1.000ε at k = 0,
  1.050ε at k = 1, 1.224ε at k = 5 and 1.353ε at k = 10 mm⁻¹. Each matches the
  closed form to 4 significant figures.

So the width premise does not keep triangles within ε. The cause is boundary
sampling that ignores the face. The draft's "the maximum of the curve's own rule
and every incident face's admissible length" (design §5.2.1) closes this case if
it is applied:

- the anisotropic Waldron bound (fast-tessellation §1.3) charges the twist term
  D_uv = k;
- for the right triangle above it gives ½·r′² = (h²(2a + k) + w²k)/8;
- that is ≥ (a·h² + k·w·h)/4, because 2wh ≤ h² + w².

But the draft's own "lines need no interior samples" overrides that maximum for
a straight edge on a twisted or ruled face (a bilinear patch, a ruled B-spline,
a hyperbolic paraboloid). That clause is a design defect (§2.4).

**Sound rule F′ [own].** Boundary-only is the first attempt for a narrow face.
Each of its triangles then passes the face's density test, e(t) ≤ ε, evaluated
on the triangle actually emitted: its r_min includes the cross-width offset.
That is the universal §5.3 rule ("every emitted edge, wherever it lies in the
face, is sized by the face's local admissible length"). A failing triangle gets
ordinary interior insertion or a boundary split. With F′, the bound is the face
density bound, ≤ ε, and the width premise only predicts that interior insertion
will rarely be needed.

**A priori sufficient condition (no per-triangle test) [own].** Map the face's
UV domain into the scaled chart of fast-tessellation §1.3. If its smallest
enclosing disk has radius R_face with ½·R_face² ≤ ε, every triangle of any valid
triangulation of the boundary polygon is within ε. A triangle's smallest
enclosing disk is no larger than that of any region containing it, so Waldron
applies directly. This is a **small-face** test (fits in a disk), not a
**narrow-face** test (close to its boundary). It covers tiny caps, bosses and
corner faces. It does not cover long narrow strips.

For strips, the a priori form is: sample both long sides at spacing
h ≤ √(8ε/M − w²) (isotropic M). Then aligned zigzag triangles, whose r_min is
√(h² + w²)/2, are within ε.

**Non-convex small faces** (L shapes, crescents, serpentine strips): F′ needs
no convexity. Its one extra requirement is that the boundary CDT runs in the
carrier metric (design §5.3, direct_cdt). A plain-UV Delaunay on a stretched
parametrisation can connect vertices that are far apart in 3D; the density test
then rejects those triangles and pays for interior points. The 3D orientation
check (§5.3) must also be able to refine. A fold inside a boundary-only
triangulation has no interior point to move, so F′ must allow Steiner insertion,
which it does by construction.

### 2.3 S: sliver collapse

**Bound [own].** A sliver of width w ≤ η between neighbours g1 and g2 maps its
long boundaries C1 and C2 onto one chain C*:

- C* takes the union of both sides' samples.
- Each C* position is the midpoint of the nearest-point pair.
- Each neighbour's boundary moves by at most w/2 (symmetric), or by up to w if
  one neighbour absorbs the sliver.
- The rule-E argument with d = w/2 gives max dist ≤ e(t) + w/2 for the
  neighbours' triangles that use C*.
- Every point of the removed sliver lies within w/2 plus the chord of C*.

At η = ε/16 the reserve is ε/32. That is small, but it is not zero and the draft
does not charge it.

**Order with E [draft, checked].** The sliver's end edges have length ≤ w ≤ η,
so E collapses them first. That leaves the sliver as a two-edge loop between
the merged ends: Shephard's "degenerate face … with just two edges". S then
merges the two edges into C*. A closed sliver such as the band of a shallow
cylindrical step of height h ≤ η has one short seam edge instead. E collapses
the seam, and S maps the two circles to one circle midway. The step becomes a
recorded chain move of h/2 on each plane. M2's recorded tangent steps are not
faces and never enter S.

**Validity [own].**

- g1 ≠ g2. When both long sides border the same face g, the sliver is an
  island or slit in g. Collapsing it to a chain would leave a dangling chain
  inside g's loop, a pinch. The sound action is to absorb the sliver into g:
  drop its loop if g's target passes within the reserve over it. Otherwise
  mesh it faithfully.
- C* must not cross any other trim in either neighbour's UV. This is the same
  UV-loop simplicity check as for E.

### 2.4 Two clauses in the draft that fail as written

- **"Lines need no interior samples"** (design §5.2.1). True for the curve rule.
  False when an incident face twists along the line, as on ruled B-splines,
  bilinear patches and hyperbolic paraboloids: the face's admissible length
  must still cap the spacing. This clause is the mechanism of the F
  counterexample (§2.2).
- **"An edge shorter than 4β emits no interior samples"** (healing §7.2, after
  Jackson 1995 §2.3). Jackson's 4× is a modelling-tolerance statement: a vertex
  inserted mid-edge would coincide with both ends. It is not a chord rule.
  - **Measured by arc length ℓ ≤ 4β:** every curve point lies within ℓ/2 of
    the chord, so the sag is ≤ 2β = ε. The rule holds at the full budget.
  - **Measured by chord:** it fails. A 350° arc whose ends are 4β apart has
    radius ≤ 0.057 mm at ε = 0.005 mm and sag about 2ρ ≫ ε.
  - Either way the curve rule already decides the sample count, so the clause
    adds nothing. Delete it.

These are draft-text defects, not code. No small-feature code exists in the
current tree: the old path refuses collapsed loops
(`native/mesher/face_preparation.cpp:792`) and has no virtual collapse.

## 3. Q2: when a collapse reduces dimension, and the checks

**What the literature establishes.**

- **Shephard 1998 §2 [src].** Step 3 of the procedure: "ensure that deletion of
  the flagged mesh entities results in geometrically valid mesh entities and
  their elimination does not introduce undesired dimensional reductions". The
  check itself is defined in Dey, Shephard and Georges 1997, which is not held
  (§11).
- **Shephard 1998 §5.1 [src].** The reduction operators, each recorded on the
  surviving entity:
  - collapse edge into a vertex;
  - collapse degenerate (two-edge) face into an edge;
  - collapse single-edge face into a vertex;
  - region analogues.
- **Dey, Edelsbrunner, Guha, Nekhayev 1999 [src].**
  - **1-complexes (Theorem A, §4):** contracting edge ab changes the
    topological type iff a and b have a common neighbour, or both have degree
    ≠ 2.
  - **2-complexes (Theorem B, §5):** the Link Conditions
    Lk₀ᵚ a ∩ Lk₀ᵚ b = Lk₀ᵚ ab and Lk₁ᵚ a ∩ Lk₁ᵚ b = ∅ are equivalent to a local
    homeomorphic unfolding. For 2-manifolds they characterise any unfolding.
- **Garland–Heckbert 1997 §6 [src].** Compare each neighbouring face's normal
  before and after a contraction; "if the normal flips, that contraction can be
  either heavily penalized or disallowed."
- **Sheffer 2001 §2.1 [src].** Arcs at non-manifold edges (more than two faces)
  are "uncontractable".
- **Sheffer 2000 [src].** States no dimension or validity check. Its operators
  are applied by a user or by the clustering algorithm.

**Where dimension drops in our setting [own].**

| Case | Drops | Allowed? | Check that decides | Record |
|---|---|---|---|---|
| Edge → point (E) | 1 → 0 | Yes, if r_K ≤ r_c | Cluster radius on curve geometry | Cluster: survivor vertex, source edges and vertices, r_K, d_f per face |
| Whole loop → point | Loop closes up | Inner loop: the hole is ≤ r_c, so the face covers it within the reserve. Outer loop: only if the whole face lies in the cluster ball | Every face using an edge of the loop either keeps a 2D outer loop or lies inside the ball | The face is "collapsed into vertex v". Accounting carries it; it is not a refusal |
| Face → point (tiny corner face, Sheffer Fig. 6) | 2 → 0 | Yes, if the face lies in the cluster ball | As above, plus Euler per shell (for a cut corner χ changes by −2 + 3 − 1 = 0) | As above |
| Face → edge (S) | 2 → 1 | Only through S: w ≤ η, two distinct neighbours | §2.3 | The face is "collapsed into chain c" |
| Face → edge **unintended** (pinhole wall whose rim edges collapse; a short seam on a long band) | 2 → 1 | No. The wall is not within r_c of anything (a mid-depth point is depth/2 from both rims) | Every face touched must stay 2D or vanish entirely | Revert the cluster; mesh faithfully |
| Pinch: two non-adjacent vertices of one loop merge | Loop → figure-8 | No (Dey Thm A: the merged vertices share a neighbour path) | **Contiguity:** K's edges in each loop form one contiguous run | Revert |
| Inner loop merges with outer loop | Hole touches rim | No | Contiguity across loops of one face | Revert |
| Genus change (through-pinhole filled; sub-ε gap closed) | Shell topology | **Operator question 2**; default no | Euler characteristic per shell before and after | Revert (default) |
| Merge across shells or bodies | Welds parts | No (contacts, gears; `PhysicalNodeKey` never aliases by position) | Clusters only follow edges inside one shell | — |
| Fold at p* | Orientation | No | Triangles at p* pass the §5.3 3D orientation check (the Garland–Heckbert flip test) | Revert |
| UV loop self-intersects after p* gets its UV | Domain | No (OCCT ModelHealer's failure class) | p*'s UV is its projection onto each incident face. Each modified loop is still a simple polygon | Revert |

The **Euler check** is the one cheap global invariant: a sub-ε collapse that
keeps topology keeps χ = V − E + F per shell. It catches what the local checks
can miss, for example a ring of clusters that closes a gap.

**Refusal never cascades to the face.** Every "Revert" means the cluster is
meshed faithfully at its own scale. A sub-ε feature is always representable
within ε, because density rules depend on curvature, not feature size. The
exception is input below the floating-point floor, where the CDT predicates
(design §6.3) decide. That is G4's and G5's ground, not a collapse question.

## 4. Q3: dispersal and history in the three sources

**Symmetric dispersal.**

- **Sheffer, Blacker, Clements, Bercovier 2000, §4.2 "Collapse" [src].**
  - Purpose: "a symmetric 'dispersal' of faces between neighboring faces …
    Merging such a face with only one of its neighbor will often lead to an
    asymmetric structure, which is not desirable."
  - Steps:
    1. Split face f's edges that are not shared with the chosen neighbours.
    2. Create parasite edges from f's centre to the shared boundary vertices.
    3. Split f into subfaces.
    4. Merge each subface with its one adjacent neighbour.
  - Figures 6–7 show a cut corner collapsed to a vertex and a chamfer collapsed
    to an edge.
- **Sheffer 2001 §4 [src].**
  - §4.1 restates the operator: split f into f_i, one per neighbour s_i, then
    merge.
  - §4.2 collapses a face only if it "shares boundary edges with all the
    clusters", "is attracted to all the clusters", and its attraction is
    "nearly identical". The face is "symmetric" with respect to them.
- **Shephard 1998 §5.1 [src].** Asymmetric: the caller names the surviving
  vertex or edge.
- **Owen 2019 [src].** Uses Cubit's collapse-curve and virtual-collapse-curve
  operations (Table 1). It says nothing about dispersal.

**The sources disagree; resolution [own].**

- The quantitative reason for symmetry is minimax displacement: w/2 against w
  for a sliver, r_K against 2r_K for a cluster (§2.1, §2.3).
- Sheffer's stated reason is analysis quality ("preserving symmetric properties
  provides more accurate analysis results", Sheffer 2001 §1). That is a CAE
  goal, not ours.
- M2 and the healing paper say lower-trust geometry absorbs the move: plane
  frozen, analytic over freeform.
- At η = ε/16 the difference is invisible.
- So: trust order when the neighbours' tiers differ, symmetric when they are
  equal. This is the same structure as M2's `tangent_split` flag (H6), and one
  rule system then covers both reconciliation and collapse.

**Geometry is kept in Sheffer's operator; not in ours.** Sheffer's collapse is
topology-only: "Both the split and merge operations modify the model
connectivity only, keeping the original geometry" (2001 §4.1). The collapsed
face's surface survives inside the merged virtual faces, and mesh nodes still
lie on host surfaces. Hence Sheffer's accuracy claim: "the mesh deviation from
the original model is bounded by the mesh element size" (2001 §1, §6; 2000 §5.2:
"no accuracy loss … the accuracy of the mesh is dependent only on the element
size"). A triangle spanning a hidden crease between hosts deviates by up to
element size × crease angle.

Our collapse is different. It is applied before meshing and replaces geometry
with a point or a chain, so element size is not the bound; r_c and w/2 are.
That is the substance of X14. The literature's bound does not transfer, and the
§2 bounds replace it.

**History.**

- **Shephard 1998 §5 [src].**
  - The original model topology is kept, plus a copy per idealisation level.
  - Each reduction stores "the operation performed and a list of the entities
    that were deleted" on the survivor.
  - Chained reductions form a tree. A vertex is the minimum survivor: deleting
    it would lose the way back.
  - The mesh is classified against the modified model, not by multiple
    classification (Fig. 2(d)).
- **Sheffer 2000 §3 [src].** Guest/host hierarchy. A virtual entity references
  the hosts it derives geometry from, and the nesting can recurse.
- **Owen 2019 §5 [src].** Each operation names "one or more surviving
  entities", whose mesh nodes are then measured.

**Our record [own].** Clusters are union-find components over a predicate
evaluated on pre-collapse resolved positions, so they do not depend on visit
order or thread count. That is what D-determinism needs; OCCT's order
dependence (healing §2.6) is the failure it avoids. One flat record per cluster
or sliver suffices:

- survivor ID (vertex or chain);
- source vertex, edge and face IDs absorbed, with the kind of absorption (to
  point, to chain, loop closed);
- r_K or w, and d_f per incident face;
- the placement rule used (V2, V3 or owner);
- the thresholds (r_c, η, ε) with their provenance (G11);
- any reverted check, by name.

Shephard's tree arises only from sequential operators; we have none. Face
accounting (design §4.5) gains the disposition "collapsed into survivor X",
distinct from meshed and refused. The cluster depends on ε, so the record and
any cached collapse are keyed by ε (lifecycle S6 [inf L4]; G7). A tighter ε
re-runs the stage: Shephard 1998 §2 issue 4 notes that eliminated features
"may reappear due to the element sizes decreasing". The record is the input
G12 needs to restore them under sharpen-in-place.

**Failure rates (Owen, Shead, Martin 2019, Table 3) [src].** The study covered
94 GrabCAD parts, four target sizes each, with Cubit operations. "Failed" means
the operation failed or the following tet mesh failed. The authors "did not
distinguish between those that failed due to a software defect and those that
failed due to topology".

| Operation | Failed / observed | Rate |
|---|---|---|
| Virtual collapse curve | 14,743 / 14,955 | 98.6% |
| Remove topology surface | 3,102 / 3,890 | 79.7% |
| Tweak remove topology curve | 5,175 / 7,056 | 73.3% |
| Remove surface | 10,026 / 17,624 | 56.9% |
| Blunt tangency | 3,982 / 8,059 | 49.4% |
| Tweak replace surface | 1,152 / 2,569 | 44.8% |
| Collapse curve | 2,113 / 13,830 | 15.3% |
| Composite surfaces | 5,020 / 43,551 | 11.5% |
| Remove cone | 20 / 232 | 8.6% |
| All operations | 45,333 / 128,484 | 35.3% |

Two limits on reading these numbers:

- Their "small" is relative to target mesh size, not to a tolerance.
- Their deviation metric M_d is centroid-to-geometry (§5), which understates
  true deviation by up to 4/3 (fast-tessellation §1.3).

Neither the rates nor M_d transfer as numbers. The lesson that transfers: on
real CAD, collapse fails often, so the fallback path is the main path's
neighbour, not an edge case. Hence decision 2 in §0.

## 5. Q4: authored families

Every family runs at ε ∈ {0.005, 0.025} mm. The metamorphic copies are
whole-model scale 10^±3 with ε scaled, and rigid rotation. Each run is checked
by these oracles:

- two-sided deviation against the trimmed target (design §10);
- per-face accounting, including the collapsed disposition;
- χ per shell;
- 3D orientation;
- the collapse record against the expected cluster.

"Draft fails" families are negative controls. The deviation oracle must flag
them; if it does not, the oracle is broken.

| # | Family | Parameters | Expected under E′/F′/S′ | Draft rule |
|---|---|---|---|---|
| A1 | Box with a planar sliver face between two planes | w ∈ {η/2, η − δ, η + δ, 2β − δ, 2β + δ, 4β} | S at w ≤ η; F′ boundary-only for η < w ≤ 2β; normal above | Same outcomes; checks the thresholds |
| A2 | Curved slivers: circumferential band on a cylinder; latitude zone on a sphere; tangent-fillet tail strip (plane–fillet) | Same w sweep; R ∈ {0.1, 1, 10} mm | Within ε; S reserve visible on the 0.1 mm sphere | Within ε + w/2 (unreserved) |
| A3 | Twisted strip z = a x² + k x y, curve-rule sampling | a = 0.5, k ∈ {0, 1, 5, 10} mm⁻¹, 3D width 2β | F′ inserts interior points or splits; ≤ ε | **Fails:** 1.00 / 1.05 / 1.22 / 1.35 ε (probe C1) |
| A4 | Ruled strip with straight long edges (bilinear patch) | Twist k sweep | ≤ ε once face admissible length caps line spacing | "Lines need no interior samples" under-samples |
| A5 | Non-convex small faces: L shape, crescent tapering to zero width, serpentine narrow strip on a B-spline with stretched parametrisation, dumbbell (narrow neck, wide lobes) | Widths around 2β | F′ density test passes or refines; the dumbbell is not F (lobes > β) | Plain-UV CDT long diagonals |
| A6 | Single short edge, line and arc | ℓ ∈ {β, 1.9β, 2.1β}; arc radius ρ ∈ {0.6β, 0.9β} | Collapse iff curve-ball radius ≤ r_c | **Fails** for the arc at ρ = 0.9β (apex 1.27β, probe C3) |
| A7 | Chain of short edges: polygonal circle hole, N segments of 1.5β | N ∈ {8, 64, 512} | No collapse for N ≥ 8 (cluster radius 1.96β … 122β); faithful mesh | **Fails:** deletes the hole (probe C4) |
| A8 | Cut corner (Sheffer Fig. 6) on planes; on a sphere and a cylinder | s ∈ {β, √3β − δ, √3β + δ, 2β, 4β} | Collapse iff s ≤ √3β (cluster radius s/√3); χ unchanged; V2 accepted only if s/√2 ≤ r_c | Per-edge test passes up to s ≤ 2β |
| A9 | Merged vertex leaving a convex face (short edge into the material from a sphere, cylinder or torus) | d ∈ {β/2, β}; R ∈ {0.1, 1} mm | Reserve ε − d_f keeps ≤ ε | **Fails:** 1.09–1.19 ε (probe C2) |
| A10 | Shallow cylindrical step (short seam) between planes; short cone frustum | Height ∈ {η/2, η, 2β, 4β} | Seam collapses; S at ≤ η; F′ up to 2β | — |
| A11 | Through-pinhole | Diameter ∈ {β, ε}; length ∈ {10ε, 1 mm} | Default: faithful mesh, χ kept (operator question 2) | Rim collapse leaves a 1D wall |
| A12 | Hourglass face (two non-adjacent loop vertices within β) | Gap ∈ {β/2, 2β} | No merge (contiguity) | — |
| A13 | Island sliver (both long sides on one face) | w ≤ η | Absorbed into the host or meshed faithfully; no dangling chain | Pinch |
| A14 | Sliver between plane and freeform; between two freeform faces | w ≤ η | Plane frozen (trust order); freeform pair split symmetrically | Symmetric everywhere |
| A15 | Two touching shells whose vertices lie within β | — | No cross-shell merge | — |
| A16 | Collapse near a thin wall (cluster within 2ε of another face) | — | Reserved triangles keep the §5.6 2ε argument; G4 query sees the reserved ε | Unreserved: separation argument breaks at ε + β |

## 6. The rule set proposed for M5 §5.2 item 5

**Thresholds.**

- **r_c (collapse radius):** default ε/2. Charged against ε through the reserve.
  Arena alternative ε/4: less reserve, fewer collapses.
- **η (sliver width):** default ε/16, as drafted.
- **β:** stays the R2 gate only.
- Operator question 1 decides the charging rule; the arena decides the values.

**Order.**

1. Run on resolved positions (after design §5.2 items 1–4).
2. Form clusters by union-find, then apply E′.
3. Apply S to two-edge loops left by E′.
4. Apply F′ in face meshing.

**Checks, in order:**

1. curve-ball radius;
2. contiguity per loop;
3. same shell;
4. UV loop simplicity;
5. Euler χ per shell;
6. 3D orientation at p*.

Any failure reverts that cluster to a faithful mesh.

**Records.** One record per cluster or sliver (§4). Accounting carries the
collapsed disposition.

**Hypotheses and falsifiers.**

| # | Hypothesis | Measurement | Falsifier | If falsified |
|---|---|---|---|---|
| H1 | E′/S′ with the reserve keep every triangle within ε | Two-sided deviation on A6–A10, A14 | Any triangle > ε | The bound or the reserve is wrong: fix the derivation before tuning |
| H2 | The draft rules exceed ε on A3, A7, A6-arc, A9 | Same oracle | Deviation ≤ ε on any of them | The oracle is broken (negative control) |
| H3 | Collapse buys speed or quality | Arena flag collapse on/off: triangles, wall time, worst cap angle, refusal count, on the corpus and families; threshold fixed before the run (proposal: ≥ 1% triangles or time on any stratified class) | No class gains | Remove E/S from the design; F′ stays as a seeding shortcut |
| H4 | Collapse refusals never refuse a face | Faces refused with a collapse-check reason | Any | A check leaks into face status |
| H5 | Cluster output does not depend on thread count or edge order | Permutation metamorphic on A7, A8 and the corpus | Any record or mesh difference | Union-find is not order-free as built |
| H6 | Trust-order versus symmetric dispersal makes no visible difference at η | M2 H5/H6 eye proxies on A14 | One dominates on every proxy | Pick the dominant rule |
| H7 | Real-CAD collapse refusal is high (Owen) but cheap | Refusal rate per check, per corpus class | — (descriptive) | Informs r_c |

## 7. Interactions with other lanes and decisions

- **M2 (¼ local feature size).** Collapse-scale features must not set lfs.
  Otherwise a sub-ε sliver shrinks lfs, and ¼ lfs then refuses leaps beside it.
  Proposal [own]: compute lfs over entities above the collapse scale, with a
  2r_c floor. Collapse candidates can be found on source positions with a
  margin of r_c plus the edge's measured discrepancy. Falsifier: arena refusal
  counts near A1, A2 and A8 with and without the floor.
- **G4 (thin features).** §5.6's 2ε separation argument holds only if every
  triangle is within ε. Unreserved collapse breaks it (A16).
- **G7 and G12.** Collapse is ε-dependent: key caches by ε, and invalidate on ε
  change (§4).
- **G11.** The collapse record feeds the findings model: thresholds with
  provenance, and the reverted-check reason.
- **G14 (sizing beyond the chord rule).** Proximity caps near a cluster must
  read the post-collapse topology, or the collapsed edges will still force
  small elements: the problem Shephard 1998 §2 step 1 describes.
- **G16 (classification map).** Mesh entities at p* and C* are classified
  against the virtual topology, with the record linking back to source:
  Shephard's modified-model classification, not multiple classification.
- **G13 (shading).** Collapse removes needle and cap sources at tiny edges.
  Whether that matters for normals is G13's measurement.

## 8. What the literature does not say, and what would settle it

- **No source bounds the deviation of a geometry-removing collapse.** The two
  bound-bearing sources (Sheffer 2000, 2001) keep geometry. §2 is owned
  derivation; probe C1–C4 checks it; families A1–A16 test it.
- **No source gives the dimensional-reduction test in operational form** among
  those held. Shephard 1998 defers to Dey–Shephard–Georges 1997 (not held). Dey
  et al. 1999 give the simplicial link condition. Its B-rep form in §3 is owned.
- **No source measures how often sub-ε features occur in real CAD at ε of
  5–25 µm.** Owen's 35% failure rate is at mesh-size scale and mixes software
  defects with topology. Settled by: histogram cluster and sliver counts per
  corpus class at both ε, then H3.
- **No source compares symmetric and owner dispersal on a fidelity or visual
  metric.** Settled by H6.

## 9. Derivation check on the fleet (probe)

- **Probe.** `g10_checks.py`, pure Python 3 standard library, in the lane
  directory `lanes/M4-G10/`. It computes:
  - **C1:** foot-point Euclidean distance by Newton on |S(u,v) − p|² over a
    200-step barycentric grid of the boundary-only triangle, plus a 3D check of
    the width premise against the strip's long boundaries;
  - **C2:** distance to the sphere over a 300-step grid;
  - **C3, C4:** closed-form geometry.
- **Run.** One process, about 2.6 s, while other lanes and services ran (load
  average 7.3). This is not a timing and is reported only as context.
- **Results.** As quoted in §2 and §5. C1 matched the closed form to 4
  significant figures. The premise measured exactly β.

## 10. Sources read

Read depth is per entry in the sources TSV. In brief:

- **Full:**
  - Shephard–Beall–O'Bara 1998;
  - Sheffer 2001 (rendered pages: the PDF text layer is font-encoded);
  - Sheffer et al. 2000 (rendered pages; same reason).
- **Method and results:**
  - Owen–Shead–Martin 2019 (§3–6, Table 3);
  - Dey et al. 1999 §1–5 (rendered pages);
  - Garland–Heckbert 1997 (contraction and inversion sections);
  - JAMA PDQ v5.1 §3.2.16–3.2.17 and the threshold table;
  - Jackson 1995 §2.3;
  - Beall–Walsh–Shephard 2003 §3.
- **Earlier repository papers read for context:**
  - geometry lifecycle map S5, S6, S20, S21, X14 and §4.1;
  - tolerant-geometry-healing §5.6, §5.7 and §7.2;
  - neighbour-reconciliation §2.3, §5.6 and §5.7;
  - fast-tessellation §1.

Newly fetched (openly available): Dey–Edelsbrunner–Guha–Nekhayev 1999, from
the journal's EMIS mirror.

## 11. For SHOPPING_LIST.md

- Dey S., Shephard M.S., Georges M.K., "Elimination of the adverse effects of
  small model features by local modifications of automatically generated
  meshes", Engineering with Computers 13(3):134–152, 1997 (Springer, paywalled).
  It holds the operational dimensional-reduction and validity checks that
  Shephard 1998 defers to.
- Sheffer A., Blacker T.D., Bercovier M., "Clustering: automated detail
  suppression using virtual topology", ASME AMD-Vol. 220, 1997 (paywalled).
  It is the planar-facet predecessor of Sheffer 2001; not needed for the
  bounds.
- Not fetched, openly available, not needed: Hoppe et al. 1993 "Mesh
  optimization" (legal-collapse conditions, superseded formally by Dey et al.
  1999).
