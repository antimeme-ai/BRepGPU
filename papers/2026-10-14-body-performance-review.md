# Body sampling performance artifact review

2026-10-14. Accepted. Independently read the published TSV artifacts under
[benchmark](../results/body-2026-10-14/benchmark), without importing the product
or executing native code, tests, benchmarks or fleet jobs.

Recomputed every median/minimum/maximum from the 1,248 finite nonnegative raw
observations: all 384 summaries match exactly. All 24 cases appear in the
planned shape/resolution/backend order. Each has all seven single observations
at iteration -1 and all nine repeated phases at iterations 0–4, with no duplicate
or missing summary keys. Per-case raw files match the aggregated raw file;
case metadata, candidate/evaluated counts and qualification statistics agree.
For every repeated call, outer wall exceeds the internal span, and the measured
component spans fit within that internal span. No wall value was reconstructed
by adding independent phase medians.

Resolution-65 plate medians in milliseconds, directly recomputed:

| Scope | CPU1 | CPU24 | P100 0 | P100 1 |
| --- | ---: | ---: | ---: | ---: |
| Actual returned-call wall | 940.162 | 852.639 | 873.782 | 877.630 |
| Native evaluate host span | 5.915 | 2.558 | 0.454 | 0.495 |
| CPU normal/scatter span | 799.795 | 763.846 | 789.333 | 797.076 |

Plate actual-wall ranges are respectively 809.173–1253.342,
831.453–871.111, 842.442–918.853 and 835.465–920.216 ms.
Across all 60 resolution-65 calls, normal/scatter occupies 84.7–93.2% of the
same call's actual wall. This is a paired raw ratio, not a sum or ratio used to
reconstruct wall from phase medians. CPU normal/row work dominates this caller.
The shorter P100 native-evaluate span does not establish an end-to-end advantage
over CPU24. Small median differences on other shapes and overlapping ranges
do not support statistical confidence or a hardware-clock explanation.

All eight recorded source hashes match the current local source bytes. The
manifest identifies core SHA-256
`3515b86d0aad1f4bac816b8cc720e2f9452759f2806b6e1938854eaf4fb355de`
and CUDA plugin
`8a6a861c17c7c2c22a0fafe40ce8cc2b46b33272322c8b48af9ce5af24d27efe`;
the core also matches the retained mutation manifest. Actual fleet binary
hashing/preservation was performed by the coordinator's reviewed runner,
not repeated by this artifact-review lane.

The reported scopes remain accurate: fresh process per case, one cold/preparation
observation, initial exact qualification outside reused timing, one excluded
warmup and five reused calls. Reused input retains body/grid/classification,
Engine and PatchSet; outer wall includes final batch publication. Native evaluate
is a host wall span, not pure GPU time. Export is absent from these trials.
Body tables, conversion, normals and row publication remain CPU work. Fixed
serial execution follows the reviewed subprocess loop; the coordinator reports
no overlapping owned jobs. The machine is shared, unreserved and clock-uncontrolled.
Five descriptive samples do not establish statistical confidence, full CAD
speedup, GPU topology acceleration or faster Booleans.
