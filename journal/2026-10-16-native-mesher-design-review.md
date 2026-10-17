# Whole mesher design under adversarial review

Research commit `0d20314` precedes the proposed whole-system design in
`docs/NATIVE_MESHER_DESIGN.md`. Three independent reviewers inspect algorithms,
geometry/source meaning, and native numerical/runtime ownership. No product or
test implementation has started. This is the requested design-first delivery,
not another sequence of old-mesher patches.

The geometry review found that common spatial endpoints do not close UV loops
when uses independently select different projections, and that analytic
angle/height inversion is not a fixed linear-projection inverse. The selected
resolution removes an unnecessary implicit-coordinate subsystem: reconstructed
uses explicitly store common dyadic face-vertex joints and finite polygonal UV
traces, accepted by whole-interval original-geometry bounds and exact domain/
branch checks. Original authoritative curved domains still require bounded
strip/star correspondence. They are not redefined as polygons. Original source
records, physical vertices/curves and occurrence identity remain unchanged.

The contact review also exposed an overclaim: an embedded bounded approximation
does not prove that every original self-intersection smaller than epsilon was
detected. The design now promises the actual embedded bounded realization and
current final contact decisions, without claiming that stronger input theorem.

The runtime review found implicit roots could not enter rational predicates,
CPU/P100 interval looseness could otherwise affect topology, and nested workers
could exhaust both execution and memory capacity while waiting for children.
The selected design uses explicit represented chart sites, two bounded rational
residual task kinds with a common exact comparison/host fallback, immutable
compiled roots, owner-local restrictions and yielding/reserved child work.
Canonical host sites/XYZ/priority decisions keep successful geometry independent
of backend completion order. Runtime and geometry rereviews accept these changes.

The algorithm review found missing optimizer potential, unconstructed C0/atlas
cut graphs and an incomplete refinement progress argument. Internal C0 knots
now enter actual-cell span clipping/bounds rather than a promised exact crease
chain. Original B-rep edges remain conforming. Initial chart admission uses
existing finite charts, explicit seams and implemented pole/apex quotients;
general new atlases are explicit unsupported work, not invented cuts. Positional
discontinuities need rejection or established matching one-sided limits.

The refinement resolution is still being completed: fixed source-derived metric
connectivity/LEPP refinement, bounded heuristic attempts inherited down cell
lineage, then a separate fixed-site optimizer after bounds/contacts settle.
Original boundary splits restart incident face generations because a recovered
source-curve node generally is not collinear with its previous UV chord.
No shrinking-cell proof may silently treat that operation as an interior edge
midpoint, and no optimizer may undo the proof's refinement forest.

Nonlinear planar parameterization and a rotated ruled ridge remain mandatory
general correctness/performance families before watch tuning. Geometric-line
correspondences must prevent parameter-matched edge bounds from erasing the
plane-chart benefit. Metric names alone do not establish anisotropic efficiency;
the actual output density/work across a curvature sweep must decide it.

The algorithm rereview now accepts the actual integrated design. It fixes the
dyadic source-derived metric, one exact metric circumcenter proposal with the
target/unspent-neighbor restriction, terminal-priority LEPP, sticky lineage and
the separate fixed-site optimizer's exact rational once-rounded potential.
All three scopes accept the whole design for implementation planning. The
finite physical-plane profile proves onto coverage of a convex projected
quadrilateral and checks whole-carrier height, including interior bubbles;
reconstructed domains are chosen directly in that working chart. Checked
monotone correspondences prevent false same-parameter error claims.

Implementation planning follows these resolved reviews. Delivery still requires the integrated source-to-mesh
operation and actual consumer, not merely the accepted document.
