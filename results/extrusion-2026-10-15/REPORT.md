# Owned convex-profile extrusion

2026-10-15. Actual input polygons now build owned closed solids on CPU with full
feature history. Existing native CPU/P100 first-jet and adjacency readers consume
the output. This is no GPU feature constructor or complete modeling speedup.
[API/support](../../docs/EXTRUSION.md), [design](../../docs/EXTRUSION_DESIGN.md),
[reviewed plan](../../docs/EXTRUSION_PLAN.md).

![Actual16-sided output boundaries and P100 surface samples](costs/many16-cuda-device0-threads1-r1/extrusion.svg)

## Supported geometry and complete source history

The input is a strict convex CCW polygon with3..32 original vertices on exact
1/256 local XY grid in[-.75,.75]², a bounded existing Frame and positive grid
height h∈[.25,2]. Output is the exact product P×[-h/2,h/2]. Global strict support
against every nonincident vertex excludes concave/self-crossing winding stars;
positive adjacent turns alone are insufficient. No sorting, welding, canonical
box/Boolean substitution, fitting or new dependency is used.

Fixed-square cap carriers preserve original source XY control bits/UV/Frame,
trimmed by the actual entire profile. Original edge endpoints and the exact two
height levels create every bilinear side. Shared original vertex IDs define
vertical edges. The lower cap reverses word/sense; intrinsic edge parameters
stay separate from occurrence orientation. Complete domains/affine maps and
vertex-link checks establish one embedded outward closed genus-zero shell.

| Profile | Source vertices | Output vertices/edges/uses/faces | Source entities | Output origins | Requested/admitted rows |
| --- | ---: | --- | ---: | ---: | --- |
| triangle |3|6/9/18/5|12|48|1445/979|
| asymmetric chamfer |6|12/18/36/8|21|90|2312/1930|
| many16 |16|32/48/96/18|51|230|5202/4826|

Every one of3N+3 source entities has two modified same-kind translated cap images;
6N+6 output origins are cap copies and8N are generated side entities. Exact maps
retain local cap translations, original edge/UV parameters, exact profile-local axial
maps and signs. Every parent names the actual source namespace/revision/
kind/ID. Publication binds precise generating IDs/roles: reciprocal history tables
alone could consistently name the wrong source. No output or partial history
escapes a failed construction; prior results/source survive and retry succeeds.

## Direct qualification

| Unit | Actual fleet outcome |
| --- | --- |
| CPU whole construction/history |8 methods passed0.762s after final ancestry-validation refinement |
| Native sampler/ownership |7 methods each CPU1/CPU24/P1000/P1001:0.387/0.393/1.703/2.933s |
| Independent profile/result/sample/topology |11 methods each CPU1/CPU24/P1000/P1001:7.471/7.385/7.987/8.390s,zero skips |
| Existing body/Boolean/adjacency/runtime |63 methods on CPU:62passed,1existing CUDA-only skip,14.721s; that device-restoration method separately passed CUDA0.619s |

These are26 new test methods across their applicable placements. Coverage includes
a verified32-vertex profile, positive-turn winding-star rejection, minimum1/256
edges and minimum height, every Frame scale, cyclic source rotations, extreme
placements, distinct equal-geometry namespaces, malformed/oversized/off-grid
inputs, exact subnormal/nextafter supports and deliberate late failures. The
independent suite reconstructs whole source/carrier/domain/graph/history truth
without construction/trim/validation helpers; it directly reads public exact
material/trim/curve/UV operations and actual native CSR/mates/neighbors, including
slot permutations. Source-bit/type/ancestry/sample faults have direct assertions.
Selected falsified outputs do not imply broad mutation coverage or alpha readiness.

Sample qualification uses original source geometry at actual copied binary64 UV,
not CPU parity. Non-axis unit normals use independent rational integer-square-root
intervals; both perfect-square and nonsquare cases, including tiny native walls at
Frame scales1/4 and4, are exercised. Across all48 campaign callers the maximum
world position component error is 0, and the conservative maximum oriented
normal component error bound is 9.9e-17, versus limits2^-36 and2^-30. All admitted
native statuses are successful; rejected candidates retain explicit status−1/NaNs.
No implicit fallback, missing mate, open boundary or seam is reported.

All five existing native core/CUDA/topology library hashes remain unchanged.
No native sanitizer rebuild was needed for these new Python-owned construction/
adapters; actual late host failure closes all3 native child handles and retries
on the same borrowed Engine/result. [Whole construction review](../../papers/2026-10-15-extrusion-construction-oracle-code-review.md),
[oracle/test review](../../papers/2026-10-15-extrusion-oracle-semantics-review.md) and
[sampler review](../../papers/2026-10-15-extrusion-sampler-root-review.md) retain
scope/findings. Actual source bits, full graph/history maps and native arrays are
saved for every caller. The [actual artifact review](../../papers/2026-10-15-extrusion-unit-artifact-semantics-review.md)
checks graph, complete ancestry, raw CSR and every SVG coordinate, including the [initial CPU triangle](caller-triangle/recipe.tsv).

## Complete costs

One excluded warmup plus three measured fresh processes per profile×placement:
48 calls,12 excluded warmups,36 measured observations in12 summary cells. These
are three real differing topology/query sizes, not replicated bodies or a large-
model throughput study. Clocks/load/filesystem caches are uncontrolled on the
shared Linux x86_64 fleet. Python3.12.3; CPU requested1/24threads; two Tesla
P100-PCIE16GB, driver580.178.04, existing CUDA12.8.1/sm60 binaries. No simultaneous
project GPU jobs or integration/residency tuning.

