# Iteration 3 numerical design input: bounded automatic discovery

2026-10-14. Proposed numerical unit for integrated design/plan review. No product
or test code is authorized by this document. Foundation:
[bounds study](2026-10-14-bounds-numerical-study.md), its pinned source/PDF catalogs,
and the existing owned [projector](../native/projection_solver.hpp).

## Scope and observable claim

For one named rational patch and an exact supplied binary64 local target,
automatically select feasible UV candidates and return one best incumbent plus
a conservative squared-distance bracket `[L2,U2]`. The exact minimum squared
distance on that patch's closed `[0,1]^2` domain satisfies `L2<=d*2<=U2`.
The selected feasible point's exact squared distance is at most `U2`.
These are claims about the supplied binary64 homogeneous controls, not an
ideal analytic source face. There are no trims, cross-patch metric comparisons,
global CAD-body claims, stationary-root enumeration or uniqueness/minimum
classification for the returned UV. A conservative bracket can still be wide.

Return eight SoA doubles per row:
`[s,t,approx_x,approx_y,approx_z,approx_distance2,L2,U2]`, and six u32 arrays:
`[status,flags,visits,pruned,refinements,evaluations]`. Approximate geometry is
descriptive and cannot be used as a certified upper witness. The latter is
`U2`, produced by a fresh interval evaluation of the original patch.

Gap option: `tolerance_squared in [0,1]`, finite, default `2^-24` in **local
squared-distance units**. Test `up(U2-L2)<=tolerance_squared`, after checking
`0<=L2<=U2` and finite endpoints. This is an additive squared-objective guarantee,
not a distance tolerance and not a UV error. Zero tolerance demands an exactly
zero conservative gap and carries no termination promise.

`GAP_SATISFIED`, `COVERAGE_COMPLETE`, `VISIT_LIMIT` and `REFINEMENT_LIMIT` are
independent facts. The valid-row status can be `GAP_SATISFIED` or `UNRESOLVED`,
derived from the final gap; flags still show caps. Gap-qualified cap results are
valid because the returned lower bound falls back to the conservative root
bound. Complete traversal means every domain subtree was bounded or excluded;
it does not mean every local minimum was discovered. The witness exists even
when the projector rejects chart rank or reports stagnation.

## Owned arithmetic and admissible environment

Retain degrees0..3, canonical `i*(q+1)+j` homogeneous records, weights `[1,256]`,
`abs(Ak)<=W`, and target components `[-4,4]`. Include degree-zero, collapsed,
curve-like, self-overlapping and rank-deficient carriers. Rejected patch
metadata/control envelopes fail hierarchy creation as a batch; never omit an
invalid patch and publish a partial cache disguised as complete.

CPU interval elementary operators use one binary64 RN operation, store its
binary64 result, then expand outward by one `std::nextafter`. Lower and upper
results are expanded independently at every primitive. Multiplication/division
of general intervals uses all endpoint products/quotients; de Casteljau
coefficients are nonnegative but numerator intervals can cross zero.
No ordinary compound expression expanded once at its end is an interval
operator. Use no implicit FMA, reassociation, approximate reciprocal or fast
math. Require `sizeof(double)==8`, radix2, digits53, IEC559, and no excess
intermediate evaluation precision (`FLT_EVAL_METHOD==0` on the qualified build).

First qualified CPU environment is Linux x86-64 with the existing owned C++17
compiler. Validate the actual CPU builder caller and each numerical worker:
standard rounding state RN, MXCSR rounding bits RN, FTZ/DAZ clear, masked
floating-point exceptions for non-stop arithmetic. Do not infer worker state
from context creation or from another thread. Do not change the environment.
Unsupported execution environment fails the batch `BG_UNSUPPORTED`, with no
result/hierarchy publication. All started workers join before unpublished
outputs are released. Other platforms need an explicit equivalent adapter,
otherwise refuse this numerical capability; the old evaluator/projector scope
does not change. Empty discovery batches do no arithmetic, start no workers and
may publish an empty result after ordinary structural/options checks.

Hierarchy creation performs CPU interval arithmetic even for a CUDA engine, so
its nonempty builder environment must pass these checks. Empty patch sets need
no arithmetic/environment check; numerical queries cannot select a valid slot
from them. The runtime owner should integrate exact environment inspection in
its batch admission/setup, not expose a per-row geometric status for it.

