# Reference carrier interchange

## Purpose and boundary

Deliver the model integration/interchange breadth beachhead: read actual foreign
surface data, produce owned native patches and complete source-face outcomes,
and evaluate them on native CPU/P100 with an independent source-basis oracle.
This is carrier conversion, not trimmed-face or solid import. The existing
USD BRep 1.0.0 reference runtime remains a separate experiment, never a core
runtime dependency. No new library or SDK is installed or adopted.

The reference Python API lacks control/knot getters. Its existing USD exporter
emits those fields, while OpenUSD's Python reader is absent. Build an explicit,
owned reader for the **self-contained static ASCII USD BRep export subset**,
with a real lexer, typed immutable syntax values, full-file consumption and
fail-closed unsupported syntax. Do not scrape arrays with regex, infer controls
from samples, reinterpret pybind pointers, or call this a general USD importer.
This revisits the earlier optional adapter omission in ITERATION_1_DESIGN;
it does not relax that document's prohibition on hidden ad hoc parsing.

## ASCII boundary

Require UTF-8, exact #usda 1.0 header, file size at most 16 MiB (capped
read of limit+1 bytes before parsing), bounded token/
array/primitive counts and nesting depth (64). Lex comments, escaped quoted
strings, identifiers including namespace colons, finite decimal/exponent numeric
literals and punctuation {}()[]=,. Strings support only declared escapes; reject
unrecognized escapes, unterminated strings and invalid byte sequences. Reject
assets (@), paths (<...>), binary USDC, expressions and arbitrary tokens.

Accept `def Xform "name" { ... }`, optional nested identity Xform/Scope wrappers,
and `def BrepArray "name" (...) { uniform TYPE[] name = VALUE ... }`.
Names form unique source prim paths. Attribute values are typed immutable arrays;
supported scalar types are uint, int, double, float, bool, string, token and
2/3/4-component numeric vector types used by the exporter, including point3d.
Validate integer ranges, element shapes, token/string distinctions in the typed
record, numeric finiteness and no bool-as-index aliases. Metadata is a separate
narrow grammar: stage defaultPrim/upAxis/metersPerUnit; prim prepend apiSchemas
and customData containing string source. Preserve these declared values. Missing
optional metadata is not inferred. Reject duplicate attributes, prim names,
metadata keys, declarations, malformed separators and trailing tokens.

Reject composition arcs (references, payload, inherits, specializes, variants,
sublayers), `over`/`class`, relationships, connections, time samples, authored
xform operations, instance/prototype and unknown declaration/metadata types.
Unknown valid geometric *surface tokens* remain per-face unsupported outcomes;
unknown syntax does not get ignored. Retain all parsed typed exporter attributes
so trim fields remain available as uninterpreted source evidence. An unsupported
scene/syntax, corrupt packed array raises a named error and
publishes no conversion snapshot. No partially decoded file counts as success.
Coordinates are the unchanged source local coordinates, with stage unit/up-axis
metadata retained rather than converted or invented; authored transforms fail.

## Packed source records and identity

A file content SHA256 plus prim path plus zero-based face index names a source
face. The face's NURBS index counts preceding NURBS surface tokens only, following
the schema. Separate control/knot offsets count every NURBS, including unsupported
ones; never move offsets only on acceptance. Preserve original degrees/orders,
counts, full expanded knot vectors, Euclidean XYZ, weights, face range, trim type,
loop count and every original faceuse index/orientation. Do not use geometric
coordinate equality or pointer addresses as identity.

Validate complete face arrays and packed NURBS-array lengths before conversion.
Validate each NURBS's finite coordinates/knots, positive finite weights, positive
orders/counts, order≤count, nondecreasing knots and nonempty active domain.
Malformed structure is an invalid input, not a partially supported face. Analytic
surface tokens can remain unsupported without interpreting their geometry arrays.
Faceuse indices must refer to actual faces, orientations be same/opposite and
have matching extents. Keep original occurrences even when both orientations
refer to one face. No unique outward face normal is invented from that pair.

Allocate the native body namespace through the existing body._namespace allocator
shared with canonical bodies. Assign source face IDs by document-wide face ordinal
plus one, chart ID equal to that face ID (one chart per converted face), revision 1.
Within each prim, preserve its original zero-based face index separately. Store full collision-
free source keys separately from bounded numeric native Lineage; namespaces do
not establish cross-session persistent naming. Equal carriers stay separate.
Publish each source face exactly once: converted with a patch slot, or unsupported
with no replacement patch and a specific reason. Every result says trims are
uninterpreted/unconverted, including source rectangular trim labels. The source
face range is evidence, not an interior-membership test or a replacement for the
full carrier knot domain. No BodySnapshot or material membership is fabricated.

## Admitted carriers and numerical conversion

Accept nonperiodic single clamped span tensor NURBS of degree 0..3 per direction:
counts equal orders, knot vectors exactly `order` repeated lower endpoint followed
by `order` repeated upper endpoint, strictly increasing endpoints. Multispan,
unclamped, higher degree and out-of-envelope carriers are explicit unsupported
outcomes. This representation itself establishes the Bernstein basis identity;
no angle-to-NURBS approximation or knot insertion is done.

Copy original row-major U/V-fast Euclidean coordinates and separate weights.
Select one exact power-of-two weight multiplier that places every weight in
[1,256], checking actual exact-ratio equality after scaling. Ratio≤256 alone is
insufficient. Reject absent scale or overflow/underflow; never clamp weights.
Select a power-of-two spatial scale in [2^-20,2^20], at least max absolute source
coordinate, with origin zero. Reject absent scale. For each homogeneous control
form (X*w/scale,Y*w/scale,Z*w/scale,w) using exact input float ratios and round
once to binary64. Native XYZ must stay within ±w. Record exact rational maximum
Cartesian control displacement delta in source coordinates, measured from the
actual emitted control XYZ/W*scale against the original Euclidean source net.

