# CUDA mode qualification/compiled-observation review

Read retained raw identity payloads/indices/source hashes, candidate exact
qualification artifacts, original focused baselines and candidate sanitizer logs,
ptxas/ELF/SASS text and current/archived CUDA source. Report:
[focused review](../papers/2026-10-14-discovery-cuda-mode-qualification-review.md).

No reporting blocker. Each card's 132 actual payloads and deterministic/input
metadata match old/new; retained exact qualification has 309 rows and 11,415 nodes
with zero violations; three candidate focused tests/card report zero sanitizer
errors/leaks. Independently parsed static instruction/resource observations match
the report and do not imply occupancy, dynamic work or speedup. Archived candidate
source matches the accepted hash; current source is restored dynamic. Live default
plugin restoration is integration-attributed. No native execution, fleet access,
test/build or product edit. Separate reviewer owns timing/figures/decision data.
