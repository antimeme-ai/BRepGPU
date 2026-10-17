# Selected-face caller refusal and publication ports

Status: root-reviewed subplan implemented; all11 process failure ports GREEN.
Owner: runtime lane. Product changes and CMake remain root-owned.

The complete caller plan requires rejected operations to publish nothing. The
seven-family performance cohort exercises successful output, not that contract.
This unit adds `tests/native_adaptive_trimmed_failures.py`, using the existing
original-source fixture writer. It does not add a geometry oracle or rerun the
successful cohort. Cancellation, stale work and direct owner work-cap behavior
remain covered by `native_adaptive_trimmed_work.cpp`.

## Grounding and product seam

The operation and failure requirements come from
`docs/adaptive-trimmed-caller-plan.md`. The actual mechanisms are
`adaptive_trimmed_main.cpp::options/run/main`, `source.cpp::load_source/read_source`,
`resources.hpp::RunResource`, `adaptive_curves.cpp`'s original endpoint guard,
`adaptive_cache_detail.hpp`'s checked depth, and
`output_directory.hpp::OutputDirectory`. The last removes its unique temporary
tree on failure and publishes through Linux `renameat2(RENAME_NOREPLACE)`.
These sources, rather than a new abstract checker, determine expected refusals.
The earlier caller plan's pinned NVIDIA/OCCT references ground the successful
packet/band operation; they do not supply our error or atomic-publication contract.

One actual interface gap is identified: memory/nodes/depth are operator options,
but construction work is not. Root approved `--construction-work` as a positive
uint64 assigned ONLY to `TrimFaceLimits::construction_work`; preserve the actual
default `1<<42`. Do not change source budgets or other owner defaults. The new
test requests one work unit, exercising the real owner cap through the caller.

## Finite direct cases

Each case runs CPU1 once against the current strict fleet executable. It uses
one quality and a fresh isolated parent directory; default other capacities
remain unchanged. P100 successful publication and shared device ownership were
already exercised by the completed cohort, so this unit does not repeat it.

| Case | Input and direct expected refusal |
| --- | --- |
| Missing definition | Valid plane source, nonexistent DefinitionID: source.decode, invalid_source, original source-ID lookup failure. |
| Wrong definition owner | Add a valid empty second product/definition; request it with face900 owned by definition200: source.decode, invalid_source, explicit face/definition relationship. |
| Missing face | Valid source, nonexistent FaceID: source.decode, invalid_source, source-ID lookup failure. |
| Original source node mismatch | Move original vertex300 by z=.125, leaving original Ce unchanged; node cap1e-8: query.construct_shared_boundary, unresolved, original endpoint source-node cap exceeded. No repeated depth exhaustion is accepted. |
| Truncated source | Remove the last byte of an otherwise valid source: source.decode, invalid_source, declared file-length mismatch. |
| Unknown wire map | Change one COEDGES_ row's literal parameter_map_kind to99, preserving every count/length: source.decode, invalid_source, unknown source parameter map kind. |
| Source-admitted unavailable map | Valid kind3 map with endpoint pairs: source.compile_original_curves, unsupported, unimplemented original use parameter map. The existing definition_curve_requests guard rejects it before trace evaluation. This differs from malformed enum99. |
| Memory exhaustion | Valid source with a2MiB adapter-revision string, request1MiB run memory: source.decode, resource_exhausted, run memory reservation exhausted while staging original bytes. No geometry allocation assumption is needed. |
| Depth exhaustion | Existing saddle has linear boundary curves, so its boundary can settle before its nonaffine surface. Positional allowance.001 and max-depth1 must reach query.construct_complete_face / unresolved / adaptive requested depth exhausted. The checked Core split guard supplies this expected failure; an earlier unrelated refusal fails the test. |
| Work exhaustion | Existing plane, --construction-work1: query.construct_complete_face, typed owner work exhaustion. Existing direct lifecycle test supplies the expected owner failure; this adds only CLI propagation/cleanup. |
| Existing target | Precreate target with a nested sentinel and fixed bytes; expect options/io_failure and the explicit no-replace message. Exact directory inventory and bytes must remain unchanged. |

The unknown-enum mutation walks only the existing fixed BGSRC001 section framing
to locate the field, using the source writer's explicit layout. It is an input
fault injector, not a second source parser. A valid kind3 fixture goes through
the normal writer and preserves its original t/s endpoint mapping.

## Oracle and execution

Require exit1 and exactly one structured `error<TAB>stage<TAB>code<TAB>message`
record with the expected stage, error class and distinguishing message. A signal,
timeout, `unexpected` exception, or unrelated earlier refusal fails the test.
For the bgfd work failure, use its existing `exact` diagnostic plus the real
work-budget message, rather than relabeling it as ErrorCode2. Require no success
completion line.

For each initially absent target, require that neither the final output directory
nor any `target.tmp-*` sibling survives. For an existing target, compare its full
relative inventory and bytes before/after, and require no temporary sibling.
Do not delete a product leftover to make a failing cleanup assertion pass.
This checks actual process-visible publication; source review of RENAME_NOREPLACE
covers its atomic primitive. A timed publication race is not added.

Save per-case source, command.txt, stdout/stderr and native elapsed time under
ignored context. Use TSV/plain text, no JSON. A fixed process timeout is only a
censor/failure, never evidence of a correct refusal. No readers are needed for
artifacts that must not exist, no new dependency and no commits in this unit.

After review: author fixture, upload narrowly, coordinate the fleet process slot
with root, execute once, report concrete product findings before any rerun. Root
owns fixes; rerun only affected cases. Then source-review the fixture and record
the precise claim and remaining limitations in the journal.

## Execution closure

The old executable rejected the new work-cap option as unknown (actual RED).
Root delegated the single product option after reviewing this plan. The isolated
strict Release rebuild and actual-main clang-tidy pass. Initial execution passed
six cases and found a wrong expected stage in the source-admitted unavailable-map
case. Inspecting definition_curve_requests identified its earlier deliberate
unsupported guard; corrected the expectation and executed only the remaining
five cases. All11 direct claims pass. Raw logs and sources remain in ignored
context; no performance claim follows from these short failure processes.

Existing-target refusal is tested before work begins. Linux no-replace rename
source review covers the publication primitive; no timed race was introduced.
Cancelled/stale owner checks and device failures remain with their existing
direct fixtures. This unit does not claim general unknown-source fuzz coverage.
