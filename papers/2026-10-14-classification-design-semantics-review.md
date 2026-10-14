# Classification design: material and contact semantics review

2026-10-14. Read `docs/CLASSIFICATION_DESIGN.md` against the completed semantics
study and owned source geometry. Accepted with no remaining semantic finding.
No product code or execution in this review.

The narrow family is complete and useful: exact rational point membership in
all three owned solids and exact closed rectangle/rectangle face contact. It
does not claim smooth curved tangency, arbitrary trimmed overlap, solid-solid
penetration or general algebraic input. Unsupported face kinds must fail, not
become disjoint. Invalid point coordinates must fail rather than become exterior.

Closed material uses feasibility before equality witnesses. This handles the
hole void, extended cap/wall carriers, all incident corner/rim faces and the
cavity's reversed material sense. Original face IDs belong to the retained
snapshot; the atlas seam cannot manufacture another boundary face. Exact
world/local conversion preserves subnormal offsets rather than rounding them
onto a boundary.

The three interval intersection describes the COMPLETE finite rectangle locus.
Its dimension and convex-hull vertices are explicit. Coplanar positive area is
overlap except for equality of complete source domains; opposing normals do
not change geometric coincidence or merge source identities. Empty/transverse
results have `same_domain=False`, as clarified by the runtime design review.

Transverse positive-length crossing requires strict relative-interior membership
of both source rectangles at the exact midpoint. This condition is complete for
the stated axis-aligned affine family: constant in-plane coordinates either
stay strictly interior or on boundary throughout, and the midpoint of positive
interval overlap is strictly inside each variable interval. The criterion is
not a generic curve sampling test. A boundary-confined segment and a singleton
are touches even though the infinite planes are transverse.

Segment endpoints are ordered in world space; area corners are positive in the
LEFT parameter chart, independent of outward orientation and future Boolean
coedge orientation. World vertex sets remain invariant under operand swap,
while both original UV representations swap with their source refs and area
sequence may change. Normals derive from oriented original U×V, preventing
front/back or left/right sign mistakes. The independent polygon/support oracle
and original-control UV reproduction attack these decisions without copying
the product interval algorithm.

Current immutable research interfaces do not settle Python's future shipping
role, native/GPU placement or integration tuning. Those operator-deferred topics
remain outside this design. Ready for the accepted bounded implementation plan.
