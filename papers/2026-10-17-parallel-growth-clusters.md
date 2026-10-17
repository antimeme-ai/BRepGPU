# Parallel growth clusters: prior art, seams, and fit for the fast mesher

Research lane of the fast-mesher study (branch `research/fast-mesher-study`).
It answers the operator's idea recorded in
[the 2026-10-17 direction journal](../journal/2026-10-17-fast-mesher-direction.md):
grow N mesh clusters in parallel from optimizer-chosen seeds, pay for welds only
at cluster seams, and make those welds cheap or free with a shared coordinate
system. Nothing was built, run or benchmarked. The sources, with their read
depth, are in
[2026-10-17-parallel-growth-clusters-sources.tsv](2026-10-17-parallel-growth-clusters-sources.tsv).
PDFs are in ignored `papers/pdfs/`.

How the reading was done: three sub-lanes acquired and read the sources:
(A) CPU parallel meshing, (B) GPU meshing and partition optimizers,
(C) seams, lattices and cut cells. I then spot-checked the numbers this report
leans on against the extracted text:

- Linardakis–Chrisochoides Table 12.1;
- Loseille Tables 1–2;
- Marot §3.4 thresholds;
- Ito's glue time;
- Löhner's shift-and-regrid loop;
- Labelle–Shewchuk §6;
- Zumbusch §5;
- Pardue's separator rule.

Tags:

- **[src]**: a claim a source establishes, with its section.
- **[own]**: my derivation, not established by any source read.
- **[inf]**: an inference from sources plus our recorded data.

## 0. Decision-relevant findings

1. **"Grow from seeds, then weld" loses in every precedent.**
   - Where growth regions meet, the literature either cannot close the front
     or re-meshes the collision zone. It does not weld.
     - Löhner 2001 shifts octree boxes and regrids the leftover inter-box
       fronts.
     - Chrisochoides 2005 §4.2: advancing-front closure on partitioner-made
       subdomains sometimes fails even in industrial meshers.
     - Loseille 2015 and Marot 2019 re-partition and re-mesh the interface band.
   - Tolerance welding has no input-independent tolerance, and the matching
     problem is NP-hard (Barequet–Sharir 1995 §§4–5). [src]
2. **Agree first is the decisive pattern, and it is cheap.**
   - Delaunay decoupling (Linardakis–Chrisochoides 2006/2008) pre-discretizes
     separators so that independent subdomain meshes union conformingly with
     zero communication.
   - Cost: 0.4% (uniform) to 0.58–2.28% (graded) extra elements.
   - Decomposition takes about 0.5% of run time.
   - Fixed-size runs are superlinear: 374.4 s → 7.2 s on 64 CPUs (2006
     Table 12.1). [src]
   - Our shared-edge design ("each physical edge sampled once, keyed") is
     already this pattern across faces.
3. **Inside one face, seams on a shared dyadic lattice cost no welds at all.**
   - Clusters whose boundaries run along lines of a 2:1-balanced UV quadtree
     produce identical seam vertices, keyed by integers, by construction.
   - p4est and Sundar's linear octrees do exactly this: unique anchor-owned
     nodes, hanging status from the child index, no weld. [src]
   - The residual seam work is three things: a read-only ghost layer of
     neighbour levels, one owner per key, and one position computation per key.
   - Warning: p4est reports Balance plus Nodes at >90% of its runtime
     (§4.1). Node numbering is the real seam cost, not the weld. [src]
4. **Free lattice seams change the optimizer problem.** The cut length inside
   the lattice no longer matters.
   - What matters:
     - balance by an a-priori work estimate;
     - never cutting through a trim band except at deliberate separators;
     - a minimum cluster size.
   - Loseille Table 1 measured the work model's effect: 16.6 s idle vs 1.5 s.
     Partitioner choice changed cut size by only about 30% (Table 2). [src]
   - The adequate "seed optimizer" is a weighted Hilbert/Morton prefix-sum
     split, O(n) after an integer sort. METIS, CVT and capacity-constrained
     power diagrams buy compactness we do not need. [src+own]
