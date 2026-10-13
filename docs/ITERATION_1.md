# Build and run the first evaluator

Iteration one evaluates positive-weight rational Bézier patches through degree
(3,3) in FP64. It returns positions, first derivatives and second derivatives
in normalized local coordinates. CPU and CUDA implement the same native batch
interface. Python uses standard-library `ctypes` and owned `array` buffers;
there are no third-party Python dependencies.

The initial supported ABI is Linux x86-64, native endian binary64 and CPython
3.12. CUDA targets P100 / `sm_60`; use the qualified CUDA 12.8.1 compiler.
The [design](ITERATION_1_DESIGN.md) specifies the input envelope, numerical
grades, frame conversion limits and lifetime rules. The
[implementation plan](ITERATION_1_PLAN.md) records the reviewed work units.

## Build

CPU only requires CMake 3.28 or newer and a C++17 compiler:

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel 4
```

To include the optional CUDA plugin, select your isolated toolkit prefix:

```sh
cad_cuda_root="$HOME/BRepGPU/toolchains/cuda-12.8.1/prefix"
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release \
  -DBREPGPU_ENABLE_CUDA=ON \
  -DCMAKE_CUDA_COMPILER="$cad_cuda_root/bin/nvcc" \
  -DCUDAToolkit_ROOT="$cad_cuda_root" \
  -DCMAKE_CUDA_ARCHITECTURES=60-real
cmake --build build/release --parallel 4
```

`libbrepgpu.so` contains the public registry and CPU evaluator.
`libbrepgpu_cuda.so` is loaded lazily from the same directory for an explicitly
requested CUDA engine. Missing CUDA support is an error for that engine.
`BREPGPU_LIBRARY` or `Engine(library_path=...)` selects a different core path;
`Engine(plugin_path=...)` selects a different plugin path. A CPU engine does
not load or initialize CUDA.

## Qualify and sample

```sh
export PYTHONPATH="$PWD/python"
python3 scripts/qualify_iteration1.py --backend cpu \
  --output-directory results/local-iteration1/cpu
python3 scripts/qualify_iteration1.py --backend cuda --device 0 \
  --output-directory results/local-iteration1/cuda0
BREPGPU_TEST_BACKEND=cpu python3 -m unittest discover -s tests -v
BREPGPU_TEST_BACKEND=cuda BREPGPU_TEST_DEVICE=0 \
  python3 -m unittest discover -s tests -v
python3 examples/sample_patch.py --backend cuda --device 0 \
  --output-directory results/local-iteration1/sampler
```

The sampler evaluates a rational cylinder carrier in one request; use
`--shape polynomial` for the owned bicubic polynomial carrier. It exports
XYZ points, an SVG parameter grid, exact input controls and query parameters,
and an analytic position/derivative qualification. These are untrimmed samples;
no triangle chord tolerance or closed-solid claim applies.

The exact corpus uses supplied binary64 input ratios and independent analytic
maps. It covers all sixteen degree pairs and all three output orders. Caller
tests separately check invalid-query NaNs, snapshot ownership, retained source
identity, closed handles, frame conversion and native ABI rejection.

## Measure

```sh
python3 benchmarks/iteration1/run.py --binary build/release/brepgpu_benchmark \
  --output-directory results/local-iteration1/benchmark
```

The default matrix has 144 cases: six query counts from 1 to 1,048,576; three
orders; one coherent patch or sixteen shuffled patches; native CPU with one
or 24 workers; and each P100 separately. Each case has one excluded warmup and
seven measured samples. Every downloaded batch must pass expanded polynomial
position/derivative checks. Both backends use the same dyadic control nets,
query bits, output fields and public C API. The sixteen-patch case represents
a small resident set, not a large imported model.

`resident_evaluate` includes fresh result allocation, checks, native execution,
completion and handle publication. `download` measures copying results and
statuses into prepared host buffers. `prepared_host_to_host` includes native
patch/query snapshot creation and upload, evaluation, download and status
consumption within an existing context. Input generation and the independent
analytic checks stay outside those timers. Child cleanup and first context
creation are separate rows. Python packing/dispatch and context initialization
are not included in the prepared native host-to-host measurements.

CUDA event durations use a separate context with profiling enabled. They bracket kernel submission and may include stream scheduling or submission
gaps; they are not public API latencies or host-to-host speedups. Normal
public timings run without event instrumentation. Native CPU workers start and
join inside each evaluate call. The benchmark does not pin cores, isolate
other workloads, control clocks or guarantee that 24 workers occupy 24
physical cores.

The full jet writes 148 bytes per query including status. Download can therefore
dominate total latency at large counts. Useful next callers should consume
resident jets directly where possible. This evaluator is the first numerical
stage; it does not yet accelerate SMLib booleans, trim classification, topology
construction or complete CAD modeling tasks.

## Native failure checks

```sh
cmake -S . -B build/runtime-check -DCMAKE_BUILD_TYPE=Debug \
  -DBREPGPU_ENABLE_TEST_HOOKS=ON -DBREPGPU_ENABLE_SANITIZERS=ON
cmake --build build/runtime-check --parallel 4
ctest --test-dir build/runtime-check --output-on-failure
```

These six fresh-process scenarios use an owned callback model to exercise
admission/close races, allocation failure before publication, cleanup and
download fatality, and device-restoration failure reporting. They verify the
core's lifetime and error paths under ASan/UBSan. They do not induce actual
hardware faults. Test hooks and the callback model are absent from the default
product build; Compute Sanitizer separately checks the real P100 kernels.
