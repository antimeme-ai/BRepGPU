# Model scratch resource review

Parent reported the phased bridge still reaches its 64 MiB child cap on actual
quad vector growth, with maximum observed integer 28 limbs. Requested a caller
resource-policy review rather than another numerical rewrite. Read actual run
reservation/Budget/scheduler backing and child context propagation. Sent scoped
ACCEPT of docs/adaptive-model-scratch-policy-plan.md; saved source facts in
papers/2026-10-17-adaptive-model-scratch-resource-review.md.

Two simultaneous twelve-worker prepaid pools at 256 MiB account for 6 GiB;
direct nested scratch and all retained data remain globally metered. Implicit
legacy behavior stays min(memory,64 MiB); explicit values are checked and never
silently reduced. No geometry/core/source/tolerance/work change, build or run
from this review lane. Actual body review follows the owner's handoff.

Actual caller and focused existing driver reread ACCEPT: validation occurs
before output creation, combined prepaid pool width uses safe division, and one
effective child cap propagates through existing owners. Metadata/byte-identity
and backing refusal checks remain direct. No additional findings, source/core/
tolerance/work change, build or run by this reviewer. Parent and owner received
the actual-source verdict for their finite closure and next operation.
