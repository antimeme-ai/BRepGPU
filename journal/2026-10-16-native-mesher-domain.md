# Indexed chart admission and typed initial CDT

`native/mesher/domain.hpp/.cpp` admits original boundary-site inventory and
fills the same `MutableFaceTopology` used for refinement. It supports one outer
and every hole, returns original-loop-indexed traversal adapters, anchors reverse
cycles at their first joint and preserves original constraint keys. Source use
senses and face orientation remain separate. Multiple outers return unsupported;
missing/repeated sites, zero area, crossings, nonincident contacts, collinear
backtracking, outside holes and nested holes reject.

The packed median-balanced segment BVH has exact grouped directed Y crossing
events. A plain indexed ray can still visit all horizontally aligned holes;
this stronger query was selected and reviewed before implementation. A directed
nonhorizontal segment adds its sign on [minY,maxY). A node wholly right of the
probe returns its exact Y-prefix sum; straddling-X nodes require leaf orientation
tests. Subtract each hole's own ray expression once, totaling O(N) own scans.
Balanced child-event merging uses O(N log N) storage/build work. No universal
logarithmic-query or crossing-candidate bound is claimed.

The bridge reads cached transformed coordinates from this topology's fixed
factor, centers them and uses ONE common dyadic exponent for both axes. It builds
owned integer descriptors in memory and calls typed `bgcdt::solve` with unit
weights. Separate axis scaling would change the metric. There is no legacy
H-squared validation scan, C ABI creation, serialization, global Lawson or mutable
incircle table. Admission and initial descriptor/predicate scratch die before
installing packed mutable triangles. Every source boundary is protected before
sealing.

The authored consumer observed actual missing-header RED on the isolated fleet.
Strict Clang18 execution passes loop reversal/source-key protection in a full
non-diagonal metric, protected collinear sites, crossing/backtracking/endpoint
contact rejection, nested/outside holes and multiouter refusal. Aligned families
alternate probes exactly at lower/upper shared Y endpoints. H=16 uses 287 ray
leaf candidates, 39 aggregates, 26 retained event entries; H=64 uses 1295, 327, 50.
Fourfold input produced 4.51x candidates, with no machine-time SLA or all-input
query theorem.

The native fixture emits actual full-metric hole triangles. A single process
reader independently Fraction-transforms literal original coordinates and invokes
the existing `qualify_cdt` coverage/embedding/source/local-legality oracle. It
replaces a duplicate native/oracle CTest or another geometry checker. Actual
ASan/UBSan/LSan process plus reader and actual-source engine/fixture clang-tidy
pass. Root's scoped independent source review ACCEPT is recorded in
`papers/2026-10-16-native-mesher-domain-code-review.md`, including exact Y grouping,
own subtraction, common dyadic scale and temporary lifetime. Root CMake
integration follows; whole source trace/cell/contact orchestration is unfinished.
