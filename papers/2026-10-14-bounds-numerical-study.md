# Conservative rational-patch bounds and automatic candidate discovery

Research study, 2026-10-14. This is a recommendation for the next design review,
not an implemented or measured bounds API. Existing evaluator/projection results
do not establish the new containment claims. No reference library is selected.

The useful next delivery is **a complete cached dyadic subpatch hierarchy with
outward boxes, conservative distance filtering, and automatic seeds for the
resident projector**. Every excluded cell needs a valid lower bound compared
with a valid feasible-point upper bound. Every other cell remains accounted for,
including when a depth, visit, output or solver limit is reached. A finite list of
projected seeds cannot establish that every local minimum was found.

Start with one supplied patch per target, in the existing local Cartesian metric.
This still removes manual UV seeding and provides reusable bounds machinery for
later intersections. Choosing the nearest among carriers with different local
frames needs a separate common-metric contract; comparing their raw local
distances is incorrect.

## 1. Source study and recent work

The [source catalog](bounds-numerical-sources.tsv) records pinned files and exact
read scopes; the [PDF catalog](bounds-numerical-pdfs.tsv) records acquired bytes,
hashes, primary URLs and gaps. Existing intact SISL/openNURBS/OCCT archives were
reused, with their existing restoration instructions. No new source extraction
or dependency installation was needed. Text extraction is private working
material; acquired PDFs remain ignored. Bibliography: [bounds-numerical.bib](bounds-numerical.bib).