5. **The trim band is the only place a seam can cost anything.** It is also
   naturally parallel on hole-riddled faces.
   - Each connected band component is an independent constrained-Delaunay
     problem with no seam. Typically there is one component per hole, merged
     where holes are close. [own]
   - A long component can be split by constrained separators that run from a
     lattice-front vertex to an *existing* trim sample, which is Pardue's rule.
     The constrained Delaunay lemma (Shewchuk–Brown §1) then makes the union
     exact with no merge step. [src+own]
6. **Every guaranteed-angle lattice method puts new vertices on the boundary,
   and that breaks agree-first edge sampling.**
   - The methods: Bern–Eppstein–Gilbert 18.4°, isosurface stuffing, lattice
     cleaving, Rushdi's repel-away quadtree.
   - Delete-near-trim plus a band CDT is the compatible design. It has no
     published angle guarantee, so its quality is a hypothesis to measure.
     [src+own]
7. **The gain is real only where a single face's work exceeds the batch's
   share per worker.**
   - Once all 9,162 OM10 faces run in one pool across definitions (the audit's
     first defect), the per-worker share on CPU12 is about 274k/12 ≈ 23k
     triangles.
   - 1713, the largest definition, holds 36.6k facets over 438 faces.
     Per-face counts were never recorded, so this cannot be checked, but
     probably no OM10 face exceeds that share. [inf]
   - Intra-face clusters matter for three cases:
     - single-part latency, i.e. incremental remeshing of a changed part on the
       explosion and complication roadmap;
     - inputs dominated by one huge face;
     - the GPU, which wants cell-level batches anyway.
   - Measure the per-face work distribution first (§6).

## 1. Prior art on parallel meshing

Chrisochoides 2005 §2 classifies methods by when interfaces are meshed
(a priori, a posteriori, simultaneously) and by coupling (tight, partial,
decoupled). The sources read fall as follows.

| Method | Work split | Interfaces | Reported cost / speedup | Interface failure modes |
|---|---|---|---|---|
| Delaunay decoupling PD³ (Linardakis–Chrisochoides 2006) | Medial-axis separators; contracted graph cut by METIS (min separator length / area) | A priori: separators pre-split to lengths in [2k/√3, 2k), k = min(lfs_min, L/4); empty diametral zones are invariant (Prop. 4.3, 6.4) | <0.4% extra elements; 374.4 → 7.2 s on 64 Sun CPUs; imbalance 1.14–1.25 (Table 12.2); decomposition ~0.5% | Separators must meet boundary and each other at ≥60° (Lemma 3.1, medial-axis spokes ⟂ boundary); area-as-work breaks for large subdomains, fixed by over-decomposition |
| Graded PD³ (2008) | Recurse until intra-subdomain size ratio ≤ R1 = 1.425 and neighbour ratio ≤ R2 = 1.5 | A priori, per-side lengths from the sizing function | 0.58–2.28% extra; 10.4B triangles in 272–289 s on 64 CPUs | The subdomain count rises with gradation (1,310 → 19,448) |
| Pardue–Chernikov 2016 | Recursive "+" cuts; balanced by estimated triangle count | Graded decoupling paths; "+" paths **connect to an already existing point closest to the midpoint of the outer border**, so neighbours are never disturbed | 70% efficiency on 256 nodes | — |
| PCDM (Chernikov–Chrisochoides 2008) | Coarse decomposition | Interfaces are constraints; encroachment sends "split at midpoint" messages | ~0.6 efficiency; comms 0.0045 s per M triangles | Rejected decoupling for its "about 2%" over-refinement |
| Uniform PDR (Chernikov–Chrisochoides 2006) | Fixed 2r̄ lattice, phase schedule; no decomposition geometry | Points ≥4r̄ apart are independent | 873.5M elements, 293.7 → 15.1 s (121 procs) | Holes empty lattice cells → load imbalance (pipe case) |
| Löhner 2001 parallel AFT | Octree boxes over the active front, 20–50× element size | A posteriori: leftover box-boundary fronts dissolved by shifting boxes and regridding, three passes | Speedups only in figures; surface meshing >25% of CPU beyond 16 procs (one surface per processor) | The voxel work estimate undershoots where fronts merge, so element caps per processor |
| Loseille–Menier–Alauzet 2015 | BFS-restart partitions balanced by a fitted work model | Freeze the interface, remesh, re-partition the cavity-grown interface band, repeat | 5 iterations, interface 69M → 0 tets; total ≈1.7–1.9× the first pass; 120 → 480 cores gave only 1.5× | Holes disconnect Hilbert parts (component-merge fix) |
| Marot–Pellerin–Remacle 2019 | Equal-count Moore-curve ranges per thread | Buffer zone; failed insertions re-partitioned under a warped, shifted curve | 3B tets in 53 s on a 64-core EPYC | Success ratio falls each round; threads halve below 1/5 success or <3000 points per thread; sequential tail |
| Ito et al. 2007 | METIS on a coarse mesh | Interface faces pre-refined once and shared; glue by **integer index, not xyz** | Serial glue+validation 6,599 s vs 16,558 s meshing | Interface smoothing in a 2-level band |
| Ivanov–Andrä–Kudryavtsev 2006 | Inertia-plane cut of a surface mesh | Cut first through triangles, then along existing edges | Superlinear | Cutting through triangles made "very acute angle or small area" triangles |
| Zagaris et al. 2009 | Advancing partition | Interface layer grown by the same AFT under the final sizing | 20.7× on 64 CPUs | Scalability "lacks the desired level" |
| Spielman–Teng–Üngör 2002 | Independent sets per round | — | O(log²(L/s)) rounds | Theory only |

