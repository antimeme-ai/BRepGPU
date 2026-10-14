# Resident conservative surface-pair discovery

2026-10-14. The next breadth-first beachhead is delivered: immutable common-world
carrier bounds, resident CPU/CUDA pair indexes, deterministic bounded mask pages
and a body-aware shortlist retaining exact source identity. **A survivor is a
closed-AABB overlap, not an intersection or trimmed-face contact.**

For the larger measured scene, 936 left charts ×940 right charts produce
879,840 examined pairs, 1,823 survivors and 878,017 excluded chart pairs
(99.79%). Complete reused shortlist medians are **9.631 ms CPU24 versus
7.776/6.920 ms on the two P100s**, descriptive ratios1.24/1.39. CPU1 takes19.182 ms.
These calls retain the common-world indexes and include page creation,
download/immutable host conversion, child cleanup and Python survivor decoding.
The smaller scene favors CPU1. Cold setup favors CPU. No whole-modeling,
intersection, Boolean or GPU-topology speedup follows.

![Actual P100 carrier-pair mask and retained body identities](caller/p100-0/pairs.svg)

## Correctness and lifetime

The geometry oracle reads original homogeneous controls as exact Fractions.
Positive-weight projected-control extrema enclose each full rational carrier;
downloaded depth0 local roots must enclose those extrema. World boxes must
enclose exact transformed original extrema and actual local-root endpoints.
The caller downloads actual resident CUDA boxes and checks bit-identical input
storage before qualification. Every output bit is independently checked using
exact closed comparisons; equality/degenerate boxes remain candidates. Native
ordering uses finite IEEE bits, including signed-zero equality and subnormals.

| Execution check | Outcome |
| --- | --- |
| CPU1 complete existing/new public suite | 83 tests;78 executed,5 inapplicable CUDA skips; PASS |
| CPU24 direct pair suite | 13 tests;12 executed,1 inapplicable CUDA skip; PASS |
| Focused family suites, each P100 under Compute Sanitizer | 68 tests/card, no skips, zero errors and zero device leaked bytes |
| Native ASan/UBSan with host leak detection | 60/60 PASS, including 18 new pair scenarios |
| Actual preserved core/plugin/adapter compatibility | Eight fresh-process cases PASS; analytic jets before/after unsupported pair requests |
| Production mutants, fleet-only isolated copies | Closed contact, page offset, tail padding and world origin all caught by direct assertions |
| Actual exported callers | All 15,024 examined bits,314 survivors,14,710 exclusions and1,624 original controls independently checked; zero violations |

New direct cases include extreme finite/subnormal/signed-zero inputs, true
face/edge/corner touches, asymmetric and unaligned pages, exact tail zeros,
empty/terminal cases, maximum page output and total2^32 with a one-pair final
page. They check whole requested chart selection, different-frame coincidence
and separation, analytic transverse/tangent contacts and explicit hole/concentric
cylinder false positives. Resource checks cover actual input copies, retained
parents, foreign/stale/wrong handles, output aliases/extents, Python publication
failure and primary exception preservation. Real CUDA callbacks restore each
caller's prior device, including host worker threads. Native modeled failures
attack upload/restore/copy/publication/cleanup, lazy capability negotiation and
cross-family poison; they do not inject physical GPU faults.

The CPU full suite completed before the legacy scope was refined. The P100
regression checks use complete body/projection/jet/runtime/pair suites plus the
existing three focused discovery-mode tests: actual all-mode callers, profiles,
tails, source snapshots, retained parents and device restoration. A broad GPU
continuation was intentionally interrupted at unchanged deep Bernstein cache
qualification; its private log remains. Deep subdivision arithmetic, bound
caches and generic discovery solver were unchanged and retain their earlier
qualification. [Reviewed scope](../../papers/2026-10-14-pair-qualification-scope-review.md).
Completed CPU checks were not repeated.

[CPU records](qualification-cpu.tsv), [P100 records](qualification-cuda.tsv),
[native record](qualification-native.tsv), [selected suites](suites.tsv),
[actual binary compatibility](compatibility.tsv) and [mutants](mutations.tsv)
retain the execution outcomes. Previous protected discovery-release libraries
remain byte-identical; actual body-era binaries/adapter were archived separately.

## Actual callers and serialization