Useful prepared-profile→consumed wall includes complete CPU extrusion return/
publication, material/edge/UV/history reads, request preparation, Engine/context,
patch/query upload, evaluation/download/descriptor/status checks, source-world
conversion/oriented row reading, Engine cleanup, topology packing/library load,
ABI5 build and actual native CSR mate/neighbor reads. The second points→consumed
wall includes profile preparation from already-owned exact point tuples. Imports/
process launch, independent qualification, SVG/export and reference work are
outside both; full child-process wall is separately saved.

| Profile | Placement | Median feature→consumed ms | Range ms | Median points→consumed ms |
| --- | --- | ---: | ---: | ---: |
| triangle | CPU1 | 85.106 | 79.103–85.878 | 85.718 |
| triangle | CPU24 | 191.416 | 88.917–208.208 | 193.213 |
| triangle | P1000 | 677.379 | 603.983–756.414 | 678.106 |
| triangle | P1001 | 1157.871 | 1106.545–1324.946 | 1158.681 |
| chamfer | CPU1 | 297.610 | 276.594–367.664 | 299.223 |
| chamfer | CPU24 | 190.853 | 145.960–200.657 | 192.163 |
| chamfer | P1000 | 822.236 | 732.642–921.720 | 823.530 |
| chamfer | P1001 | 1396.118 | 1315.148–1434.035 | 1400.100 |
| many16 | CPU1 | 660.569 | 616.780–690.937 | 666.043 |
| many16 | CPU24 | 558.940 | 405.786–615.080 | 565.093 |
| many16 | P1000 | 1033.111 | 810.305–1132.934 | 1038.321 |
| many16 | P1001 | 1477.176 | 1406.848–1688.213 | 1482.301 |

Best CPU median is CPU1 for triangle and CPU24 for chamfer/many16. Every P100
complete-call range is slower than the corresponding best-CPU range. CPU cell
variability is visible; these unpinned three-observation cells do not isolate
thread/clock/load causes or establish CPU scaling. Setup, separate synchronous
topology initialization/cleanup and exact host readers remain material costs.
For many16, the actual phase medians are:

| many16 phase ms | CPU1 | CPU24 | P1000 | P1001 |
| --- | ---: | ---: | ---: | ---: |
| cpu_extrusion | 94.977 | 90.371 | 75.403 | 77.835 |
| request_prepare | 325.823 | 298.853 | 146.749 | 224.022 |
| engine_setup | 4.498 | 2.537 | 240.715 | 538.555 |
| sample_and_consume | 116.297 | 106.048 | 103.596 | 113.458 |
| sampling/normal_scatter_ms | 87.115 | 86.733 | 87.000 | 90.412 |
| engine_cleanup | 0.018 | 0.017 | 39.057 | 86.839 |
| adjacency_build | 0.193 | 4.426 | 204.497 | 419.044 |
| independent_qualification | 1087.553 | 1097.784 | 1106.410 | 1414.228 |

These are individual diagnostic medians, **not additive**. Construction/sample/
adjacency internal phases overlap their enclosing wall spans. Inner construction
total_ms ends before final result metadata publication; cpu_extrusion outer wall
includes the returned feature. Sampling evaluate_ms is synchronous wrapper wall
with native profiling disabled, not isolated kernel time. The topology API owns
its own per-call device setup instead of continuing sampling's Engine.

[Raw48 calls](costs/raw.tsv), [summary](costs/summary.tsv), [scope](costs/scope.tsv)
and [controller pin](costs/sources.tsv) retain actual data. [Caller source review](../../papers/2026-10-15-extrusion-caller-runtime-review.md)
and [performance review](../../papers/2026-10-15-extrusion-performance-runtime-review.md)
retain dispositions. No kernel/whole-feature speedup or language rewrite is claimed.

## Like-sized optional foreign sweeps

The existing isolated USD BRep1.0.0 reference creates the same original lower-world-
plane line segments and a pure capped linear_sweep with world distance h*scale.
Every input endpoint/distance converts exactly to binary64, checked before call;
original curve domains and three samples per curve remain unchanged afterward.
Finite complete tight bounds, returned manifold status, volume and actual boundary
closest points agree with qualified owned output. Each query set combines five
exact samples per physical edge with eight spread native interior rows per face.

| Profile | Reference/analytic volume absolute difference | Largest boundary residual | Queries |
| --- | ---: | ---: | ---: |
| triangle | 2.22e-16 | 2.29e-16 | 85 |
| chamfer | 0 | 0 | 154 |
| many16 | 0 | 2.48e-16 | 384 |

Volume relative acceptance is1e-8; bounds component limit1e-8×Frame.scale and
boundary distance/residual limit1e-7×Frame.scale. [Triangle](reference-triangle/comparison.tsv),
[chamfer](reference-chamfer/comparison.tsv) and [many16](reference-many16/comparison.tsv)
retain actual endpoint bits, source/quality hashes, before-curve probes, returned
measures/residuals and separate reference construction/preservation/query costs.
No subprocess warning/error was emitted. The triangle uses the first saved CPU
caller; other recipes use measured CPU1 r1 artifacts. No public reference material/
history adapter was established; representation IDs are not compared as ancestry.

## Remaining work

Revolution is the next feature unit; fillet and analytic offset remain separately
scoped breadth units. Holes, concavity, curved/oblique/drafted sweeps and general
result meshing are unsupported here. Persistent naming/composed history and GPU
feature construction remain future. GPU lifetime/integration, Python's shipping
role and matched NCSX timing remain deferred as directed.

Applicable external community feature/B-rep adapters and required corpus remain
incomplete/non-green under the mandatory [alpha policy](../../docs/ALPHA_QUALIFICATION.md).
This operation and optional foreign comparison do not waive those release gaps.
