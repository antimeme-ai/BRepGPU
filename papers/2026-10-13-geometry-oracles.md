# Geometry oracles and a corpus for progressive GPU B-rep work

Research date: 2026-10-13. This is a design input, not an implementation or GPU performance result. Recommendations below are project proposals; cited literature supplies the numerical and geometric foundations.

## First deliverable

Build the first FP64 surface evaluator with analytic point and derivative oracles, then measure it across batch sizes with both transfer-inclusive and resident-data timings. Keep the CPU kernel as an experimental comparison. Agreement with it cannot establish correctness independently.

The [existing CPU experiment](../results/cpu-2026-10-13/REPORT.md) qualified 16 of 19 workloads under its stated checks. Three radius-10 sphere tessellations failed the requested chord-height bound. Points on actual triangles had surface distances approximately 0.220275, 0.0573932 and 0.0145261 for requests 0.2, 0.05 and 0.01, respectively. The [driver](../benchmarks/cpu/benchmark.py) preserved triangle vertices and barycentric witnesses. These are specific output counterexamples, not a general claim about all sphere meshes or a full tessellation audit.

That is the precedent for GPU work: emit a concrete bad point, missing branch, inconsistent edge or incorrect classification when an output fails. Successful status codes, plausible volume and attractive renders are insufficient.

## The evaluator's numerical contract

The first proposed unit accepts rational Bézier patches with finite FP64 control points, positive finite weights, supported degrees, sufficient control counts and a declared parameter rectangle. The later NURBS front end additionally requires nondecreasing finite knots, knot-endpoint conventions and an explicit derivative side at discontinuities. Reject invalid input with an explicit status. Singular normals, unsupported degrees, non-finite results and rational denominators outside the accepted range must not silently become valid geometry.

FP64 improves numerical accuracy; it does not guarantee correct signs near degeneracy. Shewchuk's adaptive predicates use floating-point filters and higher-precision expansions to recover exact signs under specified arithmetic assumptions. Those predicates concern discrete decisions, not a blanket solution for NURBS evaluation or surface intersection. [Shewchuk 1997](https://people.eecs.berkeley.edu/~jrs/papers/robustr.pdf).

