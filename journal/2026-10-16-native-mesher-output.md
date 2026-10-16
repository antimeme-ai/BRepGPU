# Full native mesh values and publication

Implement the accepted BGNMESH1 schema from runtime plan section6 as owned,
typed arrays with flat range pools. It retains complete original source bytes,
all definitions/faces/occurrences, exact chart sites, physical identity,
ancestry, finite profiles/domain maps/poles, actual cell/facet associations,
quality components and assembly/appearance. It is the result of the same mesher;
a display-only dump is insufficient.

Wire references are canonical1-based typed row IDs. Exact values are gathered
only from semantic fields in canonical traversal order and interned by canonical
limbs, so worker table keys, unused caches and backend paths cannot affect bytes.
The writer emits field widths explicitly, without native padding. The source
snapshot digest uses the existing owned SHA-256 definition. Readback validates
sections/ranges/finite variants and original-source ownership; it does not repeat
all construction geometry as a second native qualification layer. The independent
external reader remains the geometry oracle.

Direct checks first require the missing native writer/reader, then authored full
source-linked mesh fields, unused-pool independence, exact original-byte retention,
literal schema bytes and malformed/truncated/reference failures. Publication uses
a temporary sibling directory and rename after all required outputs succeed.

The parent observed genuine missing-codec linker RED before this implementation.
The actual owned writer/reader now covers the fixed29-section order and all
finite typed schema alternatives through one field traversal. It streams fields
and patches row/section lengths; it never builds a second complete output image.
The decoder prepasses ordered section lengths/counts, parses the embedded native
source once, releases source staging, constructs one sealed exact table, decodes
flat owned pools and applies the same constructor's structural/source checks.
Unknown tags, counts/length overflow, reserved bits, dangling refs, nonfinite
values, unreduced/duplicate/unreachable exact values and trailing bytes fail.

Exact interning is by sign/canonical limb content in first semantic-field order.
The fixture changes worker pool keys, pool order, insertion order and unused
values while preserving byte-for-byte output. Decode pools occupy the reserved
internal run-owner definition UINT32_MAX with unique non-wrapping load IDs;
those identities never enter wire geometry or semantic priorities.

The source snapshot SHA is recomputed over the embedded BGSRC bytes. The original
document digest is checked against the digest retained inside those bytes; the
codec does not claim access to unembedded original STEP bytes. Independent
literal empty/abc SHA vectors and Python hashlib check the actual output.

Current fleet results: strict Release roundtrip/hostile-input fixture passes;
Debug ASan/UBSan/leak fixture passes. Independent Python checks all29 section
headers plus literal EXACT/LIMBS/FACETS/QUALITY/APPEAR payloads and snapshot hash.
The extended real warped-plane source fixture retains a physical-plane chart,
plane cells, identity projected nodal-fan/graph-strip maps, geometric-line
correspondence, a source joint/quotient/closure and Use/Joint branches. Its full
readback is byte-identical. This checks the protocol and source associations;
it does not claim geometric qualification of every variant or a second native
geometry certifier. Finite pole descriptors are checked against original carrier
kind; numerical construction remains the geometry owner's mechanism.

Hostile input checks cover header/section flags, source digest changes, unknown
variant/enum, exact descriptor/limb damage, bad row refs, nonfinite XYZ, count
overflow, truncation, row-length mismatch and trailing bytes. Each rejected load
returns RunResource live bytes to its prior value. Byte/row/work refusals are
typed resource failures.

Independent topology-peer review found a real encoder/decoder resource-policy
asymmetry: section rows and inline list items did not share one aggregate cap.
A real base-fixture cap admitting both separately but refusing their sum first
produced semantic RED. Collect/write now counts section rows, exact limbs and
all inline list items together; Decode starts at prepassed section rows and adds
those same items. A second finding, unbounded constructor work after decode,
is fixed by `seal(Budget&)`, which receives the same remaining IO Budget;
the convenience constructor seal has a finite default work cap. A valid graph
with supplied zero-work validation Budget must fail with resource_exhausted.

Latest focused strict Release plus independent wire oracle are GREEN; ASan/
UBSan/leaks is GREEN0.19s; production and fixture clang-tidy and100-column
formatting are GREEN. The declarative X-macro schema mapping has a local
format-only preservation block because clang-format otherwise changes its
indentation repeatedly; compiler/static analysis remain fully enabled.
Independent topology-peer reread ACCEPTS the focused fixes: writer/decoder share
one cap; decode passes remaining IO work into seal; stream row/section bounds,
forward refs and canonical exact reachability remained sound in that review.
The parent retains whole delivery review/commit ownership. No watch or
whole-operation timing follows from this output unit.
