# Intersection plan: two-sided runtime review

2026-10-14. Reviewed [INTERSECTION_PLAN.md](../docs/INTERSECTION_PLAN.md)
against the accepted design and actual Engine, JetBatch and body-sampler resource
code. Accepted. No native execution or product changes.

The whole consumer owner is correctly responsible for prepared parameters,
source binding, native row packing, statuses, world conversion, grades and child
cleanup. Reusing unchanged ABI1 order-zero binaries avoids an unrelated native
qualification campaign. Focused source-level tests and actual body/pair callers
still exercise the affected wrapper/lifetime boundary. No GPU construction or
end-to-end gain is assumed.

## Concrete consumer handoff

`prepare_samples(result, resolution)` returns a factory-only
`IntersectionSampleRequest` with exact result identity, owned candidates and
resolution. `IntersectionSampler(engine, result).sample(request)` returns an
immutable `IntersectionSampleBatch(result, rows, timings)`. `count` counts
candidate/chart occurrences; `evaluated_count` counts native rows, twice count.
Distinct occurrences can share a physical span and XYZ.

Candidates retain `piece_id`, `component_id`, `span_id`, exact `parameter`,
`location` (lo/hi/point/interior), exact `left_uv`, `right_uv` and
`reference_position`. Rows retain those identities, parameter/location and exact
reference, original `left_slot`/`right_slot`, rounded left/right UV, separate
`left_position`/`right_position` and `left_status`/`right_status`. The result
retains all event/constraint records; the sampler invents no trim classification.

Native row order is interleaved: candidate i emits left row 2*i and right row
2*i+1. The cached PatchSet is dense by `(side, original_slot)`, not original slot
alone. Gather never submits lifted quadrant+u as a normalized chart parameter.
Complete descriptor comparison includes compact control offsets/counts, source
rectangle, origin/scale, degrees and lineage; actual slot/UV bytes are compared
before conversion. Order/count/array extents and statuses are checked both
before and after conversion.

## Numeric and failure details

Cap candidate accumulation before allocation and native work. Preserve exact
endpoints; interiors are rational witnesses in an inward interval with the
reviewed subdivision cap. Use exact comparison for the 2^-36 component grade:
convert each actual finite binary64 position to its exact Fraction, compare its
difference from Scalar C and the exact difference of the two supplied positions.
A rounded float residual is insufficient at the threshold. Failed grade,
provenance, row status or cleanup prevents batch publication.

Own BaseException-safe child scopes rather than inheriting the generic wrapper's
Exception-only context cleanup. Both result and query closes are attempted;
cleanup cannot replace an active primary object. Adopted PatchSet initialization
is guarded too. Closed borrowed Engine blocks sampling; geometry/request remains
usable. Empty requests return zero rows without creating a query/result.

## Measurement and verification

Expose phase names `gather_ms`, `query_ms`, `evaluate_ms`, `download_ms`,
`convert_ms`, `world_scatter_ms`, `grade_ms`, `child_cleanup_ms`,
`reused_wall_ms`. Download includes source/shape/status checks, conversion includes
converted status checking, and grade checks are separate host exact arithmetic.
Internal wall ends after child cleanup; root measures actual returned-call wall
through immutable publication externally. Empty phases are zero except actual
host gather/validation/wall work.

The plan's retained requests/PatchSet and five serial repetitions measure the
real consumer, not only evaluation. Cold construction/request/setup, final
cleanup, independent qualification and export remain separately named. Transfer
is 40 upload/56 download bytes per candidate plus actual geometry; provenance
metadata is host traffic, not silently counted as device jets.

Independent full-map/source/domain and graph oracles address geometry; direct
sampling checks address the actual approximation and cleanup path. Production
mutants target omission/hole/seam/source coupling, and must fail an associated
assertion rather than imports or crashes. Required first missing-method red
precedes product implementation; CPU1/CPU24 and both P100 sanitizer checks then
exercise the unchanged native consumer path. Session/pooling optimization remains
deferred as directed.
