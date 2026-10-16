# Native bound-runtime implementation seam

This is the concrete adapter for the accepted native mesher plan, following the
implemented and reviewed exact/CPU/P100 residual mechanisms. It introduces no new
geometry rule or dependency. Code follows root interface review.

## Immutable roots and packets

`BoundRootBank` is a shared immutable handle, allocated with RunAllocator. It
retains its original GeometryStore, a sealed ExactTable of three exact anchor
coordinates per selected root, root/source slot identities, and the packed
bgmb_root/interval arrays. Preparation imports each selected original root once,
anchors N by N-W*A with A=N0/W0, packs intervals once, and freezes A. A selected
root is either SurfaceRootSlot or CurveRootSlot, always qualified by the bank's
GeometryStore table key. Empty, duplicate, foreign and unsupported roots fail
before publication. The bank is backend-independent and shared by CPU/device.

Filter task packing imports only the root's small exact anchor/domain and its
cell/actual-L/tau values. It does not import all original controls or perform an
exact restriction per cell. Add a narrow prepared-frame pack overload to the
existing arithmetic unit so task packing reuses the exact cached anchor. Only
unresolved fallback imports the full original root and invokes the already
implemented exact restriction/comparison.

The public value seam in `native/mesher/bound_runtime.hpp` is:

```cpp
struct BoundRootSlot { uint32_t value; };
struct BoundTaskHeader {
  BoundTaskKey key;
  uint64_t source_generation, boundary_generation;
  uint32_t bound_revision; // currently1
};
struct SurfaceBoundInput {
  BoundTaskHeader header;
  BoundRootSlot root;
  std::array<ExactRef,6> original_uv;
  std::array<ExactRef,9> actual_linear_xyz;
  ExactRef tolerance;
};
struct CurveBoundInput {
  BoundTaskHeader header;
  BoundRootSlot root;
  std::array<ExactRef,2> original_parameter;
  std::array<ExactRef,6> actual_endpoint_xyz;
  ExactRef tolerance;
};
using BoundInput = std::variant<SurfaceBoundInput,CurveBoundInput>;
```

`BoundBatch` retains its bank, a sorted immutable vector of ExactTable handles,
and input records. Every ExactRef must resolve through a retained table with its
complete key/generation. Root slots are local to this retained bank, never a
global unqualified geometry index. Batch construction checks capacities, unique
table identities, all referenced bounds, source generation, revision and strictly
increasing unique task keys before publishing. Table lookup is binary search;
sealing input values may reuse their already sealed owner table rather than
exporting untracked ExactWords vectors. No mutable Rational/Budget crosses the
batch boundary. Boundary generation is retained and echoed; only the coordinator
knows whether a completion still belongs to the current face generation.

`BoundFiltered` retains the input batch and keyed answers plus separate modeled
work/filter-route counters. `BoundCompleted` contains the same headers and final
accept/bound_fail answers in original canonical order. No interval endpoint,
fallback route or cache allocation becomes semantic output.

## Child, session and completion operations

```cpp
struct BoundRuntimeLimits {
  uint32_t task_capacity, table_capacity;
  uint64_t child_bytes, child_work, total_work;
};
struct BoundDeviceLimits {
  int32_t device;
  uint64_t host_bytes, device_bytes;
};
class BoundRuntime { // serialized coordinator-owned adapter, no worker threads
public:
  BoundRuntime(std::shared_ptr<RunResource>, BoundRootBank,
               BoundRuntimeLimits);
  BoundFiltered filter_cpu(BoundBatch); // one reserved child, canonical order
  void open_device(BoundCudaModule, BoundDeviceLimits);
  BoundTicket submit(BoundBatch); // one outstanding device batch per runtime
  std::optional<BoundFiltered> poll(BoundTicket); // no exact work or mutation
  BoundCompleted resolve(BoundFiltered); // unresolved tasks only, reserved child
  void cancel(); // stop submissions; active device work still drains
  void close();  // explicit drain; typed error retains device ownership
};
```

