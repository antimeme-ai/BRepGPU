# Native sampled-face engine: external cases and performance experiments

Research/sub-plan proposal for the operator-authorized C++ transition and mesher
surgery. No mesher, test, benchmark, reference build or library installation was
run in this study. Read the pinned source and fixture bytes. This does not change
[the alpha gate](../docs/ALPHA_QUALIFICATION.md), select a new dependency, or claim
that the selected first cases represent a complete community suite.

The immediate useful combination is **NVIDIA's planar trimming recipe,
Manifold's committed polygon corpus, and the actual sampled watch face to reproduce/capture**.
NVIDIA supplies real geometry tests, but its polygon entry point is empty. The
community polygon tests contain stronger inputs than answers; our independent
domain reader must examine BRepGPU's actual output.

## Pinned sources and what their tests actually check

### NVIDIA USD/BRep

Revision `c9c5979f04cea8c38213004d4f04df0dcfb40d05`, already retained under
`quarantine/sources/usd-brep` and recorded in `reference-catalog.tsv`.
The inspected source files have SPDX Apache-2.0; keep their attribution/license
when retaining translated fixtures. Do not copy production implementation.

- [`TestSmTrimmedSurfaces.cpp::TestSmTrimSurfaceWith3dCurves`](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/tests/SM_API_test/src/TestSmTrimmedSurfaces.cpp)
  constructs the planar L-shaped outer ring `(0,0),(10,0),(10,5),(5,5),
  (5,10),(0,10)`, clockwise triangle hole `(1,5),(1,8),(4,5)`, and circle
  hole centered `(6,2.5)`, radius `1.5`. It requires one face, ordered loop
  counts `6,3,1`, ten boundary edges, an outer loop of six vertices/edges,
  valid orientation values and cyclic physical endpoint continuity. These are
  genuine source topology assertions, not a triangulated-domain answer.
  Use the complete original recipe first. Obtain the circle's sampled chord
  ring from a declared immutable schedule; distinguish the exact sampled
  polygon area from analytic area `75 - 9/2 - 9*pi/4`. Missing a circle or
  replacing it with a square is a different case. The polygon-only outer+
  triangle derivative is useful as a separately named small test, not an
  execution of the complete upstream recipe.
- `tests/SM_API_test/src/TestSmPolygons.cpp::TestSmPolygons` contains only
  commented-out calls and returns success. **No triangulation test exists in
  this entry point at this revision.**
- `tests/prog_test/src/smTessTest.cpp` is a real tessellation/mesh suite, selected
  as `tessellation` in `prog_test.cpp`. Its first regressions explicitly check
  per-vertex face/normal-table correspondence, preservation through vertex
  gluing, and tessellation of a full and partial cone. These are relevant to
  native lifetime/provenance and the later display-ready output, not a general
  exact domain oracle. Several later branches gate validity only under
  `SM_GFX_CODE`, draw meshes, accept status, or depend on files. Audit a chosen
  branch's assertions and data before adopting it; a whole-suite green is not
  automatically a geometric bound.
- `tests/usd_brep_utils_test/src/usd_brep_utils_test.cpp` checks compact/flat
  knots, analytic evaluations/inverse projections and NURBS examples against
  known values/round trips. Useful during later native carrier evaluation;
  unrelated to the first domain engine's crossing/ear complexity.
- `tools/brep_validator_test/test_validation_boundary.py` deliberately leaves
  NURBS endpoint agreement and repeated-edge seam geometry to the kernel while
  checking structural/range errors. The validator is not an independent
  trimmed-surface geometry oracle. The round-trip Python tests compare
  persistence, explicitly allowing known per-face tolerance loss; persistence
  equality is also not tessellation quality.

Current OM10 definitions `1756` and `1797` have non-admitted SDK polygon rings;
`1716` and `1811` fail the converter. Keep them as actual failure outcomes.
In particular the spring `1756` cannot provide a successful NVIDIA denominator
for the owned 8,300-point face. Prefer original main plate `1713` for the first
accepted same-part comparison, after source/output quality checks.

### Manifold

