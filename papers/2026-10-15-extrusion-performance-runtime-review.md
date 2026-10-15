# Extrusion actual performance/runtime review

Read-only stdlib analysis of saved `results/extrusion-2026-10-15/costs`; no product
execution, geometry requalification, rerun or tuning. **Accepted records, no
blocker.** Final report prose review follows when written.

Confirmed48 fresh-process runs,12 excluded warmups,36 measured and12 cells.
Recomputed every summary median/minimum/maximum from raw, including complete
profile wall, points wall, preparation, construction, sample and adjacency medians;
all match exactly. Prepared-profile→consumed-output/cleanup medians, milliseconds:

| Profile | CPU1 | CPU24 | P1000 | P1001 |
| --- | ---: | ---: | ---: | ---: |
| Triangle |85.106354|191.416264|677.379207|1157.870750|
| Chamfer |297.610337|190.853037|822.236367|1396.118388|
| Many16 |660.569383|558.940131|1033.111320|1477.175932|

Unlike scaled copies of one model, these recipes have3/6/16 original vertices,
5/8/18 charts and1445/2312/5202 actual candidates,979/1930/4826 admitted rows.
Every run's recipe/quality/sample inventory agrees; all admitted native statuses
are successful and consumed fields finite. Actual source/output histories contain
12→48,21→90 and51→230 entities respectively, matching3N+3→14N+6, with6N+6 modified
cap images and8N generated side entities. Typed CSR/input buffer extents match
the actual result edge/use counts. These are inventory checks; existing complete
geometry/history qualifiers remain the direct truth mechanism.

All13 measured source hashes and controller pin match current sources and remain
single-valued across all48 runs. Actual used core/CUDA/topology-library hashes
match the known preserved binaries. Saved graphs/carriers, source points/maps,
histories, actual sample bits/rows, material/boundary reads and raw topology arrays
support inspection without recomputing modeled geometry.

Outer disjoint phases reproduce prepared-profile complete wall within a maximum
0.006081 ms timestamp gap. At every run, points wall equals that wall plus profile
preparation within floating output precision (maximum1.75e-13 ms discrepancy).
That per-run identity does **not** allow adding separately computed medians.
Points wall starts from already-owned exact point tuples; it is not file decoding
or process startup. Both useful walls include returned feature publication, real
material/history/geometry reads, request setup, native geometry/normal consumption,
Engine cleanup and separate topology setup/build/mate-neighbor reading.

Independent qualification (177.272–1417.254 ms over all runs), SVG/record output,
reference work and process/module launch are outside useful feature walls; parent
child-process wall includes all child work. Nested construction/sample/adjacency
rows overlap their enclosing stages. The unprofiled sampling evaluate_ms is a
wrapper wall, not isolated kernel time. Fresh geometry and stateless topology
device initialization are both real costs; no residency optimization is selected.

CPU1 has the lowest triangle median; CPU24 has the lowest chamfer/many16 medians.
No measured P100 complete range beats the corresponding best CPU range. Ordered
three-sample cells on shared unpinned hardware do not isolate CPU thread-count,
clock/cache/load causes, despite the observed median differences. There is no
GPU feature construction, complete GPU win or general CAD/scaling-law claim.

Saved qualification reports zero world-position component error for this corpus;
the largest conservative exact-interval unit-normal error bound is
9.898779707530338e-17, within declared2^-36/2^-30 grades. This is finite-corpus
evidence, not exactness for arbitrary native queries. Optional foreign records
contain85/154/384 edge/interior-face witnesses, exact binary64 endpoint/distance
flags and original-curve preservation. Recorded volume differences are
2.220446049250313e-16/0/0 and maximum boundary residuals
2.2887833992611187e-16/0/2.482534153247273e-16. They do not establish foreign
material/history or the required external alpha corpus.

## Final report disposition

Read final REPORT and relevant published scope. **Accepted; no performance/report
blocker.** All12 complete/range/points cells and the32 displayed many16 diagnostic
phase values agree with actual measured records. The report correctly calls the
normal quantity a conservative interval bound, separates full command overhead,
uses genuinely different profile sizes and leaves CPU timing variability causally
unattributed. Foreign85/154/384-witness tables and missing material/history/alpha
coverage are correct. Suggested only a wording clarification: history axial maps
return exact profile-local Z, so “exact axial source maps” is clearer than
“normalized axial coordinates.”

The independent actual-geometry/artifact review is
[the semantics artifact review](2026-10-15-extrusion-unit-artifact-semantics-review.md).
This lane matched the three actually used native hashes in all campaign records;
root owns the final all-five-library fleet hash recheck and records it in the
publication/handoff journal before merge. No extra execution or source mutation
is required for this accepted performance review.
