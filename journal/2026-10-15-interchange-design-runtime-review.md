# Interchange design runtime review

Accepted the complete design against actual wheel/source/export syntax and owned
Patch/Engine contracts. Sent root two bounded clarifications: cap input reads at
16 MiB+1 before allocation grows, and define mutation detection during acquisition
without contradicting immutable bytes-owned snapshots. Concrete parse budgets and
named public errors belong in the imminent API/plan. No runtime/design blocker,
implementation, install, build or product execution in this review. Saved full
verdict in `papers/2026-10-15-interchange-design-runtime-review.md`.
