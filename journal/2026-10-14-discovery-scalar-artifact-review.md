# Retained scalar artifact review

Coordinator completed84 main CPU1 cases and six controlled off-grid replacement
cases. Reviewed only their retained raw/summary/work integration and updated
figures/report locally. Exact recomputation confirms seven indexed repetitions,
all median/min/max/quality extrema and histogram totals/means. Full native
condition/phase keys are unique across444 cases. New scalar field:84 cases,
4,536 raw rows,1,008 summaries. Integrated off-grid:24 cases,1,380 raw rows,
300 summaries. Prior18 valid CPU24/GPU off-grid rows and work bytes unchanged;
new six CPU1 cases exactly match controlled retained originals.

All three SVGs and comparison tables/figure guide reproduce byte-for-byte from
the summaries. Published source/binary manifests are unchanged, and recorded
source hashes match retained implementation files. No new binary execution or
target check was performed. Updated report scalar values/accounting agree.

Small-count analysis distinguishes scalar and requested24-worker configurations:
N1 both beat both GPUs in all21 conditions; N256 CPU1 loses to both in all21,
while CPU24 beats both in13 resident and16 cached-host conditions. Cylinder
Bernstein's observed crossover is correctly scoped. Noted runtime min(workers,N)
means both CPU settings use one actual worker at N1. No CPU ceiling inference.

Accepted focused review saved at
papers/2026-10-14-discovery-scalar-artifact-review.md. No prior geometric review
was duplicated; no fleet/native tests/builds/copies/benchmark or Git mutation.
