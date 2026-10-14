# Iteration 3 runtime / ABI design input

Design input, 2026-10-14; declaration examples below are proposals, not product
headers or implementation. Read the current
[runtime](../native/runtime.cpp), [v1 backend](../native/backend_api.h),
[v2 extension](../native/projection_backend_api.h),
[projection public header](../include/brepgpu_projection.h) and
[Python adapter](../python/brepgpu/__init__.py). The smallest extension reuses
one context, plugin, registry, admission lock, retained patch token, fatal state,
publication guard and Python resource machinery. It adds three object kinds and
one pure optional backend table; it does not change any v1/v2 POD or function.

## Accepted operation shape

Root's integrated-design choices are the basis of this proposal:

- CPU-built immutable complete binary dyadic hierarchy: depth 0..12, default 8.
  Maximum admitted nodes is at most 1,048,576. Invalid/unsupported patches or
  construction invariant failures refuse the entire hierarchy. No adaptive
  coarsening or omitted-domain publication occurs.
- Every query names one patch slot and a target in its local frame. Query inputs
  are slots (u32) plus XYZ triples (binary64); no seed input or cross-frame union.
- Discovery returns one best candidate, approximate point/distance, a conservative
  squared-distance bracket and counters/reasons. No public complete seed list.
- `max_visits`: 1..1,048,576, default 8,191. `max_refinements`: 0..64, default 8.
  `tolerance_squared`: finite 0..1, default 2^-24. Internal projector uses fixed
  accepted-update cap 32, backtrack cap 24 and gradient tolerance 2^-24.
- Eight double SoA fields: s, t, x, y, z, approximate distance squared,
  lower squared bound, upper squared bound. Six u32 slabs: status, flags,
  visits, pruned, refinements, evaluations. Exactly 88 bytes per request before
  retained source metadata and allocation overhead.
- Per-query statuses: GAP=0, UNRESOLVED=1, BAD_PATCH=2, BAD_TARGET=3, ARITHMETIC=4.
  Flags: candidate=1, gap=2, coverage=4, visit-limit=8, refinement-limit=16,
  arithmetic-reason=32. The gap check rounds U−L upward and compares to the
  squared tolerance. Gap can hold without coverage: a root-bound fallback may
  already be tight enough. No flag asserts unique UV or a nearest trimmed face.

The integrated numerical design must define absent-candidate/invalid fields,
which counters include sample versus projector evaluations, coverage completion,
reason combinations and tie-breaking. Runtime preserves these values and must
not reinterpret GAP as stationarity or overwrite the reasons with BG_OK.

## Public PODs and entry points

Use a new `include/brepgpu_discovery.h` including `brepgpu.h`. Define
`BG_DISCOVERY_ABI_VERSION=3`. All new versioned descriptors use version 3;
shared `bg_patch_desc`, `bg_diagnostic`, `bg_status`, `bg_handle`, context flags
and close function remain version 1. An ABI3-specific version helper must not
change `version()` or `projection_version()`.

Proposed Linux-x64 POD layout:

| POD | Fields in order | Size |
| --- | --- | --- |
| `bg_hierarchy_options` | u32 version,size,depth,reserved; u64 max_nodes; u32 flags,reserved2 | 32 |
| `bg_hierarchy_node` | double lower[3],upper[3]; u32 left,right,patch_slot,u_index,v_index,u_level,v_level,flags | 80 |
| `bg_hierarchy_info` | u32 version,size; u64 patch_count,node_count; u32 depth,backend,flags,node_size | 40 |
| `bg_discovery_options` | u32 version,size,max_visits,max_refinements; double tolerance_squared; u32 flags,reserved | 32 |
| `bg_discovery_info` | u32 version,size; u64 count,patch_count; u32 backend,flags,field_count,reserved; double kernel_ms | 48 |

