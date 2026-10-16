# Original source to cached adaptive triangles

The native `brepgpu-adaptive-patch` caller now constructs complete untrimmed
patch meshes on CPU and CPU+P100. It loads original SourceStore values, compiles
only requested surface roots, builds/refines reusable caches, resolves shared
hanging side schedules, emits conforming packets and bounds the actual affine
facets against original source. Python supplies external source fixtures and
independent readers; production geometry and scheduling are C++/CUDA.

This delivers the first regular-surface checkpoint of the
[whole mesher plan](../../docs/ADAPTIVE_MESHER_PLAN.md). Shared physical curves,
chosen trim domains, connected boundary regions, complete imported components,
source/assembly publication and the whole watch remain required later steps.
An untrimmed carrier rectangle is not a complete imported face or solid.

## One deeper complete-operation observation

The independent coupled quadratic `S=(u,v,3u²/4+v²/4+uv)` at 1e-5 mm produces
66,049 actual points and 131,072 triangles. The whole-facet Fraction oracle passes,
and CPU1/CPU4/P1000 cold/warm outputs are byte-identical.

| Backend | Cold query | Warm fresh mesh | Two-query process |
|---|---:|---:|---:|
| Owned CPU1 | 8.423s | 4.284s | 12.833s |
| Owned CPU4 | 5.591s | 3.959s | 9.681s |
| P1000, CPU4 host | 4.777s | 3.853s | 9.116s |

These are individual observations on a shared machine. CPU4 is the best observed
CPU baseline here; the small complete-process difference does not establish a
general GPU advantage. Query includes adaptive work, packet construction and
actual-facet bounds. Process includes both fresh constructions, source/setup,
actual files, close and exit; the external checker is separate. A warm query
reselects and constructs a new mesh rather than returning a saved mesh.

The cold P100 operation performs 66,049 original point evaluations, 131,071 node
bounds and 131,070 control restrictions. Host↔GPU transfer is 101,266,544 uploaded
and 19,443,720 downloaded bytes; device allocation peak is 256,641,928 bytes.
Aggregate run reservation peaks are 376,454,838 bytes for CPU4 and 699,488,538 for
P1000, including device reservations. These are owned reservations, not RSS or
CUDA context memory. Both cold/warm fallback/facet-failure counters are zero.

The [full current observation](../../papers/2026-10-16-adaptive-general2d-transfer-observation.md)
retains actual identities, counters and stage costs. The
[earlier fixed cohort](../../papers/2026-10-16-adaptive-general2d-observation.md)
retains all four 120s fine censors from the previous revision. A reviewed
quarter-twist/actual-vertex transfer strengthening and native adapter closure
precede the current comparison; unavailable old-process profiling prevents
isolating every cause of that completion cliff. Neither observation overwrites
the other. Tiny patches favor CPU cold process costs because device setup
dominates. NVIDIA USD/BRep is **not run on this patch scenario**; the required
reference backfill remains tracked separately and no external speedup is claimed.

## Direct checks and limits

The final registered adaptive CTest selection passes all 10 tests. The numerical
reader independently checks original analytic/rational formulas on CPU and both
P100s (216 observations plus seven analytic families). The packet Fraction
reader checks 10 cases / 174 triangles, including arbitrary hanging depths,
non-dyadic construction coordinates, holes, pinch demotion and overflow ties.
The caller checks exact coverage/incidence, source errors, actual XYZ
noncollapse, source-unit metadata invariance, history and fresh-coarse replay,
useful device work, capacity refusal and no-replace publication. Polynomial,
C0 and coupled-quadratic cases use whole-facet models; weighted-rational caller
interiors are labelled falsification samples, alongside independent numerical
bound vectors. Backend parity is not the sole geometry oracle.

Scoped host ASan/UBSan/leak checks, CPU concurrency TSan, selected clang-tidy
and actual-source peer reviews pass. Device protocol tests check every one of 513
scan prefixes, finite admission/ticket/overlap failures and outstanding drains
with complete lease return on both P100s. CUDA memcheck/leak checks report zero
errors/leaks; racecheck reports zero hazards. This is finite development
qualification, not alpha or complete external-suite qualification.

The first profile admits finite regular analytic rectangles and positive
rational surfaces through degree 3 in each axis. Local immersion is checked;
global embedding is not implied. Source knot/parameter identity and the actual
immutable source lease survive. A shared bounded RN construction replaces an
unnecessary correctly-nearest transcendental requirement. Rare unresolved
geometry uses explicit original-source host fallback. CPU-only builds do not
load CUDA. P100 executes real source evaluation, rational restriction, bounds
and canonical frontier work; exact identities/topology coordination stay native.

Read [the architecture](../../docs/ADAPTIVE_MESHER_DESIGN.md),
[unit/caller plan](../../docs/native-adaptive-caller-unit.md) and
[deferred implementation notes](../../docs/ADAPTIVE_MESHER_RETURN_PASS.md).
Further tuning follows complete shared-curve/trim/component construction.
