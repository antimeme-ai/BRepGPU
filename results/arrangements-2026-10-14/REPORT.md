# Exact planar arrangements: regions, holes and shared uses

2026-10-14. Complete exact CPU construction on the owned rectangular-face
family: axial cuts plus strictly separated circle disks inside individual cells.
Every cell-minus-disks and disk region is emitted, with shared opposite uses,
ordered outer/hole loops, retained source geometry and explicit exterior.
This advances the breadth-first face-arrangement theme. It does not assemble a
Boolean solid, implement general curve overlay or place construction on GPU.

![Actual region loops, shared edges and retained circle geometry](caller/arrangements.svg)

The drawing reads actual region loop words and edge UV readers. Gold circles
are shared boundaries between cell holes and separate disk regions; the disks
remain regions rather than removed material. A physical circle is one closed
edge/anchor retaining four original pieces and all spatial/bothUV maps/events.
Display sampling has no mesh approximation guarantee.

## Complete bounded semantics and qualification

The [API](../../docs/ARRANGEMENTS.md), [design](../../docs/ARRANGEMENT_DESIGN.md)
and [whole-unit plan](../../docs/ARRANGEMENT_PLAN.md) declare exact support.
Cuts are submitted rational/binary64 values, canonicalized without rounding.
Original boundary segments carry directed source coedge/edge t0/t1 parameters;
circle sections retain the exact original source snapshots/face/chart identities.
Semantic source keys establish shared vertices. Every edge, including target
boundary, has two opposed uses. Exterior0 owns the clockwise target boundary.
Positive-UV cell/disk outers are CCW; cell holes are CW independently of the
original body's face orientation.
The cell region removes open disk interiors and retains hole boundaries;
closed disk regions share those exact boundaries.

`locate_uv` returns all incident closed regions at grid crossings, cut endpoints,
circle boundaries and original boundaries. Exact rational witnesses and
rational-plus-pi UV areas accompany every bounded region. Circle disks must be
strictly disjoint and inside one open cell; clipped/tangent/nested/coincident/
overlapping circles, source hole caps and incompatible sections reject. Equal
target U/V spans establish actual UV circles. Unsupported input never becomes
a partially successful arrangement. Edited-body naming and cross-face Boolean
edge unification remain separate work.

Independent truth derives original cell-minus-disks/disk domains and matches a
complete region bijection, source edge subranges, oriented outer/hole boundary
words, source-map correspondence, twins/next/previous/ownership, strict witnesses,
exact area partition and closed-region lookup. Existing independent original-
control SSI truth qualifies retained complete curve maps; actual new edge readers
are separately checked against those source maps. Euler counts alone are not the
oracle: disconnected circle components change the correct Euler sum.

[Qualification](qualification.tsv) records the direct healthy-section/classifier
then absent-entry red, first arrangement green, and **72 new/compatibility tests
in12.881s**, with one existing GPU-only skip. Eleven entry/arrangement tests cover
all owned rectangle directions, complete/multiple holes, source operand reversal,
bottom/frame/selection permutations, exact subnormal/one-ULP admission, bounded
iterators, unsupported geometry and immutable failure/reader behavior.
No native implementation changed; unrelated native sanitizer campaigns were
not repeated. [Five protected native library hashes](protected-binaries.tsv)
are unchanged.

[Eight focused production mutants](mutations.tsv), with [direct targets](mutation-targets.tsv),
were assertion-killed: omitted hole, wrong twin, self next, reversed original
boundary parameters, wrong circle UV source side, missing crossing cells,
admitted tangent disks and lost use orientation. All healthy baselines and
retries passed. The controller requires exit1, an actual AssertionError and a
completed failures-only unittest summary; import/runtime/crash errors receive
no credit. These are named fault classes, not an exhaustive mutation score.

## Actual callers and complete cost

The [caller](../../examples/arrange_face.py) executes one exact CPU process on
Xeon E5-2697v2/x86_64 Linux with Python3.12.3. It retains prepared snapshots,
sections and cuts, then creates fresh owned arrangements. No native/GPU operation
or CPU24 comparison is part of this construction API. No owned copy/build/test/
mutant/performance worker overlapped the serial measured operations.

Five actual recipes, each with one first call, one excluded warmup and five
repeats: **35 independently qualified arrangements and25 timed repeats**.

| Recipe | Cells | Circles | Vertices | Edges | Opposed uses | Bounded regions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Bare rectangle | 1 | 0 | 4 | 4 | 8 | 1 |
| One-circle cap | 1 | 1 | 5 | 5 | 10 | 2 |
| Mixed cells, including two holes in one cell | 4 | 3 | 12 | 15 | 30 | 7 |
| 64 holes in one cell | 1 | 64 | 68 | 68 | 136 | 65 |
| 32×32 grid plus64circles | 1,024 | 64 | 1,153 | 2,176 | 4,352 | 1,088 |

