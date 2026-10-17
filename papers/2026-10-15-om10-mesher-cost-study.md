# OM10 mesher costs: deferred return-pass notes

Read-only study of `python/brepgpu/imported_meshing.py` at `0919d0a`, during
complete nonvisual watch qualification V4. This selects no implementation,
changes no grade and establishes no complete watch timing. These candidates
belong to [the return pass](../docs/RETURN_PASS.md), after the watch delivery.

A low-priority read-only mmap of completed source-preparation V5 tables found
51,742 original coedges in 9,768 loops across 9,162 faces. Original coedges per
face: median 4, p90=5, p99=23, max 965. Face173108/definition1808 has 965 across
55 loops; other large planes have 536, 461 and 416. There are 375 multiloop faces.
The 635 spline carriers contain 2,839 extracted spans; the maximum 103 belongs
to face167659. Summing `n*(n-1)/2` over ORIGINAL face coedge counts gives
2,046,152 pairs; the largest contributes 465,130. This is a topology-size
indicator, not measured predicate work: actual sampling changes n, adjacency
exclusions apply, and intersections can terminate scans early.

V4 completed initial walls of 16.858 s source preparation, 6.934 s decode, 0.0077 s
assembly construction, 17.684 s original geometry comparison and 15.152 s original
assembly comparison. These are qualification stages, not rendering benchmarks.
Stopped V3 ends at edge 25,000; that neither times nor identifies its subsequent
hotspot. At this inspection V4 was active without a recorded failure; silence
is not a hang. Later qualification outcomes belong in the watch delivery log.

Static code identifies these concrete costs, **not measured dominant hotspots**:

- **Before edge progress:** validation, incidence building and spline-joint
  preparation precede the edge loop. The first marker requires 1,000 completed
  edges; no separate joint-stage marker exists.
- **Edge work:** recursive interval bounds repeatedly evaluate shared spatial/
  pcurve endpoints. Circle candidates recheck all proposed edge intervals,
  including unchanged intervals; canonical materialization evaluates positions
  again. Existing profile and metric caches already remove other work.
- **After edge progress:** circle schedule coordination rebuilds affected planar
  boundaries and runs all-pairs segment scans for candidate attempts; canonical
  edge materialization then precedes all face progress. `_segments_touch`
  ALREADY rejects disjoint AABBs. A broad phase would need to reduce pair
  generation while preserving that exact narrow predicate and occurrence
  exclusions. `segments[i+1:]` additionally copies tail lists during enumeration.
  Existing exact-sequence caching already avoids duplicate final plane scans.
- **Large faces:** hole bridges test visibility against full boundaries and
  winding membership. Ear candidates scan a fixed occurrence array, including
  dead entries; common successful-ear scanning is quadratic, with rejected
  candidates adding work. The 55-loop plane matters more than the median face.
- **Refinement:** spline derivatives are cached, but each triangle bound still
  enumerates every carrier span for overlap and reduces maxima. Active triangle
  incidence, stale queue IDs and the whole source-keyed mesh remain resident.
  Actual memory peaks, queue ratios and exact-predicate fallback rates are
  unmeasured. Lawson flips already use integerized exact determinants.

First measure bounded stage walls and aggregate work counts in the EXISTING
mesh outputs: pre-edge preparation, interval refinement, circle scheduling,
edge materialization, triangulation/refinement; largest sampled boundaries;
pair visits, candidate/ear attempts, exact fallbacks, splits and active/queued
peaks. Sparse stage/face progress would make long quiet phases intelligible.
This is instrumentation, not another qualification system.

Then consider conservative indexed segment candidates with the same exact
checker; immutable edge-endpoint/interval caching; changed-edge invalidation
for reusable face boundaries; span-grid/range-maximum lookup; and indexed ear
search or an owned constrained triangulation design. Preserve shared edge
schedules, duplicate bridge occurrences, source identity and current error
obligations. Only measured counts should drive native ownership, Python's
role, GPU batching or definition concurrency; skew and full-mesh memory matter.
No implementation, dependency, rewrite or certification campaign is selected.

Inputs were existing private V3/V4 logs and immutable V5 source tables. No
second source decode, mesh, benchmark, test, profiler attachment or product/
helper/binary edit was performed. Private inputs remain ignored.
