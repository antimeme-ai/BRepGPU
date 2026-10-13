# GPU numerical geometry: an owned FP64 starting point

Research reconnaissance, 2026-10-13. This report proposes a first numerical unit for BRepGPU. It does not describe implemented GPU modeling or measured speedups. Hardware recommendations are checked against NVIDIA documentation; algorithm proposals below are our synthesis, with source-specific findings identified separately.

The useful first unit is **batched rational Bézier surface evaluation with first and second derivatives**, followed by bounded subdivision and inverse queries. These values feed tessellation, closest-point searches, surface intersection and eventually fillet construction. The same immutable geometry storage can remain on the device while those consumers develop. Topological mutation requires its own design and cannot be established by a fast evaluator.

## What changed in the literature

| Primary source | Read and acquired | Consequence for this project |
| --- | --- | --- |
| [Chudy and Woźny, *Fast evaluation of derivatives of Bézier curves*, 2023; revised February 2024](https://arxiv.org/abs/2305.18874) | PDF; algorithm sections 2–3, experiments and conclusion | Linear-time geometric alternatives to de Casteljau exist for polynomial and rational derivative evaluation. Benchmark them as later evaluator candidates. |
| [Woźny and Chudy, *Fast subdivision of Bézier curves*, 2025; revised April 2026](https://arxiv.org/abs/2509.15691) | PDF; sections 3–6 | FFT subdivision has improved asymptotics but numerical conditioning and small-degree overhead matter. It is not the obvious first cubic kernel. |
| [Valasek and Horváth, *Register-Efficient Linear-Time Evaluation in the Bernstein Basis*, 2026](https://doi.org/10.1111/cgf.70403) | Publisher full text: sections 4–8; PDF acquisition unsuccessful | Directly studies GPU register pressure, constant-storage evaluators and nested FMA interpolation. An unusually relevant optimization reference. |
| [Anas and Wolfe, *A Texture Lookup Approach to Bézier Curve Evaluation on the GPU*, 2026](https://arxiv.org/abs/2603.15447) | PDF and HTML, interpolation, limitations and experiments | Hardware interpolation can evaluate curves economically, but its quantized coordinates conflict with this project's FP64 modeling contract. |
| [Ye et al., *gMidSurf*, 2026](https://min-tang.github.io/gMidSurf/) | Author PDF; pipeline, midpoint solver and evaluation sections | A concrete recent example of selective GPU CAD: geometric work on device, fitting and trimming on host. |
| [Sloup and Havran, *Optimizing Ray Tracing of Trimmed NURBS Surfaces on the GPU*, 2021](https://dcgi.fel.cvut.cz/~havran/ARTICLES/slouphavran2021pg.pdf) | Main PDF and supplement; numerical intersection and trimming sections | Trimming is a separate parameter-space problem; good rendering throughput cannot certify modeling intersections. |

The 2023/24 derivative paper reuses geometric evaluation recurrences and compares multiple ways to obtain derivative cohorts. Its experiments cover rational curves as well as polynomial ones. It explicitly leaves rigorous numerical error analysis and useful rational derivative bounds as further work. The empirical improvement is a candidate implementation strategy, not an error certificate. [Paper](https://arxiv.org/abs/2305.18874).

The 2025/26 subdivision paper turns the subdivision transformation into convolution, then FFT/IFFT evaluation. Its direct formulation is unstable; scaling improves observed behavior. The current experiments use NumPy and treat de Casteljau output as exact. Their scaling choice is empirical, and worst accuracy worsens with degree even when average accuracy stays high. For cubics, an owned de Casteljau split is easier to analyze and avoids FFT machinery; revisit high degrees after measuring an actual workload. [Paper](https://arxiv.org/abs/2509.15691).

The 2026 register-efficient paper develops constant-storage ladder evaluators for points, derivatives and surfaces. It compares interpolation realizations and advocates a nested FMA form that preserves endpoints; subtract-then-FMA interpolation can lose the endpoint after rounding the subtraction. Its GPU experiments use an RTX 4500, not Pascal. Study the method and error analysis, then compare it against a simple FP64 implementation on P100. Its error experiments include binary64 polynomial curves against exact rational truth; those results do not determine our rational surface second-derivative behavior. [Publisher full text](https://onlinelibrary.wiley.com/doi/10.1111/cgf.70403).

The 2026 texture paper encodes polynomial interpolation in small textures, including Seiler's formulation. It documents quantization of interpolation coordinates, commonly with eight fractional bits, and workload-dependent performance. Its measured graphics workloads are not FP64 CAD queries. Treat it as a rendering or coarse candidate-generation idea; do not use texture-filtered values as the numerical modeling result. The preprint includes publication-template placeholders, so publication status should be checked separately from the paper's substantive content. [Paper](https://arxiv.org/abs/2603.15447).

gMidSurf accelerates face pairing and distance-based midpoint generation, while fitting and trimming stay on the host. Its geometry starts with discretized CAD faces; bracket-and-bisect refinement and hierarchical culling drive the numerical stages. Results on RTX 5090D are relevant architecture evidence for keeping useful stages resident, not transferable P100 performance estimates. The method is a midsurface pipeline, not a general Boolean kernel. [Author page](https://min-tang.github.io/gMidSurf/), [author PDF](https://min-tang.github.io/gMidSurf/files/gMidSurf_CAD.pdf).

Sloup and Havran separate base-surface ray intersection from parameter-space trimming. Their supplement discusses Newton initialization through subdivided patches, lack of universal convergence and alternatives such as Bézier clipping and interval operators. The main experiments target RTX 2080 Ti and RTX 3090 using CUDA 11.2. It is valuable for data organization and failure analysis, with rendering-oriented acceptance criteria that need strengthening for CAD construction. [Main paper](https://dcgi.fel.cvut.cz/~havran/ARTICLES/slouphavran2021pg.pdf), [supplement](https://dcgi.fel.cvut.cz/~havran/ARTICLES/supplementaryPG21SloupHavran/supplementaryPG21SloupHavran.pdf).

## The first numerical contract

Proposed input: immutable, rectangular rational Bézier patches of bidegree at most (3,3), finite homogeneous control points and **strictly positive finite weights**. Include lower degrees for exact planar and rational quadratic examples. Support arbitrary parameter rectangles through an explicit affine map. This narrow scope avoids knot searching and unsupported-degree behavior in the first CUDA launch. Arbitrary-degree NURBS extraction follows after the primitive is qualified.

Each request identifies a patch and a parameter pair. Emit position `S`, first derivatives `Su,Sv`, second derivatives `Suu,Suv,Svv`, and a status. A successful numeric result does not establish normal regularity, validity inside a trim loop, absence of self-intersection or valid solid topology. Consumers request and inspect those properties separately.

Use homogeneous data `H=(A,W)`, where the three components of `A` are weighted Cartesian coordinates. Evaluate the six homogeneous jet components, then recover the Euclidean jet using the quotient identities:

```text
S   = A / W
Su  = (Au  - Wu*S) / W
Sv  = (Av  - Wv*S) / W
Suu = (Auu - Wuu*S - 2*Wu*Su) / W
Suv = (Auv - Wuv*S - Wu*Sv - Wv*Su) / W
Svv = (Avv - Wvv*S - 2*Wv*Sv) / W
```

These equations follow by differentiating `A=W*S`. They expose an implementation trap: finite differences of homogeneous control points yield homogeneous polynomial derivatives; they are not yet derivatives of the Cartesian rational surface. Rational Cartesian derivatives can remain nonzero above the polynomial degree because of the denominator.

For normalized coordinates `a,b` and original rectangle widths `du,dv`, derivatives in the original parameterization require `Su=Sa/du`, `Sv=Sb/dv`, `Suu=Saa/du²`, `Suv=Sab/(du*dv)`, `Svv=Sbb/dv²`. Declare which coordinates every API uses. A geometrically correct position with a wrongly scaled derivative corrupts downstream solvers quietly.

Compute homogeneous derivatives through forward-difference control nets or derivatives of Bernstein basis functions. Start with a simple, owned tensor de Casteljau evaluation and compare a directly summed Bernstein jet before selecting the production formulation. Compile-time degrees keep temporary storage bounded. The 2026 constant-storage method is the next performance candidate, not an adopted dependency.

Normalize all weights on a patch by a common power of two when useful; this leaves the rational map unchanged mathematically. It does not fix arbitrarily poor conditioning. Recentering geometry can reduce cancellation in small features far from the origin, but the translation and restoration errors must enter the numeric contract. Publish supported coordinate scales and weight ratios from qualification results instead of assigning an unexplained global epsilon.

## Storage and execution

Use owned native C++ values and binary arrays. JSON can describe external benchmark results; it should not represent geometry internally.

- Patch records contain offsets, degrees, a parameter rectangle and a generation identifier. Flatten homogeneous coordinates into contiguous storage. Keep a canonical import representation and compare structure-of-arrays versus packed four-double control points as a performance choice.
- Requests contain original request identifiers so grouping by patch or degree never changes caller-visible ordering.
- Results use separate component arrays when consumers read only part of the jet. Avoid computing or transferring all derivatives when a position-only task suffices.
- Group coherent requests by patch; test independent threads first. A block sharing one patch's controls may help for large per-patch batches, but adds synchronization and cannot be assumed faster.
- Keep geometry resident across repeated queries. Retain immutable snapshots until all launches that reference them complete. A generation mismatch must fail visibly rather than read newly replaced controls.

A bicubic homogeneous patch has 16 × 4 × 8 = **512 bytes** of control data. The six Cartesian vectors alone occupy **144 bytes per request**, before status and indexing. One million full-jet outputs therefore produce 144 MB of component data. Copying every jet back to the CPU can dominate a fast evaluator; inverse solvers or tessellation should eventually consume jets on device and return only their requested terminal output. Benchmarks must report both the resident computation and complete public API cost.

## Failure behavior

Separate batch execution failures from geometric request outcomes. Invalid buffer lengths, launch failure or lost device fail the batch. A batch must never appear successful merely because some stale outputs exist.

Per request, distinguish at least invalid patch, unsupported degree, invalid/nonfinite parameter, out-of-domain parameter, invalid weight, numerical range/conditioning failure and success. Failed outputs should contain an unmistakable invalid value plus the status; callers must not interpret their coordinates. Do not silently clamp parameters or return a fake zero normal. The precise thresholds belong in the reviewed spec and fixture corpus.

Normal construction uses `Su × Sv`. A small or zero cross product can represent a singular chart even when position and derivatives are evaluated successfully. Test normal regularity with a scale-aware criterion and expose it as a separate outcome. A sphere pole needs explicit chart handling downstream; rejecting the numeric surface position itself would conflate two different contracts.

For NURBS ingestion, repeated knots need documented one-sided derivative semantics. Extract each nonempty knot rectangle into a Bézier patch on the CPU first; retain its original parameter map and source identity. Verify extraction numerically with a separately structured spline evaluator. Knot insertion is algebraically shape-preserving, but floating-point extraction error still exists. Periodic seams, repeated-knot discontinuities and trim boundaries require explicit adjacency/side records; matching coordinates alone is insufficient.

## Subdivision and trustworthy bounds

Homogeneous de Casteljau subdivision preserves positive weights mathematically. Positive weights make the Euclidean patch lie in the convex hull of its Euclidean control points; the control-point AABB is therefore a useful bound. A box around sampled surface points alone is not conservative.

The computational qualification is crucial: rounded subdivision and division can move stored control points and underestimate a bound. Before pruning candidates, use outward-rounded arithmetic or a derived conservative error enclosure that includes input, extraction and subdivision error. Merely adding one ULP to the final box does not account for errors accumulated earlier. Keep approximate bounds visibly distinct from bounds permitted to reject geometry.

Start with CPU-owned conservative subdivision and device evaluation. Moving subdivision work queues to the GPU follows when the bounded primitive and termination policy are established. Every adaptive queue has a finite capacity; overflow or maximum depth must emit unresolved regions, not discard them. This failure path will matter directly to tangencies and thin features in later intersection work.

## Inverse evaluation as the next consumer

For point `x`, minimize `f(u,v)=0.5*|S(u,v)-x|²`. With residual `r=S-x`, the gradient is `(r·Su,r·Sv)`; Hessian entries are `Su·Su+r·Suu`, `Su·Sv+r·Suv`, and `Sv·Sv+r·Svv`. A bounded Newton/trust-region step is a useful local candidate generator. Small parameter steps alone do not establish convergence, a minimum, or the global nearest point.

For an untrimmed patch, also search its parameter edges and corners. For a trimmed face, inspect trim curves and chart seams as explicit candidates. Otherwise a point projected onto the excluded portion of the carrier can incorrectly be accepted as a face projection.

A global query needs branch-and-bound: conservative boxes provide distance lower bounds; actual evaluated points provide upper bounds. Terminate when the upper/lower distance gap meets the declared distance tolerance, or return unresolved. A distance-gap guarantee does not locate a unique closest parameter if several minima exist. Report local convergence, unresolved and global-distance-qualified results as different outcomes. Reuse batches of evaluator requests in this solver to learn whether residency produces end-to-end gains.

## Oracles that bear on the claims

The first qualification corpus should cover rational quarter circles and cylindrical patches, planar/bilinear/bicubic polynomial surfaces, rational curved patches, extreme supported weights, geometry translations, tiny parameter rectangles, endpoints and corners, and singular charts. Keep malformed data and unsupported inputs in the same reproducible run.

An exact rational quarter-circle fixture avoids treating rounded `sqrt(2)/2` as an exact conic: use homogeneous quadratic controls `(1,0,1)`, `(1,1,1)`, `(0,2,2)`. Their Cartesian controls are `(1,0)`, `(1,1)`, `(0,1)` with weights `1,1,2`. Expanding the homogeneous curve gives `A=(1-t²,2t)` and `W=1+t²`, so `Sx²+Sy²=1` exactly. Extruding this curve supplies a rational cylindrical surface with closed-form first and second derivatives. This asymmetric parameterization is also a useful defense against accidentally assuming equal endpoint weights.

Use three complementary mechanisms, each aimed at a different fault class:

1. **Exact model fixtures.** Evaluate Bernstein sums and quotient derivatives over exact rational input using integer/rational arithmetic in the test harness. Binary64 inputs and parameters are exact rational numbers, so this checks arithmetic error against the actual provided input. Quarter-circle points also satisfy a circle equation, providing geometric checks independently of another evaluator.
2. **Metamorphic relations.** Check common weight rescaling, affine coordinate transforms, degree elevation, patch subdivision with chain-rule derivative scaling, and mixed-partial agreement. Quantify transformation rounding instead of asserting unconditional bitwise equality.
3. **Independent kernel comparison.** Compare USD BRep/OCCT or an owned de Boor CPU reference where semantics match. Preserve disagreements as witnesses; CPU parity alone is not the oracle for geometry.

Finite-difference derivative checks can detect index/sign mistakes, but step-size cancellation makes them unsuitable as the sole derivative oracle. A CPU copy of the CUDA algorithm risks sharing precisely the bug under study. Exact rational fixtures need only a small meaningful corpus, not a general symbolic algebra product or adopted numerical library.

Report componentwise absolute error, scaled relative error where the expected value is nonzero, geometric residuals and observed failure classification. Positions, first derivatives and second derivatives have different units and conditioning; one global percentage is uninformative. Failed and unresolved cases remain in the denominator of workload qualification, with timings separately labeled.

## P100 toolchain and measurement

NVIDIA's architecture matrix gives Pascal compute capabilities 6.0/6.1, CUDA 12.x as the last toolkit family and R580 as the final driver branch. Build actual `sm_60` machine code with a compatible CUDA 12 toolkit and check installed compiler/driver versions independently; the version printed by `nvidia-smi` is a driver capability, not proof that `nvcc` is installed. [Matrix](https://docs.nvidia.com/datacenter/tesla/drivers/latest/cuda-toolkit-driver-and-architecture-matrix.html).

The Pascal tuning guide distinguishes GP100 from other Pascal chips and explains occupancy, registers and shared memory. The GP100 whitepaper describes substantial FP64 capacity and HBM2, but board variants have different peak figures. Record the board and power/clock behavior of actual hardware; do not substitute SXM peak numbers for PCIe measurements. [Pascal tuning guide](https://docs.nvidia.com/cuda/archive/12.9.1/pascal-tuning-guide/index.html), [GP100 whitepaper](https://images.nvidia.com/content/pdf/tesla/whitepaper/pascal-architecture-whitepaper-v1.2.pdf).

CUDA supports binary64 operations, FMA and directed rounding. CPU and GPU results can differ because of contraction, instruction ordering and host intermediate precision; bitwise equality is not the qualification target. Use explicit FP64 FMA where intended, disable unsafe reassociation/fast-math and document the operation semantics that numerical bounds assume. [NVIDIA floating-point guide](https://docs.nvidia.com/cuda/archive/12.6.0/floating-point/index.html).

Benchmark batch sizes from one query through millions, coherent and shuffled patch access, position-only/first-jet/full-jet outputs and cold/resident geometry. Record CPU scalar and threaded baselines with equivalent outputs. CUDA events measure device work; a monotonic host clock with synchronization measures complete API latency. Separate upload, launch, evaluation, download, validation, fallback and extraction. Include warmup, median and tail timings, throughput, allocation footprint and qualification counts. Measure break-even sizes; small interactive modeling operations may lose to transfers and launch cost even when large batches win.

## Actionable sequence and open questions

First turn this proposal into a reviewed numerical spec with explicit domains, error reporting, parameter semantics and corpus. Implement the owned CPU reference/oracles and the narrow CUDA evaluator as one conceptual unit. Publish correctness and measured latency before trying to infer a modeling speedup. Add NURBS extraction and a real resident consumer next; then compare constant-storage and alternative derivative evaluation strategies against the simple kernel.

Questions still requiring source inspection or experiments:

- Which real USD BRep workloads spend material time evaluating large coherent surface batches, and how many requests can be collected without redesigning their adaptive control flow?
- Which degree/weight/coordinate ranges arise in those workloads, including analytic primitives represented through native carriers?
- Can the public kernel interfaces expose control data and derivative semantics efficiently without depending on internal ownership or scratch allocators?
- How much error enters knot insertion and patch extraction before CUDA arithmetic starts?
- Does the P100 bottleneck become register occupancy, FP64 division, irregular loads or result transfer for full jets?
- What conservative bound is practical for the first inverse consumer, and how much does certification cost compared with unqualified Newton sampling?

No library is selected by this report. Papers and source studies inform owned implementations. The current public research establishes a plausible first acceleration boundary, not a completed GPU B-rep kernel.

See [bibliography](gpu-numerics.bib) and [PDF acquisition manifest](gpu-numerics-acquisitions.tsv) for restoration URLs and the locally acquired corpus.
