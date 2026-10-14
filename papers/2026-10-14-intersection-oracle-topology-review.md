# Intersection oracle: topology and completeness review

2026-10-14. Accepted after the scoped preparation-budget additions and the
ordered-span correction. Independent source review of
`python/brepgpu/intersection_oracles.py` and `tests/test_intersections.py`;
no product imports, tests, native execution or performance work.

The oracle reads raw exact coefficients and original homogeneous controls.
Its integer-square-root interval ordering differs from the construction's
algebraic sign formulas. Expanded original Bernstein maps, world frames and
polynomial cross-products establish whole-piece spatial/UV correspondence.
Exact extrema establish closed UV and trim inequalities; sampled positions
and CPU/CUDA parity do not establish them. Prepared candidates bind actual
row coverage without supplying the geometric answer.

Physical span identity and chart occurrences remain distinct. Duplicate
generator occurrences must have the same spatial map, parameter domain and
endpoint events. Their original slots and UVs remain independently checked.
Degree and connectedness count each span once; every occurrence contributes
its own event incidence. The revised ordered traversal collapses only adjacent
occurrences of one span, rejects later revisits and checks oriented endpoint
continuity, including the closing link. Cardinal seams therefore retain two
chart uses without creating two physical graph edges.

All occurrence endpoints are represented. Exact endpoint positions, complete
active constraint labels and atlas joins are checked. Interior admitted trim
roots must remain piece boundaries. Open path endpoints and isolated points
require an actual source boundary constraint, preventing arbitrary interior
truncation from satisfying generic validity. Whole-piece boundary coincidences
remain distinct from isolated active events.

Generic validity deliberately does not assert completeness for arbitrary
configurations. Literal fixtures add known component, span, occurrence and
event counts: full and four-arc circles, hole exclusion and coincidence, disk
lens and external tangent point, retained internal tangency, cardinal and
irrational generators, tangent carrier and singleton height. Subnormal frame
offsets distinguish positive, zero and negative discriminants. Exact scalar
tests cover related radical fields, nearest ties and normal/subnormal rounding.
The literal recipe count helper reads source frames and exact endpoint answers
without recursively running the generic qualifier.

Two preparation-budget coverage gaps were reported and resolved in
`test_preparation_caps_refuse_without_invalidating_exact_result`. The real
eight-candidate circle refuses when the candidate cap is scoped to seven,
then default preparation recovers. A supported subnormal-offset plate produces
a narrow exact arc whose preparation refuses with a one-subdivision budget;
default preparation and ordinary circle preparation remain usable. This checks
the actual failure path and preservation of exact results. It does not claim
that the default 4096-subdivision cap is reached.

Actual query/result cleanup and output-corruption tests attack resource and
native-data failures directly. The peer's runtime review covers descriptor
bytes and request binding in detail; this review found no additional blocker.
No execution outcome is inferred from source acceptance. The coordinator owns
fleet execution and production mutation results.

Reviewed SHA-256:

- `python/brepgpu/intersection_oracles.py`: `24c61b425bdcb82e4e5cb34399049547e3ad744b6d17f70379d3b0ebf1c5c84a`.
- `tests/test_intersections.py`: `161924cc84fe94c3604751be942b0a3720f52e824d0dd9f00e9033a454bdfb52`.
