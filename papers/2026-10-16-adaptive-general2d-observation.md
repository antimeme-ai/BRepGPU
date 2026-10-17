# General2D cached-adaptive patch: one bounded observation

At positional allowance **1e-4 mm**, all four backends completed an actual
**8,385-point / 16,384-triangle** coupled quadratic patch. The independent
whole-facet source/coverage reader passed; all eight cold/warm actual artifacts
were byte-identical. At **1e-5 mm**, each two-query process reached its fixed
120-second censor. No completed fine-query result or accuracy pass is inferred.

This is one untrimmed patch observation of the identified development binaries,
preceding the tighter `/4` bilinear-transfer implementation. It is not a complete
B-rep, trimmed face, watch render or measurement of the later binary. The fixed
eight-process cohort was not repeated after that implementation changed.
A separately authorized [current affected comparison](2026-10-16-adaptive-general2d-transfer-observation.md)
subsequently completed the SAME fine source on CPU1/CPU4/P1000; it is reported
separately with its own binary identities.

| Allowance mm | Backend | Process, two queries s | Cold query s | Warm fresh query s |
|---|---|---:|---:|---:|
| 1e-4 | CPU1 | 1.469 | 0.988 | 0.461 |
| 1e-4 | CPU4 | 1.037 | 0.606 | 0.410 |
| 1e-4 | P1000, CPU4 host | 1.234 | 0.484 | 0.408 |
| 1e-4 | P1001, CPU4 host | 1.457 | 0.513 | 0.424 |
| 1e-5 | CPU1 | censored at 120.152 | unavailable | unavailable |
| 1e-5 | CPU4 | censored at 120.073 | unavailable | unavailable |
| 1e-5 | P1000, CPU4 host | censored at 120.250 | unavailable | unavailable |
| 1e-5 | P1001, CPU4 host | censored at 120.234 | unavailable | unavailable |

Process time includes native source decode/compile, session/device creation,
both fresh constructions, actual artifact writing, close and process exit.
It excludes the external Fraction reader. Query time includes adaptive selection,
packet construction and bounds on actual emitted facets; a warm query constructs
a fresh mesh from retained caches. CPU4 had the lowest complete process time;
P1000 had the lowest measured cold-query time in this single observation. The
shared development machine was not fenced against other activity; these are
individual observations, not statistical speedup estimates.

GPU open took 0.260/0.262 s and close 0.050/0.236 s. Each artifact write took
approximately 0.005–0.006 s. Cold GPU work included 8,385 original point
evaluations, 16,383 node bounds and 16,382 control restrictions. Cold exact
fallbacks and actual-facet failures were zero on every completed backend.

| Completed coarse mode | Session host reserved peak upper, bytes | Device allocation peak, bytes | Cold upload/download, bytes | Warm upload/download, bytes |
|---|---:|---:|---:|---:|
| CPU1 | 42,795,836 | 0 | 0 / 0 | 0 / 0 |
| CPU4 | 43,057,980 | 0 | 0 / 0 | 0 / 0 |
| P1000 | 59,410,832 | 32,080,136 | 12,678,768 / 2,441,736 | 11,009,720 / 1,703,856 |
| P1001 | 59,410,832 | 32,080,136 | 12,678,768 / 2,441,736 | 11,009,720 / 1,703,856 |

The earlier caller's host peak is a **sum of scoped reservation peaks**, an upper
bound rather than an observed aggregate peak or process RSS. Aggregate run peak
was not emitted by this binary and is blank in the data. Transfers are host↔GPU
bytes, not server/network traffic. The full
[observation TSV](../benchmarks/native_mesher/results/2026-10-16-adaptive-general2d.tsv)
retains decode/compile/setup/query/output/close scopes, counters and censors.

## Source and independent check

The explicitly untrimmed parametric asset has no inferred faces or edges. Its
original positive-weight degree-(2,2) Bernstein net uses `x=i/2`, `y=j/2`,
`z=0.75*(i==2)+0.25*(j==2)+i*j/4`, `w=1`. Every control is exactly binary64.
Thus `S(u,v)=(u,v,3u²/4+v²/4+uv)` on the unit rectangle. It curves in both
parameter directions and has a mixed term; its Hessian is indefinite. The helper
name `make_bowl` does not mean the surface is convex.

For every barycentric point of an emitted triangle, the source-minus-nodal
interpolant has only a z component, `−3 Var(U)/4−Var(V)/4−Cov(U,V)`. Its norm is
at most `3 range(U)²/16 + range(V)²/16 + range(U)range(V)/4`. The SAME independent
reader adds maximum actual nodal displacement using exact Fractions, and checks
positive chart cells, physical noncollapse, exact complete rectangle coverage,
incidence and all referenced points. Geometry peer independently reviewed this
formula. One completed artifact per quality gets the whole-facet check; every
remaining actual artifact is compared byte-for-byte. No fine artifact qualified
because no fine process completed. This is not a sampled-only accuracy claim.

The source helper/reader is `tests/native_adaptive_caller.py`; the external runner
is `benchmarks/native_mesher/adaptive_patches.py`. Limits were 2,048 MiB,
262,144 nodes, depth64 and 120 s per native process; qualities were only 1e-4
then1e-5, two same-quality fresh queries per retained session. No larger workload,
timeout extension or product geometry change was introduced.

The old six-coefficient-only facet upper can trigger precise fallbacks before
finer subdivision; the later selected covariance `/4` minimum addresses that
source-level mechanism. This observation does **not** establish it as the cause
of the fine censors: the attempted one-second attachment to the existing CPU4
process was denied by `perf_event_paranoid=4`, before any sample was collected.
No privileged change or replacement profiling workload followed.

## Reproduction identity

Machine: dual Intel Xeon E5-2697 v2, 12 cores/socket, two threads/core; two Tesla
P100-PCIE-16GB devices, driver580.178.04. Each mode was run sequentially; GPU
modes retained four CPU workers. The numeric profile was
`rn64_anchor32_p128_v1`.

| Artifact | SHA-256 |
|---|---|
| Native caller | `4baa8fb2e94e02616d2ad221d16f56ed2cba6c6aa105a4022f0fa779bbb39ab4` |
| CUDA module | `4dc828ac7f701c84c10ce3fec024cb534e456ef87aeb38d300f8f2330d9f11d1` |
| Authored source file | `162bfc54c5019ab4356b0320a3a273fbd756b132eaab2039f17b7b9c4b4b44ce` |
| Every completed actual mesh | `0c794425932a157a8e29ca0a36ebc3101980343710d776c03e894946d45b121b` |

The initial runner labelled a missing newer aggregate-memory metadata column as
an oracle failure after all source/coverage/actual-byte assertions had already
passed. Only that exact missing-column status was repaired by metadata-only
extraction from the retained original tables; no geometry or native operation
was rerun. The runner now distinguishes metadata failures and accepts either
host-peak column spelling while leaving unavailable aggregate values blank.
Raw logs, commands, artifacts, original status table and extraction log remain
private in the fleet `context` directory; the public TSV is derived data.