| Caller | Charts left/right | Examined / survivors | Exact exported-control records |
| --- | ---: | ---: | ---: |
| CPU24,4 bodies/side | 28/28 | 784/43 | 264 |
| P1000,4 bodies/side | 28/28 | 784/43 | 264 |
| P1001,16 bodies/side | 116/116 | 13,456/228 | 1,096 |

Each caller directory retains original patch/control hex records, body/frame/
face/chart identities, actual retained local/world boxes, page metadata,
little-endian LSB-first u32 mask, complete typed survivor TSV, qualification and
phase times. Output identity is scoped to its process/case: body IDs/revision1
are snapshot provenance, not persistent cross-session names. Geometry and
topology remain owned CPU records; no pointer address is exported as identity.

The SVG draws actual surviving bits. Larger drawings explicitly crop to the
first100 charts per side; the P1001 view shows190 of its228 survivors. Full
mask/TSV coverage is unchanged. Cleared bits exclude whole untrimmed carriers
and therefore their trimmed subsets; they do not individually exclude a
multi-chart face or whole body. All required chart pairs must be accounted for.

Actual export-inclusive walls were55.27 ms CPU24,366.47 ms P1000 and800.30 ms
P1001. These are separate cold single observations, with the P1001 scene larger.
They include body/bounds preparation, engine/index setup, page, actual input
download, exact qualification, cleanup and export, ending before the final
timing-file write. They are not a matched backend comparison.

[Independent output review](../../papers/2026-10-14-pair-artifact-oracle-review.md)
rechecks factory control bits, source UVs/identity, enclosure and every exported
mask/shortlist row using standalone Fraction/struct/CSV processing. Its retained
input review complements the caller's separately checked resident input download.

## Matched reused costs

![Complete reused shortlist costs with five-sample ranges](matrix/costs.svg)

| Bodies/side · examined / survivors | Backend | Actual shortlist median [min,max],ms | Native page host-span median,ms |
| --- | --- | ---: | ---: |
| 4 ·784/43 | CPU1 | .144 [.131,.249] | .043 |
| 4 ·784/43 | CPU24 | 1.586 [1.475,1.680] | 1.481 |
| 4 ·784/43 | P1000 | .295 [.266,.330] | .108 |
| 4 ·784/43 | P1001 | .341 [.274,.391] | .121 |
| 128 ·879,840/1,823 | CPU1 | 19.182 [18.860,20.595] | 12.330 |
| 128 ·879,840/1,823 | CPU24 | 9.631 [9.181,10.321] | 1.864 |
| 128 ·879,840/1,823 | P1000 | 7.776 [7.305,8.774] | .144 |
| 128 ·879,840/1,823 | P1001 | 6.920 [6.635,7.260] | .125 |

The large native page host span is12.93/14.91 times faster on P100 than CPU24,
but whole shortlist ratios are1.24/1.39. Large download/immutable-conversion
medians are4.27–4.98 ms, and survivor decoding2.16–2.71 ms. Host mask conversion
and enumeration are now the main reused costs. `download_ms` includes metadata
read, host allocation, actual copy/synchronization, tuple conversion and
metadata/padding checks; it is not a measurement of PCIe transfer alone.
CPU24 creates native workers per call, which dominates this tiny25-word scene.
No thread pool or automatic backend dispatcher is added in this pass.

Cold-to-first-page observations (before decode/qualification/final cleanup):

| Bodies/side | CPU1 | CPU24 | P1000 | P1001 |
| --- | ---: | ---: | ---: | ---: |
| 4 | 14.09 ms | 15.27 ms | 285.56 ms | 486.84 ms |
| 128 | 393.14 ms | 344.97 ms | 640.25 ms | 839.90 ms |

CPU scene/root/world preparation alone takes8.59–11.12 ms small and188.75–202.31 ms
large; body construction is separate. Index setup including Python packing is
.36–.82 ms small and3.59–5.23 ms large. Initial exact qualification is outside
reused timings. Different placements require fresh world bounds/indexes in this
immutable first implementation; the cached win does not include that rebuild.

## Measurement method and bytes

