# Model caller product source review

Scoped actual-source review ACCEPT: native/mesher/adaptive_model_main.cpp and
adaptive_component_io.hpp/.cpp, including the per-definition curve-bank amendment.
No additional execution requested; external model reader closure is separate.

The caller loads one intact SourceStore and computes original envelopes/initial
assembly before selecting placed definitions. Explicit selections resolve through
original IDs and refuse unplaced definitions; default selection retains all placed
definitions. Shared-carrier profiles come from the same retained union, while each
curve bank uses the existing definition request compiler and persists across all
quality queries. Later allowances reuse exact original envelope values and the
existing assembly transport, not rounded metadata or a source reparse. Caller
exact-table keys remain distinct across envelope/quality generations.

Component results and captured boundary leases survive semantic output and scene
construction. Snapshot pointer indexing deduplicates exact retained objects only;
it never determines row order or merges coincident geometry. Nested explicit
relative prefixes remain in faces.tsv. Every face writes its captured boundary
and exact paired current result, and final-boundary.tsv separately names the
settled snapshot. Full original bytes are written once. BOM distinguishes selected
display labels from original occurrence ordinals and escapes original names.

Actual per-call curve counters and cumulative fixed-owner surface snapshots are
labelled separately. Timing scopes include source preparation, bank compilation,
per-query transport/definition work, display/upload/evaluate/render/download,
output and close. The per-definition bank choice avoids reuploading every unrelated
curve for every component; cross-definition source aliases may duplicate extraction,
an explicit accepted tradeoff rather than a change in geometry policy.

Each display owner closes before its query ends; numerical owners and scheduler
drain before all output streams close and the no-replace directory rename publishes.
Failures retain the existing native module/drain protocol and remove only the owned
temporary tree. No new source, geometry, publication or counter finding was found.
