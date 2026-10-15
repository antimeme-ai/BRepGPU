# OM10 NVIDIA CPU reference implementation subplan

Proposed2026-10-15; review before adapter code. Read the
[source/API design](../papers/2026-10-15-om10-nvidia-cpu-reference-study.md) and
[raw display design](../papers/2026-10-15-om10-reference-display-study.md).
This completes the operator's explicit external CPU comparison on the same107
source definitions and166 occurrences, both assembled/exploded. It is not the
queued README-wide reference backfill.

1. **Review boundaries.** Root/source owner review the whole reference design,
   strict two-configuration policy, source exporter contract and measured
   finite geometry admission. Keep running owned qualification files/binaries
   unchanged. Source owner owns fresh STEP→unmeshed BinToolsV4 definition files
   and exact graph/ID manifest; native owner owns BGREFM01 CPU display. This
   lane owns genuine NVIDIA import/tessellation/export, outer caller and direct
   actual-reference geometry checks.
2. **Initial red and minimal toolchain.** Add a focused reference producer CLI
   fixture failing on its missing executable, plus a source-known box/sphere
   contract at its actual APIs. Build only a separate reference CMake directory
   on the fleet. Link official static bridge/STAGING against its matched native
   release libraries; register plugin before opening USD. Preserve package
   hash/licenses and isolate LD_LIBRARY_PATH from the installed wheel. A tiny
   ABI/import probe is not a benchmark or watch raster. Resolve actual runtime
   issues before adding more adapters.
3. **Native worker.** Add owned reference-only native code, not copied reference
   implementation. Accept a bounded list of definition IDs/input BRep paths,
   explicit heal flag and new output directory. Own one process context and
   release every SmBrep/SmPolyBrep before context shutdown. Convert each single
   solid through NVIDIA's official bridge, strict import, strict tessellation
   with the declared controls, then actual f64 corner extraction. Emit one
   BGREFM01 definition-local mesh per source definition (zero placed instances,
   one placeholder display material) plus typed TSV reports for imports,
   failures, topology/counts and scoped times. Use the shared format directly,
   no extra mesh persistence format. Check every imported face represented,
   every polygon triangular/no holes and every actual normal/index/key valid.
   Failed definitions remain explicit; batch status cannot hide them.
4. **Direct producer green/review.** At the real SDK boundary, check a known
   finite body for binary64 point preservation, per-corner orientation/indexing,
   no source face invention and definition-local geometry. Exercise strict
   import/tessellation/invalid path failure; inspect actual output triangles,
   not only status. Use one curved body to exercise distinct normals and
   normals at shared physical vertices. Review all extraction/lifetime/timing
   code. Fix findings; do not begin a corpus campaign.
5. **Complete fresh outer caller.** Add a reference-only driver that invokes
   the fresh exporter once, parses directly into frozen source graph values,
   checks complete original identities, and starts at most24 native worker
   processes. Assign definitions deterministically by source face counts with
   ID ties, reuse each worker for its list, one default context/process.
   Record available affinity and thread settings; bound inner OpenUSD workers.
   Aggregate genuine binary mesh records without coordinate/normal rounding,
   apply fresh exact166 source placements plus shared authored explosion and
   materials, and invoke the separate CPU24 raw display with byte-identical
   two-view requests. Use the existing presentation/labels only after actual
   raw outputs. Failed children terminate the complete run; output directories
   must be new. No JSON internal model, fake surfaces or fallback triangles.
6. **Direct actual-reference qualification.** Extend existing independent
   source/physical checks to the actual NVmesh, using genuine keys/ordinals.
   Add only the reviewed finite original-boundary forward and reverse witness
   queries. Their direct fixture distinguishes boundary-shell distance from
   filled-solid distance; a shifted or deliberately underresolved curved
   triangle must fail the fixed limit. Reverse closest-triangle arithmetic
   uses literal interior/edge/vertex cases. Keep these checks outside complete
   benchmark walls and label finite measured scope. Review before full source.
7. **Full source before images.** After owned whole-source PASS, nonvisual
   source/import/tessellation and admission run both heal=False and heal=True
   on all107 definitions. Retain complete outcomes, counts/topology/healing
   deltas, failures and measured geometry discrepancies. Check fresh166 source
   placements and actual view/explosion identity. A failure does not authorize
   source editing, tolerance changes, hidden dropped parts or selecting only
   the favorable policy. Discuss any new capability requirement as one unit.
8. **Actual two-view observations.** On the quiet fleet, fixed order heal=False
   then heal=True, one fresh complete source→both-labeled-views observation
   each. Complete wall includes process startup, actual fresh source export,
   all107 conversions/imports/tessellations/export, aggregation/display,
   presentation and shutdown. Report overlapping batch wall separately from
   sums of per-worker stages. Both pose posters share the explicitly labeled
   two-view total. Keep requested5micrometre setting distinct from actual
   measured admission; failed/mismatched reference cannot support a headline
   speedup. Stop after the watch delivery and hand off issue63's next tranche.