GPU work [src, sub-lane B]:

- **Qi–Cao–Tan 2011/2013 GPU CDT**, gQM 2017, Chen–Tan–Ong 2020, gCDT 2026 and
  Navarro 2011 never split space into clusters.
  - Each is element-level: candidate operations claim triangles by atomicMin
    priority; losers retry next pass.
  - Speedups need one large problem. Qi is slower than Triangle below about
    100K points. gCDT takes 36.6 ms even at 47K vertices on an RTX4090.
  - gCDT balances by cutting long constraints into equal pieces, one per
    thread, the same idea as merge-path.
- **Chen–Tan–Ong rules worth keeping:** abandon stragglers once about 80% of
  threads finish; hand long low-work tails to the CPU.
- **GPU quadtree/octree:** Wang–Witherden–Jameson 2023 build and 2:1-balance
  linear trees fully on the GPU, level by level with sort/unique/lower_bound
  and no GPU hash table.
- Not acquired: Tu–O'Hallaron–Ghattas SC05 and the Zhou et al. GPU octree full
  text.

CAD surfaces specifically: Löhner meshes one surface per processor.
Laug–Guibault–Borouchaki 2017 (abstract only) report that CAD-kernel geometry
caches serialize parallel face meshing. No source read parallelizes inside one
trimmed CAD face.

## 2. Choosing seeds and partitions

- **Best-known objective [src].** Balanced weighted graph partitioning:
  minimize weighted cut subject to each part's weight ≤ (1+ε)·W/N
  (Karypis–Kumar 1998).
  - It is NP-complete. Multilevel METIS approximates it in O(|E|) per level.
  - Chrisochoides 2005 §3 adds meshing-specific terms:
    - keep lfs(Ω) ≤ lfs(Sᵢ);
    - minimize |∂Sᵢ|/|Sᵢ|;
    - separators meet the boundary and each other at good angles.
- **Work estimate [src].**
  - From scratch, the a-priori output count for a metric M is ∫√det M dA. The
    earlier lane's (C/ε)∫√|K| dA is the same quantity for the chord rule.
  - Loseille shows that density alone misbalances when work is not
    proportional to output: idle time 16.6 s against 1.5 s with a fitted model
    (Table 1).
  - Pardue and Löhner both decompose by estimated triangle or element count.
  - Löhner caps each processor's element count because the estimate
    undershoots where fronts merge.
