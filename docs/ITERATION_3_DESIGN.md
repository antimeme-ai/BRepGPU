# Iteration 3: automatic candidates with conservative distance bounds

Reviewed design, 2026-10-14. This operation covers an
untrimmed, named rational Bézier patch in its existing local Cartesian metric.
It removes required UV seeds and reports a conservative squared-distance
interval. It does not enumerate minima, isolate UVs, handle trims or compare
different patch frames. Implementation follows design and plan review.

[Numerical review](../papers/2026-10-14-discovery-design-numerical-review.md),
[runtime review](../papers/2026-10-14-discovery-design-runtime-review.md) and
[oracle review](../papers/2026-10-14-discovery-design-oracle-review.md) found
no remaining blockers after the documented arithmetic, cache-order, alias and
deterministic-traversal clarifications. The inputs retain their proposals and
review findings; this document records the integrated choices.

## Purpose and literature

The input contract and owned scalar evaluator/projector remain those of
iterations 1–2: degrees 0..3, positive homogeneous weights 1..256, numerator
magnitudes at most their weight, normalized closed UV domain and targets in
[-4,4]^3. Supplied binary64 controls and targets define exact rational data.
Point-like, curve-like, folded and nonunique carriers remain valid.

[Completed studies](ITERATION_3_STUDY.md) supply homogeneous restriction,
interval arithmetic, feasible witnesses, independent coverage oracles and
packed residency. The numerical, runtime and oracle design inputs refine those
mechanisms. MiSo/TIGHT and earlier interval subdivision inform containment;
SISL/openNURBS/OCCT inform geometry organization; GPU traversal and Wafer/GPU
MODE inform measured scheduling. None is an adopted dependency.

The operator also requested serious performance trials of stronger geometry
methods. This delivery compares AABB, fixed-direction control-hull support
slabs (26-DOP), and rational Bernstein squared-distance coefficient bounds.
The last method uses a much larger query-independent polynomial cache; its
49-coefficient bicubic arithmetic and amortization participate in the measured
comparison rather than being dismissed as arcane.

## Immutable hierarchy

`bg3_create_hierarchy` retains the existing patch snapshot. The CPU constructs
complete binary midpoint trees, starting with U and alternating U/V when both degrees are nonzero,
using the varying axis otherwise. A degree-(0,0) carrier is a single leaf.
Default total split depth is 8; accepted depth is 0..12. At depth d a varying
carrier has 2^(d+1)-1 nodes. Checked total nodes must not exceed the caller's
`max_nodes` (default/maximum 1,048,576). Reject the entire build before
publication if storage or arithmetic admission fails; never omit a patch.

Each node records six binary64 Cartesian bounds plus eight u32 fields:
left/right child, patch slot, U index, V index, U level, V level and flags.
This 80-byte POD uses UINT32_MAX child indices for leaves and flag LEAF=1.
Indices address packed arrays. Exact logical cell identity is the integer
dyadic rectangle, not its array position or a device pointer. Root indices are
one u32 per patch. Source lineage stays in the retained patch descriptors.

Singleton input homogeneous intervals are split with outward elementary
arithmetic. Every child interval encloses the exact original-input restriction.
Positive weight and Cartesian envelopes are intersected with known ranges;
each child box is intersected with its enclosing parent box. Empty/nonfinite
intervals are arithmetic failures, never exclusions. Recursive construction
keeps temporary interval nets bounded by depth rather than storing them in
every query thread or uploading them with the immutable cache.

Build mode DOP26=1 enables ten additional support slabs per node,
stored separately as 20 doubles per node, in lower/upper pair order. Directions
are (1,1,0),(1,-1,0),(1,0,1),(1,0,-1),(0,1,1),(0,1,-1),
(1,1,1),(1,1,-1),(1,-1,1),(1,-1,-1). Their squared norms are exactly 2 or 3.
The three coordinate directions are already represented by the AABB. Support
intervals project each homogeneous numerator along the direction before
dividing by its positive weight interval. They enclose every exact Cartesian
child control projection and therefore the positive-weight carrier.
Intersect with parent supports and known global
direction ranges. DOP26 storage adds 160 bytes/node; default AABB allocates none.

