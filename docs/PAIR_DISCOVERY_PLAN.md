# Pair discovery implementation plan

2026-10-14. The corrected design passed independent runtime, topology and
oracle review. Implement a complete narrow broad-phase beachhead, then move
to another major theme. No dependency adoption or existing numeric optimization.

## Direct red and ownership

The oracle lane owns `python/brepgpu/pair_oracles.py` and `tests/test_pairs.py`.
It writes independent Fraction control/local/world enclosure and actual-box
bit checks, then focused API tests against the reviewed interface. Root runs
the missing-operation red on the fleet before product files exist. The native
lane owns all ABI4 runtime/plugin/wrapper lifecycle and callback-fault tests as
one conceptual unit. The topology lane owns the immutable body-scene adapter
and body-aware page consumer. Root owns integration, caller, measured campaign,
graphics and publication. No concurrent owners edit the same product invariant.

## Native service and wrapper

Implement `include/brepgpu_pairs.h`, `native/pair_backend_api.h` and shared
`native/pair_math.hpp`; extend existing runtime Kind/Plugin/resource cleanup/
lazy extension and synchronous exports without changing old layouts/math.
Two new resource kinds retain context and index references; use existing
Admission and publish_owned. Same-context second-index admission happens
under the first context lock. Add CUDA index/page tokens and six v4 callbacks
through existing guarded/Pending/DeviceScope/finish machinery. CPU workers own
word intervals; CUDA words use bounded grid-stride work. Exact finite-bit
ordering admits signed zeros/subnormals independently of FP mode. No empty
launch/division, atomics or candidate truncation. Actual resident boxes have
their own callback download. ABI info40/table72/options24/index24/page80 are
checked through static assertions and Python ABI admission.

Extend Engine lazily with create_pair_index/discover_pairs, PairIndex and
PairPage; bounded six-double-record admission; actual owned downloads. Keep
old core operations usable and report missing pair extension unsupported.
The native lane extends the existing test-only callback model and scenarios
for v4 refusal/base/size/transient/fatal diagnostics, index/page publication
OOM+fatal cleanup, upload/restore/download failure, retained parents and
admission-close. Test alias and exact-capacity rejection directly. Pair fatal
must poison prior CUDA families and prior-family fatal must poison pairs;
CPU remains usable. Compile/run only fleet, never on the workstation.

## Scene adapter

Implement `python/brepgpu/pair_scene.py` with factory-only frozen PairScene,
source records and `prepare_scene(cpu_engine,bodies,face_ids=None)`. Bound
body/key/chart input independently and reject duplicate/unknown full keys.
Build existing CPU depth0 roots and copy/download/verify identity. Exact
Fraction world transform plus outward rounding encloses actual roots. Retain
local boxes, world boxes, exact original snapshots and chart source order.
Require CPU preparation, existing supported builder environment and current
Frame family. Empty selections avoid arithmetic.

`PairDiscovery(engine,left_scene,right_scene)` uploads two indexes once;
page() returns frozen host ScenePairPage after download and successful guarded
child cleanup. Store `info` as owned immutable metadata with the native named
fields, `words` as tuple u32, `left`/`right` as exact scene objects, `timings`
as tuple named milliseconds. Preserve primary BaseException during cleanup.
survivors() lazily maps each set examined bit to retained source identities;
never scans tail padding or labels boxes as actual contacts. Expose actual
index download for qualification; upload/setup costs stay separately visible.

## Direct qualification

Use Fraction truth rather than CPU parity or production rounding. Cases:
finite extrema, subnormal/zero/reversed/nonfinite boxes; face/edge/corner touches;
empty and asymmetric dimensions; starts0/1/31/32/33/T-1/T; budgets1/31/32/33/cap;
invalid0/cap+1/start>T; full vs suffix completion; exact padding and every bit.
One 65,536x65,536 last-pair/terminal query verifies total2^32 without all-pairs
allocation. Qualified original controls/local roots/actual downloaded world
boxes bind the geometric claim. Different-frame coincidence and equal-local/
world-separated scenes catch wrong-frame tests. Selected repeated local face
IDs catch namespace loss; analytic tangent and known false-positive fixtures
retain carrier identity and narrow scope. Close public indexes before result
download; foreign/wrong/stale handles and Python construction/cleanup failures
must preserve ownership. Check source snapshots and scene objects exactly.

After implementation, run CPU1/CPU24 direct tests, both P100 suites under
Compute Sanitizer, updated native fault suite and host ASan/UBSan. Because
registry/plugin source changes, run existing geometry suites on new libraries
and check actual prior core/plugin compatibility with lazy unsupported behavior.
Review runtime code, scene consumer and oracle independently, integrate every
valid finding. Run a small fleet-only production mutant campaign for closed
touching, page offset, tail bits and world frame conversion; baseline green
first, isolated mutant bytes/logs in ignored context. No tests-for-tests.

## Actual caller and measured value

Root adds `examples/discover_pairs.py`, a deterministic mixed body scene recipe
and `benchmarks/pairs/run.py`. Scene recipe: grid of box/cylinder/plate bodies,
power-of-two scales, admitted origins; a translated second scene. Compare
4 and128 bodies per side (roughly tens vs~940 charts, all <=2^20 Cartesian
pairs) on CPU1/CPU24/P1000/P1001. Each case is a fresh process, initial exact
qualification outside reused timing, one excluded warmup and five repeated
calls on retained scenes/context/indexes. Measure body and CPU root/world
preparation, engine/index setup, native page, actual download, cleanup,
survivor decoding and external actual caller wall. Raw iterations retained;
derive medians/ranges without adding independent medians as wall latency.
Optional event profile comes from a separate qualification call; do not enable
event overhead in the main wall matrix. Record bytes48*(nL+nR) upload and
4*ceil(count/32) mask download; topology upload is zero. Host snapshots and
Python allocation are additional. These are host/device transfers, not network.

Caller walks complete pages and qualifies every returned page/source/bound,
exports source/local/world bounds, packed masks, survivor shortlist and numeric
timings; draws an owned SVG from actual candidate bits. No imported-body, SSI,
collision/contact or Boolean claim. Benchmark skips export but times decoding;
caller reports export separately. Keep private process logs in context.

Archive actual qualified body-era core/plugin/adapter without overwriting old
archives. Build new `build/pair-release` and `build/pair-sanitizers` owned paths
using installed CUDA12.8.1 sm60 settings; protected discovery-release unchanged.
Root serializes rsync/build/qualification/mutants/callers/performance; no owned
other workload overlaps published trials. Never use rsync --delete or touch
original forensic images/SDK archives. Local work is static review/AST and
artifact analysis/rendering only. Publish source/binary identities and shared
host limitations without private paths/hostnames/logs.

## Land promptly

Commit/push substantive design, direct red, implementation, qualification and
results milestones on feat/pair-discovery. Open PR with issue17, rewrite its
description around final delivered behavior, include actual correctness and
measured costs/limitations. Update README graphic, API guide, roadmap, topology,
research index and issue states. Merge qualified delivery with commits preserved,
retain branch and align target main. Further pruning/tuning belongs to a later
return pass; next major theme is intersection initialization/representation.
