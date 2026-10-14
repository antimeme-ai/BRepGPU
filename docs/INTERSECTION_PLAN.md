# Intersection delivery plan

2026-10-14. Integrated design accepted by independent topology, oracle and
runtime reviews after explicit trim-event, span-grouping and sampling-failure
corrections. Deliver issue20 on feat/analytic-intersections, PR21. No new native
kernel, dependency or GPU lifetime optimization is selected.

## Whole units and sequence

1. Oracle owner writes the body-facing direct circle contract first. Root runs
   it on the fleet using preserved pair-release: the existing Engine/body path
   must produce healthy known positions, then the absent
   BodySnapshot.intersect_faces method is the intended missing-operation red.
   Expected result is one closed circle, four source pieces and the coupled maps.
   No product implementation starts before that red. A second direct consumer
   contract specifies both source outputs/provenance/grade on the same circle.
2. Construction owner owns the WHOLE exact-construction invariant:
   python/brepgpu/intersections.py plus the lazy body-facing adapter. Implement
   quadratic Scalar, exact finite-float rounding, inspectable ExactMap, FaceRef,
   immutable result records, complete horizontal polynomial decomposition,
   complete vertical branches, all closed trims, named events and span grouping.
   No other owner edits this invariant. Written subplan in the journal first.
3. Oracle owner owns python/brepgpu/intersection_oracles.py and
   tests/test_intersections.py as one independent truth unit. Bind original
   controls/frames, prove coupled maps on whole pieces, check exact graph/spans
   and source domains. Literal complete fixtures independently prove counts and
   root/endpoint positions; generic map validity does not claim universal count
   truth. Attack near-tangency/subnormals, differently represented radicals,
   whole-boundary and trim tangency, seam-crossing arcs, generator occurrences,
   points, source swap, immutable snapshots and unsupported cases. Write its
   subplan before expanding tests. Never derive expected answers from the
   production constructor, scalar arithmetic or float conversion.
4. Runtime owner owns python/brepgpu/intersection_sampling.py, its prepared
   immutable requests and the WHOLE two-sided consumer/lifetime invariant.
   Use the agreed original slots/ExactMap API; dense-remap explicitly, one
   order-zero2N batch, exact source/UV-slot-byte/status verification, world
   conversion, exact-curve/two-side model grades and guarded cleanup. Empty
   results do no native query. Cross-result/closed/failure cases fail whole
   batches. Native files and ABI math stay unchanged. Written subplan first.
5. Root integrates public caller, benchmark, exports/graphics, qualification,
   delivery documents and coherent commits. Each completed unit receives
   independent code review; its owner fixes all valid findings and records
   incorrect findings. Unrelated lanes can continue while a unit is reviewed.

## Actual caller

Root adds examples/intersect_faces.py with two literal recipes at z=0:
box top scaled1 translated z=-.5 versus default cylinder wall (one circle),
and box top scaled.25 translated z=-.125 versus that wall (four quadrant arcs
inside half-width3/16 rectangle and circle radius1/4). Complete four-bit chart
pair discovery qualifies actual bounds/mask, groups into ONE face pair, then
the face operation handles all wall charts regardless of survivor subdivisions.
No unknown/unsupported scene is silently complete. Empty mask handling still
checks the declared supported expected geometry; no early prefix shortcut.

Export sources/controls/maps/exact Scalar coefficients, components/pieces/events,
two-sided samples and phase TSVs. Owned SVG plots actual geometry and labels
each physical component versus its chart occurrences, with actual component
counts for each recipe. Export exact scalar a/b/d
separately from approximated coordinates; floats are hex where bit identity
matters. Actual caller reports construction, request, engine/geometry setup,
pair preparation/discovery, sample phases, independent qualification, cleanup,
export and whole wall separately. General Boolean/import/SSI claims stay absent.

## Verification and measurement

Root serializes fleet operations and awaits each rsync before dependent work.
No local native/product tests/mutants/builds/performance. Reuse protected
build/pair-release core/plugin bytes; only Python/caller source changes.
Focused new tests CPU1/CPU24; both P100s under Compute Sanitizer. Existing body
and pair source behavior plus a known native position caller provide regression
checks; unchanged native ASan/ABI/deep-cache campaigns are not repeated.
Run direct production mutants after baseline green, isolated ignored source
copies, on the fleet: omit component, ignore plate hole, break seam/span
grouping and corrupt one source UV. Credit real oracle assertions, not imports,
crashes or generic exceptions. Pin used sources/binaries in exported summaries.

Benchmarks/intersections/run.py compares both literal cases at resolutions65
and4097, each CPU1/CPU24/P1000/P1001, serial fresh processes. Initial independent
qualification outside reused time; one excluded warmup and five reused calls
with retained result/request/Engine/PatchSet. Also report cold full constructor/
request/consumer path and final cleanup. Report exact CPU construction and
request prep separately from reused gather/query/evaluation/download/world/
model-grade/child cleanup and actual sample wall. Do not add unrelated medians
as latency. Main matrix has no profiling events; an optional separate qualified
call can expose native event scope. Count actual geometry transfer and40/56
device bytes per two-sided candidate; CPU construction/topology stay host work.

No owned copying/build/test/review execution overlaps trials. Retain raw
iterations and min/median/max; shared host is a limitation. GPU's useful value
may be confined to evaluation while Python/exact construction dominates.

## Consumer handoff

`prepare_samples(result,resolution)` returns factory-only
IntersectionSampleRequest(result,candidates,resolution). Candidate fields:
piece_id/component_id/span_id, exact parameter, location('lo','hi','point',
'interior'), exact left_uv/right_uv/reference_position. IntersectionSampler
borrows Engine and result; sample(request) returns IntersectionSampleBatch
(result,rows,timings), count and evaluated_count; native row count is2*count.
SampleRow retains IDs/parameter/location/reference_position, original left_slot/
right_slot, rounded left_uv/right_uv, left_position/right_position and both
statuses. Native rows interleave left/right at2*i/2*i+1. Dense patch keys are
(side,original_slot). No ordering relies on a global chart-slot namespace.

Timing phases: gather_ms, query_ms, evaluate_ms, download_ms, convert_ms,
world_scatter_ms, grade_ms, child_cleanup_ms, reused_wall_ms. Caller external
sample wall is separate. Prepared exact values are immutable bound inputs;
their CPU preparation belongs in request prep, not GPU evaluation. Result/
request/source identity checks happen before publication. Initial independent
qualification remains outside reused benchmark timing.

## Publication and return pass

Publish design/reviews, direct red, construction, consumer/oracles, qualification
and measured docs as coherent commits and push checkpoints. Update README with
actual SVG plus honest scope/results, roadmap, AGENTS, papers index, ISSUES and
docs/RETURN_PASS.md. Every action/review finding belongs in the journal; retain
source acquisition/restoration records and private operational logs in context.
Mark PR ready only with actual qualified delivery; merge with a merge commit,
preserve feature history/branch, close issue20, sync main to the owned fleet path.

After delivery proceed to another uncovered broad-pass theme. General SSI,
arrangements, interchange/GPU topology and round-two numerical/performance
ideas remain explicit; GPU session/residency/polish stays deferred.