- **Cheap algorithms [src].**
  - Weighted Morton or Hilbert prefix-sum split: O(n) after a radix sort, with
    negligible runtime in p4est (§4.1).
  - Sundar 2008 partitions weighted coarse dyadic blocks so that seams follow
    coarse lattice lines.
  - Cut quality:
    - Morton segments have ≤2 face-connected pieces on a full square; Hilbert
      segments are connected (Burstedde–Holke–Isaac 2019).
    - Holes break this (Loseille's component-merge pass).
    - Zumbusch §5: when 2D refinement concentrates along a curve (trim-graded
      density), the partition surface is only logarithmically below its
      volume (s ≥ v/log v).
  - BFS-with-restart gives about 20% smaller cuts than Hilbert at similar cost
    (Loseille Table 2).
- **Compact-cell optimizers [src].** These are the closest literature to
  "seeds chosen by an optimizer".
  - Plain CVT/Lloyd equidistributes energy, not mass (cell size ∝ ρ^-1/3 in
    1D), so it does not balance work. It also converges slowly (Du–Faber–
    Gunzburger §5).
  - Capacity-constrained power diagrams (Aurenhammer–Hoffmann–Aronov 1998;
    Balzer 2009) give exact balance with compact convex cells. They cost
    O(n² + nm log(m/n)) per iteration: seconds to minutes. That is acceptable
    for an offline comparison and far too slow for a per-face hot path.
- **For us [own].** With lattice seams free (§3), cut length inside the
  lattice has no cost. The objective reduces to three terms:
  - balance by a work estimate that includes band and boundary terms;
  - zero cuts through band components, except chosen separators;
  - every cluster above a minimum size.
  - Marot's 3000-point and 1/5-success thresholds are the only published
    floor. Ours must be measured.

  A weighted Hilbert split of coarse dyadic blocks, with band components as
  weighted items, satisfies all three. "Seeds" are just range starts. Morton
  and Hilbert disconnection on holed faces costs locality, not correctness,
  because seams are keyed.

## 3. Seams: weld after versus agree first

**Weld-after failure modes [src].**

- **No input-independent tolerance.** Too small misses true matches; too large
  merges distinct nearby vertices (Barequet–Sharir §4).
- **Consistent matching is NP-hard** (§5).
- **Repairs can self-intersect** (§2), and hole filling costs O(Σℓ³) (§7).
- **Rounding leaves leaks** that need a welding step that "(hopefully)" repairs
  them (Hiller–Lipson).
- **Clipping overlapping meshes** makes arbitrarily thin triangles, and
  removing redundancy leaves gaps (Turk–Levoy §5).
- **Re-evaluating the same parameter in two kernels is not bit-safe.**
  Forward-differencing order and direction change the rounding
  (Moreton 2001 §8; Nießner 2016 §3.1.1). The fix is to copy the boundary
  vertices, not recompute them (Schwarz–Stamminger 2009).
- **Even integer-index gluing has a cost when done serially:** Ito's glue plus
  quality validation took 40% of meshing time.

**Seams that are free by construction.** The rule in every precedent is the
same: each side computes the seam vertex set from the seam alone, without
looking at the neighbour (DiagSplit §2; Kumar–Manocha §4.5; Nießner). Three
instances apply to us:

1. **Across faces: the B-rep edge.** Already designed. The sample keys are
   edge identity plus the root-relative dyadic path bits that the existing
   `DefinitionBoundaryForest` keeps. The audit says to keep that model with
   doubles instead of `Rational`.
2. **Inside a face: a global dyadic UV lattice [own].** Map the face's
   parameter box to [0,1)², build a 2:1-balanced quadtree, and key each vertex
   as (i·2^(L−l), j·2^(L−l)) at the finest level L.
   - A cluster boundary on a lattice line yields identical vertex keys on both
     sides, provided transition templates depend only on the two adjacent cell
     levels. The other side's levels are read from a one-cell ghost layer.
   - Ownership follows Sundar SC07 §4: the anchor-corner cell owns the vertex,
     so each key is emitted once.
   - Each position is computed once by its owner and read by key. Recomputing
     on both sides is safe only within one kernel and one instruction
     sequence.
3. **Edge-parameter dyadic paths aligned with the lattice [own].** An edge
   whose pcurve is an iso-line, affine in one lattice coordinate (a cylinder's
   cap circle, a rectangle side, a periodic seam), can be sampled at the
   dyadic parameters of the adjacent lattice nodes.
   - The lattice then runs right up to that edge with no band.
   - The other incident face consumes the samples as ordinary constraints, so
     agree-first holds.
   - The edge rule must also satisfy the other face's density (max of rules).
     This extends NVIDIA's iso-curve block fast path with keyed nodes.

**Where cluster cuts meet trims (cut cells and slivers).**

- **Raw lattice–trim intersections give arbitrarily small angles [src].**
  - Bern–Eppstein–Gilbert: "arbitrarily small angles may arise … when the two
    endpoints, but not the midpoint, of a split side warp."
  - Ivanov: cutting through triangles made "very acute angle or small area"
    triangles.
  - Kumar §5.3, two failures:
    - placing crossing points on the trim cracks against the neighbouring
      face;
    - reusing existing samples makes adjacent cells overlap, which they fix
      with a postponed priority cleanup pass.
- **Every guaranteed remedy creates new boundary vertices on the trim [src].**
  - Bern: warp onto segments, ≥18.4°.
  - Isosurface stuffing: snap, 8.9–164.8°. But ordered warping cascades and
    resists parallelism (§3.2), and graded boundary lattices fall to 1.66°
    because cells "cannot get them to agree on their shared diagonals" (§6).
  - Lattice cleaving: an existence bound only.
  - Rushdi 2017: repel corners δ = s/4; 45–135° on smooth domains, 22.1–169.7°
    with holes.
  - All of these contradict agree-first, because the neighbouring face uses a
    different chart and cannot know about the new points.
- **Compatible design [src+own]:**
  - delete lattice vertices within δ·h of the trim;
  - fill the band between the lattice front and the existing trim samples by
    CDT, which maximizes the minimum angle over triangulations of those
    vertices;
  - add band-interior Steiner points freely (they touch no neighbour);
  - never let a cluster seam run into the band: seams end at the lattice
    front.
- **Quality targets.** Shewchuk 2002: caps (angles near 180°), not needles,
  destroy normal accuracy. So the band's quality target is a maximum angle,
  plus a minimum angle for slicer facet normals.
- **Separator constraints in a band [src+own].** Pardue's rule ends every
  decoupling path on an existing vertex. PD³'s ≥60° condition comes from
  medial-axis spokes perpendicular to the boundary. Together they suggest
  separators from a front vertex to the nearest existing trim sample,
  near-normal to the trim and about one band width long. The CDT lemma
  guarantees the union of the chunk CDTs is the CDT of the band plus those
  separators.

**Dissolving seams afterward.**

- **Literature methods [src]:**
  - Löhner's shift-and-regrid;
  - Loseille's freeze, remesh and re-partition, about 1.7–1.9× the first pass;
  - Marot's re-partition under a warped curve;
  - Turk–Levoy's tolerance clip and refill;
  - Kumar's cleanup polygons.
- None is cheap relative to a priori decoupling; all re-mesh rather than weld.
- **Under agree-first nothing in the lattice needs dissolving [own].** A band
  separator can be dissolved later by un-constraining it and running Lawson
  flips until every edge is locally Delaunay, at a cost proportional to the
  flips. No source presents this as seam dissolution.

## 4. Fit for BRepGPU

Context: today's face path fills the whole band of a face, holes included,
with one connected CPU CDT
([ADAPTIVE_TRIMMED_FACES.md](../docs/ADAPTIVE_TRIMMED_FACES.md)). Each face
runs as one single-threaded state machine inside barrier waves, and
definitions run serially
([owned-mesher audit §4](2026-10-17-owned-mesher-audit.md)).

- **Across faces.** The growth-cluster idea adds nothing here: the shared
  B-rep edge is already an a-priori separator.
  - Graded PD³'s neighbour-ratio condition R2 corresponds to the requirement
    that one edge sampling satisfy both faces: max of the incident rules, plus
    NVIDIA's 1.1× leaf rule.
  - When a fine face meets a coarse face, the coarse face pays a grading band.
    This is a sizing cost, not a seam cost.
- **Inside one large trimmed face [own].**
  - **Lattice cells** are embarrassingly parallel given the 2:1 levels. A
    cluster is just a contiguous Hilbert range of coarse blocks.
    - 2:1 balance is a global pass (Isaac 2012; Wang 2023 on the GPU).
    - If the level function comes from a sizing field graded to a
      log-Lipschitz bound, balance may hold by construction. This is
      unproven; check it with an O(n) count.
  - **Band components** are found as connected components of the band region:
    - one per hole, with close holes merged;
    - one or more per outer-loop stretch.
    - Each is an independent CDT.
    - A hole-riddled face therefore yields about as many independent band jobs
      as it has loops. The current single connected CDT is the serialization.
  - **Long band components** get Pardue-style separators.
  - **Holes:** they unbalance a fixed lattice schedule (Chernikov's pipe case).
    Weighting cells by predicted work, with deleted cells at zero, handles
    this.
  - **Periodic seams:** with the period mapped to the root cell, keys wrap
    modulo 2^l. Either the face owns its seam-edge sampling at lattice
    positions (no band, keyed wraparound), or the seam is treated as a trim
    with a band.
  - **Poles:** every lattice node on the pole row aliases to the
    degenerate-edge vertex (the existing vertex-alias model). Pole-row quads
    emit one triangle each.
    - An isotropic quadtree over-refines in u near a pole.
    - Separate u/v levels per cell, keyed (l_u, l_v, i, j), keep keys integral.
      This is an open design point.
  - **Slivers:** the δ·h deletion margin keeps cluster cuts away from trims, so
    cuts never meet trims. Only band separators reach the trim, and they end on
    existing samples at chosen angles.
- **GPU mapping [src+inf].**
  - No source maps "one growth cluster per block" for mesh generation.
  - Lattice cells across all faces of a batch form one SoA batch: a thread per
    cell or tile, Morton-ordered for evaluation locality, scan-allocated
    output, keyed node ownership. On the GPU a "cluster" is unnecessary; it is
    a CPU scheduling unit.
  - Band components are small. GPU CDT loses below about 100K points (Qi), so
    bands start on CPU SIMD with merge-path or LPT balance. Batching them on
    the GPU, warp per component, is a later hypothesis.
  - Long tails go to the CPU (Chen Rule 5). Avoid dynamic parallelism (prior
    lane).
- **Load balance on heterogeneous faces [src+inf].**
  - Use one global pool over all definitions. Items are edge-sampling batches,
    lattice ranges and band components, each with an a-priori work estimate.
  - Over-decompose so the largest item is ≤ W/(c·P) (PD³ over-decomposition;
    Löhner caps).
  - Schedule greedily, largest first. Graham's LPT bound, makespan ≤
    (4/3 − 1/(3P))·OPT, is a classical result not acquired in this lane.
  - A face needs intra-face clusters only when its estimate exceeds the
    per-worker share. This rule is derived from the inputs, not from source
    IDs, so it complies with the no-overfitting rule.

## 5. Recommendation

Adopt the operator's idea in its agree-first form: **partition along a shared
dyadic lattice; never grow-then-weld.** Concretely:

**C1. Lattice clusters with integer-keyed seams (inside a face).**

- Mechanism:
  - per-face [0,1)² dyadic quadtree from the density field, 2:1 balanced;
  - clusters = weighted Hilbert ranges of coarse blocks;
  - anchor-owner vertex keys and a one-cell ghost read;
  - positions computed once per key.
- Hypothesis H1 (correctness): the keyed triangle set of a face is identical
  for every cluster count K and every block-to-cluster assignment. This is a
  metamorphic oracle (mesh(K=1) ≡ mesh(K=k)) and replaces any seam check.
- Hypothesis H1′ (cost): ghost reads plus node numbering cost ≤ 10% of lattice
  emission time at K = P on CPU12.
- Falsifiers:
  - any triangle-set difference across K;
  - node numbering above that fraction. p4est's >90% Balance+Nodes is the
    warning; if numbering dominates, replace global numbering with per-cluster
    vertex lists plus owner references.

**C2. Band decomposition into components plus separator constraints.**

- Mechanism:
  - connected components of the band, each an independent CDT;
  - components above the size cap split by front-vertex → existing-trim-sample
    constrained separators, near-normal to the trim.
- Hypothesis H2 (correctness): the union equals the CDT of the band PSLG plus
  separators. Oracle: every non-constraint edge is locally Delaunay.
- Hypothesis H2′ (quality): within one band width of a separator, maximum and
  minimum angle histograms are no worse than in the unsplit band.
- Hypothesis H2″ (speed): band wall time on hole-riddled faces falls roughly
  with min(components, P).
- Falsifiers:
  - caps (>150°) or degenerate facets appear near separators more often than
    without them;
  - band wall time does not fall because one component dominates.
- If quality fails, dissolve separators with Lawson flips and measure the flip
  count.

**C3. A-priori work estimate and one global LPT pool.**

- Mechanism: per-item estimate (lattice cells from ∫√det M, band triangles
  from trim samples × layers, edge samples), over-decomposition, largest-first
  scheduling over all definitions.
- Hypothesis H3: predicted vs measured item time has R² ≥ 0.8 across
  independent workload families, and makespan ≤ 1.1·max(W/P, largest item).
- Falsifier: weak correlation (then fit a Loseille-style regression), or
  makespan set by an item the splitter could not shrink.

**C4. Lattice-aligned dyadic edge sampling for iso-line edges.**

- Hypothesis H4: on analytic faces (planes, cylinders, cones; 92% of OM10 per
  the earlier lane), band triangle count and band time fall measurably, at
  equal chord deviation, with no change in shared-edge agreement.
- Falsifier: aligned sampling forces denser edges than the max-of-rules
  sampling, and the triangle count rises.

**C5. GPU placement** (after C1–C3 on CPU).

- Hypothesis H5: one batched lattice-emission kernel over all faces beats CPU12
  SIMD for the lattice phase, transfers included, above a measurable total cell
  count. Bands stay on CPU.
- Falsifier: P100 is slower than CPU12 at the watch and ABC batch sizes.

**Rejected, with reasons:**

- **Seed-grown advancing fronts with post-hoc weld** (the literal idea). Front
  collisions are the AFT failure point, and tolerance welds have no correct
  tolerance. Building it as a control would be ceremony; build it only if the
  operator wants the negative result measured.
- **Interface-shifting dissolution** (Löhner, Loseille, Marot). It costs a
  multiple of the first pass, and agree-first makes it unnecessary.
- **Guaranteed-angle snapping or warping** (Bern, stuffing, cleaving, Rushdi).
  It adds trim vertices and breaks agree-first.
- **GPU single-triangulation CDT per face.** It is too small to pay off.
- **CVT, CCVT and METIS in the hot path.** Lattice cuts are free, so their
  compactness buys nothing; keep them for offline comparison only.

## 6. Measure before building the cluster machinery

1. **Per-face predicted work distribution** on the watch, the ABC sample and
   the authored families, once the a-priori sizing exists:
   - largest-face share against W/P for P = 12 and 24;
   - band components per face, and the largest component's share.

   Recorded data cannot answer this. Per-face timings were never emitted (audit
   §2), and
   `context/adaptive-watch/watch-cpu12-attempt8/query-0/definition-*/` holds
   no face artifacts.
2. If no face exceeds the share in the batch workloads, ship C3 alone first.
   C1 and C2 then serve the single-part latency case (incremental remesh) and
   the GPU, and should be measured on those.

## 7. Not acquired or not read

These belong on the shopping list; this lane touched no other files.

- Galtier–George 1996 (secondary only).
- Löhner–Cebral 1999 and Löhner 2013/2014.
- Laug–Borouchaki 2011 (paywalled).
- Laug–Guibault–Borouchaki 2017 (HAL bot-walled).
- Said et al. 1999.
- Tremel et al. 2007.
- Tu–O'Hallaron–Ghattas SC05.
- Zhou et al. 2010/2011 GPU octrees.
- Aurenhammer–Hoffmann–Aronov 1998.
- Loseille–Alauzet 2011 part I.
- Dyken–Reimers–Seland 2009.
- Guéziec et al. 2001.
- Barequet–Kumar 1997.
- Cripps–Parwana 2011.
- Rockwood et al. 1989.
- Yerry–Shephard 1983 (read only through Shephard 1988).
- Graham 1969 (LPT bound, cited from standard knowledge).
- Hamann–Tsai 1996 is local but scanned with no text layer; unread.
- Lattice cleaving was read only through the PMC HTML; no local PDF.
