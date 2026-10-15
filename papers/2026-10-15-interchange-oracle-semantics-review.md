# Interchange qualification: geometry and source review

Disposition: **accepted**, no blocking finding. Source-only review of the whole
`python/brepgpu/interchange_oracles.py`, its ten direct test methods, and
`scripts/compare_interchange_reference.py`; cross-read the source converter,
actual native `bg1_convert`, native evaluator status ordering and compiler
floating-point options. No product execution or code edit performed.

The original-source oracle uses Cox–de Boor over expanded original knots and
Euclidean points multiplied by separate weights. Interior knots use their right
limit; the active upper endpoint uses its left limit. Repeated zero-width knot
intervals contribute zero. The derivative recurrence takes the lower-degree
basis; the quotient rule includes weight derivatives in both natural parameter
directions. Degree-zero axes return zero derivatives. The direct literal rational
jet and piecewise-linear internal/endpoint derivative examples independently
anchor these formulas.

Actual emitted controls use expanded Bernstein polynomials rather than the
native recurrence or converter helpers. The source inventory is independently
rebuilt from retained typed attribute arrays: all NURBS prefix offsets, original
controls/knots/weights, face metadata and original use occurrences survive.
Complete output/patch-slot bijection, full source keys, document-wide native
face/chart lineage, actual descriptor bits and actual one-round homogeneous
coefficients are checked. Positive common weight scaling and exact Cartesian
control displacement determine the coefficient bounds; these bounds cannot be
enlarged from the observed native answer.

Native result terms, extents, descriptors, placement tags, profiling semantics,
local/world query correspondence and invalid statuses/NaN fields are checked.
The caller binds returned slots and UV bits to its submitted arrays before
qualification. Source-world truth uses exact affine original U/V from those
normalized binary64 values, retaining the exact endpoint difference. The
prescribed frame factors instead use actual binary64 span subtraction/division,
matching the existing native boundary. Every converted component must equal
the independently rounded local-value times factor, followed by native +0;
the native build disables contraction. Independent product-rounding rho and
factor discrepancy complete the original-source error allowance. The non-exact
span witness and one-ULP converted-answer corruption target these distinctions.

The saved reference comparison binds original source hashes, complete face/domain
inventory, original face/query order and submitted UV bits. It evaluates original
source truth at the actual rounded U/V submitted to the foreign runtime, rather
than substituting the native exact-affine point. Their exact source-jet difference
is a separate term in the native/reference gap allowance. The reference's own
scalar error limit is explicit; the analytic sheet also checks original exported
controls against its independently intended polynomial map. Global maximum
qualified native budgets are conservative when reused for the differential
comparison.

Scope remains bounded: the original-byte-to-typed-document parser is owned and
the independent source oracle consumes that document, so same-source foreign
reading attacks shared-parser blind spots. The comparison is observed face
ordinal/domain correspondence for these fixtures, not general persistent foreign
topological naming. Sampling qualifies actual returned jets; it does not certify
all parameter values, interpret trims, import a solid or satisfy broad external
alpha suites. Selected falsified outputs exercise named failures without claiming
mutation coverage.