Node arrays are plain POD payloads, not independently versioned descriptors.
Define `BG_HIERARCHY_LEAF=1` and a u32 absent-child sentinel `UINT32_MAX`.
Build option flags hold an enum-valued mode: `BG_BOUND_AABB=0`,
`BG_BOUND_DOP26=1`, `BG_BOUND_BERNSTEIN=2`. Value 3 and all other values reject;
they are not combinable bits. Discovery option flags remain zero. Hierarchy
info flags report the admitted build mode.
Parents/roots and dyadic cells must have deterministic ordering defined by the
numerical builder. Integer chart fields refer to the original normalized chart;
no source identity is inferred from a memory address. Validate reserved fields as zero, reject unknown flags, require canonical
descriptor sizes exactly, and check node-size inquiry exactly.
Add `bg_discovery_abi_info` reporting all these sizes, diagnostic size, eight
fields/six integer slabs and primitive widths. Compile-time size/alignment
assertions and lazy Python size inquiry protect the boundary.

Suggested entry points (all ordinary calls end with `diag,diag_bytes`):

```c
bg3_discovery_abi_info(out, out_bytes);
bg3_create_hierarchy(patches, options, options_bytes, out_handle, diag, diag_bytes);
bg3_hierarchy_info(hierarchy, out, out_bytes, diag, diag_bytes);
bg3_hierarchy_download(hierarchy, nodes, node_count, node_bytes,
                      roots, root_count, diag, diag_bytes);
bg3_hierarchy_bound_download(hierarchy, bound_cache, bound_doubles, diag, diag_bytes);
bg3_create_discovery_query_set(hierarchy, slots, count,
                              targets, target_doubles, out_handle, diag, diag_bytes);
bg3_discover(queries, options, options_bytes, out_handle, diag, diag_bytes);
bg3_discovery_info(result, out, out_bytes, diag, diag_bytes);
bg3_discovery_download(result, fields, field_doubles,
                      statuses, status_count, flags, flag_count,
                      visits, visit_count, pruned, pruned_count,
                      refinements, refinement_count, evaluations, evaluation_count,
                      diag, diag_bytes);
bg3_discovery_sources(result, patches, patch_count, patch_bytes,
                     slots, count, targets, target_doubles,
                     hierarchy_options, hierarchy_options_bytes,
                     discovery_options, discovery_options_bytes, diag, diag_bytes);
```

The final header supplies concrete parameter types using existing ABI conventions.
Payload extents are exact: target_doubles=3N, field_doubles=8N, every integer
slab=N; node_bytes=node_count*80; root_count=patch_count. Roots are u32 node
indices. Inquiry descriptor byte capacities may exceed the canonical size; only
the canonical record is read/written. Null pointers are allowed for zero payload
counts. Hierarchy construction of an empty patch set should emit an empty
hierarchy (no arithmetic), and an empty query should emit an empty result after
structural/options validation, without workers or environment checks.

`hierarchy_download` must read the actual uploaded node/root arrays for CUDA,
not merely return the cached host inputs. Thus the exact oracle can inspect
what subsequent GPU traversal reads. CPU uses its owned host arrays. Full
node export is a qualification/debug operation, outside compact result timing.
Retained original controls already define the patch carrier; fixtures keep their
exact inputs for the enclosure oracle. Result sources export retained descriptors,
slots, targets and both option snapshots after public hierarchy/query/patch close.
They do not pretend descriptor provenance alone certifies a trimmed CAD entity.

## Explicit stronger-bound experiments

Keep AABB default until full-operation measurement supports a stronger mode.
The operator requested both 26-DOP and rational Bernstein experiments; the
mathematical enclosure/query lower bounds remain subject to numerical review.
Preserve the 80-byte base node. Every hierarchy stores exactly one enum mode
and one separately owned optional immutable binary64 cache:

| Mode | Cache doubles per node | Cache bytes per node | Cache bytes at 1,048,576 nodes |
| --- | --- | --- | --- |
| AABB=0 | 0 | 0 | 0 |
| DOP26=1 | 20 | 160 | 167,772,160 (160 MiB) |
| BERNSTEIN=2 | 490 | 3,920 | 4,110,417,920 (3,920 MiB) |

DOP26 stores ten additional diagonal support lower/upper pairs. Together with
three AABB axes these are 13 directions. Specify one canonical ordering and
exact normals in the reviewed numerical contract. A valid query lower bound is
the maximum of AABB and the individually certified slab-distance-squared /
normal-norm-squared bounds. It is not exact distance to a DOP or hull.

