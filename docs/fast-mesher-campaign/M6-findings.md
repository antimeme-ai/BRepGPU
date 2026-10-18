# M6 design tribunal: findings

Seven reviewers read `docs/FAST_MESHER_DESIGN.md` (as of d13d63b) once each, in
parallel, one mandate each, under `briefs/M6-design-tribunal.md`. This file
collects their findings for the designer's single fix pass. Line numbers refer
to the design as of d13d63b.

Totals: 94 design findings, 68 bead findings, across seven mandates. Many
design findings are the same defect seen from two mandates; the overlap map
below groups them so each is fixed once.

## Dispositions

The designer fills one row per design finding in the table at the end:
`fixed (§n)`, `rejected (reason)`, `bead`, or `operator`. Bead findings are
filed by the coordinator; the designer marks any it fixed in passing.

## Operator answers during the tribunal (2026-10-18)

- **R5.5, what "matched" means: achieved.** The headline compares the two sides at achieved fidelity: each side's measured deviation (and angle term) meets the request. NVIDIA's arm for the headline is its fastest arm whose measured output achieves ε, with its chord tightened as needed. An NVIDIA arm that misses ε is not our win; it is re-run tighter. Operator's words: "achieved".
- **R5.15, steel-man scope: as shipped.** NVIDIA is measured as its shipped binary. Rebuilding it from source with its compiled-out options (TBB paths, scalable allocator) is a named exclusion. Operator's words: "as shipped".
- **R6.3, runtime self-intersection classes: toggle, default offline or on request.** The full input self-intersection check is an explicit option. By default a run evaluates only the profile criteria the mesher already computes; the self-intersection classes and the P2 rerun run offline or when requested. Operator's words: "let's make this toggle-able w/ default offline/request".
- **R3.1, owned reader before the first headline: yes.** The owned reader (`.25.7`) is on the critical path, and no headline is published until ingress fits a budget no larger than NVIDIA's minimal ingress. Operator chose the recommended option.
- **R4.12, cancellation exit code: new code 6 = cancelled.** A cancelled run exits 6 and records what completed. Codes 1, 3, 4 and 5 keep their meanings. Operator chose the recommended option.

## Overlap map (fix each group once)

| Defect | Findings |
|---|---|
| Repaired target depends on ε and on producer output; residual unmeasured; ramp unbounded | R7.1, R2.1, R1.5, R1.6 |
| Closure feedback loops; graph not acyclic; readiness counters wrong | R6.2, R3.5, R4.2 |
| Residual detector is a second validation layer; freeform bound needed instead | R6.1, R1.12, R3.b8, R7.b4 |
| Export frame and δ_exp per occurrence vs per key; oracle 1 threshold | R7.5, R1.9, R2.9, R4.10 |
| Mesh key identity and ε in the key | R7.3, R1.13, R7.4 |
| Functional equivalence and O-bytes oracles | R6.4, R2.4, R6.b2, R2.b3, R2.b11 |
| Volume oracle 10 | R6.6, R2.11 |
| GPU predicate oracle 16 | R6.7, R2.b8, R3.b9 |
| Speed target arithmetic and stage budgets | R5.14, R3.2, R3.1 |
| Calibration and cache state across runs | R5.9, R3.11, R4.13 |
| NaN, closed-form domains, signed distances, non-positive budgets | R4.3, R4.5, R1.3, R1.b3, R1.b12 |
| Oracle outcome not_evaluated; our failures scored as NVIDIA's | R2.3, R5.1, R4.7 |
| Status lattice: per-clause bits, caps, budgets, per-format | R6.5, R4.7 |
| Angle term: oracle and matched-achieved headline | R2.8, R5.5 |
| Per-lane and per-GPU reporting; no cross-link comparison | R5.6, R5.b8, R3.b2, R3.12 |
| Watch-shaped corpus and arena design | R7.2, R7.10, R5.10, R5.2 |
| Cusps | R4.11, R1.14 |
| Malformed-value refusal catalog | R4.1, R1.b14, R4.b2 |

## Reviewer 1: geometry and numerics

Reviewer summary: the five most severe would each refuse faces, or move points the wrong way, on ordinary corpus geometry if built as written.

### Design findings

**R1.1 Edge wedge budget is zero at every tangent edge (§12.1 L1333–1336; §12.2 C1–C2 L1349–1354; §12.3 L1376–1377).** class: design
- Wrong: wedge opening defined as Δψ(0) = θ "measured at every sample (§8.1)", but §8.1 L929 defines θ as the angle between the two normals (§15.2 L1564: smooth means θ ≤ 0.01°). G4's opening is the dihedral, π − θ at a convex edge. Design drops G4's clause "ρ itself when Δψ > π/2". As written every G1 edge (fillet to plane, θ ≈ 0, Δψ ≈ π) gets sin Δψ ≈ 0; star tilt budget ½·sin Δψ − m negative; r*_e = 2(ε̂+μ)/sin Δψ unbounded; every tangent-edge face refines without end or is refused. In/out indistinguishable since normal angle carries no side.
- Evidence: G4 L310–317 ("Δψ_in(0) is the dihedral"); G4 L405 ("ρ itself when Δψ > π/2"); G4 L557 ("Tangent (G1) edges … no wedge hazard, Δψ ≈ π").
- Fix: Δψ_in(0) = π − θ_s, θ_s the normal angle signed by (n_f × n_g)·t_e on coedge orientation; Δψ_out = 2π − Δψ_in. s(Δψ) = sin Δψ for Δψ ≤ π/2, 1 above, in C1, C2, r*_e and E3 budgets. Curvature correction on the opening, not on θ.

**R1.2 3D "local feature size" behind every ¼-lfs bound undefined; its usual meaning is zero at edges (symbols L62; §8.5 L990–1004; §8.8 L1035–1038).** class: design
- Wrong: medial-axis lfs is 0 at every sharp convex/concave edge, exactly where used. With 2r_c floor, ¼·lfs = ε/4 < β = ε/2: rung 2 range (move > β, ≤ ¼ lfs) empty, rung 3 steps capped at ε/4, structural gross gate refuses every gap above ε/4. NIST FTC-07 case cited at L1001 would refuse.
- Evidence: G2 L363–369 (corners make medial-axis LFS zero; use PSLG-style lfs), G2 L410–411 (lfs measured to features disjoint from e) — applied only to UV lfs (§11.5 L1295). M2 L485–486 cite Xiao and Smith–Tautges–Wilson without defining it.
- Fix: define 3D feature size PSLG-style: at point p of item x, radius of smallest ball centred at p meeting an entity disjoint from the closure of x's star. T1b computes it as a lower bound from box distances. Keep floor rule.

**R1.3 Distances unsigned but solve needs signed (§8.1 items 3–4 L927–930 → §8.3 L951).** class: design
- Wrong: d_f = |P − S_f(u_f)|; rows n_i·x = −d_i and |x| formula need signed d_i. Unsigned moves the point the wrong way whenever d_i < 0; opposite signs and small θ understate |x| by ~2/θ. At θ → 0 or π, |x| divides by sin θ → MEASURE_ "max |x|" Inf/NaN on tangent edges.
- Evidence: M2 §1.5 L325–330 ("signed distances dᵢ"; |x| identity holds only for signed).
- Fix: d_f = n_f·(P − S_f(u_f)), n_f oriented by face sense. Report the move from the truncated solve; record no |x| where σ₂ < σ_cut.

**R1.4 Truncation threshold taken on weighted matrix; hard-plane block unspecified (§8.3 L951–957).** class: design
- Cutoff: M2 derived θ_cut ↔ σ_cut (8.1° ↔ σ ≈ 0.1) for unit weights; with "high finite" analytic weight w, σ scales by √w, so near-tangent plane–fillet and cylinder–cylinder junctions pass as full rank, are solved along the ill-conditioned direction (move ≈ gap/sin θ) and routed to rung 2's big leap, which M2 §2.3 says is ill-posed there.
- Hard block: elimination unspecified when hard plane rows rank-deficient (coplanar split faces, common in exports; planegcs refuses rank-deficient hard sets) or overdetermined (≥4 planes at a vertex).
- Evidence: M2 L349–360 (unit-weight singular values √(1 ± cos θ)); M2 L263.
- Fix: classify conditioning on unit-normal rows after hard elimination; weights enter only the solution. Hard block by truncated SVD with same angle cutoff. Overdetermined hard rows by least squares; commit only if residual ≤ η, else rung 3, or refuse when both faces of one tier (§8.7).

**R1.5 Displaced target not bounded by the ledger (§3.2 L198–203; §3.4 L223–225; §14 L1525–1527; §3.10 L285).** class: design (overlaps R7.1, R2.1)
- Wrong: e(t) uses only carrier D bounds. Target near a displaced edge = carrier + linear ramp of height d over ℓ₃, with kinks at edge and at ℓ₃, not linear in UV along curved edges. First-row triangles (apex ~ℓ₃/2 away, Lemma 1) straddle the ramp, picking up ~d/4 interpolation error: up to ε/8 at rung 0 (d ≤ β), far above ε for rung 3 (d up to ¼ lfs). Ledger can't see it; nothing refines. Composition of two edges' ramps near a vertex unstated. Whether A0/Rule B "exact normal" is carrier's or target's unstated; a steep ramp tilts target normal by ~atan(d/ℓ₃).
- Evidence: §13.1 D bounds (L1442–1452) contain no displacement term; M2 §5.3 L734–739 calls the field an oracle definition only.
- Fix: C¹ decay (e.g. cubic Hermite in distance) with its second-derivative bound added to D inside the zone, or charge the ramp's interpolation term per triangle. Define composition near vertices (nearest-edge partition, or max). Name the normal: target's, or carrier's with tilt charged to A0 and shading.

**R1.6 Rung 0 residual predicted, never measured, never charged (§8.3 L958–959; §8.4 L974; §3.2 L198).** class: design (overlaps R2.1)
- Wrong: rung 0 commits on r_i ≈ κ_i(|x|+|d_i|)²/2, an approximation; freeform κ_i has no bound (§6.2 L726). L3 then slides samples up to β along linearised tangent, adding κβ²/2 off-surface, > η wherever edge radius < 8ε; nothing re-verifies. Surviving residual up to η (larger after L3) neither charged in §3.4 nor covered by §3.2 (which displaces only for "a truncated direction, a recorded tangent step"), so boundary vertices sit off target uncharged.
- Evidence: §8.5 rung 0 "verified by chain monotone" only (L988).
- Fix: after L3 evaluate actual residual per sample and occurrence (one seeded projection; closed form on analytic). Commit on measured value; either always displace the target through the resolved point or add a residual term to the ledger.

**R1.7 Multi-span B-spline faces and low-continuity knot lines not handled (§13.1 L1440; §13.2 L1459–1462; §3.4 L224).** class: design
- Wrong: D per Bézier span, but band, CDT and boundary triangles cross span boundaries; "face's scaled chart" for one CDT undefined when D varies. At knot multiplicity ≥ p − 1 second derivative jumps; at multiplicity p (C⁰ crease inside the face) Waldron fails (error first-order). Exact normal there two-valued, so Rule B, A0, §15.1 undefined. Knot multiplicity appears only in the fuzzer (L2307).
- Evidence: FT §1.2–§1.3 (bounds per span); FT L87 (M must bound second directional derivative over T).
- Fix: a triangle's D = max over every span its UV hull meets. Knot lines of multiplicity ≥ p become constraint lines in lattice and band, two normals per vertex like a face-internal seam, classified as interior creases. Define band chart per face (max D, or local rescaling inside per-triangle test).

**R1.8 Zheng–Sederberg rule used outside what it proves (§13.1 L1450–1451; §3.4).** class: design (overlaps R6.12)
- Wrong: ZS Theorem 4 is a step rule for linear interpolation along parameter segments or grid steps; design uses it as D_uu, D_uv, D_vv inside the arbitrary-triangle Waldron rule, which needs sup ‖D²_dd S‖ of the rational surface. Extension to arbitrary band/CDT triangles is an unestablished owned step.
- Evidence: FT §1.2 L147–151; FT L87.
- Fix: derive M by quotient rule on recentred homogeneous data: ‖D²S‖ ≤ (‖D²H‖ + 2‖Dw‖·L_S + ‖S − c‖·‖D²w‖)/min w, L_S ≤ (‖DH‖ + ‖S − c‖·‖Dw‖)/min w (H homogeneous numerator, c recentring point). Or prove a triangle form of ZS. Either way falsify on a high-weight-variation family.

**R1.9 Export rounding bound depends on occurrence, mesh is per key (§18.1 L1717–1719; §18.2 L1737; §3.4 L233–235; §16.2 L1621–1624).** class: design (overlaps R7.5, R2.9, R4.10)
- Wrong: 3MF/STL rounding set by world coords, differing per occurrence; mesh and C1–C3 belong to one shared key. 3MF item-local float32 + PrusaSlicer's bake round twice ("world rounding plus one more"); ledger charges one δ_exp. C1–C3 checked on stored values, not what consumer holds.
- Evidence: G3 §4.4 L734–738; G3 §2.1 L189–198.
- Fix: δ_exp(key, format) = max over key's occurrences of (σ_max·δ(R_local) + δ(R_world) + transform-parse term) for baking consumers; build C1–C3 with 2·δ_exp. Put that δ class in the key, or quantise to a power-of-two class so keys stay shared.

**R1.10 Clearance budgets use a per-face global displacement (§12.1 L1328–1330; §12.3 L1373–1374).** class: design
- Wrong: μ_f = face's largest target displacement, subtracted in every local budget ½(c − μ_f − μ_g)(1 − m). One rung 2/3 step anywhere on a face makes every clearance below μ_f + μ_g on that face unbuildable → face refused. Senders legally declare 0.127 mm gaps (§8.5 L1001). §3.2 displacement only reaches ~ℓ₃ from the edge.
- Fix: μ(zone) = sup of displacement field over the clearance zone's footprint, zero outside the decay band.

