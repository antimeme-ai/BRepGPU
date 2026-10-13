# USD BRep CPU baseline — 2026-10-13

USD BRep 1.0.0; logical CPU 22; 1 excluded warmup and 5 fresh-input samples per case. Timings exclude setup and validation; I/O cases include I/O. See environment.tsv and cpu-topology.txt for hardware/load/affinity.

| Workload | Result | Median wall ms | Range ms | Median CPU ms |
| --- | --- | ---: | --- | ---: |
| box_100 | pass | 94.088 | 90.074–94.910 | 94.105 |
| sphere_25 | pass | 61.879 | 60.306–62.397 | 61.890 |
| cylinder_25 | pass | 101.321 | 99.119–105.206 | 101.326 |
| union_boxes_10 | pass | 33.911 | 33.820–39.500 | 33.918 |
| intersection_boxes_10 | pass | 29.565 | 29.146–30.388 | 29.571 |
| difference_boxes_10 | pass | 30.534 | 29.184–36.127 | 30.541 |
| bore_1 | pass | 4.158 | 4.020–4.319 | 4.162 |
| bore_16 | pass | 77.333 | 76.123–87.029 | 77.345 |
| bore_64 | pass | 472.456 | 470.568–479.604 | 472.315 |
| linear_sweep_10 | pass | 44.549 | 43.398–53.449 | 44.557 |
| rotational_sweep_3 | pass | 17.144 | 13.581–17.198 | 17.143 |
| fillet_edge | pass | 4.513 | 4.487–4.781 | 4.517 |
| surface_eval_10000 | pass | 25.099 | 22.821–27.044 | 25.094 |
| surface_derivatives_10000 | pass | 19.203 | 17.475–23.151 | 19.209 |
| closest_sphere_1000 | pass | 24.731 | 24.596–26.637 | 24.741 |
| tess_sphere_0.2 | failed | — | —–— | — |
| tess_sphere_0.05 | failed | — | —–— | — |
| tess_sphere_0.01 | failed | — | —–— | — |
| usd_roundtrip_bore16 | pass | 4.767 | 4.617–5.480 | 4.771 |

Failed/invalid cases have no qualified timing. All samples, phase markers, generated model artifacts and triangle witnesses are published per case. Native operational logs remain local. Primitive/Boolean batches and scalar surface/query cases include Python loop and binding overhead; elapsed time is per complete named workload. Five samples are an exploratory baseline on a shared host, not a confident ranking. Affinity constrains one logical CPU, without reserving its SMT sibling or fixing clock speed.

## Findings

- **tess_sphere_0.2**: failed, last phase validation 0; AssertionError: sphere triangle violates radial bound: radius=9.779725027351045; chord=0.2
- **tess_sphere_0.05**: failed, last phase validation 0; AssertionError: sphere triangle violates radial bound: radius=9.942606814692091; chord=0.05
- **tess_sphere_0.01**: failed, last phase validation 0; AssertionError: sphere triangle violates radial bound: radius=9.985473881610929; chord=0.01

The tessellation oracle checks sphere-specific radial/volume bounds and mesh incidence/link/orientation/connectivity; it does not implement a general triangle self-intersection detector. Kernel manifold flags are additional diagnostics. USD round-trip topology counts are diagnostic, not equality requirements.

## Provenance and publication

The retained [driver](benchmark-source.py) is byte-identical to the [runnable driver](../../benchmarks/cpu/benchmark.py); its SHA-256 is recorded in [environment.tsv](environment.tsv). The runtime wheel is pinned by hash in [requirements.txt](../../benchmarks/cpu/requirements.txt). Hostname and absolute wheel path were removed from published metadata; numerical data is unchanged. These results measure USD BRep 1.0.0 on CPU. No BRepGPU implementation or GPU speedup is measured.

[Methods and qualification limits](../../docs/CPU_BENCHMARK.md) · [Tessellation finding](../../docs/TESSELLATION_FINDING.md)
