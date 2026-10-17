# Native progress trace: numerical boundary review

Scoped ACCEPT of docs/adaptive-model-progress-trace-plan.md. Diagnostics read
retained scalar identities, sizes and work counters only. No exact-coordinate
import, source evaluation, topology query, geometry callback, acceptance change
or Budget/Failure/CUDA ABI change is selected. The separate Ce-to-Q bound must
not be added again to the existing original selected-trace-to-SAME-Q contract.

Stack-owned scoped TLS is coherent for synchronous FaceTrim step/helper calls.
Its pointer must be restored on every return/unwind and never captured by queued
CPU or CUDA work. Original FaceID/query/assignment/horizon disambiguate pooled
owners and resumptions. Whole stderr rows are serialized, diagnostic exceptions
suppressed, and the off path returns before formatting. No log history is kept.

Actual source review must check snapshot validity before early cancellation,
guard destruction before the local child context dies, and copying the effective
inner work cap before cumulative counters are updated. CDT N/B should come from
the already built local site/cycle inventory. Trace row balancing and actual
original-output byte parity in the existing small model driver are direct
observability checks; no extra geometric qualification is needed. No build,
probe or numerical run by this reviewer.

Actual helper/Face/Surface/region/model source review ACCEPT. Inline TLS is
installed only around the synchronous owner call and restored by Scope. Early
stop closes only an already open admitted-query span. The child Budget guard is
destroyed before its local JobContext; effective inner worklimit is copied
before cumulative spent changes. Formatting and I/O are inside noexcept emit's
catch-all, while the native failure is retained and rethrown unchanged.
Actual CDT N/B come from built input rows. The current cdt_end guard brackets
the complete RegionBuilder::triangulate wrapper, including output copying after
construct_region_cdt; paired work counters describe that wrapper delta, not
pure core time. No numerical or ownership finding and no execution by this lane.
