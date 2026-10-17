# Native immutable source unit

Sub-plan: implement the accepted source/value milestone in native C++20. Decode
all existing BGSRC001 fields directly into immutable typed rows and flat pools,
retain original bytes, validate lengths/counts/ownership and build reusable
incident CSR. Source units are retained metadata; the adapter already emits mm.
Run allocations use the aggregate resource. Source IDs remain distinct from slots.

The direct fixture independently writes source bytes. It checks original signed
zero/control words, inventory, ID resolution, metadata and lifetime accounting;
then attacks every truncated prefix, structural fields, weights, cyclic usages
and allocation/count limits. The first fleet build is RED because read_source
and the immutable data constructor/lookup have no implementation. No old mesher
or watch worker is involved. Implementation now follows that observed RED.

## Implemented and reviewed

The native reader decodes all13 sections into immutable typed flat pools,
preserves the original wire bytes, checks positive original controls, natural
knot relationships, reciprocal source ownership and assembly cycles, and builds
definition/edge incidence once. All container capacities and shared root/control
blocks use the run resource. A first source-admission diagnostic exposed exact
reserve(end-of-row) growth of the shared scalar pool; removed that quadratic
copy pattern before qualification. The censored diagnostic is not a meshing
benchmark. Shared pools now grow geometrically.

The literal native fixture passes every truncated prefix and malformed header,
weight, cycle, work/count/memory case. A separate fixture authored with the
existing independent writer checks literal fields in all13 sections, including
original authority/map/lifts, saved spans, appearance and UTF-8 diagnostics.
Release GREEN and ASan/UBSan/leak GREEN; actual-source clang-tidy is GREEN.

Independent numerical-owner review found a null resource could reach the file
loader's staging-vector allocator before the byte reader's guard. Fixed at the
public loader entry; the direct fixture requires a typed failure before opening
a nonexistent filename. Resource ownership survives both source construction
and error-path staging-vector destruction. Review ACCEPT is retained in
`papers/2026-10-16-native-mesher-source-code-review.md`.

Actual full-watch source admission:138 products,107 definitions,196 usages,
17,129 vertices,25,871 edges,51,742 coedges,9,768 loops,9,162 faces,9,162 carriers,
77,613 curves,219 styles,46 diagnostics. Original bytes42,485,161; read/validation/
indexing0.257180s, peak reserved158,752,234bytes. These are source preparation
observations only. No replacement full-watch mesh or image exists from them.
