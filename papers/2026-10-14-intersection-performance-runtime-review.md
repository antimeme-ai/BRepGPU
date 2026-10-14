# Intersection performance: runtime artifact review

2026-10-14. Accepted, no artifact blocker. Standalone TSV/statistics/hash analysis
of [the actual matrix](../results/intersections-2026-10-14/matrix/summary.tsv).
No product imports, native execution, fleet work or new performance trials.

## Direct artifact checks

All 16 cases are present: circle/clipped × resolution 65/4097 × CPU1/CPU24/
P1000/P1001. Every case has the exact ten single phases at iteration -1 and ten
reused phases at each iteration 0..4, without duplicate/missing rows. The matrix
contains 960 raw rows, 320 summary rows and 80 reused calls. Durations are finite
and nonnegative. Recomputed median/min/max and sample counts match every summary
row exactly; per-case raw rows equal their aggregate rows. Identity, component/
piece counts, query counts, profile=0, repeat=5 and byte metadata match inputs.

All 11 source hashes match the actual `00455eb` snapshots. They also matched live
files at this review's read. This identifies the matrix's exact stage, not a
requirement that later standalone caller output-label corrections retain its
old hash. Do not replace historical manifests with final caller hashes. The
benchmark source remains unchanged; it does not execute pair discovery in timed
workers. Root reports serialized execution and successful source/binary drift
checks throughout the campaign.

The recorded core/plugin hashes match the preceding qualified pair matrix:
core `b63af7f6d64ab06f2dccfdd85b27931ec008a8586313463d2d4cbac4ce8dcfd6`,
plugin `4aeb0d7dd124fd00044f6be7dbe7a04b104748366fd01058e9815f838d148e61`.
This review compares recorded manifests; it does not claim another local ELF
execution/qualification. Native source and binaries are unchanged by the Python
intersection delivery.

Each initial case's independent quality table has the literal complete counts
and 2N native rows. Maximum reported initial position/pair errors are
1.889834066170702e-16 / 1.942890293094024e-16. These rounded descriptive maxima
are not the acceptance mechanism: the qualifier compares exact actual binary64
positions against its independent original-source algebraic model. Independent
qualification runs on the first batch outside reused time; all 80 reused calls
still execute the product's source checks and exact model grade. No CPU parity
or claim of independent requalification on every reused call is substituted.

## Actual returned-call wall

All values below are five-call medians in milliseconds. Count N means two-sided
candidate/chart occurrences; native rows are 2N.

| Recipe | N | CPU1 | CPU24 | P1000 | P1001 |
|---|---:|---:|---:|---:|---:|
| Circle | 260 | 33.176 | 36.230 | 33.884 | 34.344 |
| Clipped arcs | 260 | 35.193 | 38.618 | 34.413 | 35.629 |
| Circle | 16,388 | 2298.884 | 2325.786 | 2336.565 | 2320.830 |
| Clipped arcs | 16,388 | 2501.803 | 2489.515 | 2478.909 | 2505.555 |

No full-consumer GPU advantage is established. For large circle calls, actual
ranges are CPU1 2124.414–2649.177, CPU24 2087.282–2333.994,
P1000 2103.169–2363.508 and P1001 2149.634–2790.957 ms. Large clipped calls
range CPU1 2231.288–2549.665, CPU24 2217.839–2520.689,
P1000 2306.778–2554.281 and P1001 2325.573–2552.795 ms. They overlap widely.
Small apparent differences and CPU24's small-batch startup cost do not establish
a general benefit. Shared-host clocks/load and five repetitions limit inference;
min/max are descriptive, not confidence intervals.

## Native evaluation versus host consumer work

Large-case evaluation host API spans are:

| Recipe | CPU1 | CPU24 | P1000 | P1001 |
|---|---:|---:|---:|---:|
| Circle | 2.181 | 1.983 | .128 | .113 |
| Clipped arcs | 2.197 | 1.912 | .125 | .114 |

The descriptive CPU24/GPU ratios are 15.3–17.5×; CPU1/GPU ratios are 17.1–19.2×.
This is native evaluation with already-created query inputs and fresh result
allocation/synchronization/publication, not a kernel-only timing or GPU
intersection construction. Upload/download/host model work remain outside that
phase and inside actual returned-call wall. Small-case GPU evaluation is slower
than CPU1 (.064–.073 versus .049 ms), despite beating CPU24's startup span.

Large world/scatter medians are roughly 1501–1632 ms and exact product grades
742–820 ms. Those host phases dominate reused wall; full-source download and
conversion are about 2.3–3.2 ms each and gather about 18–20 ms. Do not add unrelated
phase medians to derive wall. The actual external timer is the measured outcome,
including immutable publication and ordinary caller assignment.

Cold CPU work is also material. Exact construction is only 12.9–24.4 ms across
the full matrix. Request preparation is 0.438–0.846 s at N260 and 26.757–31.964 s
at N16,388; it includes exact maps and prepared nearest UVs. Initial independent
qualification is separately 0.806–1.071 s / 40.689–45.611 s. These one-off spans
are neither five-call estimates nor part of reused wall. Large cold-to-first
batch spans are 29.136–34.909 s, ending before independent qualification and
final cleanup. Thus “construction dominates” alone would misdescribe the costs:
request preparation dominates cold CPU setup, row/scatter and grading dominate
cached sampling. CUDA cold Engine spans and final cleanup remain measured costs,
without attributing them to an unmeasured driver/module cause.

## Transfer and contract scope

Device geometry setup is 976 bytes once per cached sampler: 896 control bytes
plus 80 compact descriptor bytes. CUDA input/output payload per candidate is
40/56 bytes. N260 sends 10,400 bytes and downloads 14,560 bytes; N16,388 sends
655,520 and downloads 917,728 bytes per call. Actual host source provenance is
40N+680 bytes: 11,080 / 656,200 bytes. It is not extra device jet traffic.
CPU cases record zero CUDA payload. No GPU topology upload is claimed.

The experiment measures two-sided source-surface consumption of an exact CPU
intersection result. Prepared patches are cached, while each sample call creates
fresh native queries/results. Exact construction, trims, graph, world scatter,
model grades and exports are CPU work. General SSI/Boolean/modeling acceleration,
GPU construction and a whole-process gain are not established. Integration,
sessions/residency and pooling remain deferred until the broad-pass big rocks
are complete, as directed.

## Final report disposition

The delivered [report](../results/intersections-2026-10-14/REPORT.md) and
[guide](../docs/INTERSECTIONS.md) match these numbers/scopes. Qualification is
correctly split into 16 initial tests plus one additional consumer-class method
per backend; initial/additional GPU tables both record zero errors/leaked bytes.
Caller totals independently read as 1,040 candidates, 2,080 native rows,
400 coefficients, 24 events and 48 polylines, across two circle/two clipped
executions. Root's detailed exported-geometry analysis is a separate mechanism;
this note checks the actual artifact counts and the report's claims.

The later caller label/footer changes leave imported make_inputs identical in
AST, and shared helper/benchmark bytes identical to checkpoint00455eb. The old
matrix is therefore kept under its measured stage, not rewritten to current
caller versions. No final scope/number blocker; no new performance execution.
