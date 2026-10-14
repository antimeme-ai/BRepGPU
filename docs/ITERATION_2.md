# Build and use resident seeded projection

Iteration two refines provided parameter seeds against the owned rational Bézier patches. CPU and CUDA run the same bounded solver, with independent Bernstein and tensor de Casteljau evaluators. CUDA keeps jets and iterations on the selected device and downloads compact candidates. Python uses standard-library `ctypes` and `array`; no third-party Python or geometry library is required.

The supported boundary is Linux x86-64, native-endian binary64 and CPython 3.12. GPU execution targets P100 with CUDA 12 and real `sm_60` code. Read the reviewed [design](ITERATION_2_DESIGN.md), [implementation plan](ITERATION_2_PLAN.md), [public native header](../include/brepgpu_projection.h) and [results report](../results/projection-2026-10-14/REPORT.md) for the exact contract, qualification and measured timing scopes. The [first evaluator guide](ITERATION_1.md) remains applicable to `bg1_*` calls.

## Build

CPU-only builds require CMake 3.28 or newer and a C++17 compiler:

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DBREPGPU_ENABLE_CUDA=OFF
cmake --build build/release --parallel 4
export PYTHONPATH="$PWD/python"
export BREPGPU_LIBRARY="$PWD/build/release/libbrepgpu.so"
```

For the optional CUDA plugin, use the [qualified isolated toolkit](CUDA_TOOLCHAIN.md):

```sh
cad_cuda_root="$HOME/BRepGPU/toolchains/cuda-12.8.1/prefix"
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release \
  -DBREPGPU_ENABLE_CUDA=ON \
  -DCMAKE_CUDA_COMPILER="$cad_cuda_root/bin/nvcc" \
  -DCUDAToolkit_ROOT="$cad_cuda_root" \
  -DCMAKE_CUDA_ARCHITECTURES=60-real
cmake --build build/release --parallel 4
```

`libbrepgpu.so` owns the common registry and both CPU operations. An explicitly requested CUDA engine loads `libbrepgpu_cuda.so` beside the core. `Engine(library_path=...)` overrides core discovery; `Engine(plugin_path=...)` selects a plugin. CPU engines do not load or initialize CUDA. Missing CUDA is an explicit error; projection has no automatic CPU fallback.

## Project a batch from Python

Each patch uses homogeneous controls `(Xw,Yw,Zw,w)` in U-major, V-minor order. Degrees are 0..3, weights are in `[1,256]`, and each local Cartesian control coordinate lies in `[-1,1]`. Targets are local XYZ triples with finite components in `[-4,4]`; seeds are normalized `(s,t)` pairs in the closed unit square. Source UV rectangles, lineage, origin and scale remain metadata. The operation performs no world-target conversion, seed clipping, implicit reseeding or trim classification.

This plane has the exact candidate `(s,t)=(.25,.75)`, local point `(.25,.75,0)` and squared distance `.25`:

```python
from array import array
from brepgpu import Engine, Patch, ProjectionOptions

controls = (
    (0., 0., 0., 1.), (0., 1., 0., 1.),
    (1., 0., 0., 1.), (1., 1., 0., 1.),
)
with Engine(backend="cpu", threads=1) as engine:
    with engine.create_patch_set([Patch(1, 1, controls)]) as patches:
        with engine.create_projection_query_set(
            patches,
            patch_slots=array("I", [0]),
            targets=array("d", [.25, .75, .5]),
            seeds=array("d", [.75, .25]),
        ) as queries:
            with engine.project(queries, ProjectionOptions()) as result:
                batch = result.download()