CUDA query arithmetic uses explicit `__dadd_rd/ru`, `__dsub_rd/ru`,
`__dmul_rd/ru`, `__ddiv_rd/ru` with pinned CUDA12.8/sm_60 and existing strict
flags. CUDA filtering/witness intervals can be tighter than the CPU RN-expanded
ones. CPU-built hierarchy AABBs are identical uploaded bytes for both backends;
query lower/upper endpoints and pruning workloads need not be identical.
Each backend is independently checked against exact-input quantities.
No square root is required by the first squared-gap API.

Every interval operation checks finite endpoints and ordered bounds. Check NaNs
before min/max. After intersections, any empty interval is an invariant failure.
Division requires a strictly positive lower denominator endpoint. Exact known
ranges permit intersecting weights with `[1,256]`, numerators with `[-256,256]`
and Cartesian values with `[-1,1]`. Do not reject a homogeneous interval because
its unrelated A/W endpoints fail the scalar `abs(A)<=W` input predicate.
Input subnormals and signed zero remain admitted; flushing them is unsupported.

## Complete cached hierarchy

Build a complete binary midpoint tree once for the exact retained patch
snapshot; default total split depth8, maximum12, depth0 allowed. Alternate u/v
by level, beginning with u, skipping any degree-zero axis. If both degrees are
zero, publish a single root leaf regardless of requested depth. No sampled
flatness/rank shortcut may remove parameter-domain coverage. A degree-nonzero
constant-image patch still gets the requested tree.

Each edge of each homogeneous channel uses
`lo=down(.5*down(lo0+lo1))`, `hi=up(.5*up(hi0+hi1))`.
Extract left/right de Casteljau triangle edges, preserving canonical net
indexing. Induction over these outward affine averages encloses the exact
restriction of the original patch. Cartesian child controls use all four
outward A/W endpoint quotients; positive rational weights put the child image
inside their Cartesian control hull and thus its AABB. Intersect child boxes
with parent boxes and the known cube. No empirical epsilon padding.

Use a deterministic preorder node layout: left child first, right child second.
Store exact dyadic cell integer coordinates plus exponent/depth; reconstruct
endpoints and centers with power-of-two scaling. The inactive axis remains
`[0,1]` with midpoint.5. Root/leaf flags and child IDs are explicit; avoid
inferring degree or ancestry from memory addresses. Integration chooses the
canonical80-byte packed node record. Every index/extent/byte calculation is
checked before cache publication. Depth12 bounds nodes by8191 and leaves4096.

Temporary interval controls cost at most1024 bytes per full bicubic net; the
builder can keep depth-bounded DFS scratch and release nets after descendants
are processed. Uploaded query nodes contain boxes/IDs, not interval control
nets. The CPU exact input controls stay retained, and CUDA reuses the existing
original patch device controls. Hierarchy changes never overwrite those inputs.
Cache failure publishes nothing; partial node arrays are cleaned up through
the existing runtime resource ownership.

## Bounds and fresh original-patch witnesses

For a target and an AABB, each axis's lower gap is0 if inside, otherwise
`max(0,down(lo-X))` or `max(0,down(X-hi))`. Square gaps downward and sum downward
in fixed x-then-y-then-z order; clamp the result to0. This lower bound never
exceeds the exact distance squared of any point in that node's original patch
restriction. Count every attempted box-distance calculation, including an
arithmetic-failing one, as one visit.

For a valid UV, interval-evaluate only the original patch value. Tensor
de Casteljau starts from singleton input controls. Enclose `1-s` and `1-t` by
outward subtraction too; they are not assumed exactly representable. Endpoint
branches may copy exact edge/corner controls. Apply all needed interval
multiply/add operations outward; then dehomogenize with positive interval W
and intersect the known cube. For each coordinate point enclosure, subtract the
target outward, take the maximum endpoint absolute value upward, square upward
and sum upward in x/y/z order. This produces a finite nonnegative `U_candidate`.
No ordinary solver distance, approximate XYZ or stationarity success is used in
this computation.

Select a new best witness **only when** `U_candidate<current_U`; retain the first
on equality. Copy its exact binary64 UV, approximate XYZ/d2 and interval upper
bound together. All witnesses are feasible by UV/domain identity. Backend
rounding differences may choose different near-tied witnesses; exact-input
qualification attacks their individual brackets, not bitwise result parity.

