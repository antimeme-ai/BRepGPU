# 2026-10-13: projection design oracle review

Reviewed the iteration-two design as a first-order box-stationary candidate
operation. Saved `papers/2026-10-13-projection-design-oracle-review.md` with
exact fixture/status/flag expectations, conditioning counterexample, stable
distance witness, counter bounds, lifetime and task-value checks.

Sent concrete integration requirements: curvature threshold must be defined
through eigenvalue-equivalent shifted SPD rather than pivots; weak-bound
diagnostics use a tolerance-relaxed cone and cannot prove a nonminimum; scaled
gradient fallback and actual rounded chord descent need explicit definitions.
The affine conditioned-family witness prevents a universal UV accuracy
assertion despite a small residual and admitted rank. No product/test code or
remote execution was performed.

Re-read the integrated design. O1–O3 are resolved through strict reduced
eigenvalue thresholds, explicit tolerance-relaxed flag meaning, canonical
solve/fallback scaling and actual rounded trial descent. Counter stagnation
wording and fixed flag semantics are explicit. Recorded no remaining geometry/
oracle design blocker; the forthcoming plan must set finite-corpus reported-
field/error allowances and direct counter cases. No implementation was started.