Build mode BERNSTEIN=2 precomputes five interval Bernstein polynomials per node:
C0=sum(A_k^2), C1=Ax*W, C2=Ay*W, C3=Az*W, C4=W^2. Product coefficients use
enclosed exact binomial factors and the interval homogeneous child controls.
Each polynomial has K=(2p+1)*(2q+1) active coefficients, at most 49. Store five
blocks of 49 lower/upper pairs, padding inactive pairs with zeros: 490 doubles,
3920 bytes/node. C0 coefficients may be negative; do not clamp them. Active
C4 coefficients are positive and can be intersected with [1,65536]. Padding
is never part of the query's minimum/division.

This is an enum-valued build choice in the flags field: 0=AABB,1=DOP26,
2=BERNSTEIN. Value 3 and other values are rejected. Bernstein and DOP storage
are mutually exclusive. All mode-dependent checked bytes participate in
admission; allocation/backend OOM publishes no partial hierarchy.

Cache order is node-major. DOP index is node*20+2*direction+endpoint,
where endpoint 0 is lower and 1 is upper. Bernstein index is
node*490+polynomial*98+2*coefficient+endpoint, polynomials C0..C4 in order,
coefficient=a*(2q+1)+b for doubled degrees a=0..2p,b=0..2q.
All remaining coefficient pairs up to 49 are exactly zero padding and ignored
by traversal. These formulas apply to construction, upload and actual download.

GPU hierarchy creation uploads the actual nodes, roots and optional bound cache
once. The same CPU-generated cache is used on CPU/GPU; no GPU builder or sort
is needed in this delivery. Hierarchy download returns actual resident GPU
arrays, enabling exact qualification of storage as well as construction.

## Query and deterministic bounded traversal

A discovery query snapshots u32 patch slots and XYZ targets and retains its
hierarchy. An out-of-range slot or nonfinite/out-of-envelope target is a per-row
failure. Structural errors, failed allocation or backend errors publish no
result handle. Empty query batches remain valid.

Options: `max_visits` 1..1,048,576 (default 8191), `max_refinements` 0..64
(default 8), and `tolerance_squared` finite in [0,1] (default 2^-24).
Flags/reserved are zero. Each refinement calls the existing owned projector
with 32 updates, 24 backtracks and gradient tolerance 2^-24. These are fixed
operation choices, not inferred nearest-point guarantees.

First evaluate the original patch at root UV=(1/2,1/2), using both the scalar
position evaluator for display and a fresh interval evaluation for a valid
upper squared distance. Count the scalar callback in `evaluations`. A valid UV
is a feasible witness without rank or stationarity requirements. If refinement
budget is nonzero, project the root seed and enclose its returned valid UV on
the original patch. Adopt a new witness only if its upper bound decreases.
Skip the root leaf's duplicate midpoint solve, including a depth-zero varying
patch. If the root-first solve is withheld by a zero refinement budget, record
REFINEMENT_LIMIT; otherwise record that reason only when an eligible leaf solve
is actually withheld.

Compute the root lower distance downward. Traverse near child first with a
depth-bounded per-query DFS stack; store computed child lower values with IDs.
Tied child lower values visit the left child first. Nodes are built in preorder,
left subtree before right subtree. Visit counts every box-distance computation, including the root and child
ordering computations. Compute children only if two remaining visits exist;
otherwise stop with VISIT_LIMIT. Pop/prune uses strict LB>current U so equality
is retained. Count `pruned` as rejected subtree roots, not omitted leaf count.

For a reached nonpruned leaf, its lower bound contributes to the terminal
minimum. If refinement budget remains, project its exact dyadic midpoint and
enclose the returned UV on the original patch. Projection may leave that leaf;
it improves a global feasible witness without resolving the leaf's geometry.
Once the budget is exhausted, continue bounding traversal and set
REFINEMENT_LIMIT when an eligible leaf cannot be refined. No extra leaf samples
are performed after exhaustion. `refinements` counts actual projector calls;
`evaluations` counts the initial scalar call plus their evaluator callbacks.
Successful rows perform 1+refinements interval witness evaluations separately.

