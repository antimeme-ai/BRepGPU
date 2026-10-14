# Independent retained discovery performance review

The strongest supported headline is **about 36× faster resident conservative
discovery, or 29–30× faster with new host queries and downloaded native
consumption, on each P100 versus the owned CPU24 implementation**, for the
65,536-request quarter-cylinder benchmark using Bernstein bounds. This is one
named local rational patch, depth 8, 8,191 visits, eight refinements and squared
gap tolerance `2^-24`. Every row attains that gap. It establishes neither a CAD
modeling speedup nor performance on arbitrary independent targets.

This review reads retained data and owned source only; it performs no native or
remote execution. Sources are the four [native result directories](../results/discovery-2026-10-14/),
[benchmark](../benchmarks/discovery/benchmark.cpp),
[fixtures](../benchmarks/discovery/fixtures.hpp),
[runner](../benchmarks/discovery/run.py) and
[figure scope](../results/discovery-2026-10-14/figures/FIGURES.md).
The main and off-grid tables currently contain CPU24 and each P100; no CPU1
comparison is included here. The depth sweeps also contain CPU1. The six initial
CPU1 off-grid cases were excluded from public results because copying overlapped
their run; their preserved originals are not used here. Controlled CPU1 reruns
are pending and require their own inspection.

## Matched operation and attained precision

Specific rows in [native-cpu24-gpu/summary.tsv](../results/discovery-2026-10-14/native-cpu24-gpu/summary.tsv):
`corpus=cylinder,count=65536,mode=2,depth=8,max_visits=8191,max_refinements=8,tolerance_squared=5.9604644775390625e-08`.
Times below are seven-sample medians; brackets contain observed min/max times,
not confidence intervals. Each device ran separately.

| Phase | CPU24 ms | P100 0 ms | P100 1 ms | CPU24/P100 0, 1 |
| --- | ---: | ---: | ---: | ---: |
| Resident discovery | 1,255.984 [1,145.678–1,420.614] | 34.749 [34.619–35.092] | 34.695 [34.658–34.829] | 36.14×, 36.20× |
| Cached-hierarchy host to host | 1,235.173 [1,216.591–1,293.619] | 41.047 [40.896–51.260] | 42.162 [40.501–42.604] | 30.09×, 29.30× |

All three have 65,536 GAP and coverage-complete rows. CPU maximum squared
bracket width is `8.526512829121202e-14`; GPU maximum is
`7.549516567451064e-14`. Mean widths are `5.093932391187722e-14` and
`4.538686579198323e-14`, respectively. Requested precision and successful gap
population match; attained widths are slightly different. Mean work matches:
30.4997 visits, five refinements and 37.3000 surface evaluations per request.
These widths certify a squared-distance interval, not unique nearest UV or a
world-space spatial error bound.

The regular cylinder generator repeats 60 distinct targets at this batch size.
Its 15 UV pairs use multiples of `1/16`, aligned with depth-8 cell boundaries
(four splits per parameter). Four radial scales complete the target period.
The axis generator has only 15 targets. Repetition is appropriate for measuring
this resident workload, but does not simulate 65,536 independent CAD queries.
At the same Bernstein settings, CPU24 wins at `count=1` (0.222 ms versus
1.853/1.811 ms) and `count=256` (4.544 ms versus 5.897/5.929 ms). The large-batch
headline must retain its batch size.

## What each timer includes

`resident_discover` allocates a new result, executes discovery and synchronizes,
using an existing immutable query/hierarchy. Download, native consumption and
cleanup occur afterward. `download_prepared` writes preallocated output arrays;
consumption is outside that timer. `cached_hierarchy_host_to_host` creates a new
query snapshot/upload, discovers, downloads into prepared arrays and consumes
the numerical results. Result/query destruction is the separate
`prepared_cleanup` phase. None of these includes initial context/patch/hierarchy
setup, input generation, output allocation, Python wrapping, provenance export
or file export. CUDA event `profiled_kernel` is a separate profiling-context
measurement, not the full native API call.

Warmups are excluded. Each repeated phase has samples 0–6; first-use and final
cleanup observations have sample -1 and one observation. Setup-phase quality
columns repeat the initial consumed result statistics; they do not describe a
query executed inside the setup timer. All actual operation phases consume and
check their corresponding answers. Medians of independently timed phases must
not be added to invent a measured full call. In particular, the slightly lower
CPU cached-host median than resident median is not negative transfer overhead.

## Off-grid results and method choice

The [controlled off-grid table](../results/discovery-2026-10-14/native-offgrid/summary.tsv)
uses `count=4096`, the same depth/budgets/tolerance, and odd multiples of `1/64`.
These are interior offsets rather than depth-8 cell boundaries. Cylinder has
1,024 distinct targets repeated four times; axis has 32 distinct targets.
Every method has 4,096 coverage-complete rows and **zero GAP rows**.

