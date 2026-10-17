# Reconciliation from well-conditioned neighbours

Research lane M2 of the fast-mesher campaign (branch `research/fast-mesher-study`).
Brief: [docs/fast-mesher-campaign/briefs/M2-neighbour-reconciliation.md](../docs/fast-mesher-campaign/briefs/M2-neighbour-reconciliation.md).
It answers the operator's question recorded in
[the 2026-10-17 direction journal](../journal/2026-10-17-fast-mesher-direction.md):

> Does the desired final position of inexact or malformed geometry come mostly
> free from easier calculations on neighbouring geometry that is closer, better
> aligned or better conditioned? For grossly malformed starts, when is a small
> reconciliation leap right, when a big one, and what do the second-pass
> branches look like?

It builds on [the tolerant-geometry healing paper](2026-10-17-tolerant-geometry-healing.md)
(edge classes R1–R6, the R4 owner rule, vertex rules V1–V4, staging), the
[whole-mesher geometry contract](2026-10-16-whole-mesher-geometry-contract-study.md)
and the [OM10 tolerant-incidence reading](2026-10-15-om10-tolerant-incidence-reading.md).
It does not redo them. Nothing was built, run or meshed.

Sources, with read depth, are in
[2026-10-17-neighbour-reconciliation-sources.tsv](2026-10-17-neighbour-reconciliation-sources.tsv).
PDFs are in ignored `papers/pdfs/`.

**How the reading was done.** Five sub-lanes read in parallel and reported to
this lane:

- (A) anchored, weighted and conditioning-truncated least squares, and the
  constraint solvers in SolveSpace and FreeCAD planegcs;
- (B) confidence-weighted constraint projection (PBD, XPBD, Flex, Vivace),
  parallel priority order (Δ-stepping), and batched tiny solves;
- (C) CAD and mesh repair: ordering, small versus big leaps, choosing among fixes;
- (D) kernel documentation (Parasolid, ACIS, VDA) and healer source code (OCCT,
  NVIDIA SMLib, FreeCAD, Gmsh, Netgen);
- (E) perception and surface-quality assessment: what an eye checks.

I re-read these load-bearing passages against the extracted text:

- Ju 2002 "truncate those singular values with a magnitude of less than 0.1";
- Lindstrom 2000 "threshold parameter currently set to 10⁻³";
- Zhou 2026 "1D uniform Laplacian smoothing to displacements" and the
  revert-on-failed-validation passages;
- Benkő 2002 "uneven error distribution";
- Langbein 2004 "does not, however, maximize the priority sum".

The derivations in §1.5 and §5 are mine.

OCCT is OCCT 8.0.1 at `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`. Code paths are
relative to each project's root: `occt/src/...`, `smlib/src/...` for NVIDIA
usd-brep 1.0.0 SMLib, `solvespace/src/...`, `planegcs/...`. Parasolid page
numbers are PDF pages.

**Tags**

- **[src]**: a source establishes this, at the cited location.
- **[own]**: my derivation or recommendation. No source read establishes it.
- **(inferred)**: a reader's inference from code or text that it did not state
  directly.

---

## Decision-relevant findings in one page

1. **Yes, mostly, and the boundary of "mostly" can be computed.**
   - Every method read fixes a position in the directions its trusted
     constraints determine, and fills the remaining directions from a prior.
     - Ju 2002 and Lindstrom 2000 truncate small singular values and solve
       toward a mass point or cell centre.
     - SolveSpace takes a minimum-norm Newton step from the previous values.
     - planegcs minimises the prior only in the null space of the hard
       constraints.
     - Zhou 2026 snaps to trusted vertices and spreads the displacement
       harmonically.
     - [src]
   - For an edge sample between two faces at dihedral angle θ, the two
     surfaces determine the position fully when θ is not small (Ju's cutoff
     corresponds to θ ≳ 8.1°, Lindstrom's to θ ≳ 3.6°, Xiao's default to 10°).
     Along the edge the prior always decides. [own; §1.5]
   - **The directions the surfaces cannot determine are, to first order,
     exactly the directions in which the error does not change.** Moving a
     point by δ along a direction with singular value σ changes its distance to
     the incident surfaces by at most about σδ, plus a curvature term Δκ·δ²/2.
     The conditioning that makes a position hard to compute is the same
     conditioning that makes it matter little. [own; §1.5]
2. **The data for the first-order final position is already in hand after
   measurement.**
   - Stage 1 of the healing paper already evaluates, at every edge sample, each
     incident face's foot point, normal and distance.
   - One weighted, truncated 2×2 or 3×3 solve over those values gives the
     reconciled point, with no further surface evaluation.
   - Its residual is about κ(|x|+|d|)²/2. For R2-sized discrepancies
     (|d|, |x| ≤ β = ε/2) and η = ε/16, that is within η whenever the
     curvature radius is ≥ 8ε (0.04 mm at ε = 0.005 mm).
   - Newton is needed only below that radius, or for larger moves.
   - [own; §5.1]
3. **What the systems trust, consistently** [src]:
   - topology, i.e. declared shared identity, first (Zhou §4; Smith–Tautges–Wilson
     2010; Willhalm 1998; the OM10 note "Preserve that identity");
   - lower-dimensional entities and intersections over higher ones (Zhou:
     vertex > curve > surface; Huang 2023; ACIS rigid vertices "shared by more
     than two analytic faces");
   - analytic and planar surfaces over freeform ones (ACIS, CATIA);
   - for representations, **the one that measures closer** (OCCT keeps the
     lower-tolerance variant everywhere it compares; openNURBS; Parasolid help
     geometry "the one with the closest curve to the supplied geometry").
   - The stored 3D curve is the least trusted in Parasolid (deleted), ACIS
     (display only) and Zhou ("can be arbitrarily far"), and the most trusted
     in OCCT. Measurement settles it per edge.
4. **Small versus big leap: published rules are few.**
   - The only explicit numeric rule is Xiao 2021's normal-angle split (10° by
     default): sharp junctions keep the curve under protecting balls; smooth
     ones are rebuilt locally. [src]
   - The kernels say:
     - re-intersect where surfaces cross (ACIS, Parasolid `reset_precision`, VDA);
     - keep one face's representation where they meet tangentially;
     - re-trim curve ends to meet rather than insert a bridge piece (VDA 4955
       §3.1.4.1).
     - [src]
   - Everything else is a ladder: try the cheap fix, measure, accept only if
     no worse, escalate. Examples:
     - OCCT FixLacking, RemoveLoop and FixIntersectingEdges;
     - SMLib `CheckAndSetUVTrimCurve`;
     - Parasolid `close_gaps` and wound healing;
     - ACIS solver cascade;
     - Zhou's optimistic heuristics with topological revert.
     - [src]
   - [own] Because the operator measures error against the final position, the
     size of the leap is not what decides it. **Conditioning decides it.**
     - A transverse edge goes to the surface intersection however far that is
       from the stored curve, subject to topological verification and a
       feature-size bound.
     - A near-tangent edge takes the small leap (owner surface, prior along the
       slide).
5. **Choosing among candidates: structure first, then trust, then measured
   displacement.**
   - Willhalm found structural rules always right where they overrode numeric
     plausibility (§5.1.5).
   - ComplexGen solves topology first and only then refits geometry.
   - OCCT, SMLib and FreeCAD keep the measured-better or check-valid variant
     and roll back otherwise.
   - No system read keeps several geometric hypotheses alive to the output.
     Branches survive only as candidate supersets pruned before output
     (Willhalm, ComplexGen) or as a reversible sequence a user scrubs
     (Borodin 2002).
   - [src]
