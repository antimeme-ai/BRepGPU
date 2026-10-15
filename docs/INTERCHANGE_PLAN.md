# Reference carrier interchange delivery plan

Read INTERCHANGE_DESIGN.md and its source/runtime/oracle reviews first. This
plan completes the whole bounded file-to-owned-carrier-to-native-reader unit.
No production code before design and plan reviews accept. No new dependency,
native rebuild, topology importer or NCSX timing lane.

## 1. Strict ASCII source boundary

Own `python/brepgpu/usda_subset.py` and `tests/test_usda_subset.py`. Public immutable dataclasses:

- `Attribute(name, type_name, value)`; value is an immutable tuple array, with
  nested numeric tuples where required. Preserve the declared scalar/vector type.
- `Prim(path, type_name, attributes, metadata)`; flat document preorder, metadata
  as immutable key/value pairs. No authored wrapper transforms accepted.
- `Document(prims, metadata)`; all source attributes retained, not only carriers.

`parse(data: bytes) -> Document` accepts the designed static ASCII subset and
consumes EOF. `SourceError(ValueError)` means invalid syntax/data,
`UnsupportedSource(SourceError)` means syntax/scene outside the supported subset,
`SourceBudgetError(SourceError)` means a declared resource limit. No successful
partial Document. Fixed limits: 16 MiB bytes, 1,000,000 tokens, 500,000 scalar
array components overall, 4,096 prims and 64 nesting levels. Count before adding
past a limit; a capped path read belongs to the converter. Restrict names to
valid unique prim/property identifiers; reject duplicate paths/properties.
Allow actual exporter vector3d/point3d, double2/3/4, float2/3/4 and int2/3/4 types;
uint/int width32, floats correctly rounded to declared width, all finite.
Metadata: stage defaultPrim/upAxis/metersPerUnit; prim apiSchemas with optional
prepend and customData containing string source. Unknown metadata/syntax rejects.
No import of reference libraries. Write focused literal parse/rejection tests
first and show direct red on the fleet, then implement and pass on the fleet.

## 2. Source packing, normalization and immutable carrier conversion

Own `python/brepgpu/interchange.py` and `tests/test_interchange_conversion.py`.
Re-export the three source error classes.
Public `convert(data: bytes) -> Conversion`, `load(path) -> Conversion` where load
reads at most MAX_BYTES+1 bytes; hash and parse the same retained bytes.

Immutable data contract:

- `SourceNurbs(order_u, order_v, count_u, count_v, knots_u, knots_v, points,
  weights)`, tuples copied directly from the actual source. `domain` property
  returns (Umin,Umax,Vmin,Vmax) from active knot endpoints.
- `SourceFace(key, ordinal, surface_type, trim_type, face_range, loop_count,
  faceuses, nurbs, status, reason, patch_slot, position_bound, du_bound, dv_bound)`.
  key=(sha256,prim_path,zero_based_face_index); ordinal is document-wide face
  index. faceuses is tuple of (original_prim_faceuse_index, same/opposite).
  bounds are exact Fraction values in source coordinates; unsupported faces
  have no patch and zero bounds, status='unsupported'. Converted status='converted'
  and reason='' with explicit global trim scope='uninterpreted'.
- `Conversion(source_bytes, sha256, document, body_id, faces, patches)` factory-
  only immutable snapshot; patches ordered by converted source-face ordinal.

Require exact relevant field types from schema, complete face/type metadata and
all NURBS arrays before slicing. Advance all NURBS offsets including unsupported
carriers; malformed packing aborts. SourceNurbs points are Euclidean XYZ, U-major
V-fast. Face range is (Umin,Umax,Vmin,Vmax) reordered from its two double2 corners.
Use shared body._namespace for numeric Lineage, face kind5, document-wide face
ID ordinal+1 and chart ID ordinal+1, revision1. Full source keys remain separate.
Every original face and every original use survives. No reference pointers,
coordinate welding, false outward normal, or BodySnapshot construction.

Implement the reviewed numerical envelope, exact power-of-two weight selection,
spatial normalization, one-round homogeneous coordinates and exact coefficient
conversion position/partial bounds. Record specific unsupported reasons for
analytic types, multiple spans, clamping/degree/envelope limits. A source with all
unsupported faces still has a valid complete inventory; a sampling caller must
raise rather than silently evaluate nothing. Add direct tests first on fleet,
then source review/fix before caller publication.

## 3. Independent source/native qualification

