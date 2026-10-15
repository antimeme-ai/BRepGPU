# Executable reference-to-owned carrier acquisition

2026-10-15. Runtime/acquisition study; no product implementation, build, install,
or production dependency adoption. Read the working doctrine, roadmap, original
iteration-1 adapter limitation, CPU reference caller, pinned exporter/schema and
public surface/control accessors. Fleet probes used the existing reference
environment; local work only inspected sources and saved artifacts.

## Recommendation and actual availability

Implement an explicitly supported, strict ASCII BrepArray subset reader as a new
owned unit, with a lexer, typed syntax tree, whole-input consumption and explicit
rejection of composition. This is a newly researched interchange operation, not
the ad hoc USD scraping rejected in ITERATION_1_DESIGN. That old limitation must
remain visible until the new design/plan is reviewed and the actual path qualifies.
No parser has been written or qualification claimed here.

The existing Linux CPython 3.12 reference environment imports **usd-brep 1.0.0**.
Its Surface exposes scalar evaluate/derivatives/normal and domains/periodicity,
but no control/knot accessor. `pxr`, `OCP`, `OCC` and `cadquery` were absent in this
interpreter. The wheel nevertheless carries native OpenUSD, SMLib and API shared
libraries. `sm.usd.export_brep` and `sm.usd.import_breps` work without pxr, although
native module-loader warnings report the missing Python modules. Those warnings
must be retained as reference diagnostics, not treated as conversion failures or
suppressed to fabricate a clean runtime.

Two actual generated exports were retained in ignored context for static design:

| Actual export | Bytes | SHA256 |
| --- | ---: | --- |
| One-face bicubic sheet | 4116 | `54d6e6f8457f6bb6d967389ed2c801c63d8bdf25f7fd33c8fa286e3c33250c4b` |
| Mixed analytic cylinder body | 4678 | `ce10c73cb5d4af2aa70a1fe0dfcafc6a405a7adbbee7719ac6fa75405356b7d8` |

The sheet was made by reference `create_surface_from_points`, then consumed with
four reference 3D boundary curves by `trim_surface_with_3d_curves`. Its exported
carrier has 4×4 controls, orders 4/4, full knots `[0,0,0,0,1,1,1,1]`, unit weights,
one authored face and four exported UV trims. These are actual exporter values;
the eventual reader must consume those arrays, never reconstruct the constructor
recipe. Scalar reference jets at (.25,.75) were finite and nonplanar.
The cylinder exported a **ConeAPI with semiAngle 0**, plus two planes. Therefore
classifying a face by the constructor name or expecting CylinderAPI is unsound.

## Existing external fixture works

Pinned `TestFiles/brep_validator/mouse.usda` is 37,613 bytes, SHA256
`0fedf1649d094ad38adb14095ddd0fef888016c80245ef488d66b70db1b4d39d`.
Copied unchanged to the fleet reference workspace and imported with
`sm.usd.import_breps`: **one body, four faces**. Each actual Face and Surface domain
equals its authored face range, all with lower bound (0,0):

| Face ordinal | Upper U | Upper V | Control grid | Orders |
| --- | ---: | ---: | --- | --- |
| 0 | 11.15419580309991 | 15.97234071195468 | 10×8 | 4/4 |
| 1 | 6.044749683231338 | 2.566025062287801 | 3×2 | 3/2 |
| 2 | 8 | 13 | 2×2 | 2/2 |
| 3 | 29.01020273569565 | 4.075460448547599 | 10×2 | 4/2 |

All were nonperiodic in both directions and returned finite midpoint position,
du and dv. This proves reference acquisition/query viability, not all-parameter
correspondence, exact trims, face ordering beyond this observed fixture, or an
owned import implementation. Faces 0 and 3 have interior knots; an implementation
restricted to single spans must explicitly report them unsupported. The semantics
study decides whether bounded owned span extraction belongs in this first unit.

## Actual source representation and parsing boundary

Use the pinned **source/schema/omniSolid/resources/schema.usda**, not the older
proposal schema, which lacks current analytic surface types. The actual files
contain `#usda 1.0`, a `def Xform` parent, `def BrepArray` child, parenthesized
metadata with `prepend apiSchemas` string lists and typed `customData`, then
`uniform TYPE[] NAME = [...]` attributes. Values include tuples, signed integers,
decimal/exponent floats, strings and empty arrays. Relevant actual types include
double, uint, token, point3d, vector3d, double2/double3, int2 and float3.

