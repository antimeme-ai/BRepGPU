# Discovery scalar field: retained-artifact integration review

Accepted with no blocker. Reviewed only the newly retained scalar artifacts,
controlled off-grid integration, regenerated figures and their report claims.
No qualification, build, native test, target job or benchmark was rerun.
Analysis uses local CSV/SVG/source artifacts; earlier geometric reviews remain
separate.

## Field completeness and preservation

The new native-cpu1 field contains84 distinct full-condition cases,4,536 raw
rows and1,008 summary rows, with84 work files. It covers the four batch sizes,
seven corpora and three methods at depth8. Native-offgrid contains24 cases,
1,380 raw rows and300 summaries, with24 work files. These cover two corpora,
three methods and CPU1/CPU24/both P100s at N4096.

Every repeated phase has exactly seven uniquely indexed samples0..6; the
initial/first-use/final phases retain their single observations. Recomputed
medians/min/max and every reported quality/work extremum exactly match raw
rows. Each work histogram totals the request count, and its visit/refinement
weighted mean matches the corresponding raw phase/sample value. Full condition
keys include backend/device/requested workers/count/corpus/mode/depth/visits/
refinements/tolerance/phase. They are unique across all delivered native fields:
336 main cases,24 off-grid,36 diagonal-depth and48 axis-depth =444 cases.
Depth8 diagonal rows are not duplicated in the separate depth sweep.

The earlier18 off-grid CPU24/GPU cases remain unchanged: their sample and
summary rows match the preserved prior delivery, and their work files are
byte-identical. All six added CPU1 cases' sample/summary rows and work bytes
match the controlled retained originals. The earlier scalar off-grid timings
remain excluded rather than mixed into new medians.

The source and binary manifest bytes are unchanged from the prior delivery.
Every recorded source checksum also matches the retained implementation now
on disk. This verifies source/checkpoint continuity; no new target-binary
inspection or execution is claimed. The report names the same numerical/
benchmark checkpoint and separates later rendering/documentation changes.

## Figures and new report claims

All three SVGs, both figure comparison TSVs and FIGURES.md reproduce
byte-for-byte from the retained CPU1 + CPU24/GPU main summaries and controlled
off-grid summary. SVGs parse successfully. Every rendered median/spread comes
from the same full requested condition. The ratio TSVs remain explicitly
CPU24-versus-P100; adding the CPU1 timing series does not silently change their
comparison denominator or hierarchy-only reuse estimate.

The updated report's444-case accounting and added scalar large-cylinder
medians are correct: N65536/Bernstein/depth8 CPU1 resident17,398.099129ms and
cached-host17,706.571340ms. The retained GPU headline still compares CPU24,
with its existing native scopes and arithmetic/quality caveats.

## Small-batch crossover

These are observed median relationships for the retained implementation, not
a CPU ceiling or a hardware crossover law:

| Main batch | Resident observations | Cached-hierarchy host-to-host observations |
| --- | --- | --- |
| N1,21 corpus/method conditions | CPU1 and CPU24 each beat both P100s in21/21 | CPU1 and CPU24 each beat both P100s in21/21 |
| N256,21 conditions | CPU1 beats neither GPU in every condition; CPU24 beats both in13/21 | CPU1 beats neither GPU in every condition; CPU24 beats both in16/21 |

For coarse cylinder/Bernstein/depth8, resident medians are:

| Count | CPU1 ms | CPU24 ms | P1000 ms | P1001 ms |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 0.191499 | 0.222099 | 1.853051 | 1.810662 |
| 256 | 68.723506 | 4.543764 | 5.897483 | 5.929212 |
| 4096 | 1082.241272 | 77.210968 | 6.189751 | 6.188828 |

The report's cylinder example therefore correctly distinguishes GPU advantage
over CPU1 at N256 from CPU24's remaining advantage, then both GPUs' advantage
over both CPU configurations at N4096. At N1, CPU24 is a requested configuration,
not24 active query workers: runtime uses min(requested_workers,N), so both
CPU settings execute the single query on one worker. Their observed difference
must not be interpreted as multicore scaling.

Shared-host conditions, no worker affinity, independent calls, uncontrolled
clocks and repeated targets remain material limits. Resident includes result
allocation and completion; cached-host includes query ingestion, download and
native consumption. These medians are not additive and neither is complete
CAD modeling latency. No inference about a best possible SIMD/CPU algorithm,
cross-patch/trimming work or a specialized CUDA kernel follows from this field.

## Disposition

The new scalar artifacts and controlled replacements integrate cleanly, retain
the earlier18 valid cases, and support the updated report/figures. No duplicate
condition, sample-count error, summary mismatch, work inconsistency, stale
figure or unsupported new scalar number was found. CUDA specialization remains
a distinct experiment and is not part of these preserved measurements.