Own `python/brepgpu/interchange_oracles.py` and `tests/test_interchange.py`.
Do not import converter normalization, parser tokenization, or basis helpers into
the oracles. Public functions:

- `source_jet(nurbs, u, v) -> tuple(Sxyz, SUxyz, SVxyz)` with Fraction components,
  using exact Cox–de Boor basis and quotient rule at actual supplied values.
- `local_jet(patch, s, t)` using independently expanded Bernstein polynomials and
  rational first jets from actual supplied homogeneous controls.
- `qualify(conversion, local_batch, world_batch) -> dict` validates complete actual
  source/Patch/descriptor/lineage/UV/native output binding, native local grades,
  every exact source-to-emitted coefficient and conversion bound, complete face
  inventory and source-world first-jet budgets. Return counts and max errors/
  budgets suitable for TSV; failures raise assertions with source identity.

The oracle uses local_batch._slots/_uv only after caller/native request equality
checks. For a normalized query, evaluate original source at exact affine U/V.
Account for native.convert's rounded double span/factor and independent expected
product-rounding rho as defined in the design. Scalar reference comparison uses
source_jet at the actual submitted float U/V, not a nearby exact-affine point.

Fixtures: unchanged upstream mouse.usda + upstream Apache2 license/attribution;
literal asymmetric rational single-span degree(2,3), nonunit domains, duplicate
geometry with distinct original face IDs/faceuses; nondyadic products; degree-zero
axes; malformed/mixed/unsupported syntax/data. Parser tests independently assert
literal token/numeric values. Foreign reference reads the actual same source
bytes using its own parser, attacking shared-parser blind spots. Do not require
a broad external suite to pass; mark existing external alpha gap unchanged.

## 4. Actual reference, caller and complete costs

Root owns `examples/import_carriers.py` and a small optional reference utility
under scripts. The normal caller imports only owned/stdlib code, reads the file,
converts/inventories and samples each accepted patch on a declared dyadic grid.
Use existing core/plugin and native convert(); validate all statuses, finite
values and actual returned ownership. After correct evaluation, consume a simple
checksum/XYZ/first-derivative rows without doing independent truth work in that
timer. Export source bytes, face/use inventory, original and native controls and
knots, normalized/source query bits, native local and source-world jets, bounds,
hashes and named cost rows as TSV/native binary. Optional SVG may show actual
sample curves only if useful; no new renderer necessary for this input operation.

A fresh worker wall begins before capped source read and ends after native cleanup
and reader consumption; module import/process launch, artifact writing, reference
acquisition and independent qualification have separate rows. Retain required
native validation, uploads and status handling inside the useful caller cost.
Run mouse plus the actual reference-created/exported polynomial sheet and an
asymmetric rational literal file. Reference export uses a copy and reacquires
borrowed handles after consumed construction; do not share reference objects
with the owned process. Import same source in reference runtime and save scalar
query jets and actual requested source UV, with reference warnings/failures
explicit. Full original source hash/prim/face map is mandatory.

Use one excluded warmup and three measured fresh-process repetitions for native
CPU1, CPU24, P1000 and P1001; enough to report exploratory medians/ranges without
starting a tuning campaign. Each case must qualify; suppressed performance output
on failure. Repeat timing only after material changes/failures. Larger sample
count only if needed to expose a meaningful batch, not to claim whole-CAD speedup.
Separate exact qualification cost from file-to-consumed-result cost. No ad hoc
speedup from reference scalar Python calls versus native batches.

## 5. Review, qualification, publication

After the units pass direct fleet red/green checks, review source as whole parser,
conversion and oracle/caller concepts; fix all valid findings. Run affected existing
Patch/runtime/body compatibility tests on CPU/P100; no unrelated topology native
rebuild because ABI unchanged. Selected production-site mutants for weight,
control order/domain, dropped unsupported face and parser duplicate/composition
handling may be used if the direct suite needs fault-sensitivity evidence. Each
must be an actual exercised change and run only on the fleet; no coverage claim.

Review saved actual results/cost scopes after the real caller and reference runs.
Update README/run commands, roadmap theme state, AGENTS, issue36, papers index
and return-pass notes. Publish meaningful branch checkpoints for research,
design/plan, boundary/conversion, direct qualification/caller results and docs.
Merge a PR preserving commits and branch, sync fleet source, retain protected
native binaries unchanged. Continue Boolean/history next after this unit; do not
turn general USD/NURBS/trim support into prerequisites for this first beachhead.
