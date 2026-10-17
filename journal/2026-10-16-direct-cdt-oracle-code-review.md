# Independent direct CDT oracle code review

Reviewed `tests/direct_cdt_oracle.py`, `face_domain_oracle.py`,
`direct_cdt_cases.py`, `test_direct_cdt_oracle.py`, the original external-domain
reader move/import, and `docs/DIRECT_CDT_STRATEGY.md`. ACCEPT; no blocking
findings and no code changes required.

The generic Fraction elimination correctly accumulates pivot values and row
swap parity while eliminating without row normalization. Its original-value
four-column matrix has the required positive-inside determinant sign for a
positive triangle. Metric conversion preserves the exact actual binary64
values. Raw source node rows and loop slots are checked before dictionary or
set construction could hide duplicate or unreferenced sites. Complete source
domain, original directed segment incidence and embedding checks precede the
local constrained-Delaunay decision. Only constraints are exempt; cocircular
zero accepts either valid diagonal. The generic rational matrix does not call
native predicates or reproduce its optimized lifted determinant expression.

The shared external-domain reader was moved verbatim and remains available
through the old test module import, preserving existing benchmark callers.
Authored convex, collinear, concave, holed and wide-rational domains are
independent of the watch; transformations supplement rather than replace them.
The strategy correctly distinguishes filtered exact predicates on rational
inputs from expansion predicates on their rounded binary64 images, and does
not turn literature complexity or measured foreign throughput into a product
promise.

This is a source review. Root reports the focused 13-method fleet run GREEN;
the reviewer did not repeat geometry execution. The current semantic reader
is intentionally quadratic and bounded, and no native direct CDT constructor
has been implemented or performance-qualified by this unit. A later native
producer must feed its own actual outputs through these checks.
