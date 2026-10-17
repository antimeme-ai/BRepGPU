# Persistent numerical service: actual source review

Read `numerical_service.{hpp,cpp}` and the selected-range extension to the same
`CellBoundFold::add`. The existing original-root residual, analytic-cell formula
and CPU/P100 filter policies are unchanged.

One service retains one immutable root bank and one BoundRuntime. Native owner
steps prepare bounded cells in fresh scheduler scratch, retain only sealed exact
packets/folds, submit one bounded device batch and return at polls. No face Budget
or Rational crosses owners. Device state is reused between original-curve and
face-cell requests. An opened/failed session cannot silently turn into a labeled
CPU success. Cancellation uses the same scheduler token and terminal failure;
close preserves the previously reviewed exceptional drain lifetime.

The real frontier supplies geometric-priority order, not numerical key order.
Prepared cells are sorted for the runtime while retaining original response
indices. Exact tables are service-owned with monotone ordinals, separate from
the face-generation producer/seed/engine table sequence. Mixed-cell packing
obeys both table and task caps. Full headers and the caller's child request
identity survive every route. All pieces settle before an immutable cell batch
is exposed. The selected-range fold validates the entire range before changing
seen state, and retains the same duplicate/missing/full-header checks.

ACCEPT after actual-source review. Focused strict CPU and both P100 executions,
host ASan/UBSan/leaks, scoped device memcheck, actual-source lint and format are
GREEN as recorded in the owner journal. Direct cases include real curve→cell
session reuse, mixed C0 cells, analytic cells, reversed priority/key order,
stale/foreign metadata, bounded scratch, cancellation and lifetime/accounting.
This accepts the reusable numerical service, not a complete mesh or speedup.