batch.require_stationary()
print(batch.uv(0), batch.point(0), batch.distance_squared(0), batch.residual(0))
```

Use `Engine(backend="cuda", device=0)` or device 1 for either P100 separately. Uploaded patch/query snapshots can be reused by further `engine.project(queries, options)` calls. Each call allocates an independent result and synchronizes before returning; download is explicit. Public patch/query close revokes those handles while dependent results retain their exact source snapshots. Context close invalidates all resident children. Downloaded batches own their host arrays and remain usable after native resources close.

`ProjectionOptions(max_iterations=64, max_backtracks=24, gradient_tolerance=2.**-24)` is frozen. Limits are 0..128 accepted updates, 0..24 halvings after the initial full trial, and a finite tolerance in `[2^-32,2^-12]`. Zero updates still evaluates the seed; zero halvings still permits one full trial. Options are copied and retained with each result. A native batch error raises `BgError` (`MemoryError` for native allocation failure); a failed convenience guard raises `QueryError`. Wrapper construction failure cleans native publication while retaining the original exception and any cleanup diagnostic.

## Interpret candidates and statuses

`require_stationary()` checks the first-order status. A stationary saddle can pass this guard and carry a negative-curvature flag. Stationarity alone supplies no universal UV/spatial error bound or nearest-point guarantee. Several seeds for one target remain independent ordered rows; the operation supplies no complete minimum discovery, branch selection, seam deduplication, face trim membership or solid topology.

For `r=S-X`, `g=(r·Ss,r·St)` and `u=(s,t)`, the reported residual is `max |u-clip(u-g,0,1)|`, evaluated at the exact returned binary64 UV. It uses local objective/normalized-parameter coordinates. Squared distance and this residual have different meanings; a positive distance is compatible with stationarity.

| Status | Value | Numerical output |
| --- | ---: | --- |
| STATIONARY | 0 | Finite first-order candidate with rank admission and a curvature classification |
| BAD_PATCH | 1 | Invalid slot; all seven floating fields NaN, counters/flags zero |
| BAD_PARAMETER | 2 | Invalid target or seed; all seven floating fields NaN, counters/flags zero |
| ARITHMETIC | 3 | Unusable evaluation/arithmetic; floating fields NaN, attempted-work counters retained, flags zero |
| UNSUPPORTED | 4 | Reserved per-row unsupported-carrier outcome; the current patch creator enforces the supported envelope |
| ITERATION_LIMIT | 5 | Finite last accepted candidate, unresolved under requested budget |
| LINESEARCH_LIMIT | 6 | Finite last accepted candidate; trials exhausted without an accepted step |
| STAGNATION | 7 | Finite last accepted candidate; no admitted nonzero descent step |
| RANK_UNRESOLVED | 8 | Finite candidate whose evaluated metric failed numerical admission |

Flags are `CANDIDATE_VALID=1`, `BOUNDARY=2`, `RANK_DEFICIENT=4`, `ILL_CONDITIONED=8`, `CURVATURE_POSITIVE=16`, `CURVATURE_NEGATIVE=32`, and `CURVATURE_AMBIGUOUS=64`. Curvature bits are mutually exclusive and occur only on stationary rows. Their tolerance-relaxed feasible-direction classification concerns observed numerical quantities; positive curvature is no global certificate, negative curvature at a weak bound is no automatic exact nonminimum verdict, and rank flags are no proof of geometric singularity. Finite unresolved candidates keep CANDIDATE_VALID so a caller can inspect them without accepting them as stationary.

A `ProjectionBatch` exposes `data`, `statuses`, `iterations`, `evaluation_counts`, `flags`, copied `targets`/`seeds`, frozen `options` and `lineage`. `field(name_or_index, row)` uses names `s,t,x,y,z,distance_squared,residual`; `uv(row)`, `point(row)`, `distance_squared(row)` and `residual(row)` provide scalar access. Floating data has seven SoA planes, indexed `data[field*count+row]`; the four integer arrays have one entry per row. There are 72 result bytes per request before host provenance/allocation overhead, and 44 input bytes per request before patch storage and duplicated host/device snapshots.

## Native ABI and failure behavior

Include `brepgpu_projection.h`. New options/info/ABI-info descriptors use version 2 and canonical sizes 32/48/40 bytes. Shared `bg_diagnostic` stays ABI 1, 288 bytes. Existing `bg1_*` layouts, evaluator meanings and v1 plugin factory remain unchanged.

Create a projection query with `bg2_create_projection_query_set`, then use `bg2_project`, `bg2_projection_info`, `bg2_projection_download` and `bg2_projection_sources`; close every resource with `bg1_close`. `bg2_projection_sources` copies retained patch descriptors, slots, original targets/seeds and the exact options used by that result. Source/options metadata is host provenance rather than another GPU transfer. New/old query or result kinds cannot be substituted for each other.

All numeric extents are exact: N slots, 3N target doubles, 2N seed doubles, 7N output doubles and N elements in each integer slab. Nonempty pointers must be valid/aligned/stable during the call. Empty batches accept null numeric pointers and launch no kernel, while still validating handles and options. Descriptor byte capacities may exceed canonical size; `struct_size` remains exact. Initialize source-export options destinations with ABI 2/canonical size.

Download/source outputs must be mutually disjoint. Preflight rejection writes no payloads; diagnostic alias rejection writes neither the diagnostic nor any output. A later copy/completion/restoration failure can leave any destination partially written, so discard every output slab. Python publishes no batch on such failure. Fatal CUDA operation or cleanup state disables both evaluator and projection families in all contexts until process restart; CPU and completed host arrays remain usable.

The plugin's optional pure v2 extension must name the same v1 table that created context/patch tokens. An old core or v1-only plugin continues evaluator use and explicitly refuses projection. Stable capability refusal is distinct from propagated runtime/fatal errors; transient errors are not cached as unsupported.

## Qualify and inspect

Run from the checkout root with Python 3.12 and the environment above:

```sh
python3 scripts/qualify_projection.py --backend cpu --threads 24 \
  --output-directory results/local-projection/cpu
