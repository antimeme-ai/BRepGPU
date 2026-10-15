# Arrangement oracle and test runtime review

2026-10-14. Static review of `arrangement_oracles.py` and the ten methods in
`tests/test_arrangements.py`; no product imports or execution locally.

Accepted for fleet qualification. Expected geometry comes from original
homogeneous chart controls, exact frame values, original boundary curves/UV
maps and original cylinder radius/center. The oracle does not use product
rectangle/circle construction helpers, private caches, emitted area totals or
actual reader outputs as expected truth. The existing intersection oracle owns
the complete retained section/map identities; the new model independently
derives target UV disks, source-side binding, orientation and cell ownership.

The region check enumerates every cell-minus-disks and every source disk, then
matches complete directed outer/hole words. It proves unique vertex/edge source
coverage, original directed boundary parameters, every twin pair and ordered
next/previous walk, exactly-once loop/use ownership, strict witnesses and exact
rational-plus-pi areas. It does not substitute Euler equality for partition
truth. Source-piece readers are checked against independently evaluated raw
rational coefficients at endpoints, joins and interior values. Closed point
location comes from an independent inclusive domain scan, which also attacks
the product's bisection/per-cell lookup through a different algorithm.

Tests directly check bare graphs, degree-four crossings and degree-three target
boundary intersections; every owned original rectangular face and both source
parameter directions; one physical full-circle edge, actual world/both-UV
reader values and original operand reversal; two holes in one cell and mixed
cell/disk ownership; bottom charts and translated/scaled original sources;
section permutation; subnormal-width cells and one-ULP strict separation;
64-cut admission; touching, overlapping, duplicated, nested, clipped, empty,
wrong-source and unsupported-face sections; bounded infinite and failed input
iterators; retained immutable records and invalid reader values.

One transient test finding was identified: using an IntersectionSet as a
dictionary key recursively hashes deliberately unhashable Scalar coefficients.
The owner had corrected it before fleet readiness. Final inspection accepts
the original left/right body/revision/face tuple key used to compare section
permutations. No remaining source finding. Actual fleet greens, focused
assertion-sensitive production mutants and saved performance artifacts remain
separate execution evidence; this source review makes no claim they ran.
