# CUDA discovery mode specialization numerical design review

Date: 2026-10-14. Reviewed the design proposal against the actual shared
discovery/projection arithmetic, CUDA kernel, retained hierarchy mode, callback
launch/event/publication path and existing independent qualification sources.
This review contains no product edits, builds, tests, target operations or timing
claims. It belongs to the separate CUDA mode-specialization worktree.

## Disposition

Accepted design; no unresolved arithmetic, output or counter blocker. The owner
integrated the explicit bitwise comparison mechanism and corrected event-scope
wording identified below. Both concrete clarification findings are resolved.

Reviewed checkpoints:

| Source | SHA-256 |
|---|---|
| `papers/2026-10-14-discovery-cuda-mode-specialization-design.md` | `1357469985e1f9870e3b1f4bcb68a7817854adefecf81c9fa5db280e40ff79b7` |
| `native/cuda_backend.cu` | `3da09294aa1d291218344fa004ea0db638f331681ceb0fe633811e2430cda4ca` |
| `native/discovery_math.hpp` | `7a3fe53a15a1e58613432b431ccf6ce5c638b46a40a043f7a42cd53f4e322f39` |

## Constant propagation preserves the numerical unit

The proposed three `discovery_kernel<Mode>` instances pass a compile-time unsigned
constant into the unchanged `discover_local` interface. The entire helper chain is
already `__host__ __device__ __forceinline__` under CUDA. Consequently the mode
comparisons in `prepare_target` and `node_lower` can become constant conditions
without changing their arithmetic source. The proposal does not need a second
solver, templated polynomial degree or new cache layout to expose those conditions.
Actual binary inspection remains necessary to determine whether dead paths and
unused prepared-target members disappear.

Mode conditions select independent code/data before each active bound calculation.
AABB has no support-cache loads and does not read prepared dots/norm; a null cache
is valid. DOP reads exactly ten prepared interval projections and ten node support
pairs, retains norm-squared divisors 2/3, and combines the result with AABB.
Bernstein reads only `(2p+1)(2q+1)` coefficients, preserves the C4-positive guard,
uses C4.lo for a negative numerator lower and C4.hi otherwise, and combines its
minimum-ratio bound with AABB. The degree-(0,0) active extent is one coefficient,
not 49; unused padding remains unread. These facts do not depend on whether mode
is an argument or a template constant.

The directed `__dadd/sub/mul/div_rd/ru` primitives retain their operand ordering
and rounding modes. Constant elimination of an unrelated mode branch does not
authorize reassociation of x/y/z sums, denominator selection, clipping or new
arithmetic identities. Existing CMake flags disable implicit FMA and FTZ and keep
precise division/square root. Scalar position/jet evaluation and the resident
projector remain unchanged, including their explicit nested FMA and RN decisions.
Compiler register allocation, instruction scheduling and live ranges may change;
those changes alone do not change scalar operation results or work counts.

The shared traversal preserves root position evaluation, optional root projector,
fresh original-control interval witness upper, root bound, thirteen-entry stack,
strict lower>U pruning, left<=right tie choice, pairwise child visit reservation,
leaf refinement/root deduplication and capped root-lower fallback. Any changed
returned UV or bound can alter later pruning/counters, which is why all eight
binary64 fields and all six integer slabs belong to the identity comparison.

## Host dispatch and invalid inputs

Dispatch from the retained hierarchy token is correct. `hierarchy_upload` admits
only modes 0/1/2 and records that mode alongside the actual mode-specific cache.
Dispatching from a new caller field would create a cache/type mismatch, and the
proposal correctly excludes it. Keep a default host-switch branch that reports
an existing Operation evaluation error and prevents token publication; never
interpret an unexpected private mode as AABB.

The kernel's slot check must remain first. Valid slots retain root/source checks
before entering `discover_local`; valid source/roots with bad targets retain
BAD_TARGET and eight canonical failed fields. No host specialization moves per-row
validation or changes precedence. Grid-stride indexing and full-query-count SoA
stride are independent of mode and must stay as written. Empty batches continue
to skip both dispatch and profiling events, with zero flags and NaN duration.