The owned subset must tokenize punctuation, identifiers, quoted strings/escapes,
comments and numbers with offsets; parse a typed immutable tree; reject duplicate
prim paths/properties; validate tuple widths and scalar types/ranges; and consume
EOF. Define accepted metadata explicitly. Unknown metadata or semantics cannot
be silently skipped. Reject references, payloads, subLayers, inherits, variants,
timeSamples, relationships, authored transforms, external asset paths, USDC/USDZ,
and unsupported prim/type declarations. A plain Xform container without authored
transform is identity. A file rejected for syntax/composition is different from a
well-formed face with an unsupported carrier: the latter belongs in the inventory.
Do not advertise general USD loading or stage composition.

NURBS arrays store **Euclidean points plus separate positive weights**. Exporter
`SmuConvert.cpp::smu_AddSmBSplineSurface_ToBrepArrayData` divides native homogeneous
XYZ by W and writes controls U-major, V-fast. Orders mean degree+1; each full knot
slice has vertexCount+order entries. Homogeneous owned controls require multiplying
the parsed Euclidean values by W; that operation has a real rounding budget.
Validate total control products, complete array consumption and knot slices before
extracting any patch. Face-to-type-array identity is the occurrence ordinal among
faces of that surface type, not the face index into every carrier array. Preserve
the actual file hash, full prim path, original face ordinal, typed carrier ordinal,
original domain and span/chart mapping. An ordinal is scoped to this file revision,
not a persistent cross-session CAD name.

## Native alternative considered

Read-only symbol inspection found exported SMLib GetDegree, GetControlPoint,
GetControlPointNet, GetControlPointsExpert, GetKnots/GetKnotsPointers/Expert and
SmApiReadBrepFromFile/GetFaces. A standalone reference extractor could load an
actual `.smb` into its own context and copy through these documented APIs. It must
never decode pybind object addresses or rely on guessed native layouts. However,
the wheel has no matching SDK headers; acquired Face::GetSurface is inline and
therefore couples the compiled helper to header layout. Matching source/wheel ABI,
reference initialization, linking, RTTI and destruction still need an actual build
qualification. No such build was attempted. This is plausible future reference
tooling, not an already runnable solution or a reason to change the product ABI.
The selected bounded ASCII path avoids this build/ABI coupling and library install.

## Lifetime, failure and cost contract

Keep reference acquisition as a separate caller/tool dependency. Export mutates
generated UV trims, caches and destination-path attributes even when some export
options are false; export a **body.copy()**, retain original body/face/surface
handles for reference checks, and never concurrently mutate or convert that body.
`turn_to_nurbs` can replace geometry and invalidate borrowed handles; avoid it in
this acquisition lane. `trim_surface_with_3d_curves` consumes its supplied surface
and curves even on failure, so the probe reacquired the resulting face carrier
rather than reusing consumed inputs. No borrowed reference pointer enters Patch.

The reader takes bounded external bytes, retains their digest and copies validated
native values into recursively immutable owned records. Set explicit byte/token/
nesting/prim/face/control/knot/span limits before allocation, checked products and
bounded exact-arithmetic work where used. Reject nonfinite or overflowed numeric
values, zero/negative weights, malformed counts, invalid knot multiplicities and
unsupported degrees/envelopes. Never clamp weights or discard faces to succeed.
Reference crashes/timeouts are caller failures, with exit status and diagnostics;
they cannot become unsupported geometry or a passing partial body. Parse or
resource failure publishes no partially assembled result. Syntactically valid
unsupported faces remain explicit immutable records with typed reasons.

Measure actual source load or creation, copied-body/export preparation, disk I/O,
read/lex/parse, carrier extraction/normalization, native packing/allocation,
upload/evaluation/download, frame/parameter derivative restoration, reader and
cleanup. A complete consumer wall includes the selected required phases; report
reference scalar correspondence and independent geometry qualification separately
if outside it. Distinguish first call after imports from a fresh process. Retained
source/parsed/Patch/Engine states are different scopes, not interchangeable warm
numbers. No speedup or low extraction cost is established by these probes.

The first useful caller should consume actual mouse carriers and an asymmetric
exported bicubic, bind every query to original face/span/domain, compare positions
and chain-rule first jets to actual reference Surface queries, and independently
grade native jets against the retained imported control bits. Preserve all face
inventory and opaque trim/topology source data; none of this constructs a trim-
qualified BodySnapshot, imported solid, Boolean or new topology.

Sources: [pinned exporter](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/BREP_SM_USD/src/SmuConvert.cpp),
[current schema](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/schema/omniSolid/resources/schema.usda),
[public extraction header](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/inc/SmBSplineSurface.h),
[OpenUSD concepts](https://openusd.org/release/glossary.html),
[USD import/export reference](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/.agents/operations/usd_import_export.md).
See `interchange-runtime.bib` and `interchange-runtime-sources.tsv`.