Bernstein stores five blocks of 49 coefficient lower/upper pairs per node:
ΣA_k², AxW, AyW, AzW, W². K=(2p+1)(2q+1) coefficients are active per block;
remaining pairs are zero padding and never participate in query minima. Degree
metadata comes from retained patches. Coefficients are enclosed from the
CPU-built interval control net. Query forms the target-dependent numerator
coefficients and minimizes certified ratio lower endpoints against positive
W² coefficients, then takes the maximum with AABB. The cache is an interval
carrier, not presumed exact polynomial data. Numerical review owns the ratio,
padding, denominator and cancellation behavior.

Generic `bg3_hierarchy_bound_download` / private `hierarchy_bound_download`
exports actual resident cache values for independent exact qualification. Its
extent is exactly node_count times 0/20/490 according to mode; AABB permits null
with zero extent but still validates the hierarchy handle. Node ordering and
cache ordering must match; define node-major cache layout and lower/upper pair
order explicitly. A cache entry cannot acquire meaning from an arbitrary pointer.
`hierarchy_upload` receives the owned cache, count and admitted mode. Backend
tokens own node/root/cache allocations; any failure drains all allocations and
publishes no token. Never silently downgrade an explicitly requested mode.

Before allocation, derive actual node totals from each patch's degree: a
(0,0) patch has one node; other patches have 2^(d+1)-1 nodes. Check total against
max_nodes. Select stride by exact mode equality, then checked u64 and size_t
products for node bytes, root bytes and cache doubles/bytes, plus vector maxima.
At the maximum, base nodes add 83,886,080 bytes (80 MiB). Bernstein cache plus
base nodes is 4,194,304,000 bytes (4,000 MiB), before roots, patch/query/result
storage and simultaneous host/device copies. Node count alone does not establish
available memory. Allocation/backend OOM refuses the whole unpublished hierarchy;
benchmark and docs report peak host bytes and uploaded/resident bytes. Do not
allocate DOP and Bernstein caches together or hide cache construction in a
resident query scope. Recursive builder scratch stays bounded by depth; only
immutable node/cache output persists.

Compare all three modes on identical geometry, targets, budgets and projector:
complete call time, CPU build time/peak memory, upload bytes/time, node visits,
refinements, bracket widths and unresolved rates. Setup/cache amortization is
part of the comparison. No box-throughput-only result selects the default.

## Owned graph, construction and admission

Add `Kind::Hierarchy`, `Kind::DiscoveryQuery`, `Kind::DiscoveryResult`.

- Hierarchy retains `shared_ptr<PatchSet>`, owned node/root/optional bound-cache vectors, owned build
  options and optional CUDA token. It uses the same context and plugin lifetime.
- DiscoveryQueries retains `shared_ptr<Hierarchy>`, slots/targets snapshots and
  optional CUDA token. No borrowed Python payload survives the call.
- DiscoveryResult retains `shared_ptr<DiscoveryQueries>`, owned discovery options,
  CPU output vectors or CUDA token, and existing `bg_backend_profile`.

Complete binary depth d has `2^(d+1)-1` nodes per varying patch and `2^d` leaves;
a degree-(0,0) patch contributes one leaf. Compute
these bounded shifts first, then checked patch-count multiplication and max_nodes
admission before allocation. Root indices fit u32 under the global node cap.
Allocate/build unpublished arrays; validate every patch's envelope and every
construction invariant; only then upload/publish. A failure in patch k destroys
all unpublished arrays/token and emits no hierarchy ID. The maximum-node cap
must not silently alter requested depth or omit patches.

Reuse Admission's lookup/promote, registry release, context lock, public-validity
recheck and OPEN/poison check. Build holds that admission lock while reading the
retained patch snapshot and publishing the tree. CPU workers use immutable native
snapshots without looking up public handles. Context close waits for the operation,
then invalidates public IDs and drains tokens as it does now. Separate contexts
can run concurrently; no process-wide CPU lock or mutable global FP environment
is needed. Keep the registry lock off arithmetic and backend callbacks.