With identical positive rational basis weights, max componentwise control
displacement delta bounds position conversion error over the whole carrier in
L-infinity norm. Source first-partial conversion error is bounded by
4*p*(wmax/wmin)*delta/Lu and 4*q*(wmax/wmin)*delta/Lv respectively. Degree zero
makes the corresponding derivative bound zero. These bounds concern control
conversion only; native arithmetic/frame conversion errors remain separate.

Native Patch.uv retains the actual source knot endpoints in (Umin,Umax,Vmin,Vmax)
order; normalized owned query s,t∈[0,1] corresponds to U=Umin+Lu*s,V=Vmin+Lv*t.
Native local evaluation returns S/scale, S_s/scale, S_t/scale. Existing convert()
restores positions and natural source derivatives with scale, scale/Lu,
scale/Lv. Admit source spans in [2^-20,2^20] and reject collapsed rounded span.
Oracle comparisons use exact source U/V from actual binary64 s/t and exact
endpoint ratios; separately account for rounding when passing source U/V to a
scalar reference evaluator: evaluate the source oracle at those actual submitted
float U/V values for the differential comparison, and report displacement from
the native exact-affine query separately. Preserve original domain endpoints
even when non-unit.

For every consumed component, let E be the exact normalized emitted-control jet,
a the actual returned local binary64 value, f=scale (position) or scale/L
(partial) as an exact ratio, g the actual binary64 factor used by native
convert(), and y its actual converted binary64 output. Native local acceptance
is |a-E|≤epsilon with epsilon=2^-40 or 2^-30. Check the actual converted value
against the exact original source with bound
`coefficient_conversion_bound + |g|*epsilon + |g-f|*|E| + rho`,
where rho=|Fraction(float(Fraction(a)*Fraction(g)))-Fraction(a)*Fraction(g)|
is computed from the independently correctly rounded exact product, never from
the observed output y. A corrupt output cannot enlarge its own allowance;
position origin is zero. This covers rounded endpoint subtraction, factor
rounding and product rounding without requiring an exactly representable span
or changing native convert(). The coefficient conversion bound uses the exact
source span L. Compute g from the prescribed binary64 scale/span factor independently;
check converted statuses, fields, descriptors and source UV binding match the
request; this budget must not conceal a wrong domain factor. Record maximum
observed source errors and their per-query acceptance budgets.

## Owned consumption and output

The immutable conversion owns copied source values and native Patch controls;
closing/mutating reference objects or editing the file later cannot affect it.
Read source bytes once under the file budget, hash/parse those same immutable
bytes and retain them for output; never hash one read while parsing another.
Parser and converter import only the standard library and the existing owned
Patch/Lineage API. Optional reference generation/queries live outside that module.
Conversion returns typed faces, converted patches, metadata and source hash, with
explicit unsupported reasons. Native callers use existing Engine; unchanged ABI
and binary libraries are sufficient. No CUDA-only import path or implicit CPU
fallback is selected. Output records use TSV/native binary, never owned JSON.

A real command reads an input .usda, inventories every face, creates native patch
and query sets for converted carriers, evaluates first jets and consumes source
XYZ/derivatives. Export original source bytes, every face/status/use and map,
original control/knot values, actual emitted control bits, conversion bounds,
query bits and native results, source/native hashes and costs. Fail on zero
converted faces for a sampling request; inventory-only conversion can legitimately
contain unsupported faces. Reproducible records must not expose private paths.

## Direct qualification and release scope

Use the already acquired upstream `mouse.usda` unchanged: four NURBS faces,
two admitted single spans (one curved quadratic×linear, one bilinear), two
multispans explicitly unsupported; all general trims remain unconverted. Preserve
source/license attribution in the public fixture directory. Also use an actual
reference-created/exported polynomial sheet with independently known analytic
geometry. A separately authored asymmetric rational degree-(2,3) source fixture
uses non-unit domains and duplicate geometry with distinct source identities;
its original values are independently specified before import. Include non-dyadic
weights to exercise conversion bounds, and degree-zero directions.

Independent source Cox–de Boor basis/quotient first derivatives use exact Fraction
arithmetic over original source arrays, without converter helpers. An independent
expanded Bernstein actual-control oracle checks native normalized grades 2^-40
(position) and 2^-30 (first partials). Check full source-net correspondence,
whole-face status bijection, original occurrence identity and native returned
query/descriptor/lineage binding. Sample source corners, boundaries, asymmetric
interiors and invalid query rows; finite differences and CPU parity are not truth
oracles. Scalar reference queries test actual available source correspondence,
with requested UV/reparameterization limitations explicit.

Run meaningful rejection/property tests for the strict file boundary and source
normalization: truncation, malformed shapes/offsets, duplicate identities, missing
fields, nonfinite/nonpositive data, unsupported composition/type/syntax, array
budgets and independence from later source-file changes. Selected production-site fault injections may check
named parser/layout/weight/domain/identity faults on the fleet; they do not claim
mutation coverage. Build/test/run only on the fleet. No new native ABI or native
memory machinery is introduced, so broad native sanitizer rebuilds add no new
fault-class evidence for this Python-owned adapter.

Measure actual complete file-to-consumed-result CPU1/CPU24/P1000/P1001 work with
fresh subprocesses: parse, validate/convert, context/setup, patch/query upload,
evaluate, download, frame conversion, consume and cleanup. Measure preparation
and independent qualification separately; retain raw repetitions and matched
query fields. Optional reference generation/export is a separately timed stage,
not concealed import work. No whole-CAD speedup or alpha qualification follows.
NIST/CAx-IF/OCCT/general exchange coverage remains missing release work in issue29.
