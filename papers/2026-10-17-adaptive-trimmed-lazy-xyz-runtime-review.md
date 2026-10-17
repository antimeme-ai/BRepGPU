# Deferred virtual XYZ: runtime review

Read docs/adaptive-trimmed-lazy-xyz-plan.md and the current clipped-piece
preparation, base/source packers, precise piece route and executor admission.
This review owns no product changes, builds or new execution campaign.

PLAN verdict: ACCEPT after the nonfinite-routing clarification. Exact clipping
already retains barycentrics relative to the captured actual parent triangle.
Sealing those nine scalars instead of nine eagerly computed XYZ scalars preserves
the same fifteen-entry piece capacity and immutable source/view/domain/quality
capture. Both ordinary packers must use one outward interval dot in canonical
parent-corner order. The referenced Q must come from
work.facets()[piece.parent].construction_facet, since fallback merging remaps
piece.parent. The precise route alone reconstructs the exact dot in its requesting
Budget, using those same retained barycentrics and parent binary64 words.

For each exact lambda and Q, Rational::bounds encloses lambda, point(Q) is exact,
and existing outward mul/add enclose the same linear expression. Wider intervals
can alter conservative filter outcomes, but cannot accept a different L. The
existing same-source fixture's Fraction reconstruction from original UV and
parent Q should check containment of every packed virtual component for both
packers, including signed coordinates/cancellation, plus the demanded precise
path. Old eager XYZ was mathematically correct; its RED is the selected staging
contract, not evidence of an old geometric error.

One concrete plan finding was closed: current source-triangle admission rejects
nonfinite affine intervals before executor precision routing. The amended plan
therefore requires typed unresolved at the common packing helper and explicitly
does not promise automatic precise recovery. Finite nonaccept retains existing
execution; interval width never establishes an actual source-point floor.

Actual implementation verdict: ACCEPT. Read the coherent
adaptive_trimmed_geometry.hpp/.cpp and existing C++/Python work-fixture changes.
Preparation seals lambda triples in the previous nine-scalar slot. Base and
source packs share piece_xyz_intervals; canonical corner-order outward arithmetic
encloses each component without exact XYZ dot construction. Parent lookup uses
the remapped current work row and the retained immutable construction. Only
compare_trim_piece_precise calls piece_xyz_exact, with the exact same lambda/Q
expression and requesting Budget. Whole-parent pieces use direct point(Q).
Invalid interval sums throw ErrorCode::unresolved before kernel admission.

The existing Fraction reader computes weights from exact piece/parent UV
determinants independently of stored lambdas, reconstructs parent L, compares the
test-only exact expression export, and checks both base/source packed endpoint
pairs enclose that truth. Its signed original-source translation requires actual
cancelling terms. This attacks enclosure and capture directly; it is not a
regression-only comparison or a claim that the old exact geometry was wrong.
The fixture invokes the real precise piece route without supplying a numerical
answer. No acceptance theorem, kernel, device ABI, authority lease or complete
result ordering changed. No additional execution was requested by this review;
the owner records the bounded direct fixture outcome separately.