**MiSo, SIGGRAPH 2025.** Sections 3–5 distinguish exact-domain objectives,
tolerance-qualified outputs and bounded search. The interval backend performs
subdivision and other numerical computations conservatively; merely using
interval range analysis with ordinarily rounded construction is explicitly the
non-conservative backend. Its Minimize algorithm samples feasible points for
upper bounds and prioritizes region lower bounds; capped search returns a wider
interval. Polynomial leaves use Bernstein coefficient hulls, while division is
handled through expression-tree interval operations. The paper includes a
rational sphere example, but specialized rational-polynomial collapsing is
listed as future work. This supports a small owned homogeneous-interval unit,
without adopting its DSL, code generator or NFG dependency. Its benchmark
numbers are not BRepGPU or P100 measurements. [Author preprint](https://fsichetti.gitlab.io/papers/miso.pdf),
[DOI](https://doi.org/10.1145/3731207).

**TIGHT, STAG 2025.** Sections 2–3 separate interval extension from correct
rounding, discuss changing rounding modes versus one-sided arithmetic with sign
changes, and explain why transcendental routines need more machinery. Our first
bounds unit needs only elementary arithmetic, comparisons and optionally square
roots, so the transcendental extension is unnecessary. Its narrowest-interval
claim is stronger than the proposed CPU nextafter expansion; containment is the
needed property here. [Author PDF](https://fsichetti.gitlab.io/papers/tight.pdf).

**Strictly Conservative Neural Distance Fields, SGP 2026.** Sections 3–4 build
distance underestimation from ball covers and 1-Lipschitz networks; the practical
conversion adds a fixed `10^-6` margin against floating-point inaccuracies.
This is interesting future proxy research, but a different shape representation
and arithmetic contract. It does not justify importing a fixed epsilon into
binary-input rational-patch exclusion. We already possess the exact parametric
carrier and can bound it directly. [Author PDF](https://homepages.uni-paderborn.de/campen/papers/Strictly_Conservative_Neural_Distance_Fields.pdf).

**Rational Bézier Guarding, 2022.** Section 3.1 gives rational bisection and the
homogeneous interpretation. Section 3.2 explains how positive weights evolve
under subdivision. Its injectivity/meshing claims are separate from outward
FP64 containment, and its regular-curve arguments must not be used to reject
degenerate patches from our bounds unit. The author's publication page also
links an erratum; no guarding theorem is adopted here. [Author PDF](https://homepages.uni-paderborn.de/campen/papers/Rational_Bezier_Guarding_SGP2022_Paper.pdf).

**Breuel, 2003.** Sections 3–4 explain why an ordinary rounded bound may lose
solutions, and construct enclosures with predecessor/successor floating-point
numbers. This is the relevant foundation for a deliberately simple CPU path.
The primary full text was read in the browser; direct PDF acquisition failed.
[Primary PDF](https://www.dfki.de/fileadmin/user_upload/import/1999_IntervalArithB%2BBpatrec.pdf).

The IMPA SIGGRAPH 2003 notes, chapter 7, discuss priority subdivision, midpoint
upper bounds and overestimation near minimizers. Their convergence conditions
require shrinking cells and shrinking range enclosures; bounded FP64 execution
cannot silently claim the limiting theorem. [Primary notes](https://www.visgraf.impa.br/otim-03/course-notes.pdf).

The 1996 Yamada/Yamaguchi homogeneous-box paper addresses mixed-sign rational
weights. Only its publisher abstract was accessible; it is in the acquisition
shopping list. Mixed-sign weights are outside the proposed positive-weight
unit, and none of its unseen algorithms are assumed. [Publisher](https://doi.org/10.1007/BF01782323).

## 2. What the pinned source actually does

SISL `s1711.c` selects `rcoef` and dimension plus one for rational surface
subdivision. It uses Oslo transformation coefficients and ordinary multiply/sum
arithmetic. `construct.c::newCurve`/`newSurf` derive Euclidean `ecoef` by ordinary
division. `s1992.c` computes axis-aligned and rotated control-polygon boxes from
`ecoef`; rotation uses ordinary arithmetic. These are useful structural
references, not an outward-enclosure implementation. `s1712.c` is a curve-part
extraction routine, not the primitive surface splitter; do not infer its role
from its adjacent number. [Pinned SISL splitter](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s1711.c),
[pinned boxes](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s1992.c).

openNURBS `ON_BezierSurface::Split` packs the other surface dimension into a
nonrational curve of homogeneous-vector dimension and calls
`ON_BezierCurve::Split`. The midpoint branch computes `0.5*(p+q)` ordinarily.
Surface `GetBBox` delegates to point-list bounds; the double-precision workhorse
dehomogenizes with `1/w` followed by multiplication and min/max, without directed
rounding. Zero weights can be skipped with failure recorded. Its `ON_Interval`
parameter domains are not arithmetic enclosures. [Pinned splitter](https://github.com/mcneel/opennurbs/blob/eb92af3ba1806b0a34a99aba0d3bda83e3d46083/opennurbs_bezier.cpp),
[pinned box workhorse](https://github.com/mcneel/opennurbs/blob/eb92af3ba1806b0a34a99aba0d3bda83e3d46083/opennurbs_bounding_box.cpp).

OCCT 8.0.1 now routes `BndLib_AddSurface` through `GeomBndLib_Surface`.
`GeomBndLib_BezierSurface::Box` uses the full control-pole hull and enlarges by
caller tolerance. Its parameter-subrectangle branch falls back to grid
sampling; `BoxOptimal` combines sampling/deflection and PSO/Powell adjustment.
These inspected paths do not maintain an interval-valued homogeneous control
net or certify all unsampled extrema of the exact binary-input restriction.
The full-pole hull is the useful mathematical pattern; an API name containing
“Box” or “Optimal” is not sufficient evidence for our stronger exclusion claim.
[Pinned Bézier bounds](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKGeomBase/GeomBndLib/GeomBndLib_BezierSurface.cxx).

## 3. Exact object and arithmetic boundary

The object to enclose is the exact rational surface defined by the **supplied
binary64 homogeneous records**, not a real-valued cylinder idealization and not
a nominal rounded child patch:

`H(s,t) = sum_ij B_i^p(s) B_j^q(t) (A_ij,W_ij)`, `S = A/W`.

Retain degrees `0..3`, canonical `i*(q+1)+j` packing, `W_ij in [1,256]`,
`abs(A_ij,k)<=W_ij`, closed normalized domain `[0,1]^2`, and exact binary64
target components in `[-4,4]`. A zero-degree axis remains a valid carrier axis;
constant, curve-like, rank-deficient and self-overlapping images are valid for
bounds. No chart derivative or projection stationarity is needed for
containment. NaN, infinity, invalid metadata and nonpositive weights are
rejected before hierarchy publication.

Treat each input component as the singleton interval `[x,x]`. An interval
primitive must enclose the real elementary operation on its supplied endpoints.
Notation below: `down(expr)` and `up(expr)` mean the complete expression has
been decomposed into explicitly outward elementary operations, not one ordinary
compound expression expanded once afterward.

The envelope provides useful exact invariants. Every exact subdivided weight
stays in `[1,256]`; every exact numerator component stays in `[-256,256]`; and
every exact Euclidean subpatch control remains in `[-1,1]`. These allow safe
intersection with known global ranges. Independent numerator/weight intervals
lose correlation, so do not test `abs(A_endpoint)<=W_endpoint` and reject a
valid child. Bounds controls are private interval data, not ordinary product
patches that must pass the existing scalar-control validation again.

## 4. Owned subdivision and Cartesian enclosures

Use binary midpoint subdivision of one parameter axis at a time. For each
homogeneous component and de Casteljau edge, compute

`L = down(0.5 * down(L0+L1))`,
`U = up(0.5 * up(U0+U1))`.

Copy triangle edges into the left/right child net. Apply this independently to
all rows/columns and all four homogeneous channels. Degree at most three makes
the triangle small and statically bounded. Fixed midpoint splitting avoids
general split-parameter interval multiplication and keeps cell coordinates
dyadic. Do not repeatedly dehomogenize, split Cartesian controls and attempt to
restore weights: the affine averaging identity lives in homogeneous space.

Containment follows by induction: singleton root controls enclose themselves;
addition and multiplication by positive `1/2` are monotone; outward endpoints
therefore enclose each exact new control; de Casteljau's restriction identity
then describes the original surface on that child domain. This is an actual
algorithm argument tied to elementary arithmetic, not a sampled geometry test.

For a child control with `A in [a,b]` and `W in [c,d]`, require `c>0`. Compute
the lower quotient as the minimum of the four downward endpoint divisions and
the upper as the maximum of the four upward divisions. Sign-specialized formulas
can reduce divisions after review; starting with four makes negative and
zero-crossing numerator cases explicit. `[a/c,b/d]` is not generally correct.
Use direct division, not a reciprocal/multiply shortcut without a separately
validated composition. Intersect the Cartesian result with `[-1,1]`.

Take the minimum lower and maximum upper Cartesian control endpoint in each
axis. Because positive rational Bernstein coefficients normalize to nonnegative
weights summing to one, the entire child image lies in this AABB. Intersect a
child AABB with its parent AABB to enforce nesting; both contain the actual
child. An empty interval intersection is an arithmetic/invariant failure, never
permission to prune a cell.

At most three averaging stages occur along a single cubic row for one split.
Interval widths accumulate with depth and can stop improving. This creates
false-positive candidates or a wide final distance bracket, not false-negative
exclusion. No fixed depth establishes nearest-point accuracy for every accepted
patch. Finite splitting cannot isolate a continuum of equally nearest UVs.

## 5. Lower bounds, witness upper bounds and exclusion

Let `box=[lo,hi]` enclose a child and `X` be the exact supplied target. For each
axis choose a nonnegative lower gap:

- If `X<lo`, `g=max(0,down(lo-X))`.
- If `X>hi`, `g=max(0,down(X-hi))`.
- Otherwise `g=0`.

Then `LB2=down(down(gx*gx)+down(gy*gy)+down(gz*gz))`, with fixed pairwise
addition order and a final clamp to zero. This is no larger than the squared
distance to any point in the actual child. A nearest-looking **sample** or an
ordinary numerical closest point on the convex hull does not supply such a
lower bound: a feasible hull point gives an upper bound to hull distance.

An incumbent must include patch identity and an exact binary64 feasible UV.
Use a fresh outward value-only evaluation on the **original** controls at that
UV. Interval de Casteljau must enclose `1-s` and `1-t` too: using an ordinarily
rounded complement as an exact coefficient changes the enclosed expression.
Endpoint-specialized evaluation may copy exact edge/corner controls. General
interval multiplication uses all endpoint products. Intersect known positive
weight and local Cartesian ranges as above.

If this produces `S_k in [l_k,h_k]`, bound each residual with outward subtraction,
take an upward maximum absolute residual, square upward and sum upward to obtain
`UB2`. It bounds the distance of this actual feasible patch point, hence the
global minimum from above. A candidate is usable even if the resident projector
reported iteration limit, stagnation, rank unresolved or negative curvature,
provided its UV is valid and the fresh enclosure succeeds. Do not use the
projector's ordinary `distance2` as a certified upper bound.

**Nearest-preserving exclusion:** discard a cell only if `LB2 > UB2`.
Equality is retained. This preserves every exact global minimizer, including
ties, on the explicitly covered patch domain. Replacing `>` with `>=` loses
equal-distance regions. Lack of an incumbent means no distance exclusion.

**Optional candidate band:** if the requested meaning is “retain every cell
which might contain a point within distance `tau` of the incumbent”, compute
`T=up(up_sqrt(UB2)+tau)` and discard only if `down_sqrt(LB2)>T`. This is a
conservative superset of that band; it is different from nearest-preserving
pruning and must be named separately. Adding a tolerance to a squared distance
does not implement a distance tolerance.

For complete leaf coverage, the minimum leaf `LB2` is a global lower bound.
If pruning/refinement is used, maintain a lower bound for every terminal region
and the entire unvisited frontier. A safe returned bracket is
`[min_region_lower, best_witness_upper]`; tighter bookkeeping may discard
pruned-region lower bounds only after proving they cannot lower this minimum.
In particular, do not reinterpret an empty work queue as a domain with no
nearest point: a valid nonempty closed patch has a closest point.

A distance-qualified witness can be reported if
`up(up_sqrt(UB2)-down_sqrt(LB2_global)) <= tau`.
This proves the selected feasible point is within `tau` in distance of the exact
minimum, without proving UV isolation, local minimality or uniqueness. A
squared-gap option instead proves `UB2-LB2_global<=delta2`; its units differ.
Validate `0<=LB2_global<=UB2` before any success predicate; a reversed bracket
is an invariant failure, not a negative gap that passes tolerance. An infinite
upper bound carries no finite distance-qualification success.
Keep squared bounds as the core output if square-root environmental support is
not yet qualified. The first discovery delivery can expose the bracket and
retain `UNRESOLVED` when it is wide, without offering either success predicate.

## 6. CPU C++17 and CUDA 12.8 / sm_60 feasibility

For an owned CPU baseline, perform one elementary binary64 operation in
round-to-nearest mode, then expand its stored result with
`std::nextafter(result,-infinity)` and `std::nextafter(result,+infinity)`.
One adjacent representable value on either side contains the exact elementary
result under correctly rounded binary64 execution, including gradual
underflow. It intentionally widens exact operations too. Use the expanded
endpoints at the **next** operation; expanding only the final compound result
does not account for preceding errors. Do not rely on `long double` being
wider or having the same format on all hosts.

The claim is qualified to owned compiler/platform execution, not bare C++17
portability. Require radix 2, 53-bit double significand, IEC-559 behavior,
binary64 evaluation without excess intermediate precision, preserved signed
zeros/NaNs, no reassociation, reciprocal substitution, implicit contraction or
fast-math, gradual subnormal input/output handling and masked floating-point
exceptions for the owned non-stop arithmetic path. Check the active worker
environment, not only the creating thread. FTZ/DAZ can invalidate one-nextafter
containment because a flushed nonzero value need not be adjacent to zero.
Checking only `fegetround()==FE_TONEAREST` does not check FTZ/DAZ.

Two alternatives deserve explicit design choice: reject unsupported arithmetic
environments without touching them, or save/select/restore the full supported
platform environment around each owned worker operation. Standard fenv alone
does not expose every FTZ/DAZ control. Runtime rejection is the smaller first
contract; an owned x86 adapter can later save/restore MXCSR, alongside standard
rounding state, without leaking altered state into callbacks. Dynamic directed
host rounding is another feasible implementation but needs compiler treatment
of fenv and measured mode-switch cost. GCC labels `-frounding-math`
experimental; it is not by itself a universal conservatism certificate.
[GCC documentation](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html),
[Clang environment/denormal documentation](https://clang.llvm.org/docs/UsersManual.html#floating-point-environment).

CUDA can directly use `__dadd_rd/ru`, `__dsub_rd/ru`, `__dmul_rd/ru` and
`__ddiv_rd/ru`; optionally `__dsqrt_rd/ru`. CUDA 12.8's programming guide
Table 20 identifies these FP64 arithmetic intrinsics as IEEE compliant and the
division requirement is satisfied by sm_60. Use explicit intrinsics and the
existing strict compile flags (`ftz=false`, precise division/square root,
implicit FMA off); no device global rounding mode is needed. GPU exceptions
are masked, so inspect nonfinite/invalid intervals explicitly. Check for NaN
before `fmin/fmax`, which can otherwise return their non-NaN operand and hide a
failed bound. [Pinned guide](https://docs.nvidia.com/cuda/archive/12.8.0/cuda-c-programming-guide/index.html#intrinsic-functions),
[pinned FP64 API](https://docs.nvidia.com/cuda/archive/12.8.0/cuda-math-api/cuda_math_api/group__CUDA__MATH__INTRINSIC__DOUBLE.html).

CPU expanded-RN and GPU directed endpoints need not be bitwise equal or equally
tight. Each must satisfy the exact-input enclosure claim independently. Wider
CPU boxes may retain more cells; report those workload differences rather than
calling timing of unequal candidate work a pure hardware speedup.

No ordinary overflow is expected in this bounded unit: homogeneous averaging
adds magnitudes at most roughly 512; local distance residuals are at most 5,
and squared distances at most 75 before small outward widening. Subnormal
numerators and squared gaps remain valid inputs. Keep checked arithmetic and
finite interval validation; the envelope is not permission to ignore failures.

## 7. Practical first delivery and resource limits

Build an immutable, complete binary tree once per patch snapshot. Alternate
nonzero-degree axes; use the only varying axis for curve-like carriers; a
degree-(0,0) patch is one leaf. Recommend initial default **total split depth 8**,
maximum 12, as an explicit resource scope to review, not an accuracy guarantee.
Depth 8 gives 256 leaves and 511 nodes for a genuine two-axis carrier. Depth 12
gives 4096 leaves and 8191 nodes. Store cell indices/exponents, not accumulated
floating domain endpoints; at these depths dyadic endpoints and centers are
exactly representable. Retain closed cells so shared-boundary points are covered
by either adjacent leaf; duplication is harmless and can be deduplicated only
as an output convenience.

A full bicubic homogeneous interval net costs
`16 controls * 4 channels * 2 endpoints * 8 bytes = 1024 bytes` per node.
Keeping that for every depth-12 node costs about 8 MiB per patch before metadata.
The runtime query needs only 48 bytes of AABB plus cell/patch metadata at each
node. A CPU builder can release interval nets once descendants and boxes are
constructed, then upload the immutable traversal cache. This is a useful first
resident acceleration even when setup is CPU-owned. A GPU builder is a later
measured alternative; computing/storing 128 doubles per active node in a
query thread would be a poor default register/stack contract.

For bounded automatic discovery, scan/traverse all covered nodes, obtain a
feasible witness from root midpoint and valid projected candidates, reject only
proven-distant nodes, and emit retained leaf IDs with midpoint seed UVs. Reuse
the existing projector on the original patch; midpoint seeds may converge
outside their cell, which is fine for an upper witness but does not resolve that
cell's remaining candidate set. If restricted-cell projection is later added,
its feasible-box and derivative transformations need a separate contract.

Make output capacity explicit. A top-K promising seed list can be useful, but
discarding other retained cells loses candidate completeness. Report exact
retained count plus an overflow/unresolved flag, and preserve their lower-bound
contribution; alternatively allocate a full bounded leaf list. A visit cap must
retain a lower bound for unvisited subtree roots. Returning complete compact
records or replayable frontier IDs is needed if the caller must resume discovery.
No queue overflow may silently discard a node. Refuse excessive cache size
before publishing the cache; count checked bytes as well as nodes.

Proposed facts to expose: patch snapshot identity; hierarchy depth/node/leaf
counts and setup bytes; supplied target; witness UV and interval distance;
global lower bound; retained/pruned/visited counts; emitted capacity/count;
coverage-complete versus capped/output-incomplete status. Use separate flags
for a complete conservative filter and a tight distance bracket. “Complete”
means all domain cells were processed/accounted for, not all stationary roots
were enumerated. Report setup/upload, filtering, projection and download costs
separately; no performance claim is established by this research.

## 8. Stronger bounds worth keeping in view

The rational control hull is stronger than its AABB, but a floating quadratic
program's approximate hull projection is not automatically a valid lower bound.
A later support-plane method can use an arbitrary finite direction with an
outward upper bound on all control dot products and a lower bound on the target
dot product, then divide a positive separation by an outward upper direction
norm. It can refine a lower bound using a validated dual witness without needing
an exact primal hull optimizer. AABB distances remain the simpler owned start.

Another option bounds the objective directly. For degree-(p,q) homogeneous
`A,W`, the squared distance is
`sum_k (A_k-X_k W)^2 / W^2`, with numerator and denominator degrees at most
`(2p,2q)`, hence at most 49 Bernstein coefficients for bicubics. Exact
Bernstein-product factors are positive; denominator coefficients are positive.
Enclosed coefficient ratios therefore give a rational objective range by the
same normalized-weight argument. Numerator coefficients may be negative even
though the function is nonnegative; clamp the lower range to zero. This
target-dependent higher-degree construction can outperform loose spatial boxes
in some cases but costs more preprocessing, rounding analysis and per-target
storage. It is an owned future comparison, not required for the first hierarchy.

## 9. Direct qualification obligations

Use exact rational arithmetic on supplied binary64 inputs to compute child
homogeneous controls at dyadic cells and feasible-point distances. A direct
restriction/basis oracle can independently attack child indexing; a literal
copy of product subdivision alone cannot. Check every stored interval endpoint
against exact quantities, not only CPU/CUDA parity and sampled images.

Necessary attacks include all 16 degree pairs; W=1/256 and asymmetric weights;
negative/zero-crossing numerator intervals; cancellation to signed zero;
`denorm_min/2` midpoint underflow; subnormal inputs under deliberately altered
FTZ/DAZ; endpoints and nextafter parameters; cells sharing an exact minimizer;
targets on a face/edge/corner and exactly at a bound threshold; near-threshold
strict pruning; original-input versus nominal-child witnesses; axis/layout
swaps; point/curve/rank-deficient carriers; repeated equal minima and continuous
nearest sets. A degree-zero exact point with `A/W=1/3` directly attacks rounded
dehomogenization being mistaken for an upper distance witness.

Known plane and rational-cylinder patches provide analytic distance checks
against their **supplied** rational controls. The existing double-well projection
fixture supplies two equal minima and a stationary saddle: candidate discovery
must retain both potentially nearest cells even if one projected seed reaches
the saddle. Disabling interval expansion, reversing quotient endpoints,
rounding the lower gap upward, reusing ordinary projector distance, pruning
equality, and dropping a capped frontier should each be attacked directly.
Mutants run on the qualified target fleet, following repository doctrine.

The enclosure induction, the documented elementary arithmetic semantics and
these direct exact-input oracles bear on distinct fault classes. No finite test
sample alone proves containment for every real UV, and no generic extra proof
framework is needed to state and review this small algorithm.

## 10. Decisions still needed before implementation

Review the one-patch/common-local-metric boundary, depth/cache/output caps,
CPU environment rejection versus save/restore, squared-distance versus distance
tolerance semantics, and whether first delivery returns the complete retained
cell list or a count plus explicit truncated seeds. Choose CPU-built boxes
uploaded to GPU versus independently built CPU/GPU caches with workload
differences reported. Keep later adaptive refinement, world-frame transforms,
trim membership, cross-carrier nearest selection, root enumeration and certified
isolated minima separate. The proposed first scope gives automatic useful
seeds and trustworthy exclusions without those additional claims.
