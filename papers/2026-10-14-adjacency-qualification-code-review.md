# Adjacency qualification and mutation controller review

Reviewed `scripts/qualify_adjacency.py` and
`scripts/run_adjacency_mutations.py` against actual production source sites,
direct tests, reviewed native layouts and fleet-only doctrine. One concrete
mutation-controller input-pin finding must be resolved before the campaign.
No products, builds or mutants were executed in this review.

## QC1 — guard actual build and healthy-baseline dependencies

The mutation controller guards changed native/wrapper source, independent
oracle/tests and used libraries, but omits `CMakeLists.txt`,
`include/brepgpu.h` and `native/topology_test_hooks.h`. These are actual source
inputs to the isolated native compilation. It also omits
`python/brepgpu/__init__.py`, `body.py` and `body_sampling.py`, which the literal
cylinder's first healthy geometry check and wrapper import use.

Add these files to the existing guarded paths. A live change to one of them
could otherwise produce a different isolated baseline/build contract without
the controller's before/after check noticing. This extends the existing actual
input guard; it does not add a new certificate-for-certificate mechanism.

## Qualification scope accepted

Four serial subprocesses cover CPU1 with representative old body/pair/
intersection compatibility, CPU24 new adjacency and both P100s new adjacency.
Exact used old and new library paths are passed through the environment. GPU
qualification uses real Compute Sanitizer memcheck/leak-check with a nonzero
error exit code; both zero-error and zero-leak summaries are required. Failing
processes and missing unittest summaries stop publication. Actual test/skip
counts and separate test/process wall are exported, so a skip is not silently
represented as execution. Native C admission/fault/host-sanitizer campaigns
remain separate root execution scopes and must be reported separately.

## Production mutation scope accepted

The eight entries mutate real production expressions: CPU fill/neighbor,
CUDA fill, actual recursive uniform-add, neighbor, same-face classification,
sign conflict, and Python public mate identity lookup. Every before-marker
occurs exactly once in the current source (checked by static AST extraction
and text counting without importing or executing products).

Targets are direct semantic tests with independent raw/literal expectations.
The multilevel scan mutant uses the uneven65,539-edge fixture, not a tile-only
case. Its missing prefix makes offsets nonmonotonic and loses exact source
rows; the independent offset assertion should reject it. Current fill targets
remain withinC for this bounded fixture, but only real execution establishes
the fault outcome. If corrupted native reads/statuses or crashes occur, the
controller correctly refuses to credit them as assertion kills.

Each mutant first runs its healthy actual direct baseline. Isolated directories
copy real code without bytecode/detritus, replace exactly one site, and compile
the actual topology target on the fleet. The selected mutated library path is
used by the same copied direct test and independent oracle. CPU mutants use
CPU-only builds; GPU mutants use the declared CUDA compiler/toolkit. The Python
mutant uses the healthy original native binary, changing actual public lookup.

Build/configuration errors, missing markers, crashes and unqualified failures
abort. Credit requires nonzero test exit, real AssertionError and the exact
single-failure unittest summary. A runtime error in an unrelated import or
native status does not earn credit. The scope is CPU1/P1000, not a claim of
mutant execution on every backend or universal mutation coverage. Live source/
binary checks occur around each case; adding QC1 dependencies completes that
guard's intended input scope.

## Publication boundary

Logs/build trees remain ignored fleet context; public TSVs name actual tests,
source sites, backend, healthy baseline and assertion kill with duration. Keep
observed campaign counts and actual fault outcomes, rather than treating the
planned eight mutants as already killed. Root serializes build/qualification/
mutation work and prevents overlap with later measured trials. No local product
execution or second-order oracle machinery was introduced.

Disposition: accepted. Root added QC1's CMake/base ABI/hook and healthy import/
body source dependencies to the existing guard, plus the protected old CUDA
binary. Root separately reported fleet greens: CPU1's49 new/compatibility tests,
CPU24's10 new tests and10 new tests on each P100, with zero device errors/leaks.
Actual host fault/sanitizer and mutation execution remains a separate campaign;
those outcomes cannot be inferred from this static controller review.
