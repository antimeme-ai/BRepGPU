# Canonical native point filter

Actual fleet Release binary, Clang 18.1.3. Each row evaluates 1024 complete
original-source points with one reused owner context, including argument
creation, source formula arithmetic, unique nearest-even XYZ admission and
checksum. The exact reference uses the unchanged public original enclosure
formula/refinement path; the selected point path tries fixed 128/192/256 then
that same exact fallback. All four channels have identical point-word checksums.

| Original source family | Exact reference ms | Taylor ms | Chebyshev ms | Selected ms |
| --- | ---: | ---: | ---: | ---: |
| Unit circle, t in[0,1) |4337.79|221.22|207.59|207.49|
| Nonorthogonal ellipse, t in[0,6) |5608.55|457.25|467.74|501.85|
| Nonorthogonal torus, two parameters |9999.91|1021.42|983.00|924.88|

Selected/reference reductions in this run are 20.9x, 11.2x and 10.8x respectively.
These are point-operation observations on the shared fleet, not whole-mesher
speedups or statistical latency guarantees. Rows run sequentially while other
integration work can use the machine. Taylor/Chebyshev wall differences are
mixed; Chebyshev uses fewer charged exact/limb operations in every family and
retains a uniform lower-degree default. Both certified controls remain callable.
The raw rows and preceding all-endpoint-product run are retained alongside this
report. No GPU work occurs in canonical point construction.

`peak_bytes` in the raw TSV is the owned Budget's heap peak, not process RSS;
bounded fixed-limb stack arrays are additional. `work` is the existing charged
operation counter, not a hardware instruction count. Source and binary SHA256,
compiler and CPU details are in environment.txt. Inputs are independent
geometric families with arbitrary original frames, not watch-specific constants.

Qualification: genuine missing-API RED; 249 independent rational-angle vectors
across all profiles and both polynomials; actual correct-nearest circle source
XYZ; signed directed truncation, carry/overflow and all interval sign cases.
Strict Release passed 1.33s; ASan/UBSan/leaks passed 57.80s. Existing actual
analytic, jets, point-jets and metric fixtures passed after the shared formula
refactor. Public explicit-precision bound/metric arithmetic remains unchanged.

Reproduce:

```
cmake -S native/mesher -B build/point-trig-unit-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++-18 -DBREPGPU_MESHER_BUILD_BENCHMARKS=ON
cmake --build build/point-trig-unit-release --target brepgpu_native_mesher_point_trig brepgpu_mesher_bench_point_filters -j4
ctest --test-dir build/point-trig-unit-release -R '^native_mesher_point_trig$' --output-on-failure
build/point-trig-unit-release/brepgpu_mesher_bench_point_filters
```

Arithmetic/error derivation is in
papers/2026-10-16-native-point-filter-arithmetic.md. No minimax assertion,
new dependency, relaxed rounding policy or changed source geometry is involved.
