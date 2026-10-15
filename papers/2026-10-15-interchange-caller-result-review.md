# Interchange actual caller and cost review

Reviewed saved `results/interchange-2026-10-15/` records using standard-library
artifact analysis only. No product execution, fleet work, source changes or
repeated geometry qualification. **Actual records accepted; no blocker.** Final
report prose is reviewed separately when available.

The three campaigns each contain 16 fresh-process runs: four placements with one
excluded warmup and three measured repetitions. Total **48 qualified runs,
36 measured**. Mouse retains four original faces/two converted carriers and 578
queries; rational has two distinct faces/two carriers and 578 queries; sheet has
one face/carrier and 289 queries. Every case's metrics records all submitted rows
successful, without invalid rows; source digests match retained original bytes.

Recomputed every summary median/min/max directly from raw measured rows; all
match. Complete file-to-consumed-output-and-cleanup medians in milliseconds:

| Fixture | CPU1 | CPU24 | P100 0 | P100 1 |
| --- | ---: | ---: | ---: | ---: |
| Mouse | 34.258673 | 34.835116 | 319.585585 | 710.947107 |
| Rational | 7.579430 | 10.193890 | 290.973302 | 638.473402 |
| Exported sheet | 8.190083 | 11.099403 | 289.156894 | 548.396584 |

Each case's disjoint named phase sum differs from the complete wall only by the
small final timestamp gap (under 0.1 ms). Native result/query/patch cleanup and
context cleanup are included. Exact independent qualification, artifact writes,
process/module overhead and optional foreign acquisition are outside the useful
wall; full child-process time is separately recorded. Every process creates its
own Engine, so these results do not establish retained-context reuse. Recorded
P100 context setup is a large cost; no NUMA, hardware-health or device-throughput
cause is inferred from differences between cards. Neither these small batches nor
native evaluation phases establish a GPU modeling gain.

All six child source hashes are single-valued over all 48 runs and match current
measured source. All campaigns record the same controller source hash. Actual
core/plugin hashes match the preserved pair-release binaries; the five-library
preservation table agrees with the known unchanged binaries. Output tables retain
original/source controls and knots, typed face/use outcomes, patch lineage/domains,
query bits, raw local/source jets and consumed rows. Source keys remain revision-
scoped; source metadata and trims are not promoted into imported-solid claims.

The six separate foreign comparisons cover CPU1 and both P100s for mouse/sheet:
**2,601 query correspondences / 23,409 scalar components**. Query table counts match
checks; comparator source pins match current files. Mouse foreign-source maximum
position error is 2.220446049250313e-15 and first-partial errors at most
3.573327804945612e-16. Sheet position error is 2.134523027080837e-17 and partial
errors at most 1.1102230246251565e-16; all three sheet comparisons record intended
analytic checks. These are actual errors under the separately declared foreign
limit, not a claim that foreign evaluation inherits the native local grades.
Reference acquisition warnings about absent optional pxr remain explicit.

Read docs/INTERCHANGE.md and README's new scope/run text: they correctly describe
a static bounded subset, two of four mouse carriers, explicit multispan rejection,
uninterpreted trims, fresh-context costs and outstanding external alpha exchange
coverage. No general USD/STEP/IGES, solid import, or full-CAD speedup is claimed.

## Final report disposition

Read the complete final REPORT draft. **Accepted**, with root's already planned
artifact-review link correction and table formatting generated from raw summaries.
The report's 23,120 first jets/208,080 scalar components across 48 qualified calls
are consistent with the three actual query counts. It explicitly distinguishes
867 actual foreign queries (578 mouse +289 sheet) reused for CPU1 and both P100
comparisons from the 2,601 total correspondence rows; those are not three new
foreign executions. Direct/compatibility test claims remain their own recorded
execution scope. Fresh contexts, dominant recorded setup/cleanup, separate exact
qualification, unpinned three-sample ranges, uninterpreted trims and missing full
external alpha exchange coverage are stated correctly. No publication blocker.