AABB lower squared distance is the downward sum of squared nonnegative
coordinate gaps. In DOP26 mode, compute target projection intervals once per
query. A slab separation gap uses the target interval endpoint nearest the
slab; its downward squared gap divided downward by the exact direction norm
squared is another valid lower bound. Take the maximum with the AABB lower.
This is a lower bound from individual separating slabs, not an exact distance
to the DOP or control hull. All projections, subtraction, products and sums
use outward elementary operations.

In BERNSTEIN mode compute an interval for q=sum(X_k^2) once. For every active
coefficient, enclose N_i=C0_i-2*sum(X_k*Ck_i)+q*C4_i and divide by its positive
C4_i interval. The minimum of coefficient-ratio lower endpoints, clamped to
zero, is a rational Bernstein objective lower bound: the exact squared-distance
function is a positive-weight average of the exact coefficient ratios. Take
the maximum with AABB distance. This preserves a useful lower bound when some
numerator coefficients are negative. No direct polynomial cache coefficient
is assumed exact merely because it was constructed once.

After complete traversal, L=min(U,minimum unpruned terminal lower). Pruned
regions have lower greater than their earlier incumbent, which is no smaller
than final U, so they cannot invalidate that L. On a visit limitation,
return the already-computed root lower instead of tightening from incomplete
leaves. This conservatively covers all omitted regions. The fixed depth-12
DFS stack has 13 entries; exceeding it is an arithmetic/invariant failure,
not a supported stack budget. Depth exhaustion at a
prebuilt leaf is ordinary terminal coverage, not a fabricated accuracy result.

## Result meaning and arithmetic failure

Eight double SoA fields: s,t,x,y,z,approximate_distance_squared,lower_squared,
upper_squared. Six u32 SoA fields: status,flags,visits,pruned,refinements,
evaluations. Total resident/download payload is 88 bytes/request.

Status values: GAP_SATISFIED=0, UNRESOLVED=1, BAD_PATCH=2, BAD_TARGET=3,
ARITHMETIC=4. Flags: CANDIDATE_VALID=1, GAP_SATISFIED=2,
COVERAGE_COMPLETE=4, VISIT_LIMIT=8, REFINEMENT_LIMIT=16, ARITHMETIC=32.
The gap predicate requires finite 0<=L<=U and upward-rounded U-L no greater
than the requested squared tolerance. It means only squared-distance accuracy.
A capped traversal can have GAP_SATISFIED and VISIT_LIMIT together; it must
not have COVERAGE_COMPLETE. Conversely a completed traversal can remain wide.
COVERAGE_COMPLETE means the tree's regions were processed/accounted for,
not that every minimum was found or every surviving cell emitted.

Any arithmetic inability to maintain the admitted intervals, any invalid
bracket or scalar/projector arithmetic failure returns ARITHMETIC with all
eight doubles NaN and no candidate/gap/coverage flags. Preserve actual counters
and add ARITHMETIC. Local rank refusal, budget/stagnation or saddle diagnostics
are not arithmetic failures: a valid returned UV can still supply an upper
witness. Invalid-slot/target rows use all-NaN doubles and zero work counters.

CPU arithmetic requires owned Linux x86-64 binary64 execution, nearest rounding,
gradual subnormals, masked exceptions and no reassociation/contraction. Reject
an unsupported environment before hierarchy construction; validate every CPU
worker's environment before numerical work. Unsupported worker environment
fails the entire operation before publication rather than returning plausible
rows. Do not change caller rounding/FTZ/DAZ modes. CUDA uses explicit directed
FP64 intrinsics with existing strict sm_60 compile flags. CPU expanded-RN bounds
may be wider than CUDA bounds; equality/parity is not a correctness oracle.

## Additive ABI, ownership and diagnostics

