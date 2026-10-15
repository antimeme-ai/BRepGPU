# Complete canonical trim-aware meshes

The owned CPU mesher now constructs complete shared-index triangle shells for
box, cylinder and one-hole plate snapshots, including original rims, hole loops,
face/chart correspondence and both occurrences of the cylindrical seam. Exact
rational construction and actual rounded binary64 meshes have explicit world
error bounds. This is the bounded approximation/meshing beachhead; general
trimmed NURBS, arbitrary imported bodies, triangle-quality refinement and GPU
mesh construction remain future.

![Actual plate mesh](../../assets/meshing/plate.svg)

## Geometry and direct qualification

The independent model reads original body/control/curve/pcurve records. Every
vertex and corner is bound to exact source geometry and parameters. The full
integer directed boundary chain, together with strict projected triangle signs,
proves complete single coverage of each chordal planar domain and each actual
wall chord strip. A count/area/centroid check alone would not establish that.
Global opposite edge uses, single-cycle vertex links and connected shell checks
establish the supported manifold topology. The plate retains genus1 even at
coarse tolerances. Original seam atlas0/4 occurrences remain separate while
physical vertex indices are shared.

The product chooses quarter subdivision n using a conservative world bound
R/(2n²), reserving at least half the tolerance for rounding. Independent exact
minor-arc/dot-product inequalities check each original circle chord and exact
whole-triangle radial minimization checks inward deviation. Actual shared float
coordinates are compared as exact ratios; their maximum L1 displacement is
added to the source approximation bound. Rounded polygon order/convexity,
containment, planarity, vertical correspondence and complete domains are checked
separately. No rounded facet is silently dropped.

Circular caps are approximations: inscribed cylinder caps omit a narrow rim,
while inscribed plate-hole polygons include a narrow strip inside the analytic
void. The two-sided face/shell bound covers that difference. This is not a claim
that all triangle interiors lie in the exact analytic trim.

[Qualification outcomes](qualification.tsv): genuine absent-entry red after
healthy existing native geometry, entry green, all11 direct mesh methods green
in2.995s, then83 affected compatibility methods in15.542s:82passed and one
pre-existing GPU-only pair test skipped for explicit CPU placement. An initial
broad compatibility invocation omitted the topology library environment variable
and was stopped; it earns no pass. The corrected affected-family run is the
reported compatibility result, not a rerun of every historical numerical test.

Direct cases cover all families/frame scales, extreme/translated/subnormal
origins, tolerance thresholds, atomic budget rejection, immutability, original
parameters and chart/seam occurrences. Real falsified outputs exercise missing,
duplicate, reversed and hole-filling triangles; changed source parameters,
split identities, UV lifts, rounded coordinates, bounds and native-type aliases.
These are named fault witnesses, not a mutation coverage or alpha-readiness score.

## Actual serial caller costs

Linux x86-64, CPython3.12.3, Xeon E5-2697v2, one exact CPU process. The operation
constructs a fresh complete mesh and then packs shared XYZ, triangle indices,
face IDs and actual geometric facet normals while consuming every source/corner/
edge/coedge record. No native mesher, CPU24 execution, CUDA or host/device
transfer is involved. Reference input preparation, independent qualification,
repeat correspondence and filesystem export are outside the named complete wall.

Four cases each have first-after-imports/preparation, one excluded warmup and
five repeated calls. All four first outputs passed independent qualification;
all24 later complete frozen outputs and actual indexed buffers matched those
qualified first outputs. Saved exports retain the FIRST qualified mesh. The
[raw28calls](caller/raw.tsv), [summary](caller/summary.tsv),
[counts/bounds](caller/counts.tsv), [single phases](caller/single-phases.tsv) and
[scope](caller/SCOPE.md) preserve actual observations.