New native token destructors use `cleanup_child()` and `device_children`;
registry publication uses `publish_owned()`. Cleanup consumes each token once,
retains ctx/plugin until completion, preserves original allocation/publication
failure while OR-ing fatal cleanup flags, and poisons future CUDA operation
families. Final process-poison checks stay serialized with successful publication.
The new object graph has only child-to-parent retention, not registry/context
shared-pointer cycles.

## Pure v3 plugin extension

Use separate `native/discovery_backend_api.h`, factory symbol
`brepgpu_cuda_backend_v3`, capability bit 4 and this conceptual static table:

```c
typedef struct bg_backend_v3 {
    uint32_t abi_version, struct_size;
    uint64_t capabilities;
    const bg_backend_v1* base;
    /* callbacks below; status + ABI1 diagnostic on every path */
    hierarchy_upload;
    hierarchy_close;
    hierarchy_download;
    hierarchy_bound_download;
    discovery_query_create;
    discovery_query_close;
    discover;
    discovery_result_close;
    discovery_download;
} bg_backend_v3;
```

With nine pointer callbacks this table is 96 bytes on the supported platform.
It is not appended to or reinterpreted as v1/v2. The factory is pure: static table
selection only, no device change, allocation or CUDA call. Its `base` must equal
the v1 table that created ctx/patch tokens. The numerical backend can call its
owned projector helper internally; it need not negotiate v2 or pass v2 public
tokens into v3. Therefore a missing projection extension cannot accidentally
refuse discovery, and discovery failure cannot rewrite the projection cache.

Callback signatures should mirror current conventions:

- `hierarchy_upload(ctx, patch_token, const node*, node_count, const u32* roots,
  root_count, const double* bound_cache, bound_doubles, bound_mode, out_token, diag)`: core has already built and validated the tree.
- `hierarchy_close(ctx, tree_token, diag)`; `hierarchy_download(ctx,tree_token,
  nodes,node_count,roots,root_count,diag)`; `hierarchy_bound_download(ctx,
  tree_token,bound_cache,bound_doubles,diag)`.
- `discovery_query_create(ctx,tree_token,const u32* slots,N,const double* targets,
  out_token,diag)`; `discovery_query_close(ctx,query_token,diag)`.
- `discover(ctx,query_token,const discovery_options*,out_token,profile,diag)`;
  `discovery_result_close(ctx,result_token,diag)`.
- `discovery_download(ctx,result_token,fields,8N,statuses,N,flags,N,visits,N,
  pruned,N,refinements,N,evaluations,N,diag)`.

CUDA upload/query/result callbacks return only after owned-stream completion and
successful per-thread device restore; failure leaves output token null. Close
callbacks consume tokens even on cleanup error. Download failure marks all supplied
host destinations unusable; classify resident state/fatality through the existing
backend diagnostics. CPU-only paths never probe/load CUDA. Old v1/v2 binaries and
plugins remain useful for their existing operations. Lazy negotiation caches only
stable missing/refused/malformed capability or a validated table. Runtime/fatal
errors, including fatal+OK/UNSUPPORTED anomalies, pass through `backend_status`
and are not misclassified as capability absence.

## CPU floating-point environment and worker failure

Hierarchy construction uses CPU outward arithmetic even for a CUDA context.
Validate the actual nonempty builder thread before control arithmetic. For CPU
query execution, validate every actual worker thread before its first arithmetic,
including the caller used by the single-worker path. Numerical-lane agreement:
round-to-nearest, gradual underflow (MXCSR FTZ/DAZ clear), masked exceptions for
non-stop arithmetic and consistent supported host environment. Checking only
`fegetround()` is insufficient. This iteration rejects unsupported environments
with batch BG_UNSUPPORTED/out0; it does not silently mutate them.
[Reviewed numerical study](2026-10-14-bounds-numerical-study.md).