CPU/GPU bitwise equality is also an unsuitable general acceptance condition. Fused multiply-add, reassociation and reduction order can change rounding while both implementations follow floating-point rules. Record contraction, fast-math and rounding settings; evaluate a geometric/numerical error budget instead. [NVIDIA floating-point documentation](https://docs.nvidia.com/cuda/archive/12.2.2/floating-point/index.html).

Use coordinate-aware absolute and relative budgets derived from the declared supported input range. Report the worst residual and its input, not only a mean. Do not enlarge budgets after discovering a failure to make an advertised result pass. A normal-direction comparison needs an explicit singularity policy because normalizing a nearly zero cross product amplifies error.

## Actionable prototype fixtures

Two small analytic families expose different fault classes without asking another CAD kernel to supply truth.

**Polynomial bicubic patch.** The current CPU fixture represents

\[
S(u,v)=(12u,9v,27u(1-u)v(1-v)),\quad 0\leq u,v\leq1.
\]

Its first derivatives are

\[
S_u=(12,0,27(1-2u)v(1-v)),\qquad
S_v=(0,9,27u(1-u)(1-2v)).
\]

The second derivatives are `S_uu=(0,0,-54v(1-v))`, `S_vv=(0,0,-54u(1-u))` and `S_uv=(0,0,27(1-2u)(1-2v))`.

Use its known Bernstein control net directly in the owned evaluator. This exposes coordinate strides, control-net ordering, endpoint handling, derivative indexing and the actual batch interface. Preserve the analytic formulas as the oracle; do not have the evaluator generate its own expected values.

**Rational cylinder patch.** A degree-2 rational curve with Euclidean control points `(1,0)`, `(1,1)`, `(0,1)` and weights `(1,1,2)`, using a clamped single-span knot vector, has the parameterization

\[
C(u)=\left(\frac{1-u^2}{1+u^2},\frac{2u}{1+u^2}\right).
\]

Extrude the control net linearly in `v` to obtain `S(u,v)=(C_x(u),C_y(u),v)`. Then

\[
S_u=\left(\frac{-4u}{(1+u^2)^2},\frac{2(1-u^2)}{(1+u^2)^2},0\right),\qquad S_v=(0,0,1).
\]

The second derivatives are

\[
S_{uu}=\left(\frac{-4+12u^2}{(1+u^2)^3},\frac{4u(u^2-3)}{(1+u^2)^3},0\right),\qquad S_{uv}=S_{vv}=(0,0,0).
\]

The proposed evaluator returns `S`, `S_u`, `S_v`, `S_uu`, `S_uv` and `S_vv`, with explicit validity for each requested order. At repeated knots, second derivatives follow the stated one-sided convention when they exist; they cannot be assigned a fabricated common value across a discontinuity.

This tests rational normalization and the derivative quotient rule with exactly representable control data. For dyadic FP64 parameters, evaluate these formulas with exact integer-ratio arithmetic in the host oracle and compare rounded results using the stated budget. The fixture avoids replacing a circular control weight by an approximation to an irrational number. The construction and formulas here are direct project derivations.

Add the following probes before calling either family a qualified GPU prototype:

| Probe | Fault it attacks | Acceptance |
| --- | --- | --- |
| Endpoints, midpoints and `nextafter` neighbors of interior knots | Wrong span ownership and one-sided derivatives | Point/derivative matches the declared side and analytic or exact-rational model |
| Nonuniform knots, repeated knots, asymmetric nets and mixed degrees | Uniform-knot assumptions and transposed layout | Independent rational basis calculation from the input agrees within budget; discontinuous derivatives are treated explicitly |
| Common scaling of all homogeneous coordinates/weights within safe range | Incorrect division or weight handling | Same Euclidean points and derivatives |
| Known knot insertion and degree elevation, with transformed control data | Dependence on representation rather than geometry | Same points and applicable derivatives before/after transformation |
| Rigid motion and uniform scaling of control points | Hidden origin/unit assumptions | Points, tangent vectors and lengths transform accordingly; normals use the correct transform |
| Empty, singleton, odd-sized, mixed-surface and shuffled batches | Tail errors, stale buffers and cross-surface indexing | No dropped/duplicated/reordered result; statuses stay attached to input indices |
| Invalid counts, knots, degree, non-finite data and disallowed denominator | Silent bad geometry and invalid memory access | Explicit failure; output does not claim success |

Knot insertion is a shape-preserving operation and therefore provides a useful metamorphic relation. The expected transformed controls should be constructed by the fixture model, not obtained through the GPU routine being checked. [Michigan Technological University surface knot-insertion notes](https://pages.mtu.edu/~shene/COURSES/cs3621/LAB/surface/knot-insrt.html), [Huang et al. 2005](https://archive.ymsc.tsinghua.edu.cn/pacm_download/12/444-cagd_2005_degree.pdf).

Finite differences can diagnose derivative faults, but their step-size and cancellation errors make them a weaker acceptance oracle than the formulas above. Randomized fixtures supplement these cases; they do not replace them.

## Advancing from numerical work to a B-rep kernel

Each new operation needs its own output contract. A residual verifies where a returned intersection point lies; it does not show that every component of the intersection was found.

For surface/surface intersection, the MIT hyperbook treats finding starting points for all branches separately from tracing. This distinction must survive a parallel implementation: a fast tracer with one missed loop is incorrect. [Patrikalakis, Maekawa and Cho, intersection chapter](https://web.mit.edu/hyperbook/Patrikalakis-Maekawa-Cho/).

Start with analytic fixtures that have zero, one, two or several known components: disjoint planes, transverse planes, plane/sphere circles, tangent plane/sphere point contact, plane/torus two loops, cylinder/cylinder crossings and coincident patches. Treat point contacts, overlaps and curves as different result categories. Add close branches, seam crossings and intersections wholly inside a patch so a boundary-only seeding strategy cannot pass.

Later, use conservative subdivision or interval enclosures to rule out unvisited regions. An overlapping interval box is a candidate, not automatically a proven root. Every remaining region must be resolved or returned as unresolved; exhausting a work queue or iteration budget is not equivalent to finding no intersection. Gleicher and Kass show progressive interval refinement, but explicitly lack strong guarantees for linking the samples and report ambiguous cases to the user. Hu and colleagues address interval polynomial intersections including ill-conditioned cases. These are approaches to study, not guarantees inherited by our future implementation. [Gleicher and Kass 1992](https://graphicsinterface.org/wp-content/uploads/gi1992-28.pdf), [Hu et al. 1997](https://doi.org/10.1016/S0010-4485(96)00099-1).

Topology changes require robust decisions and coherent constructions. Lévy's mesh CSG work separates exact predicates, exact constructed intersection coordinates and combinatorial classification. Its discussion of conversion back to floating-point coordinates shows why correct internal decisions alone do not establish a correct exported object. Applying those lessons to trimmed NURBS requires additional research; mesh exactness is not a NURBS Boolean oracle. [Lévy 2024/2025](https://doi.org/10.1145/3744642).

For each later stage, use the direct checks below, with a declared distinction between approximate sampling and validated bounds.

| Stage | Required output checks | Useful independent oracle |
| --- | --- | --- |
| Intersection curves | Residual against both surfaces, endpoints, complete component count, connectivity, tangency classification | Analytic intersections; interval exclusion of remaining domain where available |
| Trimming | UV-loop closure, periodic seam convention, loop nesting/orientation, absence of trim self-crossings, agreement between spatial edge and both incident pcurves | Analytic planar/cylindrical trimmed fixtures and bounded curve/surface discrepancy |
| Tessellation | Finite nondegenerate triangles, matching shared-edge discretization, oriented incidence, vertex links, global self-intersection, approximation error | Analytic spheres/cylinders/planes; conservative surface bounds for general patches |
| Solid topology | Referential consistency, legal coedge pairs, shell connectivity/orientation and vertex neighborhoods | Known topology of constructed fixtures; explicit incidence/link traversal |
| Boolean result | Correct material membership, boundary orientation, expected components/cavities and geometric approximation | Analytic primitive membership model, known volume and independent boundary-distance checks |

For regularized solid Booleans, fixtures must specify whether lower-dimensional contacts are discarded. Check membership away from the declared boundary uncertainty band. Check contact geometry separately. Boolean set identities and operand permutations provide metamorphic checks, but compare material geometry, not face identifiers or triangulation counts. Approximate tolerance semantics can invalidate identities applied without their preconditions.

Euler characteristic and correct edge incidence are useful for a known spherical shell; they do not prove embedding, exclude self-intersection or generalize to every B-rep using unadjusted `V-E+F`. Face holes, periodic seams, cavities and nonmanifold contracts need explicit treatment. Sampling distances on shared edges is only a sampled check until supported by a conservative bound.

USD BRep's own API distinguishes integration accuracy, manifold status, tolerance and stitching diagnostics. Its returned shell flag can echo an input flag rather than certify the result. Preserve these diagnostics, while relying on independent checks for acceptance. [USD BRep Python API](https://nvidia-omniverse.github.io/usd-brep/latest/docs/python.html).

## Corpus: obtain breadth without confusing it with truth

Start with owned parametric generators and minimized failures. These give an unambiguous construction history, analytic oracle and clear publication provenance. Vary scale, translation, aspect ratio, degree, knot spacing/multiplicity, weight ratios, thin features, tangency offsets, periodic seams and nearly coincident boundaries within declared input limits. Save the recipe and exact offending inputs so a case can be reproduced without a proprietary editor.

| Source | Useful role | Limits and publication handling |
| --- | --- | --- |
| Owned analytic fixture families | Qualification of evaluator, intersections, trims, Booleans and tessellation | Small but strong; every result has a specified oracle |
| ABC dataset | Real parametric CAD surface types, control nets, STEP import/export, feature distributions | Parametric source is valuable, but an imported file is not a proven Boolean truth oracle; record import failures separately |
| Thingi10K | Adverse mesh geometry, self-intersections, degeneracies, multiple components and export/repair stress | Primarily mesh input; use input validity categories rather than assuming every object is a solid |
| ThingiCSG | Constructive trees, coplanar facets, repeated operands and large composition workloads | Mesh CSG corpus; resolution and evaluation semantics must be recorded before adapting it to curved B-rep operations |
| Literature-derived NURBS hard cases | Closed interior intersection loops, tangencies, singularities and close branches | Reconstruct from stated geometry; label unavailable control nets and non-reproduced figures |

ABC supplies explicitly parameterized curves and surfaces and several corresponding formats. Its paper says the dataset is distributed under MIT, but the current dataset website says model copyright belongs to creators and directs readers to Onshape terms. The repository's MIT software license alone does not resolve that mismatch for every asset. Keep source identifiers, creator metadata and retrieval manifests; resolve asset provenance before mirroring model files into a public repository. [ABC paper](https://openaccess.thecvf.com/content_CVPR_2019/html/Koch_ABC_A_Big_CAD_Model_Dataset_for_Geometric_Deep_Learning_CVPR_2019_paper.html), [current dataset site](https://deep-geometry.github.io/abc-dataset/), [repository license](https://github.com/deep-geometry/abc-dataset/blob/master/LICENSE).

Thingi10K's maintainer explicitly separates its Apache-2.0 filtering code from each model's license field. Retain attribution and that field for each selected model. ThingiCSG's repository has a BSD-3-Clause license and incorporates files from multiple sources and a BOSL2 submodule; inspect embedded notices and submodule terms before redistributing a selected example. No dataset binaries or external code were adopted by this report. [Thingi10K repository](https://github.com/Thingi10K/Thingi10K), [ThingiCSG repository](https://github.com/BrunoLevy/thingiCSG).

Stratify a later corpus by geometry/topology features and known adverse conditions. Publish failures and unsupported inputs alongside successes. A convenient import-success subset alone would conceal the unsupported cases that matter to a full kernel.

## Fair CPU/GPU measurements

The CPU surface baseline includes individual Python binding calls and list construction. It is a useful application baseline but cannot establish native CPU versus CUDA arithmetic efficiency. Add an owned native batched CPU implementation of the same supported operation and include a specified multicore configuration when claiming server-scale throughput.

Report three distinct quantities: **first-use latency** including initialization/allocation, **transfer-inclusive operation latency** from ready host input to ready host result including packing/transfers/fallback, and **resident operation latency** with pre-existing device buffers. Show GPU event time for the compute region as a component. State whether preprocessing can be amortized and how many operations amortize it.

CUDA launch calls are asynchronous. CPU wall timing needs completion synchronization; events measure the declared device stream region. Do not sum independently timed overlapping stages as if they equal elapsed time. NVIDIA's guide also emphasizes transfer costs and batching. [CUDA 12.5.1 Best Practices Guide, performance metrics and memory sections](https://docs.nvidia.com/cuda/archive/12.5.1/cuda-c-best-practices-guide/index.html).

Sweep query counts and surface counts separately, from single queries through memory-limited batches. Include coherent queries on one surface, shuffled queries across heterogeneous surfaces, and changing geometry that invalidates residency. Record degree, knot/control counts, bytes transferred, precision settings, kernel/runtime/compiler versions, CPU threads, device model, memory pressure and shared-host conditions.

Exclude validation from operation time only when it is a benchmark oracle. Include checks required by the product to return a trustworthy result. Warm both paths, disclose excluded warmups and record all measured samples. Qualification happens before successful-speed summaries; wrong outputs retain diagnostic timings and witnesses.

For adaptive work, record rejected candidates, queue exhaustion, fallback counts/fraction and fallback elapsed time. Include fallback transfers and CPU work in end-to-end latency. A high resident speed with frequent CPU fallback is a useful result only with those costs visible.

## Reading and acquisition record

The companion [BibTeX](geometry-oracles.bib) lists primary sources. [PDF catalog](geometry-oracles-pdf-catalog.tsv) records acquired files, source URLs, byte counts and SHA-256 digests; PDFs remain ignored. The report inspected the existing owned CPU driver's analytic and mesh oracles plus relevant downloaded paper sections. It did not execute a new kernel or reproduce the cited authors' benchmarks.

The Graphics Interface 1992 PDF initially returned HTTP 403 to the downloader; retrying with a browser user-agent acquired the publisher's free PDF. Six PDFs were acquired in this research lane. Hu et al. was inspected through its publisher abstract; its full paper remains to be acquired before adopting its algorithmic details. Huang et al.'s free PDF was readable through the web tool, but the direct acquisition timed out; its URL remains in the bibliography for restoration.

The remaining design gap is general validated trimmed-NURBS output correctness. The prototype fixtures are deliberately narrower. A finite corpus or CPU agreement cannot close that gap by itself.
