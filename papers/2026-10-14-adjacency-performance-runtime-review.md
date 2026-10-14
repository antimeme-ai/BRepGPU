# Adjacency performance: independent runtime and scope review

Reviewed saved results/adjacency-2026-10-14/benchmark artifacts and the measured
controller/caller/native source on 2026-10-14. Analysis used only standalone
stdlib CSV, hashing and arithmetic over retained files. No product import,
native execution, new measured trial or production edit occurred.

## Artifact consistency

The campaign contains 24 cases, 2,544 raw rows and 720 summary rows. Independently
recomputed every summary sample count, median, minimum and maximum from raw data;
all match exactly. All durations are finite and nonnegative. The fourteen source
hashes match the inspected local source files. Measured library SHA256
`37ff97b5586dfdf869e828657a44ab2c40a029e2c02468e1f8aa4c1c7e1a3900`
matches the retained qualified release library hash. This checks recorded
identity consistency; root's before/after campaign checks establish unchanged
fleet sources/binary during execution.

Metadata matches the three declared sizes, both storage orders and four
backends. Each case records five repeats, one warmup, per-call residency and no
CUDA event profiling. Every case's saved consumption counts read every edge and
member, and every emitted mate/neighbor for these paired primitive families.
Each case records seven independently qualified results: cold, warm and five
repeats. Consumption sums are observations of reads, not an independent oracle;
the separate relation qualification provides that mechanism.

For all 120 repeated calls, summed native phase durations do not exceed native
total; native total <= wrapper native-call wall <= external build wall. Actual
build-plus-reader wall contains external build plus Python reader durations.
Do not sum phase medians as though they came from one call. No additional timing
consistency mechanism is required.

## Measured value and its boundary

Largest cases contain 32,768 separate plate occurrences, 491,520 edges, 229,376
faces and 983,040 coedges. They do not constitute one complex constructed solid.

| Large input | CPU1 native median [min,max], ms | CPU24 | P100 0 | P100 1 |
| --- | --- | --- | --- | --- |
| Ordered | 8.193 [7.638,9.409] | 9.063 [7.995,21.265] | 17.325 [15.959,18.780] | 17.008 [13.873,18.841] |
| Permuted | 100.415 [83.586,166.782] | 19.427 [18.142,22.115] | 21.789 [19.612,32.357] | 15.306 [14.981,19.341] |

CPU1 wins the ordered native operation. On permuted input P100 1 has a lower
median than CPU24, but their ranges overlap. P100 0 does not beat the best CPU
median. A broad native GPU speedup claim is unsupported; hardware placement and
storage distribution materially change the measured operation's ranking.

The count/scan/fill/reader phases are faster on GPU. Summing those four disjoint
host spans *within each iteration*, then taking medians, gives 5.572 ms for
ordered CPU24 versus 0.315–0.317 ms for GPUs; permuted CPU24 is 16.219 ms versus
1.397–1.419 ms. These are derived synchronous phase totals, not kernel-only
event timing and not complete native/caller cost. Publication should prefer the
actual phase table over presenting this derived ratio as the main result.

Full large caller medians are 4.000–5.091 seconds and the combined observed
ranges span approximately 3.79–5.81 seconds. The Python reader dominates these
costs. Backend full-caller ranges overlap; no full-caller gain is established.
The tiny ordered mixed-body native CPU1 median is 0.002235 ms, compared with
CPU24 3.950876 ms and GPUs 0.867931/0.505896 ms. Worker creation and CUDA call
setup are included; selecting GPU or 24 threads unconditionally is not warranted.

## Transfer, setup and qualification

Actual native upload is `12*C` bytes for edge slots, face slots and signs.
Download is `8*E+12*C+4` for offsets, kinds, members, mates and neighbors. The
largest call uploads 11,796,480 bytes and downloads 15,728,644 bytes. There are no
source-identity or geometry uploads. These are host/device transfers, not fleet
network traffic. Allocation storage, scan scratch traffic and source tables on
the CPU are different costs, not additional DMA bytes.

GPU warm native calls still create stream/buffers, upload, build, read, download,
free and restore. Only host prepared input, loaded library and initialized CUDA
runtime persist. Host phase clocks synchronize boundaries, so the reported
operation includes that instrumentation schedule. No resident topology or
asynchronous overlap claim follows.

Large packing takes roughly 8.08–9.10 seconds; large global permutation adds
7.77–8.50 seconds. Cold-to-first-consumer is 12.31–13.40 seconds ordered and
21.25–22.43 seconds permuted, including packing/library load/first build/typed
reader. Initial independent qualification, outside that cold scope, takes
15.86–16.72 seconds ordered and 21.79–23.54 seconds permuted. Repeated independent
qualification is also outside timed caller wall, with per-case medians
10.48–16.10 seconds. It is not free and is not hidden in the reported operation.

Large first-call native CUDA context spans are 218–252 ms; warm context spans
are much smaller. Ordered P100 1's first total is 444.993 ms, larger than its
218.686 ms context span; do not equate all cold overhead with context creation.
Permuted P100 0 upload median is 8.528 ms versus P100 1's 3.898 ms. These data
do not identify a NUMA, PCIe, page-locking or device-clock cause. Such attribution
needs a separately designed experiment and is deferred with integration tuning.

## Environment ambiguity and disposition

Environment TSV records the CMake cache default `CMAKE_CUDA_ARCHITECTURES=52`,
while the actual target source property specifies `CUDA_ARCHITECTURES 60-real`.
The cache field is not the target build property. Keep the historical artifact
intact, explain the difference, and record root's direct `cuobjdump` inspection
of the measured release binary showing the sm_60 image. Do not relabel the cache
value as the actual target architecture or infer the binary from that field alone.

Recommendation: accept measured costs/scopes and identity consistency. Publication
must retain the architecture clarification and avoid GPU full-operation,
full-caller, kernel-only, resident-topology or single-solid speedup claims. Record
the observed packing/permutation, reader, transfer and allocation costs as
round-two opportunities; no active optimization lane or repeated campaign is
required by these observations.

## Final report disposition

Read the completed results/adjacency-2026-10-14/REPORT.md and independently
recomputed all 24 rows in construction-reader-stages.tsv directly from the four
raw per-iteration spans. Every derived median/min/max matches. Reported native,
full-caller, cold setup, initial/repeated qualification and transfer values match
the saved artifacts. Actual-cuda-architecture.tsv now records target `60-real`,
`--generate-code=arch=compute_60,code=[sm_60]`, the embedded sm_60 ELF name, the
historical global cache default 52 and the same measured library hash. The
architecture ambiguity is resolved without rewriting historical data.

The report explicitly records different initialization placement: CPU initial
counter/sentinel writes are in allocation, GPU zeroing/sentinels in count/reader.
Its four-phase totals consequently name those measured host spans rather than
claiming a kernel-only or identical initialization scope. Exact CPU24/GPUs stage
median ratios are 17.59/17.67 ordered and 11.43/11.61 permuted. An approximately
17.6 and 11.4–11.6 ratio is supported only with that named scope and the separate
complete-native/caller caveats. Full native is the corresponding operation cost.

Two small wording findings were sent to root for integration: qualify the phrase
"every call uploads" as CUDA calls because CPU has no DMA, and replace ambiguous
"1.27 times lower" with approximately 21% lower median or an explicitly named
CPU24/P1001 ratio. Neither affects product, measurements or qualification. Accept
the final report after those wording dispositions; no additional native execution
or performance campaign is warranted.
