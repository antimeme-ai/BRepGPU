# Runtime review of measured adjacency costs

Independently analyzed the retained 24-case campaign with standalone stdlib CSV,
hashing and arithmetic. Recomputed 720 summaries from 2,544 raw rows; checked all
fourteen local source hashes and measured/qualified binary identity consistency,
case sizes, transfer formulas, consumption extents and 120 repeated timing scopes.
Saved papers/2026-10-14-adjacency-performance-runtime-review.md.

Accepted honest native/caller costs: ordered native CPU1 wins; only one P100's
permuted native median beats CPU24 and their ranges overlap. Fast GPU construction
phases do not establish full native or typed-caller gain. Initial and repeated
independent qualification remain explicitly outside the measured product scope.
Recorded cache-default architecture 52 versus target 60-real ambiguity for root's
direct measured-binary inspection, and rejected causal NUMA/PCIe attribution from
upload differences alone. No production execution, code edits, new benchmarks or
optimization occurred; all tuning stays in the operator's deferred return pass.

Reviewed the completed REPORT and derived construction-reader table. Recomputed
all 24 stage rows from per-iteration raw values; exact agreement. Actual compiler
flags/cuobjdump architecture artifact ties sm_60 to the measured binary and
resolves cache-default 52 ambiguity. Accepted report scopes including explicitly
different CPU/GPU initialization placement. Sent two wording improvements to root:
uploads apply only to CUDA calls; prefer 21% lower median over "1.27 times lower."
No measured source or product file changed during this final review.
