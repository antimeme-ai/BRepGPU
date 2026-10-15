# Reference-to-owned carrier interchange: source semantics

2026-10-15. Recommend a complete **strict ASCII USD BRep subset → owned
single-span rational carrier** operation, using the already acquired actual
NVIDIA fixture `TestFiles/brep_validator/mouse.usda`. This produces a real
curved foreign carrier without rebuilding our own control recipe. Inventory
all four source faces: two are supported single-span carriers, two explicitly
remain unsupported multi-span surfaces. All trims remain separately retained
and uninterpreted. No production implementation or product/fleet execution
occurred in this study.

## Actual source and acquisition

The existing USD BRep source is pinned to
`c9c5979f04cea8c38213004d4f04df0dcfb40d05`. Read original schema, surface/face
Python bindings, public C++ control accessors, actual evaluator/type-index code,
export semantics and the actual fixture. Its intact source archive/restoration
is already in `papers/reference-catalog.tsv`; no new NVIDIA archive is needed.
`interchange-semantics-sources.tsv` records exact inspected paths and hashes.

The isolated reference runtime is usd-brep 1.0.0, already used by the CPU
characterization. Source HEAD and that wheel must not be assumed ABI-identical.
Public Python Surface exposes domains/evaluation/derivatives, but no control,
weight or knot getters in the inspected binding. C++ `SmBSplineSurface` exposes
those fields, but compiling an ABI-matched extractor would need separate runtime
verification. Root's runtime investigation found no usable existing typed
OpenUSD reader and explicitly selected the bounded owned ASCII subset parser
instead. This is a declared interchange implementation, not a concealed regex
scrape or a claim of general USD composition support.

## Concrete foreign fixture

[`mouse.usda`](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/TestFiles/brep_validator/mouse.usda)
is 37,613 bytes, SHA256
`0fedf1649d094ad38adb14095ddd0fef888016c80245ef488d66b70db1b4d39d`.
It contains `/World/mouse`, one BrepArray and four NURBS faces. Source-only
inspection of literal arrays yields:

| Original zero-based face | U×V poles | U×V orders | Natural U/V intervals | Disposition |
| --- | --- | --- | --- | --- |
| 0 | 10×8 | 4×4 | [0,11.15419580309991] × [0,15.97234071195468] | Unsupported multi-span |
| 1 | 3×2 | 3×2 | [0,6.044749683231338] × [0,2.566025062287801] | Curved quadratic×linear carrier |
| 2 | 2×2 | 2×2 | [0,8] × [0,13] | Bilinear carrier |
| 3 | 10×2 | 4×2 | [0,29.01020273569565] × [0,4.075460448547599] | Unsupported multi-span |

All weights in this fixture are one. Face 1's three U rows are
`(3,4.8,z)`, `(0,5.25,z)`, `(-3,4.8,z)`, each with z=0 and
z=2.566025062287801. These are actual reference coefficients, not a fit or our
factory data. Face 2's poles are (-4,6,0), (-4,-7,0), (4,6,0), (4,-7,0).
Every face has trimType `general`; carrier acceptance must not imply the full
rectangular interval is inside the trimmed face. The source authors an Xform
container without transform operations; no unit metadata or time/composition
operation was found in this file. Keep source units unqualified.

The repository LICENSE and THIRD_PARTY_NOTICES identify NVIDIA-authored material
as Apache-2.0 except where otherwise marked. No separate restrictive notice was
found in this fixture. Retain the fixture unchanged, upstream LICENSE and source
attribution/hash if it is published under `data/interchange`; do not describe it
as NVIDIA's as-built engineering model. No code dependency is adopted by
redistributing this small original regression input.

## Grammar and whole-file admission