python3 scripts/qualify_projection.py --backend cuda --device 0 \
  --output-directory results/local-projection/cuda0
python3 scripts/qualify_projection.py --backend cuda --device 1 \
  --output-directory results/local-projection/cuda1
BREPGPU_TEST_BACKEND=cpu python3 -m unittest discover -s tests -v
BREPGPU_TEST_BACKEND=cuda BREPGPU_TEST_DEVICE=0 python3 -m unittest discover -s tests -v
BREPGPU_TEST_BACKEND=cuda BREPGPU_TEST_DEVICE=1 python3 -m unittest discover -s tests -v
```

The supported-host run passed 104 exact projection rows per backend with zero violations. The complete 43 public tests passed on CPU with two device-only skips and on each P100 with no skips. Compute Sanitizer ran that same suite on both cards with zero errors, zero leaked bytes and zero remaining allocations. These finite-corpus checks derive geometry/residual/classification independently; CPU parity is not their truth source.

Native checks are opt-in:

```sh
cmake -S . -B build/runtime-check -DCMAKE_BUILD_TYPE=Debug \
  -DBREPGPU_ENABLE_CUDA=OFF -DBREPGPU_ENABLE_TEST_HOOKS=ON \
  -DBREPGPU_ENABLE_SANITIZERS=ON
cmake --build build/runtime-check --parallel 4
ctest --test-dir build/runtime-check --output-on-failure
```

All 21 native cases passed in Release-hook and ASan/UBSan builds: 20 owned callback-model lifetime/loading/error cases plus one direct scalar numerical case. The model induces callback failures, without inducing hardware faults. Test hooks/model binaries are absent from default product builds. Compute Sanitizer checks real GPU execution separately.

## Measure and export a cloud

```sh
python3 benchmarks/projection/run.py --binary build/release/brepgpu_projection_benchmark \
  --output-directory results/local-projection/benchmark
python3 examples/project_cloud.py --backend cuda --device 0 --shape cylinder \
  --count 1024 --output-directory results/local-projection/cloud-cylinder
python3 examples/project_cloud.py --backend cpu --threads 24 --shape polynomial \
  --count 1024 --output-directory results/local-projection/cloud-polynomial
```

The default native matrix compares CPU1/CPU24 and each P100 across plane, tilted, cylinder, parabolic, mixed and adverse corpora at six counts from 1 to 1,048,576. It uses one excluded warmup and seven samples. For CPU-only sampling add `--cuda-devices ''`; use `--counts 1,32,256,4096` for a smaller matrix. It writes raw samples, summaries and work distributions, with direct checks outside timers.

`resident_project` includes fresh result allocation, execution, completion/restoration and public publication on reusable patch/query inputs. `download_prepared` copies all five slabs into prepared host buffers. `prepared_host_to_host` includes patch/query snapshot/upload, project, download and required status consumption in an existing context. Generation, independent geometric checks, cleanup, first context use and Python packing/materialization are separately reported. Profiled CUDA events run in a separate context and are not public wall latency. Avoid overlapping owned build/test jobs while collecting published timing samples.

The cloud caller exports target/seed/candidate records and a projected-segment view, with independent exact qualification and named preparation, upload, project, owned-array/provenance download, cleanup, qualification and file/visual export times. Its synthetic inputs incur no source-CAD export/import, topology or automatic fallback cost. Measured values and source/environment identity belong in the [results report](../results/projection-2026-10-14/REPORT.md); this guide makes no additional performance claim.
