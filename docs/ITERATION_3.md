# Automatic bounded surface discovery

The third owned operation finds a candidate on a named untrimmed rational
patch without a supplied UV seed. It returns a conservative squared-distance
bracket, the candidate's UV and descriptive XYZ, and independent precision,
coverage and work-budget flags. The C++17 CPU core, CUDA `sm_60` backend and
standard-library Python adapter implement the same contract.

Implementation, independent code review and direct qualification are complete
on the supported Linux x86-64 host and both P100s.
[Retained qualification receipts](../results/discovery-2026-10-14/QUALIFICATION.md)
include exact geometry, memory, compatibility, mutations and actual callers.
The [444 native comparisons](../results/discovery-2026-10-14/REPORT.md) include
CPU1, CPU24 and both P100s, with setup, transfers, work and attained precision.
The [reviewed design](ITERATION_3_DESIGN.md) and
[implementation plan](ITERATION_3_PLAN.md) specify the operation in detail.
No modeling-task or full B-rep kernel speedup follows from this query operation.

## Call the operation

Use the [native build instructions](ITERATION_2.md) and the
[qualified CUDA toolchain](CUDA_TOOLCHAIN.md). For a CPU-only discovery build:

```sh
cmake -S . -B build/discovery-release -DCMAKE_BUILD_TYPE=Release \
  -DBREPGPU_ENABLE_CUDA=OFF
cmake --build build/discovery-release --parallel 4
export BREPGPU_LIBRARY="$PWD/build/discovery-release/libbrepgpu.so"
export PYTHONPATH="$PWD/python"
```

Keep that environment setting for the Python example and qualification commands
below; it selects the new core explicitly. For CUDA, configure the same folder
with the optional backend enabled as described in the linked build guide.

Then call the operation:

```python
from brepgpu import Engine, Patch, HierarchyOptions, DiscoveryOptions

plane = Patch(1, 1, [
    0., 0., 0., 1.,  0., 1., 0., 1.,
    1., 0., 0., 1.,  1., 1., 0., 1.,
])
with Engine(backend="cpu", threads=1) as engine:
    with engine.create_patch_set([plane]) as patches:
        with engine.create_hierarchy(
            patches, HierarchyOptions(depth=8, mode="aabb")
        ) as hierarchy:
            with engine.create_discovery_query_set(
                hierarchy, [0], [.25, .75, .5]
            ) as queries:
                with engine.discover(queries, DiscoveryOptions()) as result:
                    batch = result.download()
                    print(batch.uv(0), batch.bracket(0), batch.flags[0])
```

Use `Engine(backend="cuda", device=0)` or device 1 to select a P100. Selection
is explicit; an unavailable CUDA capability does not silently run CPU work.
The optional discovery ABI3 is negotiated lazily, so older cores/plugins can
still perform their supported evaluation/projection operations.

The patch stores homogeneous `[Ax, Ay, Az, W]` controls with v varying fastest.
Degrees are 0 through 3 in each parameter, W is in [1,256], and each
`abs(Ak)<=W`. Cartesian controls therefore lie in the local [-1,1] cube.
Targets lie in [-4,4]^3; UV lies in the closed [0,1]^2 square. These are the
existing local numerical coordinates, rather than imported model units.
Constant patches, rank-deficient curves, folds and continua are admitted.

## Three literature-informed bounds

The immutable CPU-built binary dyadic hierarchy bounds restrictions of the
original binary64 input patch using outward homogeneous interval subdivision.
CUDA uploads the resulting packed arrays once. Per-query traversal, local
refinement and original-patch interval witness evaluation remain on the device.

| Mode | Query lower bound | Additional cache per node |
| --- | --- | ---: |
| `aabb` | Sum of squared distances to Cartesian interval axes | 0 bytes |
| `dop26` | Maximum of AABB and ten additional diagonal support-slab bounds | 160 bytes |
| `bernstein` | Maximum of AABB and rational squared-distance Bernstein coefficient bounds | 3,920 bytes |

All nodes additionally use an 80-byte record, and each patch root uses four
bytes. A degree-positive two-axis depth8 tree has 511 nodes: approximately
40 KiB, 120 KiB or 2 MiB respectively. These are native cache payloads; host and
device ownership, original controls and allocation overhead are additional.
Complete-tree depth is a resource choice, rather than an accuracy guarantee.

The DOP cache bounds a homogeneous directional expression before division by
W, retaining coordinate correlation. It divides squared support separation
by the direction's exact squared norm. The Bernstein cache contains five
query-independent coefficient nets for sum(Ak²), AxW, AyW, AzW and W². The
query combines these into the target's squared-distance numerator. Negative
numerator coefficients remain valid; inactive padding is excluded. Neither
method is selected solely because it prunes more nodes. Setup, cache memory,
full-operation latency and achieved precision determine its useful cases.

