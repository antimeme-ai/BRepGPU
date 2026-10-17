# Actual-facet contacts and mutable broadphase

Implemented the contact dependency of the accepted whole-mesher topology plan,
not a completed face engine. `native/mesher/contact.hpp/.cpp` consumes the shared
typed `NodeSlot`/`FacetHandle` model and actual emitted binary64 XYZ. It performs
exact rational triangle-plane cuts or coplanar convex clipping and classifies
the full intersection as empty, point, segment or area. Permission comes only
from the explicitly declared shared node/edge simplex; shared endpoints do not
excuse additional crossing or overlap. Equal coordinates with different node
identities remain distinct, and repeated shared node identities must have
identical XYZ words. Facet degeneracy and inconsistent declarations fail.

The same owner implements packed BVH nodes/records, four-facet leaves, a flat
facet-slot index, 16-bin SAH construction with exact comparison fallback,
deterministic centroid-median fallback, local insertion/removal/refit and
bounded-height/changed-leaf rebuilds. Queries return closed-box candidates in
canonical handle order. Interval SAH/bin decisions are filters for the exact
decision, guarded by the actual floating-point environment. All allocations and
structural work use the owning budget. Mutable failures poison the index rather
than expose partially repaired connectivity.

Authored the native fixture first and observed actual missing-header RED on the
isolated fleet checkout. Strict Clang18 execution and ASan/UBSan/LSan then passed:
literal noncoplanar/coplanar intersections, legal shared point/edge, extra
intersection beyond the declared shared simplex, inconsistent shared XYZ,
foreign-node coincidence/T-junction, containment and transformed corner/axis
orders. Eighty broadphase boxes are checked against an independent all-pairs
closed-box specification, then checked again after twenty erase/reinsert edits;
stale generations are refused. The emitted counter was 4,270 broadphase box
tests, recorded descriptively without an arbitrary speed threshold.

Actual-source lint identified a symmetric coplanar pair signature and two
different integer roles in the recursive builder. Replaced these with named
pair/position aggregates. Actual-source engine/fixture lint passes; independent
code review is in progress.

Root correctly required the filtered narrow phase in this unit, rather than
deferring it as integration polish: exact Rational imports for every adjacent
pair would preserve the old cost pattern. Added immutable per-revision
`PreparedContactFacet` edge/normal enclosures; the BVH box can consume the same
preparation. Pair classification first uses certified strict plane separation,
declared shared-edge noncoplanarity, shared-vertex strict-sidedness, exact-zero
coplanarity plus projected separating edges/opposite-sided shared edges, then
the nine edge-cross-edge SAT axes. Cheap adjacency cases precede general SAT.
Only unresolved pairs import Rational coordinates. Non-nearest or FTZ/DAZ
floating-point state disables these filters; overflow or inconclusive intervals
never imply separation or coplanarity.

Authored the new preparation/stats consumer first, observed its real missing
API RED, then implemented it. Actual strict execution and sanitizer execution
pass with six literal filtered pairs and four exact fallbacks. The latter
include a shared vertex with an additional segment crossing, non-nearest
rounding, underflowed normals at scale1e-200 and overflowed normals at scale1e200.
The SAT case has straddled planes and overlapping boxes, so it cannot be passed
by plane separation alone. Twenty-four identical boxes at x=1e308 exercise
overflowing doubled centroids, median fallback, last-leaf removal and rebuilding
an empty root. Actual-source engine/fixture lint passes; independent review
follows, with the root-owned CMake run covering the final aggregate signature.
Full source/face/contact-epoch integration is still subsequent work.

Root's substantive review strengthened the projected filter: strict 2D
projection separation and opposite projected third vertices on a declared
common edge do not require 3D coplanarity. Removed the unnecessary exact-zero
plane gate and renamed the route `filtered_projected`. Inclined planar adjacent
and disjoint facets, plus a genuinely noncoplanar one-ulp third-point
perturbation, now exercise that route directly. The actual strict result reports
nine filtered pairs and four exact fallbacks. The exact classifier/BVH source
review found no geometric defect in its scoped claims; the final CMake check
will cover this review change.