The exact declarations may use private PIMPL/move-only completion handles to
keep owning vectors hidden. Errors use existing native Error codes; bgfd failures
are translated at these operation boundaries. Filter failure is a geometry bound
decision; CUDA/API/resource errors are run failures. CPU-only construction never
loads a CUDA module. BoundCudaModule owns its dlopen handle and validates the
existing C ABI; the device owner retains that handle until session destruction.
No new callbacks, worker pool, global mutable Budget or silent CPU error fallback.

CPU interval work occurs in a child that imports its own exact task values;
device packing does likewise before submit copies owned staging. Poll merely
collects completed filter answers. Exact resolution is a separate worker action,
not work in the event pump. It reuses one bounded child scratch at a time and
imports only unresolved source roots. Answers must match every original key and
slot; stale/duplicate tickets and result key mismatches are errors. A batch is
indivisible for successful delivery: no accepted prefix escapes a later failure.
The implemented filter/submit/resolve methods accept const handle references and
retain their own immutable ownership internally. An optional external stop token
reaches every child Budget, so scheduler stop interrupts charged arithmetic
without sharing a mutable Budget. Native cancellation errors retain the same
atomic completion rule.

Root owns the ResourceReservation seam:
`reserve_resource(shared_ptr<RunResource>,bytes)` returns a shared lease that
charges the full cap once before admission. Actual PMR allocations consume its
prepaid cap without a second parent charge. Each child declares its lease before
its Budget and uses `Budget(limits,reservation.get())`. No suspended parent
Budget is borrowed. Results are copied into retained run-owned completion storage
before releasing child scratch. The parent reserves packet/completion capacity
and child_bytes before admitting a batch; inability to reserve fails immediately.

The device owner reserves host_bytes+device_bytes with checked addition before
calling create, independently of the actual run-owned packed bank. This declared
external capacity remains charged for the entire session. Register the owner and
preallocate its bookkeeping before create: a nonzero C status can still return a
nonnull poisoned session. Every nonnull session remains reachable with its module
and reservations until successful destroy/drain. close failure leaves that owner
available for retry; teardown must not release live DMA storage or dlclose code.
The root run's failed-device retention policy must survive final CLI teardown;
this is the one lifecycle seam to agree before coding the public wrapper.
Root selected an intrusive preregistered process-owned record list: final drain
failure leaves its already allocated owner reachable for process lifetime, with
module, bank and reservation retained. Teardown emits the retained owner ID
without allocating. Explicit close still leaves the live object available for
retry; healthy close unlinks and destroys it. This is exceptional failure
retention, never a static geometry cache or a successful leaked session.

The runtime owns a narrow serialized cumulative work counter. Reserve the known
sum of degree-model filter work before CPU/device submission. Before each exact
child, reserve its configured child_work upper cap; completion commits actual
Budget.work and refunds unused reservation. Overflow or insufficient work fails
immediately. No thread mutates another child's Budget. The explicit counter can
later be folded into the coordinator's run-wide work reservation without changing
task meanings. Device C ABI's own work_limit is a secondary local guard, not a
second source of geometry decisions.

Cancellation stops admission, drains/collects active work, discards the canceled
generation's completion and then releases inputs. It does not free or reuse a
busy session. Sessions remain serialized; multiple P100s use distinct runtimes
with retained common root banks. Host scheduling, boundary/contact epochs and
canonical failed-cell frontiers remain the coordinator/topology unit's contract.

## Meaningful RED and checks

Author native real-GeometryStore packets with exact equality/near thresholds,
translated roots and at least one forced unresolved task. Observe missing API
RED before adapter implementation. Require CPU and each actual P100 to return
identical final keyed decisions through the same exact fallback, with caller
table handles destroyed after packet publication. Test foreign table generations,
missing leases, duplicate keys, wrong roots/revisions, zero work, child memory
exhaustion, busy/stale tickets, cancellation and drain. Verify reservations return
to baseline after successful close and that forced unresolved fallback uses its
own Budget. Retain original independent Fraction vector coverage of arithmetic;
do not build another formula-copy oracle or driver-failure certification campaign.

Run strict Release, ASan/UBSan/leaks, actual-source lint/format and one scoped
actual-P100 sanitizer pass for the adapter's ownership path. Geometry/preparation
and filter durations remain distinct from whole meshing. No whole-watch speedup
or complete pipeline delivery is claimed by these unit checks.
