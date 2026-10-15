# NVIDIA USD/BRep CPU reference for the complete OM10 movement

Study and proposed design,2026-10-15. This is an external reference execution
unit, not an adoption of NVIDIA code into the owned kernel. The operator requires
the same complete movement, assembled and exploded, with NVIDIA CPU timings in
addition to the owned CPU/P100 comparison. The later README-wide reference
backfill is issue63; it does not expand this watch unit.

## Sources actually read

NVIDIA's [v1.0.0 release](https://github.com/NVIDIA-Omniverse/usd-brep/releases/tag/v1.0.0)
and retained source commit
[`c9c5979f04cea8c38213004d4f04df0dcfb40d05`](https://github.com/NVIDIA-Omniverse/usd-brep/tree/c9c5979f04cea8c38213004d4f04df0dcfb40d05)
are the reference. The official Linux asset build is
`1.0.0.v1-0-0.7419.dbb624c8`; its intact44,445,717byte archive has SHA-256
`a30bcd6f1c9028bd29092288e0963d5450ab2b9105fc92a034a6c8ce9d8de2b3`.
[QUARANTINE.md](../QUARANTINE.md) records restoration and licenses. The asset
build identifier is distinct from the annotated release tag's source commit.
The installed Python1.0.0 wheel and release package have different binary hashes;
the native bridge must use the matching package libraries in isolation.

Read source includes `OCCT_BREP_IMPORT/{OcctFileConverter,OcctFileSource,
OcctBinaryParser,OcctAsciiParser,OcctStagingDriver}`, `SM_API_USD/{SmApiUsd,
SmApiUsdTessellate}`, `SM_API/SmApiBrep.h`, `SmApiTessellationParams.h`,
`SMLib/{SmPoly.h,SmBrep.cpp,SmTess.cpp,SmGraphicsExtern.h}`, the public Python
bindings and concurrency implementation. Archived operational guidance was
studied as reference, not adopted as workspace instructions. Existing reference
drivers and [the known chord-tolerance finding](../docs/TESSELLATION_FINDING.md)
also constrain the claims below.

## What the release can actually import

The public Python runtime imports SMLib native BRep/part files and USD BRep;
neither its native-BRep reader nor its USD importer reads STEP. The official
release ships `libOCCT_BREP_IMPORT.a`, `libBREP_STAGING.a`, matching SMLib,
SM_API and SM_API_USD libraries, and their public headers. Its converter entry is
`occt::ConvertOcctFileToUsd(input,output,error)`, with three `std::string`
arguments and no USD types in that signature. Register the schema with
`SmApiUsdEnsurePluginRegistered()` before opening a stage.

The converter reads unmeshed OCCT BRepTools ASCII formats1..3 or BinTools binary
formats1..4. It detects the banner and rejects files larger than256MiB. It does
not require OCCT to run and does not itself read STEP. The selected neutral
boundary is explicit **BinTools V4**, exported by the already discussed isolated
OCCT8.0.1 bridge: binary doubles avoid the ASCII writer's15-digit topology
rounding. This preserves the translated OCCT values, not a claim that STEP
translation is identity. Disable serialized triangulations/polygons/normals.

Export **one original definition-local single solid per file**, all107 original
MANIFOLD_SOLID_BREP definitions, with the source IDs and fresh assembly graph
from the same STEP parse/transfer. Do not export the assembly as a compound:
NVIDIA's compound traversal explicitly skips malformed children. With a single
solid root, solid/shell/face conversion failures propagate. Source exporter and
serialization checks are the source lane's separate reviewed conceptual unit;
no product extraction or meshing file changes are needed.

The NVIDIA converter normalizes and sometimes approximates geometry/UVs. It can
author natural boundaries and pole connectors. Native import healing is a
further distinct operation. Therefore neither heal=False nor successful import
means unchanged source coefficients/topology. The converter's original
`iFaceStorageIndex` appears in errors, but is not retained as a guaranteed public
SmBrep face attribute. Preserve original storage IDs in the source manifest and
label native faces by actual NVIDIA ordinal. Never infer STEP face identity
from two traversals having the same length.

## Two configurations, declared before results

Run **both** independently through the complete fresh caller. Their order is
heal=False followed by heal=True. This is not a failure fallback or a fastest
configuration selection.

| Control | API default | Selected in both runs |
| --- | --- | --- |
| Import healing | native C++ TRUE; public Python FALSE | FALSE in source-preserving-policy run; TRUE in native-default-healing run |
| Partial import | strict FALSE | FALSE, with actual member results retained |
| Chord height |0,disabled |.005mm |
| Curve angle |25degrees |0,disabled |
| Surface angle |25degrees |0,disabled |
| Maximum edge length |0,disabled |0 |
| Maximum aspect ratio |0,disabled |0 |
| Partial tessellation |FALSE |FALSE, with failed original-NV-face indices and report retained |
| Normals |generated |generated, retain actual output |

The release's public headers define absolute chord height in world units and
zero as disabled. Millimetres come from the original-source exporter contract.
This is the same **requested**5micrometre target, not evidence that NVIDIA
achieves it: the existing radius10 sphere study already records a successful
reference tessellation exceeding its request. Native-default-healing is an
honest primary gallery label only when its actual outcome is shown alongside
heal=False and the common requested-quality setting.

## Native API and actual exported triangles

Use `SmApiUsdImportBreps(path,breps,heal,FALSE,&results)`, require exactly one
non-null Brep per definition and every import result successful, then
`SmApiTessellate(brep,mesh,params,FALSE,&failures,&report)`. Keep reports on
failure. Strict output failure remains a failed definition and failed complete
movement; no other mesh fills its place.

The native SDK avoids a needless precision handicap in Python:
`to_mesh_arrays(face_varying_normals=True)` casts XYZ and normals to float32.
`SmPolyVertex::GetPoint()` retains binary64. For every emitted polygon, retain
each genuine physical vertex's point/key and each outgoing polygon edge's
`SmPolyVertAuxData::m_vNormal` as its corner normal. `SmTess.cpp` creates these
from original-surface normals, adjusts for original face orientation, and
attaches them to the emitted edge. This also preserves differing seam/apex
corner normals that a single physical-vertex normal could lose. Confirm those
correspondences with a direct producer fixture before actual watch execution.

Every emitted polygon must have exactly three corners and no inner loop.
Unexpected polygons fail; no owned fan triangulation. Every imported NVface
must contribute triangles or produce an explicit unsupported/failure outcome.
Retain genuine `GetOriginalFace()` correspondence to actual imported SmFace
ordinals while the Brep is alive. Retain the genuine physical-key topology
separately from normal-corner duplication. Reject absent/zero/nonfinite normals,
nonfinite coordinates, invalid indices or physically degenerate triangles.
Only positive normal normalization for display is allowed; no repaired signs
or normals reconstructed from owned surfaces.

The selected output is definition-local. The fresh original occurrence graph
must agree exactly with the independently qualified166 stable source paths,
definition IDs and rigid placements. Apply those placements and the same
authored explosion once. Use byte-identical BGVIEW01 requests, palette and
label map. Own source-carrier geometry is never supplied to the reference
renderer.

## CPU throughput and API alternatives

Ordinary SmApi operations use the process default context; concurrent calls
through that context are unsafe. The supplied kernel operations are individually
single-threaded. Use at most **24 worker processes**, one default context per
worker, reuse that process across its assigned definitions, and never call its
kernel concurrently. Both healing configurations use the same deterministic
assignment from source face counts and definition-ID tie order. Record actual
process count, available CPU affinity, OpenUSD/OMP/BLAS thread environment and
package/binary hashes. Bound OpenUSD's worker limit per process, rather than
silently multiplying24 outer processes by an uncontrolled inner pool. The
outer batch wall encloses startup through all worker exits, not the sum of
overlapping worker stages. A failed worker cannot leave a successful batch.

The supplied private-context `SmApiUsdTessellateBrepArray` is safe across calls,
and `SmApiUsdTessellateFile` uses TBB parallelism across BrepArray prims. These
were considered, not ignored: their public result structure is
`VtArray<GfVec3f>` for points and normals and their implementation invokes
`SmApiTessellate(...,TRUE,...)`, retaining partial members. They also require
OpenUSD C++ types/headers absent from the release package's public include tree.
The direct native worker API gives strict complete output, genuine binary64
corner data and perface reports while still exercising24-way kernel throughput.
It is the stronger admitted reference for this comparison. Do not use an
unsafe shared context to manufacture apparent parallelism.

The public default-context API has Create/GetOrCreate but no destroy/reset call.
`SmApiGeneral.cpp33–63` stores a raw process-global pointer; official examples
retain it for the process lifetime. Explicitly destroy owned Brep/PolyBrep
objects, then include actual worker process exit/resource reclamation in the
outer wall. Do not delete a borrowed default-context pointer and leave the
SDK's global dangling. Source-normal orientation is likewise explicit:
`SmTess.cpp7335–7343` negates the evaluated normal when `bReverse`, and line6976
copies the resulting value into corner auxiliary data. No second reversal is
appropriate in this exporter; direct reversed-box-face checks test that fact.

## Display and complete timing boundary

The [separately reviewed raw display](2026-10-15-om10-reference-display-study.md)
consumes BGREFM01: actual f64 local XYZ/supplied normals/triangles plus original
166 occurrences. It shares the unchanged qualified projection, snapped raster,
lighting and resolve mathematics and uses24 CPU workers. It has no carrier
evaluation, CUDA, OCCT, or NVIDIA dependency. Its raw source-face IDs remain
opaque NV ordinals; actual primitive IDs need not equal owned-mesh IDs.

Call the result **NVIDIA USD/BRep CPU import+tessellation + shared owned CPU
display**. NVIDIA's viewer/Storm/VTK path is not a CPU kernel renderer.

For each healing configuration, one fresh complete two-view process wall starts
before STEP loading and encloses one source transfer/export, all107 NVIDIA
conversions/imports/tessellations, genuine mesh export/aggregation, both CPU
views, labeled PNG/SVG output and all worker/process shutdown. Record separate
STEP/source export, NVIDIA conversion, import/healing, tessellation, f64 export,
actual overlapping batch wall, scene packing, native display, presentation and
complete outer wall. Do not add overlapping stages or halve a two-view wall to
invent per-pose totals. Include this one total on both posters with its scope
explicit. Prepared display timings are distinct, and do not include kernel
tessellation. OS caches are not represented as flushed.

Qualification runs outside timed observations, using the **actual exported
geometry**, and is reported separately. A reference helper/build failure is
not a NVIDIA geometry failure; diagnose it before drawing a kernel conclusion.
Actual complete benchmarking waits until the owned whole-source run passes and
the fleet is quiet. Neither the reference unit nor its dependency path edits
the qualification-pinned owned source/native executables.

## Bounded source and output admission

Success codes and matching tolerance names are insufficient. Before a matched
performance claim, check the complete original source hash/107definition
inventory and fresh166 occurrence identities/placements. Compare original,
translated neutral and imported NVIDIA topology inventories; record every
healing delta. Do not require face counts to remain9162 if real healing merges
faces, and do not hide those changes. Require each imported face represented
and no omitted/partial definition. Reuse the existing direct physical incidence,
vertex-link and perdefinition Euler checks on genuine NVphysical keys; no
spatial coordinate welding or second checker framework.

Proposed finite geometry checks, to be reviewed before implementation:

* Forward: choose the middle emitted triangle of every actual imported NVface,
  plus up to64 evenly spaced additional triangles per definition. Its centroid
  and three edge midpoints must be within.005mm plus the existing numerical
  grade of the **original CAD boundary**. Use original unmeshed OCCT boundary
  shells/faces in `BRepExtrema_DistShapeShape`, never the filled Solid: a point
  inside a solid can otherwise report zero and conceal an inward chord error.
* Reverse: choose one deterministic noncollapsed triangle of every original
  face from the independently qualified owned mesh plus up to64 per definition;
  its physical centroid is a source-boundary witness within the owned5micrometre
  approximation. Its distance to the genuine NV triangles must be at most
  .010mm plus numerical grade, the sum of the two approximation allowances.
  A native perdefinition direct point-to-triangle search suffices; retain actual
  closest-triangle index/barycentrics/distance. This is a qualification query,
  not source/display geometry substitution and not a new mesh/BVH system.
  Witnesses may be read from the already qualified owned scene and CPU
  source-jet files, with their existing hashes; no owned mesher rerun or cached
  geometry enters the fresh NVIDIA timed caller.
* Record native triangle signed volume and bounds against original reference
  diagnostics. They help explain an outcome; they do not prove5micrometre error.

These finite probes can reject underresolution, shifted definitions and missing
source regions; they do not certify a universal Hausdorff bound or exact source
isotopy. If actual probes/topology fail, preserve both configurations' timings,
failed IDs and any explicitly labeled diagnostic image, but do not publish a
matched-quality speedup. If runtime is unexpectedly excessive, stop and review
the actual query cost; do not silently reduce predetermined coverage.

## Remaining blockers and bounded next work

No actual NVIDIA OM10 conversion or tessellation has run yet. First establish
release static/shared-library ABI/runtime linkage and one known unmeshed source
body. Then directly check f64 points/corner-normal orientation/indexing,
strict failure behavior and genuine triangle topology. The original source
exporter and raw CPU display each have their own reviewed plans and initial
reds. The reference worker/outer driver and finite geometry-check additions
need the [subplan](../docs/OM10_NVIDIA_REFERENCE_PLAN.md) reviewed before code.
Do not ship NVIDIA source, introduce it as a product dependency, replace failed
geometry, or expand into README-wide baseline work during this unit.

## Settled adapter interfaces

The separately reviewed [source exporter design](2026-10-15-om10-reference-source-export-study.md)
selects standalone `brepgpu-reference-source INPUT.stp NEWDIR` and the88byte
header/six-section `BGOCC001` manifest `source.bgocct`. Its strict immutable
`load_reference_source` reader reuses typed products/usages/styles and the
existing assembly function. Definition records retain relative BinToolsV4
filenames and original face/carrier/storage diagnostics. No existing product
source adapter or pinned binary is edited or additionally run in the fresh
reference caller. Manifest-only raw checks establish IDs/assembly; they do not
pretend to run the original full coefficient oracle without carrier tables.

The proposed worker is `brepgpu-om10-nvidia`, built only under
`native/om10_nvidia/` in a separate build directory. CLI takes `--heal 0|1`,
`--chord-height .005`, `--output-directory NEWDIR` and repeated
`--definition ORIGINAL_ID INPUT.brep` pairs. Arguments are direct native values,
not a JSON/TSV internal task model. It emits `definition-<id>.bgrefmesh` for each
successful definition, using the existing BGREFM01 format with one definition,
zero instances and one placeholder material. Physical keys and actual NVface
ordinals are definition-local. TSV files are external human-readable reports,
not an IPC model. The complete driver aggregates actual binary mesh records,
assigns source materials and exact placements, and adds no geometric vertices.

## Missing pcurves are an explicit NVIDIA input path

The corrected OCCT no-processing lifecycle exposed absent recovered pcurves on
all 24,602 nonplanar coedge uses. This must not automatically be called invalid
NVIDIA input or used to force owned projection costs into its benchmark.
Pinned official behavior is explicit:

* `OcctStagingDriver.cpp2338–2340` obtains analytic trim bounds from ordered 3D
  boundary curves when pcurve boxes are absent. `StageEdgeuseUvCurve3856–3860`
  leaves the missing curve's `(0,0)` placeholder without deleting 3D topology.
* `SmuConvert.cpp882–884` accepts zero UV order/count as absent. Lines901–908
  deliberately discard even supplied pcurves on analytic surfaces, whose
  internal SMLib parameterization differs; SMLib reconstructs their trims from
  the 3D edges. Import2852–2865 explicitly defers UV creation instead of eagerly
  constructing every curve, independently of the chosen healing policy.
* The NURBS path also supports absence: `ComputeNaturalDomain1442–1464` obtains
  the natural parameter box from source knots. Lines2592–2595 explicitly warn
  that valid open NURBS faces may have an explicit wire and no pcurves; they
  must keep that outer wire, rather than become a whole implied surface.
  Lines2628–2631 may supply the natural **parameter domain**, while original
  explicit loops remain general trims. Implied outer boundaries have separate
  closed/periodic-source admission conditions.
* `SmuConvert.cpp3829–3859` records missing edgeuse UV as `SM_NO_OBJECT`.
  `SmEdgeuse.cpp4255–4310` creates an absent UV curve on demand;
  `CreateUVTrimCurve1230+` uses the original edge curve, interval, owning generic
  surface and face domain. This is an intended projection path for general
  surfaces, not merely a planar exception. Failure remains an actual import/
  boundary/tessellation result; support is not a guarantee that every source
  curve projects successfully.

An identified bridge limitation is outer-loop selection: `OcctFileSource.cpp411`
falls back to source loop 0 when missing pcurves prevent measuring all loop
boxes. Periodic recovery may also synthesize boundary/chart topology. Actual
whole-source counts, closed physical shells and finite original-boundary
coverage must judge those results. No unmentioned repair, dropped source face
or universal success claim follows from the intended missing-pcurve path.

The numerical OCCT distance checker has a different input requirement: trimmed
face classification needs usable pcurves. Its explicit source-query-preparation
failure is **not** a NVIDIA quality failure. Any output-only 2D preparation for
the owned source reader or distance checker needs separate declared scope and
preservation of original 3D curves/ranges, carriers, vertices, topology and
orientations. Do not add that CPU work to the NVIDIA fresh caller unless the
actual NVIDIA input contract requires it. No such blanket requirement was found.
