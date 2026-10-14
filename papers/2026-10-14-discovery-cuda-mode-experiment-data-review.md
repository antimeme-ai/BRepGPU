# CUDA mode experiment: retained-data and decision review

Accepted with no blocker. Reviewed the completed experiment report, full and
reverse-order raw/summary/work artifacts, both figures, four published binary
identity captures and qualification metadata. Only local artifact analysis was
performed: no native call, public test, benchmark, build, fleet job or new audit
framework. Earlier client/numerical/source reviews remain separate.

## Measurements and original artifacts

The initial field contains112 successful native invocations:28 reviewed cases,
two variants, two sequential cards. It retains6,832 raw rows and728 matched
condition/phase comparison rows. Reverse confirmation contains24 invocations
for case IDs3/9/23/25/26/27 on both cards,1,464 raw rows and156 matched phases.
Stable case-parity variant order and its reversal agree exactly with order.tsv;
all recorded invocations returned0.

Every repeated phase has seven unique sample indices0..6. All initial/final
single observations remain separate. Recomputed medians/min/max and every
quality/work extremum match raw samples. Histogram bin totals equal request
counts and their weighted visit/refinement means match the corresponding raw
phase/sample values. Full requested keys, case recipe, shapes, tolerance,
budgets and both-card phase pairing are coherent. The entire comparison TSVs
reproduce from their retained per-invocation artifacts. Raw quality values and
histogram bins match between variants for every invocation.

The shared core, benchmark and runner provenance is identical in comparison
and confirmation. Recorded plugin identities differ deliberately:

| Artifact | SHA256 |
| --- | --- |
| Preserved core | 3515b86d0aad1f4bac816b8cc720e2f9452759f2806b6e1938854eaf4fb355de |
| Preserved native benchmark | 39ded902fc6b005204357524a29449b28e07b3766b10b3fb7e05d7a36be1d8c1 |
| Dynamic plugin | 8a6a861c17c7c2c22a0fafe40ce8cc2b46b33272322c8b48af9ce5af24d27efe |
| Specialized plugin | 80d4e829091f5b332b806b2200d9ef09f874c9775cd9adf83fb924567b9ad8da |

The current product CUDA source hashes to
3da09294aa1d291218344fa004ea0db638f331681ceb0fe633811e2430cda4ca,
matching the restored dynamic source. Candidate source at ba44794 remains
0823748fbec55d21f8b3c76be06d101edaa0b3a8082b08d59a5af2a2488447bd.
This review reads source/history and retained provenance; target-default-plugin
restoration is coordinator-reported execution evidence, not a new host check.

## Qualification artifact consistency

Each card's published old/new captures have309 corpus rows and132 total
batches. Indexed raw slab, source/options/input and deterministic metadata
bytes match exactly, with deliberate plugin/label provenance differences.
Duration/profile evidence satisfies empty/unprofiled NaN and profiled finite
nonnegative rules. Current client-source checksums match captured metadata;
core/benchmark/plugin identities agree with staged performance provenance.

Both exact qualification directories contain309 request rows,11,415 downloaded
hierarchy nodes,0 recorded violations and no failure rows, with the correct
explicit card/backend labels. This validates the report's retained counts and
metadata. It does not rerun exact geometric qualification or hardware sanitizer
checks. The report identifies coordinator-run focused/sanitizer outcomes and
correctly limits finite checks to their targeted fault classes.

## Tables, figures and decision

Both SVGs reproduce byte-for-byte from their comparison TSVs and parse as SVG.
They show all28 original or six selected confirmation conditions, both cards
and both native operation phases. Case labels retain count/method/depth/budgets;
titles retain tolerance, spreads and attained gap population/width. Percentage
sign means lower specialized median is positive. No condition, regression or
unresolved result is removed to improve the picture. Spreads are explicitly
not confidence intervals.

All five report example rows reproduce at their displayed three-decimal
precision. No original case with N>=4096 has a resident or cached-host median
improvement>=5% on both cards. The capped DOP/refinements8 card1 resident case
is0.374616→0.420378ms initially (-12.2157089%) and0.368167→0.407646ms
in reverse order (-10.7231229%). The N256 AABB sign reverses with order;
the cap-only Bernstein card0 gain is accompanied by a card1 regression.

The plan required a matched large-case native-phase gain>=5%, confirmed in
reverse order on both cards, without material confirmed native regressions.
It is therefore not met. Retaining the dynamic kernel follows that predeclared
scope; static register/instruction reductions or event-only gains cannot
override it. No mixed dispatch or scheduling change was added after seeing
the data. Neither rejection nor these repeated-input results imply a general
verdict about specialization, arbitrary geometry or a CPU performance ceiling.

Resident, event-bracket and cached-host scopes remain distinct. The event
bracket may include submission delay; cached-host includes native analytic
consumption. No independent medians become a synthetic complete caller time.
The report preserves shared-host/background/clock and repeated-target limits.

## Publication checks and disposition

Local report links resolve, including the reviewed plan, captures, exact
qualification, condition/provenance files and subsequent numerical study.
Published metadata and text artifacts contain no workstation/home/root paths
or SSH/operator identifiers in the inspected patterns. Binary/source identities
use hashes/basenames rather than private staging locations.

No arithmetic/table/sample-count/key/order/work/identity/figure or decision
error was found. Publish the measurements and retained candidate history with
the original qualified dynamic product kernel, as proposed. Static compilation
observations remain observations rather than claims of dynamic occupancy or
hardware speedup.
