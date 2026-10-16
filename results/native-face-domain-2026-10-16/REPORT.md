# Native exact sampled-face construction

The actual 8,300-node OM10 spring face now produces all8,298 source-keyed
triangles in **18.754s median** on one CPU core, including exact input
normalization, native ownership, construction, output and teardown. Independent
exact domain checking passes separately in0.510–0.514s. The unchanged Python
construction was censored at300s without a completed construction observed.
This is one sampled-face domain operation, not complete part meshing, watch
rendering, source import or GPU execution. No numeric speedup is inferred from
the censored run.

The selected original NVIDIA and Manifold domain cases pass actual-output
oracles.54 fresh scaling trials also pass and expose both the improved boundary
search and remaining quadratic ear work. These are bounded compatibility and
performance observations; complete external alpha qualification remains open.

## Scope and setup

The retained actual input is original face167661 in definition1756: one outer
loop,8,300 exact sampled UVs, no holes. It was reproduced from unchanged source
and schedules before boundary triangulation. Source-closure preparation was
checked separately; its51.891s reproduction cost is outside this fixed-input
construction experiment. Original node identity and UV values survive exactly.
The face belongs to a spring definition the current NVIDIA whole-watch path
fails; it provides no passing NVIDIA timing denominator here.

Build: owned C++20, Clang18.1.3/libstdc++13, release `-O3 -DNDEBUG`, strict
warnings, no fast-math, FP contraction disabled. Both work counters and resource
checks remain enabled in the product configuration. Host: Xeon E5-2697v2;
CPU-only, CUDA hidden. Final native/scaling trials pinned one physical core on
socket1 (logical CPU9); the legacy cutoff used another physical core on that
socket (logical CPU1). Other compute was running. These are contended
observations, not a quiet matched speed-ratio experiment.

The final binary hash is
`8c99eb3ad8fbfba82485c4211878d596576c21f6f83d7b7ae9386eb549dd62a2`.
[Artifact hashes](ARTIFACTS.tsv) name exact source/binding/runner/snapshot bytes.
The three earlier17.9–18.2s trials predate final counter/layout refinements and
are excluded from the public trial denominator. [All58 final rows](TRIALS.tsv)
contain3 actual native passes,1 legacy cutoff and54 authored scaling trials.
Raw CAD/UV snapshots and operational logs remain private.

The legacy worker loaded the original mesher module before the isolated checkout
received production dispatch integration. Its original module hash is retained
in ARTIFACTS.tsv (Git base `86e5e49`). Static AST comparison confirms every legacy
helper and class, including crossing/bridging/triangulation and their predicates,
is identical in the integrated module; only the separate consumer dispatch and
circle-schedule routing changed. Neither those new callers nor native geometry
participated in the explicit legacy construction attempt. The exact-domain
oracle source is pinned separately as well.

## Actual face: all three fresh trials

| Trial | Complete construction | Independent domain check | Process peak RSS |
| --- | ---: | ---: | ---: |
| 1 | 18.801747s | 0.512629s | 42.215MiB |
| 2 | 18.672921s | 0.510058s | 42.281MiB |
| 3 | 18.753637s | 0.513860s | 42.223MiB |
| Legacy, one attempt | CONSTRUCTING_TIMEOUT at300s | Not reached | Not observed at cutoff |

All native trials emit8,298 positive triangles whose complete oriented boundary
chain matches the original sampled domain. They retain original source node IDs.
Native tracked allocation high-water is4,008,828bytes (3.823MiB), distinct from
process RSS. Encoded input is597,792bytes; maximum observed arithmetic magnitude
is7 base-2^32 limbs. The practical target was under300s and2GiB; all three pass.
This domain oracle does not substitute for complete source-surface, physical
shell or chord/rounding qualification at whole-mesher integration.

For the median-wall trial: Python exact normalization/packing0.170856s;
native decode/rank/initial index0.009933s; boundary search0.001667s; no-hole
bridge handling0.000005s; ears18.549995s. Remaining complete-call time includes
other native validation/index work, loading/layout checks, export and teardown.
These named stages are parts of the complete construction wall; the separately
timed independent check is excluded.

The boundary search executes193,852 BVH-node visits and84,844 leaf-object box
tests, before overlap filtering. Zero nonadjacent candidates survive to exact
segment predicates. This is direct search work; a low post-filter predicate
count alone would not demonstrate removal of exhaustive enumeration.

Ear construction still visits34,426,334 candidate occurrences, including
25,796,416 dead occurrences, and performs25,823,381 exact orientations across
validation/construction. This remaining work is substantial. Active-subtree
pruning and arithmetic-allocation specialization are recorded for later units;
the current result does not establish which would dominate wall improvement.

## Scaling: valid outputs, including the unfavorable curves

Each cell is the median of three fresh processes; all54 trials pass the actual
independent positive/full-boundary-chain oracle. Each process includes complete
normalization-to-triangles construction. See TRIALS.tsv for every wall, separate
check time, process/native memory and exact work counter.