Public header `brepgpu_discovery.h` uses version 3 for its POD descriptors;
diagnostics remain ABI1. Build options are 32 bytes (u32 version,size,depth,
reserved; u64 max_nodes; u32 flags,reserved2); discovery options are 32 bytes
(u32 version,size,max_visits,max_refinements; double tolerance_squared;
u32 flags,reserved). Node is 80 bytes. Hierarchy info is 40 bytes (u32
version,size; u64 patch_count,node_count; u32 depth,backend,flags,node_size).
Cache bytes are derived from node/root counts and mode. Result info follows
the existing 48-byte projection-info shape with field count, backend, profile
flags and optional kernel milliseconds. The ABI-size query records six integer
slabs as well as eight double fields.

Functions provide ABI-size discovery, hierarchy create/info/download,
`bg3_hierarchy_bound_download` for the mode-dependent auxiliary arrays,
query create, discover, result info/download and retained source snapshots.
Source snapshots expose patch descriptors, slots, targets and both option
records. `bg1_close` handles all new kinds in the same checked u64 registry.
Do not append to ABI1/2 tables or change their POD layouts or callbacks.

Private pure optional `brepgpu_cuda_backend_v3` returns a separate static table
whose exact base-v1 pointer matches the instance that created context/patch
tokens. It does not require v2 negotiation; the numeric solver is reused
internally. Missing extension refuses only discovery. Cache only stable
capability refusal/success; transient/fatal errors retain current normalization
and process-fatal state across all operation families.

Hierarchy retains patch; query retains hierarchy; result retains query and
options. Native arrays/tokens stay alive after parent public close and support
source download. Context close invalidates all public children and drains
tokens under existing admission, device save/select/restore, synchronization,
unpublished-token cleanup and fatal-error rules. Python uses the same native
create/wrap-or-close guards; no second registry or destructor-only ownership.

New creator alias preflight runs before zeroing handle output or diagnostic
writeback. Reject overlap of any written output/diagnostic with any read
descriptor/payload without writing aliased storage. Other structurally failed
creators clear a valid disjoint handle output. All descriptors use exact
canonical struct size, capacities may be larger, payload extents are exact.
Downloads validate pointer alignment, products, ranges and all output overlap
before any write. On backend copy failure every destination is unusable.
Every ABI3 info, source and download call also preflights its output descriptor
and all payload outputs against the diagnostic before `call()` can write.
Output-diagnostic alias and creator handle-diagnostic alias return
BG_INVALID_ARGUMENT with every aliased byte unchanged, including on an
otherwise successful operation. Descriptor used extents are canonical sizes.

## Direct qualification and performance decisions

Independent exact binary-input restriction uses Bernstein-to-power conversion,
affine dyadic substitution and power-to-Bernstein conversion, not the product
splitter. Check every Cartesian child ratio, DOP support and Bernstein coefficient against exported
intervals. Check integer parent/child paths and complete domain partition.
Evaluate original controls at returned binary64 UV as Fractions for U; analytic
planes/cylinders/folds/continua bound the true minimum independently.

Attack subnormal midpoint loss, false rounded 1/3 witness, shared boundaries,
ties, negative/zero-crossing projection terms, arbitrary weights, all degrees,
depth/visit/refinement limits, malformed outputs, stale/foreign/wrong kinds,
parent close, Python wrap failures and callback fatal/cleanup paths. Reuse the
existing fault backend seam. Compare actual ABI1/2 binaries/plugins in fresh
processes to ensure additive behavior. Mutation attacks run only on the fleet.

Benchmark CPU 1/24 workers and each P100 on identical targets, admitted caches,
budgets and accuracy requests. Separate build/upload, request ingestion,
resident discovery, compact download, native host consumption and Python
conversion. Report visits, refinements, unresolved rates and bracket widths.
Benchmark AABB, DOP26 and BERNSTEIN with equal numerical contracts; narrower bounds may
perform different useful work, which must be reported along with time.
Choose the default from full-operation measurements and amortization, not a
count of pruned boxes. Do not claim full B-rep modeling acceleration.

Produce owned SVG geometry diagrams and benchmark plots from actual exported
data, with scope/caption/source links in README. Decorative graphics cannot
substitute for geometry or timing qualification.
