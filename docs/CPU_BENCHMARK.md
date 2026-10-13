# CPU benchmark methods

The baseline measures the public Python API of NVIDIA USD BRep / SMLib 1.0.0,
using an owned standard-library driver. This runtime is a reference experiment.
It is not a selected dependency for the GPU kernel.

## Reproduction and provenance

Follow the [README commands](../README.md#run-the-cpu-baseline) on Linux x86-64
with CPython 3.12 and glibc 2.34 or newer. The
[requirements file](../benchmarks/cpu/requirements.txt) pins the
[PyPI wheel](https://pypi.org/project/usd-brep/1.0.0/) by SHA-256, with optional
dependencies disabled. The published run retains its exact driver bytes and
hash, runtime hash, hardware topology, environment, samples and model artifacts.

Each workload runs in a fresh subprocess pinned to logical CPU 22. One warmup is
excluded, followed by five measured samples. Every sample prepares fresh input;
setup, qualification and output cleanup happen outside the timed operation.
`perf_counter_ns` supplies wall time and `process_time_ns` supplies CPU time.
CUDA is hidden from the child processes and thread-count environment variables
are set to one. Each child has a 180-second timeout.

Failed qualification suppresses the workload's aggregate timing. The samples
retain operation duration and failure phase for diagnosis. The three sphere
meshes fail during their first, excluded warmup, so they have no qualified samples.

## Workloads and direct expectations

| Workload | Expected geometry |
| --- | --- |
| 100 boxes | 10 × 20 × 30 boxes, volume 6,000 each |
| 25 spheres | Radius 10, volume 4πr³/3 |
| 25 cylinders | Radius 3, height 10, volume 90π |
| 10 box booleans each | Overlapping 10³ boxes: union 1,500; intersection/difference 500 |
| Plates with 1, 16 or 64 holes | 64 × 64 × 6 plate; disjoint radius-1.5 through holes, volume 24,576 − nπ(1.5)²6 |
| 10 linear sweeps | Radius-3 circular profile swept by height 10 |
| 3 rotational sweeps | Rectangular profile revolved into radius-2-to-4 annular cylinder, height 5 |
| One edge fillet | Radius 2 along a height-30 edge of a 10 × 20 × 30 box; removed volume (1 − π/4)2²30 |
| 10,000 surface point calls | Analytic bicubic patch below |
| 10,000 surface derivative calls | Analytic patch points and first derivatives |
| 1,000 closest-point calls | Sphere queries checked against analytic radial projection |
| 3 sphere meshes | Radius 10; absolute chord tolerances 0.2, 0.05 and 0.01 |
| USD round trip | Export/import a curved 16-hole plate, including file I/O |

Solid checks use analytic volume with relative acceptance `1e-6` and absolute
floor `1e-8`, requesting integration accuracy `1e-8`. They also check tight
bounds and selected closest-point distances. Hole cases query the wall distance
at every hole center. A kernel manifold flag is an additional diagnostic.
These checks qualify the listed fixtures; they do not prove arbitrary B-rep
topological validity or geometric equivalence.

The unit-weight, clamped bicubic patch has control points
`P[i,j] = (4i, 3j, z)`, with `z = 3` for the four interior controls and zero
otherwise. On the unit parameter domain its independent analytic map is

```text
S(u,v)  = (12u, 9v, 27u(1-u)v(1-v))
Su(u,v) = (12, 0, 27(1-2u)v(1-v))
Sv(u,v) = (0, 9, 27u(1-u)(1-2v))
```

The sphere mesh oracle uses actual triangle closest points, oriented shared-edge
incidences, connected cyclic vertex links, Euler characteristic, radial vertex
positions and a sphere-specific volume envelope. It does not implement a general
triangle self-intersection detector. The current meshes stop at the first radial
witness; later checks are not claimed to have passed for these failed cases.
See the [finding and witnesses](TESSELLATION_FINDING.md).

## Interpretation limits

The machine has two Xeon E5-2697 v2 sockets, 24 physical cores and 48 logical
CPUs. Pinning does not reserve the SMT sibling, lock frequency or isolate host
activity. Five samples provide an exploratory baseline. Primitive batches,
surface evaluation and query loops include Python allocation and binding costs.
The surface-point workload varied from roughly 9.4 ms in an earlier local run to
25.1 ms in the published run; it should not stand in for native evaluator throughput.
The USD case includes filesystem and cache effects.

These are CPU reference timings, with no GPU speedup claim. Future comparisons
must report native CPU and GPU work, transfer-inclusive and resident-data paths,
batch size, synchronization, setup, fallback and qualification separately.

Hostname and absolute wheel path were removed from the published metadata.
Native operational logs remain local. Numerical samples and witnesses are
unchanged. [Published report](../results/cpu-2026-10-13/REPORT.md).