| Boundary nodes | Long thin strip | Strict convex ring | Strip crossing node+box work |
| ---: | ---: | ---: | ---: |
| 256 | 0.019474s | 0.007778s | 5,496 |
| 512 | 0.059808s | 0.012588s | 12,024 |
| 1,024 | 0.218131s | 0.023467s | 26,104 |
| 2,048 | 0.839908s | 0.042976s | 56,312 |
| 4,096 | 3.298043s | 0.088156s | 120,824 |
| 8,192 | 13.143241s | 0.186396s | 258,040 |

The strip retains every collinear boundary sample, with exact dyadic dimensions
2^20 by2^-10. Its boundary search stays below the declared64*N node-plus-box
budget, but ear candidates grow from14,749 to14,693,373. Total construction
therefore remains approximately quadratic on this family. The strict-convex
family uses the proved no-hole/all-turns-positive fast path, emits the same
cursor-order ears, builds no point index and performs zero blocker queries.

| Family | Actual nodes | Holes | Median construction | Crossing node+box work |
| --- | ---: | ---: | ---: | ---: |
| Dense concave | 256 | 0 | 0.016678s | 10,888 |
| Dense concave | 1,024 | 0 | 0.192649s | 126,776 |
| Dense concave | 4,096 | 0 | 2.773204s | 1,789,816 |
| Separated holes | 40 | 9 | 0.004507s | 1,125 |
| Separated holes | 200 | 49 | 0.032918s | 10,703 |
| Separated holes | 488 | 121 | 0.170945s | 28,706 |

Dense-overlap boundary boxes remain unfavorable: a median BVH does not promise
subquadratic queries. Hole-family requested sizes are targets; the table reports
actual nodes and complete separated-hole grids. No duplicate/collinear source
samples are removed and no tolerance is widened to obtain these results.

## Correctness, failure and tooling checks

- 11 focused authored tests include252 signed add/subtract/multiply vectors from
  independent Python Fraction results, wide carry/borrow/cancellation, exact
  non-dyadic coordinates, crossing order, holes, original source IDs, collinear
  samples, malformed snapshots, bounded resources and context reuse.
- 3 external test methods exercise5 original domains plus declared metamorphic
  derivatives from the selected Manifold cases and complete NVIDIA planar
  L/triangle/circle recipe. Their independent readers check actual exact domain,
  directed source incidence and embedding; upstream counts/status are not the
  owned geometry oracle. See the [case study](../../papers/2026-10-16-native-meshing-reference-test-plan.md).
- Release native CTest passes. ASan/UBSan native ownership/hole-reuse/failure
  checks pass in0.03s;252 arithmetic vectors also pass through the sanitized
  native executable. Python does not load an ASan DSO.
- Strict compilation, formatting, clang-tidy and Static Analyzer pass over actual
  compiled module/test units and every reached owned header. Independent
  numeric/algorithm/resource code review accepts the fixed implementation.

No mutation campaign, universal performance gate or alpha certificate was added.
The next whole-mesher claim needs its own complete-part source/physical/quality
outcome; faster initial triangles alone cannot establish that result.

## Complete main-plate consumer: censored

The next actual integration attempt selected the complete original main plate,
definition1713, all438 faces. Original source records and the complete part graph
were retained unchanged from the independently qualified whole-source snapshot.
The selected source/library were pinned; the old full-watch process was untouched.
The prepared-source-to-complete-mesh call includes original edge sampling,
circle scheduling, initial native domains, Python connectivity and refinement.

This attempt **did not finish within its300s construction budget**. The last
durable marker reported200/438 faces; that marker is not a complete output count
at cutoff. Independent complete-mesh qualification never started. There is no
complete-part result or whole-operation speedup. No extra Python whole-part
baseline or fresh whole-watch retry was launched after this censored outcome.

The [complete-part outcome](MAIN_PLATE.tsv) retains its source/library/quality
identity and non-green denominator. Separate provenance/decode/closure walls
were0.495/6.922/0.235s; total
supervised elapsed time was308.108s, including termination and process overhead.
A single3s nonblocking profiler interval during construction returned210 valid
samples and89 read errors.167 valid stacks included Python
`_improve_connectivity`, the exact metric Lawson-flip pass. This is evidence for
the next migration boundary, not a whole-operation cost percentage. Private
locals read during that interval are not reliable counters.

The first native domain unit passed its practical target; the actual larger
consumer exposed a different remaining Python algorithm. The next bounded
study [designs fixed-vertex native connectivity](../../papers/2026-10-16-native-lawson-connectivity-option.md),
captures its fixed original input and evaluates owned native
triangle/edge incidence, exact weighted incircle arithmetic and queue ownership,
keeping the same constraints, strict flip rule and quality. It does not tune ears
further or widen tolerances to make this complete-part attempt green.
