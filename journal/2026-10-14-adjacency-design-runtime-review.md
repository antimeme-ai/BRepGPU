# Runtime review of the adjacency design

Pressure-tested docs/ADJACENCY_DESIGN.md after completing primary-literature and
current-runtime research. Saved papers/2026-10-14-adjacency-design-runtime-review.md.
Sent three findings to the owning root lane: distinguish C=0/E>0 output from the
E=0 singleton sentinel; defer diagnostic writes until pointer/alias admission;
describe attempted cleanup and reported cleanup errors instead of unconditional
reclamation when a CUDA cleanup itself fails.

Accepted dedicated synchronous ABI5 scope, owned hierarchical GPU scan,
unspecified physical row order with exact typed membership, actual post-fill
reader, serial CPU prefix disclosure, separate build tree and warm-runtime
cost accounting. Recorded partial worker launch joins, padded-lane barriers,
sentinel initialization and stream/device restoration as plan-review targets.
No implementation, fleet execution or changes to protected binaries occurred.