## Initial witness and bounded projector reuse

Always create a root midpoint witness `(.5,.5)`: one ordinary position
evaluation for descriptive approximate XYZ/d2, plus one fresh interval witness
evaluation. This works even when refinement budget is zero. On a constant or
singular patch it supplies a feasible point without any derivative-based solve.

Count that ordinary root position evaluation in `evaluations` as well as every
callback inside the owned projector. This gives meaningful approximate XYZ
fields with no hidden ordinary evaluator call. The chosen convention is
`evaluations=1+sum(projector.evaluations)` on valid rows.

If budget remains, invoke `brepgpu::project_local` first at the root midpoint;
then once per reached unpruned leaf midpoint. Skip the root leaf's duplicate
midpoint because that seed was already attempted. No other deduplication or
stochastic seed selection. All solves use the original patch's full unit box,
not the subcell, and may move outside the leaf. Such movement is fine for an
upper witness; it does not resolve other UVs in that leaf.

Reuse fixed iteration2 decisions with an internally prepared
`bg_projection_options`:32 accepted updates,24 maximum halvings, gradient
tolerance `2^-24`, flags/reserved0. The established bound is
`1+32*(24+2)=833` ordinary evaluator invocations per solve. `max_refinements`
is a count of actual `project_local` calls, range0..64, default8, including the
root-first call. Therefore the recommended evaluator counter is at most53313.
All reached leaves remain bounded after this budget is spent; do not
interval-sample new leaf midpoints once refinement is withheld.

CPU supplies the existing scalar Bernstein evaluator: one original
`bg_patch_desc` retaining its original control offset, slot0, N1, stack jet
buffers and `evaluate_cpu_range` order0/2. The existing
`projection_cpu.cpp::ScalarCPU` is currently anonymous/private; numerical owner
can expose/move this owned helper in the implementation plan, with ABI1/2
behavior unchanged. CUDA reuses `ScalarCUDA` over the original net and its
independent tensor derivative evaluator. `project_local` stays the shared
bounded decision loop, not a copied new optimizer.

Every returned finite `CANDIDATE_VALID` UV is freshly interval-enclosed,
regardless of projection status STATIONARY, rank unresolved, iteration limit,
line-search limit or stagnation. Sum its reported callback count into the
discovery counter; count the attempted solve itself before invocation. An
arithmetic failure, missing candidate flag, nonfinite fields or out-of-domain
returned UV is a discovery arithmetic failure; no successful certificate is
advertised from partially trusted optimizer state. The bounds algorithm does
not interpret projection curvature flags as exclusions.

On valid rows interval witness evaluations are exactly `1+refinements`.
They are not included in the proposed ordinary evaluator counter and must be
named in documentation/benchmark work accounting. They happen even for
projected candidates that do not improve U. Projection updates need no new
public counter; individual solver update semantics remain in iteration2.

## Deterministic bounded traversal

`max_visits` range1..1048576, default8191. Tree size limits actual visits to8191;
the larger option ceiling simply permits a budget greater than any admitted
tree. One visit means one box-distance calculation, not a pop, stack push,
projection iteration or geometry-value evaluation. Compute the root lower
bound once, counting visit1, and retain it unchanged as fallback `L_root`.

Initialize a fixed-size DFS stack with `(root_id,L_root)`. Depth12 needs at
most13 pending entries, independent of4096 leaves. Store each child's computed
lower bound with its ID so sorting and popping never recompute it. Push the
larger-LB child first, smaller-LB child second, so the more promising child is
popped next; tie goes left. Do not sort by approximate candidate geometry.

For every pop:

1. If stored `LB>current_U`, increment `pruned` and skip that whole subtree.
   Equality is retained. It counts one pruned node/subtree, not inferred leaves.
2. If this is a leaf, accumulate its `LB` into `minimum_retained_leaf_LB` and
   attempt its midpoint projector when eligible/budgeted. When eligible but no
   budget remains, set REFINEMENT_LIMIT and continue traversal. This withheld
   solve does not consume a refinement or evaluator invocation.
3. Otherwise expansion needs **two additional visits**. If fewer than two remain,
   stop traversal before computing either child and set VISIT_LIMIT. Pending
   sibling/subtree coverage has not been dropped: use root lower fallback.
