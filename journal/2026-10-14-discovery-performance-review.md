# Independent final discovery timing/quality review

- Read the four retained native data directories, fixture/consumer/runner and
  generated figure scopes locally. No native or remote execution, deployment,
  runner/product/report/README edits, or Git mutation.
- Inspected full requested-condition pairing, raw sample labels 0–6, medians,
  extrema and all quality/statistic ranges. Main: 14,784 raw rows / 3,192 summary
  groups; off-grid: 1,056 / 228; diagonal depth: 2,070 / 450; axis depth:
  2,760 / 600. All matched; repeated quality fields were unchanged.
- Excluded all six initial CPU1 off-grid cases from analysis. Public main and
  off-grid data currently use CPU24 plus each P100; controlled CPU1 repeats are
  pending. Depth sweeps contain CPU1 as well; this review uses CPU24 comparisons.
- Saved papers/2026-10-14-discovery-performance-review.md. Strongest matched
  headline: cylinder 65,536 / Bernstein / depth 8 / tolerance 2^-24, 36.14× and
  36.20× resident, 30.09× and 29.30× cached-host versus owned CPU24, with all
  65,536 gap-qualified rows and actual width differences stated.
- Recorded fixture alignment/repetition (60 distinct regular-cylinder targets),
  small-batch CPU wins, unresolved off-grid outcomes, precise phase boundaries,
  total packed hierarchy bytes and conditional hierarchy-only amortization.
- Identified shallow sufficient trees as the next measured optimization target:
  diagonal root-only DOP and axis-midpoint Bernstein depth 2 already satisfy the
  requested gaps, while deeper trees increase time/storage. Off-grid curved
  bound resolution remains a separate quality target. No extrapolated CAD claim
  or retained-data/figure-scope blocker.
