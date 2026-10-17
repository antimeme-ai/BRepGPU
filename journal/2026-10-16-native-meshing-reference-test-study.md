# Native sampled-face reference tests and performance study

Operator requested external NVIDIA/reference cases and rigorous performance
testing while beginning the C++ transition. Read the workspace doctrine, repo
instructions, alpha qualification contract, actual pinned NVIDIA test sources,
Manifold polygon corpus/test/fuzz readers, OCCT mesh regressions and Geogram CDT
runner and Robot wrappers. Read the actual license metadata; no new library or
reference runner adopted. No product edits, builds, tests or benchmarks executed.

Written study/sub-plan and concise case inventory live in
`papers/2026-10-16-native-meshing-reference-test-plan.md` and
`papers/2026-10-16-native-meshing-reference-cases.tsv`. The useful first external
cohort is NVIDIA's complete planar L/triangle/circle trimming recipe and
Manifold SimpleHole/Eberly/Concave2/AlignedHoles, through owned exact sampled-domain
adapters and an independent actual-output reader. Source hashes/revisions and
oracle limitations are explicit. Eberly's corpus case matches an existing owned
counterexample under translation, so it is not new independent family coverage.

Found NVIDIA TestSmPolygons to be a success-only placeholder. Actual tessellation
tests do contain valuable corner-normal/source-label regressions. Validator and
round-trip tests have structural/persistence scope. Found some OCCT regression
data absent, but terrain.brep present; Geogram's CDT data absent. No missing case
is marked passing and no development cohort replaces the alpha denominator.

Performance plan starts with reproducing/retaining the actual 8,300-point boundary before another
whole watch, compares exact same input/output/grade and measured candidates,
predicates, ear visits, time and memory on one physical core; later complete
main-plate and fixed 1/12/24-worker runs keep output scope and denominator fixed.
Sparse and dense synthetic families distinguish successful candidate pruning
from a faster quadratic transcription. Timeouts remain censored failures, with
no inferred speedup. Current NVIDIA spring/spiral failures cannot be accepted
performance denominators. Root owns design selection/review and implementation.

After independent design/plan acceptance and the operator's implementation GO,
authored `tests/face_domain_external_cases.py`,
`tests/test_face_domain_external.py` and `tests/fixtures/face-domain/`.
The original 44KB Manifold corpus bytes and Apache-2.0 license are retained;
the complete NVIDIA literal planar recipe has source-file hash, attribution
and a declared 32-point exact rational circle schedule. These are test adapters,
not adopted production algorithms. Tests use the existing exact positive-boundary
reader and direct source-ID edge incidence/embedding checks on native output.
Hole triangle ID parity is not required after the design's exact bridge-ranking
change. Rational translated/rotated source-ID-remapped derivatives are named
separately from the original cases.

Actual initial seam RED on the clean fleet checkout:
`PYTHONPATH=python:tests python3 -B -m unittest -v test_face_domain_external`.
Import failed with `ModuleNotFoundError: No module named 'brepgpu.face_domain'`,
one loader error, exit 1. This establishes only missing capability at the native
seam, not sensitivity to a numerical or geometric defect. Native implementation
is owned by the separate owner; actual-output GREEN is pending its build.

After the owner supplied the actual strict debug library, ran the same focused
external suite on the isolated fleet checkout with its library path selected.
All three tests passed in 1.597 s: five original external domains, five exact
rotated/rational-translated/source-ID-remapped derivatives, and the complete
NVIDIA circle recipe/schedule check. These inspect actual native triangles with
the independent positive boundary chain, directed original source-ID incidence,
no new/omitted IDs, exact crossings/T-junctions/overlap and strict containment
checks. This is bounded external sampled-domain evidence, not a full upstream
suite pass, analytic circle chord-grade admission or alpha qualification.
