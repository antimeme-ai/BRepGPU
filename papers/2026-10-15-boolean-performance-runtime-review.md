# Boolean actual performance/runtime review

Read-only review of saved `results/boolean-2026-10-15/costs` records using stdlib
CSV/hash/statistics analysis, not product execution or repeated geometry checks.
**Accepted records; no blocker.** Final report prose will be checked when ready.

Confirmed 48 fresh-process runs, 12 excluded warmups, 36 measured repetitions,
12 summary cells (three recipes × four placements). Recomputed every median,
minimum and maximum complete wall, and every construction/sampling/adjacency
median directly from raw rows; all match saved summaries exactly.

Complete cut-to-consumed-output/cleanup medians in milliseconds:

| Recipe | CPU1 | CPU24 | P1000 | P1001 |
| --- | ---: | ---: | ---: | ---: |
| Unit | 180.907527 | 270.094253 | 790.900728 | 1278.801108 |
| Small translated | 179.104323 | 408.834753 | 838.042736 | 1257.718597 |
| Large translated | 174.576078 | 182.771734 | 710.412084 | 1256.375902 |

These three recipes have identical topology/request size: every actual run has
2,890 candidates, 2,712 admitted successful rows and178 rejected rows. Scaling
does not create a larger modeling workload. Actual placement metadata agrees;
all admitted saved fields are finite and native statuses successful. Recorded
qualification counts consistently include10 vertices/15 edges/7 faces/30 uses/
10 charts, genus1 and complete58 retained/13 modified/14 deleted/10 generated
history roles. Native source/CSR artifact byte extents match those actual counts.
The independent geometry/history qualifier remains the truth mechanism; these
inventory reads do not claim a second qualification of the same geometry.

All11 measured child source hashes are single-valued across the campaign and
match current files. Controller hash matches current source. Actual used core,
CUDA plugin and topology-library hashes match the known preserved binaries.
Source/output records and operand table hashes remain available alongside exact
graphs/history/maps and native sample/reader arrays.

Disjoint outer stages reproduce each complete wall within the final timestamp
gap (maximum0.015771 ms). Construction, material/boundary/history reads, actual
request preparation, Engine setup, sampling/checksum, Engine cleanup, topology
packing/library load, adjacency build and actual mate/neighbor reads are inside.
Operand preparation, independent truth checking, output/visualization and process/
module overhead are outside; parent child-process time includes the child's full
work. Independent qualification itself ranges661.377–1058.439 ms over all runs.
The useful-operation number is not full command runtime including qualification.

Nested construction/sampling/adjacency phases overlap their enclosing stages;
neither those rows nor per-phase medians may be added to form a new complete
median. For unit P1000/P1001 the actual median Engine setup is272.908621/
561.779823 ms, adjacency build246.308266/460.277673 ms, Engine cleanup41.840079/
84.197069 ms. Fresh geometry Engine and stateless topology setup are distinct
real calls. No residency reuse or setup optimization is being hidden in these
fresh-process costs. sampling/evaluate_ms measures the synchronous wrapper stage
with optional native kernel profiling disabled, not isolated CUDA kernel time.

CPU1 has the lowest median on all three recipes. CPU1/CPU24 ranges overlap;
these exploratory unpinned three-sample cells do not isolate thread/clock/load
causes. No GPU win, GPU Boolean construction, larger-model scaling, resident
throughput or full-CAD speedup follows. The separately executed optional foreign
reference's146 boundary witnesses per recipe and its volume/bounds checks remain
geometry comparisons, not foreign material/history or external alpha coverage.

## Final report disposition

Read the complete final REPORT and its relevant README/return-pass claims.
**Accepted, no performance/report blocker.** All12 cells/ranges, displayed unit
phase medians and cold/fixed-workload distinctions agree with actual records.
The largest saved position error is4.426331796538329e-16 and normal error
1.322255008107945e-16, matching reported rounding. All three actual foreign
volume differences/residuals and146-witness counts match their comparison tables.
The report correctly uses the outer returned CPU difference wall, declares
overlapping nested phases and unprofiled wrapper evaluation, excludes independent
qualification from useful time while retaining child-process time, and preserves
the foreign material/history and alpha-corpus gaps. A small stale README SSI
sentence should say “broader Boolean assembly” rather than implying all Boolean
assembly remains future; sent root that wording clarification. No measured source
change, rerun or tuning is required.
