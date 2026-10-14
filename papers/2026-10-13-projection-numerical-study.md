# Seeded bounded point-to-surface projection: numerical study

## Recommendation and scope

Build an owned, bounded **local box-stationarity solver** over the iteration 1 rational Bézier carriers. One request names an immutable patch, a target in that patch's local coordinates, and an explicit normalized parameter seed. Use the existing six-jet evaluators inside a resident CUDA loop: safeguarded positive-definite full-Hessian Newton steps, a damped Gauss–Newton fallback, and a projected-gradient fallback. Evaluate actual surface distance during a bounded Armijo search. Return the final candidate, observed box-stationarity residual, terminal status and separate numerical classification flags.

This operation does not find the globally nearest point, prove a local minimum, traverse neighboring patches, enforce trims, repair charts, unwrap periodic parameters, or choose its own seed. A stationary saddle is a possible result. A constant or nearly collapsed chart is a possible input. Those facts must appear in the result classification rather than disappear behind a “projection succeeded” name.

The proposed constants below are an implementable first contract for design review. They are qualification settings, not universal conditioning or convergence guarantees. No solver product code or dependency adoption was made during this study.

Companion artifacts: [bibliography](projection-numerical.bib), [pinned source-entry catalog](projection-numerical-sources.tsv), [PDF acquisition/read catalog](projection-numerical-pdfs.tsv).

## Primary sources actually inspected

The existing intact source archives were reused; no reference source was copied into product files. Restoration instructions are in [the numerical restoration report](first-iteration-numerical-restoration.md) and [the OCCT restoration report](acceleration-roadmap-reference-restoration.md). The acquisition catalog [projection-numerical-pdfs.tsv](projection-numerical-pdfs.tsv) records downloaded primary papers; their PDFs remain ignored.

| Source | Pinned revision / entry points | Findings relevant to this solver |
| --- | --- | --- |
| SISL | `e0a82b1a851e932657f3e1af25033fbbcd33e907`; `src/s1773.c`, `src/s1958.c`, `src/s1954.c`, `src/s1421.c` | Local D1 tangent-plane iteration is distinct from edge comparison and full closest-point machinery. Its completion labels cannot serve as our stationarity oracle. |
| OCCT | `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`; `src/ModelingData/TKGeomBase/Extrema/Extrema_FuncPSNorm.cxx`, `Extrema_FuncPSDist.cxx`, `Extrema_GenLocateExtPS.cxx`; `src/FoundationClasses/TKMath/math/math_FunctionSetRoot.cxx`, `math_NewtonFunctionSetRoot.cxx/.lxx`; `gp/gp_Vec.hxx` | Correct full Hessian in the normal-root path; minimization/root solving are separate. The inspected distance-gradient branch has a sign/scaling anomaly. |
| openNURBS | `eb92af3ba1806b0a34a99aba0d3bda83e3d46083`; `opennurbs_surface.cpp/.h`, `opennurbs_nurbssurface.cpp/.h`, `opennurbs_evaluate_nurbs.cpp` | Public surface six-jet and chart APIs are usable study material; the pinned public ON_Surface / ON_NurbsSurface sources do not provide a general seeded NURBS surface closest-point optimizer. Rhino's larger capabilities are not evidence that this public source contains that solver. |

### SISL: the name “Newton” hides a first-order model

[`s1773`](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s1773.c) requests `kder=1` and solves the 2×2 tangent Gram system in `s1773_s9dir`. The residual-weighted second derivative terms are commented out. In optimization terms it is Gauss–Newton for squared distance, despite the higher-level Newton description. It clips both parameter increments into the rectangle, runs at most 30 trials, and can accept a trial because the old and new step directions have nonnegative dot product. The initial previous distance is huge. Consequently its branch condition is not a monotone actual-distance line search.

Stopping includes relative parameter step size and a rounded no-motion test. After that, status 2 labels a minimum unless the distance or degeneracy branches apply; no final box-KKT or second-order check establishes that label. In some rejected-trial cases the distance variable describes the trial while the output parameters still describe the accepted guess. Derive all final metrics again at the actual returned parameters.

The degenerate `Su=0` branch contains `dot(PS,Sv)/l2*l2`; ordinary left-to-right evaluation does not divide by `l2*l2`. The numerical policy also uses a fixed small angle and geometric-length thresholds for degeneracy. These are reference behaviors to avoid reproducing, not product dependencies or trusted oracles.

