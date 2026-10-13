# BRepGPU

**Building a GPU boundary representation kernel, one useful operation at a time.**

BRepGPU is an early research and engineering project working toward a solid
modeling kernel whose geometry and topology operations run on GPUs. The first
target is NVIDIA P100: FP64 geometry on CUDA 12, with an owned implementation.

A boundary representation retains surfaces, curves, trims and their connections.
That makes a cylinder an editable cylinder, with a display mesh derived from it.
Getting those connections right is as important as evaluating surfaces quickly.

The repository currently contains a runnable **CPU benchmark**, qualified results,
and a reproduced geometric failure. GPU implementation and performance results
are the next steps. The CPU measurements below exercise NVIDIA's newly released
[USD BRep / SMLib](https://github.com/NVIDIA-Omniverse/usd-brep), which is the
reference experiment here. They do not measure a BRepGPU kernel.

[Acceleration roadmap](docs/ACCELERATION_ROADMAP.md) ·
[First-iteration design](docs/ITERATION_1_DESIGN.md) ·
[Topology IDs and packed snapshots](docs/TOPOLOGY.md) ·
[Qualified P100 CUDA toolchain](docs/CUDA_TOOLCHAIN.md)

CUDA 12.8.1 is installed in an isolated prefix on the initial execution host.
Both 16 GB P100s passed a C++17 native `sm_60` FP64 setup probe and memory/leak
checking. That establishes the toolchain; GPU CAD product code is the next step.

## What is already measured

**16 of 19 workloads met their declared geometry checks** with USD BRep 1.0.0.
The baseline covers primitives, booleans, sweeps, fillets, bicubic surface
evaluation, closest points, tessellation and USD interchange.

Median wall time from five fresh-input samples after one excluded warmup, pinned
to one logical CPU on a dual Xeon E5-2697 v2 machine:

| Workload | Median | Range |
| --- | ---: | ---: |
| One through hole | 4.16 ms | 4.02–4.32 ms |
| 16-hole plate | 77.33 ms | 76.12–87.03 ms |
| 64-hole plate | 472.46 ms | 470.57–479.60 ms |
| Single-edge circular fillet | 4.51 ms | 4.49–4.78 ms |
| Curved model USD export and import | 4.77 ms | 4.62–5.48 ms |

Hole workloads time sequential subtraction of prepared cylinders. Setup and
geometry checks are outside the timed regions; the USD workload includes file
I/O. These are exploratory public-API measurements on a shared host. CPU clock,
SMT sibling activity and filesystem cache were not controlled.

[Full results and sample data](results/cpu-2026-10-13/REPORT.md) ·
[Benchmark methods](docs/CPU_BENCHMARK.md) ·
[Runnable driver](benchmarks/cpu/benchmark.py)

## A correctness finding

The three unqualified workloads are sphere tessellations. For a radius-10 sphere
with both angular criteria disabled, points on emitted triangles exceed the
requested chord tolerance:

| Requested maximum distance | Observed witness distance |
| ---: | ---: |
| 0.2 | 0.220275 |
| 0.05 | 0.057393 |
| 0.01 | 0.014526 |

The kernel reports successful tessellation. The benchmark retains a triangle,
a point on that triangle and its barycentric coordinates, so the discrepancy can
be checked against the analytic sphere. These are witnesses, not measured global
maximum errors. The finding is specific to this release and these settings.

[Reproduction and witnesses](docs/TESSELLATION_FINDING.md)

## How selective acceleration becomes a kernel

The intended architecture gives each accelerated stage reusable data and error
contracts. Geometry starts in contiguous buffers with stable entity identities;
GPU work produces bounded results or explicit unresolved cases. Topology can
initially be assembled on the CPU while the numerical pipeline develops.

| Stage | Useful deliverable | What it builds toward |
| --- | --- | --- |
| Geometry storage and evaluation | Batched FP64 points and derivatives | Shared device representation and CPU reference contracts |
| Bounds and geometric queries | Subdivision, candidate filtering, projection | Work queues for irregular geometry |
| Surface intersections | Curves in 3D and both surfaces' parameter domains | Complete intersection branches and consistent trims |
| Face splitting and classification | Boundary pieces selected by Boolean operations | Device topology construction with shared edges |
| Solid assembly and features | Closed solids, then sweeps, fillets and offsets | A progressively broader GPU modeling kernel |

The proposed first prototype is deliberately small: positive-weight rational
Bézier patches through bidegree (3,3), with FP64 positions and first/second
derivatives on a P100. Analytic polynomial and exact rational cylinder fixtures
qualify the results, alongside an independent CPU reference. Both resident-data
and transfer-inclusive timings matter. Tiny loops,
tangent contacts, periodic seams and coincident surfaces enter the design early.

This is the research direction. The implementation scope and numerical contracts
will come from the first research reports and design review.

## Run the CPU baseline

Linux x86-64, CPython 3.12, glibc 2.34 or newer:

```sh
python3.12 -m venv .venv
.venv/bin/python -m pip install --only-binary=:all: --no-deps \
  --require-hashes -r benchmarks/cpu/requirements.txt
CUDA_VISIBLE_DEVICES= .venv/bin/python benchmarks/cpu/benchmark.py \
  --output results/local-cpu
```

The driver chooses an available logical CPU by default; use `--cpu N` to select
one. Use a fresh output directory. A run exits nonzero when any workload fails
qualification, while preserving the report and completed samples. The published
baseline includes such failures deliberately.

## Research and contributions

The first three frumentarii reports cover GPU numerical geometry,
intersection/topology machinery and geometric correctness oracles.
[Research and bibliographies](papers/README.md) ·
[Projects to inspect](papers/PROJECTS.md) · [Open work](ISSUES.md).

Useful contributions include small analytic fixtures, difficult intersection
cases, relevant papers, and independently reproducible findings. State what the
case should produce and how that expectation is known.

Owned code is [MIT licensed](LICENSE). NVIDIA USD BRep and other reference
projects retain their own licenses. Collecting or benchmarking a reference does
not select it as a production dependency.