**R1.11 P1 and P2 run on quadrature estimates, not bounds (§11.1 L1215; §11.3 L1273–1274).** class: design
- Wrong: τ(s), ℓ(s) from fixed-order quadrature can miss turning between nodes; Lemma 1's sufficiency and "no count" claim (L1211) need τ ≤ τ₀ and w̄ to be true upper bounds.
- Evidence: G2 L614–616 chose quadrature for order-independence, not soundness.
- Fix: bound τ by angular spread of the hodograph's Bernstein coefficients (B-spline and rational pcurves; order-free, hereditary under subdivision). Bound ℓ by control-polygon length; closed forms on analytic arcs.

**R1.12 Open σ̲(DS) and normal-turning bound has a standard construction (§6.2 L725–731; §11.3 L1276–1282; §27 #25–#26).** class: design (resolves R6.1's freeform alternative, R2.8 partially)
- Wrong: normal cones from Bernstein form of S_u × S_v (polynomial, degree (2p−1, 2q−1)) or of the homogeneous triple product (rational) bound normal turning per sub-span (Sederberg–Meyers-style; FT §1.5 L225 names the Bernstein normal cone). Same construction gives |S_u × S_v| ≥ min|N|, so σ̲(DS) ≥ min|N|/max‖DS‖. Current defaults are not bounds (corner-normal spread; sampled foot points with 2× margin); the second governs closure on the normal path, where 98.7% of non-planar coedges have no pcurve (§5.1 L486).
- Fix: adopt Bernstein normal cone as default bound. Where the hull contains the origin (pole or irregular point), refuse or treat the locus as a quotient.

**R1.13 Rigid key uses ε/σ_max (§16.1 L1603; symbols L55).** class: design
- Wrong: rigid placements have σ_max = 1 ± a few ulps, differing per occurrence; literal keys split every rigid instance into its own mesh (ends instancing); if shared, world deviation exceeds ε by up to 1 + τ.
- Fix: for rigid and mirrored kinds use one canonical ε_def = ε/(1 + τ).

**R1.14 Material-cusp edges refused by default (§12.3 L1385–1389).** class: design
- Wrong: G4 shows no finite unsigned budget exists at a material cusp (faces tangent with material between); only signed credit works. Signed credit is an arena flag default off, so every such edge's faces refuse and the part is incomplete.
- Evidence: G4 L555.
- Fix: signed credit on by default for edges whose unsigned budget is provably zero (cusp classification at T7), off elsewhere; or state the refusal as a known coverage gap.

### Bead findings
- R1.b1 Symbols L65, §3.4 L234: δ "≤ 6.6 nm under 64 mm" is √3·ulp; correct 3.3 nm (G3 §1 row 1 same slip). "26 nm for R ≤ 512" should be R < 512 (R = 512 gives 52.9 nm). — fixed (symbols table; §3.4)
- R1.b2 §10.3 L1155 conic sag κ_max·ℓ²/8 is a bound only when ℓ is arc length or its upper bound; chord length understates (~7% on a 60° arc). — fixed (§10.3)
- R1.b3 §6.2 L721, §13.1 L1437 acos(1 − ε/R) NaN for ε ≥ 2R and inaccurate as ε/R → 0; use 4·asin(√(ε/(2R))) with clamp. (Overlaps R4.5.) — fixed (§6.2)
- R1.b4 §6.2 L724 torus angle term κ_max → ∞ at spindle and horn pole lines; needs quotient treatment.
- R1.b5 §11.2 L1246–1248 zero-speed parameterisations (clamped coincident control points, degenerate pcurves) give ρ = ∞, q = 1; packing-depth formula undefined. Cap by speed lower bound or arithmetic floor.
- R1.b6 §3.4 L226, §11.1 P4 r_trim and P4 evaluate carrier extension outside UV domain; B-spline carriers have none beyond knot rectangle; define extension or forbid chords leaving it.
- R1.b7 §10.3 L1164, §13.1 table ε_b and generator rules use ε_loc where they should use residual budget ε_loc − δ_exp − r_coll − r_trim. — fixed (§10.3)
- R1.b8 §12.1 L1326 ε̂ ≤ 2ε_loc + s_e stale once r_trim is in the ledger (ε̂ = ε_loc). — fixed (§12.1)
- R1.b9 §12.3 L1366 "cylinder–plane" closed form is 0 for non-parallel carriers; needs trim-aware bound. — fixed (§12.3)
- R1.b10 §13.2 L1461 state the fixed-point UV is the UV of record; evaluate interior points at snapped UV; charge boundary snap as L_S·g/2.
- R1.b11 §24.1 #14 L2277 scaling by 2^k with odd k puts √2 into P4 and Waldron comparisons; compare squared forms or scale by 4^k, else oracle fails without a bug. — fixed (§24.1 row 14)
- R1.b12 §15.1, §3.10 irregular interior points of B-spline faces (S_u × S_v = 0 away from declared pole/collapsed edge) leave Rule B undefined; detect with normal cone, refuse or use limit normal. (Overlaps R4.3.) — fixed (§6.2, §5.8)
- R1.b13 §16.1 L1603 n′ = A·n unnormalised for rigid within τ; normalise (glTF 1e-6 unit-length oracle). — fixed (§16.1)
- R1.b14 §5.4 row 3 non-positive rational weights violate ISO 10303-42 and make "min w" meaningless; refuse at ingress by name. (Overlaps R4.1.) — fixed (§5.8)
- R1.b15 §16.1 L1597 take σ from AᵀA by the deviatoric (Smith) form, not cubic coefficients; near-triple eigenvalue otherwise errs ~u^(1/3) ≈ 5e-6, above τ. — fixed (§16.1)
## Reviewer 2: fidelity and oracles

Reviewer summary: §24 says each oracle runs against the original source, never trusting a producer status, but several oracles take target, threshold or exemptions from what the producer wrote. Oracle 1 does not exist in code today.

### Design findings

**R2.1 Deviation target built from the producer's own chain; reconciliation errors have no oracle (§3.2 L198–203; §8.5 L988; §24.1 rows 1, 4 L2264, 2267).** class: design
- Wrong: oracle 1 "rebuilds the same target" from chain bytes, so the target passes through whatever position the chain records — oracle 1 can't fail at a boundary sample. Ramp width ℓ₃ is the producer's segment length, so target changes with backend and LOD level. Oracle 4 checks a vertex against its resolved position (G16 §3.4 item 1), again producer output. Rung-0 commit "verified by: chain monotone" only; commits on a predicted residual κ(|x|+|d|)²/2 (L958–959). Nothing measures how far a resolved point sits from each incident carrier, that a plane moved < η, that a move stayed < β (rung 0) or < ¼ lfs (rungs 2–3). Any wrong leap < ¼ lfs passes every oracle. Since target needs producer chain bytes, oracle 1 can't apply to NVIDIA output; §22.5 L2116 "measured deviation" of NVIDIA has no defined oracle.
- Evidence: M2 §5.7 H1(a) falsifier "any point whose actual residual exceeds η where the prediction said ≤ η"; M2 §5.7 H3 model oracle (global solve) not in §24; G7 §3.3 N6 (self-certification).
- Fix: source-anchored reconciliation oracle: for every chain sample and vertex, recompute with its own projection the distance to each incident source carrier and the move from source position; fail on hard-tier move > η; rung-0 residual > η or rung-0 move > β; move > ¼ lfs with lfs recomputed by oracle. Add M2 H3 global solve as model-tier row (fail when positions differ > η). Oracle 1 then judges faces against verified chains. Define oracle 1 on a foreign producer: against source with d_e allowance.

**R2.2 Oracles the stack depends on don't exist; kept readers encode the old contract (§25 L2335–2336; §24.1 rows 1–2; §5.3 L554–558).** class: design
- Wrong: kept "independent original-source readers" require every vertex on its source generator within 2⁻³² (imported_oracles.py:2140), every edge sample on the original curve (:2171), endpoint departure ≤ tol/8 (:2164), a 2D pcurve on every coedge (:1399). New design moves vertices on purpose and 98.7% of non-planar coedges have no pcurve (G1). Readers will fail correct output or be loosened ad hoc. qualify_mesh says it is "not a claimed global approximation certificate" (:2104). Existing distance query refuses "face-edge has no usable trimmed pcurve" (NVIDIA-complete report L18). Oracle 1 "to the trimmed target face" and oracle 2 "per-face UV" don't say where the trim domain comes from when the file has no pcurve; if from producer per-occurrence UVs (§8.1), it restates §8.1.
- Fix: oracle 1 and the trim domain are explicit build units. Oracle builds each face's trim domain by its own projection of source 3D curves, separate from §8.1's seeded projection, never reading producer UV. Replace readers' "on source to 2⁻³²" checks with R2.1's oracle; lift mandatory-pcurve rule.

**R2.3 Oracles have no not_evaluated outcome (§24 L2256; §7.1 L787–792).** class: design
- Wrong: "every oracle changes exit status" but nothing says what an oracle returns when it can't evaluate (freeform distance query out of budget, missing trim domain, undefined bound per R2.7). Today reads as "unqualified" not failed.
- Evidence: Map S25 open item "oracles need an indeterminate outcome distinct from pass and fail"; ALPHA_QUALIFICATION "Skipped/not-run/xfail … cannot count as passes".
- Fix: outcome is pass, fail, or not_evaluated with reason; not_evaluated is non-green for exit status; arena counts it separately.

**R2.4 O-bytes fails with no bug on the default configuration (§24.2 L2283–2288; §20.4 L1942–1950; §23.2 F8 L2153).** class: design
- Wrong: shipped lane mixes CPU and GPU with load-reactive placement; bytes = f(input, placement log) per §4.2 L370. O-bytes across thread counts, injected delays, GPU0 vs GPU1 will differ with no bug; also cold vs warm because sticky placement (§20.4) changes the log. No oracle checks replay-of-log reproduces bytes; none checks functional equivalence between two mixed runs.
- Evidence: G12 §5.1 E3 and §5.4 perturbation "for mixed runs, the placement seeds of G7 H3"; G7 §3.3 (a comparison that fails with no bug is the wrong oracle).
- Fix: O-bytes on single-backend configurations. Replay arm: default mixed config with recorded placement log forced, under every perturbation, must give equal bytes. Functional-equivalence checks between mixed runs with different placement seeds. (Coordinator note: R6.4 proposes trimming FI-3..FI-5; reconcile — operator .53 = functional equivalence.)

**R2.5 Negative controls named as known-bad inputs, but output oracles need known-bad outputs (§24 L2257–2258).** class: design
- Wrong: a correct mesher refuses bad input, never emits a bad mesh; bad inputs never exercise the fail branch of oracles 1, 2, 3, 5, 6, 7. Mutation testing deferred by operator; until then nothing shows these oracles can fail. Dropped lane controls: G16 §3.4 (swap two face ranges, shift one t by 10ε, drop a collapse record); G3 §4.5 CGAL snap-rounding fixtures; G4 H2 K1 with tilt rule off must produce forbidden pairs; mesher-test-plans-meshgen §397, §407 CGAL meshes/degeneracies (16 files) and Alpha_wrap_3 defect set (17 files) ("otherwise a buggy watertight checker passes everything").
- Fix: a negative-control column per oracle row naming defective output artefacts with exact expected verdicts, run in the same pass (keeps no-separate-tests rule). Include NVIDIA's recorded invalid outputs: void-block in 8 configurations, the 1808 duplicate-triangle class, the inside-out fillet class. Include a vertex pushed 1.01ε along the normal.

**R2.6 Self-intersection oracle trusts producer's exemption list; c_floor settled in a circle (§3.7 L265–266; §12.5 L1399–1402; §24.1 row 5; §27 #17 L2417).** class: design
- Wrong: oracle 5 "exempts only reported source pairs", producer writes the report; a wrongly reported pair hides a real intersection. c_floor decides which pairs are reported, so decides exemptions; settling c_floor by running C1, T1, K3 under oracle 5 lets larger c_floor make it pass more.
- Evidence: §24 "never trusting a producer status"; G7 N6.
- Fix: for every exempt pair oracle 5 independently recomputes source clearance; fails if clearance > c_floor or mesh intersection lies farther than ε+μ from the source contact set. Settle c_floor by that source-side measurement.

**R2.7 Normal oracle false-fails on cylinder strips and doesn't check Rule B on most carriers (§24.1 row 6 L2269; §3.10 L285–288; §15.1 L1537–1546; §13.1 L1437).** class: design
- Wrong: "facet error ≤ arcsin(√3·R·κ)" drops G13 H2's precondition R·κ ≤ 1/√3. Cylinder strip triangle along a ruling has R ≈ L/2, κ = 1/r; for any strip longer than ~1.15r the bound is undefined or fails though true facet error ≈ Δθ/2. Rule B (each corner carries exact normal at its own UV) checked only by interpolation on planes/cylinders/spheres and by edge dihedrals in glTF; interior corner normals on cones, tori, revolution, B-splines never checked. G13 H5 per-corner check "within 1e-6 rad of exact corner normal" dropped. Limit normals at poles, apexes, collapsed edges have no oracle.
- Fix: state the precondition; use A0 (angle < 90°) as the check everywhere with closed-form strip bound on cylinders and cones. Add G13 H5 for every carrier at the oracle's own projection of the written position. One-sided limit-normal check at quotient vertices.

**R2.8 Render profile's angle promise, used in the headline, has no oracle (§6.2 L725–731; §22.2 L2047–2048; §27 #25).** class: design
- Wrong: θ_max on freeform enforced only by corner-normal spread measured at corners; no §24.1 row can fail on normal turning inside a triangle. Headline arm is where "both sides meet ε plus the 25° angle term"; neither side's angle term measured by an oracle.
- Evidence: G14 §6 needs "dense evaluation of normal turning per emitted triangle"; §27 #25 names it, §24 omits it.
- Fix: dense normal-turning oracle row (per triangle sampled with covering bound, plus curve turning per chain segment). Run on NVIDIA's arm before calling it matched (overlaps R5.5).

**R2.9 Oracle 1 threshold comes from the producer; checked artefact unnamed (§24.1 row 1 L2264; §3.3 L210–213; §3.4 L233–236; §18.1).** class: design
- Wrong: "beyond ε_loc": ε_loc is producer's per-triangle min of clearance and tilt budgets; undefined for target-to-mesh direction (target point has no triangle). Promise is "as written" and δ_exp depends on frame; 3MF writes world frame per occurrence while one key serves many occurrences. If oracle 1 checks FP64 at ε and oracle 7 checks displacement ≤ δ, a mesh at ε − δ/2 passes both and its written bytes exceed ε.
- Fix: thresholds from inputs only (ε_f, r_c for collapses); budgets left to oracle 5. Either check FP64 at ε_f − δ_exp of the largest frame that key is written in, or run oracle 1 on each written format in its own frame. (Related R7.5.)

**R2.10 Incremental oracle lost most of G12's oracle and contradicts itself (§24.3 L2303–2311; §24.1 row 13; §19.2 L1823–1824).** class: design
- Wrong: "must both hit and miss … or discarded as vacuous" conflicts with row 13 "fails when … a case does not both hit and miss"; both conflict with "pure renumbering must hit everywhere" (zero misses). Generator cut to perturbations of one input, mostly 1 ulp — never change closure split sets, collapse membership or clearance-region crossings, so R₂ halo and component couplings never exercised. Lost: halo check, Δ-channel = F-channel check, undo.
- Evidence: G12 §5.4 steps 5–6; edit-class table (topology, two-step spatial, collapse shorten/lengthen, sequence with undo, transform, scale); H3; H6.
- Fix: carry G12 §5.4 whole. Identity-only and transform-only classes expect zero T1–T9 misses; every other class must both hit and miss; a case breaking its class expectation fails, never discarded.

**R2.11 Volume oracle either can't fail alone or fails with no bug (§24.1 row 10 L2273; §7.4 L891).** class: design (agrees with R6.6)
- Wrong: G11 §5.3 derives |ΔV| ≤ 2εA from oracle 1 passing — fails only if oracle 1 wrong (test of a test; G7 N1) or with no bug where εκ not small. "Exact volume" doesn't say target or source; against source, reconciliation moves (not charged against ε) can break it. Profile lists B-ME-VP as soft goal but row 10 changes exit status.
- Fix: remove from status-changing stack; keep H-VP as recorded measurement against target with the curvature term.

**R2.12 No oracle can fail on over-refinement (§10.2 L1137–1139; §11.2 L1246–1248; §13.1).** class: design
- Wrong: every oracle one-sided; a mesher refining everything to depth limit passes. Promised bounds have no oracle: "at most 2× samples", §11.2 count/depth bounds, "strips of 2n triangles".
- Evidence: G14 §6 H-angle-cost falsifier "emitted counts differ from prediction on analytic faces".
- Fix: spec-tier row: on untrimmed analytic families (cylinder strips, sphere lattices, planes with no Steiner points) chain and triangle counts equal closed form. Optionally on families, check every split node's parent fails a predicate the oracle recomputes (pointwise N of §11.3).

**R2.13 Authored families never check they reached the branch they force (§23.5 L2219–2220).** class: design
- Wrong: each family states the branch it forces but nothing checks it fired (K1 never hits tilt budget, A7 never collapses → passes with nothing tested). O-bytes and incremental oracle are guarded against vacuity; families aren't.
- Evidence: G4 H5 ("0 forbidden pairs … is no evidence"); meshgen §8.
- Fix: each family names the record it expects in ACTIONS_ or FINDINGS (rung, collapse op, closure split, tilt refinement, refusal reason); family whose branch didn't fire fails.

**R2.14 Fuzzer has no oracle beyond timeout (§23.5 L2236–2239).** class: design
- Wrong: "timeout counts as failure" is the only tier; NaN/Inf injected at evaluator, projection, sizing outputs have no expected verdict though §1 L97–99 makes "never a silent accept" a design decision.
- Fix: every fuzz output passes full stack or carries refusal records for exactly the affected entities; no face whose computation read an injected value may be valid (oracle 15's check).

**R2.15 Tier labels wrong, hiding that stack has no spec-tier mesh oracle (§24.1 tier column L2262–2279).** class: design
- Wrong: rows 1, 2, 4, 15 are universally quantified predicates = property tier, not spec. Spec tier only in row 6 (three carriers), 8 (raw values), 9, 16. JSMNTL picks red tests "at the right oracle tier" (§29 L2522). R2.12 adds the missing spec row.
- Evidence: BLACKBIRD oracle hierarchy.
- Fix: relabel the four rows.

### Bead findings
- R2.b1 §15.3 L1580–1581 "≤ 1.25·ε_k" and "coarse samples ⊂ fine" have no oracle; oracle 3 never run on mixed per-face levels. LOD phase.
- R2.b2 §24.1 row 14 "chain sets identical across thread counts and shuffled face orders" is a subset of O-bytes; drop. — fixed (§24.1 row 14)
- R2.b3 §24.2 L2286 O-bytes "incremental against fresh" duplicates oracle 13; keep one. (= R6.b2) — fixed (§24.2)
- R2.b4 §24.1 row 8 "meshed boundary checked against raw master" states no tolerance (η or u_ctx).
- R2.b5 §24.1 row 8 sender validation properties "any mismatch" fails on sender errors .39 says to warn and comply on; record sender mismatches, fail only on our own property computation. — fixed (§24.1 row 8)
- R2.b6 §18.4 L1763 contact fit report called "an offline oracle" with no fail condition; call it a report or give it one. — fixed (§18.4)
- R2.b7 §27 #28 "drop r_trim if it never binds (oracle 1)" uses oracle silence as evidence; decide from derived magnitude of w̄. — fixed (§27 #28)
- R2.b8 §21.3 L2007 and oracle 16 vs §20.1 item 4 forbidding CDT on GPU (= R6.7). — fixed (§21.3, §24.1)
- R2.b9 §23.6 under O-bytes repeated processes give identical bytes; run expensive oracles once per distinct output digest.
- R2.b10 §24.1 row 9 "mating diameter D − 2δ_f" equality only if δ_f is achieved max sagitta of that face; else ≥. — fixed (§24.1 row 9)
- R2.b11 §24.2 FI-4/FI-5 c·u·S needs a derived c per kernel; M7 plan must carry each kernel's c or FI-5 undefined. (See R6.4.) — fixed (§24.2)
## Reviewer 3: performance and architecture

### Design findings

**R3.1 Ingress time unaccounted for; already loses the headline alone (§5.3 L539; §5.6 L646–663; §22.3 L2057–2060).** class: design + operator
- Wrong: new mesher consumes the adapter's source store directly; owned headline counts ingress from STEP bytes. §5.6 quotes only the bridge's 3.1 s for the watch. The adapter takes 25.0 s on OM10 vs ~17 s at HEAD; design never mentions it, splits it into phases, or budgets ingress.
- Evidence: B2-ad REPORT L15, L279 (25.0 s OM10). G1 §6: bridge read+transfer of watch 3.06 s; G1 L522 adapter source_uv work "comes on top and was not timed". B2-ad L133, L311 orientation contact test 1.5–24 s on single fixtures. B2-ha §1 NVIDIA ingress export + batch wall 10.17 s total. Voron 2.4 bridge read+transfer 37.1 s (G1 §6).
- Fix: measure adapter phase split on watch; name which phases the new path keeps (raw walk, placements, units) and which leave the headline path (§5.3 already says source_uv reconstruction not used for geometry); ingress budget ≤ NVIDIA's minimal ingress; say whether definitions stream from ingress into the graph or wait for the whole store; put owned reader (.25.7) on critical path to first headline.
- Operator: does "meshing performance first" (§1 L108) allow the owned reader to come before the first headline?

**R3.2 Speed target miscalculated; no stage has a time budget (§22.6 L2122–2127; §1 L78–84).** class: design (agrees with R5.14)
- Wrong: "847 s over 24 cores is 35 s … 5× to tie … 37× to match 22.9 CPU-s": treats wall as work (old run 1,578 CPU-s, 1.86 cores busy); uses 24 cores but shipped lane has 11 workers (§20.1); targets NVIDIA's pre-fix 6–8 s (face-count assignment, unpinned, incl. our work). Corrected: NVIDIA strongest-arm meshing makespan on a lane ≥ max(22.86/12, 1.995 s for definition 1713) ≈ 2.0 s; today's work on the lane 1,578/11 ≈ 143 s; per-unit cost must fall ~70× (§1 L79 itself says 69×). No per-stage CPU-s budget: T2, T7 clearance, T8 closure, T9 CDT with A0, ledger and residual detector (§11.4), T10, key digests; per-face fixed cost unbudgeted though 92% of watch faces analytic at ~30 triangles/face.
- Evidence: Audit §0 items 1–2, §2; SM §4 (1713 takes 1.995 s; busiest worker 2.75 s under face-count assignment); om10-nvidia-complete REPORT L3 marks 6.13/7.64 s pre-fix.
- Fix: restate target from steel-man lower bound; per-stage CPU-s and per-face budget for the watch at 0.005 mm, totalling ≲ 20 CPU-s for 11 workers; name counters checking each line (Audit §5 list).

**R3.3 GPU-eligible classes mostly unreachable; splitting a producer across devices undefined (§13.4 L1491–1492; §20.1 L1906–1910; §4.2–4.3; §19.1; §20.4).** class: design
- Wrong: "all clusters of one face stay on one backend" and GPU never runs bands or CDT, so any face with a non-empty trim band can't put lattice or cell classification on GPU — nearly every trimmed B-spline face. Seeded projection on GPU for the freeform occurrence of an edge whose other face is analytic splits one chain's production (L5, "one producer per physical edge") across devices. backend_id, cache key and placement log each carry one backend per task, so the composite case is undefined.
- Evidence: G7 §4 item 5 (source of one-backend rule); G8 §4.2 and PC C5 assume batched lattice work with bands on CPU.
- Fix: make the lattice interior (incl. rim vertices keyed by lattice coordinates) its own produced entity, band consumes it by key as faces consume chains; or define composite placement per (task, stage) in key and log and extend O-bytes and FI. Measure corpus share of freeform faces with empty band (only eligible set today).

**R3.4 Kept direct_cdt has measured pairwise many-hole cost on the planar path (§13.1 L1436; §13.2; §25 L2333; §29 L2517).** class: design
- Wrong: planes meshed as one CDT of the whole chain polygon; C2 component split applies only to lattice bands; direct_cdt validates hole containment pairwise; plate-like faces with many holes common.
- Evidence: direct-cdt REPORT: separated square holes, 8,104 sites, 13.18 s vs 0.23–0.34 s other 8,192-site families; docs/DIRECT_CDT.md L70 "hole containment validation still has a pairwise cost"; NVIDIA meshes the whole 438-face main plate in 1.995 s.
- Fix: loop nesting from topology plus closure Lemma 1 (§11.1); drop the pairwise check or replace with O(n log n) sweep; allow component/separator splitting on many-hole planar faces; require it in the plan, not "left to the builder".

**R3.5 Dependency graph incomplete; readiness rule contradicts it (§4.1 table L331–335; §4.5 L433–435; §10.3 L1162–1165; §3.4 L236; §12.3 L1379–1381; §11 L1209).** class: design
- Too strong: T8 reads only source-derived arcs (L333, L339; §11.3 "never from other edges' current chords"), yet the face waits two hops; one deferred edge blocks two-hop faces for nothing T8 reads.
- Too weak: T9 reads T6 and T7 slices; T7 reads μ of both faces of every spatial pair (from T5′, healer); counter covers neither. P4 uses ε_b = ε_loc − …, ε_loc includes clearance budget, so T8 needs T7; T8 row omits it. E3 "re-splits the shared chain through closure" reopens a frozen chain: feedback T7/T9 → T8 (= R6.2).
- Fix: derive readiness mechanically: one counter per task over its declared reads. Add T7→T8, or compute T7 on source geometry with μ folded in at T9. Drop two-hop rule if T8 stays source-only. Turn E3 UV rule into a P2/P3 predicate on source arcs. State resulting critical path.

**R3.6 Batching policy contradicts "no barrier waves" (L318–319 vs L916, L429–432, L1901–1902, L1982).** class: design
- Wrong: T2 "one batched stage" over all edges of all definitions = global barrier. Healer holds items until a SIMD/warp batch fills or immediate pass drains; with R3.5 spatial coupling every face near a deferred edge waits for the whole immediate pass, serialising the tail. Sorted job arrays and HeteroPrio batch allocation assume complete batches. G8 G1 assumes batches aggregated per stage generation = waves. Deferred-item counts never measured.
- Fix: batch-formation rule: per-class staging buffers; idle CPU worker takes any non-empty buffer (masked tails); GPU takes a buffer only above its own device's N*; healer item whose blocked downstream rank exceeds a threshold dispatches at once on CPU. Name every remaining global barrier (graph build after T1). Measure B-ED-CL counts early.

**R3.7 No plan for what lives on the GPU or how transfer volume is controlled (§20.1 items 4–6; §20.3 L1929; §4.6).** class: design
- Wrong: only staging buffers described; nothing says span tables, control nets or trim polygons stay device-resident; β per class undefined, so N* can't be computed per device.
- Evidence: Audit §0 item 6, §4 item 7: old regime uploaded ~2.7 KB per surface point and ~766 B per curve point; P100 changed nothing. G8 §4.1: 1M jets 6.1 ms resident vs 73–85 ms host to host. GPU0 5.7 GB/s.
- Fix: upload L2 span tables once per definition or run after T1, keep resident; trim polygons once per face for cell classification; batches send (span id, u, v) in, fixed-size results out. State β per GPU class and a per-lane device-memory plan within 16 GB with eviction.

**R3.8 SIMD layout doesn't fit AVX-only CPU (§21.1 L1981–1984).** class: design
- Wrong: jobs (item id, u, v) across spans → each lane loads a different control net → gather; Ivy Bridge AVX has no gather (VGATHERDPD is AVX2) and only 128-bit integer SIMD. FT §6.2 says sizing lanes are spans; design doesn't state the axis. One generic class up to degree 25 "padded to the class degree": a degree-4 span costs (26/5)² ≈ 27× its work.
- Fix: state lane axis per kernel: sizing/flatness lanes over spans, SoA across spans; evaluation lanes over points of one span, control points broadcast, jobs sorted by span; GPU a block per span, net in shared memory. Bucket generic degrees or evaluate them with lanes over points.

**R3.9 Memory plan not concrete; arenas fight NUMA and TLB (§20.5 L1955; §20.1 L1914; §4.6 L448).** class: design
- Wrong: per-task arenas each bound by mbind = one syscall per task over 10⁴–10⁶ tasks (G8 §6 recommended per-node arena sets, per-task scratch from worker's node). Host THP is madvise (§2), nothing requests huge pages; G8 §2.3 local latency 125–128 ns at 4 KiB pages incl. TLB misses. D1 outputs "SoA per definition" written concurrently by tasks with unknown output size (closure grows chains) — needs a barrier per definition or chunked storage; design says neither. Old regime 256 MiB scratch per thread, reservation peak 7.2–8.4 GB vs 2 GB RSS (Audit §4 item 8).
- Fix: per-worker scratch slabs bound once with MPOL_BIND and MADV_HUGEPAGE, bump-reset per task. Outputs in per-definition chunk lists, compacted at T10 by the exclusive scan. State expected peak bytes per triangle and per sample.

**R3.10 T7 clearance cost is the largest unknown; general fallback doesn't exist for analytic carriers (§12.3 L1366–1371; §12.5; §12.6 L1418–1420).** class: design
- Wrong: old analogue 19.5% of time, 64–67 µs per pair over 2.2M pairs (Audit §0.3). Closed forms cover only six analytic pairs; non-coaxial cylinder pairs (cross holes, pins), cone–cylinder, torus pairs fall to "span-pair box distance with Bézier convex hull", but native analytic carriers have no Bézier spans. Work-budget exhaustion → refusal turns a performance limit into an owned loss.
- Fix: measure E1 candidate counts and closed-form coverage on watch and corpus now with existing AABB pair-discovery path; add cheap generic analytic bounds (e.g. axis-to-axis distance minus radii); per-pair cost target next to G4 H4.

**R3.11 Cost-model calibration is hidden persistent state that changes cold runs (§20.3 L1931–1938; §22.3 L2065).** class: design (agrees with R5.9)
- Wrong: model persisted beside binary keyed by its digest → every rebuild starts uncalibrated, falls back to GPU-off, first process of each 10-process series differs. "Cold, empty caches" quietly depends on this file. Two concurrent lanes both write it.
- Fix: calibration an explicit pre-pass artifact recorded by digest in RUN_____; read-only during measured runs, one file per device; first-run defaults only outside measurement.

**R3.12 Worker count, driver core and pinning ambiguous, confounding F8 (§20.1 L1896–1897; §27 #23 L2421; §23.2 F8; §20.6 G0).** class: design
- Wrong: §20.1 11 workers ("minus the core"), §27 "12 workers". CPU-only arm and fallback don't say whether reserved core is returned; if not, F8 main effect includes a 1/12-core loss, same size as the G0 null effect tested. CUDA host sync mode unstated (spin-wait burns the hyperthread). Workers not stated pinned one per physical core, while NVIDIA's are (§22.2 L2037).
- Fix: CPU-only 12 pinned workers; GPU-on 11 pinned workers + driver thread with blocking/yield sync stated; gpu_driver_core arm measures the difference.

**R3.13 Keying/digest cost and whether cold headline writes the store unstated (§19.1 L1784–1812; §4.2 item 10; §22.3 L2065).** class: design
- Wrong: every task computes SHA-256 keys and output digests; tie-breaks need digests even with caching off. Ivy Bridge has no SHA extensions; owned SHA-256 (native/step_source/main.cpp:229–250) is scalar and copies its whole input before hashing. Whether cold headline writes the cache store (disk I/O in timed scope) unsaid.
- Fix: hash in place; budget digests in R3.2 table; write store off critical path after publication, or disable in cold headline and list separately.

### Bead findings
- R3.b1 §20.1 L1904 spoliating a running CPU batch needs item-level cancellation; §25 removed per-charge stop tokens, so a spoliated CPU worker keeps burning its core.
- R3.b2 §20.2 L1921 whole-host LPT pre-partition assumes symmetric sockets but GPU0 x8 and GPU1 x16 differ; weight each socket by its own calibrated capacity. — fixed (§20.2)
- R3.b3 §2 L160–164 --fmad=false halves P100 FP64 mul-add throughput for non-predicate GPU kernels; bead .25.2 should decide device side explicitly.
- R3.b4 §20.1 L1911 cudaHostAlloc pages follow the allocating thread's policy; specify cudaHostRegister on mbind'd memory or allocation under bound policy. — fixed (§20.1 item 7)
- R3.b5 §13.4 L1496 τ_split in seconds but estimates are integer units; state conversion through calibrated model and fallback when uncalibrated. — fixed (§13.4)
- R3.b6 §20.5 L1958 face-budget multiplier over its own estimate unspecified; a wrong estimate becomes an owned refusal. — fixed (§4.7, §20.5)
- R3.b7 §4.1 T1b / §8.8 algorithm and cost of definition-level lfs query unspecified.
- R3.b8 §11.4 per-face residual detector has no stated cost; include in R3.2 budget (see R6.1, which deletes it). — fixed (§11.4)
- R3.b9 §21.3 L2007–2008 "before any GPU triangulation result" names a nonexistent path (= R6.7); conformance applies to GPU cell classification. — fixed (§13.4, §21.3)
## Reviewer 4: robustness, pathology and termination

Reviewer summary: the "acyclic" graph contains two feedback loops into closure with no bound; most loop bounds are named but never given a formula; ingress refusals today exit 1 with text only, conflicting with §5.4 and §7.2.

### Design findings

**R4.1 No catalog of malformed-value refusals; refusals today are whole-file text exiting 1 (§1 L97; §5.2 L497–500; §5.4 L596–598; §7.2 L829).** class: design
- Wrong: §1 says non-finite/malformed values refused at ingress; §5.4 says every refusal carries a typed record with reason code; but no table of malformed-value checks; only geometric-validity row is torus R < 0.
- Evidence: native/step_source/main.cpp:217 throws runtime_error("nonfinite source scalar"); so do knot-edit refusals (341–380), periodic-pcurve no-match (955), shell/face count mismatch (665). Every throw exits 1 (1279–1280), which §7.2 reserves for tool failure. native/mesher/source.cpp:88 throws whole-file invalid_source. Map S4 asks ordered checks (syntax → topology → consistency → geometry). CAD test-plans paper finding 7 recommends usd-brep's 191-rule validator as a refusal catalog. Missing rows: radius ≤ 0 on cylinder/sphere/cone; cone semi-angle outside (0, π/2); zero or parallel frame axes; extrusion |V| = 0; ellipse b ≤ 0; weight ≤ 0; edge range t₀ ≥ t₁; degree > 25.
- Fix: catalog table (check, scope entity/file, reason code, consequence, status). Map every adapter throw to a row. File-level refusal exits 4; exit 1 only for internal, I/O and device faults.

**R4.2 The "acyclic" graph has two unbounded feedback loops into closure T8 (§4.1 L333/L339; §11.5 L1293–1296; §12.3 L1379–1381; §12.5 L1404; §4.5 L433).** class: design (overlaps R6.2, R3.5)
- Loop (a) T8 → T5′ → T8: new closure samples on deferred edges become deferred items; a rung move changes the arc and closure "re-runs from scratch"; no bound or fixed-point argument.
- Loop (b) T9 → T8: UV clearance budget "re-splits the shared chain through closure"; "tighten local ε" changes P4's ε_b which reads ε_loc and so the clearance budget; T8's input bundle omits T7; §11 says chain frozen after closure.
- Two-hop counter mismatch: G2 (L674) needed two-hop pending counters because closure read neighbours' resolved arcs; §4.1 now reads source arcs, so either the two-hop wait is dead weight, or D(s) misses neighbours' moves up to ¼ lfs and the residual detector refuses faces whose premises were met.
- Evidence: G4 L556 ("re-splits a shared edge chain, so it feeds the face-to-edge loop"); G2 L664–683.
- Fix: resolution is pointwise in t (stage-1 data plus L1/L2 only), so closure runs once on the final arc. A new closure sample needing a rung move beyond the closure margin refuses; never re-runs closure. Add T7 to T8 inputs; compute ε_loc once in closed form before closure. Budget failure after T9 is a refusal, not a re-split. Two-hop rule → one hop plus a D̲ margin equal to neighbours' max recorded move.

**R4.3 NaN and Inf after ingress have no rule (§8.1 L929; §4.2 L396; §4.6 L454; §23.5 L2236–2237).** class: design
- Tangent case: |x| = √(d₁²+d₂²−2d₁d₂cosθ)/sinθ is 0/0 at θ = 0 with d₁ = d₂ = 0 — the ordinary exactly-tangent fillet edge.
- Order-dependent reductions: per-edge max |x| and min θ depend on order once NaN present; std::max(NaN, a) = NaN but std::max(a, NaN) = a, breaking §4.2 item 6.
- Undeclared singular points: carrier record lists declared poles/apexes only; a freeform surface with ‖S_u×S_v‖ → 0 at an undeclared point gives a NaN exact normal into A0; native/om10/math.hpp:126,151 returns false there.
- Fuzzer injects NaN at evaluator, projection, sizing outputs, but nothing says what those become.
- Evidence: CAD test-plans finding 3 (OCCT PR #1598: five `value < bound` checks pass NaN; Gmsh HXT never breaks; truck fans to depth 100).
- Fix: every gate pass-iff (x ≤ bound) so NaN fails closed into Rule A. Finiteness check on each task output (T2–T5′, T9, T10); failure refuses with nonfinite_evaluation naming task and entity. Reject NaN before any min/max reduction. Define |x| for θ < θ_cut as the truncated minimum-norm move. Normal-magnitude floor: below it limit-normal route or refuse undeclared_singularity.

**R4.4 Termination bounds named but not derived (§13.3 L1481–1483; §8.5 L989; §8.1 L925; §12.3 L1368–1370; §15.1 L1547–1550; §20.5 L1958; §24.1 #15 L2278).** class: design
- No formula/value: quadtree depth, band refinement rounds, flip counts; A0 refinement loop and freeform corner-normal refinement; A1 cap repair by Steiner insertion or apex deletion; E2 "work budget"; face budget "derived from its own estimate"; rung-1 Newton "iteration bound from the geometry" (M2 L480–482, L746 never derive it); seeded projection's bound.
- Unmapped statuses: native/projection_solver.hpp:185–226 has iteration-limit, stagnation, line-search statuses; design maps none.
- F1 plain-double arm: with plain-double predicates Lawson flips and constraint recovery can cycle (meshgen paper L547, L589–590: Shewchuk's tilted-lattice non-termination).
- Map S9 agrees: "termination limits are named but not derived". Oracle 15 forces "each bound" but there is no list.
- Fix: termination table, one row per loop: bound formula with count backstop (e.g. Newton ≤ 8 steps + residual-decrease test; quadtree depth ≤ log₂(span / fixed-point quantum); band Steiner points ≤ c·area / min-area(ε_loc)); stop name; status (soft → valid_goal_unmet, hard → refused); record. Oracle 15 iterates the table.

**R4.5 Closed forms leave their domain; budgets go non-positive (§6.2 L721–723; §13.1 L1437/L1439; §9.1 L1062; §10.3 L1164; §6.1 L695).** class: design
- acos(1 − ε/R) NaN for ε > 2R; sphere ρ² = 2Rε_loc − ε_loc² negative there; request set includes 1e-3 of the diagonal (§23.4), so R < ε/2 fillets common.
- ε_loc − d_f ≤ 0 at a collapse survivor whenever clearance budget puts ε_loc below r_c (family A16: collapse beside thin wall) → refinement to floor and refuse, against §9 L1048 ("never refuses a face"). ε_b ≤ 0 makes P4's √ NaN.
- Relative ε over shape_extent's unbounded diagonal (B2-ad .37.39) is ∞. ε ≤ 0 or NaN not validated.
- Fix: clamp closed forms (ε ≥ 2R → Δθ = π with §10.4 floor). Check every budget subtraction > 0 before refinement (collapse checks positivity in §9.4 and reverts; clearance goes straight to §12.5 refusal). Validate requests up front; unbounded diagonal refuses by name.

**R4.6 What a refused face does to neighbours' tasks is undefined (§4.5 L443–444; §8.3 L951; §8.7 L1030; §24.1 #3 L2266).** class: design
- Wrong: unstated for refused face's L1 rows ("one row per incident surface"), closure arcs, clearance candidates, collapse clusters, L2 vertex stars. Refusal not stated as terminal releasing pending counters → every refusal could end "pending at end of run" = exit 1. Oracle 3 would flag free edges next to a refused face.
- Evidence: ingress carries refused faces "for topology, no consumer may mesh it" (main.cpp:910); D-partial-output.
- Fix: refusal terminal and releases dependents. Evaluable refused carrier still feeds neighbours' L1, closure, clearance so their bytes don't change. Non-evaluable carrier drops its rows with a record. Oracle 3 classifies these edges open_by_refusal.

**R4.7 Status for caps, budgets and unresolved checks inconsistent (§7.1 L805–808; §11.6 L1310–1311; §11.2 L1255; §20.5 L1958; §12.5 L1404; §7.3 STATUS__ L848).** class: design
- §7.1 clearance out of budget = indeterminate; §12.5 refuses it. §11.6 unresolved lfs query indeterminate, though D̲ gates splits and Rule A sends gating not_evaluated to refusal (§11.2 stop 2 refuses). §20.5 "refuses with budget" but budget is a not_evaluated reason.
- No per-format axis: status differs per format (.38, 3MF pair refusal §18.3 L1753, glTF > 2²⁴ faces L1693, §3.11 L299) but STATUS__ has no per-format row; exit code over several formats undefined.
- Fix: one outcome → status table keyed by (hard/soft, gating/non-gating), removing indeterminate exits Rule A forbids. STATUS__ rows per (subject, format) for export clauses. Exit code = max over requested formats.

**R4.8 Ingress orientation depends on source_uv, which the design retires (§5.2 L508–511; §5.3 L545/L554–558).** class: design
- Wrong: S1/S2 orientation verdicts use OCCT classifier and Gauss integration on source_uv's projected pcurves (main.cpp:666–675 sets solids.at(si)=prepared.raw_shape, used at 1070–1072). Design says source_uv not used for geometry and lifts its refusals (C/S residual 44, multi-wire 39) but doesn't say what decides orientation without it, or that source_uv's throws stay whole-file inside the adapter before the new mesher runs.
- Fix: either decide orientation per face from T2's projected UVs with owned Green's sums (reason 7 per face), or keep source_uv for orientation only and demote its throws to per-face refusals.

**R4.9 Bridge's OCCT work has no bound (§5.2; §5.6 L657–660).** class: design
- Wrong: contact test (BOPAlgo fuzzy section, BRepExtrema) and classifier run with no time/memory budget; "fails to complete" detected only by exception.
- Evidence: B2-ad contact tests 5–24 s per face in round 7, 1.5–4.4 s on ring-on-block; .37.27 overflowed the stack on mapped-item cycles (only that recursion guarded); OCCT dataset stratum (333 bug reproductions) will drive these paths.
- Fix: CPU and memory rlimits on adapter process, refusing as ingress_budget (exit 4), plus per-face budget on contact test (reason 7).

**R4.10 Export-frame construction has no failure path (§18.1 L1716–1728; §16.2 L1621; §14 L1520; §13.2 L1466; §15.1 L1549).** class: design (overlaps R7.5, R2.9)
- No needle pass: C1 "enforced by the quality pass; refine or collapse needles" but no such pass exists (post-processing defaults off; bands never insert on chains; A1 never splits a chain). A needle whose longest edge is a chain segment has no remedy/outcome. G3 L682 assumed the draft's quality pass.
- Per-occurrence frames: 3MF stores world-frame values per occurrence while meshing is per key; C1–C3 can pass for one occurrence and fail for another, no consequence defined.
- ε − δ_exp negative at +100 m (δ = 0.0068 mm > ε = 0.005 mm, "100 m offsets" family).
- Fix: δ_exp per key = max over requested occurrences and formats, recorded in key. C1 a T9 predicate. Chain-bounded needle with no interior fix refuses that format for the part with a record. ε ≤ δ_exp → recentre (STL) or format refusal before meshing.

**R4.11 Closure's stop rule breaks at cusps (§11.1 L1222/L1233–1237; §11.2 L1243).** class: design
- Wrong: at α_v = 0, P3 needs w̄ < dist·sin 0 = 0, never passes. L* gets an α_v/(4κ̄) = 0 term, so L*_min = 0 and stops 3, 4 vacuous.
- Evidence: G2 §2.3 (L525–541) exempts the innermost segment and grades from ℓ_first; design doesn't carry it; "Δκ measurable above the floor" has no number.
- Fix: P3 waived at cusps (measured |Δκ| above floor); innermost segment exempt; L*(r) = r√(2|Δκ|/κ̄) for r ≥ ℓ_first with count bound in log(ℓ_edge/ℓ_first); Δκ floor = k × evaluation error of κ.

**R4.12 No cancellation or deadline mechanism (§25 L2327; §19.5 L1879–1881; §7.2).** class: design (mechanism) + operator (new exit code, .40)
- Wrong: per-charge stop-token check removed, nothing replaces it, yet ErrorCode::cancelled exists (native/mesher/values.hpp:16); per-frame hairspring remeshing and sharpen-in-place produce superseding requests; fuzzer "timeout" has no threshold.
- Fix: cooperative cancellation checked at task dequeue, `cancelled` run status with records of what completed; completed pure results may be cached; fuzzer timeout = k × integer work estimate.

**R4.13 Cache hits not verified (§19.1 L1809).** class: design
- Wrong: output-digest → bytes hit served without checking bytes; dependencies enter keys by output digest, so a torn/corrupt entry propagates into every downstream key as silent accept. Concurrent writers (whole-host, two lanes) and corrupt persisted cost model (§20.3) unaddressed.
- Fix: re-hash on read (mismatch = miss + record); write to temp file and rename atomically; validate cost model finite and positive, fall back otherwise.

**R4.14 One device fault fails the whole run (§7.2 L829; §20.1 L1901–1904; §20.3 L1935–1938).** class: design
- Wrong: mid-run device fault (allocation, launch, ECC) → exit 1, though spoliation already drops and replaces batch results and fallback covers calibration only.
- Fix: re-place failed batch's items on CPU and keep that placement for the rest of the run, with device_fallback record; outcomes functionally equivalent (.53); exit 1 only if CPU path also fails.

**R4.15 Refusing equal-tier near-tangent pairs (§8.7 L1027–1031 vs §3.11 L298, §9.2 L1075).** class: design
- Wrong: two near-coplanar planes (or same-tier analytic surfaces) with a gap in (η, ¼ lfs) refuse every face using the item, at η = ε/16 = 0.31 µm. §3.11 reserves refusal for what can't be meshed; §9.2 already applies symmetric tangent_split when tiers equal. Frequency unmeasured.
- Fix: count the class on the corpus; default to the symmetric recorded step reported in b_f, keep refusal as arena arm. If designer keeps M2 §5.6's refusal, it goes to the operator.

### Bead findings
- R4.b1 §11.2 L1253 "doubles bound depth by about 52" wrong near t = 0, where subnormal midpoints allow depth ~1075; real bound is CDT fixed-point floor; dyadic path width must cover it. — fixed (§11.2)
- R4.b2 §5.4 no row for B-spline degree > 25 (degree_above_reader_limit, G5 §5). — fixed (§5.8)
- R4.b3 §4.2 item 10 and §10.4 digest tie-breaks undefined for exactly duplicate entities; add coincident_source_entities refusal (coordinator refuses these today). — fixed (§5.8)
- R4.b4 §16 NAUO-path expansion of a shared-subassembly DAG grows exponentially; count paths by DP before expanding, with refusal (old limit at source.cpp:802; .37.32 removed depth bound). — fixed (§5.8, §4.7)
- R4.b5 §8.5 rung 4: bound alternatives by edge's representation count; state rung 4 doesn't recurse. — fixed (§8.5)
- R4.b6 §11.2, §11.6 M̄ never defined in §11. — fixed (§11.2)
- R4.b7 UR-7 and §8.1 "within vertex uncertainty" coincidence decisions have no rule when u_ctx absent or unusable.
- R4.b8 §10.4 and §9 a closed edge below δ_exp or below the CDT quantum whose collapse checks fail has no terminal outcome.
- R4.b9 §7.1 Rule A: say "the gate's conservative branch", which for collapse is revert, not refuse. — fixed (§7.1)
- R4.b10 §20.5 say whether exit 1 on run exhaustion still writes the records (should; meshes should not). — fixed (§7.2, §20.5)
## Reviewer 5: measurement honesty

### Design findings

**R5.1 Our oracle failures are scored as NVIDIA's (§22.1 L2025–2026; §22.5 L2108–2113).** class: design + code bead
- Wrong: NVIDIA "valid" has only valid/invalid; no "our check could not decide". Harness puts that in NVIDIA's column.
- Evidence: scripts/qualify_om10_nvidia.py:196–202 adds every definition whose distance query did not complete to `failed`, status FAIL (covers SOURCE_QUERY_SOLVER_FAILURE and preparation exceptions, both ours). scripts/benchmark_om10_nvidia.py:417–423 and :505–506 record the arm FAIL/failed_qualification; :490 maps any non-zero qualifier exit, exceptions included, to FAIL; selected_outcomes then reports "no valid arm". Live: results/om10-nvidia-complete-2026-10-17/REPORT.md:17–20 records query tool refusing definition 1808 ("face-edge has no usable trimmed pcurve"), "not a measured NVIDIA distance failure".
- Fix: NVIDIA verdict gets §7.1 axes: pass, fail (completed check found fault in NVIDIA's mesh), not_evaluated (ours: exit 5, input excluded and listed, never "no valid arm"). Add §22.5 row.

**R5.2 Nothing stops the owned headline using its best configuration per input (§22.3 L2058–2060; §22.5; §23.3 L2190–2191).** class: design
- Wrong: NVIDIA gets fastest valid arm per input (L2042, steel-man). Design never says owned side is one configuration fixed in advance. With 64 arena configs plus follow-ups, best-of-arena per input is selection bias in our favour. Defaults chosen from arena data then measured on same runs/inputs are optimistically biased.
- Evidence: §20.1 "the shipped configuration that must win" never tied to headline; §23.3 arena defaults provisional; G8 paper §7.6 controls bias only within run order.
- Fix: owned headline is one declared configuration (flag set, binary digest, calibration digest), same for every input in a stratum, recorded before the headline series. Defaults adopted on arena data, confirmed on fresh processes, on held-out inputs where stratum is large enough. Never select per input on owned side.

**R5.3 No rule for what counts as a win; no uncertainty for the corpus claim (§22.4 item 1 L2075–2078; §22.5 L2106–2118).** class: design
- Wrong: G8 paper §7.5 decision rule dropped (compare non-overlapping CIs or "not distinguishable"). Real-input strata from 3 processes per input: rank interval at n=3 is min–max at 75% coverage, no 95% CI. Sum-of-costs ratio dominated by largest inputs, hides losses on small ones (Audit §2: worst ratios on small definitions, median 32.5×, max 279× on a 5-face part). §22.5 has no row for censored runs (harness timeouts either side); AGENTS.md records a 180 s NVIDIA censor.
- Fix: claim per stratum = ratio of summed paired costs over inputs where both valid, bootstrap CI resampling inputs and runs. Report fraction of inputs won with CI and per-input ratio distribution. Win = CI wholly above 1, else "not distinguishable". Owned losses and coverage counted separately and can block a stratum win. Censored row: censored on our budget is neither NVIDIA-invalid nor our win; record budget.

**R5.4 NVIDIA charged ingress + makespan back to back while owned graph overlaps; two stronger NVIDIA arms missing (§22.3 L2055–2060; §22.2 Assignment row L2038; L2063–2064).** class: design
- Wrong: owned overlaps decode with meshing (§4.1 L318–319) and CUDA context creation with ingress (§20.1 item 5); NVIDIA headline is ingress process wall + worker makespan, no overlap. NVIDIA assignment static LPT (examples/render_om10_nvidia.py:247–258); dynamic pull from calibrated-sorted queue strictly stronger under noise; owned side gets work stealing. "Meshing-stage ratio" (L2064) undefined for an owned global task graph with no stage boundaries. Owned headline end point and process start/teardown/exit never stated, though NVIDIA is charged all three.
- Fix: NVIDIA arms for streamed start (workers take each .brep as it lands) and dynamic LPT pull; until then report NVIDIA's ideal-overlap bound beside headline. Owned stage cost = CPU-seconds per task class plus a separate meshing-only run on decoded source for the stage ratio. Owned headline runs fork to exit, includes context creation and teardown, ends when every key's L6/L7 output is resident.

**R5.5 Design and harness contradict on whether deviation affects validity; "matched fidelity" unproven (§22.2 L2042–2048; §22.5 L2116–2117; §6.2 L712).** class: operator, then design
- Wrong: design says NVIDIA deviation "reported … never used to slow it down", validity topology only. Harness finite mode makes geometry quality a validity condition (benchmark_om10_nvidia.py:87–99 "NVIDIA's time counts only when its finite geometry quality is PASS"), letting a slower arm be "fastest valid". §6.2 "quality must match" contradicts both. Headline called "matched fidelity" but SmTess study §3 shows NVIDIA's chord estimate understates deviation (sphere witnesses 0.01 → 0.0145); its 25° angle semantics differ from owned normal-turning rule and were never studied (every SM mechanism "live at 0.005 mm, angles 0"). Audit records owned emitting 2–4× NVIDIA's triangles on small curved parts.
- Fix: one rule; recommended validity = topology + finite points. Rename headline "matched request". Report NVIDIA measured δ and triangle count with every ratio. Labelled side comparison (never headline) with NVIDIA chord tightened until its measured deviation meets ε. Align harness finite mode.
- Operator: does "matched" mean request or achieved?

**R5.6 Lane pairing for NVIDIA unspecified; its two-socket arm doesn't exist (§22.4 items 7–8 L2097–2104; §20.2 L1925; §23.2 F8).** class: design
- Wrong: owned runs both lanes; NVIDIA "on the same socket" but no NVIDIA reference set required per lane. Journal fairness note asks ≥2 reps per lane; harness runs one lane per campaign (--lane-node, default 0). Nothing stops quoting the better lane. Arena F8 (GPU on) effect would pool GPU0 (x8) and GPU1 (x16) whole-run times that carry transfer cost — the cross-GPU comparison .68 forbids. Whole-host mode "compared only with NVIDIA's two-socket arm", undefined in §22.2 and harness can't run it (G8 paper §7.11: 24 or 48 workers across both sockets).
- Fix: NVIDIA measured on each lane; each owned lane ratio paired with same-lane NVIDIA, both reported, never pick one. Arena GPU-on effects estimated and reported per lane, lane as block, no pooled estimate. Add two-socket NVIDIA arm to §22.2 or drop whole-host comparison.

**R5.7 Quiet gate ignores the lane, contradicts itself, penalises NVIDIA's pinned arm (§22.4 items 3–4 L2081–2087).** class: design
- Wrong: foreign CPU ≤ 150% measured host-wide (scripts/measurement_lane.py:332); that much on the lane's own 12 cores costs a 12-worker run up to ~12%, on the other socket ~nothing. Same load hits NVIDIA's statically partitioned pinned workers harder than owned work stealing. Item 4 gates on 1-minute load ≤ 4.5 then says classification uses foreign CPU only; code records load average but doesn't use it (measurement_lane.py:44–49). Two lanes run at once; the other lane's run either fails the gate as foreign or is hidden as own tree; its state never recorded or matched.
- Fix: gate on foreign CPU on lane's CPUs and SMT siblings from per-CPU /proc/stat, tight bound. Record host-wide load as covariate. Remove load-average clause. Other-lane activity a recorded column and blocking factor; owned/NVIDIA headline pair shares the same other-lane state.

**R5.8 CPU frequency and GPU link state not recorded during the timed interval (§22.4 item 5 L2088–2093).** class: design
- Wrong: delivered clock probed only before/after, outside interval; under schedutil idle-adjacent probe doesn't represent run (G8 §2.5: 3.06 GHz 1 thread, 2.53 GHz 24). GPU SM clock, throttle reasons, current PCIe gen/width not in row (G8 §2.5 caught GPU0 at 987 MHz; links drop gen at idle). Without current link width per row, .68 can't be enforced mechanically.
- Fix: record cycles/wall per CPU across timed interval (APERF/MPERF or perf_event cycles), keep probe as cross-check. Sample GPU SM clock, throttle reasons, current link gen/width during run. Reporting tool refuses any table placing transfer-bearing figures from different link widths side by side.

**R5.9 "Fresh process" runs share state (§22.3 L2065–2069; §20.3 L1931–1933; §19.1 L1809–1810).** class: design
- Wrong: cost model recalibrated from previous run's per-task timings persisted beside the binary; each of ≥10 headline processes places work from its predecessor — serially dependent, drifting. On-disk store not stated empty/disabled for cold runs. NVIDIA's LPT calibration comes from one separate pass.
- Fix: freeze calibration file per measurement series, read-only, digest in RUN_____. Cold runs use an empty cache directory, verified and recorded per row. Calibration from its own pass, as for NVIDIA.

**R5.10 Arena sampling plan can't run as written; no default-adoption rule (§22.4 item 1; §23.2; §23.4 L2207–2215; §23.6 L2246–2248; §29 L2521).** class: design
- Wrong: 64 configs × 2 lanes × 3 processes × 4 requests × strata doesn't fit in "hours"; leaves a handful of builder-chosen inputs (§29) against never-pooled strata. Saturated 2^(8−2) fraction (8 main + 28 two-way) across strata, lanes, requests and outcomes will throw spurious "intriguing" effects, each followed with no time box. No response metric, no adoption threshold.
- Fix: replicate across inputs not processes: n=1 per (config, input, lane), inputs as blocks. Fixed seeded per-stratum sample sized from time budget, recorded before any result. Pre-name primary response (log of stratum cost sum) and correctness gates. Adopt a default only if effect CI excludes zero and a fresh confirmation at ≥10 processes reproduces it; everything else exploratory.

**R5.11 No procedure for NVIDIA's strongest arm on the corpus; harness only handles the watch (§22.2 L2029–2042).** class: design + code bead
- Wrong: arm axes ~64 per angle arm (ingress 2 × prep 2 × heal 2 × workers 2 × placement 2 × path 2), per input, at ≥3–10 runs, with calibration each. Harness hard-codes the watch: inventory 107/9162/166 (benchmark_om10_nvidia.py:418–420, 461, 474), chord .005 (:258, :324); can't take per-key chords ε/σ_max. Taking min over many noisy n=1 arms (fastest_valid :102–111) then re-measuring is never stated; bias toward NVIDIA acceptable as steel-man but must be declared.
- Fix: screening protocol: arms at n=1 on a seeded calibration subset per stratum, then top-k arms per input at full n with fresh runs; NVIDIA time = best arm by median; state bias favours NVIDIA. Generalise harness to arbitrary inventories and per-key chords before any corpus speed claim.

**R5.12 Owned and NVIDIA runs not interleaved in one campaign (§22.4 item 2 L2079–2080).** class: design
- Wrong: "configurations interleaved" applies within each harness; owned (measurement_lane.py run) and NVIDIA (benchmark_om10_nvidia.py) are separate campaigns; host drift lands on one side.
- Fix: headline pairs run as one seeded schedule per lane, alternating owned and NVIDIA within blocks, same gate, same other-lane state. Compatible with journal's "canonical pinned NVIDIA run set".

**R5.13 Warm results have no NVIDIA counterpart (§22.3 L2067–2069).** class: design
- Wrong: owned warm keeps retained caches, possibly mesh-key hits; NVIDIA has no warm arm; nothing forbids pairing owned warm with NVIDIA cold or counting cache hits as meshing.
- Fix: NVIDIA warm = second query in same long-lived workers, ingress done. Warm vs warm only. Exclude output-cache hits from warm meshing ratios. List retained caches both sides.

**R5.14 §22.6 target arithmetic contradicts §1 and uses pre-fix NVIDIA times (§22.6 L2122–2127 vs §1 L78–83).** class: design
- Wrong: "847 s over 24 cores is 35 s" spreads a 12-core wall over 24 SMT threads; §1 (Audit §2) gives 70–131 s on 12 cores. "NVIDIA's 6–8 s" pre-fix (n=1, face-count assignment, unpinned; om10-nvidia-complete REPORT L3), excludes NVIDIA ingress; B2-ha §1 gives 10.17 s with it. Text copied from Map L7 arithmetic (Map L867), superseded by Audit.
- Fix: restate targets in CPU-seconds and 12-core wall; NVIDIA ingress-plus-batch scope; label every NVIDIA figure pre-fix and n=1 until fixed harness has run.

**R5.15 Steel-man = NVIDIA's shipped binary or strongest build from source? (§22.2).** class: operator
- Wrong: SmTess study §4 shows shipped build compiles out NVIDIA's own parallel paths (SM_USE_TBB never defined; TBB loops at SmTess.cpp:11256, :11436) and its scalable allocator (SM_USE_TBB_SCALABLE). Source in quarantine. "Strongest arm" could include NVIDIA build options or an allocator arm on shipped binary.
- Fix: operator decides; record in §22.2 as named axis or named exclusion.

### Bead findings
- R5.b1 §22.4 item 5 says 50 ms probe; measurement_lane.py CLOCK_MILLISECONDS = 100. Align. — fixed (§22.4 item 5)
- R5.b2 posix_fadvise does nothing on tmpfs (B2-ha §8). Record filesystem type of STEP file and .brep intermediates; refuse tmpfs for cold runs. — fixed (§22.3, §22.4 item 5)
- R5.b3 ~1 ms Python pre-fork cost charged to NVIDIA (B2-ha §8). Move the fork timestamp.
- R5.b4 Our own writeback/softirq time counts as foreign load (measurement_lane.py Recorder docstring); can mark our runs loaded unevenly.
- R5.b5 Foreign GPU processes checked only before/after a run; sample during. — fixed (§22.4 item 4)
- R5.b6 measure_quietly allows ≤3 tries per estimate; for ≥10-process series record discarded loaded runs per side and per run length.
- R5.b7 n=10 rank interval (ranks 2–9) has ~97.9% coverage; label actual coverage not "95%". — fixed (§22.4 item 1)
- R5.b8 §20.6 L1966 "break-even about twice that on GPU0" is a cross-GPU transfer comparison (.68); state each device's figure alone. — fixed (§20.6)
- R5.b9 Code side of R5.1: qualify_om10_nvidia.py:196–202, benchmark_om10_nvidia.py:490, 505–506 map our query incompleteness to NVIDIA FAIL. (Live violation of standing rule "our failures never scored as NVIDIA's" — fix-all-bugs applies.)
- R5.b10 Code side of R5.11: benchmark_om10_nvidia.py:258, 418–420, 461, 474 hard-code watch inventory and chord.
## Reviewer 6: doctrine and ceremony

### Design findings

**R6.1 §11.4 residual detector (L1284–1289; §11.3 L1280–1282; §27 #26 L2424).** class: design
- Wrong: post-triangulation hot-path pass per face checking chord crossings, loop containment, 3D folds, boundary-triangle ε. Each is already a construction rule (Lemma 1 P1–P3 for crossings/containment; A0 on every new triangle §13.3 for folds; ε ledger §3.4). Second validation layer, per-facet hot-path check; on analytic faces it restates closed-form construction; on freeform it backstops an unsound estimate (sampled κ̄ with 2× margin).
- Evidence: BLACKBIRD tenet 5 ("replace it with a better one; do not build a second-order validation system around it"); operator excision list in 2026-10-17 journal names "per-facet proof obligations, exact self-intersection checks in the hot path"; G2 paper F13 is a negative control built to make the detector fire (test for a test); G2 L716, L780 make the in-product detector the pass oracle for closure families.
- Fix: delete detector from construction. On freeform carriers either derive the σ̲(DS) bound now or declare the 2×-margin estimate the construction rule. Offline oracle 2 (exact UV) and oracle 5 catch estimate failures. G2 H2 counts come from oracle 2 on corpus and families.

**R6.2 §12.3 E1(d) (L1364–1365) and E3 "in UV" bullet (L1379–1381).** class: design
- Wrong: "chord deviation below half the UV clearance to other loops and tilt below half the UV angle at loop junctions" restates closure predicates P2 (w̄ < ½D) and P3 (corner rule) from §11.1 L1220–1225. "A failure re-splits the shared chain through closure" adds a T7→T8 feedback edge, contradicting §11 policy L1207–1211 ("chain frozen after closure … needs no count") and §4.1 acyclic graph L339–340.
- Evidence: §11.1 L1219–1226, Lemma 1 L1228; G2 settle-and-protect.
- Fix: delete UV parts of E1(d) and E3; point to §11 P2/P3 as the one UV mechanism; keep only 3D clearance budgets in §12.

**R6.3 §7.3–§7.4 data-quality profile run on every input (L851–855, 867–868, 870–899).** class: design + operator
- Wrong: ~20 JAMA/CAx-IF criteria evaluated twice (P1 and P2), including G-SH-IS, G-LO-IS, G-FA-IS (L886) = full source self-intersection check every run. Most rows' action is "report"; no construction step reads them. P2 rerun re-measures what the rung-0 commit already guaranteed. `brepgpu report` emits Part 59 fields and Quality Stamp XML nobody requested. Cost unmeasured; §22.3 (L2055–2062) doesn't say whether inside owned headline. Criterion measures (G-FA-NA, G-SU-DC…) not defined in design.
- Evidence: G11 H5 "P2 re-measurement, same code path as P1" (receipt on the commit rule); G11 §8 "cost of P1 + P2 double measurement … not in any source"; journal "full self-intersection check stays an offline oracle"; D-formats lists STL, 3MF, glTF, later USD and AP242, not Quality Stamp.
- Fix: keep D-profile as data. Default-evaluate only criteria the mesher already computes (d_f, |x|, θ from T2; short edges; free/non-manifold edges from use counts; degree). Move self-intersection classes and P2 rerun to offline P5 or on request. Drop Part 59 / Quality Stamp emission until a consumer asks. State record emission is timed inside the owned headline. Define each runtime measure.
- Operator question: does D-profile require input self-intersection classes at runtime?

**R6.4 §24.2 FI-2..FI-5 (L2289–2295); oracle 12 (L2275).** class: design
- Wrong: FI-4/FI-5 require cross-backend positions within each kernel's forward-error bound c·u·S — needs a written forward-error analysis per kernel (proof machinery) and enforces near-identity, which operator said is not a goal. FI-3 repeats oracle 3, which already runs per backend. Oracle 12 fails on "an unattributable difference" — not a predicate.
- Evidence: journal "Functionally identical is the bar: each backend meets tolerance and the topology guarantees"; .53 answer.
- Fix: functional equivalence = each backend passes oracles 1–10 and 15 on its own; FI-1 accounting; FI-2 status/class agreement with a difference excused only when higher-precision recomputation puts the deciding quantity on its threshold. Drop FI-3, FI-4, FI-5 and per-kernel forward-error bounds.

**R6.5 §7.1/§7.3 per-clause status bits (L787–796, L848 STATUS__ 2 bits per clause).** class: design
- Wrong: hard clauses (deviation, topology, orientation, embedding) are carried by construction; nothing at runtime evaluates them independently, so a "pass" bit records that construction ran — a receipt. Rung fact stored three times: chain per sample (L457), MEASURE_ per edge (L845), ACTIONS_ "M2 rungs 0–5" (L847, L856).
- Evidence: G11 §3.2 (~L190) "construction carries the hard clauses"; §24 L2257 oracles never trust a producer status; tenet 5.
- Fix: derive face status from construction events only (refusal with reason, cap/budget fired, fallback rule used). Keep lattice and .40 exit codes. Drop per-clause pass bits and FINDINGS rows for hard clauses. Store rung data once (chain); ACTIONS_ rows only for rungs 1–5.

**R6.6 Oracle 10 and profile row B-ME-VP (L2273, L891).** class: design
- Wrong: mesh volume within 2εA follows from oracle 1's two-sided ε bound; fails only when oracle 1 fails or is buggy — a check on oracle 1 in the same fault class.
- Evidence: G11 §5.3 derives 2εA(1+O(εκ)) from the deviation bound; tenet 5.
- Fix: delete oracle 10 and B-ME-VP goal. Keep B-VP-SV (sender properties, .39).

**R6.7 §21.3 L2007–2008 and oracle 16 (L2279).** class: design
- Wrong: "per-device predicate conformance checked before any GPU triangulation result is believed", but §20.1 item 4 (L1906–1910) never sends bands, CDT or topology to GPU; lattice emission uses no orientation/in-circle predicate. Oracle cannot fail on any shipped path.
- Fix: delete both; add the oracle with any future arm running a predicate on device.

**R6.8 §23.3 arms with no arena question (L2181–2188; §23.2 L2158–2160).** class: design
- Wrong: `topology_change` is a contract option (.47) — changes what output promises, can't be scored vs default. `mirror_3mf` settled by slicing both variants (§27 #21), not arena; corpus frequency zero (G9: no mirror in 500 corpus STEP files). `directed_rounding_3mf` cites only its own §18.3 [call], no hypothesis. `simd` both excluded from factorial ("microbenchmark pair") and listed as arm; implies a maintained scalar product path only to be measured.
- Evidence: journal: arena is for "contested choices the literature does not settle"; G9 census; §27 #21.
- Fix: move topology_change to user options; mirror_3mf a one-off consumer check in placements unit; drop directed_rounding_3mf until 3MF pair refusals are seen on corpus; simd a kernel microbenchmark only, no scalar product path.

**R6.9 Owned transcendentals §4.2 (L379–383); §27 #24 (L2422).** class: design
- Wrong: only claimed benefit is one CPU build sharing a cache across hosts, but §19.1 (L1811–1812) keeps cross-backend reuse off by default and backend_id already includes libm identity and IFUNC level. Owning acos/asin/sin/cos/atan2 buys nothing by default; the "required vs tidier" probe serves no question.
- Evidence: §19.1; backend_id L373–377; D-determinism only requires identity on one backend.
- Fix: use libm with identity in backend_id; revisit if cross-host cache sharing becomes a requirement.

**R6.10 Margin m means two things: §12.2 C1 (L1350) vs §12.3 (L1384).** class: design
- Wrong: C1 subtracts m as an angle ("≤ ½ sin Δψ_min − m"); E3 fixes "(1 − m) = 0.9" as a factor. Together m = 0.1 rad → C1 budget negative for Δψ_min below ~11.5°; every K1 crescent 0.5°–11° would refuse.
- Evidence: G4 L394 "margin m > 0" only; G4 L552 uses (1 − m).
- Fix: two named symbols, angular margin m_α and clearance factor (1 − m_c), each with value and basis.

**R6.11 §6.4 last bullet (L777–778) and oracle 9 "values against NIST drawings" (L2272).** class: design
- Wrong: reporting per PMI feature of size the mating size, two-point range and least-squares size needs feature recognition and fitting — a metrology report beyond "warn and comply"; no consumer reads it. Oracle 9 vs drawings means hand-transcribing PDFs.
- Evidence: D-PMI "read PMI to warn when ε is loose … Warn and comply."
- Fix: report m_t, c_t and the four warnings only; oracle 9 keeps count, metamorphic and nominal-on-limit checks.

**R6.12 Rational sizing rule not plannable: §10.3 L1158–1160, §13.1 L1450.** class: design
- Wrong: "D·δ² ≤ 8ε·min w" never defines D; δ already defined in symbol table (L65) as float32 rounding move.
- Evidence: fast-tessellation literature paper L148: D = |Δ²(wP)| + (r − ε)|Δ²w|.
- Fix: write the rule in full with its own symbols in §10.3 and §13.1.

### Bead findings
- R6.b1 §20.3 L1932–1933 (persisted cost model), §20.4 L1945–1946 (placement log), §23.6 and §22.4 item 5 (per-run/context rows) don't name a format. Name the tagged-table binary container; harness writes TSV today (scopes.tsv). — fixed (§20.3, §20.4, §23.6)
- R6.b2 §24.2 L2286 O-bytes "incremental against fresh" repeats oracle 13; "D0 also across every backend" compares one shared host decode within a run, cannot fail by design's own list L2296–2298. Restrict to separately built host binaries. — fixed (§24.2)
- R6.b3 Oracle 4 chain-sample mesh-edge check (L2267) overlaps oracle 3 shared-edge identity (L2266); implement once.
- R6.b4 §13.4 L1500 and oracle 14 both state mesh(K=1) ≡ mesh(K=k); keep one implementation. — fixed (§13.4)
- R6.b5 §11.2 L1257 "cusp certificate" undefined, certification language; replace with "measured |κ₁ − κ₂| above the floor (§11.1)". — fixed (§11.2)
- R6.b6 §19.3 L1855 "certifies absences" → "records that no feature lies within r" (.25.3 language sweep). — fixed (§19.3)
- R6.b7 §9.1 L1059 "multi-surface point (V2)" never defined; define as §8.3 point solve over the cluster's surfaces. — fixed (§9.1)
- R6.b8 §3.2 L201 metric behind "measured inward along the face" not stated; oracle 1 needs it to rebuild the same target. — fixed (§3.2)
- R6.b9 §5.7 L678 uuid_relationship, uuid_provenance, uuid_context "kept verbatim" but no §17.3 carriage row reads them; name a reader or drop.
## Reviewer 7: overfitting and roadmap fit

### Design findings

**R7.1 Repaired target depends on ε, breaking LOD, sharpen-in-place and per-scale instances.** class: design
- Where: §3.2 L198–203; symbols L58–62; §8.2 L943–945; §8.3 L955–960; §8.5 L990–991; §8.8 L1035–1036. Claims broken: §15.3 L1575–1580, §19.5 L1875–1876.
- Wrong: the repaired coherent solid uses request-tied thresholds: β = ε/2 and η = ε/16 decide whether a vertex keeps its stored point or moves to the ball centre, which directions are truncated, and whether rung 0 commits; lfs floored at 2r_c = ε so the ¼-lfs leap and refusal bounds move with ε; target displacement decays over ℓ₃ of the adjacent chain segment, whose length depends on ε. So each LOD level ε_k = 4^k·ε, each sharpen step and each uniform-scale key (ε/σ_max) resolves a different solid. §15.3's "switch moves a point ≤ 1.25·ε_k" and dyadic-ancestor geomorphs assume one fixed target. A rung-2/3 decision flipping between levels can move a boundary up to ¼ lfs.
- Evidence: LOD paper §5.2 pop bound is stated as "two levels both lie within their own ε of the same target". M2 paper defines β = ε/2, η = ε/16 for one check (finding 2); nothing examines cross-level effect.
- Fix: resolve the target once per definition, independent of request: derive reconciliation gates from u_ctx or a session-fixed ε_ref (finest ε the session will use); decay length from source geometry; key L4/L5 positions without ε. Only dyadic cut, collapse and budgets stay ε-dependent. Lets sharpen/LOD steps reuse per-sample measurement and projection keyed by (edge, dyadic path).

**R7.2 Today's ingress gates select a watch-shaped arena corpus (§5.2 L530–535; §5.3 L539–552; §2 L165–167; §23.2).** class: design
- Wrong: adapter refuses 268/500 corpus files (non-mm units 155, C/S residual 44, multi-wire faces on curved carriers 39, leaf binding 14, periodic spline charts 12). OM10 passes every gate with 0 refused faces. ABC blocked (.28). Arena permutations "run as mechanisms land" and nothing orders gate lifts (.25.6, .25.13, .25.12, .25.5) before the first defaults-setting arena. First 64-run arena would choose defaults on the 232 watch-like files.
- Evidence: §5.2 corpus result; M4-decisions .28 open; project instructions require independent workload families before tuning.
- Fix: in §23, sequence these gate lifts before the first defaults-setting arena, or scope first-arena defaults to "admitted set at time T" and rerun (not perturb) the arena as each gate lifts. Report admitted fraction per stratum and report class beside every arena verdict.

**R7.3 Mesh key carries file and entity identity (§16.2 L1621–1624).** class: design
- Wrong: key holds "source digest and #id, UUIDs", contradicting §4.4 L419–420 (#N only in identity-binding layer, never in keys), §19.1 L1797–1798 (entity numbers in no key), §24.3 L2310 (pure renumbering hits everywhere). If source digest = file SHA-256, any channel-F edit misses every key. Owned-operation results have no #id or file digest, so no defined key.
- Fix: definition identity = content digest (Merkle digest of the definition's source records by content, raw IEEE bits). #id, file digest, UUIDs live in identity layer and §19.4 correspondence only.

**R7.4 Relative ε from the world box couples the whole model (§6.1 L694–697; §23.4 L2207–2209; §19.2 L1848–1850; §19.5 L1877–1878).** class: design
- Wrong: relative request uses root world-box diagonal, computed per input; §23.4 runs relative levels on every input. Under channel F an edit growing the box changes ε for every key; a transform change (explosion, motion) does too, contradicting "animation and exploded views are remesh-free". A global coupling of the kind §19.2 forbids but not on its list.
- Fix: relative request resolves once per session from the unexploded root box into an absolute ε, recorded in request and key, never recomputed on edits/transforms unless asked. Add "relative ε re-resolved per input" to forbidden couplings.

**R7.5 Export-frame rounding makes a key's mesh depend on occurrence placement (§3.4 L233–235; §18.1 L1717–1719; §16.2 L1629–1631; §19.5 L1877).** class: design
- Wrong: δ_exp is "δ(R) of the frame the item is written in"; mesh built to ε − δ_exp. STL/3MF bake world coords; C1–C3 enforced in that frame. So one key's construction depends on each occurrence's world position (fasteners across a 350 mm frame, exploded view, moving part), yet key excludes placement and rigid moves touch only the placement table. At authored +100 m family, δ(1e5 mm) ≈ 6.8 µm > ε = 5 µm.
- Fix: construction frame bound is a request field in the key: δ_exp from declared R_max, default the unexploded root box. At export, an occurrence beyond R_max is recentred or refused per occurrence with a record, never rebuilt silently.

**R7.6 Owned-operation results have no ingress path, yet their meshers move to the new path now (§25 L2337–2340 vs §5.1 L470–483, §5.4, symbols L64).** class: design
- Wrong: cut/extrusion/other result meshers retire onto the new path immediately; §25 says other operations follow immediately. Authority rule, units, master choice and u_ctx are defined only for Part 21 contexts; such a result has no representation context, so UNC-FINE, pole snapping "within vertex uncertainty" and C/S grade are undefined. Channel Δ names owned feature operations but no admission row covers them.
- Fix: add a native B-rep source record: owned results written into the source store; their own representation is the declared master; exact rationals rounded once with recorded residual; u_ctx declared by the operation. Add to admission matrix and oracle 8 fixtures.

**R7.7 GPU rules assume the mesh returns to host; G0 judged on the watch (§20.1 item 4 L1906–1910; §20.6 L1969–1973).** class: design
- Wrong: "Never: analytic-surface evaluation" hard-codes a G8 hypothesis (G3 has its own falsifier) into dispatch. G8's basis: "92% of OM10 faces are analytic" and every output crosses PCIe to host. On the roadmap mesh destination is GPU-resident (render, explosion, LOD swaps, sharpen-in-place, physics), so upload is paid anyway. G0 (GPU = CPU-only) stated only on the watch, least GPU-favourable input.
- Fix: charge only bytes that must cross the bus given consumer residency in β_s. Replace the "Never" for analytic evaluation/emission with cost-model gate plus G3 arena test; keep CDT/topology exclusion only with its own evidence. State G0 per report class: analytic-only, mixed, B-spline-heavy.

**R7.8 One-sided tolerance (I9) has no slot in the vertex model (§26.1 L2374 vs §3.5 L245, §17.1 L1664–1669, §24.1 row 4 L2267, §18.4 L1761–1763).** class: design
- Wrong: I9 needs vertices off the surface (contact paper I9 row: vertices outside so chords lie on surface). But every vertex is classified S(u,v) or C(t) and oracle 4 checks it lies there; Waldron e(t) assumes on-surface vertices; chain key carries one position shared by both faces. Mechanism physics needs pins/holes without FP64 interpenetration; §18.4 concedes interpenetration up to ε. Adding signed band later changes vertex record, classification map, ledger, two oracles.
- Fix: reserve now a per-vertex signed normal offset, default 0, in face-output and chain records and classification rows. Oracles 1 and 4 measure against S(u,v) plus offset. Note shared-chain constraint: key position must satisfy both incident faces' bands.

**R7.9 Dyadic counts for LOD nesting cost up to 2× samples on circles/cylinders, no arena arm (§10.2 L1136–1139; §13.1 cylinder row L1437; §23.3).** class: design
- Wrong: replacing closed-form circle counts with dyadic splitting costs up to 2× samples (~1.4× average); θ-slab lattices follow. Watch has 4,431 cylinders, 341 cones, 8,901 circle curves — direct triangle/time cost vs NVIDIA's closed-form counts, paid for LOD nesting, which is in-process display only for alpha (.41). LOD paper §5.2 proposed rounding to base·2^j behind a flag; design dropped it; no arm.
- Fix: arena arm `closed_form_counts`: dyadic vs base·2^j with odd base fixed per edge from ε_0 (nesting kept on 4^k ladder) vs plain closed form. Report per surface-kind class.

**R7.10 Watch sits inside a pooled stratum; speed targets are watch-only (§23.4 L2195–2196; §22.4 item 1 L2077; §22.5 L2117–2118; §22.6).** class: design
- Wrong: watch listed inside production-export stratum; stratum summaries are sums of costs; with ABC absent the speed goal is judged mainly on that stratum; §22.6 targets come only from the watch. A mechanism could win the stratum by winning the watch.
- Evidence: project instructions require separate outcomes per family, watch transformations only supplementing independent geometry; journal "the watch is one benchmark".
- Fix: watch its own report stratum, never pooled. Every arena default and speed claim stated with and without it.

### Bead findings
- R7.b1 §17.3 L1693 glTF _FACEID as FLOAT refuses definitions > 2^24 faces; split attribute or primitive instead of refusing. — fixed (§17.3)
- R7.b2 §15.3 L1575–1578 mixed per-face LOD levels multiply face variants by neighbour-level configuration, so 4/3 pyramid cost understates; E′/S′ at r_c = ε_k/2 undefined across a cluster whose faces sit at different levels (crack risk). Fix one level per collapse component.
- R7.b3 §19.5 L1878–1880 flexing hairspring re-meshes every frame with no per-frame cost estimate while physics takes only final meshes; evaluate re-evaluating positions at same UVs through I3 for small deformations.
- R7.b4 §11.3 L1279–1282 and §6.2 L1727–1731 unchecked freeform defaults (κ̄ with 2× margin, corner-normal angle test) barely exercised by watch (7% B-spline faces); report residual-detector refusals per surface-kind class. — fixed (§11.4, §6.2)

Reviewer note: findings 1, 4, 5 restate that decisions already taken (incremental equals fresh, remesh-free animation, LOD by re-tessellation) need target, ε and key independent of request and placement.

## Disposition table

Designer's fix pass. Every design finding is written into the design's sections;
none is left as errata. Conflicts chosen:

- R2.11 against R6.6 (volume): R6.6. Volume against exact follows from oracle 1, so
  keeping it even as a recorded measurement is a metric for a metric.
- R2.4 against R6.4 (determinism): reconciled. O-bytes holds per placement
  function, a forced placement-log replay reproduces a mixed run's bytes, and
  functional equivalence (FI-1, FI-2) holds across backends and between mixed runs.
- R1.4's "refuse when both faces are of one tier" against R4.15: R4.15. The
  adopted M2 proposal meshes small tangent gaps as recorded steps, refused only
  above ¼ lfs; the refusal becomes an arena arm. No operator question arises.
- R6.1's freeform alternatives: R1.12's bound adopted, so the 2× estimate is not
  kept as a construction rule.

Partial rejections, each with its reason in the row: R2.5 (hand-made defective
outputs are mutants, deferred by the operator), R2.11 (above).

| Finding | Disposition |
|---|---|
| R1.1 | fixed (§12.1, §12.2, §12.3): openings from the signed dihedral, s(Δψ) capped at 1 above π/2 |
| R1.2 | fixed (§8.8): PSLG-style 3D feature size, floored at ε_ref |
| R1.3 | fixed (§8.1): signed d_f; \|x\| only where full rank |
| R1.4 | fixed (§8.3): conditioning on unit rows after hard elimination; hard block by truncated SVD, overdetermined by least squares; equal-tier case per R4.15 (§8.7) |
| R1.5 | fixed (§3.2, §3.4, §3.6, §13.2): C¹ smoothstep field, r_disp in the ledger, reflex bisectors as constraints, carrier normal with tilt margin in A0 |
| R1.6 | fixed (§8.3, §8.4, §8.5, §3.2): rung 0 commits on the measured residual after L3; the target passes through every resolved point |
| R1.7 | fixed (§13.1, §3.4, §4.6): D as max over spans met; C⁰ knot lines as interior creases with two normals |
| R1.8 | fixed (§13.1; family §23.5): quotient-rule bound on recentred homogeneous data; ZS kept for curves only |
| R1.9 | fixed (§18.1, §3.4): one δ_exp per key from R_max, 3MF double rounding included, C1–C3 at 2δ_exp |
| R1.10 | fixed (§12.1, §12.3): μ per zone from edge move bounds, zero outside decay bands |
| R1.11 | fixed (§11.1): hodograph Bernstein spread, control-polygon length, foot-point bounds; quadrature dropped |
| R1.12 | fixed (§6.2, §13.1, §11.1): Bernstein normal cone and σ̲ ≥ min\|N\|/σ̄ are the defaults |
| R1.13 | fixed (§16.1): canonical ε_def = ε/(1 + τ) for rigid and mirrored |
| R1.14 | fixed (§12.3, §27 #15): signed credit on by default at material cusps only |
| R2.1 | fixed (§24.1 rows 1 and 10, §3.2, §8.4): source-anchored reconciliation oracle with M2 H3 model tier; oracle 1 judges faces against verified chains; foreign-producer form defined |
| R2.2 | fixed (§24, §25): oracle 1, its own trim domain and oracle 10 are build units; readers' on-source and mandatory-pcurve checks replaced |
| R2.3 | fixed (§24, §7.1): pass / fail / not_evaluated; not_evaluated non-green, counted separately |
| R2.4 | fixed (§24.2, §4.2): O-bytes per placement function; forced placement-log replay; FI between mixed runs (reconciled with R6.4) |
| R2.5 | fixed in part (§24): real defective outputs (NVIDIA's recorded invalid outputs, guarantee-dropping arena arms) exercise the oracles in the same pass. Rejected in part: the hand-made negative-control column (vertex pushed 1.01ε, swapped ranges) is a set of output mutants, and the operator deferred mutation testing until the stack settles (§24.4) |
| R2.6 | fixed (§24.1 row 5, §12.5, §27 #17): oracle recomputes every exempt pair's source clearance; c_floor settled by that measurement |
| R2.7 | fixed (§24.1 row 6): G13 H2 precondition stated, A0 and strip bounds, H5 per corner on every carrier, limit normals |
| R2.8 | fixed (§24.1 row 16, §22.2): dense normal-turning and curve-turning oracle, run on NVIDIA's angle arm |
| R2.9 | fixed (§24.1 row 1, §3.3, §18.1): thresholds from inputs only; FP64 at ε_f − δ_exp of the key and each written format at ε_f |
| R2.10 | fixed (§24.3): G12 §5.4 carried whole; class expectations fail, never discarded; halo check |
| R2.11 | fixed (§24.1, §7.4): removed from the stack and the profile. Its "keep as a recorded measurement" part rejected: the number follows from oracle 1 and no claim reads it (tenet 5); R6.6 chosen over it |
| R2.12 | fixed (§24.1 row 17): spec-tier count oracle on untrimmed analytic families |
| R2.13 | fixed (§23.5): each family names the record that shows its branch fired; absent record fails |
| R2.14 | fixed (§23.5, §4.1): fuzz outputs pass the stack or carry refusals for exactly the affected entities |
| R2.15 | fixed (§24.1): rows 1, 2, 4 relabelled property; row 15 now checks the §4.7 table's named status and stop, a spec-derived value, so it stays spec |
| R3.1 | fixed (§5.6, §1, §22.3): operator answer carried; adapter phase split an M8 measurement; kept and removed phases named; streaming per definition |
| R3.2 | fixed (§22.6): target from the steel-man lower bound (≈ 2.0 s lane makespan, ≈ 70× cut); per-stage budget table with counters |
| R3.3 | fixed (§4.3, §8.1, §13.4, §20.1): per-occurrence measurement and lattice interior are their own produced entities; M8 measures the empty-band share |
| R3.4 | fixed (§13.1, §13.2, §29; family §23.5): nesting from topology and Lemma 1; pairwise check dropped; many-hole planes split; a plan requirement |
| R3.5 | fixed (§4.1, §8.1, §12.3): readiness mechanical over declared reads; T7 on source with T3 move bounds; two-hop rule removed; critical path stated |
| R3.6 | fixed (§4.5, §4.1): per-class staging buffers, per-device N*, urgent healer items on CPU; remaining barriers named; B-ED-CL measured in M8 |
| R3.7 | fixed (§20.1 items 5–6): resident span tables and trim polygons, β per class by layout, per-lane device-memory plan with eviction |
| R3.8 | fixed (§21.1): lane axis per kernel; degree buckets |
| R3.9 | fixed (§20.1 item 8, §4.6): per-worker bound huge-page slabs, per-definition chunk lists compacted at T10, expected bytes stated |
| R3.10 | fixed (§12.3, §12.6, §22.6; family X1 §23.5): generic analytic bounds; E1 counts an M8 measurement before T7; per-pair target |
| R3.11 | fixed (§20.3): calibration an explicit read-only pre-pass artifact, digest in RUN_____, one file per device |
| R3.12 | fixed (§20.1 item 2, §23.2, §27 #23): 12 pinned CPU-only; 11 + driver with blocking sync; gpu_driver_core arm |
| R3.13 | fixed (§19.1, §22.3): hashing in place; keys only with the store on; store opt-in and off in the cold headline |
| R4.1 | fixed (§5.8, §7.2): refusal catalog; every adapter throw mapped; exit 4 for file-level, 1 only for internal faults |
| R4.2 | fixed (§4.1, §4.5, §8.1, §11.1, §11.4, §12.3): pointwise resolution, closure once, T7 before T8, no re-split; one hop with a D̲ margin |
| R4.3 | fixed (§4.1, §8.1, §6.2, §5.8): fail-closed gates, finiteness per task, NaN before reductions, truncated \|x\|, undeclared_singularity |
| R4.4 | fixed (§4.7): termination table; projection statuses mapped; plain-double backstop |
| R4.5 | fixed (§6.2, §3.1, §3.4, §6.1, §9.1, §9.4): asin form with clamps, positivity checks, request validation |
| R4.6 | fixed (§4.1, §24.1 row 3): refusal terminal and releasing; evaluable carriers still feed neighbours; open_by_refusal |
| R4.7 | fixed (§7.1, §7.2, §7.3, §3.9): event → status table; status per (subject, format); exit = worst over formats |
| R4.8 | fixed (§5.3): S1 per face by owned Green's sums on T2 UVs, S2 per shell by signed mesh volume at T10; source_uv leaves the path |
| R4.9 | fixed (§5.3, §5.8): CPU and address-space rlimits, ingress_budget exit 4 |
| R4.10 | fixed (§18.1, §13.2): δ_exp per key; C1 a T9 predicate; chain-bounded needles refuse the format; oversized reserve recentres or refuses before meshing |
| R4.11 | fixed (§11.1, §11.2): P3 waived at measured cusps, innermost segment exempt, graded L*(r) with count bound, Δκ floor |
| R4.12 | fixed (§4.8, §7.2): cancellation at task dequeue; exit 6 per the operator; fuzzer timeout defined |
| R4.13 | fixed (§19.1, §20.3): re-hash on read, atomic rename, cost model validated on read |
| R4.14 | fixed (§4.8): failed batch re-placed on CPU, device leaves the run, device_fallback record |
| R4.15 | fixed (§8.5, §8.7, §23.3, §27 #41): refusal not kept; symmetric recorded step by default, refusal an arena arm, class counted in M8 |
| R5.1 | fixed (§22.5, §22.1): NVIDIA check outcomes pass / fail / not_evaluated; not_evaluated row; code side is R5.b9 |
| R5.2 | fixed (§22.3): one declared owned configuration per stratum, recorded first; held-out confirmation |
| R5.3 | fixed (§22.5): bootstrap interval, fraction won, per-input distribution, win rule with coverage, censored rows |
| R5.4 | fixed (§22.2, §22.3): streamed and dynamic-pull NVIDIA arms, ideal-overlap bound meanwhile; owned fork-to-exit scope; stage cost per task class plus meshing-only run |
| R5.5 | fixed (§22.2, §6.2): operator answer "achieved" carried; validity = topology + finite points; chord tightened until achieved; δ and counts with every ratio |
| R5.6 | fixed (§22.2, §22.4 item 7, §23.2): NVIDIA per lane, same-lane pairing, per-lane GPU effects, two-socket NVIDIA arm added |
| R5.7 | fixed (§22.4 item 4): per-lane per-CPU gate, load as covariate, other-lane state a blocking factor |
| R5.8 | fixed (§22.4 items 5 and 7): cycles per CPU and GPU clock, throttle and link state sampled during the run; tool refuses mixed link widths |
| R5.9 | fixed (§20.3, §22.3): frozen read-only calibration per series; empty verified cache directory per cold row |
| R5.10 | fixed (§23.6): n = 1 per (configuration, input, lane), seeded recorded sample, named primary response, adoption rule with fresh confirmation |
| R5.11 | fixed (§22.2): screening protocol with stated bias; harness generalisation required before corpus claims (code side R5.b10) |
| R5.12 | fixed (§22.4 item 2): one seeded schedule per lane alternating owned and NVIDIA |
| R5.13 | fixed (§22.3): NVIDIA warm arm; warm vs warm only; cache hits excluded |
| R5.14 | fixed (§22.6): CPU-seconds and 12-core wall, NVIDIA ingress + batch scope, pre-fix n = 1 label |
| R5.15 | fixed (§22.1, §22.2): operator answer "as shipped" carried; source rebuild a named exclusion |
| R6.1 | fixed (§11.4, §11.1, §6.2): residual detector deleted; sound freeform bounds instead (R1.12) |
| R6.2 | fixed (§12.3, §11): UV parts of E1(d) and E3 deleted; closure P2/P3 the one UV mechanism; no T7→T8 feedback |
| R6.3 | fixed (§7.4, §7.3): operator toggle carried (default runtime set only; self-intersection classes and P2 re-run offline or on request); Part 59/Quality Stamp emission dropped; runtime measures defined; emission timed in the headline |
| R6.4 | fixed (§24.2, §4.2, §19.2): FI-1 and FI-2 only, excuse rule a predicate; FI-3–FI-5 and forward-error bounds dropped |
| R6.5 | fixed (§7.1, §7.3): status from construction events; no per-clause pass bits; rung stored once |
| R6.6 | fixed (§24.1, §7.4): oracle 10 (volume) and B-ME-VP deleted; B-VP-SV kept, on request |
| R6.7 | fixed (§21.3, §24.1): runtime conformance gate and oracle 16 deleted; device cell classification conservative (§13.4) |
| R6.8 | fixed (§23.2, §23.3, §18.3, §27 #21): topology_change a user option, mirror_3mf a consumer check, directed rounding dropped, simd a microbenchmark only |
| R6.9 | fixed (§4.2): libm with identity in backend_id |
| R6.10 | fixed (§12.2, §12.3, symbols): m_α (angular, absolute) and m_c (clearance factor), each with value and basis |
| R6.11 | fixed (§6.4, §24.1 row 9): m_t, c_t and warnings only; drawing transcription dropped |
| R6.12 | fixed (§10.3): ZS curve rule written in full with its own symbols; surfaces use §13.1 |
| R7.1 | fixed (§3.2, §8.3, §8.4, §19.1, §15.3, §19.5): one target per definition from ε_ref; pointwise resolution; decay length from source geometry; reconciliation keys without ε |
| R7.2 | fixed (§23.4): defaults adopted only after the four gate lifts; earlier runs exploratory; re-run as gates lift; admitted fraction reported |
| R7.3 | fixed (§16.2): Merkle content digest; #id, file digest and UUIDs in the identity layer only |
| R7.4 | fixed (§6.1, §19.2): relative ε resolved once per session from the unexploded box; added to forbidden couplings |
| R7.5 | fixed (§18.1, §16.2, §19.5): R_max a request field in the key; occurrences beyond it recentred or refused per format at export |
| R7.6 | fixed (§5.4 row 28, §24.1 row 8, §25): native source record for owned-operation results |
| R7.7 | fixed (§20.1 items 4–5, §20.6): consumer residency in β; analytic offload by cost model and arena G3; G0 per report class |
| R7.8 | fixed (§4.6, §26.1, §24.1 row 4): signed normal offset slot reserved, default 0 |
| R7.9 | fixed (§10.2, §23.3, §27 #43): arm closed_form_counts, reported per surface-kind class |
| R7.10 | fixed (§23.4, §22.5): the watch its own stratum; claims with and without it |

## Coordinator check

The coordinator read the revised design whole against every finding and every
overlap group, without re-engaging any reviewer. Each "fixed" row's fix is in the
section it names. The two partial rejections stand: hand-made defective outputs
are output mutants and wait for mutation testing (§24.4, the operator's
deferral), and a volume measurement derived from oracle 1 would be a metric for a
metric. The 21 bead findings not fixed in passing are filed under M8
(`brepgpu-12s.25`), with R3.b3 noted on `.25.2`.