[`s1958`](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s1958.c) explicitly disclaims finding the correct global solution. It compares all four extracted boundary curves and a seeded interior iteration; seed construction is another operation. [`s1954`](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s1954.c) instead builds a scalar squared-distance surface and searches its minima, including continuous closest-point sets. A one-seed rectangular iteration is a deliberately smaller operation than either complete pipeline.

### OCCT: full Hessian, root search and minimization are different paths

[`Extrema_FuncPSNorm::Values`](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKGeomBase/Extrema/Extrema_FuncPSNorm.cxx) computes the two orthogonality equations and their Jacobian using D2. These are exactly the gradient and Hessian derived below. `GetStateNumber` records roots; it does not classify them as local minima.

[`Extrema_GenLocateExtPS`](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKGeomBase/Extrema/Extrema_GenLocateExtPS.cxx) distinguishes normal projection from distance minimization. The normal path calls a function-set root solver and a Newton fallback. The distance path calls BFGS then FRPR. `IsMinDist` checks eight neighboring parameter samples; it is a finite probe, not a proof. It is not called by the inspected `Perform` body.

The generic function-set solver contains gradient fallbacks, SVD/least-squares machinery, boundary direction recalculation and direction minimization. Its merit function is squared norm of the root equations; decreasing orthogonality residual is not the same as decreasing point-to-surface distance. The simpler Newton root solver clips updates and tests both step and root tolerances, but supplies no sufficient-decrease safeguard for our distance objective.

There is an important source anomaly in [`Extrema_FuncPSDist::Gradient/Values`](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKGeomBase/Extrema/Extrema_FuncPSDist.cxx): its value is squared distance, while its vector is `gp_Vec(surface_point,target)`, dotted with the tangents. The pinned [`gp_Vec` constructor](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/FoundationClasses/TKMath/gp/gp_Vec.hxx) subtracts its first point from its second. Relative to the stated value this has the wrong gradient sign and omits the factor two. The independent oracle lane found the same discrepancy. We have not executed that OCCT branch and make no wider claim about all OCCT projection behavior. Our owned descent sign must follow the objective's derivative, not this branch.

### openNURBS: derivative and domain study, not an available optimizer

