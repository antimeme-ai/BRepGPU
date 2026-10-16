# Direct CDT oracle design

The operator clarified that performance ambition means reaching for strong
algorithms from the literature before measurements, then resumed work. Root
delegated the independent semantic oracle/reference contract as one unit.

Read current domain/source readers, external fixture adapters and embedding
reader, imported-meshing Lawson implementation and its focused tests. Read
primary CGAL/Triangle documentation and Shewchuk's local Delaunay discussion.
Saved `docs/DIRECT_CDT_ORACLE_DESIGN.md` for review; no product/test code or
geometry execution occurred.

The substantive missing check is exact local constrained-Delaunay legality of
actual output edges. Proposed a generic Fraction 4x4 elimination determinant on
original metric-space coordinates, independent of native normalized bigint and
cached lifts. Existing boundary-chain/incidence/embedding mechanisms remain the
single geometry reader. Exact cocircular alternatives are accepted; source-ID
and protected collinear segment preservation are mandatory. Native flippable
flags do not excuse a positive local violation.

Predetermined independent workload families and reference-input comparison
rules prevent the watch from becoming the specification. No new dependency,
alpha corpus reduction, universal complexity claim or performance guarantee is
implied. Root reviewed design and plan ACCEPT, with two amendments: implement
the missing oracle unit against actual existing native ear/Python Lawson
consumers now, and validate original node rows before dictionary/set collapse.
The future proposed product entry is `face_domain.constrained_delaunay`; no new
constructor or ABI is implemented, selected or imported by these tests.

## Focused implementation and fleet results

Authored `tests/test_direct_cdt_oracle.py` before the new oracle module and ran:

```
PYTHONPATH=python:tests python3 -B -m unittest -v test_direct_cdt_oracle
```

The isolated fleet checkout failed at the intended missing capability:
`ModuleNotFoundError: No module named 'direct_cdt_oracle'` (exit 1). This is
missing-entry RED, not a claim of numerical fault sensitivity.

Implemented the full rational-matrix local test and original-row admission in
`tests/direct_cdt_oracle.py`. Moved the existing complete domain reader verbatim
to `tests/face_domain_oracle.py` and re-exported it from the old test module so
existing benchmark imports remain valid. Added seven independently authored
bounded families in `tests/direct_cdt_cases.py`, selected before native CDT
tuning. The full design names additional large/adverse benchmark families; the
bounded semantic implementation does not claim that whole performance cohort.

Ran on the isolated fleet checkout with the existing native face-domain library:

```
PYTHONPATH=python:tests python3 -B -m unittest -v test_direct_cdt_oracle test_face_domain_external
```

Result: **13 methods passed in 3.494s** (10 focused oracle methods and 3 affected
external-domain methods). Actual native ear output of a four-site domain passes
coverage but is rejected for a strict local-circle violation. The actual existing
Python Lawson result passes. Both exact cocircular square diagonals pass; the
hand-derived anisotropic reversal rejects the opposite diagonal under each
metric. Protected collinear source omission, unknown/collapsed source triangles,
malformed original row inventory and nonfinite/invalid metric fail explicitly.

All seven authored families passed on actual native ear→existing Python Lawson
outputs under isotropic and anisotropic metrics. All five retained original
NVIDIA/Manifold domains passed after the same consumer. All twelve cases also
passed after exact rational translation/180-degree rotation, reversed node
insertion, high/zero original-ID bijection, cyclic loop rotation and hole-order
reversal. One exact compensated power-of-two axis/metric transform preserves the
same semantic statistics. These are bounded complete-domain checks; no large
watch rerun, new native constructor or performance claim is implied.

Independent source-inventory/embedding review found Python hash/equality could
accept output `0.0` or `False` as the original integer ID0, and bool original
IDs also passed `isinstance(int)`. Fixed the reader to require actual Python
integer values in uint64 range for original rows, loop slots and output IDs
before shared geometry qualification. Native output already consists of ints;
this is reader-contract hardening, not a product defect. Added direct aliased-ID
rejections to the existing methods. The two affected fleet methods passed in
0.002s. No broader rerun is needed for this source-ID guard.

Independent determinant/numerical review subsequently ACCEPTED the generic
elimination sign, metric exactness, constraint exemptions, zero policy and shared
reader reuse. Its scoped review journal is
`journal/2026-10-16-direct-cdt-oracle-code-review.md`. Root inspected the later
strict-ID guard and affected test results; no extra geometry run was warranted.
