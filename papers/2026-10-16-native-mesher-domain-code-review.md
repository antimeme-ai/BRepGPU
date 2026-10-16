# Indexed original-domain admission and typed CDT review

The parent reviewed actual `domain.cpp` and its same-owner topology interface.
The bridge admits complete original loop inventories, records traversal adapters
without mutating source use order, constructs the initial full-metric CDT and
protects constraints in the same mutable topology used by refinement.

Closed outward segment boxes drive candidate discovery; exact orientations and
coordinate comparisons reject crossings, adjacent backtracks and nonincident
touches. Every site belongs to one initial cycle. With nonintersecting simple
boundaries, one half-open winding query per hole over all other cycles rejects
outside and nested holes. The implementation normalizes the outer CCW and holes
CW, then requires winding one after subtracting the queried hole's own boundary.

Wholly right BVH nodes aggregate directed crossing events by exact Y order.
Grouped equal-Y deltas and half-open prefix lookup preserve endpoint semantics;
straddling nodes retain exact segment predicates. Child event lists merge by
index rather than retained references across vector growth. This avoids the
unconditional quadratic hole-pair scan, including the authored aligned-hole
family. It is not a universal logarithmic query-time claim.

Both transformed metric axes share one dyadic scale after a common translation.
Independent axis scaling would change Delaunay predicates; this code does not do
that. Typed in-memory descriptors call the owned direct-CDT core, bypassing the
old serialized face-domain wrapper. Temporary admission/predicate storage dies
before mutable triangle population. Failure discards the incomplete generation.

The existing independent Fraction CDT reader checks actual emitted triangles
against independently transformed original points for coverage, embedding and
the full metric. Native literal cases cover source ordering, collinear constraint
preservation, invalid contacts, ownership and shared-Y aligned holes. Strict,
ASan/UBSan and actual-source lint checks pass.

Decision: ACCEPT the indexed ordinary-domain/initial-CDT bridge. It does not
establish source trace recovery, curved-domain displacement or completed adaptive
meshing; those are the remaining consumers in the whole implementation plan.