| Case | Shared vertices | Triangles | Quarter n | Complete median ms [min,max] | Construct median ms | Reader median ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Box | 8 | 12 | 0 | 7.228 [7.009,7.680] | 6.938 | 0.294 |
| Cylinder | 130 | 256 | 16 | 88.216 [86.386,108.176] | 83.851 | 4.343 |
| Plate | 136 | 272 | 16 | 102.886 [98.771,154.283] | 96.688 | 4.655 |
| Fine translated/scaled plate | 1,032 | 2,064 | 128 | 757.459 [751.416,771.074] | 722.285 | 38.534 |

Independent phase medians need not sum to the complete median. Standard cylinder/
plate tolerance is1/1024, with source bound1/2048 plus actual rounding about
1.99e-17. The fine plate has Frame origin(3,-2,1), scale4 and tolerance1/16384;
its source bound is1/32768 plus rounding about4.41e-16. Box bound is0. Exact
ratios remain in the saved bounds tables. No speedup or general quality ranking
is inferred from these single-host observations.

First independent qualification costs were22.029/237.456/426.524/2,182.195ms
for the four cases; filesystem exports5.555/35.871/18.573/120.123ms. These are
single observations. First complete calls were7.954/84.555/155.317/761.381ms,
not process-cold startup measurements.

A separate [standalone plate caller](standalone/timings.tsv) directly observed
preparation through qualification and filesystem export at751.409ms:0.919ms
body preparation,122.451msconstruction,4.856msreader,602.817msindependent
qualification and20.366msexport. It is a separate observation, not the sum of
independent medians. Final source/timing-file writes and interpreter shutdown
are outside that wall.

## Sources, exports and display revision

Each case directory retains actual source/control bits and packed body tables,
exact/rounded mesh vertices, triangles, chart-local corners, physical edge and
directed original coedge parameters/maps, error bounds, qualification, triangle
PLY and actual indexed SVG views. The cap view draws every top triangle and
leaves the hole open; the second view draws every face without hidden-line
removal and highlights the physical seam. PLY facet normals are geometric,
without smoothing sharp source boundaries.

The cost campaign [source pins](caller/sources.tsv) match commitb68875f for the
caller. Actual image inspection found a caption touching the geometry; commit
ebbef8f changes only three caption y-coordinates. Benchmark raw SVGs/source pins
remain intact. The standalone caller ran that revised source and retains its
own [source pins](standalone/sources.tsv); its SVG supplies the README asset.
The PNG is a static raster of that SVG using the already installed standalone
rsvg-convert2.62.1. No kernel/timing change or new dependency is implied by the
caption correction. Existing protected native libraries were not rebuilt.

Reproduce on the fleet from repository root:

```sh
PYTHONPATH=python python3 benchmarks/meshing/run.py \
  --output-directory results/local-mesh-costs
PYTHONPATH=python python3 examples/mesh_body.py --shape plate \
  --tolerance 1/1024 --output-directory results/local-mesh
PYTHONPATH=python:tests BREPGPU_LIBRARY=build/pair-release/libbrepgpu.so \
  BREPGPU_TOPOLOGY_LIBRARY=build/topology-release/libbrepgpu_topology.so \
  python3 -m unittest test_meshing test_meshing_entry test_arrangements \
  test_arrangement_entry test_classification test_classification_entry \
  test_body test_pairs test_intersections test_adjacency -v
```

## External qualification and next scope

[Inspected OCCT meshing cases/assertions](external-status.tsv) remain NOT_RUN:
no external operation/output adapter executed them against BRepGPU. The actual
mesh/end harness checks missing face meshes, free links, cross-face/asynchronous
edges and free nodes; hole/deflection historical cases add useful external
fault families. This inventory is a candidate starting point, not the complete
required alpha denominator or an external pass. Required applicable community
qualification must be green before alpha under the [release contract](../../docs/ALPHA_QUALIFICATION.md).

Continue narrow interchange and the uncovered solid/feature themes. General
adaptive meshing, quality, arbitrary trim arrangements, native/GPU placement and
integration polish belong to later units. The operator selected NCSX as the
continuing trophy target; its [acquired reference pack and comparison brief](../../docs/NCSX_TROPHY_BRIEF.md)
currently contain documentary images/drawings, without native model geometry.
