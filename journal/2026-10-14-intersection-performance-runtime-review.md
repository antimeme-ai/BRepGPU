# Actual intersection matrix artifact review

Used only standalone CSV/statistics/hash analysis and read-only Git snapshot
reads, with no product imports/native/fleet/performance execution. Verified all
16 cases, 960 raw rows, 320 summary rows, exact phase/iteration sets, case/aggregate
row equality, exact min/median/max/sample counts and transfer/identity metadata.
All 11 source hashes match checkpoint 00455eb; current files also matched at read.
Core/plugin manifests match the preceding qualified pairs matrix. Later caller
qualification-label edits are a separate stage, not a reason to relabel old data.

Initial independent quality counts and rounded maxima match expected scope.
Native GPU spans improve substantially at N=16,388, but full consumer wall remains
about 2.3–2.5 seconds across backends with broad range overlap; no established total win.
Host scatter/grade dominate reused calls, CPU request preparation dominates cold
setup. Independent first-batch qualification is outside reused timing and remains
visible. No sum-of-medians total, clock/occupancy inference or CPU parity oracle.

Saved papers/2026-10-14-intersection-performance-runtime-review.md with calibrated
headline and actual medians/ranges/byte counts. No artifact blocker. Root owns
serial-run/clock/load attribution and publication; no Git mutations by this lane.

## Final delivery report/guide check

Read results/intersections-2026-10-14/REPORT.md and docs/INTERSECTIONS.md against
the accepted artifact review. Actual wall tables/ranges, native evaluation spans,
15.3–17.5x CPU24 ratios, cold construction/request/first-batch ranges and byte
counts match the raw summary scope. No full-consumer or GPU construction gain is
claimed. Separate initial qualification and cleanup costs remain visible.

Qualification tables correctly distinguish 16 initial methods from one added
consumer-class method on each backend, with GPU device errors/leaked bytes zero.
No rerun of prior methods or unchanged native campaign is inferred. Caller
exports total 1040 candidates/2080 native rows, 400 coefficients, 24 events and
48 polylines across two circle/two clipped executions, matching published counts.

Read-only AST/snapshot comparison confirms the post-matrix caller change leaves
imported make_inputs unchanged; shared helper and benchmark source bytes remain
identical to checkpoint00455eb. Historical matrix manifests keep that exact stage;
new caller manifests name their regenerated exports. No product imports/native,
fleet, test or benchmark execution. Final delivery report/guide accepted, no
remaining number/scope blocker.