6. **What an eye checks**, in the order the perception literature supports [src]:
   1. silhouette and contour integrity, including background showing through
      cracks;
   2. unexpected creases or shading jumps on smooth surfaces ("a few degree"
      of normal error reads as a real discontinuity, Guthe 2006);
   3. bumps and roughness on smooth, unmasked regions, worst on planes;
   4. broken regularities (straightness, parallelism, symmetry, right angles);
   5. curvature breaks, visible mainly in reflections;
   6. least visible: smooth sub-pixel displacement with correct normals.

   Hausdorff and RMS distance correlate poorly with what viewers see (Corsini
   2013 Table 3).
7. **Cost.** No priority queue, global solve or iteration is needed.
   - Faces are fixed carriers, never refitted. Resolution therefore runs in
     three data-parallel levels: samples, then vertices, then propagation of
     the vertex correction along each edge. [own; §5.3]
   - Every level is a fixed-size, branch-free solve per item:
     - a 2×2 closed form;
     - a 3×3 eigen with fixed sweeps (McAdams 2011: 81 ns per SSE core,
       input-independent);
     - a 1D blend.
   - Strict priority order is inherently serial (Meyer–Sanders). Parallel
     Gauss–Seidel and Jacobi converge slowly next to anchors (Vivace), so
     neither is needed.
8. **Branch-choice rule** (proposed; one paragraph in §5.5):
   1. discard any candidate that breaks structure;
   2. then prefer no movement of a higher-trust carrier;
   3. then a preserved continuity class;
   4. then the least maximum boundary deviation over all incident faces;
   5. then the least kink against resolved neighbours;
   6. then a deterministic tie-break.

   Two branches stay alive only on a tie within η, as first-class provisional
   state. The mesher never waits for a user.

---

## 1. Propagation from trusted geometry (question 1)

### 1.1 What is trusted, and why

| Trusted entity | Who | Why given | Locator |
|---|---|---|---|
| Declared topology (shared identity) | Zhou 2026; Smith–Tautges–Wilson 2010; Willhalm 1998; OCCT `CheckConnected` (OM10 note) | Proximity alone mis-pairs: on one part, ≥ 749 false pairs remain when all 409 true neighbours are required, even with the best per-part threshold | Willhalm Table 4.1 p. 21; Zhou §4 |
| Vertices over curves over surfaces | Zhou 2026 | B-vertex points are exact; the curve "can be arbitrarily far from the parametric surfaces" | Zhou §4, §4.3 |
| Vertex > Edge > Face skin | Smith–Tautges–Wilson | "the Vertex facet point p is located on the geometric model" | STW §2.3 p. 10, Alg. 1 |
| Vertex with more incident edges | SMLib vertex merge | Valence wins; curves are then re-trimmed to the vertex by projection | `smlib/src/SmBrep.cpp:1305, 1334-1346, 33032-33112` |
| Vertices over representations | OCCT | Representations that miss the vertex are removed; UnifySameDomain rebuilds merged lines between vertex points | healing paper §5.4; `ShapeUpgrade_UnifySameDomain.cxx:2342-2369` |
| Vertex shared by more than two analytic faces is rigid | ACIS healing | "does not possess any freedom of movement" | ACIS R10 Healing p. 33 |
| Lower-curvature or analytic surface | ACIS (cylinders snap to planes); CATIA (freeze canonic) | Visibility of a ridge on a plane | healing paper §1.2–1.3 |
| Lower-dimensional intersection | Huang 2023 | "lower errors" than face–face | healing paper §4 |
| End points over interior | Willhalm | "the end points might be more precisely positioned than the interior"; midpoint terms weighted 1/3 | Willhalm Ch. 4.1 p. 17, Def. 8 p. 28 |
| Neighbours over data | ComplexGen | Fit targets weighted 5 for adjacent elements, 1 for input points | ComplexGen Alg. 1 p. 9 |
| 3D curve when SameParameter | OCCT IntersectionTool | 3D points from the 3D curve when SP, otherwise from S(pcurve) | `ShapeFix_IntersectionTool.cxx:66-83` |
| The measured-closer representation | OCCT FixSameParameter, BRepLib, sewing, FixIntersectingEdges; SMLib UV trim; openNURBS; Parasolid help geometry | The variant with lower measured deviation is kept | `ShapeFix_Edge.cxx:896-910`; `BRepLib.cxx:1651`; `smlib/src/SmEdgeuse.cpp:4857-4864`; Parasolid FD §72.5 p. 1297 |
| First in traversal order | OCCT (several places), UnifySameDomain reference face, IntersectionTool `UnionVertexes` | No reason given; it is arbitrary | healing paper §2.6; `ShapeFix_IntersectionTool.cxx:529-534` |

[src] throughout.

Synthesis [own]:

- Trust in **surfaces** is declared by type: plane, then other analytic, then
  freeform.
- Trust in **representations** (3D curve, pcurve lifts, stored vertex points)
  is measured. Each one is ranked by its own residual to the surfaces it
  should lie on.
- The OM10 cylinder evidence shows why a fixed representation order fails on
  real data: a foreign pcurve sat 2.47 µm off while the 3D curve sat 0.14 µm off.

### 1.2 How positions are derived from neighbours

| Mechanism | Systems | Locator |
|---|---|---|
| Surface–surface re-intersection | ACIS sharp-edge solver; Parasolid `PK_EDGE_reset_precision_2`, `PK_EDGE_repair`; VDA G-FA-EG "als Schnittkurven oder Projektionen" | healing paper §1; VDA §3.1.5.1 p. 43 |
| Curve–curve intersection, else projection, for vertices | ACIS ("Vertex geometries are also recomputed using curve–curve intersectors or projectors"); VDA G-FA-VG "Projektion … sonst Neuerzeugung" | ACIS Healing p. 17; VDA §3.1.5.2 p. 43 |
| Curve–curve extrema, else projection of the vertex onto both curves | OCCT FixGap3d/2d by-ranges pass | `ShapeFix_Wire_1.cxx:543-672, 1272-1571` |
| Midpoint of two evaluations | OCCT FixIntersectingEdges; IntersectionTool new vertices; Kahlesz sewing; OCCT sewing centroid | `ShapeAnalysis_Wire.cxx:1510`; `ShapeFix_IntersectionTool.cxx:1219-1221`; Kahlesz Fig. 3c |
| Harmonic spreading of a trusted-boundary displacement | Zhou: 1D uniform Laplacian along curves, Δu = 0 inside faces | Zhou §4.3 p. 5, §4.5 p. 7 |
| QEF minimiser of neighbouring planes | Bischoff–Kobbelt 2005 (feature vertices); Ju 2002; Lindstrom 2000 | healing paper §4; Ju §2.3; Lindstrom §3.1.1 |
| Alternating refit to neighbours | ComplexGen: patches, then curves to patches and corners, then corners to curves and patches | ComplexGen Alg. 1 |
| Implicit bridge interpolating trusted surroundings | Xiao 2021 blending balls | Xiao §5.2, Fig. 7 |
| Smart estimate when intersections fail; make tolerant and retry | ACIS kernel | ACIS Kernel ch. 6 pp. 4, 7 |

[src] throughout.

Two facts change what the healing paper implied about OCCT and SMLib [src]:

- **OCCT's neighbour-intersection gap closure is unreachable by default.**
  - The by-ranges pass of FixGap3d/2d intersects or projects the neighbouring
    curves. `myFixGapsByRanges` defaults to false
    (`ShapeFix_Wire.cxx:173`), and neither ShapeProcess nor STEP import
    exposes it (`ShapeProcess_OperLibrary.cxx:634-664`).
  - Only the pole-move pass runs. Vertex points are never moved; their
    tolerance is reset to Confusion and regrown later
    (`ShapeFix_Wire_1.cxx:773-790`).
- **SMLib closes no gaps geometrically.**
  - `Fix_BadGaps` reads "TODO: Not yet implemented!" and has an empty body,
    although it is called as step 15 (`smlib/src/SmHealData.cpp:4036-4083, 2123`).
  - Its healing sizes tolerances, which may shrink (`:2877`), merges vertices
    with the higher valence winning, and re-trims curves to them.

### 1.3 Anchored, weighted and truncated solves: what fills the undetermined directions

- **Ju et al. 2002** [src]
  - Energy and failure: E[x] = Σ(nᵢ·(x−pᵢ))² with unit normals. With "noisy
    normals that are nearly coplanar … the minimizing x̂ may lie far outside the
    defining cube."
  - Remedy: "Based on experimentation, we typically truncate those singular
    values with a magnitude of less than 0.1. Using the resulting
    pseudo-inverse, we then approximately solve Âx̂ = b̂ while minimizing the
    distance of x̂ to the centroid" (§2.3).
  - The threshold is absolute on σ(A), so it scales with √(sample count).
    (inferred)
- **Lindstrom 2000** [src]
  - Solve: x = x̂ + VΣ⁺Uᵀ(b − Ax̂), with σᵢ⁺ = 0 when σᵢ/σ₁ ≤ 10⁻³. The result
    is "the orthogonal projection of the cell center onto the space of all
    solutions" (§3.1.1, eq. 3–4).
  - Here A is Σ nnᵀ, so the threshold applies to its eigenvalues.
- **SolveSpace** [src]
  - Earlier groups' parameters become constants (`solvespace/src/expr.cpp:322-327`).
  - Each step is "the minimum norm solution of the underdetermined system"
    (`system.cpp:299-334`), so free directions keep their previous values.
    (inferred)
  - Dragged parameters have their Jacobian columns scaled by 1/20
    (`system.cpp:278-285`). That weights them 400× in the squared step norm
    [own], so they stay at the mouse.
- **planegcs** [src]
  - Constraints tagged "lower priority" are minimised by SQP subject to the hard
    constraints. `qp_eq` returns the row space Y and null space Z of the hard
    Jacobian, and x = −Yc + Zy (`planegcs/GCS.cpp:4680`; `qp_eq.cpp`).
  - The prior is honoured only where the hard constraints leave freedom.
  - A rank-deficient hard set is refused, not truncated (`qp_eq.cpp`).
  - The last resort anchors every parameter to its initial value
    (FreeCAD `Sketch.cpp` internalSolve case 3).
- **Benkő et al. 2002** [src]
  - Constraints are eliminated in priority order through d̃ = Md*. A row that is
    all zero "within prescribed tolerance" is dependent; one whose constant
    term is nonzero contradicts, and the constraint is backtracked out (§3.3).
- **Werghi et al. 1999** [src]
  - Penalty weights grow until the Hessian becomes "ill-conditioned" (eq. 38).
  - (inferred) Hard trust belongs in elimination, null-space projection or
    truncation, not in large penalty weights.
