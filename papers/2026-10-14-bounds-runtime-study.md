# Conservative discovery: packed residency and bounded work

Research recommendation, 2026-10-14; no product implementation or ABI declaration
is introduced here. Build the next operation around one named, positive-weight
rational patch and a target in that patch's existing local frame. Discover UV
candidates automatically through a resident conservative hierarchy, report a
valid squared-distance bracket, and say when finite budgets leave discovery
unresolved. A useful bracket does not imply a unique UV, a global nearest-point
answer, a trimmed-face answer or complete stationary-point enumeration.

## Reference reading and lessons

We read the pinned Warp hierarchy layout, host builder, CUDA refit and tile
traversal sources. Its packed node halves combine three float coordinates with
index/leaf bits; leaf ranges refer to a reordered primitive-index array. The
BVH borrows caller-owned item bounds. Its host builder stops subdividing at the
query stack's depth limit by collecting remaining primitives into a leaf. Its
CUDA refit publishes child bounds before a fence/counter sequence allows the
second child to propagate the parent. Those are concrete index-packing,
coverage-preservation and publication patterns. Borrowed lifetime and FP32 boxes
are not our FP64 carrier/enclosure contract.
[Warp layout](https://github.com/NVIDIA/warp/blob/28159649a2bb8b91e1badf692007c38a9f62d6b5/warp/native/bvh.h),
[builder](https://github.com/NVIDIA/warp/blob/28159649a2bb8b91e1badf692007c38a9f62d6b5/warp/native/bvh.cpp),
[refit](https://github.com/NVIDIA/warp/blob/28159649a2bb8b91e1badf692007c38a9f62d6b5/warp/native/bvh.cu),
[tile traversal](https://github.com/NVIDIA/warp/blob/28159649a2bb8b91e1badf692007c38a9f62d6b5/warp/native/tile_bvh.h).

SISL's `s1992` caches axis-aligned and additional rotated control-polygon boxes
on objects; `s1790` distinguishes disjoint, overlapping and degenerate box cases.
Its `s1958` combines a control-point parameter guess, local iteration and edge
checks, while explicitly disclaiming guaranteed closest-point discovery.
These sources separate exclusion and local candidate generation. They do not
supply an outward-rounded proof for our exact input carrier or an immutable
resident ownership model.
[SISL boxes](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s1992.c),
[box classifications](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s1790.c),
[closest-point routine](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s1958.c).

Karras's hierarchy construction makes the radix-tree build parallel, handles
identical Morton codes with index suffixes, and computes bounds bottom-up with
child counters. Its measurements use GTX 480/CUDA 4.0. Morton keys arrange work;
they do not define CAD identity or certify curve/surface bounds. We acquired and
read the free paper, especially Sections 3–4.
[Karras 2012](https://research.nvidia.com/sites/default/files/publications/karras2012hpg_paper.pdf).

Laine, Karras and Aila separate divergent path-tracing stages into kernels,
retain state in global SoA storage and route integer path IDs through
preallocated queues. They discuss both register-pressure benefits and queue,
launch and state-traffic costs. Their queues are sized to their finite path pool;
that capacity argument does not transfer to a subdivision process that emits
two children per input. We read Sections 2–5; their ray-tracing performance is
not evidence for our CAD operation.
[Megakernels Considered Harmful](https://research.nvidia.com/sites/default/files/pubs/2013-07_Megakernels-Considered-Harmful/laine2013hpg_paper.pdf).

Pinned source paths/hashes are in [bounds-runtime-entrypoints.tsv](bounds-runtime-entrypoints.tsv).
Existing archive restoration is in
[the roadmap source catalog](acceleration-roadmap-sources.tsv) and
[SISL restoration](first-iteration-numerical-restoration.md).
Acquired PDF URLs/hashes are in [bounds-runtime-pdfs.tsv](bounds-runtime-pdfs.tsv).
Warp is Apache-2.0; SISL includes AGPL-3.0. We studied patterns and did not adopt
or copy their implementation. The mandatory Wafer/GPU MODE acquisition and
actual-code audit are [a separate execution study](2026-10-14-wafer-gpu-mode-study.md).

## Proposed first operation and retained geometry

Admit a patch slot, finite local target XYZ, immutable hierarchy and bounded
options. Preserve the current control/weight envelope initially. A hierarchy
leaf represents a dyadic rectangle in the original normalized `[0,1]^2` chart;
its bound encloses the exact rational carrier defined by the supplied binary64
homogeneous controls over that rectangle. The surface remains untrimmed.

The initial useful result is one best sampled/refined candidate, its UV and
approximate point, a conservative squared-distance bracket `[L,U]`, explicit
candidate validity, work counters and an unresolved/reason status. A bounded
list of seeds can be an additional diagnostic/output, but a fixed list of K
seeds does not make enumeration complete. Reuse the owned projection numerical
helper internally to improve candidates; do not run a Python iteration loop or
mistake its `STATIONARY` status for global success.

Only compare candidates within the named patch's local metric. A patch-set
union requires an explicitly common comparison frame and metric: different
origins/scales cannot make their local squared distances comparable. World
conversion, source CAD export and transformed enclosure costs need their own
future contract. Source entity IDs and chart IDs remain provenance, not evidence
that a patch is a trimmed face.

Start with CPU interval preprocessing of a shallow binary U/V hierarchy,
then upload immutable boxes/IDs once. This obtains repeated-query residency
without first owning a GPU radix sort or interval-net construction scheduler.
It is a recommendation for the next design, not a claim that host preprocessing
is always cheaper. CPU construction time, peak host memory and upload are visible
costs. Parallel GPU construction becomes a useful later operation if those costs
are measured to dominate.

## Packed identity, layout and storage costs

Retain original controls in the existing patch token. Store physical node and
control offsets as checked integers. Device pointers may exist privately in a
backend token; public handles and chart/source identity must never be pointers.
Keep three identities distinct:

- Physical node index: a checked integer addressing packed arrays, allowed to
  change when nodes are reordered.
- Logical cell identity: patch slot plus `(u_level,u_index,v_level,v_index)` in
  the original chart. It survives scheduling/reordering and identifies shared
  closed boundaries without welding distinct source entities.
- Source identity: the patch's retained body/entity/revision/chart metadata.
  Store it once per patch and map through the slot, rather than copying strings
  or source records into every node.

One candidate node layout costs 64 bytes: six binary64 box endpoints (48), two
u32 children (8), and leaf/flags fields (8). Separate logical-cell metadata costs
roughly 16–24 bytes per leaf, depending on chosen index/depth widths. Validate
all index/range arithmetic, allocation products and node caps before allocation;
a depth encoding must reject shifts that exceed its integer width.

An interval homogeneous control record has four lower/upper pairs: 64 bytes.
A bicubic leaf's 16 records therefore cost 1,024 bytes before metadata. Keep
interval nets only when later adaptive refinement actually needs them; a fixed
leaf traversal can upload boxes alone and evaluate candidate UVs from retained
original controls. Do not put a whole interval net into every query thread
without measuring local-memory/register costs.

Binary depth d requires at most `2^d` leaves and `2^(d+1)-1` nodes; a full
quadtree requires `4^d` leaves. Build admission must cap storage. At a build cap,
retain a coarser enclosing leaf covering the remaining domain instead of
omitting it. Immutable rebuilding emits a new hierarchy; no in-place public
refit is needed initially. This keeps queries/results tied to the exact geometry
and enclosure method used to compute them.

## Coverage, upper bounds and budget exhaustion

The numerical lane owns the enclosure arithmetic. Runtime must preserve its
meaning. Rounded scalar child controls followed by a final one-ULP box expansion
are not an adequate substitute: accumulated subdivision/division error can
change the represented carrier. CPU rounding-environment and FTZ/DAZ handling,
CUDA directed operations, positive denominator enclosure and squared-distance
rounding all need the reviewed numerical contract before any bound claim.

A lower box-distance bound can prune only against a valid upper bound. A valid
UV is a candidate on the exact carrier; the ordinary FP64 evaluator's point and
squared distance alone are not an upper-bound certificate. Enclose the exact
carrier at that reported UV and round the target-distance upper bound outward.
Approximate solver output may propose UV; the enclosing evaluation supplies U.
Keep approximate distance, interval U and stationarity/classification separate.

Traversal maintains a covering frontier for every region not safely excluded.
Each pending or terminal unexcluded cell contributes a conservative lower
bound. Candidate limits cap what can be refined, not what domain exists. On a
visit, depth, frontier or candidate cap, retain an unresolved lower-bound
summary rather than silently dropping that region. A root lower bound is always
an available conservative fallback, though potentially loose. Tightened L must
account for all retained terminal/frontier regions and every region omitted
because of a budget. Equality cannot discard a tied nearest branch.

If candidate sampling still yields a certified finite U, a budget-limited query
can return a valid but wide bracket and explicit unresolved status. Without a
candidate, return the appropriate absent-candidate fields; do not invent U or a
successful nearest UV. A later distance-gap status may state only its precise
squared-distance criterion, with no uniqueness or complete-candidate implication.
Arithmetic inability to enclose a cell must disable unsafe pruning and report
unresolved arithmetic; it must not disappear as an empty child.

## Bounded queues and GPU scheduling

First prefer deterministic per-query bounded traversal over a persistent global
queue. A fixed depth-first stack is simpler to bound; order near children first
to obtain useful U early. A best-first frontier can improve pruning but costs
more memory and ordered maintenance. Whichever order is chosen, its exhausted
entries retain coverage and lower-bound information.

If a later wavefront implementation expands cells, derive queue capacity from
its actual producer bound. A binary subdivision can emit two cells; the
path-pool argument from the megakernel paper does not bound it. Preallocate a
finite work pool with checked counters and explicit overflow. Integer queue
entries route `(request,cell)` into packed state; publication must occur only
after the state they identify is readable. Kernel-stage boundaries are a simple
publication mechanism. Do not introduce spin-wait dependencies on blocks that
might not be scheduled concurrently.

P100 scheduling makes resource accounting concrete. GP100 has 64k 32-bit
registers and 64 resident warps per SM, maximum 255 registers per thread, and
64 KiB shared memory per SM with a 48 KiB block ceiling. A simple bounds kernel
plus queued solver can avoid imposing the solver's scratch needs on every
traversal thread, but adds global state/queue traffic and launch cost. These are
alternatives to measure, not an automatic wavefront speedup.
[Pascal tuning guide](https://docs.nvidia.com/cuda/pascal-tuning-guide/index.html#occupancy).

For example, N=1,000,000 requests with Q=32 frontier entries at 16 bytes each
require 512,000,000 bytes of frontier storage alone; a 24-byte entry requires
768,000,000. N times K candidate output also grows quickly. Count these bytes
in admission and benchmarks, and allocate once within a clearly reported scope.
A bounded dense output is a reasonable first ABI: exactly N query summaries and
N*K candidate slots with per-query counts. Unused slots have specified sentinels.
Ragged count/scan/fill output can follow when useful, with explicit capacities,
u64 offsets, overflow behavior and separate timing of its scan/fill stages.

Nsight Compute is not a qualified P100 tool: the current table excludes Pascal
and its first release excluded GP100. Use existing host/CUDA-event measurements,
compiler resource reports, binary inspection and independently qualified memory
checks. No Nsight Systems support or new profiler installation is inferred.
[Profiler applicability study](2026-10-14-wafer-gpu-mode-study.md#p100-applicability-and-profiler-support).

## Additive runtime and compatibility recommendation

A later public ABI3 can add hierarchy, discovery-query and discovery-result
kinds to the same core registry. Preserve all ABI1 evaluator and ABI2 projection
PODs/functions unchanged. A pure optional private extension factory validates
version/size and exact base-v1 table identity before callbacks are used; if it
exposes or requires the v2 extension, that identity must also match this plugin
instance. The same context, patch token, device stream and process-fatal state
serve all operation families. CPU-only operation does not load CUDA, and a
missing extension refuses only the new capability. Cache only stable capability
refusal/table success; transient/fatal backend errors propagate through existing
normalization and process poisoning.

A hierarchy owns its native arrays and retains its exact patch set; a query owns
its target snapshot/options and retains that hierarchy; a result retains that
query and compact resident outputs. Public close removes IDs while retained
objects still support needed cleanup and provenance. Context close invalidates
all public children, drains tokens under admission, and preserves fatal cleanup
flags. Per-host-thread device save/select/restore, synchronization before
publication, unpublished-token cleanup, and Python native-create/wrap-or-close
guards remain the same machinery, rather than a second resource registry.

New descriptor validation must use ABI3 explicitly while keeping ABI1 diagnostic
storage. Reject bad sizes/versions, alignment, extents, integer products,
wrong-kind/foreign/stale handles and output/diagnostic overlap before copying or
writing payloads. Queries/options use the same private snapshot for validation
and upload. Batch structural/allocation/backend failures publish no result ID;
per-query geometric budget exhaustion belongs in an otherwise valid result.
Download runtime failures classify resident state and unusable partial host
storage exactly as existing operations do.

## Benchmarks and direct fault classes

Compare CPU native 1/24 threads and each P100 on identical controls/targets,
hierarchy construction, numerical rules, visit/frontier/candidate budgets and
stopping criteria. Separate hierarchy construction, interval-control storage,
H2D upload, request snapshot/upload, traversal, optional candidate projection,
synchronization, result allocation, compact download and Python conversion.
Also report end-to-end host consumption and the number of repeated queries used
to amortize hierarchy setup. Resident timing includes required fresh result
allocation and synchronization; allocation reuse is a separate explicit scope.

Record node visits, depth/frontier high-water, pruned cells, candidate counts,
projection/evaluation calls, unresolved reasons and certified bracket widths.
A faster variant that discards unresolved branches cannot be compared as the
same operation. Schedule tests/compilers outside performance trials and do not
combine independent-device speeds into an unmeasured two-device throughput.

Direct qualification should attack exact-carrier containment, exact plane and
rational fixtures with analytic distances, hidden narrow minima, boundary/tied
regions, degree/weight extremes and subnormal arithmetic environments. A forced
one-entry frontier must retain a valid bracket with unresolved status; it must
not declare a nearest answer. Test integer/depth/output caps, unchanged output
on structural rejection, exact retained provenance after parent public close,
old v1/v2 binaries/plugins, and modeled allocation/fatal/cleanup failures through
the existing fault seam. CPU/GPU parity and sanitizers attack different faults
and cannot independently certify the geometric interval.

## Decisions needed before design

Choose the numerical enclosure primitive/environment contract; the exact
first-result fields and bracket-gap semantics; fixed versus adaptive hierarchy;
build-depth/node budgets; per-query visit/frontier/candidate caps; and whether
one best candidate or fixed-K candidates are public output. The runtime
recommendation is named-patch/local metric, CPU-built immutable binary hierarchy,
fixed bounded traversal, valid bracket plus explicit unresolved status, additive
capability negotiation and reuse of existing lifetime/fatal machinery. Automatic
seeds become useful immediately without claiming full B-rep proximity or topology.

Bibliography: [bounds-runtime.bib](bounds-runtime.bib). This is a design input,
not a reviewed API contract or a measured implementation result.
