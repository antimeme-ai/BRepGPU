# Pair runtime/CUDA/wrapper source review

2026-10-14, coordinator review independent of the implementation owner.
Read public/private layouts, runtime additions and pair_runtime.inc, shared
pair_math.hpp, CUDA additions/pair_cuda.inc, Python lazy declarations and the
eighteen new modeled callback scenarios. Initial release build, closed-contact
test and actual CPU/profiled-P100 caller were green before this source review.

No unresolved runtime finding. Exact bit keys are monotone on finite doubles,
normalize both zeros and reject exponent-all-ones before admission. Word tasks
are disjoint, initialized, capped and use page-local offsets; CPU worker
boundaries cover all words, CUDA bounded grid-stride traversal does the same.
Empty results never divide by zero or launch. Counts include the2^32 maximum
total and fit checked arithmetic independently of output page allocation.

Same-context right lookup occurs while left Admission owns the context lock.
Close also obtains that lock before removing registry entries, so right lookup
cannot admit a stale same-context object during concurrent public close. Foreign
contexts fail without a second lock. Pages retain both indexes; destructors
consume device tokens through existing cleanup_child and diagnostic history.
Publication failures use the existing primary/fatal-secondary path. Alias/span
preflight precedes zeroing and diagnostics; numeric downloads require exact
extents and expose actual resident storage. No arbitrary native box is claimed
to be a certified carrier without the separate scene enclosure mechanism.

CUDA callbacks use guarded Operation, checked allocation/copy, stream finish,
DeviceScope and Pending. Output release occurs after successful device restore;
events are destroyed before successful publication. A failed download cannot
publish a successful immutable scene page. Event flags only apply to nonempty
measured CUDA work. The v4 factory returns a static exact-base table and does
not alter earlier tables or numerical kernels. Lazy refusal and malformed
tables are stable unsupported; transient/fatal diagnostics remain distinct.

Python creator allocation guards encompass native publication and wrapper
construction. Input iterables and rows are bounded before native allocation;
missing core declarations preserve old APIs. The body scene owns its separate
BaseException guard and immutable metadata, reviewed independently by the
oracle lane. Existing arithmetic source/flags are unchanged; actual old binary
compatibility and new-library regressions remain required execution checks.

The eighteen native scenarios attack token retention, race admission, both
unpublished token kinds, actual index/mask partial copy, poison propagation in
both directions, upload/restore refusal, lazy table/cache states, aliases and
unrepresentable spans. The environment case directly sets CPU DAZ/FTZ/rounding
modes, admits/rejects subnormal boxes and checks two-worker masks before restoring
the original mode. These are modeled failures at existing callback seams, not
physical GPU fault injection. Fleet ASan/UBSan and CUDA memory outcomes belong
in the qualification report; source review alone establishes none of those.
