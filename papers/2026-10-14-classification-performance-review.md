# Exact classification: saved performance review

Independently read the saved caller raw/summary/preparation/environment/source
files with standalone stdlib CSV, hashing and arithmetic. No product imports,
execution or new measured trials. Accept the reported scopes and statistics.

Twelve cases contain 84 operation runs: one first call, one excluded warmup and
five repeats each. All 48 summary rows exactly match independently recomputed
repeat medians/minima/maxima. All times are finite/nonnegative; complete wall
equals operation plus consumption within floating representation error. Every
run reports PASS and the same consumer read marker on retained input. The eight
source pins match local recorded product/oracle/test/caller sources.

There are 295,092 qualified query results, including 210,780 queries in the sixty
timed repeats. Saved small callers contain 36 exact point records, 17 contact TSV
rows (15 actual vertices and two explicit empty records), and nine source faces.
The independent saved-geometry review remains separate from these cost checks.

| Largest operation | Complete median [min,max], ms | Outside qualification median, ms |
| --- | ---: | ---: |
| 12,288 box points | 402.611 [378.285,497.854] | 1365.806 |
| 12,288 cylinder points | 364.951 [352.924,377.008] | 900.528 |
| 12,288 plate points | 489.557 [468.598,516.221] | 1722.956 |
| 512 rectangular contacts | 147.465 [129.454,299.998] | 1527.958 |

Large point preparation is 33.129–40.643 ms; 512-contact preparation is 6.573 ms.
The first operation-plus-consumer calls are 394.206/360.440/501.093 ms for large
box/cylinder/plate and 140.716 ms for 512 contacts. They occur after process
imports and input preparation, not in fresh processes. Do not relabel those
observations as runtime/library startup or complete cold-to-first task cost.
Preparation and initial/repeated independent qualification have separate rows.

Operation timing includes exact submitted-coordinate conversion, rational
geometry construction and immutable result publication. Consumer timing reads
every label/supporting face and exact point, or every contact metadata/world/
bothUV vertex value. Qualification afterward additionally binds outputs to actual
submitted query count, world input/source identity and FaceRefs, then certifies
geometry independently. Its cost is explicit and cannot be silently included in
or omitted from a differently named complete task claim.

This is one pure exact CPU process. No native/GPU path, CPU24 comparison,
reference-kernel comparison, allocation residency or modeling speedup was
measured. Point clouds query each declared primitive; contact batches repeat
the same eight prepared source pairs, not 512 distinct complex geometries or a
solid Boolean. The wide largest-contact range is retained; no scheduling,
thermal or other causal diagnosis follows from this single serial campaign.

The source/caller and complete-scope semantics agree with the reviewed design.
Final report should retain those boundaries, source hashes and independent
qualification costs. Native acceleration, caching/source ingestion, integration
and Python's eventual shipping role remain the operator's deferred return pass.

## Final report disposition

Read the completed results/classification-2026-10-14/REPORT.md and actual final
qualification/mutation/protected-binary TSVs. All reported table values, first-
call spans, preparation and outside-qualification medians match saved data.
The report correctly retains the one imported process, eight repeated contact
pairs, fresh owned outputs, wide ranges, rational bit-size costs and no native/
GPU/comparative speedup claim. The 61-test compatibility scope with one skip and
final twelve-test scope agree with qualification.tsv.

Corrected mutations-final.tsv contains seven healthy baseline/mutant/retry groups:
baseline/retry exit zero, mutants exit exactly one and report assertion kills.
Five protected libraries have identical before/after hashes. The corrected
controller requires completed assertion-only unittest failures, resolving CM1.
Accept the final report without an additional finding. No measured source edit,
new trial, product execution or further implementation is needed from this review.
