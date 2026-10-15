# Interchange design: runtime review

Reviewed `docs/INTERCHANGE_DESIGN.md` against the actual existing reference wheel,
the saved bicubic/mixed exports, pinned mouse file, exporter/schema, owned Patch
and Engine boundary, roadmap and original iteration-1 adapter limitation.

**Accepted with two implementation clarifications; no design blocker.** The new
unit explicitly owns a lexer/typed immutable tree, consumes the complete file and
rejects composition instead of scraping known arrays. Actual exported syntax fits
the stated primitive/metadata/array grammar. A mixed analytic source remains
inventory, not a guessed conversion; source face/type ordinal mappings advance
through every NURBS, including unsupported carriers. Degree/count/knot validation
and native envelope checks occur before publishing the whole immutable result.

The 16 MiB input limit must bound acquisition itself: read at most limit+1 bytes
and reject overflow, rather than loading an arbitrarily large file then checking.
The plan/API should give concrete token/primitive/array budgets and named public
errors. Exact finite binary64 ratios bound arithmetic operand sizes; no new native
allocation ABI or reference handle escapes into the product.

“File mutation raises” should mean a detected change during path acquisition.
The retained byte snapshot and its digest define identity. Later file edits cannot
invalidate an already owned result, and no ordinary path read can promise to
detect every concurrent writer. The caller should read once and hand those same
bytes to both parsing and export/provenance recording, avoiding a second read
that could silently describe a different source revision.

Control/weight normalization uses actual retained values, one-round homogeneous
coordinates and exact power-of-two weight preservation. Positive-weight convex
bounds concern conversion displacement; native arithmetic and restored parameter
derivatives retain independent grades. Full original endpoints and normalized
query bits make non-unit-domain reference rounding visible. This is compatible
with existing ABI1 Patch/convert(), with no core rebuild or dependency install.

Actual mouse import supplies four reference faces with matching source domains;
two single-span carriers are feasible and two multispans remain explicit. The
separate source Cox–de Boor oracle, actual-control native oracle and scalar foreign
queries attack different fault classes. Full face/use/status input binding is
necessary; reference parity alone is insufficient. Trims remain uninterpreted.

Complete fresh-subprocess file-to-reader walls and separate preparation/reference
qualification costs support honest CPU/P100 comparisons. Cleanup belongs in the
complete scope; optional source generation/export cannot disappear as unrecorded
setup. No speedup, general USD support, imported solid or alpha readiness follows.
No implementation or execution was performed during this source review.
