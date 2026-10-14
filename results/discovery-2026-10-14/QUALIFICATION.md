# Discovery qualification

The owned native CPU/CUDA operation passed direct geometry, public ABI, memory,
failure, compatibility and production-mutation qualification on the supported
Linux x86-64 host and both P100s. Independently reviewed source defines the
admitted envelope and failure behavior; finite direct cases attack particular
faults rather than proving every floating execution.

| Direct evidence | Actual result |
| --- | --- |
| Exact original-input standalone corpus, CPU24 | 309 rows; 11,415 downloaded nodes; zero violations |
| Same corpus, P100 device 0 | 309 rows; 11,415 downloaded nodes; zero violations |
| Same corpus, P100 device 1 | 309 rows; 11,415 downloaded nodes; zero violations |
| Full public suite, CPU | 58 tests passed; three device-only skips |
| Full public suite under Compute Sanitizer, each P100 | 58 tests each; zero memory errors; zero leaked device allocations |
| Native Release and ASan/UBSan suites | 42/42 each; native leak detection enabled |
| Exact CPU corpus under ASan/UBSan, CPU24 | 309 rows; zero geometry violations; CPython process leak detection disabled |
| Native ASan/UBSan cylinder consumers | Three methods; 4,096 rows each; leak detection enabled; passed |
| Python publication/fatal-cleanup scenarios | Projection, hierarchy, query and discovery passed the owned callback model |
| Actual preserved-binary compatibility | 23 fresh-process combinations; 7,728 evaluator requests; 77,280 exact jet component checks; passed |
| Isolated production numerical mutations | Nine associated baselines green; all nine mutants killed; no infrastructure failure counted as a kill |
| Actual point-cloud callers, CPU24 and both P100s | 24 batches of 1,024; every returned witness passed exact qualification |

The standalone corpus includes all three bound methods, original homogeneous
input bits, complete dyadic cells, every actual AABB/support/coefficient cache,
positive denominators, negative C0 and inactive padding. It evaluates each
returned binary64 UV on the original input with Fraction and requires U to
contain that exact distance without allowance. Independent flat leaf/root
lower bounds, available analytic global answers, gaps and coverage/cap flags
are separate checks. CPU/GPU parity does not supply geometry truth.

Retained standalone receipts:
[CPU24](qualification/cpu-0/QUALIFICATION.md),
[P100 0](qualification/cuda-0/QUALIFICATION.md),
[P100 1](qualification/cuda-1/QUALIFICATION.md).
The final standalone runs preserve the current oracle default after its
additive caller-only analytic lower-check option; existing conformance uses
the unchanged flat/root lower path.

The actual caller checks every downloaded cache and every original-UV upper,
analytic global bracket and reported gap. Its known-global lower check is
explicitly separate from the standalone construction-specific leaf reduction.
It exports all valid unresolved rows. At depth8, the finer curved clouds remain
unresolved at the requested squared tolerance; the diagonal 26-DOP cloud
achieves all 1,024 gaps. Both are successful qualification outcomes, with
different attained precision. The capped cloud with no refinement also keeps
valid unresolved witnesses. [Caller and scope](../../docs/DISCOVERY_CLOUD.md).

[Compatibility identities and outcomes](compatibility.tsv) retain actual
v1/v2 core/plugin hashes and old adapter source identities, plus current hashes.
Fourteen combinations perform two nonfatal discovery refusals each; three
perform two supported gap-qualified discovery calls each; six use an old
adapter without that API. Existing evaluator geometry is checked before and
after these operations. Supported seeded projection or its explicit refusal
is also exercised. The protected baseline inputs remained unchanged. Missing
old v3 factories cannot model transient errors; the existing native transient
callback scenario supplies that distinct fault evidence.

[Mutation outcomes](qualification/mutations.tsv),
[actual red witnesses](qualification/mutation-witnesses.tsv) and
[source identities](qualification/source-manifest.tsv) record isolated owned
source copies. Faults cover subdivision/outward arithmetic, descriptive distance
as a false witness upper, DOP norm/sign, negative C0 clipping, padding, capped
partial lower and CUDA equality pruning. Several mutants fail valid-input
construction or produce an arithmetic-failure row instead of the admitted
answer; these are geometric contract kills. Build, loader and device failures
are separate and did not occur. The runner preserves authoritative source and
requires each associated original case to pass first.

Source reviews:
[geometry](../../papers/2026-10-14-discovery-code-geometry-review.md),
[numerical/runtime](../../papers/2026-10-14-discovery-code-numerical-runtime-review.md),
[runtime boundary](../../papers/2026-10-14-discovery-code-runtime-review.md),
[independent oracle/benchmark](../../papers/2026-10-14-discovery-code-oracle-review.md).
Reported findings were fixed and re-reviewed before repeated measurements.

No general NURBS extraction, trim-valid witness, cross-frame comparison,
unique nearest UV, complete minimizer enumeration, intersection or topology
operation is established by these receipts.