- **XPBD** [src]
  - Eliminating λ from eqs. 8–9 gives the stationary point of
    ½(x−x̃)ᵀM(x−x̃) + ½Cᵀα̃⁻¹C [own derivation from the source's equations].
  - M is the prior precision and α̃⁻¹ the constraint precision. Inverse mass
    w = 0 is a fixed anchor (PBD §3.6).

The common shape [src for each component, own for the unification]:

- trusted constraints fix the row space;
- a prior fills the null space;
- "ill-determined" is a threshold on singular values, or exact zero within a
  rank tolerance.

### 1.4 Sequential versus simultaneous

- **Benkő** [src]: sequential construction "leads to an uneven error
  distribution (objects fitted later have greater error). In extreme cases, it
  may even lead to a failure to find a solution" (§1.3).
- **Werghi** [src] chose batch because incremental insertion is "around N times"
  slower.
- **Langbein 2004** [src] accepts constraints greedily by priority, then solves
  once. It "does not, however, maximize the priority sum."
- **SolveSpace** [src] is sequential across groups and staged within a group:
  substitution first, then single-unknown equations, then "the leftovers as one
  big system" (`system.cpp:544-586`).
- [own] Benkő's warning applies when the trusted entities are themselves
  estimates refitted from data. In BRepGPU **the face carriers are given and
  never modified**, so anchoring edges and vertices to them is a legitimate
  first stage, not a source of uneven error. The only joint coupling left is
  edge to vertex, at shared end points. §5.3 resolves it in a fixed acyclic
  order.

### 1.5 The answer, quantified [own]

**Setting.**
- Take a point with prior p₀: the stored 3D curve point, the owner pcurve
  lift, or the stored vertex.
- It has k incident surfaces Sᵢ, with foot points fᵢ, unit normals nᵢ and
  signed distances dᵢ, all measured in stage 1.
- Linearised, the reconciled point is p₀ + x with nᵢ·x = −dᵢ.
- Weighted, minimise Σ wᵢ(nᵢ·x + dᵢ)². With A = W^½N and b = −W^½d, solve
  x = VΣ⁺Uᵀb, truncated. That is Lindstrom's eq. 4 with x̂ = p₀.

**Two surfaces, unit weights.**
- The Gram matrix of n₁, n₂ has eigenvalues 1 ± cos θ, on (1,1)/√2 and (1,−1)/√2.
- Splitting the minimum-norm move into those two components:

  |x|² = (d₁+d₂)²/(2(1+cos θ)) + (d₁−d₂)²/(2(1−cos θ)).

  This equals the healing paper's |x| = √(d₁²+d₂²−2d₁d₂cos θ)/sin θ.
- The ill-conditioned component is the **surface gap** (d₁−d₂), divided by
  2 sin(θ/2).
- The singular values of N are √(1 ± cos θ), with ratio tan(θ/2). They give:

  | Cutoff | Keeps the second direction when |
  |---|---|
  | Ju, absolute 0.1 | 1 − cos θ > 0.01, i.e. θ > 8.1° |
  | Lindstrom, relative 10⁻³ on eigenvalues | tan²(θ/2) > 10⁻³, i.e. θ > 3.6° |
  | Xiao | normal angle 10° (its sharp/smooth split) |

  The third singular value, along the edge, is zero. Along the edge the prior
  always decides.

**Truncated, the point moves only along the bisector normal**, by
(w₁d₁ + w₂d₂)/(w₁ + w₂).
- Equal weights give the average; w_owner → ∞ gives the owner surface.
- The healing paper's R4 owner rule is therefore the infinite-weight limit of
  the truncated solve.
- The rejection of averaging in the literature (Steinbrenner §2.3) was for cost
  and curve representation, not for visual quality.

**Sensitivity.**
- Moving the point by δ along right singular vector v_j changes the weighted
  residual by σ_j·δ. For two surfaces, the distance to each changes by
  |nᵢ·v₂|δ = sin(θ/2)·δ.
- Beyond first order, two tangent surfaces whose normal curvatures across the
  edge differ by Δκ separate as Δκ·δ²/2.
- Example, a fillet of radius 2 mm tangent to a plane (Δκ = 0.5/mm):
  - at θ = 1°, a 0.1 mm slide changes the distance by 0.87 µm (first order)
    plus 2.5 µm (second order);
  - against ε = 5 µm, the second-order term bounds the free slide at about
    √(2η/Δκ) = 0.035 mm when η = ε/16.

**So, quantitatively:**
- Directions with σ above the cutoff are determined by the neighbouring
  surfaces, from data already measured.
- Directions below it are undetermined, and the incident-face error is
  insensitive to them out to √(2η/Δκ).
- The genuine decisions that remain are:
  - the tangent **gap** (d₁−d₂), which no position removes and which the trust
    rule assigns;
  - slides larger than the second-order limit, which are gross cases.

This is the sense in which the final position comes "mostly free". The
literature supplies every component, but none measures the fraction of real
edges in each regime. Hypothesis H1 (§5.7) measures it.

---

## 2. Small versus big leaps (question 2)

### 2.1 Published decision rules

| Rule | Source | Numbers |
|---|---|---|
| Sharp → keep the curve (protecting balls); smooth → local implicit rebuild (blending balls) | Xiao 2021 §3.2, §4.3, §5 | θ_f = 10° default; protecting radius ≥ 2ε, blending radius max(r_min, 2ε), where ε is the measured trim error |
| Fails when defects exceed surface size ("the entire surface feature may be swallowed") | Xiao §6.5 | — |
| Transverse: extend and re-intersect; tangent: keep one face's iso edge or tolerant edge | ACIS Healing pp. 13–16; ACIS Kernel p. 6 ("Non-tangent tolerant edges … reverted … using the precise geometry from the intersection"); Jackson §2.3 | — |
| Re-intersection "may not always be successful if the adjacent surfaces are tangent or nearly tangent"; options: any curve within precision / prefer / require true intersection | Parasolid FD §16.3.1.4 | — |
| Re-trim ends to meet rather than insert a small bridge curve ("Gefahr einer ungewollten Schlinge" — risk of an unwanted loop) | VDA 4955 §3.1.4.1 p. 40 | — |
| Face gap: regenerate both faces on a common boundary curve, keeping G1/G2 | VDA 4955 §3.1.6.1 p. 48 | — |
| Automatic gap closers create tiny curves and narrow faces | VDA 4955 §3.1.1.10, §3.1.2.10 | — |
| Local perturbation "can be used to improve the accuracy of small gaps, but yields bad-shaped surfaces if the gaps are too large"; global remodeling is the alternative | Marussig–Hughes §3.4.1, §5.4 | No threshold |
| Local repair: "Low invasiveness, but only few guarantees"; global: "High robustness, but often loss of detail"; zip or stitch chosen by gap width | Campen–Attene–Kobbelt tutorial | No threshold given there; PMR05 not obtained |
| Edge discontinuity smoothed below 0.01 rad, else split; surface smoothing below 0.1 rad with deviation ≤ 0.01 | ACIS Healing p. 9 | as stated |
| "No analytic surface is transformed by more than the corresponding tolerance value" | ACIS Healing p. 20 | — |
| Local operations make an edge tolerant only below 100 × SPAresabs | ACIS Kernel p. 6 | — |
| Merge radius from a proven tessellation error bound | Kahlesz 2002 §7 | d_s = 2κ |
| Watertight sealing fails once facet tolerance exceeds feature size | Smith–Tautges–Wilson Table 2 | 1019 failed faces at 10⁻¹ cm on one model |
| Wound healing: cap (needs one covering surface) / shrink / grow-from-parent / rubber face; `heal_loops_auto` recommended because "only one method will yield a valid body" | Parasolid FD §63.2.1 pp. 1064–1070 | — |
| `PK_EDGE_delete` heals only if adjacent 3D curves "can meet when extended and lie in the surface", else `cant_heal_wound` | Parasolid FD §63.4 p. 1081 | — |

[src] throughout.

### 2.2 Ladders: try the cheap fix, verify, escalate

- **OCCT FixGap3d/2d acceptance gate** [src]. A new joint is accepted only if
  all of these hold:
  - its distance is ≤ the current gap;
  - it is not at the far ends;
  - it stays near the old range (`ShapeFix_Wire_1.cxx:582-585, 678-681, 1420-1423`).
  - This is a "no worse, stay local" rule.
- **OCCT IntersectionTool** [src]. Three rungs:
  1. cut back by a range change and grow the vertex, when the crossing is
     within MaxTolVert = min(max face vertex tolerance, 1.0);
  2. else split at a new midpoint vertex;
  3. for overlaps, a 3-way split that removes both overlap segments.
  - Locator: `ShapeFix_IntersectionTool.cxx:1046-1163, 1335-1822`.
  - It is capped at 30 splits per wire (`:1065`).
- **OCCT FixIntersectingEdges** [src]. "Always try to modify the tolerance
  firstly as a better solution" (`ShapeFix_Wire.cxx:3054`). The tolerance option
  wins when its measured value is below the move option's; otherwise it cuts
  and moves the vertex.
- **OCCT FixLacking** [src]. A speculative bend of both pcurves to the 2D
  midpoint, each measured with `TryNewPCurve`. If only one bend succeeds, that
  pcurve is bent all the way to the other. Self-intersection checks then re-run,
  because the "self-intersection [is] not detected until curve is bent"
  (`ShapeFix_Wire.cxx:3669-3709, 3948-3951`). After that the ladder in the
  healing paper §2.2 continues, with the extra conditions sub-lane D found
  (§6 below).
- **SMLib `CheckAndSetUVTrimCurve`** [src].
  1. Measure at 8 samples, then precisely.
  2. Accept below 100 × B-rep tolerance or edgeLength/10.
  3. Otherwise re-project, keeping the result only if it is ≥ 2× better.
  4. Otherwise restore the old trim (`smlib/src/SmEdgeuse.cpp:4732-4891`).
- **Parasolid `close_gaps`** [src]. `minimise` → `close_cu` (extend or trim
  SP-curves) → `close_all` (then reparameterise the surface). It "will minimise
  gaps, but is not guaranteed to close them" (FD §16.3.3 pp. 241–244).
- **Parasolid `PK_FACE_change` and offsets** [src]. `replace_use_attempt_c`:
  "try to use the entity … but create a new curve if this is not possible"
  (§72.5). Offset self-intersections run trim → repair → extend (§58.3).
- **ACIS solver cascade** [src]. Analytic → isospline → sharp-edge → generic
  spline → wrap-up.
  - A solver fails "if the surface intersectors return intersection curves such
    that each of them deviates from the original curve by more than the
    tolerance" (p. 33).
  - Unhealed junctions fall through to the next solver; the final fallback is
    `api_hh_make_tolerant`.
  - Rollback is application-driven: "the model can be rolled back to the
    previous state and that phase of healing can be tried again using different
    methods or settings" (p. 8).
- **Zhou 2026** [src]. Optimistic heuristics "are validated solely on
  topological criteria"; "if either heuristic fails its validation checks, we
  automatically revert to the topology-preserving correct algorithm" (§4.7).
  A trace longer than 100× its b-edge is stopped and resampled at a smaller
  tolerance.
- **Smith–Tautges–Wilson Alg. 2** [src]. Coincide → merge (≤ ε_f) → split the
  skin edge → split the curve edge. Before that, features below facet tolerance
  are collapsed.
- **ComplexGen** [src]. Validness threshold 0.3, relaxed to 0.1 for the single
  failure in 3k. Spline fits come before typed primitives because cone and
  cylinder fitting is "highly sensitive to the normal vectors".

### 2.3 The answer [own]

The operator's decision that error is measured against the desired final
position changes the question.

- **Transverse junctions** (σ₂ above cutoff): the surfaces define the final
  position. A "small leap" that stops short leaves residual on both faces and
  buys nothing. The stored curve's distance from the intersection is a
  starting-condition metric, reported, never charged against ε.
  - So the transverse leap is always to the intersection, however large.
  - It is bounded only by:
    - convergence (Newton from the linear step, within a geometry-derived
      iteration bound);
    - topological verification (Zhou-style: chain monotone, UV loops simple,
      ends meet the resolved vertices, no crossing of other trims);
    - feature size (Xiao §6.5, Smith–Tautges–Wilson): the move must stay below
      ¼ of the local feature size.
  - This is what ACIS, Parasolid and VDA do.
- **Near-tangent junctions** (σ₂ below cutoff and the gap move above β): the
  big leap is ill-posed (Jackson, Parasolid). Only the small leap exists:
  - truncated solve, weighted by trust;
  - slide filled from the best-measured prior.
  - The residual surface gap lands on the lower-trust face's boundary
    triangles and is recorded.
- **Bridge geometry is never invented.** No bridge curve or sliver face is
  inserted (VDA §3.1.4.1; sewing is out of mesher scope per the healing paper).
- **Gross tangent gaps** (gap above ¼ of the local feature size) are refused.
  Gaps below that mesh faithfully as a recorded step on the non-owner side.
  This follows the design draft's §4.9 call, "invalid inputs people still use:
  faithful plus reported".

---

## 3. Second-pass branch choice (question 3)

### 3.1 Scoring terms systems actually use

| Term | System | Locator |
|---|---|---|
| Lower measured deviation (max distance) wins | OCCT FixSameParameter, BRepLib SameParameter, sewing master swap, FixIntersectingEdges; SMLib UV trim (≥ 2× better); openNURBS | `ShapeFix_Edge.cxx:896-910`; `BRepLib.cxx:1651`; `smlib/src/SmEdgeuse.cpp:4857-4864` |
| Check-valid on a copy, else keep the other | FreeCAD `TopoShape::fix` ("may actually make a valid input shape invalid") | `TopoShape.cpp:3394-3445` |
| Closest outcome to supplied help geometry | Parasolid `PK_FACE_change` | FD §72.5 p. 1297 |
| End-point closure + midpoint agreement, scaled by length; orientation by minimal end-point distance; conservative gates (chord angle < 29°, plane angle < 8.1°, …) | Willhalm Def. 8, Stage 4 | pp. 28–30 |
| Orientation-aware mean distance | Smith–Tautges–Wilson | pp. 8–9 |
| max w·F_topo + (1−w)·F_geom, w = 0.5; S = exp(−d²/ε²) | ComplexGen §5.1 | p. 9 |
| Priority w(r) = w_c(c_e·w_e + c_q·w_q + c_b·w_b), greedy, generic-DOF solvability | Langbein 2004 eq. 1–2, Alg. 1 | pp. 10, 22 |
| Culprit = the constraint in most conflict groups, then more equations, then latest | planegcs | `GCS.cpp:5573-5605` |
| Keep the longest common extent at folds; midpoint placement | Kahlesz | pp. 6–7 |
| Movement checked so no change "distort[s] the model" beyond tolerance | ACIS Healing | p. 13 |

[src] throughout.

### 3.2 Structure before score

- **Willhalm §5.1.5** [src]. Where the structural rule (transitivity; no two
  segments of one element are neighbours) dropped the more plausible pair, "in
  all of these cases, the first rule was right … the second rule relies on mere
  numerical computations." The result was 14596 reference pairs, 213 false,
  0 missed.
- **ComplexGen** [src] solves topology first and refits geometry after.
- **Zhou** [src] validates on topology only.
- **Langbein** [src] warns that outcomes depend on the weights. On a perturbed
  cube, favouring tolerance chose "integer angles different from 90°"; and "No
  clear decision can be made if neither the tolerances nor the quality …
  differ distinctively" (§7.1).

### 3.3 Keeping branches alive, deferring to a user

- **No system read keeps several geometric hypotheses through to output.** [src]
  - Willhalm overestimates on purpose and prunes.
  - ComplexGen solves one ILP over a candidate superset.
  - Borodin keeps the whole contraction sequence reversible, and the user
    chooses where to stop because some gaps are intentional.
- **Users** [src]:
  - CADfix vets each fix (Butlin–Stops, via the healing paper).
  - Mezentsev–Woehler hands leftovers to user-assisted repair.
  - Barequet–Kumar lets the user "mark the corrections that conflict with the
    original design intent" (Kumar project page, abstract only).
  - Smith–Tautges–Wilson prompt the user in ambiguous situations.
  - Willhalm's checker colours each edge point by degree, because checking
    every neighbourhood "would be as time consuming as the generation of the
    output by hand".
- **Disagreement on the value of healing** [src]: González-Lluch: "There is no
  general agreement on whether model healing is beneficial … repairing a local
  problem in one area of the model may create new local problems elsewhere."

### 3.4 What an eye checks

The healing paper's §5.7 principles now have perception evidence behind them
[src]:

- **Position error is a poor proxy.**
  - Hausdorff correlates with viewer scores at 1–26% and RMS at 8–49%, against
    66–94% for curvature, roughness and dihedral metrics (Corsini 2013 Table 3).
  - Sorkine weighted Laplacian coordinates "guessing that the visual perception
    is more sensitive to smoothness/roughness variations than to pure geometric
    displacements" (Lavoué 2011 p. 1428).
- **Unmasked smooth regions show defects most.**
  - Noise "is indeed far less visible on rough regions" (Corsini p. 113); it is
    "much more visible when applying on smooth parts" (Lavoué p. 1432).
  - "a convex surface with flat points looks to have dents" (Hagen 1993 §5.2).
  - (inferred) A bump on a plane is the worst case. This is independent support
    for freezing planes and analytic surfaces.
- **Normals.**
  - "Even a slight normal deviation of a few degree can lead to visual artifacts
    that are indistinguishable from real surface discontinuities" (Guthe 2006 §6.3).
  - "Human vision is much more sensitive to such unusual edges and
    discontinuities" (Corsini §6.3).
- **Silhouettes.** Details there are "more perceptible than interior details
  because of higher contrast" (Luebke–Hallen §2.4). (inferred) A crack that
  shows the background has silhouette contrast.
- **Continuity in reflections.**
  - "A discontinuity on a surface is magnified by an order of one in the
    highlight line" (Beier–Chen Property 2).
  - Isophotes of a C^r surface are C^(r−1) (Hahmann 2008 p. 11).
  - Reflected-image distortion tracks curvature (Fleming 2004).
  - (inferred) A glossy display exposes curvature breaks and lost planarity
    that matte shading hides.
- **Regularities.**
  - Straightness, collinearity, symmetry, parallelism and cotermination are
    detected fast (Biederman 1987 p. 115, p. 119 search-task evidence).
  - Observers read parallelism, perpendicularity, planarity and symmetry into
    engineered drawings; images that break them perplex (Lipson–Shpitalni p. 6).
- **Sub-pixel position error with correct normals is invisible** (Yeo 2012 §1;
  Claux 2014 §1, B-rep gaps visible "only … at extremely high zoom levels").
  The exception is animation (Corsini §4).
- **What the literature does not give:**
  - a measured angle at which a crease becomes visible;
  - a pixel threshold for cracks;
  - numeric class-A thresholds. Class A has "no absolute rules for tolerances
    or continuity levels" (Autodesk Alias help).
  - The JAMA and CAx-IF numbers in the healing paper (smooth ≤ 0.01°, crease
    ≥ 0.2°, sharp 1°) remain the only thresholds.

For BRepGPU this ranks the coherence terms [own]:

1. closure (no cracks);
2. continuity class (no new crease on a G1 edge, no softened crease);
3. no movement of planes and analytic surfaces;
4. regularities preserved, which follows from (3), because analytic carriers
   carry the regularities;
5. least maximum boundary deviation;
6. least displacement of the position itself.

Position error comes last, which is consistent with the design's exact
per-corner normals.

---

## 4. Cost and batching (question 4)

| Mechanism | Cost per item | Batching | Evidence |
|---|---|---|---|
| Measurement (feet, normals, d) | Already paid in stage 1 of the healing paper; it is the tessellation sampling | SoA by surface kind (design §6.2) | healing paper §7.2 |
| Linear 2-surface step | Closed form from (d₁, d₂, cos θ): about 20 flops, one division, no surface evaluation [own] | Branch-free SoA; lanes blend truncated and untruncated | §1.5 |
| 3×3 weighted solve (vertex, k ≥ 3) | Symmetric eigen with fixed sweeps: 81.4 ns per SSE core, 4.78 ns per decomposition at 12c24t, input-independent (McAdams Fig. 4); "10−15 iterations" to float roundoff, 4 sweeps benchmarked | Fixed control flow, SoA; bin by valence | McAdams §5 |
| GPU 3×3 SVD | 0.37 ns on a Titan Xp, authors' figure, unverified; no P100 figure found | One system per lane in registers; batched BLAS targets sizes ≤ 512, not 3×3 (MAGMA) | Gao README; Haidar p. 4 |
| Two- or three-surface Newton | Dominated by surface evaluation per iteration; bounded iterations | Batched by surface kind; the existing R3 queue | design §7 |
| Propagation blend of a vertex correction along an edge | One pass over the edge's samples | Per edge, SoA over samples | Zhou §4.3 (1D Laplacian = linear blend in 1D) |
| Strict priority-queue propagation | Inherently sequential | Δ-stepping buckets trade phases (dc/Δ) against re-relaxations | Meyer–Sanders pp. 2–5 |
| Jacobi / Gauss–Seidel projection (PBD/XPBD) | ≈ 1.4 ns per constraint-iteration on a GTX1070 (derived from XPBD Table 1) | Jacobi needs averaging; rank-deficient duplicates oscillate (Flex Fig. 2); colouring adds irregular rounds; convergence is slow next to anchors (Vivace §limitations) | XPBD; Flex; Vivace |
| Global sparse least squares | One sparse QR per Newton step; diagnosis costs extra factorizations | Irregular | SolveSpace, planegcs |
| Harmonic face extension | One sparse solve per face | Irregular | Zhou §4.5 |
| Branch verification | One UV-loop simplicity check per incident face per branch | Per face; cost scales with branch count | OCCT ModelHealer check (healing paper §7.4) |

[own] What this implies:

- The background healer needs no global solve, no iteration and no priority
  queue. Each level (§5.3) is one fixed-size, branch-free solve per item, in
  SoA batches shared by CPU SIMD and GPU warps.
- The fleet CPUs are AVX without FMA. McAdams's figures are on a pre-AVX SSE
  CPU, so they carry over; an 8-wide float AVX gain is unmeasured.
- The only irregular work is:
  - Newton, which is batched by surface kind;
  - branch verification, which runs per face and should stay rare (H5 measures
    how rare).
- The harmonic face extension and the global solve belong in the offline model
  oracle (H3), not in the product.

---

## 5. Fit: a concrete procedure (question 5)

Everything in this section is **[own]** unless it cites a source. It is a
design input for M5 and the tribunal, not a selected design.

### 5.1 One point solve that contains R1–R5 and V1–V3

For every edge sample and every vertex:

- **Prior p₀.** Among the available representations, take the one with the
  smallest measured maximum residual to the incident surfaces. Candidates:
  - for a sample: the stored 3D curve point, or the owner's pcurve lift;
  - for a vertex: the stored point, or the centre of the smallest ball
    enclosing the incident edge ends.
  - This follows OCCT, SMLib, openNURBS and Parasolid help geometry (§1.1),
    and the OM10 evidence.
- **Constraints.** One row per incident surface: nᵢ·x = −dᵢ, from stage-1
  measurements. Weights:
  - plane w = ∞ (eliminated as hard);
  - other analytic surfaces a high finite weight;
  - freeform surfaces 1;
  - ties within a tier broken by the healing paper's R4 owner rules 2–4.
  - Hard trust is applied by elimination or null-space projection, as planegcs
    does, not by penalty (Werghi's ill-conditioning).
- **Truncation.** Solve singular direction j iff σ_j ≥ σ_cut, or its required
  move |u_jᵀb|/σ_j ≤ β. Otherwise leave the prior in that direction.
  - σ_cut is set as an angle θ_cut ∈ {3.6°, 8.1°, 10°} (Lindstrom, Ju, Xiao),
    swept in the arena (H9).
  - The "or move ≤ β" clause keeps today's R3 behaviour for small gaps on
    near-tangent edges: closing exactly is harmless when the move is small.
- **Linear step.** x = VΣ⁺Uᵀb, then the predicted residual
  rᵢ ≈ κᵢ(|x| + |dᵢ|)²/2.
  - If every rᵢ ≤ η, commit in pass 1.
  - Otherwise queue for Newton (pass 2), seeded at p₀ + x.

The existing classes become cases of this solve:

| Healing-paper class | In this procedure | Change |
|---|---|---|
| R1 exact | All dᵢ ≤ η: x ≈ 0, commit | none |
| R2 near-exact | Linear step to both surfaces, commit when the predicted residual ≤ η | **Proposal:** today R2 keeps the 3D curve with the move recorded. The linear step closes the solid at no extra evaluation cost. Arena flag `r2_linear_step` |
| R3 transverse | Full-rank solve, then Newton if residual > η | Leap bounded by verification and ¼ local feature size, not by β (§2.3). Arena flag `transverse_leap_by_conditioning` |
| R4 tangent | Truncated solve with trust weights; the slide comes from the prior | Owner = infinite weight. Equal-tier freeform pairs may split by weight instead (H6) |
| R5 single face | k = 1: x = −d₁n₁ when d₁ > β, else keep the prior | none |
| R6 gross | No verified candidate (§5.4) | Refusal becomes the last rung, not a distance threshold alone |
| V1 stored point | p₀ = stored point when within β of every edge end; full-rank systems still project | none in effect |
| V2 three-surface | Rank-3 solve, then Newton | none |
| V3 enclosing ball | p₀ = ball centre, then solve the well-determined directions | **Proposal:** the ball centre is only the prior. The vertex then lands on every surface it can be placed on, instead of floating off all of them |
| V4 refuse | No verified candidate | as R6 |

At a vertex where a plane P, a tangent fillet F and a transverse wall W meet,
the rank-2 null direction is n_P × n_W, the direction of the P∩W line. The
vertex slides along that line and the prior fixes where. This is the correct
behaviour: it is where the fillet's tangency line hits the wall, which is
ill-conditioned by geometry.

### 5.2 What is trusted first

- **Topology.** Never edited by reconciliation. Shared IDs are issued in
  stage 1 (healing paper §7.3).
- **Face carriers.** Never modified (design §4; healing paper §7.1). Declared
  trust tiers: plane > other analytic > freeform.
- **Well-conditioned intersections.** Full-rank vertex and sample solves
  (Huang; ACIS rigid vertices).
- **Representations, by measured residual.** Pcurve lifts, the 3D curve and
  stored vertex points, used only as priors in null directions.

### 5.3 How confidence propagates: three fixed levels

| Level | Work | Depends on |
|---|---|---|
| L1 | Every edge sample: §5.1 solve. Every vertex with a full-rank system: §5.1 solve | Stage-1 measurements only |
| L2 | Rank-deficient vertices: prior = the stored point if consistent, else the enclosing-ball centre of the **L1-resolved** incident edge ends; then the §5.1 solve | L1 |
| L3 | Each edge: replace its end samples by its vertices. Spread each end's correction Δ₀, Δ₁ along the chain as (1−s)Δ₀ + sΔ₁, with s the normalised arclength. Project the spread onto each sample's null space, so surface constraints already met stay met | L1, L2 |

- The levels are acyclic: no level reads its own outputs. There is no priority
  queue and no iteration.
- L3 is Zhou's 1D uniform Laplacian (§4.3), which in 1D is the linear blend,
  restricted to the free subspace.
  - On a transverse edge the free subspace is the tangent, so L3 only
    reparameterises.
  - On a tangent edge it carries the slide from the vertices.
- L3 replaces the healing paper's "each incident edge's last segment absorbs the
  end move" (§7.2, stage 3). H7 tests the difference.
- **Faces.** Interiors stay on their own surfaces. The fidelity contract's
  target displacement field (design §4.2, linear decay across the band) is the
  oracle's definition of the final position. Zhou's harmonic extension (§4.5)
  is the literature alternative for that definition, and costs a solve only in
  the offline oracle.

### 5.4 When to take a small or a big leap: the ladder per deferred item

| Rung | When | What | Verified by |
|---|---|---|---|
| 0 (pass 1) | Linear-step residual ≤ η and move ≤ β | Commit | Chain monotone (the step is ⟂ to the tangent to first order) |
| 1 | Residual > η, full rank | Newton seeded at the linear step, geometry-derived iteration bound | Converged; chain monotone; UV loop simple; ends meet resolved vertices |
| 2 (big) | Transverse, move > β | Newton to the intersection; any distance up to ¼ local feature size | As rung 1, plus no crossing of another trim of either face (Zhou-style topological check; OCCT FixLacking re-checks crossings after bending) |
| 3 (small) | Near-tangent (σ₂ < σ_cut and move > β), or rung 1–2 failed verification | Truncated weighted solve; slide from the best prior | Chain monotone; recorded gap ≤ ¼ local feature size |
| 4 | Alternative prior (the other representation, or the other owner) when representations disagree by more than β | Re-run rungs 0–3 from that prior | As above |
| 5 | Nothing verified | Refuse the item and its faces, with the record (design §4.6) | — |

- A rung that fails verification reverts to the next rung. This is Zhou §4.7
  and FreeCAD's copy-check-rollback, not the OCCT habit of inflating tolerance.
- Rungs 2–4 run in the background healer, batched by kind.

### 5.5 How pass 2 branches and chooses

**Branches.** A deferred item carries at most the candidates its rungs produce:

- truncated small leap;
- intersection big leap;
- alternative prior or owner.

Each candidate is evaluated cheapest first, and those that fail verification
are dropped.

**Branch-choice rule (proposed).**

1. Discard every candidate that breaks structure: shared-ID closure, monotone
   chains, simple and consistently oriented UV loops, ends on the resolved
   vertices.
2. Among the rest, prefer the candidate that moves no higher-trust carrier
   beyond η. Planes stay flat; analytic faces carry no recorded displacement
   when a freeform neighbour can carry it.
3. Then the candidate that preserves each edge's continuity class measured from
   the source: G1 stays G1, a crease stays a crease.
4. Then the least maximum boundary deviation over all incident faces, which is
   the ε-ledger cost and OCCT's "keep the lower tolerance".
5. Then the least change in end displacement relative to already-resolved
   neighbours, i.e. no kink at a vertex.
6. Then a deterministic tie-break by source identity, so the result does not
   depend on order.

**Keeping branches alive.** Two candidates stay alive only when steps 1–3 tie
and step 4 differs by less than η. Both go into the item's provisional state,
the first-class state the design already requires for sharpen-in-place. The
deterministic winner is emitted, and both are reported.

**Deferring to a user.** The mesher never waits for a user. A later user-facing
tool reads the recorded alternatives, as Borodin's reversible sequence and
Barequet–Kumar's design-intent marking do.

### 5.6 When refusal is the honest outcome

- No candidate passes step 1 of the rule.
- Two top-tier carriers conflict: two planes, or two analytic surfaces in the
  same tier, meet near-tangentially with a gap that must move one of them
  beyond η. No trust order settles it, and the eye would see either choice.
- The move or gap exceeds ¼ of the local feature size. Xiao §6.5 and
  Smith–Tautges–Wilson show repairs failing exactly there; Steinbrenner's
  global condition is "the largest mismatches in the model must be smaller
  than the smallest surface or feature".
- A transverse item's Newton fails its bound, and the small leap leaves a gap
  above ¼ of the local feature size.

Refusal is per item, with every face that uses it refused and recorded. The
rest of the part meshes (journal design default). Ties are not refusals: both
candidates are coherent, so the deterministic one is emitted and both are
reported.

### 5.7 Measurable hypotheses and their falsifiers

| # | Hypothesis | Measurement | Falsifier | If falsified |
|---|---|---|---|---|
| H1 | Most non-R1 edge samples and vertices are final after the linear step from stage-1 data (pass 1, no Newton). The residual estimate κ(\|x\|+\|d\|)²/2 predicts which | Per corpus family: fraction of non-R1 points committed at rung 0. Actual residual after the step vs. predicted residual | (a) Any point whose actual residual exceeds η where the prediction said ≤ η: the estimate is unsafe. (b) The committed fraction is small on a family: for that family the answer to the operator's question is "no" | (a) Always Newton. (b) The healer's queue size, not the linear step, dominates; cost the queue |
| H2 | The incident-face deviation is insensitive to truncated directions out to √(2η/Δκ) | Metamorphic: perturb p₀ along the null direction by δ; measure the change in each incident face's boundary deviation | Change > sin(θ/2)·δ + Δκδ²/2 + arithmetic floor | The linearisation or the conditioning classification is wrong; revisit θ_cut |
| H3 | The three fixed levels give the same positions as a global weighted least squares over each connected component of inexact entities | Model oracle (offline, slow): global solve with the same weights; max position difference | Difference > η on any corpus item | Add an iteration at L2/L3, or a component solve for the failing pattern |
| H4 | Deciding the leap by conditioning (transverse → intersection at any distance up to ¼ local feature size) beats deciding by distance (today's R3/R4 \|x\| ≤ β split) | Arena flag `transverse_leap_by_conditioning`: refusals, max boundary deviation, oracle failures (self-intersection, orientation), time | Not fewer refusals at equal-or-fewer oracle failures | Keep the distance split |
| H5 | The branch rule's choice is never dominated on eye proxies | For items with ≥ 2 verified candidates, emit all (offline) and score: plane/analytic flatness deviation, normal jump across source-G1 edges, crack count, recorded step height, max boundary deviation | A rejected candidate is at least as good on every proxy and strictly better on one | Reorder the rule terms; record the case |
| H6 | For equal-tier freeform tangent pairs, splitting the gap by weight is better than owner-takes-all | Arena flag `tangent_split`: max boundary deviation per face, normal jump across the edge | Split not lower on max per-face deviation, or worse on normal jump | Keep the owner rule |
| H7 | Spreading the vertex correction along the edge (L3) is better than last-segment absorption | Max chord-direction kink and boundary-triangle normal deviation within one segment of each vertex | No measurable difference | Keep the simpler absorption |
| H8 | The background healer is off the critical path | Per face: wait time on pending items; healer busy time vs. meshing pass time | Faces' summed wait is a material fraction of wall-clock meshing (threshold set at G1) | Inline rung 1 for small queues; revisit batching flush policy |
| H9 | The truncation angle matters less than the "or move ≤ β" clause | Arena sweep θ_cut ∈ {3.6°, 8.1°, 10°} | Outcome measures of H4 change materially across the sweep | Select θ_cut by the arena, per surface-pair kind if needed |

The authored families in the healing paper's §7.5 already provide inputs with
known resolutions for H1–H4: offsets swept against ε, dihedral sweeps across
the R3/R4 boundary, pcurve-free variants, vertices off curves, and a
plane–cylinder step. Add one family for H2: a fillet tangent to a plane with
controlled Δκ and stored-curve slide.

### 5.8 What the literature does not say

- No system read runs a background healer. This lane found none beyond the
  healing paper's survey.
- None keeps several geometric hypotheses alive to output.
- None publishes a decision rule for small versus big leaps other than Xiao's
  normal-angle split and the kernels' option ladders.
- None orders trust between two freeform surfaces with a reason. Parasolid
  calls it "arbitrary"; OCCT takes the first.
- None measures, on real CAD, the fraction of inexact edges in the
  well-conditioned versus tangent regimes. H1 does.
- No perceptual threshold in degrees for a visible crease, and no pixel
  threshold for a visible crack.

---

## 6. Corrections to the 2026-10-17 healing paper

These come from sub-lane D's reading of the code. They are recorded here
because this lane edits no other file. [src]

1. **§2.2 FixGaps3d** is incomplete.
   - When the vertex is shared, the target is that vertex's point; the mean of
     two vertex points is used only when they differ.
   - The end of a curve that cannot be converted wins.
   - An intersection, extrema and projection pre-pass exists but is off by
     default and unreachable through ShapeProcess or STEP
     (`ShapeFix_Wire.cxx:173`; `ShapeProcess_OperLibrary.cxx:634-664`).
   - Vertex points are never moved.
   - FixGaps2d uses an independent 2D-midpoint target.
2. **§2.2 FixLacking.**
   - Step 1 is a measured speculative bend with a one-sided fallback and
     re-verification.
   - Step 3 has more conditions than stated (force, Prec, MaxTol, no
     back-going zigzag).
   - Step 5 is a three-way choice, not always a degenerate edge
     (`ShapeFix_Wire.cxx:3726-3814`).
3. **§2.2 FixSelfIntersection.**
   - The tolerance and move options are compared numerically, with tolerance
     preferred.
   - A moved vertex goes to the midpoint of the two 3D evaluations.
   - The non-adjacent pass (`ShapeFix_IntersectionTool`) has its own reuse and
     3-way-split rules. Its interior–end case grows the tolerance to d/2, which
     does not cover the split point (`:1001, :1010`; inferred defect).
4. **§1.2 ACIS phases.** Geometry building merges the calculate and fix stages
   (ACIS Healing pp. 5, 7).
5. **§6.1 SMLib.**
   - SMLib does not close gaps geometrically; `Fix_BadGaps` is an empty TODO.
   - Its edge tolerance is max(Def, maxFaceTol, 1.1·EdgeEdgeGap/2,
     1.1·EdgeFaceGap − Def) (`smlib/inc/SmTol.h:636-650`), and tolerances may
     shrink.
   - Citing SMLib as a gap-healing precedent overstates it.

---

## 7. For SHOPPING_LIST.md

Not acquired, or read only in part. The operator decides whether to pursue each.

- **Papers**
  - Barequet & Kumar 1997, "Repairing CAD models" (IEEE Vis '97). The Technion
    page lists it with no link; the Kumar project page has an abstract only.
    Already listed under parallel-growth-clusters.
  - Weihe & Willhalm 1998, WAE'98 paper itself. It is located in the MPI-I-98-1-019
    proceedings but was not fetched; the Willhalm Diplomarbeit carrying the
    same study was read. The Algorithmica 2000 version was not fetched.
  - Xiao, Alliez, Busé, Rineau 2021, PDF. Only the EG diglib full-text bundle
    was read; figures and appendix equations are garbled there. HAL bot wall,
    a truncated Wayback copy, and diglib PDF 401. Already listed.
  - Urick et al. 2019, "Watertight Boolean operations" (CAD 115). No open copy
    found.
  - Patel, Marcum, Remotigue 2005 (IMR), zip versus stitch by gap width.
  - Langbein 2003 PhD thesis (Cardiff), and Langbein et al. GMP 2002 (on ORCA;
    not fetched, not on the reading list).
  - Werghi et al. 1999, CAD 31(6), published version; the DAI preprint was read.
  - Meyer & Sanders 2003, J. Algorithms 49(1); the ESA 1998 version was read.
  - Müller et al. 2007, JVCIR journal version of PBD; the VRIPHYS 2006 version
    was read.
  - Gao et al. 2018, "GPU optimization of material point methods" (for the
    GPU 3×3 SVD figure).
  - Kaufmann & Klass 1988, "Smoothing surfaces using reflection lines", CAD 20(6).
  - Hagen et al. 1992, "Surface interrogation algorithms", IEEE CG&A.
  - Gouraud 1971 thesis and Phong 1975 (Mach-band visibility); Lavoué &
    Corsini 2010 (IEEE TMM).
- **Kernel documentation**
  - ACIS `api_tighten_gaps` reference (already listed as "Tightening Gaps").
  - Parasolid's internal logic for `PK_FACE_grow_auto_c` and automatic loop
    healing. The Functional Description gives only the option semantics.
- **Measurements nobody publishes** (fleet work, not shopping): a batched 3×3
  eigen figure on P100 and on AVX-without-FMA.

---

## Acquisition notes

- **26 new PDFs**, each checked with `file`. Bytes and sha256 are in the sources
  TSV.
- **Werghi 1999** was fetched as PostScript and converted with Ghostscript. The
  PDF hash depends on the Ghostscript build; the `.ps.gz` sha256 is in the TSV.
- **Archive.org `id_` captures** supplied these:
  - Benkő 2002 and Langbein 2004 (live ORCA returns a bot check);
  - Lavoué 2011;
  - Beier–Chen 1994 (University of Michigan Deep Blue);
  - Biederman 1987 (author page).
- **Xiao 2021** was read as repository full text only; no PDF was saved.
- **Web pages, not saved**: the 3×3 SVD CUDA README, the Patrikalakis hyperbook
  sections, and the Autodesk Alias help page.
- **Code** was read in place in the shared reference copies: OCCT 8.0.1,
  usd-brep SMLib 1.0.0, SolveSpace, planegcs, FreeCAD, Gmsh, Netgen, and the
  Eigen vendored by Gmsh. The Eigen actually linked by SolveSpace and FreeCAD
  is unverified, because SolveSpace's `extlib/eigen` is empty in the held copy.
