# Interchange caller and reference runtime review

Source-only review of `examples/import_carriers.py`,
`scripts/reference_interchange.py` and `benchmarks/interchange/run.py` against
the accepted design/plan, existing Engine/JetBatch implementation and actual
reference API probes. No concurrent product execution or measured source edits.

**Three bounded findings sent to root before measurement:**

1. The reference utility currently records domains and finite first jets but does
   not compare them with source domains or the independent source-basis oracle.
   Its “checked separately” metadata therefore needs an actual comparison path.
   Check the reference domains against original source carrier domains, full
   source/face/query correspondence, and all nine jet components at the exact
   actual submitted binary64 U/V. Do the truth work outside the scalar-query
   timer; record errors and declared limits. The source oracle at an exact-affine
   native parameter is not the same query. The generated polynomial sheet should
   also retain its explicit known-analytic qualification rather than treating
   successful reference export as correctness of its intended construction.
2. The actual caller checks grid width but constructs slots/UV before bounding
   `len(conversion.patches)*grid**2`. An admitted 16 MiB source can contain many
   small carriers and request enormous host arrays. Add a fixed total-query
   admission before those allocations; rejection must not allocate the page.
3. The campaign controller has no own source hash in campaign metadata. Existing
   child pins cover the product/caller/oracle, not repetition/placement/summary
   logic. Record the controller's hash directly in campaign source/scope output.

The reference helper's corrected `lower,upper=surface.uv_domain()` and explicit
flattening match the actual wheel API. Sheet construction correctly treats the
standalone surface and boundary curves as consumed and exports a copied resulting
body. The optional reference remains isolated from the owned converter process.
It currently supports a single actual imported body and one prim explicitly;
those limitations are appropriate for the selected mouse/sheet fixtures. No
magical pybind pointer access, C++ ABI dependency or new install is introduced.

The owned caller reads one capped source snapshot, binds actual returned slots/UV
to submitted arrays, checks output extent/order/backend, requires statuses and
finite consumed fields, then independently qualifies the exact source/coefficient/
native descriptor/frame relationship. The checksum reads every first-jet value;
it adds no manufactured string processing. Result, query and patch resources
close before host conversion; the copied JetBatch remains valid. Context cleanup
falls inside the complete wall. Native plugin hashing matches the actual default
core-sibling plugin discovery path.

Timers cover source read/parse/conversion, actual query preparation, context,
patch/query snapshots and uploads, evaluation, downloads/provenance, required
validation/source conversion, checksum and all native cleanup. Exact independent
qualification, output writes and optional foreign acquisition/scalar evaluation
are outside that useful wall, while parent child-process time includes those
costs. Every repetition creates a fresh process and Engine: excluded warmup does
not establish retained GPU-context reuse. “Exploratory unpinned latency” correctly
prevents clock-isolated or general CAD speedup claims. Final acceptance awaits
root integration of the three findings and source re-read.

## Final disposition

Re-read the revised caller/controller and new
`scripts/compare_interchange_reference.py`. **Accepted; all three findings
resolved.** The total-query cap of 1,048,576 precedes host slots/UV allocation;
the controller records its source hash and rejects changes during the campaign.
The separate comparator binds retained source hash, all original domains, full
query order/face/patch identities, exact-affine source coordinates and actual
rounded foreign U/V. All nine foreign jet fields are compared to exact original
source-basis truth at those actual submitted values, under a declared relative/
absolute floor of 2^-32. Its native/reference gap includes source-query rounding
and separately qualified native source budgets. The foreign comparison tolerance
is a distinct scope from the stricter local native grades; publish actual errors
and limits together.

For the analytic sheet, all 16 source coefficients/weights/counts/orders/domain
are checked against the independently specified construction. Maximum Cartesian
coefficient displacement bounds positions; 12*delta bounds each unit-domain
cubic partial conservatively. Exact source-basis values and foreign jets are
checked against the intended polynomial map under those coefficient and foreign
budgets. Truth work has its own comparison timer and remains outside the useful
caller/scalar-query timers. This acceptance is source review, not a claim that
the pending actual comparison/cost campaign has run successfully.
