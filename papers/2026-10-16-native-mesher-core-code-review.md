# Original geometry and mutable topology: scoped code review

The parent reviewed the actual original-root extraction, immutable geometry
store and mutable face topology against the accepted whole-mesher design.
This is a code review of these components, not acceptance of a completed mesher.

Geometry extraction preserves original positive homogeneous coefficients and
natural knot domains. Cox-on-span conversion supports unclamped knots; rational
restriction and tensor-to-triangle conversion retain the same original function.
The store seals one exact table and indexes roots without retaining mutable
compiler budgets or one qualified reference per coefficient. Imports belong to
the destination budget. A retained store owns its source and exact-table leases.

Review found malformed public control nets could reach restriction indexing.
The owner added shape, ownership, positive-weight and range validation before
access, reproduced the failure under ASan, and passed the corrected fixture.
Review also found partial outside-domain queries silently returned clipped
coverage. Queries now require their entire closed box inside the natural domain;
outside coverage reports unsupported. Unused whole-net assignment operations
were deleted to prevent partial cross-budget assignment. These findings are
resolved. Independent original de Boor and literal weighted/unclamped/mixed
polynomial fixtures attack the numerical semantics directly.

Mutable topology owns packed adjacency and hash indexes. Exact fixed-metric
lengths select terminal-priority LEPP propagation; midpoint bisection updates
both incident triangles. Equal-longest incoming edges terminate the pair;
otherwise propagation strictly increases length. Refinement performs no global
flips or repeated initial triangulation. One heuristic attempt remains spent
along descendant ancestry.

Review found rejected heuristic insertion could leave an orphan site that later
blocked a legal midpoint. Admission now checks before publishing a site. Review
also required local removed/inserted triangle deltas so consumers need not scan
every triangle after each split. `take_changes()` now returns ordered generation-
qualified edits, including committed propagation before a boundary yield. The
owner's direct regression replays these deltas against the actual topology.
Both findings are resolved in the reviewed code.

Decision: ACCEPT these original-root/store and mutable-topology units within
their stated scope. Their independent geometry, topology and sanitizer checks
do not establish chart recovery, full refinement, complete contact epochs,
whole-watch meshing or end-to-end performance. Those remain integrated-plan work.