[`ON_Surface::Ev2Der`](https://github.com/mcneel/opennurbs/blob/eb92af3ba1806b0a34a99aba0d3bda83e3d46083/opennurbs_surface.cpp) reads `S,Su,Sv,Suu,Suv,Svv` from `Evaluate(...,2,...)`. The headers also distinguish parameter domain, side of evaluation, singular boundaries and normals. These distinctions matter when a carrier is split into normalized patches. No implicit change to source UV, periodic unwrapping, normal-based seed recovery or trim membership belongs in the first solver.

General seeded ON_Surface/ON_NurbsSurface closest-point routines are absent from the inspected pinned public definitions. Specialized line, plane, circle and other primitive closest-point methods elsewhere in openNURBS do not fill that gap. We therefore cannot use a supposed public openNURBS surface solver as a dependency or black-box truth source.

## Optimization literature and recent work

[Bertsekas (1982)](https://web.mit.edu/dimitrib/www/ProjectedNewton.pdf) shows why arbitrary positive-definite scaling followed by Euclidean clipping can fail at simple bounds, and develops partly diagonal projection methods with Armijo safeguards. The takeaway for our tiny system is practical: clipped Newton displacement must be checked for descent. We do not inherit his convergence results automatically for a finite, floating-point implementation with different active-set and stopping rules.

[Moré's primary LM report](https://www.osti.gov/servlets/purl/7256021) discusses scaling and reliable least-squares solves. It warns that normal equations are unreliable near rank deficiency and favors pivoted QR. Our two-variable damped Gram fallback trades that general machinery for a bounded, explicitly regularized system and must expose numerical rank problems. It is a damped Gauss–Newton safeguard, not an implementation of Moré's full trust-region LM algorithm.

[Hu and Wallner (2005)](https://www.geometrie.tugraz.at/wallner/sproj.pdf), Section 3, use tangent-plane and curvature-circle geometry for surface projection. Their surface derivation assumes a regular chart; curvature normalization introduces additional divisions. This is useful comparative research, but does not directly resolve rectangular KKT, collapsed charts or a finite work budget. Full objective Hessians are already available from our six-jet, so a safeguarded algebraic 2×2 step is the smaller first implementation.

[Ferry, Gill, Wong and Zhang (2024)](https://doi.org/10.1080/10556788.2023.2241769), with an [author PDF](https://www.ccom.ucsd.edu/~peg/papers/quasiwolfe.pdf), study searches along projected paths, whose bound hits create kinks. Their introduction specifically warns that gradient dotted with an arbitrarily projected displacement need not be negative. We avoid a piecewise projected search: project the proposed endpoint once, reject any non-descent displacement, and search along the resulting feasible straight chord. Quasi-Wolfe interpolation is not required for the first bounded solver.

[Smee and Roosta (ICML 2024)](https://arxiv.org/abs/2405.12401) extend two-metric methods for nonconvex problems and explicitly address non-positive curvature and approximate optimality. This reinforces separate first-order, rank and curvature reporting. Their Hessian-free MINRES machinery targets large systems; our dimension two does not justify importing it.

[Wu and Zou (2025)](https://arxiv.org/abs/2504.11498) provide recent GPU projection work using matrix representations, cubic approximation, Bézier clipping and cooperative scheduling. The inspected derivation and experiments concern non-rational B-spline curves; the conclusion explicitly lists rational splines as future work. The reported RTX 3090 performance and approximate inversion errors are not evidence for P100 FP64 rational surface projection. Their preprocessing-versus-per-point timing separation and divergence discussion are useful. Their global curve machinery is not replaced or reproduced by a seeded surface solve.

The acquired [Schmidt, Kim and Sra chapter](https://optml.mit.edu/papers/sksChap.pdf), Sections 11.2–11.3, also explains feasible-chord searches and two-metric projection. Its principal presentation assumes convexity, which our distance surface can lack. A mirror named `ProjectedNewton.pdf` actually contained this chapter rather than Bertsekas; inspected authors/title were used to correct the acquisition filename before citation. Bertsekas was then acquired from his MIT page.

## Owned objective, sign and stationarity

For target `X`, let `x=(s,t)`, `r=S(x)-X`, and

\[
f(x)=\tfrac12 r^Tr,\qquad
g=\begin{bmatrix}r\cdot S_s\\r\cdot S_t\end{bmatrix},\qquad
G=J^TJ.
\]

The full Hessian is

\[
H=\begin{bmatrix}
S_s\cdot S_s+r\cdot S_{ss}&S_s\cdot S_t+r\cdot S_{st}\\
S_s\cdot S_t+r\cdot S_{st}&S_t\cdot S_t+r\cdot S_{tt}
\end{bmatrix}.
\]

`G` is positive semidefinite; `H` need not be. Gauss–Newton omits the residual-curvature terms. Near a regular on-surface inversion, those terms vanish with the residual. Off the surface they can dominate, so a tangent metric alone cannot classify a distance minimum.

Use componentwise unit-box projection `P`. The reported residual is

\[
\rho(x)=\|x-P(x-g(x))\|_\infty.
\]

In exact arithmetic zero is equivalent to box first-order conditions: interior `g_i=0`; lower bound `g_i>=0`; upper bound `g_i<=0`. This handles constrained edge/corner answers for which interior normal equations do not vanish. The scale in this definition is fixed at one. Do not silently divide by a huge Hessian norm and make a small chart look solved. Numerically small `rho` remains a parameter/objective-space observation, not a Cartesian distance or UV error enclosure.

## Concrete first solver contract

### Input, coordinate and seed semantics

Keep iteration 1 degrees `0..3`, bounded homogeneous controls, normalized `[0,1]^2` parameters and immutable patch associations. First target envelope: finite local coordinates with each `|X_axis|<=4`. Require finite seed coordinates in the closed unit square; reject outside seeds instead of silently clamping them. Targets and seeds are snapshotted before native execution. Per-patch source UV and affine provenance remain metadata; a named host adapter must explicitly transform world targets if needed.

No hidden reseeding, control-net seed search, multiple-start selection, boundary enumeration or neighboring-patch traversal. A caller may submit multiple rows with different seeds or patches and perform a separately timed selection. That produces the best candidate among those requests, not a global nearest-point certificate. Warm starts inherit the caller's chart identity; identical patch dimensions do not make seeds interchangeable.

### Proposed constants and caps

| Setting | First proposal | Meaning |
| --- | --- | --- |
| Stationarity tolerance | `tau=2^-24` by default | Threshold for final observed `rho`, not position accuracy |
| Accepted update cap | 64 default; explicit option in `1..256` | Upper bound on accepted UV updates |
| Backtracking trials | 24 per search | `alpha=1,1/2,...,2^-23`; bound applies to each of at most two searches per update |
| Raw UV step cap | infinity norm at most `1/4` | Apply common scalar reduction before projection; does not certify trust-region quality |
| Armijo coefficient | `c=2^-12` | Use the actual rounded feasible displacement |
| Full-Hessian pivot guard | `2^-40` after scaling matrix entries by `max(1,||H_FF||_inf)` | Reject unstable/nonpositive Cholesky pivots |
| GN damping | `mu=2^-20*max(1,||G_FF||_inf)` | Positive regularization, not geometric tolerance |
| Gradient fallback scale | `1/max(1,||G||_inf,||H||_inf)` | Diagonal common scale before box projection |

If tolerance is exposed initially, propose finite `tau` in `[2^-40,2^-12]`. A tighter permitted tolerance can return unresolved; permission to request it does not promise universal attainable accuracy. Classification guards should be named separately from the termination tolerance.

### Iteration rules shared between CPU and CUDA

1. Evaluate the six-jet at the exact current UV. Form `r,f,g,G,H` with specified FP64 operation order and finite checks. Compute `rho`. If it is within `tau`, stop with the stationarity terminal reason and classify the point; do not substitute a small-step or objective-change test.
2. Freeze a coordinate only when it is **exactly** on its box bound and its gradient points outward: lower bound with `g_i>=0`, upper bound with `g_i<=0`. Recompute this set every iteration. A near-boundary coordinate is not frozen by an invented UV epsilon. At a weak active zero gradient, freezing is a step-construction choice, not proof that all critical directions are fixed.
3. Solve `H_FF*p_F=-g_F` on remaining coordinates only if scaled 1×1/2×2 Cholesky passes the pivot guard. Fix other components to zero. Check every computed component for finiteness. Do not use an unscaled determinant/Cramer's-rule inversion.
4. Otherwise solve `(G_FF+mu*I)*p_F=-g_F` with the same bounded solver. This is the GN fallback. A failed arithmetic solve falls back to the diagonal projected-gradient proposal if the six-jet and objective are still valid; a corrupted surface evaluation is an arithmetic failure, not a seed repair.
5. Apply the common step cap; form `z=P(x+p)`, `d=z-x`. Require a nonzero, finite actual displacement and `g·d<0`. If clipping destroys descent or direction construction fails, form the projected-gradient endpoint instead and apply the same cap/feasibility/descent rules.
6. Search the feasible chord from `x` to `z` using at most 24 trials. Form a trial by the specified interpolation/clamping order, obtain its **actual rounded UV displacement** `dx`, and require `g·dx<0`. Evaluate `S_trial` with order zero. Accept only a negative stable actual-distance change satisfying Armijo. Never add a positive acceptance fudge that permits an increasing distance.
7. If the Newton/GN chord search exhausts its budget, try one fresh projected-gradient chord search with the same bounded rules. If that search also fails, return `LINE_SEARCH_LIMIT` or `STAGNATION`, after recomputing final metrics. Do not change tolerance, reseed, reset the device or launch a host recovery silently.
8. An accepted trial becomes current. Recompute its full jet and `rho` before returning it. At the accepted-update cap, this final recomputation can still yield stationarity; otherwise return `ITERATION_LIMIT`. Count accepted updates explicitly, not the initial evaluation as an update. Trial-evaluation counters may be retained for qualification/benchmark diagnostics without writing all intermediate jets.

Because every accepted step decreases the chosen computed-distance change, the current candidate is the best accepted candidate. Keep the initial seed as the candidate if no step is accepted. Maximum work is one initial six-jet plus at most 256 accepted-update six-jets and `2*24*256` value trials, plus a bounded final recomputation. The default caps reduce those bounds to 64 updates. No dynamic subdivision or unbounded convergence loop exists.

### Distance cancellation and no-motion handling

Avoid subtracting two rounded squared distances. Use

\[
\Delta f=\tfrac12(S_{trial}-S_{current})\cdot
\big((S_{trial}-X)+(S_{current}-X)\big).
\]

Require `Delta_f<0` and `Delta_f<=c*(g·dx)`. Compute its three-term sum using a specified owned FP64 operation order; if explicit FMA is chosen, choose it explicitly on both backends rather than allowing implicit contraction. This identity concerns distances to the **evaluated points**; without validated surface-evaluation bounds it is not a formal exact-surface monotonicity certificate.

Direct witness: plane `S=(s,t,0)`, target `(1/2,1/2,1)`, current `s=1/2+2^-30,t=1/2`, trial at the target UV. Both rounded `f` values equal `1/2`, but the stable change is exactly `-2^-61`. A suitably tight requested stationarity threshold must not force this real improvement to look like objective stagnation. Conversely, if the trial UV equals the current UV or the evaluated surface points coincide, a zero change is not sufficient decrease. Recompute the final stationarity residual; if it fails, emit unresolved stagnation rather than success.

## Terminal outputs and classification

Keep terminal reason distinct from geometric interpretation. Suggested terminal reasons: `STATIONARY`, `ITERATION_LIMIT`, `LINE_SEARCH_LIMIT`, `STAGNATION`, `BAD_PATCH`, `BAD_TARGET`, `BAD_SEED`, `ARITHMETIC`. Exact numeric encoding belongs to the integration design. Preserve input ordering and patch association. Batch structural/runtime errors publish no usable result.

For a valid numerical solve, emit final `s,t`, `S`, squared distance `2f`, observed `rho`, accepted-update count and classification flags. Invalid input or an untrustworthy arithmetic evaluation gets all NaN numeric fields and no valid candidate. Solver-limit/stagnation outcomes may retain their **finite best accepted candidate**, explicitly marked as unresolved by status and a candidate-valid indication. This is a distinct projection-result contract; do not accidentally inherit the evaluator's all-NaN rule for every nonzero per-query status. A convenience stationarity check must inspect status; no minimum/global-nearest helper is justified.

Recompute all final fields at the exact reported UV. Never return a seed's gradient with a trial's point, or classify a rejected trial while returning the previous UV. `distance2` may be positive at perfectly valid stationary projection; it is not an inversion-only acceptance test.

Flags should report observed interior/boundary location, ill-conditioned/degenerate tangent metric, negative **feasible** curvature, positive reduced curvature or ambiguous curvature, as justified. First practical rank screen: compare the smallest observed Gram eigenvalue against `2^-32*max(1,||G||_inf)`. This is a numerical warning in the chosen coordinates, not a theorem that a chart is geometrically singular. Constant/curve-like carriers are accepted inputs but visibly degenerate for two-dimensional chart inversion.

For curvature, use a conservative proposal such as `delta_H=2^-12+2^-40*||H||_inf` to separate clear signs from an ambiguous band. This is an empirical classification guard; qualification must record it. Positive full/reduced curvature is an observed matrix property, not a validated local-minimum certificate. Near-zero curvature is ambiguous, not automatically chart singular.

### Boundary curvature uses critical directions

At exact KKT points, feasible directions point inward at active box coordinates. Strict active multipliers remove their directions from the critical cone; zero multipliers leave one-sided critical directions. Therefore deleting all active coordinates or inspecting full-Hessian eigenvalues is insufficient at weak boundaries.

Implementable conservative first classifier: interior eigen-sign screening; reduced tangent Hessian on coordinates with clearly strict outward multipliers; and an ambiguous boundary-curvature flag when weak active multipliers remain unless a negative direction is explicitly shown feasible or positive full Hessian is observed. An optional complete 2D critical-cone classifier can enumerate its unit-ray endpoints and feasible Hessian eigenvectors, but is not required to return an honest first-order candidate.

Exact counterexample: `S=(s,t,st)`, target `(0,0,-2)`, seed `(0,0)`. `g=0` and `H=[[1,2],[2,1]]` has eigenvalue `-1` along `(1,-1)`. That direction is infeasible at this corner. Every nonzero feasible `(ds,dt)` has quadratic form `ds²+4ds*dt+dt²>0`; the corner is the unique constrained closest point on the unit square. Flagging it as a nonminimum from the unconstrained eigenvalue would be wrong.

## Range, conditioning and error limitations

The target envelope and iteration 1 derivative magnitudes keep residual components within 5, squared distance within 75, gradient components within 34,560 and Hessian entries below roughly 70 million, before small qualification margins. This avoids ordinary overflow in objective/system arithmetic and makes the matrix scaling concrete. It does not bound inverse metric/Hessian norms: valid control inputs include arbitrarily collapsed charts and nearly zero curvature.

If the iteration 1 corpus budgets were actual pointwise bounds, their `E0=2^-40`, `E1=2^-30`, `E2=2^-20` would give a gradient-entry perturbation of approximately `2e-8` at the worst envelope and Hessian-entry perturbation of approximately `4e-5`, plus arithmetic rounding. Those are **conditional calculations** explaining the proposed default stationarity and conservative curvature guard. The budgets are empirical corpus targets, not universal runtime enclosures; do not promote this calculation into a certificate.

A small observed gradient can come from tiny tangents far from the target. A regular chart can have a nearly singular distance Hessian at an evolute/focal configuration. A well-defined Cartesian footpoint may have nonunique UV in a collapsed chart. A small step can come from damping, clipping, line-search exhaustion or rounding. All are reasons to expose the residual and rank/curvature classification rather than infer UV or distance accuracy from solver completion.

## Direct qualification attacks

The independent oracle lane owns the fixture corpus; these mathematical cases specify the claims it must attack.

| Case | Direct expected property |
| --- | --- |
| Axis-aligned/oblique affine plane | Analytic constrained least squares; interior, all four edges and all corners; inward seed at a boundary must be released |
| Nonzero normal-offset plane with tiny tangential error | Stable distance-change witness above; objective subtraction and step-size-only stopping fail |
| `S=(2s-1,t,(2s-1)^2)`, `X=(0,1/2,3/4)` | Exact stationary UV `(1/4,1/2),(1/2,1/2),(3/4,1/2)`; outer points have distance² `1/2`, center `9/16` and `H=diag(-2,1)`; exact center seed must expose negative curvature |
| Same parabolic cylinder with target height `1/2` | Center has `H=diag(0,1)` and a quartic minimum despite a regular chart; ambiguous curvature must not become chart singularity |
| `S=(s,t,st)`, target `(0,0,-2)` | Weak corner counterexample above; unconstrained negative eigenvector is infeasible |
| Constant/curve-like/collapsed carriers and nearly collinear tangents | Degenerate/ill-conditioned classification; finite candidate is not a unique inverse chart |
| Exact rational quarter-cylinder with nonuniform weights | Off-surface analytic cylindrical footpoint with rational UV map; axis target yields continuum/ambiguous angular projection |
| Weighted bilinear/bicubic patches | Exact rational six-jet oracle recomputes final point, distance, gradient and box residual at supplied binary64 returned UV |
| Caps forced to one update or exhausted search | Explicit unresolved status and valid retained candidate; no fabricated stationarity |
| Multiple seeds on a multi-basin patch | Seed-dependent outcomes visible; best-of-submitted candidates is not global nearest |

The CPU outer iteration rules should match CUDA, while jet evaluation remains CPU Bernstein versus CUDA tensor de Casteljau. CPU parity cannot independently certify the solver. Exact rational truth at the returned binary64 UV verifies final distance/gradient/Hessian; analytic plane/cylinder/parabolic fixtures verify actual solution and classification where known. No general reference optimizer can prove global nearest for an arbitrary nonconvex carrier merely by agreeing.

## P100 implementation and benchmark implications

Start with one query per thread and the same bounded loop on CPU. Keep current UV, target, six-jet, residual/system scalars and trial values inside each thread. Do not allocate an all-query six-jet array per iteration or download intermediate jets. Only final candidates/status/classification/counters leave the solver. Resident patch/query storage and retained source provenance follow the iteration 1 ownership boundary.

The existing order-2 CUDA evaluator already uses 72 registers and a 192-byte stack frame in its measured compilation. A solver adds state and can increase registers/local memory; inspect the resulting compiler resource report and measure before choosing a block size or describing it as register-resident. P100 FP64 arithmetic is sufficient; tensor-core-based recent algorithms do not apply to this hardware. No FP32 solve or reduced-precision acceptance shortcut belongs in the first contract.

Bounded per-thread iterations and backtracking will cause warp divergence. First measure distributions of accepted updates, value evaluations, fallback use and terminal statuses; later compaction/wavefront scheduling is justified only by those measurements. Do not delete difficult requests to improve throughput. Benchmarks must use the same seeds, targets, caps, tolerance, classification policy and required outcome checks for CPU and CUDA. Report unresolved fraction and stationarity/error distributions alongside wall time, with upload, resident solve including output allocation, download, host adapters, validation and cleanup scoped separately.

## Questions the integration design must settle

Choose the public status/flag encoding and whether raw unresolved candidates are downloadable through a distinct candidate-valid indication. Specify exact iteration/evaluation counter semantics and whether backtracking is fixed or configurable. State the coordinate adapter's target-input contract separately from the existing jet-output adapter. Choose the conservative initial weak-boundary classifier or the complete tiny critical-cone enumeration. Qualify proposed constants against the independent corpus before making performance claims. These are concrete integration choices; none require importing a library or claiming global projection correctness.