The template does not add a resource/lifetime category. The switch stays inside
the existing device/stream scope; all selected launch errors reach Operation,
stream completion and device restoration still precede publication, and pending
result cleanup/fatal propagation stay intact. Device restoration and profiling
checks attack the host dispatch integration; there is no need to rewrite the
unchanged v1/v2/v3 factory or registry machinery.

## Concrete clarifications

### MS-N1 — specify raw-bit output comparison

The design correctly requires bitwise identity, including signed zeros and
canonical invalid NaNs. Implement that literally with raw contiguous field bytes
or binary64 bit views, plus raw integer slabs. Comparing Python float tuples is
insufficient: +0 equals -0, while even equal canonical NaN bit patterns compare
unequal as floats. Do not normalize failed fields or discard signed zeros to make
the comparison pass. Compare identical input/control/query/option snapshots and
exclude only intentionally nondeterministic profile duration/metadata as named.
Retained dynamic plugin plus preserved core/adapter supplies the old execution;
an implementation rebuilt from the old source is a different experiment.

### MS-N2 — event brackets can include host enqueue gaps

The current callback records the start event before the kernel launch; the new
switch sits between those submissions. If the GPU executes the start event before
the host enqueues the selected launch, elapsed event time can include the enqueue
gap. Thus the proposal's categorical statement that host dispatch stays outside
event time is too strong. Allocation remains before the event bracket; later
synchronization/restoration/download remain outside its timestamps, but dispatch
can influence the bracket duration for short work.

Keep the same event locations so old/new comparisons preserve the existing scope.
Describe `kernel_ms` as CUDA-event duration around the single selected launch,
including any intervening stream/host submission gap; avoid promising a pure
device-instruction duration. No event relocation or new profiling subsystem is
needed for this experiment. Resident/native host phases remain separate measured
claims and determine whether the change delivers value.

## Qualification proportional to the change

Raw old/new identity checks the promised unchanged execution results and counters.
The existing Fraction original-input oracle independently checks geometry, cache
containment, witness upper and qualified gap behavior. These are distinct claims;
they are sufficient direct mechanisms without CPU parity as truth or a new
numerical reference. A single runner can collect both plugins' outputs and apply
the existing oracle, sharing exact input truth calculations. It need not invent
a second report layer to certify that runner.

Cross-mode invalid/empty/128-thread tail tests are a useful extension because the
dispatch now creates three entry points. Mixed source offsets/degrees, positive
denominator and inactive padding cases constrain those entry points. Refine budgets
0/1/8/64, caps 1/2/full, maximum depth and profile on/off target real scheduling
and active-path interactions. Exact counters apply where independently specified;
otherwise old/new integer identity preserves the existing bounded work behavior.
Existing exported hierarchy/cache checks remain useful but upload/build arithmetic
is unchanged; do not characterize their repetition as new hierarchy implementation.

Baseline cross-mode tests should pass before specialization. There is no desired
functional red for this optimization. Reuse one independent exact qualification
mechanism and one identity comparison, focused sanitizer/device-restoration checks,
and v1/v2 smoke checks. Re-running the CPU baseline, complete mutation campaign or
all unchanged modeled cleanup scenarios would add cost without a new claim.

Resource inspection is an outcome, not a pass condition. The dynamic kernel's
254-register/448-byte-stack report does not imply occupancy is the bottleneck,
and zero compiler spill loads/stores does not mean zero local-stack memory. Keep
block size, grid cap, budgets, benchmark/core bytes and exact workload conditions
fixed for the first comparison. Preserve aligned/off-grid unresolved populations
and original repeated-target scope. Adopt only for a useful measured native-phase
benefit with unchanged qualified output; otherwise retain the dynamic kernel.

## Integrated clarification disposition

Re-read the corrected design, SHA-256
`b8a74566cbc1b6d87d061e238960fde23ba10c6d5260bbdfe5baec18efc05066`.
MS-N1 is resolved: qualification explicitly compares raw double/integer buffer
bytes, preserving canonical failed NaNs and signed zeros rather than using Python
float equality. MS-N2 is resolved: the event scope is the unchanged stream-event
bracket, explicitly admitting submission/idle gaps and making no pure device-time
claim. No numerical or resource behavior changed in this disposition. Design
accepted for the subsequent implementation plan; no product implementation or
target qualification occurred in this review.