The [strong-bound design study](../papers/2026-10-14-discovery-strong-bounds-design.md)
and [independent oracle study](../papers/2026-10-14-strong-bounds-oracle-study.md)
record the computational-geometry sources and their admitted assumptions.
These are owned implementations, without adopting reference libraries.

CUDA query input uses 28 bytes per row, and compact output uses 88 bytes per
row. The hierarchy/cache upload is preprocessing. These transfer sizes describe
same-machine host RAM to/from GPU VRAM over PCIe, rather than network traffic.
Python snapshots and provenance add separately measured host copying costs.

## Interpret an answer

For a valid row, `batch.bracket(i)` returns `(L2, U2)` with
`L2 <= exact global squared distance <= U2`. The upper endpoint also bounds
the exact distance at the returned binary64 UV on the original input patch.
The descriptive XYZ and approximate squared distance have separate evaluator
error grades and do not supply either certificate endpoint.

| Flag | Meaning |
| --- | --- |
| `BG_DISC_CANDIDATE_VALID` | The returned UV is a valid patch-domain witness |
| `BG_DISC_GAP` | The upward squared bracket width satisfies `tolerance_squared` |
| `BG_DISC_COVERAGE_COMPLETE` | Traversal discharged every subtree by a bound or retained leaf |
| `BG_DISC_VISIT_LIMIT` | Visit budget stopped traversal; the original root lower bound is retained |
| `BG_DISC_REFINEMENT_LIMIT` | An eligible local refinement was withheld by its budget |
| `BG_DISC_ARITHMETIC_REASON` | An arithmetic/invariant failure prevented a valid answer |

Precision and coverage are independent. A capped root bracket can already meet
the tolerance; complete traversal can retain a wide bracket. A valid candidate
can remain unresolved. `batch.require_gap()` raises on any row without gap
success; it does not require complete traversal or enumerate all minimizers.
Gap success bounds squared distance, without establishing unique or accurate UV.

Invalid slot and target rows have distinct statuses. Arithmetic failures expose
eight NaNs and their actual counters; they cannot be mistaken for a zero-distance
candidate. Visit counts measure node-bound calculations, including child bounds
used for ordering. Pruned counts measure discharged subtree roots, rather than
number of skipped leaves. Evaluation counts cover ordinary evaluator callbacks;
interval witnesses are separately accounted as one plus refinement
calls for a valid row.

## Ownership and budgets

Patch creation snapshots controls; the hierarchy retains that immutable patch
snapshot. Query creation snapshots patch slots and targets. Results retain
original descriptors, slots, targets and both option
records. Closing a parent resource does not invalidate an already published
child. Closing the context invalidates its shared handle graph. The API exports
packed node IDs and dyadic cell/source identity rather than device pointers.

Default hierarchy depth is 8, with complete admission under `max_nodes`.
Depths 0 through 12 are admitted; no partial-cache fallback is published.
Default query limits are 8,191 node calculations and eight refinements, with
`tolerance_squared=2**-24`. Each internal projector uses at most 32 updates and
24 backtracks per update. Refinement budget zero is useful for bounds-only
comparisons while retaining the initial midpoint witness.

Nonempty CPU arithmetic requires the qualified rounding/exception environment,
including round-to-nearest and gradual underflow, on the builder and actual
workers. Unsupported modes are refused rather than modified. Empty structural
operations do not run arithmetic workers. CUDA uses explicit directed FP64
operations for certificate arithmetic.

## Reproduce direct qualification

Use fresh output directories and an explicitly selected backend:

```sh
PYTHONPATH=python python3 scripts/qualify_discovery.py --backend cpu \
  --threads 24 --mode all --output results/local-discovery-cpu
PYTHONPATH=python python3 scripts/qualify_discovery.py --backend cuda \
  --device 0 --mode all --output results/local-discovery-cuda
```

The independent Fraction oracle expands original homogeneous controls in a
power basis, restricts by exact dyadic substitution, and converts to Bernstein
coefficients. It checks actual exported nodes/caches and exact returned-UV
upper certificates. It does not copy product subdivision, traversal or pruning.
Analytic global answers, flat leaf reductions, degeneracies and explicit caps
attack coverage and precision separately. Successful status, CPU/GPU agreement
and a rendered figure are insufficient qualification.

The native benchmark driver is `benchmarks/discovery/run.py`. It retains raw
phase timings, work histograms, cache bytes and precision populations. Native
timed rows check exact dyadic analytic global brackets and exact binary gap
widths; expanded analytic witness checks have a stated rounding allowance.
Exact original-UV upper qualification belongs to the independent Fraction
corpus and actual caller, rather than those descriptive native checks.

This operation supplies automatic candidates and bounds for later proximity,
pair discovery and intersection work. Cross-patch queries, imported frames,
trims, complete minimizer enumeration and topology mutation require subsequent
explicit contracts.