No additional product library is adopted. No edits to pinned owned geometry,
native target, shader/camera or existing reported measurements are needed by
this implementation. Actual source and operational logs stay ignored/private.

Root whole-design and subplan review ACCEPT, with these implementation details:
keep original boundary shapes resident during finite forward queries and measure
one complex-definition query cost before the complete predetermined coverage.
If cost is excessive, review it without quietly reducing coverage or thresholds.
Reverse witnesses can be read from the completed qualified BGMESH01's retained
XYZ/triangle/source-face records; do not rerun owned meshing merely to recover
those witnesses. Its original mesh/error provenance remains required. Embedded
source-export timing values are excluded from cross-run source-identity equality.
Initial direct red precedes producer/driver implementation.

## Finite geometry helper: concrete reviewed boundary

Proposed appendix before helper code. Source/export owner may own this whole
bounded checker after the exporter closes; reference owner supplies witness
packing and actual caller integration. It is one direct actual-geometry
mechanism, not a checker framework. Add only a separate
`native/om10_reference_geometry` target, linked to the existing pinned OCCT8
prefix and shared raw BGREFM01 decoder, with no NVIDIA dependency:

```
brepgpu-om10-reference-geometry --source original-definition.brep \
  --mesh genuine-definition.bgrefmesh --witnesses source-witnesses.bgprobe \
  --tolerance .005 --output-directory NEWDIR
```

Require one definition-local raw mesh, zero occurrences. Load the entire original
unmeshed BinToolsV4 solid once; collect every trimmed original face into a
boundary FACE compound (or all original boundary shells). Never query the filled
Solid, never NVIDIA-converted surfaces, and never create an OCCT tessellation.
Keep the boundary resident throughout this process's queries. This is the
**translated original OCCT boundary**, whose source correspondence is checked
separately, not a direct exact raw-STEP distance oracle.

Forward selection is fixed before outcomes: group actual NVtriangle indices by
opaque native face ordinal, select each group's middle triangle in emission
order, and union with `min(64,N)` evenly spaced whole-mesh indices
`floor((2*i+1)*N/(2*min(64,N)))`. Deduplicate indices, sort them, query centroid
and all three edge midpoints. Every measured distance to original trimmed
boundary must be finite and at most tolerance plus2^-32mm numerical grade.
Explicitly set distance solver deflection rather than inheriting a default.
Failure to solve is a failed check, not distance0 or a skipped probe.

Reverse witness `BGRPROB1` is little endian: magic8, u64 count; each40byte row
is u64 original STEP face ID, u64 qualified-owned triangle index, three f64
physical-centroid XYZ. Derive the same middle-per-original-face plus64 global
selection from already qualified BGMESH01 source-face/triangle/retainedXYZ
records (or corresponding qualified CPU jets); no owned mesh rebuild. All
selected triangles must be positive/noncollapsed. Source witness provenance
includes existing scene/jet hashes. The helper evaluates distance to actual
genuine NVIDIA triangles with a direct perdefinition point-to-triangle search,
retains selected closest triangle ID/barycentrics/distance, and requires at most
2*tolerance plus2^-32mm numerical grade. No BVH, fabricated source geometry or
replacement mesh. Validate positive finite tolerance and finite witness inputs;
check exact input lengths and bounded counts. A nearest point must actually be
on the returned original triangle, including vertex/edge/interior cases.

Write external `forward.tsv`, `reverse.tsv`, and summary/timing TSV files with
counts, maxima, failures and query wall. Keep genuine failed probes, with
nonzero exit; no successful qualification table on exception/partial output.
Volume and bounds are explanatory diagnostics, not additional gates purporting
to prove the same metric. The existing independent physical-key shell mechanism
and source coverage checks remain outside this helper.

Initial red precedes code. Bounded direct fixtures distinguish filled-solid
zero from correct boundary distance (sphere center has distance R), known
underresolved/shifted mesh rejection, literal triangle interior/edge/vertex
nearest points, and malformed/nonfinite input. After greens and independent
code review, pilot one actual complex definition and report real query cost
before complete predetermined coverage. Do not silently reduce probes or raise
limits if costly; return the measured problem for review. Qualification remains
outside actual complete benchmark walls, and finite probes establish measured
admission only, never a universal Hausdorff certificate.

Root reviewed and ACCEPTED this concrete appendix before helper code. The source
owner owns the whole native query unit after closing the exporter; the reference
owner packs retained qualified witnesses and integrates the caller. Record the
actual OCCT solver deflection in result metadata. Its finite numerical distance
is a measured admission check, not a certified exact distance. The authored
missing-entry red precedes implementation and independent code review precedes
actual source query pilots.
