# Extrusion design: independent oracle review

Reviewed `docs/EXTRUSION_DESIGN.md` against the selected scope in
`2026-10-15-extrusion-oracles-study.md`. **Accepted; no blocking finding.**
Research/design review only; no product execution or implementation.

The source polygon is independently defined by original lattice vertices and
the strict support test against every nonincident vertex. This excludes winding
stars as well as concavity and collinearity. Positive centered height gives the
complete product boundary: two whole profile domains and one complete swept
rectangle for each original edge. Exact carrier/domain/UV identities, original
source edge bijections and the explicit loop words provide the geometry proof;
counts, volume and Euler characteristic remain supporting checks.

The intrinsic parameter of each horizontal side use still follows its source
edge; the upper use reverses traversal through its separate orientation. Vertical
use axial maps use the intrinsic edge parameter. Thus the declared four-use
side word is compatible with both cap words and shared physical edge IDs.
Strict support prevents nonadjacent sides intersecting; positive height and the
complete face domains prevent accidental internal sheets. Opposed uses, one
shell and cyclic vertex links remain required independently.

The six-kind history totals agree: each of the 3N+3 source entities has two
modified cap images, giving 6N+6 modified origins; the N vertical edges, 3N
side face/chart/loop records and 4N side uses give 8N generated origins.
Centered translation justifies no retained source base. The explicit parent
keys, intrinsic affine maps and separate orientation permit complete forward/
reverse checks without borrowing foreign history semantics.

All selected control coordinates are dyadic and fit the existing control/frame
contract. The side normal formula uses original integer edge deltas; exact-square
handling and the isqrt(M<<128) interval independently enclose the true irrational
normal. Comparing a published component with both interval endpoints supplies a
valid conservative error bound. The direct native position/normal requirements
remain observed output contracts rather than a claim inferred from partial-jet
grades. Actual submitted float UVs, per-Patch frames and exact reader results
must be checked as written.

The explicit unsupported cases, factory separation, private final-validation
failure boundary and per-call native child cleanup keep rollback and ownership
claims testable. The isolated optional reference comparison supplies additional
evidence; it does not replace the product-domain proof or qualify broader alpha
coverage. No design correction requested.