Revision `761661c15bacb6c8459d682c715de84591d51774`, retained
`quarantine/sources/manifold`, Apache-2.0 files. Read
[`test/polygon_test.cpp`](https://github.com/elalish/manifold/blob/761661c15bacb6c8459d682c715de84591d51774/test/polygon_test.cpp),
`polygon_corpus.h`, `polygon_fuzz.cpp`, `test_main.cpp`, and committed corpus
`test/polygons/{polygon_corpus,sponge,zebra,zebra3}.txt`.

The direct corpus oracle checks no throw and expected triangle count on the
original, rotated 180 degrees and duplicated input. It does **not** check exact
coverage or embedding. `TriangulatorReuse` checks state clearing and parallel
thread-local reuse. The fuzz target accepts its geometry exception and primarily
checks no unexpected error/timeout; it is not an answer oracle. Borrow the input
diversity and state-reuse fault class; do not mistake these checks for our contract.

First bounded external cases, selected before product results:

1. `SimpleHole`: seven expected triangles, integer four-vertex outer and
   three-vertex hole; a small exact foreign-domain test.
2. `Eberly`: ten expected triangles, seven-vertex concave outer and triangle
   hole. Its input is our existing nearest-reflex counterexample translated by
   `(0,-5)`. Preserve the upstream identity, but do not count this as a new
   independent geometric family.
3. `Concave2`: twelve distinct integer vertices, expected ten triangles; no
   floating tolerance needed to state the domain.
4. `AlignedHoles`: six loops, expected 27 triangles; retain saved binary64
   coordinates exactly and inspect domain admission before benchmarking.

Later explicit compatibility cases include `Colinear`, `Duplicate`,
`TouchingHole`, `Precision4`, `Comb`, `SquareHoles`, and the sponge/zebra sets.
Some intentionally contain repeated/near-coincident positions, nonzero epsilon,
touching holes or multiple outer components. Our occurrence identity and strict
source constraints must be stated before translation. Do not silently deduplicate,
apply Manifold's epsilon, change fill rules or require its triangle count when
it merges vertices our contract preserves. A rejected/missing adapter remains
visible. This bounded first cohort does not select the alpha-required denominator.

Preserve the upstream decimal bytes, then parse to binary64 as upstream does
and supply those **exact saved binary64 values** to Fraction/native predicates.
Parsing decimal text as exact decimal rationals would change the input. Our
general planar native seam also contains non-dyadic rationals; separate authored
`1/3,1/7` cases exercise that domain rather than pretending this corpus does.

Corpus hashes (SHA-256):

| File | SHA-256 |
| --- | --- |
| polygon_corpus.txt | `69b9709fb8608b827aedbf0c0afbe13e04364c84e1cb626385c7899a7bdac509` |
| sponge.txt | `d019f0c8582b1bbc677898d158f33644389bcb111cddaf0e4d26c12d48f8b39f` |
| zebra.txt | `b2b1593642b060d81b633e1734b821d5622764f6e4a2e8aec5e3bca875cfa5bd` |
| zebra3.txt | `9c2a4151e7740001bbfe0e243434ff9779e9e752fa8fea2b803523f3102a314d` |

### OCCT, Geogram, Triangle and Eberly

OCCT `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`, retained
`quarantine/new-iteration/occt`, LGPL-2.1 with Open CASCADE exception:

- `tests/bugs/mesh/bug26664`: tiny polygon, `incmesh 1e-4`, expects `NoError`;
  `bug26664_f.brep` is absent from this extraction. Status alone is insufficient.
- `bug27239`: expects meshing success, empty `tricheck`, valid shape and no
  duplicated polygon nodes. `bug27239_f1.brep` is absent. Strongly relevant
  historical fault, acquisition/adapter pending.
- `bug28089_1`: compares two algorithms on `data/occ/terrain.brep`, which **is
  present**, but essentially prints `trinfo` and draws results. A useful later
  input/performance case, not an expected geometric answer. Preserve exact
  surface/boundary data before selecting its first face.
- `bug27693`: excess triangles along full U/V stripes from fine edge sampling;
  relevant later refinement/density test, external STEP absent here. Upstream
  triangle/node counts are implementation regression checks, not required owned
  triangulation counts.
- `bug25061`: explicitly requests skipping a self-intersecting inner wire and
  has a TODO for disconnected mesh. That behavior conflicts with our no-dropped-
  boundaries contract. Preserve original case but expect explicit rejection at
  this domain seam; never silently erase the troublesome wire to turn green.

Geogram `390b14d5b9df949c45d07d5af208700f6be3a616`, retained
`quarantine/sources/geogram`, BSD-3-Clause top-level:
`tests/CDT_2d.robot` and `CDT_no_Delaunay_2d.robot` identify duplicated constraint,
external-removal and 100K-constraint cases. `src/tests/test_CDT_2d/main.cpp` calls
`check_consistency`, writes a mesh and returns success; the Robot wrapper checks
process success. Files under `VORPATEST_ROOT_DIR/data/CDT2d` are absent from the
current extraction. These are future stress inputs once acquired, not runnable
tests already owned. They can also introduce constraint intersections and new
vertices, which our sampled-simple-domain seam explicitly rejects.

The bundled Triangle 1.6 README specifies separate non-commercial/free-use and
commercial-distribution terms; Geogram's BSD license does not replace them.
[Triangle's author site](https://www.cs.cmu.edu/~quake/triangle.html) and retained
algorithm documentation supply useful constrained-mesh/predicate design, but no
verified acquired complete answer corpus for this unit. No Triangle dependency,
commercial distribution or reference runner is selected. Eberly's retained ear-
clipping paper supplies the bridge counterexample and data-structure reasoning;
it is literature, not a universal external conformance suite.

## Direct correctness checks for the first engine

Use the original Python exact implementation as the simpler differential model
for bounded inputs and the existing independent domain reader for geometric
truth. They attack different failures; do not add another wrapper that only
checks their summaries. Capture immutable input loops, occurrence IDs, exact
coordinates, physical/source keys and expected admission before a timed trial.

For a pruning-only replacement, require the same first crossing tuple in the
original lexicographic enumeration, same exception category and same deterministic
triangle occurrence-ID sequence as baseline. Index traversal order must not
become a new geometric tie breaker. Bridge duplicate occurrences retain separate
slots even when physical points agree. No false-negative prune is allowed.

Independently require exact positive triangle area; complete oriented original
boundary chain; paired interior edges; no missing/extra hole or disconnected
domain; nonoverlapping embedded triangles; unchanged source occurrence/physical
maps; and explicit malformed/crossed-ring rejection. Counts/area totals alone
cannot establish these. Apply the existing full mesh physical edge/vertex-link,
source and chord/rounding checks at integration rather than duplicating them
inside the benchmark. Surface-source quality remains the unchanged original
0.005/0.001 mm contract for watch integration.

Carry the existing nearest-reflex counterexample, 55-hole case, collinear sample
chain and exact rational translations through the native seam. Add literal
crossing, touching, zero determinant, cancellation, wide-exponent and non-dyadic
sign/order witnesses appropriate to the selected exact-number representation.
Do not replace Fraction truth with float parity. Sanitizers exercise ownership
and malformed binary inputs separately from optimized performance runs.

## Performance experiment: isolate work before whole-watch retries

First capture/reproduce the actual 8,300-boundary-point face `167661` from definition
`1756`, with original exact coordinate numerators/denominators and occurrence
words. The diagnosis's 34.4M pair and 68.9M ear-slot figures are derived potential
counts, not measurements. Do not repeat an hours-long whole watch to discover
whether candidate generation improved. Keep that original run separate.

Compare three implementations on identical saved inputs: unchanged Python
baseline; owned Python indexed candidate pruning if that slice lands first;
and native single-thread engine. Report native parallel batches separately
after shared schedules are stable; threads must not hide single-face complexity.

For each implementation emit direct stage wall/CPU time, peak process RSS,
input/output bytes, total segment pairs generated, AABB candidates, exact
segment predicates, ear attempts, candidate points tested, active/dead slot
visits, bridge visibility tests, exact slow-path count/operand bit-size where
applicable, and complete triangle count. Count actual executed work at the
decision point. Separate index construction/query, solve, input decode and
output write. Existing independent result checking has its own wall and is not
included in the construction denominator. Disabled counters and instrumented
builds must not be mixed in a ratio.

Run a bounded pilot on fleet hardware, then three fresh matched repetitions
where affordable; show every result and median/range, not the fastest. Pin one
physical CPU/core/NUMA socket and inner threads one, CUDA hidden, optimized
compiler/build flags recorded. Record contention; do not call concurrent runs
quiet. Optional bounded `perf stat` cycles/instructions/branches/cache misses
can explain an unresolved cost; profiling is not a permanent certification job.

Scaling inputs: dyadic comb/long thin strip with `N=256,512,1024,2048,4096,8192`,
exactly constructed separated-hole grids, and a deliberately dense-overlap AABB
family. Preserve the same geometric request at each size and report N, holes,
coordinate bit widths and output work. The sparse family should expose reduction
from quadratic generated candidates; the dense family reveals the index's
worst case. Do not promise universal subquadratic time when actual intersections
or overlapping bounds are quadratic. Repeated/touching occurrences test both
correctness and memory behavior; don't use tolerance simplification to win.

Set a per-case wall budget before the pilot (proposed 300 s for legacy diagnosis,
adjust once from actual small-case costs). A timeout is a published censored
outcome, not an inferred speedup or pass; don't keep rerunning it. New-code
candidate counts and native work should explain the gain before an integration
claim. If native code still generates roughly N-squared candidates on sparse
inputs, language migration has not solved this algorithmic defect. If a supposedly
faster implementation alters accepted domain/output/quality, no speedup qualifies.

Once the isolated hard face passes and improves, run all original faces of the
first matched main-plate definition `1713`, then whole watch. For NVIDIA compare
both declared policies, same original input and stated grade on one core first;
its returned mesh must pass independent complete original-boundary acceptance
before any ratio. A common display-ready scope must include owned CPU normal
evaluation because SDK tessellation emits corner normals already. Separate
official conversion/import/healing and independent checking from construction.
Afterward compare fixed complete batches at 1,12,24 physical workers with fixed
CPU/NUMA placement and unchanged full denominator, total RSS and tail latency.
Never compare 12 successful NVIDIA workers excluding spring/spiral against one
owned Python thread doing the complete watch and call it whole-watch speedup.

The deliverable is one table of actual cases, complete correctness outcomes,
wall/memory/work counters and any timeout/failure, plus the real whole-operation
improvement. No performance score, mutation campaign or new alpha certificate
is created by this plan.
