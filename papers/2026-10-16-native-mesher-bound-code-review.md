# Actual-root CPU/P100 bounds: scoped code review

The parent reviewed the actual exact predicate, CPU interval filter, CUDA
restriction kernels and owned session lifecycle. Geometry-owner peer review
separately checked the numerical coefficient and anchor identities.

The implementation restricts original homogeneous roots, forms the residual
against the actual emitted affine chord/facet, proves positive minimum weight,
and compares squared coefficient norms to squared allowance times minimum
weight. Tensor blossom averaging and degree-32 curve endpoint handling preserve
the original root. A common exact anchor avoids translation-dependent interval
width without changing the residual. Unresolved filters reach the same owned
exact comparison; CPU/GPU agreement alone is not the oracle.

Independent power-basis substitution and power-to-Bernstein conversion supplies
88 original-function cases, including near-threshold degree combinations and a
large translation. All proved CPU and both-P100 decisions match those values.
Unresolved outcomes are reported rather than silently accepted. Actual device
memory, race and synchronization checks pass on both cards.

Review required failed-create ownership to be explicit: a failed drain can leave
a nonnull poisoned session that must remain owned. It also found a failed device
guard constructor must restore the previous device itself, and required all
creation failures to share rollback/drain logic. Those findings are resolved.
Work reservations precede host filtering and device submission; pinned and
device storage is bounded. Inputs are copied before submission returns and
stale/concurrent tickets cannot overwrite a live batch. Resident immutable task
references replace per-thread task copies; compiled stack usage fell from 400
to 96 bytes per thread without changing the numerical policy.

Decision: ACCEPT this original-root bound and session unit. This is not a
complete native mesher, an analytic whole-trace bound, or a whole-operation
CPU/GPU speedup claim. Runtime scheduling and geometry-aware refinement remain
the consumers specified by the accepted implementation plan.
