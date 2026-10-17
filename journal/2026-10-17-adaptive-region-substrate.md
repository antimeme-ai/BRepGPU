# Shared exact region substrate

Extracted the existing domain segment index and exponent-safe metric arithmetic
for reuse by the cached-adaptive face constructor. The old sampled-domain path
uses the same extracted index and metric implementation. The direct CDT now
accepts typed directed cycles and strict interior anchors as described in
docs/adaptive-trimmed-construction-plan.md; no external dependency is added.

The selected mechanisms and primary-source grounding remain in that plan and
papers/native-adaptive-boundary-regions.md. Actual directed-region output passes
the existing independent exact positive-chain/incidence reader, including
disjoint components, nested rings, interior anchors and exact1/3 under shear.
The original domain and metric compatibility tests pass on the fleet (0.20s
combined). This checkpoint shares machinery; it does not declare the complete
trimmed-face or watch operation delivered. Whole-face curved failures remain
under investigation in the face journal.
