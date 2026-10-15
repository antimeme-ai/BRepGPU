# OM10 reference source export design

Design for review, 2026-10-15. This unit supplies the original watch to the
official NVIDIA CPU reference. It does not change the owned mesher or use OCCT
triangles. V3 qualification is running; its source, helpers and binaries remain
unchanged. Implementation follows root design/plan acceptance.

## Original source and retained implementations

The supplied private STEP SHA-256 is
`c60675abe3314dca55a31da0406deef851e128e2ee8608536525f926f56d61e3`.
Its complete inventory is 107 solid definitions, 9,162 original faces, 138
products, 196 local usages and 166 root-to-leaf solid occurrences. The original
representation unit is millimetres. Original bytes and derived geometry remain
ignored and private.

The current owned adapter already performs one STEP read and one STEP/XDE
transfer with shape processing disabled. It retains the transfer process,
original `MANIFOLD_SOLID_BREP` entities and their mapped `TopoDS_Shape` values,
original face entities, product/definition relationships, styles and local usage
matrices. Its existing source geometry has independently passed original STEP
coefficient, topology and placement checks. The reference export must use live
original-entity bindings from its fresh transfer; it must not reconstruct shapes
from the owned mesh.

The authorized OCCT revision is
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42` (8.0.1). Read:

- `BinTools/BinTools.cxx`: `PutReal` writes the eight-byte double payload;
  `Write` sets format/triangle/normal flags before adding the shape.
- `BinTools/BinTools_ShapeSet.cxx`: `Add` retains the original carriers and
  locations, `Write` writes the reversed TShape storage order, and root/subshape
  references use `NbShapes - Index(shape-without-location) + 1`.
- `TopTools/TopTools_ShapeSet.cxx` and `GeomTools` curve/surface sets: ASCII
  topology, vertices and locations use 15 digits, although curve and surface
  coefficients use 17. Setting the caller stream precision does not override
  the topology writer's local setting.

The pinned official NVIDIA source is
`c9c5979f04cea8c38213004d4f04df0dcfb40d05`, with the separately acquired official
1.0.0 Linux package recorded in `QUARANTINE.md`. Read
`source/OCCT_BREP_IMPORT/src/OcctFileConverter.cpp`, `OcctBinaryParser.cpp`,
`OcctFileSource.cpp` and the matching public headers. The string entry point is
`occt::ConvertOcctFileToUsd(input, output, error)` after plugin registration.
It accepts OCCT BRep files, not STEP, and rejects files over 256 MiB. Its binary
reader explicitly supports BinTools V1 through V4, assumes little-endian
IEEE-754 doubles and retains supplied double values. Compound conversion can
skip failed children; therefore every exported file has one solid root.

Primary sources are the [pinned OCCT tree](https://github.com/Open-Cascade-SAS/OCCT/tree/b8f597c677811d1f9f4d8a97f5ae2825c0353a42)
and [pinned NVIDIA bridge](https://github.com/NVIDIA-Omniverse/usd-brep/tree/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/OCCT_BREP_IMPORT).
Studying and running these isolated adapters does not adopt NVIDIA as a product
dependency.

## Selected source boundary

Write one **binary BinTools V4** `.brep` per original solid definition, named
`definition-<original-id>.brep`, with `withTriangles=false` and
`withNormals=false`. Explicit V4 avoids both an implicit current-version choice
and the ASCII writer's avoidable decimal rounding. The initial direct bridge
probe must establish actual V4 compatibility before the complete export is
used. Both processes run on the little-endian fleet.

Each file contains the single mapped original solid in its original definition
coordinates. Require an identity root location, one solid root and an exact
one-to-one match between original solid faces and mapped solid faces. Reject a
located definition instead of silently baking or stripping its placement. Do
not rebuild the solid, normalize frames, repair curves, apply occurrence or
explosion transforms, run `BRepMesh`, or export cached triangulation. Require
actual surface-backed faces; BinTools can retain triangulation-only faces even
with the triangle flag disabled, so those must fail explicitly.

OCCT's existing translation reconstructs pcurves, including five diagnosed pole
chart connectors. Export retains that foreign B-rep representation. It is not
the owned corrected-UV mesh and is not claimed bit-identical to raw STEP
semantics. Binary serialization preserves the translated binary64 values; the
independent source/bridge checks establish the actual scope of correspondence.
No added healing or fallback may be hidden under a successful export.

Use the same `BinTools_ShapeSet` for writing and source mapping: set V4 and both
mesh flags false, `Add(solid)`, write the set, then its root reference, matching
the native `BinTools::Write` sequence. For every original face, store its
serialized TShape storage index and original `ADVANCED_FACE`/carrier IDs. This
is an input diagnostic mapping, not an assertion that NVIDIA preserves face
order. The official staging path does not retain an exact source-face attribute
through USD import. Downstream NVIDIA triangles therefore use **opaque NVIDIA
face ordinals** unless a separate explicit correspondence is established.

`original_face_orientation` comes from original `ADVANCED_FACE.SameSense`,
not an assumed translated traversal sign. Check the translated solid root and
its actual face orientations separately; a disagreement is reported rather
than overwriting original metadata.

## CLI and one-import timing contract

Keep the existing source extractor and its exact source/binary pins unchanged.
Add a standalone reference-only target under `native/reference_source/`:

```
brepgpu-reference-source INPUT.stp NEW_OUTPUT_DIRECTORY
```

It reads and transfers STEP once, extracts the original graph/styles and live
solid/face bindings, writes 107 fresh files and the binary manifest, then exits.
It does not compute owned Bezier spans, build `BGSRC001` geometry tables, mesh,
or parse STEP again. The current product adapter is a monolithic `main`, not a
reusable import-session library. This separate target therefore owns a small
explicit import/graph path using the same pinned OCCT calls and settings:
STEPCAF name/color/material/property modes, no shape processing, mm-only source
admission and system length unit 1. Its live transfer binds original entities
directly. It reuses public OCCT and existing Python typed-value/assembly APIs,
not private `.cpp` inclusion or macro-renamed mains. A shared-source refactor
would alter already executed product pins without improving this reference
boundary; it is not selected.

Only the new exporter runs in each fresh NVIDIA caller. The separate ordinary
source adapter is not also invoked. Existing reviewed source geometry and the
independent raw STEP mechanism are qualification comparators, explicitly outside
the benchmark wall; they are not cached geometry presented as a fresh NVIDIA
import. New exporter qualification attacks its actual original entity bindings,
serialized values and placements. It does not require rerunning the unchanged
owned mesher. V3, the qualified owned scene, pilot source pin checks and the
ordinary source adapter remain untouched before and after V3 PASS.

The reference-only manifest has separate nonoverlapping `read_step`,
`transfer_step`, `collect_source_graph`, and `write_reference_brep` durations,
plus the process complete wall measured by its caller. SHA/input-byte reading is
named separately if excluded from `read_step`; all remain inside the full wall.
Cold external wall encloses fresh STEP export, official NVIDIA conversion,
NVIDIA import/tessellation/extraction, common owned CPU24 display, both poses'
PNG/SVG generation and process exit. No cached prepared geometry is called fresh.
Prepared reference/native timings may additionally reuse an explicitly named
dataset, with their separate timing scope. Fresh processes do not flush OS caches.

## Owned manifest and API

No JSON surrogate is needed. Write `source.bgocct` after every solid file has
closed successfully. All integers are little-endian, doubles finite binary64,
strings `u32 byte_count` plus UTF-8 bytes, row lists length-delimited as in the
existing `Bytes` helper. Header is 88 bytes:

| Field | Type |
|---|---|
| magic | 8 bytes, `BGOCC001` |
| version, flags | u32 each, 1 and 0 |
| original STEP SHA-256 | 32 bytes |
| units_mm, context_uncertainty_mm | f64 each |
| root_product_id | u64 |
| section_count, reserved | u32 each, 6 and 0 |
| total_byte_count | u64 |

Each section has an 8-byte tag, u64 row count and u64 payload-byte count. Each
row has a u64 byte count followed by the row bytes. Exactly these six sections:

- `META____`: one row: exporter/OCCT version string, u32 BinTools format=4,
  u32 mesh flags=0, four f64 named phase durations in the order above. The
  original input-byte/hash phase is included in `read_step` for this format.
- `PRODUCTS`: existing typed source product rows: u64 id, definition_id,
  style_id; name string, reference string.
- `DEFS____`: u64 definition_id, product_id; relative filename string;
  u64 face count; that many records `(u64 source_face_id, u64 carrier_id,
  u64 serialized_storage_index, i32 original_face_orientation, u32 reserved0)`.
- `USAGES__`: existing typed source usage rows: u64 id, parent_product_id,
  child_product_id, style_id; name string; twelve f64 child-to-parent matrix
  values, row-major 3x4 applied to column vectors.
- `STYLES__`: existing exact source style records (color/material/density and
  original source IDs), unchanged from `BGSRC001`.
- `FACE_STY`: u64 source_face_id, u64 style_id, retaining face overrides.

The new small module `python/brepgpu/reference_source.py` exposes immutable
`ReferenceSource`, `ReferenceDefinition` and `ReferenceFace` values and
`load_reference_source(path)`. It reuses the already owned typed
`Product`, `Usage` and `Style` records from `imported_source` rather than a second
graph model. `ReferenceSource` exposes `.source_sha256`, `.units_mm`,
`.uncertainty_mm`, `.root_product_id`, `.products`, `.definitions`, `.usages`,
`.styles`, `.phase_seconds`. Definition values expose `.id`, `.product_id`,
`.filename`, `.faces`; `.face_ids` derives the ordered original IDs. This is
enough for the existing `imported_assembly.assemble(source)` to reconstruct all
166 stable DFS occurrences and exact original matrices. Explosion offsets are
authored later by the same root helper; they are not source metadata.

The reader rejects unknown versions/flags, duplicate/missing sections, malformed
counts/rows, nonfinite timings/units/matrices, unresolved graph/style references,
duplicate IDs, zero/duplicate storage indices within a definition, paths outside
the manifest directory and trailing bytes. Expected actual counts are checked
by the watch caller, not hardcoded into a generic reader. A partial output has
no completed manifest and cannot be consumed. Use a new directory, temporary
files and a final manifest rename; failure reports the original source entity
and stage. Do not overwrite a prior prepared dataset silently.

## Direct qualification and bounded implementation sequence

1. Record a genuine absent-reference-mode fleet red. Add a compact authored
   STEP fixture with two unequal definitions and repeated rotated/translated
   occurrences, using explicit expected source IDs, coordinates and matrices.
   Actual source remains private.
2. Implement only the separate reference-only native target, binary source
   export and its typed manifest reader. The ordinary source target, source
   reader and their pins stay unchanged; no affected product test rerun is
   required merely because another executable uses OCCT.
3. On the fleet, export the authored fixture. Check full original definition and
   face IDs, exact coefficient/coordinate and placement expectations, no mesh
   payload, binary format V4, graph expansion and actual imported solid type.
   Feed one definition to the official NVIDIA bridge: parser/converter success
   alone is not geometry qualification; independently check source bounds and
   surface/edge probes against this authored source. The reference owner owns
   the NVIDIA-side checks.
4. Exercise direct failure cases in the same test module: duplicate or escaping
   filename, dangling product/usage, corrupt face map, invalid/non-mm unit,
   partial export and write failure. No second permanent source checker or
   broad corpus campaign is introduced.
5. Run one full-source reference export outside official timing, using one fresh
   transfer. Compare all 107/9,162 original identities and all 166 occurrence
   paths/matrices against the existing independent raw STEP mechanism. Reference
   owner imports every definition without compound skipping and reports every
   source failure; none may be omitted. Bound individual file sizes before the
   official 256 MiB limit. Source-face map is diagnostic, opaque NVIDIA face
   identity is explicit.
   `BGOCC001` intentionally has no carrier tables: reuse the existing raw-entity
   and assembly checks, not the complete `BGSRC001` geometry qualifier on this
   smaller manifest. Direct V4 fixture/readback checks serialized geometry and
   absent mesh payload; the manifest alone does not qualify all raw carriers.
6. Root and independent owner review the exporter/reader. Only after findings
   close, enable fresh external baseline calls. V3 product files/binaries remain
   unchanged; the reference target uses its own build directory and separate pins.

This is one source-export unit. NVIDIA tolerances, tessellation/normal precision,
reference mesh qualification, and the shared raw display are owned by their
respective lanes. The exporter neither silently repairs a conversion failure
nor substitutes another tessellator's triangles.

Root design and plan review ACCEPT. The new target avoids product pin changes
and a second source parse. Keep phase durations nonnegative and distinct from
source identity: fresh manifests contain different timing values, so compare
original IDs/graph and neutral-body input hashes rather than whole-manifest byte
equality across observations. The completed manifest is the publication point;
partial exports never supply a successful source dataset.
