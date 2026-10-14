# GPU-resident seeded projection — 2026-10-14 UTC

The second owned operation refines provided UV seeds against resident rational Bézier patches. Intermediate jets and iterations stay on the P100; the public result is a first-order box-stationary candidate with separate rank, curvature and unresolved-status diagnostics. The operation does not promise general nearest points, trim membership or solid topology.

**All 144 native cases qualified**, including every measured download and the deliberate adverse population. CPU and both P100s passed the 104-case exact projection corpus. Both cards passed all 43 public tests and device memory checks. [Usage](../../docs/ITERATION_2.md) · [Reviewed design](../../docs/ITERATION_2_DESIGN.md) · [Accepted plan](../../docs/ITERATION_2_PLAN.md).

## Nonlinear cylinder throughput

1,048,576 requests, one selected rational quarter-cylinder carrier, independently constructed off-surface dyadic targets and distinct seeds. Every row returned STATIONARY with positive curvature and passed independently known UV/point/distance checks. The seven-patch snapshot contains the benchmark's analytic carriers; this corpus selects the cylinder slot.

Median milliseconds, seven samples after one excluded warmup:

| Backend | Resident project | Prepared download | Prepared host-to-host |
| --- | ---: | ---: | ---: |
| CPU, 1 worker | 2710.418 | 24.972 | 2757.002 |
| CPU, 24 requested workers | 229.728 | 28.304 | 227.462 |
| P100, device 0 | 25.270 | 29.035 | 87.579 |
| P100, device 1 | 25.271 | 22.515 | 77.353 |

Against this 24-worker CPU implementation, the GPUs are about **9.1× for resident projection** and **2.6×/2.9× for prepared native host-to-host**. This is a numerical query operation, not a CAD Boolean or topology speedup. The default cylinder population needs 2–7 accepted updates (mean 5.350) and 5–15 evaluator calls (mean 11.700). No intermediate jet is downloaded between updates.

CPU24 resident samples span 164.069–247.634 ms and prepared host samples span 216.957–296.026 ms. Prepared host and resident/download rows time separate calls; their medians must not be added or subtracted. Placement, allocation/cache state and shared-host variation can make an independently measured host median lower than a resident median.

## Other operations and small batches

At 1,048,576 requests:

| Corpus | CPU24 resident | P100 0 resident | P100 1 resident | CPU24 host | P100 0 host | P100 1 host |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| plane | 106.715 | 13.181 | 13.112 | 152.620 | 75.413 | 65.187 |
| tilted | 122.065 | 15.099 | 15.053 | 168.619 | 77.390 | 71.036 |
| parabolic | 222.573 | 27.597 | 27.556 | 301.268 | 93.222 | 86.595 |
| mixed | 154.080 | 27.376 | 27.556 | 195.013 | 89.649 | 86.699 |
| adverse | 53.773 | 7.668 | 7.692 | 98.272 | 64.642 | 66.800 |

Plane/tilted/parabolic/cylinder/mixed regular corpora have independently known solutions; all their rows returned stationary positive-curvature candidates. The parabolic normal-target family has a unique analytic solution and positive distance Hessian throughout the box. Mixed regular rows interleave these four maps and retain their differing work counts.

The **adverse** corpus uses different options (one update, zero halvings) and is not solved-projection throughput. At N=1,048,576 it retains 131,072 rows of each deliberate outcome: update limit, exhausted line search, rank refusal, bad slot, bad target, stationary saddle, flat cylinder candidate and weak-corner candidate. Exactly three eighths are stationary, with one eighth negative curvature and two eighths ambiguous. Invalid rows remain in the timing population with all floating fields NaN and zero counters/flags.

CPU1 wins at N=1 and 32 in this matrix; GPUs win by N=256 for this cylinder population:

| N | CPU1 resident | P100 0 resident | CPU1 host | P100 0 host |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 0.003235 | 0.285010 | 0.004587 | 0.388462 |
| 32 | 0.117046 | 0.396133 | 0.118624 | 0.500986 |
| 256 | 0.830478 | 0.429280 | 0.851678 | 0.653413 |
| 4096 | 10.350513 | 0.433025 | 10.373192 | 1.135463 |
| 65536 | 162.044744 | 1.909639 | 163.452743 | 5.649763 |

## Measurement scope and raw artifacts

[Every wall/event sample](native/samples.tsv) · [Medians and ranges](native/summary.tsv) · [Per-case work histograms](native/work/) · [Environment](environment.tsv).

The matrix is 4 backends × 6 counts × 6 corpora: CPU 1/24 requested workers and each P100; N=1,32,256,4096,65536,1048576. It writes 5,976 timing rows. All native calls use the same request bits/options/required fields. Synthetic targets repeat deterministic dyadic families; this is not an imported CAD corpus.

- `input_generation_and_host_output_allocation`: fixture generation and prepared destination allocation, outside the solve/host timers.
- `context_first_use`: fresh public context/plugin/device initialization in each native process.
- `snapshot_upload`: native patch/query copies, validation and upload. Queries are 44 bytes/request plus the patch snapshot.
- `resident_project`: reusable inputs, fresh result allocation, complete bounded solve, stream completion/device restoration and public publication.
- `download_prepared`: seven FP64 planes plus four u32 slabs into prepared destinations, 72 bytes/request.
- `prepared_host_to_host`: new patch/query snapshot/upload, project, download and required status consumption in an existing context. Generation, independent geometric qualification and close are outside.
- `prepared_cleanup` and `context_cleanup`: separately measured resource close.
- `profiled_kernel`: event interval around submission on a separate profiling context, including possible scheduling/submission gaps; not public latency and not subtracted from other runs.

