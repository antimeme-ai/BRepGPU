# Adjacency cost artifacts: topology and scope review

2026-10-14. Independently reviewed saved `results/adjacency-2026-10-14/benchmark`
TSV/SVG artifacts with standalone stdlib CSV/statistics/XML/hash analysis. No
product imports, code changes, native execution or new benchmark in this lane.
Cost graphics and bounded topology interpretation accepted. The report must
carry the coincident-instance and qualification-scope wording below.

## Input family and identity

The 24 cases are three sizes × two storage orders × CPU1/CPU24/P1000/P1001.
Small input is a box+cylinder+plate batch: 30 edges, 16 faces, 60 uses, 3 instances.
Repeated plates have 512 or 32,768 topology instances. Largest E=491,520,
F=229,376,C=983,040. Each plate has 14 ordinary opposed-face pairs and one
same-face pair, yielding 458,752 ordinary pairs and 32,768 seam-role pairs.
Every saved case reports exact complete member/mate/neighbor traversal and
all-seven-result qualification, with no omitted or unmatched primitive use.

Important: `prepare_case` repeats the SAME immutable snapshot at the SAME frame
coordinates. The instance ordinal separates its graph occurrences; these are
topologically separate records with repeated geometry, not a spatially separated
assembly and not a single complex solid. The permissive short graphic phrase
"disjoint primitive instances" must be read as graph occurrence separation;
the report should state the repeated same-coordinate geometry explicitly.

Globally permuted cases shuffle all edge, face and coedge arrays using the
specified deterministic RNG, then the reviewed wrapper rewrites both coedge
reference fields. Semantic source keys remain unchanged and original loop/UV
records remain on retained snapshots. The native CSR row order is unspecified;
the consumption statistic is commutative and the independent oracle checks
membership multiplicity and exact typed relationships without sorting authority.
The large matrix retains aggregate quality rather than serializing every large
result array; this review confirms recorded scopes and statistics, not a second
offline reconstruction of unsaved large native arrays.

All 24 case metadata, qualification totals, typed-reader traversal totals and
byte formulas agree. All 14 saved source pins match the current measured code.
Each CUDA call uploads 12*C=11,796,480 bytes and downloads 8*E+12*C+4=15,728,644
bytes at the largest size. Stable source tables stay on CPU; geometry controls
are not uploaded. Repeated calls retain host input/library/runtime state and
allocate/fill/free GPU topology on each call.

## Native and full-consumer results

Largest input, native total median[min,max], milliseconds:

| Storage | CPU1 | CPU24 | P1000 | P1001 |
| --- | ---: | ---: | ---: | ---: |
| Ordered | 8.193[7.638,9.409] | 9.063[7.995,21.265] | 17.325[15.959,18.780] | 17.008[13.873,18.841] |
| Permuted | 100.415[83.586,166.782] | 19.427[18.142,22.115] | 21.789[19.612,32.357] | 15.306[14.981,19.341] |

CPU1 is the fastest ordered native placement. In shuffled storage, CPU24 is
close to GPU total: P1000 has a slower median, while P1001's lower median still
has a range overlapping CPU24. Large gains against shuffled CPU1 do not
establish a universal gain against the best CPU placement. Stage host spans
may show faster device count/scan/fill/reader execution while native total
includes admission, context, allocation, uploads, downloads and cleanup.

Complete build+publication+typed-reader median is 4.000–4.337 s ordered and
4.824–5.091 s permuted across placements, with substantial overlapping ranges.
Python reader alone has median 3.977–4.322 s ordered and 4.791–5.065 s permuted.
No full-consumer GPU speedup is established by these data. Publication itself
has median 4.5–5.5 ms; the seconds of source-aware Python traversal dominate.
This is a return-pass opportunity, not justification to tune before finishing
the broad themes.

Initial input packing takes 8.08–9.10 s in the largest cases. Global permutation
adds 7.77–8.50 s. Cold-to-first-consumer wall takes 12.31–13.40 s ordered and
21.25–22.43 s permuted, and excludes subsequent independent qualification.
Initial literal+incidence qualification takes 15.86–23.54 s; each repeated
result's incidence qualification median takes 10.48–16.10 s OUTSIDE the plotted
build/reader wall. These explicit separate costs must remain visible to callers
and cannot be silently excluded from a claim about a completely qualified job.

## Saved chart checks

Recomputed all 720 phase summary rows from 2,544 raw measurements, including
sample counts, medians and min/max. Both 24-row SVGs' numeric labels, bar widths
and range-mark endpoints exactly agree with their respective saved raw-derived
scope: native total versus full build/typed reader. Legends state five calls,
one excluded warmup, separate setup/qualification and no resident topology.
`event_profile=0` means CUDA event timing is absent; synchronous host phase
instrumentation remains enabled and cannot be described as an unprofiled
kernel-only measurement.

Accepted for publication with the report explicitly naming same-coordinate
topology instances, CPU placement sensitivity, lack of established full-call
gain, and the separately reported independent-qualification costs.

## Final report review

Read the complete results report after integration. All native/full medians and
ranges, setup and qualification costs, source/binary provenance, measured sm_60
target despite global cached architecture 52, precise seven-result qualification
scope, generic seam limits and same-coordinate occurrence wording agree with
saved artifacts. Independently recomputed the 24 derived construction/reader
stage summaries by summing the four host spans per iteration before taking
medians; all values agree. CPU/GPU initialization placement differences are
explicit, and these stage comparisons do not replace native total.

The report correctly rejects universal GPU/native gains and established full
Python-caller speedup. Only one wording clarification requested: distinguish
aggregate large-matrix quality/timings from the four callers' saved binary/SVG
offline reconstruction. All 168 matrix outputs were directly qualified on the
fleet, but their full large arrays were not serialized for a second offline
qualification. No numerical, topology or implementation finding remains.
