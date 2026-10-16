# Direct CDT oracle implementation plan

Status: root reviewed and accepted with the amendments below. This is the independent
semantic test lane for `DIRECT_CDT_ORACLE_DESIGN.md`, not a native implementation
or a separate large qualification framework. All geometry execution is fleet-only.

1. Build the independent output-oracle unit now, consuming actual existing
   native ear and Python Lawson outputs. A future proposed product seam is
   `brepgpu.face_domain.constrained_delaunay(loops, points, *, metric=(1.,1.), ...)`;
   no such constructor or ABI is selected or imported by this unit. The oracle
   consumes source loops, original node rows, actual positive diagonal metric,
   and actual source-ID triangles, never native adjacency flags.
2. Reuse the complete existing `qualify_external` implementation. If sharing it
   requires a neutral helper module, move that function verbatim to
   `tests/face_domain_oracle.py` and import/re-export it from its current test
   module, preserving existing benchmark imports. Do not create a second copy.
   Run the existing three external-domain methods once on the fleet after the
   move; then stop refactor verification.
3. Add `tests/direct_cdt_oracle.py`: exact metric validation, generic Fraction
   matrix determinant, and local interior-edge postcondition. It consumes the
   established complete-domain reader first, then independently reconstructs
   edge incidence from actual triangles. It never calls Python Lawson, native
   arithmetic or native topology. Errors identify source edge/triangle IDs and
   the failed obligation; no certification records or new persistence format.
4. Add a small owned deterministic case module and `tests/test_direct_cdt_oracle.py`.
   Before oracle code exists, run its focused methods to genuine missing-oracle
   RED on the fleet; retain the command and failure class in the journal. Cases
   include hand-derived legal/illegal diagonals, exact zero alternatives,
   anisotropic reversal, protected collinear sites, concave comb/corridor,
   separate multiple holes, non-dyadic/wide cancellation and all five original
   external domains. Seeded input-order/source-ID/metric-preserving transforms
   supplement the independent geometries. Bounds remain small enough for the
   complete direct embedding reader; benchmark-size generation is separate.
5. Include direct rejected-output examples at the same semantic seam: a strict
   illegal diagonal whose domain coverage passes, missing protected collinear
   sample and crossing/overlap. They establish sensitivity of the direct reader
   to different actual geometry errors, without a tests-for-tests campaign.
   Both exact-zero square triangulations must pass the same semantic contract.
6. Run actual native ear outputs through existing Python Lawson, then qualify
   the result on the fleet for all small authored and original external cases.
   Demonstrate an actual valid-domain native ear result is rejected as CDT
   illegal and the existing Lawson result passes. Run affected external-domain
   methods after sharing their reader. No new native constructor is required
   to complete this unit. Python parity or a matching foreign output cannot
   turn a semantic failure green. Independent review
   attacks determinant sign, metric interpretation, constraint exemptions,
   source incidence and cocircular policy; fix findings and rerun affected cases.

Root review additionally requires original node-row admission before any dict
or set collapse: unique source IDs, distinct exact UVs, finite two-dimensional
values, and exactly one loop occurrence of every supplied site. Apply these
guards before the shared domain reader; malformed original rows are not silently
repaired. The future product test file `tests/test_direct_cdt.py` stays reserved.

Benchmarks should use the same case construction functions when useful but do
not time this exact Python reader as native solve. Record product preparation,
construction, output, qualification and whole-process cost separately. Full-size
embedding qualification needs a separately reviewed efficient reader if its
quadratic direct checks become material; do not silently skip it or advertise
only bounded local checks as full embedding qualification. Do not run the whole
watch as the first correctness experiment or derive dispatch from its IDs/counts.