Read the primary OpenUSD v25.05 PEGTL text grammar, pinned commit
`25f3d3d84440d0a80cb27f7e3383f2e71d8cfb28`,
[`pxr/usd/sdf/textFileFormatParser.h`](https://github.com/PixarAnimationStudios/OpenUSD/blob/25f3d3d84440d0a80cb27f7e3383f2e71d8cfb28/pxr/usd/sdf/textFileFormatParser.h).
The 57,970-byte original and its LICENSE were acquired unchanged into ignored
quarantine with a restoration manifest. Inspected comment/number/string,
array/tuple/dictionary, metadata and prim/property grammar sections; this is not
a claim to implement the entire upstream grammar.

Own a bounded lexer plus typed syntax tree for the declared exported subset:
header, allowed `def` containers/BrepArray, narrowly allowed metadata including
this fixture's `prepend apiSchemas` and `customData` source string, and explicit
typed scalar/tuple/array properties. Preserve property type and value kinds.
Reject duplicates, unknown syntax/types/metadata, trailing tokens, malformed
numbers, nonfinite values, excessive nesting/counts and overflow. Reject whole
files with composition arcs, variant/instance constructs, transforms, connections,
time samples or unsupported effects; never extract apparently usable numeric
arrays while ignoring surrounding scene semantics. Strings/comments must be
lexed so a keyword in inert text is not mistaken for an operational statement.
[Official USD composition/value concepts](https://openusd.org/release/glossary.html).

This is an external file boundary. Parse directly into owned native values;
do not introduce JSON as internal storage or an intermediate representation.
Distinguish whole-file syntax/schema errors from well-formed per-face geometry
that is explicitly unsupported. Include every face's status and raw source
provenance in the result.

## NVIDIA field meanings and conversion

The authoritative schema is
`source/schema/omniSolid/resources/schema.usda`, not the older proposal copy.
`brep:surface:nurb:controlVertices` are **Euclidean**, unweighted XYZ, in U-major,
V-fast order. `uOrder/vOrder` are degree+1. Each flat knot slice has length
vertexCount+order and includes multiplicities. Weights have exactly one positive
value per control vertex; do not treat source XYZ as already homogeneous or
clamp/substitute malformed weights. The actual NVIDIA evaluator independently
confirms V-curve evaluation per U row. Geometry arrays are packed by surface
type: a face's type-local rank counts preceding faces of that same type. Packed
Brep arrays add body-relative span offsets. A global face index is not directly
a NURBS index when types are mixed.

Admit only degree≤3, pole counts exactly degree+1 in each direction, finite
positive weights and clamped knots with exactly two distinct finite endpoints
a<b, c<d, each repeated order times. No knot insertion, arbitrary B-spline
extraction, analytic-to-NURBS conversion or periodic unrolling is implicit.
Keep source and native-envelope unsupported reasons distinct.

For normalized owned parameters s,t, retain the exact source map
`u=a+(b-a)*s`, `v=c+(d-c)*t`. Convert Euclidean source pole P and weight w to
homogeneous `(w*(P-origin)/scale,w)`, preserving original order. A common
power-of-two weight multiplier preserves geometry when required to fit native
weights [1,256]; a power-of-two coordinate scale fits |A|≤w. Never silently
round these products and call the conversion exact: the first path can require
exact binary64 representability or explicitly reject normalization rounding.
The mouse unit-weight carriers admit exact power-of-two normalization, e.g.
origin zero/scale 8. Native numerical error against submitted controls remains
separate from source conversion error.

Native returned derivatives are local ds/dt. World/source derivatives require
scale/(b-a) and scale/(d-c); second derivatives use the squared/mixed intervals.
Source faceuse `same/opposite` denotes side relative to intrinsic surface normal,
not a universal per-face outward sign. Retain each original faceuse, shell and
region occurrence; carrier jets do not determine material orientation alone.
Identify the source file hash, prim path, packed-body occurrence, original local
face index and emitted carrier/chart map. Do not substitute live pointer values
or assume exporter/live `faces()` iteration establishes a stable cross-session
mapping.

## Trims and direct qualification

Retain the complete original loop/coedge/edge/UV arrays and face ranges with
explicit `trim_status=uninterpreted`. A face range is a bounding rectangle, not
a membership oracle. Unsupported multi-span faces remain in the inventory;
they are not dropped or counted as carrier successes. Optional analytic planes
should wait unless their actual source parameter map is qualified: the exported
schema uses `origin+u*refDirection+v*(axis×refDirection)`, while SMLib can absorb
UV scale into exported ranges. Likewise cylinders can change from angle to
nonlinear rational parameter after NURBS conversion. Export may generate trim
caches and attributes; use a reference copy and retain exact exported bytes.

Independent source de Boor/quotient evaluation against the original arrays,
plus exact Bernstein identities for the admitted clamped span, supplies direct
whole-carrier correspondence. Use asymmetric off-grid samples and first/second
derivative chain-rule checks for native numerical qualification, with endpoints
and explicit unsupported cases. Reference-runtime evaluation is a separate
cross-check only after original face/parameter identity is established; CPU/GPU
parity or successful reference import alone cannot certify conversion. All
product execution remains fleet-only. Broad external alpha qualification,
general USD composition, general trims and imported solid topology remain
unfulfilled, visible work after this useful narrow carrier path.
