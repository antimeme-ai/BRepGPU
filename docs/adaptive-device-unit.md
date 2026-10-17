# Resident adaptive CUDA operation

Root takes ownership of cuda/adaptive_cuda.cu and its finite API while runtime
owns the native cache/coordinator. Runtime explicitly handed this whole device
module/lifetime unit over; no device implementation existed. The accepted parent
design/runtime unit supplies the numerical and canonical frontier contracts.

The API retains original/prepared root descriptors, controls, constructed points,
node records, two frontier buffers, prefix/count scratch, operation staging and
one CUDA stream/event. Inputs are owned before asynchronous work starts. Every
operation returns a ticket; poll reports pending without a host synchronization.
Only poll-ready publishes outputs. A second outstanding operation refuses.
The finite API adds explicit node capacity, cached base NodeBounds in node input,
and actual host/device allocation/transfer counters so runtime does not invent
memory or transfer measurements. Root updates this header with runtime informed.

Analytic samples/bounds use threads. Rational restrictions and bounds use one
warp per record, a lane-strided degree-bucket shared control net and per-lane
residual coefficients; no per-thread bicubic net. Common HD RN arithmetic and
geometry formulas are unchanged. Same-profile cached bounds bypass recomputation
but still execute device accept/split marking, scan and canonical child frontier
compaction. Exact original dyadic/path identities remain native coordinator data.
GPU prefix outputs determine canonical child placement. Regularity is required
alongside the actual positional packet bound; unresolved arithmetic is explicit.

Capacity checks cover all indices, array products and complete destination ranges
before mutation. Grow buffers between operations while retaining old allocations
on failed drain; separate host/device caps meter actual owned allocations.
Required native reserve/release callbacks join each actual module allocation to
the run's aggregate reservation before acquisition, without reserving both entire
caps. Supplied sample records install only demanded cached/promoted canonical
values; ordinary cold points still execute the device numerical kernel.
Create exposes any allocated owner even on failure. Healthy destroy drains then
releases resources; failed destroy retains all remaining pointers for retry.
The native owner preregisters the returned token/module and retains poison state
on final failed close. No CUDA callback can allocate a replacement poison owner.
Restrictions read only controls resident before their ticket, with disjoint
complete destinations and no destination/parent overlap in that batch. Dependent
parent→child restrictions use separate tickets. Sample destinations and node
slots are unique within each operation. Validate these races and ranges before
any pool mutation or launch.

Direct RED: a native volatile reference to the declared module factory must fail
to link precisely that absent operation before the body exists. GREEN uses the
existing original-source geometry reader through actual device operations and
the integrated source→PatchSession→triangles caller on both P100s. Focused invalid
capacity/ticket/lifetime paths and relevant CUDA checking attack device ownership;
no second geometry evaluator or benchmark certification framework. CPU-only
builds stay independent. Actual source review closes this module before delivery.

The focused protocol fixture `tests/native_adaptive_device.cpp` uses the real
CELL SourceStore→SourceCache and module reserve/release callbacks backed by
RunResource. A 513-row cached-bound mixed accept/split operation compares every
exclusive split prefix with a simple integer count across scan block boundaries;
the cached bounds are trusted test input, not a second geometry oracle. The
same finite fixture rejects duplicate destinations/slots, restriction write/read
and write/write overlap, malformed root shapes/families, undersized owner caps,
stale/consumed tickets and short poll buffers, then drains successful work and
requires every callback lease to return. Root owns CMake and existing numerical
readers; no driver-loss injection or further geometry campaign is introduced.