4. Compute both child lower bounds once, count both attempted calculations,
   validate them, sort/push in the stated order, then continue.

Root refinement is outside/earlier than DFS and obeys the same attempt budget.
If max_refinements0, set REFINEMENT_LIMIT because the root-first solve was
withheld. Skip a degree-(0,0) root leaf's repeated seed even when its root-first
solve was withheld; it still has the always-generated initial witness.

No early termination merely because a transient gap passes. Traversal finishes
the fixed tree or reaches the visit cap, giving interpretable final counts and
coverage flags. No dynamic subdivision, adaptive frontier, query-time queue
allocation or publicly emitted candidate arrays are needed in this delivery.

If traversal completes, set COVERAGE_COMPLETE and return
`L2=min(current_U,minimum_retained_leaf_LB)`. There must be at least one
retained leaf: the feasible incumbent lies in some covered leaf whose lower
bound cannot exceed its exact distance or U. If none was retained, fail the
invariant instead of interpreting the domain as empty. Previously pruned nodes
need not be included in this minimum: they were excluded with LB>U_at_prune,
and U only decreases. Retained leaves whose LB later exceeds the new U may
remain in the accumulator harmlessly.

If capped, clear COVERAGE_COMPLETE and return `L2=L_root`; keep the best U
obtained so far. Validate `L_root<=current_U` and the returned nonnegative
ordered bracket. A cap after partial traversal therefore weakens tightness but
never claims false coverage. Gap qualification then uses the final bracket and
can still pass. A consumed budget by itself is not VISIT_LIMIT if no expansion
was withheld; a consumed refinement budget is not REFINEMENT_LIMIT unless an
eligible attempt was withheld.

## Failure outputs and counters

Invalid slot: BAD_PATCH=2, eight NaNs, flags0, counters0. Invalid target: BAD_TARGET=3,
the same outputs. Runtime structural validation and hierarchy construction can
instead reject malformed batches before numerical work, as existing APIs do.
Within a structurally admitted query, failed interval/operator invariants,
ordinary root position failure, optional optimizer arithmetic failure,
nonfinite approximate distance, impossible bracket/stack or corrupted node
state produce ARITHMETIC=4: all eight doubles NaN and only ARITHMETIC flag32. Preserve actual
visits/pruned/refinements/evaluations performed up to failure, including failed
attempts. Never emit a finite successful bracket after a bound failed.

Cap exhaustion is not arithmetic failure: keep all eight finite fields and
CANDIDATE_VALID plus cap/gap/coverage flags as applicable. There is no separate
BOUNDS_VALID flag; every finite CANDIDATE_VALID row in this API has a valid bracket.
`UNRESOLVED` means finite valid evidence with a gap above the requested threshold,
not invalid data. GAP_SATISFIED does not erase cap flags. Device runtime/fatal,
allocation, restoration and partial-download failures are batch failures in
the existing core/backend ownership contract; numerical status must not hide
them. No fallback to CPU is implied for a failed requested CUDA operation.

Record setup construction/interval scratch, cache upload, query upload,
traversal, root approximate/interval witness, projector callbacks, subsequent
interval witnesses and download costs in the named benchmark scopes. CPU/GPU
counts can differ because lower/upper rounding differs. The result is not a
full CAD modeling or GPU topology benchmark.

## Direct review/qualification targets

Exact binary-rational child controls and point distances attack every enclosure
stage; direct polynomial restriction attacks net indexing independently. Analytic
plane, supplied rational cylinder, equal-minima/saddle and continuum cases check
the returned global bracket. Explicit root-only max_visits1/2 cases test two-child
reservation and fallback; refine0 tests root descriptive output and interval
witness; exact exhausted budgets distinguish consumed from withheld work.
Attack strict `>` equality, numerator sign/weight endpoints, underflow and
FTZ/DAZ, duplicate root leaf seed, original-offset scalar CPU reuse, valid rank
failure witnesses, impossible brackets and counter preservation on failed
attempts. Existing finite-corpus evaluator grades cannot replace interval
enclosures. Runtime faults and resource lifetimes are a separate review unit.

Mode-specific owned arithmetic, cache layouts,
primary literature and concrete three-mode delivery measurements are specified
in [the strong-bounds numerical design](2026-10-14-discovery-strong-bounds-design.md).
That input is part of this delivery's numerical unit, not a deferred idea list.