| Off-grid corpus/method | CPU24 resident ms | P100 0 / 1 resident ms | GPU mean / maximum squared width |
| --- | ---: | ---: | ---: |
| Cylinder / AABB | 13.912 | 6.689 / 6.607 | 0.024834 / 0.063931 |
| Cylinder / 26-DOP | 18.651 | 8.932 / 8.709 | 0.016816 / 0.061578 |
| Cylinder / Bernstein | 70.429 | 5.010 / 5.072 | 0.001518 / 0.002070 |
| Axis / AABB | 27.607 | 2.097 / 2.095 | 0.108141 / 0.108141 |
| Axis / 26-DOP | 58.612 | 4.038 / 4.059 | 0.090218 / 0.090218 |
| Axis / Bernstein | 192.518 | 4.712 / 4.718 | 0.000732422 / 0.000732422 |

Bernstein tightens these brackets substantially, but does not meet `2^-24`.
Its cylinder mean work is 28.3359 visits and 3.0957 refinements, with no
refinement-limit rows; axis has 91 visits and every row reaches the refinement
budget. These are different failure-to-resolve patterns. Increasing a projector
budget alone is therefore not an established fix for the cylinder case.
Regular-cylinder AABB/DOP timings must not be compared to Bernstein as equal
achieved-accuracy methods: those methods also have zero GAP rows at depth 8.
Plane needs no stronger method in the measured batch; oriented diagonal
geometry benefits specifically from 26-DOP. There is no universal best mode.

## Setup, storage and next measured target

Depth 8 has 511 nodes. The `cache_bytes` column actually totals nodes, method
cache and roots: AABB 40,884 bytes; 26-DOP 122,644; Bernstein 2,044,004. Bernstein
coefficients alone occupy 2,003,120 bytes. These figures exclude patch/query/
result storage and retained host snapshots, so they are not total process or
device memory. A fresh 65,536-query CUDA batch transfers 1,835,008 input bytes
and 5,767,168 output bytes. Hierarchy construction uses CPU interval arithmetic
for both backends; the CUDA build phase additionally uploads the packed arrays.

For the headline Bernstein case, repeated hierarchy-build/upload medians are
36.208 ms CPU24 and 25.149/25.154 ms P100, with substantial observed ranges.
The comparison TSV consequently gives zero *extra* hierarchy setup and zero
estimated hierarchy reuse calls. This is a conditional difference of independent
medians, not zero setup or immediate end-to-end benefit. Single context-first-use
observations are 0.019 ms CPU and 295.331/460.960 ms CUDA. Application first use,
patch setup, output allocation and their variability are not amortized by that
estimate. [Figures](../results/discovery-2026-10-14/figures/) expose both requested
conditions and achieved widths; their hierarchy-only estimate labels are sound.

The clearest measured optimization target is **the shallowest sufficient
hierarchy/method for the requested precision**, followed by bounds that resolve
off-grid curved targets. The [diagonal sweep](../results/discovery-2026-10-14/native-depth-diagonal/summary.tsv)
already gives all 4,096 GAP rows with root-only 26-DOP: 244 packed bytes and
0.214 ms P100 0 versus 1.239 ms at depth 4. The
[axis-midpoint sweep](../results/discovery-2026-10-14/native-depth-axis/summary.tsv)
gives all 4,096 GAP rows with Bernstein depth 2: 28,004 bytes and 1.023 ms
P100 0 versus 2,044,004 bytes and 7.915 ms at depth 8. CPU24 is 21.369 versus
313.680 ms. Both depths achieve widths below `2^-24`; their widths are not
identical. Axis-midpoint repeats one target, so these results motivate a measured
depth-selection experiment, not a universal shallow-tree policy. Kernel staging
or queues are not shown to address the measured precision limitation.

The raw groups, summary extrema/medians, sample labels and complete requested
condition matches were inspected across all four tables. Every repeated quality
field is unchanged across its seven samples. Native consumers directly check
the exact dyadic analytic global minimum and reported gap; descriptive point/
distance checks have stated arithmetic allowances. Exact original-input UV
upper qualification is supplied separately by the
[standalone and actual-cloud evidence](../results/discovery-2026-10-14/QUALIFICATION.md),
including 24 cloud batches. Neither CPU parity nor timing supplies geometry
truth. No trim-valid witness, cross-frame distance, general B-rep nearest face,
complete minimizer enumeration, intersection or topology operation follows from
these results. No material retained-data or figure-scope blocker was found.
