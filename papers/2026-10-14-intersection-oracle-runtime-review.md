# Intersection oracle: actual-source and runtime review

2026-10-14. Accepted after IR-O1/IR-O2 fixes. Read the complete independent
intersection oracle and direct tests, actual body oracle control/frame binding,
consumer source-artifact layout and accepted exact construction/consumer plan.
Static review only: no product imports/native execution or new qualification.

## IR-O1 — actual prepared-input binding: resolved

The initial batch qualifier proved each supplied row and native source artifact,
but could not detect duplicated/reordered otherwise valid samples without the
actual request. The corrected optional request argument binds exact result
identity, count, candidate order/IDs, exact parameter/location/reference and
prepared exact UVs. Original-source map evaluation still determines geometric
truth independently. Actual caller/benchmark and sampling tests pass their
request. The one-argument form explicitly makes only per-submitted-row claims.
This is actual-input binding, not trusting the product request as model truth.

## IR-O2 — ordered physical spans: resolved

Initial graph checks used component piece membership as a set; reordered CCW or
increasing-height traversal could pass. The corrected qualifier collapses only
adjacent chart occurrences of one span, rejects later revisits, checks oriented
hi→next lo event continuity and closes the final→first link for closed curves.
Local quarter UV/world-height identities establish direction. Duplicate generator
occurrences survive while physical graph degree uses each span once.

## Independent numerical and geometric mechanisms

The oracle reads raw a/b/d fields rather than calling product Scalar operations,
rounding or map evaluation. Single-field signs and cross-field comparison use
integer-square-root rational interval refinement, distinct from product closed
sign formulas. Related fields are recognized through reduced rational radicand
ratios. Rational rounding uses Fraction conversion; irrational rounding narrows
independent exact intervals until both endpoints round identically. An irrational
value cannot equal a rational binary64 midpoint, so this converges in the bounded
consumer domain. The same supported RN/gradual-underflow environment applies.

Whole source maps are independently composed from the actual binary homogeneous
control nets using expanded Bernstein polynomials in submitted UV-map numerators
and denominator. World origin/scale come from original patches. Polynomial
cross-products prove equality to the submitted spatial map over whole pieces.
Existing body truth independently binds those actual controls, frames, lineage
and source rectangles to canonical factory roles; no CPU/CUDA parity is truth.

Normalized UV and source trim inequalities use exact polynomial extrema and
endpoints. Degree-four circle expressions divide proved-positive 1+t² factors
exactly. Positive denominator checks, interior admitted trim events, whole-span
boundary tags, endpoint constraints and atlas joins are explicit. Exact event
positions, every incidence and grouped span maps/ends are checked. Generic
validity does not claim unknown-configuration component completeness; literal
recipes/count/endpoints separately cover the stated complete configurations.

## Actual native metadata and output

The independently packed descriptor layout `<4I6Q2I8d` is 136 bytes and matches
the public ABI fields in order. It includes compact control offset/count, degree,
lineage flags/identities, source rectangle and world frame. Expected descriptor
bytes bind actual returned bytes to original patches, not reconstructed metadata
presented as actual. Native slots and UV bytes have exact 2N/4N extents; interleaved
row indices bind each original side/slot to its dense descriptor. Independent
nearest UVs are compared bitwise to row values and actual native UV artifacts.

Each finite actual left/right XYZ is graded with exact Fraction/independent
algebraic arithmetic against the original spatial map at the exact requested
parameter, with an exact pairwise difference check at 2^-36. Row statuses/counts,
request/result identity, span/piece IDs and empty no-query artifacts are explicit.
Host provenance is not extra device jet traffic. The oracle does not infer exact
boundary labels from rounded UVs.

## Direct tests and failures

Literal tests cover full circle/cardinals, clipped endpoints/source swap, hole
exclusion/coincident boundary, disk lens/external tangent point/internal tangent
event, cardinal and irrational generators, tangent carrier, singleton height,
subnormal branch decisions, exact float ties/normal-subnormal transition and
unsupported pairs. The first construction/consumer tests require healthy actual
input objects before the missing-operation boundary; their red/green outcomes
remain coordinator-reported.

Sampling faults wrap real native query/result resources, perform original closes
before raising SystemExit/KeyboardInterrupt and assert exact primary-object
identity, both handles consumed and healthy retry. Returned UV and position
buffer corruption must raise the promised whole-operation failures. Scoped
candidate/refinement-cap refusal leaves exact geometry/default preparation
usable; it does not claim that the default 4096 cap was reached. No fabricated
model answer or import/crash failure is credited as a geometry assertion.

No remaining oracle/runtime blocker found. Actual fleet executions, production
mutants and serial performance qualification are coordinator-owned.

Reviewed source hashes:

- `python/brepgpu/intersection_oracles.py`: `24c61b425bdcb82e4e5cb34399049547e3ad744b6d17f70379d3b0ebf1c5c84a`.
- `tests/test_intersections.py`: `161924cc84fe94c3604751be942b0a3720f52e824d0dd9f00e9033a454bdfb52`.