Eight cases run in a fixed serial order, with a fresh process per case:4/128
mixed box/cylinder/plate bodies per side ×CPU1/CPU24/P1000/P1001. Distinct body
frames form an admitted grid, with a translated second scene; this is not a
duplicated-box throughput fixture. Each fresh engine/index pair returns a cold
page, independently qualifies all actual input bounds and mask bits, performs
one excluded warmup, then five reused calls. Reused calls retain checked
metadata/source/padding and decode the full survivor list; repeat output/count
comparisons occur after the wall timer. Excluded preparation/qualification/
export are not added back as sums of independent medians.

Eight single phases and six repeated phases produce 304 finite raw observations
and 112 recomputed summaries. `actual_shortlist_caller_ms` spans page publication
through final survivor tuple creation. `reused_wall_ms` spans page construction,
download/checks and successful native child cleanup, before decoding.
`native_page_ms` is a host call span including fresh result allocation, launch,
synchronization/restoration and publication; it is not pure kernel time. No
CUDA event profiling was enabled in this matrix. `cold_to_first_page_ms` includes
body/CPU-scene preparation, engine/index setup and first returned page, stopping
before survivor decoding, input download/qualification and final cleanup.

CUDA payloads, excluding host metadata and runtime allocations:

| Scene | One-time index upload | Mask download/call | Topology upload |
| --- | ---: | ---: | ---: |
| 28×28 charts | 2,688 B | 100 B (25 words) | 0 B |
| 936×940 charts | 90,048 B | 109,980 B (27,495 words) | 0 B |

These are host-to-device/device-to-host bytes, not server/network traffic. CPU
receipts record zero CUDA bytes. Bounds, source records and immutable Python
word/survivor objects add host storage/work. Packed bits avoid a record for each
excluded pair; this implementation still performs all Cartesian comparisons.

All builds, copying, qualification, mutants and actual exports completed before
these trials. No other owned work overlapped them. The host is shared/unreserved;
affinity, frequency and clocks were uncontrolled, and the order was not randomized.
Five samples establish observed costs for these matched fixtures, not universal
hardware superiority or a statistical guarantee. Ranges/outliers are retained.

[Raw data](matrix/raw.tsv), [summaries](matrix/summary.tsv), [all cases](matrix/cases),
[17 source identities](matrix/sources.tsv), [actual binaries](matrix/binaries.tsv),
[environment](environment.tsv) and
[independent performance review](../../papers/2026-10-14-pair-performance-review.md).

## Reproduce and next theme

Build through [the established CUDA12 toolchain](../../docs/CUDA_TOOLCHAIN.md)
into a new owned directory (`build/pair-release`). Use
`BREPGPU_ENABLE_CUDA=ON`, the explicit compiler/toolkit prefix and real sm60.
Host fault checks use a separate build with `BREPGPU_ENABLE_TEST_HOOKS=ON` and
`BREPGPU_ENABLE_SANITIZERS=ON`. The new operation changes neither previous ABI
layouts nor numerical compilation flags. CPU core and CUDA plugin digests are
`b63af7f6d64ab06f2dccfdd85b27931ec008a8586313463d2d4cbac4ce8dcfd6`
and `4aeb0d7dd124fd00044f6be7dbe7a04b104748366fd01058e9815f838d148e61`.

```sh
PYTHONPATH=python python3 examples/discover_pairs.py --backend cuda --device 0 \
  --size 4 --library-path build/pair-release/libbrepgpu.so \
  --output-directory context/my-pair-scene
PYTHONPATH=python python3 benchmarks/pairs/run.py \
  --library-path build/pair-release/libbrepgpu.so \
  --plugin-path build/pair-release/libbrepgpu_cuda.so \
  --output-directory context/my-pair-matrix
```

Qualification tools are [public suites](../../scripts/qualify_pairs.py),
[actual compatibility](../../scripts/check_pair_compatibility.py) and
[production mutations](../../scripts/run_pair_mutations.py). Run native work
on the fleet and serialize it with performance. Operational logs/builds/SDK
archives remain private; public data contains no hostnames or private paths.

The [reviewed design](../../docs/PAIR_DISCOVERY_DESIGN.md) uses literature-informed
conservative culling and explicit identity/coverage. BVH/sweep/grid pruning,
tighter boxes, worker reuse and less host boxing are measured return-pass
targets. Following the operator's broad-first direction, move next to
intersection initialization and spatial/UV representation rather than tuning
this beachhead first. Complete SSI branches, general trims, imported bodies,
editable topology and Booleans remain future operations.
