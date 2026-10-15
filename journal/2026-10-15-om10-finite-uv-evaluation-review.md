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

## Code review

Reviewed the implementation after genuine authored RED commit `db1416c`.
Code ACCEPT: `spline_probe` rejects nonfinite evaluation; recovery requires a
nonrational, nonperiodic B-spline, a detached copy segmented to the exact saved
interval, and finite control coordinates exactly inside the original carrier
rectangle. Each escaped finite coordinate retains the existing 128 ULP gate.
Only a local UV value changes. No stored pcurve, knot, original range, source ID,
source tree or query tree mutation occurs in this helper.

Selected conic endpoints and original-per-span finite probes, plus final query
correspondence, share the helper. The original unconstrained guide continues to
feed the explicitly named extension diagnostic. Cumulative coefficient change
accounting is unchanged. The new authored fixture checks literal query controls
and original spatial LINE values, preserves raw nonbinary V, and requires an
actual native constant-coordinate escape so an insensitive fixture cannot pass.
Existing beyond-grade coefficient negatives remain in the focused run. No
blocking findings. This is static code acceptance; the owner/root run the
already planned focused fleet checks and bounded sanitizer case before the next
complete source/query attempt. No broader test campaign is requested.
