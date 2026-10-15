# Interchange design: oracle and numerics review

2026-10-15. Reviewed `docs/INTERCHANGE_DESIGN.md`, actual native `bg1_convert`,
the Python conversion reader and pinned upstream `mouse.usda`. Source-only;
no product execution or edits. Initial review required one numerical qualification
correction. **Final verdict: accepted after the documented correction below.**

## IDO1 — Converted source derivatives need the actual binary64 frame map

The design correctly separates coefficient conversion from native arithmetic,
but its qualification contract does not specify how to combine them. Source
geometry uses exact L=Fraction(Umax)−Fraction(Umin). Existing native conversion
uses d=fl(Umax−Umin), k=fl(scale/d), and a rounded multiplication. These are
potentially different even with valid, noncollapsed admitted spans. A comparison
that merely scales the local derivative grade by scale/L omits a real fault/
rounding class.

For each derivative component let b be the exact normalized derivative from the
actual emitted controls, a the returned local derivative, c the actual converted
component, g=2^-30, and B the whole-source coefficient derivative bound. A safe
acceptance budget is

`B + abs(k)*g + abs(k-scale/L)*abs(b) + product_round_bound(a*k)`.

All values in this expression except the explicit binary64 operations can be
computed as exact fractions. Bound the final product rounding independently
(e.g. a conservative relative term plus a subnormal absolute term), rather than
choosing an allowance from the observed source discrepancy. Positions have the
analogous scale/local-grade/product-rounding budget with coefficient bound
delta. Origin is fixed zero here. This keeps the native ABI unchanged.
Alternatively restrict admission to exactly representable spans, but that would
unnecessarily narrow the current proposal. Include a non-unit, non-exact endpoint
difference witness, not only spans 5 and 2.

Foreign scalar queries need a related explicit rule: if exact affine source UV
is rounded for the reference call, compare that reference result to the source
model at the **actual submitted** binary64 UV. Report its exact-coordinate
separation from the owned normalized query. Logging rounding while comparing
results at unlike source points is insufficient. No derivative precision or
parameterization promise should be inferred from the reference's success status.

## Reviewed claims without blockers

The whole-carrier position bound is valid. Exact common positive weight scaling
leaves the rational partition-of-unity coefficients unchanged, so their convex
combination of Cartesian control displacements has L-infinity norm at most
delta. Measuring displacement after actual homogeneous rounding and world scale
accounts for the complete coefficient change.

The stated derivative coefficient bound is conservative and valid. For degree p
Bernstein basis on source interval length L, the sum of absolute basis
first derivatives is at most 2p/L. With weight ratio R and denominator at least
the minimum weight, differentiating normalized rational basis coefficients bounds
their absolute derivative sum by 4pR/L. Multiplying by maximum control
displacement gives 4pR*delta/L; tensoring with the other axis preserves the sum.
Degree zero yields zero. This proves coefficient conversion error only and does
not resolve IDO1 by itself.

Exact source UV evaluation from the original endpoint ratios and actual s/t bits
is correct. The independent source Cox–de Boor/quotient model and the existing
actual-control Bernstein native oracle attack different mechanisms. Full net
correspondence provides a whole-carrier conversion argument; sampled first jets
check the actual consumer. Source/face-use/chart identity remains independent of
geometric equality, and opposite face use must not reverse intrinsic derivatives.

The lexer/typed syntax/full-file-consumption proposal is appropriate for an owned
static exporter subset. Explicit rejection of composition, transforms, unknown
syntax, malformed types/shapes and budgets closes the dangerous silent-ignore
paths. A per-face unsupported geometric token is distinct from malformed file
syntax. Trims and remaining parsed topology are retained source evidence, never
validated imported topology; tests/reports must preserve that distinction rather
than describing the entire upstream BRep as validated. File-change rejection
concerns the source-read/publication operation; subsequent external edits cannot
invalidate a correctly owned snapshot.

The real upstream mouse fixture contains four NURBS faces: source indices 1 and
2 have counts equal to orders and clamped single spans; indices 0 and 3 have
multispans. Its two orientations per face and general trims exercise retained
occurrences without manufacturing a unique outward face normal or trimmed-body
claim. The separate actual reference-generated polynomial export and independently
authored asymmetric rational fixture are complementary: neither should be
represented as an unchanged foreign corpus test. Existing reference runtime
limitations and unresolved external alpha coverage remain explicit.

## Disposition

Re-read the revised design. IDO1 is resolved: it now states the complete
coefficient/local-grade/actual-factor/final-rounding budget, computes product rho
independently from correctly rounded exact multiplication, and compares foreign
scalar results at their actual submitted rounded UVs. The source UV displacement
is separate. Shared body namespace allocation and a single capped immutable
source-byte read also make identity/read ownership explicit. Accepted for plan;
no remaining blocker in this oracle/numerics review.
