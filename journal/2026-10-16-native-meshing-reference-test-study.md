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

Performance plan starts with the actual saved 8,300-point boundary before another
whole watch, compares exact same input/output/grade and measured candidates,
predicates, ear visits, time and memory on one physical core; later complete
main-plate and fixed 1/12/24-worker runs keep output scope and denominator fixed.
Sparse and dense synthetic families distinguish successful candidate pruning
from a faster quadratic transcription. Timeouts remain censored failures, with
no inferred speedup. Current NVIDIA spring/spiral failures cannot be accepted
performance denominators. Root owns design selection/review and implementation.