Each includes exterior0 in addition to the bounded region count. These are
actual distinct grid/disk domains; each of the64 circles has its own retained
source section. The five final full graph exports contain1,168 total regions,
including five exteriors, and4,536 directed uses. Large fixtures are analytic
structured inputs, not general imported CAD assemblies.

Complete reused milliseconds, median [min,max] of five repeats:

| Recipe | Construction | Construction plus all readers | Outside independent checking |
| --- | ---: | ---: | ---: |
| Bare rectangle | 1.096 | 3.164 [2.071,4.013] | 3.340 |
| One-circle cap | 6.336 | 34.114 [28.827,36.409] | 137.067 |
| Mixed cells | 16.364 | 87.008 [85.133,174.688] | 445.403 |
| 64 holes | 397.822 | 2,091.441 [2,066.417,2,288.753] | 9,786.943 |
| 32×32 plus64circles | 471.046 | 2,400.587 [2,327.950,2,605.276] | 10,570.279 |

Construction includes complete input/source-map validation, pairwise disk
admission, topology/geometry creation and publication. Reader wall follows every
actual next/twin and reads all region/source metadata; evaluates line/circle
world/UV values and retained opposite-surface maps; and locates all region
witnesses, vertices and three extra boundary/outside queries. Original boundary
curves/coedge maps are evaluated at emitted directed parameters and compared
to actual arrangement reader values **inside geometry-reader time**. The large
case has6,912 edge parameter queries and2,245 region-location queries; source
piece map reads occur in addition to the edge query count.

The large complete cost is dominated by exact curve consumption: geometry
reader median1,866.881ms, graph9.069ms and location51.761ms. The64-hole case
spends1,595.346ms in geometry,0.534ms in graph and99.641ms in location. Separate
phase medians need not sum to the complete median. These measurements expose
later placement opportunities; no comparative speedup or causal diagnosis is
assigned to the ranges.

Source preparation is outside reused wall. Complete preparation costs0.751/
18.983/39.893/890.424/922.280ms for the five recipes. Exact section construction
alone is870.681ms for64holes and898.272ms for the large grid; body/frame work
and cut construction make up the remaining preparation. First complete calls
after imports/preparation are2.814/34.799/96.273/2,010.991/2,359.240ms. These
are first calls in the already imported process, not fresh-process startup.
Imports, final shutdown, TSV export and SVG rendering are outside complete wall.
No host/device or network data transfer is timed by this exact CPU API.

Independent whole-domain/source/read/query-binding qualification runs after
every complete operation and is explicitly expensive:9.79/10.57s large-case
repeat medians, outside the2.09/2.40s caller wall. Initial check times are in
raw.tsv too. Source section preparation and checking costs must remain visible
when comparing a future complete modeling workflow.

## Reproducible saved outputs and next scope

[Raw observations](caller/raw.tsv), [phase statistics](caller/summary.tsv),
[preparation](caller/preparation.tsv), [counts](caller/counts.tsv),
[environment](caller/environment.tsv) and [eleven unchanged source pins](caller/sources.tsv)
retain all trials. Full final graphs, regions/areas/witnesses, point queries,
target/source controls/charts/maps/events/components and original boundary
records are exported for **all five** cases:
[bare](caller/bare/regions.tsv), [single](caller/single/regions.tsv),
[mixed](caller/mixed/regions.tsv), [64holes](caller/many-holes/regions.tsv),
[large grid](caller/grid32/regions.tsv). Neighboring TSV files in each directory
contain the complete corresponding topology and source records, not aggregate
tables standing in for missing final geometry. All35 trial outputs were qualified
in memory; exports retain the final output of each recipe.

[Independent artifact review](../../papers/2026-10-14-arrangement-artifact-review.md)
checks every saved graph:1,242vertices,2,268edges,4,536uses,1,300loops,
1,168regions and2,425locations; all13,200source coefficient rows match the
original circle/target/wall maps. [SVG/result review](../../papers/2026-10-14-arrangement-result-semantics-review.md)
reproduces all2,268edge paths and1,163region paths from the saved geometry.
[Performance review](../../papers/2026-10-14-arrangement-performance-runtime-review.md)
recomputes every summary and accepts the final report's costs and scope.

[Source studies](../../papers/README.md#exact-planar-arrangements) and retained
reviews document complete reading and scope. Continue trim-aware meshing, narrow
interchange and other uncovered solid/feature themes. Generalized arrangements,
native/GPU bulk readers and the [Python shipping-role investigation](../../papers/2026-10-14-python-role-return-pass.md)
remain after the breadth sweep; [round-two notes](../../papers/2026-10-14-arrangement-round2-notes.md)
record those opportunities without starting integration tuning.
