# Pair compatibility and production mutation tooling review

2026-10-14. Accepted after PT-R1/PT-R2 corrections. Read-only review of
[check_pair_compatibility.py](../scripts/check_pair_compatibility.py) and
[run_pair_mutations.py](../scripts/run_pair_mutations.py), with the corresponding
direct test methods and production mutation anchors. No execution or source
edits by this review lane.

Reviewed script SHA-256: compatibility
`26e0a0de715e40686a48aca82f2dbde2bd08eccc4b46299469a1ee1823bd0f7d`;
mutations `ff5c2236921c1602bfb12526c5897250fb43c33c3d9c9689da3be200af95d3c2`.

## PT-R1 — identify bytes used by trials: resolved

The first source hashed compatibility inputs only after calls, and did not
check stable mutation qualification inputs. The corrected compatibility runner
pins adapter/core/plugin hashes before each fresh subprocess, checks them after,
and writes those original identities. The mutation runner pins its baseline
core, direct tests, wrapper, oracle, scene, shared predicate and runtime include
before the campaign, checks them before publishing results, and identifies each
mutant's changed source and actual used core. No authoritative source/library
is modified. This closes the experiment-record omission.

## PT-R2 — exclude errors from mutation credit: resolved

The earlier loose FAILED(failures=...) substring could also admit a unittest
summary containing errors. The corrected verdict requires an AssertionError and
an exact failure-only summary; import/runtime errors therefore do not qualify
as direct assertion kills. Configuration/build failures abort earlier and are
never credited. This is a bounded verdict correction, not a test framework.

## Compatibility truth and failure behavior

Eight fresh-process cases use the selected actual prior core/plugin/adapter
files: new adapter with old core on CPU and both GPUs; old adapter with new
libraries on those backends; new core with old plugin on both GPUs. The worker
loads its selected adapter directly. A dyadic bilinear patch supplies an explicit
18-component analytic six-jet vector and successful query status; two calls
check 36 components, with the pair refusal between calls where applicable.
Expected geometry is not CPU parity. Unexpected pair acceptance or a different
native error fails the case; old operations must remain usable after refusal.
The current point-mask branch also has direct closed-box truth when selected.

Binary paths and raw subprocess logs remain private. Public records contain
scenario/backend/device, source/binary hashes, analytic check counts and scoped
pair behavior. These checks establish the named additive-boundary combinations,
not another general numerical qualification campaign.

## Production mutant scope

Four one-site changes attack real production code: strict rather than closed
overlap, discarded page start, a forced high tail bit, and discarded world
origin. Their targeted tests directly check singleton closed contact,
asymmetric offsets/complete walks and every padded word, or exact transformed
scene bounds. Native mutants build owned isolated CPU core copies; the world
mutant uses an isolated Python scene copy against the unchanged core. Every
associated unmutated test must pass first. Source anchors must match once;
no test/oracle expected value is mutated.

Only native/include/python and the build description are copied into ignored
context; references/forensic archives are not executed. Configure/build/test
logs stay private. Public results record original/mutant source and used binary
hashes, direct test and assertion verdict. No hardware performance or CUDA
numerical qualification is implied by this CPU production-mutation campaign.
