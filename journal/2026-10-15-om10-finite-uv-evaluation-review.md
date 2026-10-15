# Finite native UV evaluation recovery review

Independent design/subplan review of the native evaluation follow-up in
`papers/2026-10-15-om10-explicit-uv-source-preparation.md` and its source subplan.

The actual 170932 failure is distinct from source-chart deformation: all 25
active V coefficients equal the original nonbinary global boundary, while native
Value(t) escapes by one ULP. The selected correction admits only a finite
observed escape within the existing 128 ULP global-domain rule, after directly
checking the detached active nonrational/nonperiodic B-spline coefficient hull
against the original rectangle. This is not a global error theorem for the
foreign evaluator. Outside coefficients, beyond-grade evaluations, changed
original t, or unsupported bases do not gain admission.

Design and subplan ACCEPT. The shared finite checker must serve selected conic
correspondence and final query correspondence, preserve stored curves and exact
original t/IDs, and evaluate the unchanged source C/S grade at recovered in-domain
UV. Existing raw-guide extension diagnostics remain named as such. No source or
query coefficient charge is added for an evaluation performed only by this
checker. Later actual owned meshing still performs its own finite UV recovery,
residual checks and exact-to-binary64 corner conversion accounting; those are not
replaced or established by this source checker.

Review basis: inspected `native/source_uv/source_uv.cpp` existing correspondence,
conic selection and cumulative QueryBudget; inspected owned
`imported_meshing._recover_global_uv`, its actual recovery shift reporting and
`_uv_rounding_bound`. A genuine literal native constant-coordinate RED precedes
production. Bounded code review and actual complete source/query conversion
remain pending; this acceptance says nothing about whole-watch mesh or images.