Each timed batch is checked outside its timer against expanded long-double analytic maps/derivatives, original target, exact returned UV, box residual, nonincrease from seed, independent status/curvature witnesses and bounded counters. Known-answer allowances are confined to the named conditioned basins. The counter histograms retain resident/prepared/profiled samples separately. Non-project timing rows carry their associated qualified-batch metadata; setup/first-use rows carry the excluded warmup's outcomes.

Across this native corpus, maximum reported-point/distance/residual discrepancies are 2.220446049250313e-16, 4.443060502845597e-16 and 4.852888924045118e-16. Acceptance uses the reviewed finite-corpus E0/E1 and propagated arithmetic budgets, not universal error enclosures. CPU/GPU parity is not the oracle.

The dual Xeon E5-2697 v2 host has 24 physical/48 logical CPUs; workers are unpinned and start/join per call. CPU Bernstein is a portable implementation without a persistent worker pool, tuned SIMD or a claim to be the fastest CPU projector. The two 16 GB P100s run native sm_60 under CUDA 12.8.1, driver 580.178.04. No owned build/test job overlapped the published matrix; clocks, SMT placement and other users' workloads remain uncontrolled. These limitations and visible ranges constrain the speedup claims.

The projection kernel's strict-FP ptxas report is 166 registers, 224-byte stack, zero spill stores/loads. Default wall samples disable events. No fast math or implicit FP contraction is enabled; explicit evaluator interpolation FMA remains.

## Exact geometry, runtime and memory checks

[CPU exact rows](qualification/cpu-0/QUALIFICATION.md) · [P100 0 exact rows](qualification/cuda-0/QUALIFICATION.md) · [P100 1 exact rows](qualification/cuda-1/QUALIFICATION.md) · [Actual preserved-binary compatibility](compatibility.md).

104 requests per backend: independent Fraction geometry at returned binary64 UV, affine boundaries, nonlinear radial/normal targets, distinct endpoint-local candidates, regular saddles/flats, weak cones, strict metric/curvature threshold equality, admitted non-nearest conditioned seed, degenerate charts, search/update limits and bad-row precedence. Each directory preserves controls, target/seed/candidate bits, exact ratios, worst witnesses, violations and work/status distributions. All three runs have zero violations.

All 43 public tests passed on each GPU; CPU passed with two explicit CUDA-device-only skips. The retained evaluator suite still checks all sixteen degree pairs and all derivative orders against exact supplied inputs. New checks cover ownership/provenance/options, wrong/stale/foreign handles, all output overlap pairs, diagnostic aliases, wrapper allocation/post-return exceptions and actual device restoration through the projection callbacks and host workers.

21 native CTest cases passed in Release and ASan/UBSan: 20 callback-model lifetime/loading/failure cases and a direct scalar math check. ASan/UBSan also passed all six native corpora at N=4096 with CPU24. A fresh-process Python probe composes wrapper MemoryError with modeled fatal native cleanup, preserving the primary exception, exposing fatal cleanup, draining tokens, poisoning both CUDA families and retaining CPU usability. These are modeled failures, not induced hardware errors.

Compute Sanitizer ran the 43-test suite on each real P100: zero errors, zero leaked bytes and zero remaining allocations. The preserved actual core/plugin and committed old caller pass all eight compatibility combinations (21,840 independent component checks). Old interfaces refuse projection nonfatally and still evaluate after refusal.

## Actual Python point-cloud caller

[GPU cylinder visualization](caller/cuda-0-cylinder/cylinder-projection.svg) · [GPU polynomial visualization](caller/cuda-0-polynomial/polynomial-projection.svg).

Six actual 1,024-request calls (CPU and both GPUs, each shape) passed exact geometry/known-answer qualification after native closure. Each caller directory contains records/controls with exact binary64 bits, individual phase timings, an SVG and its recipe. The target family has independently known nearest solutions; this fact does not enlarge the product's general seeded-stationarity contract.

Python reports preparation, context first use, patch/query packing and upload, project, allocating result download plus copied host provenance, cleanup, exact qualification and file/visual export separately. Host provenance adds 44 bytes/request plus descriptors/options and Python objects beyond the 72-byte result. These are single caller samples, not the native medians. Source-CAD export/import, global search, trims/topology and automatic CPU fallback are absent.

## Source identity and reproduction

Source checkpoint: [`e9d63c56a0af18e1e5eb38101cf86e226d0e695a`](https://github.com/antimeme-ai/BRepGPU/commit/e9d63c56a0af18e1e5eb38101cf86e226d0e695a). [Source SHA-256](source.sha256) covers 33 compiled/called files; the execution-host copies match those committed bytes. [Binary identities](binary.sha256) identify the production core/plugin/native benchmark and separate sanitized native-check build. Private operational logs and host identifiers remain outside the repository.

The [operation guide](../../docs/ITERATION_2.md) contains CPU/CUDA build, qualification, native-check, benchmark and cloud commands. The reusable native runner produces the raw timing/histogram files; the independent qualifier produces the exact corpus artifacts. Preserve the actual old binaries before changing builds if rerunning the compatibility checker. No dependency beyond the owned C++17/CUDA and standard-library Python machinery was adopted for this operation.