Use a noexcept worker wrapper that records setup/arithmetic-entry failure into
owned synchronized state rather than throwing across `std::thread`. Allocate
all result/scratch storage before workers. Each worker validates then either
returns without doing arithmetic or runs its full-N/[begin,end) numerical helper.
If any worker rejects the environment, join every started worker and publish no
result; other workers may have written private unpublished buffers, which are
discarded. A start barrier is unnecessary if no success can be published early.
On partial thread creation failure, join the already-created workers before
unpublished output destruction, as existing evaluator/projector paths do.
Thread-local environment validation requires no global mutex. No thread may
change its own mode during its owned operation. Empty payload paths perform no
arithmetic and create no workers; validate options/structure regardless.

The numerical CPU-range signature must retain full N as SoA stride, separate
[begin,end), complete immutable tree/root/patch/control/query inputs, options,
caller-owned scratch with explicit capacity if needed, and six output slabs.
Environment admission belongs in runtime worker setup; the arithmetic helper
stays allocation-free/noexcept. An unexpected construction enclosure invariant
failure is whole-build refusal, not a published tree with a missing patch.

## Pointer, alias and publication rules

New ABI3 functions explicitly reject storage overlap. Validate pointer integer
extents/alignment and checked products before reads/copies; never manufacture
raw C pointers from Python callers. Borrowed nonempty buffers are valid and
stable for the duration of the native call; Python first copies user inputs
into private arrays and native validation/build/upload use that same snapshot.

For creators, compare handle-output and diagnostic canonical written regions
against each other and every descriptor/payload read region **before**
`zero_handle()` or `call()` can write. On forbidden overlap return
BG_INVALID_ARGUMENT without writing the diagnostic, handle or aliased input.
On ordinary nonalias failures, zero the valid handle destination and report the
ABI1 diagnostic normally. This is an ABI3 rule; do not change old ABI behavior.
A descriptor's used region is its canonical sizeof, not arbitrary extra capacity.

Every download/info/source export preflights all output-output and
output-diagnostic pairs before payload writes. The diagnostic-alias guard runs
outside `call()` because its final diagnostic write would otherwise corrupt an
output even on rejection. Source-export option records are outputs as well as
initialized version/size descriptors; validate all regions before copying any.
On structural rejection all payloads remain unchanged. Arbitrary bad/dangling
C addresses remain a caller violation; checked lengths/alignment/alias cannot
validate allocation ownership. Null zero-count extents do not overlap.

## Python surface and benchmark boundaries

Proposed names:
`HierarchyOptions(depth=8,max_nodes=1048576)`,
`DiscoveryOptions(max_visits=8191,max_refinements=8,tolerance_squared=2**-24)`,
`Engine.create_hierarchy(patches,options=None)`,
`Engine.create_discovery_query_set(hierarchy,patch_slots,targets)`,
`Engine.discover(queries,options=None)`. All native creators use existing
`_create_resource` guarding `_invoke`, handle extraction and resource constructor
against BaseException, including interrupts after native publication.

Declare ABI3 symbols lazily on the first discovery/hierarchy call and query
canonical sizes before use. Import and ordinary CPU evaluator/projection usage
must still work with old cores lacking ABI3. Resources use inherited close;
`Hierarchy.info()/download()` returns plain owned nodes/roots for inspection.
`DiscoveryResult.download()` returns owned field/integer arrays plus retained
host source snapshots; lineage remains readable after native resource closure.
`DiscoveryBatch.require_gap()` checks only GAP status/gap flag, with its limited
squared-distance meaning. It is not named require_nearest or require_stationary.
Accessors expose UV/point/approximate distance and bracket distinctly.

Build timing includes CPU environment validation, complete interval construction,
allocation and upload even on CUDA. Resident discovery includes fresh output and
scratch allocation, traversal/refinement and synchronization; kernel profiling
is a separate field/scope. Full hierarchy download is an oracle/export cost,
not compact-query download. Host-consumed timing includes query snapshot/upload,
compact download, sources export and Python array/provenance construction.
Fixed internal refinement options are recorded in the public design/constants;
no Python iteration loop is permitted. Compare CPU1/24 and each P100 with exactly
the same controls, tree depth, target bits, budgets and numerical contracts.

No code or tests are written in this design input. Root integrates decisions,
then adversarial design and plan reviews precede implementation.
