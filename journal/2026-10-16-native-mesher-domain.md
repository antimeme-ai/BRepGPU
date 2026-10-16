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

Initial selected-domain geometry now has an explicit report mode in this SAME
validator. Genuine missing-type/API RED is retained in fleet
`context/domain-report-red.log`. Default callers still reject. The opt-in mode
returns too-few-sites, zero-area, exact boundary-contact or hole-placement issues
before any CDT/connectivity mutation. Contact witnesses name original loop and
segment occurrences; reversed normalization does not rewrite their identities.
Broken inventories remain structural errors. Reporting a selected-polygon issue
does not prove that original geometry is invalid or that subdivision fixes it:
the geometry producer must use its actual source/trace evidence to request a
bounded shared interval split, justify a source-contact refusal, or report
unresolved. A collapsed one-site hole survives as an issue regardless of its
size relative to positional tolerance. The existing strict native fixture and
single Fraction reader pass the new literal cases alongside prior domain work;
the broader producer/forest multi-epoch source family is being integrated by the
geometry owner. No second polygon checker or generic invalid-to-split catch was
introduced.

The report-mode closure also passes actual-source clang-tidy and the full native
ASan/UBSan/LSan domain target through the same existing Fraction process reader
(`context/domain-report-tidy.log`, `context/domain-report-asan.log`). The literal
malformed repeated-site inventory still returns structural BGFD_INVALID_DOMAIN
in report mode; it is never reclassified as a refinable geometric issue.
