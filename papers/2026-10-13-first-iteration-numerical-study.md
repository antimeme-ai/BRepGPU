# First iteration: numerical evaluator source study

2026-10-13. This report owns the first numerical evaluator unit: a rational Bézier patch of degree at most three in each direction, emitting position and all partial derivatives through total order two. The choices below are proposed for design review; no evaluator, GPU benchmark or production dependency has been implemented or selected.

The source study supports a small, owned **homogeneous surface jet plus Cartesian quotient recovery**. Use a directly summed Bernstein CPU evaluator and a bounded-storage tensor de Casteljau CUDA evaluator initially. Analytic and exact-rational fixtures qualify their outputs independently; CPU agreement is useful differential evidence, never the entire oracle. Separate valid numeric evaluation from chart regularity and from certified geometric decisions.

## Acquired sources and exact entry points

ZIP archives are retained intact; clean extractions are in ignored `quarantine/new-iteration/`. [Restoration TSV](first-iteration-numerical-sources.tsv) records full revisions, source URLs and archive hashes. None of these sources is vendored or linked into BRepGPU.

| Source | Pinned revision | Entry points actually read |
| --- | --- | --- |
| [SINTEF SISL](https://github.com/SINTEF-Geometry/SISL/tree/e0a82b1a851e932657f3e1af25033fbbcd33e907) | `e0a82b1a851e932657f3e1af25033fbbcd33e907` | `src/s1421.c`: triangular surface jet, basis contraction, normal warning; `src/s6strider.c`: rational derivative recovery; `include/sislP.h`: equality tolerances |
| [openNURBS](https://github.com/mcneel/opennurbs/tree/eb92af3ba1806b0a34a99aba0d3bda83e3d46083) | `eb92af3ba1806b0a34a99aba0d3bda83e3d46083`, branch `8.x` | `opennurbs_evaluate_nurbs.cpp`: basis, derivative and span evaluators; `opennurbs_math.cpp`: quotient recovery; `opennurbs_nurbssurface.cpp`: side and span dispatch; `opennurbs_bezier.cpp`: Bézier surface entry point |
| [LTCSBezier](https://github.com/valasekg/LTCSBezier/tree/44fdd9838a1e87fe21de3118b1a79e7949fb7542) | `44fdd9838a1e87fe21de3118b1a79e7949fb7542` | `src/bezier_eval.hpp`: interpolation and constant-storage variants; `src/ltcs_fma.hpp`: FMA bridge; `test_precision/main_precision.cpp`: exact-rational experiment; `test_performance/main_benchmark.cpp`: benchmark indexing; build files |

SISL source headers identify AGPL-3.0-or-later plus a producer-line condition and commercial licensing. openNURBS retains its own notices; its root `LICENSE` also contains zlib material, which should not be mistaken for the complete toolkit terms. The acquired LTCSBezier tree has no standalone license file. These are observations about the snapshots, not dependency selections or permissions to copy implementation code. Reference build systems were not run; LTCSBezier's build would fetch dependencies automatically.

### SISL: reuse the mathematical structure, examine the failure path

[`s1421`](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s1421.c#L53) emits derivatives in triangular total-order sequence. It contracts one parameter direction into workspace, then contracts the other direction, calculating only the requested triangle. Rational surfaces use homogeneous coefficients and call [`s6strider`](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s6strider.c#L55). Its normal status distinguishes zero and near-degenerate tangents; this is useful evidence that normal regularity differs from obtaining derivative coordinates.

The denominator path needs different behavior here: [`s6strider.c:184`](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s6strider.c#L184) replaces a denominator considered zero with `1.0`. [`DEQUAL`](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/include/sislP.h#L430) uses `REL_PAR_RES=1e-12` and an absolute scale floor of one. This can produce ordinary numeric output without reporting an invalid rational denominator. BRepGPU should reject unsupported weights before dispatch and report an invalid runtime denominator explicitly, without substituting a value. This is a static source finding, not a reproduced executable failure.

### openNURBS: compact output order and explicit side selection

[`ON_NurbsSurface::Evaluate`](https://github.com/mcneel/opennurbs/blob/eb92af3ba1806b0a34a99aba0d3bda83e3d46083/opennurbs_nurbssurface.cpp#L990) selects spans with quadrant/side information and optional hints, then calls [`ON_EvaluateNurbsSurfaceSpan`](https://github.com/mcneel/opennurbs/blob/eb92af3ba1806b0a34a99aba0d3bda83e3d46083/opennurbs_evaluate_nurbs.cpp#L1052). The latter specializes first and second homogeneous derivatives and uses the same six-entry triangle. [`ON_EvaluateQuotientRule2`](https://github.com/mcneel/opennurbs/blob/eb92af3ba1806b0a34a99aba0d3bda83e3d46083/opennurbs_math.cpp#L3959) recovers rational derivatives with special cases through second order.

The quotient helper returns false for an exactly zero denominator. However, [`ON_EvaluateNurbsSurfaceSpan` at its rational call](https://github.com/mcneel/opennurbs/blob/eb92af3ba1806b0a34a99aba0d3bda83e3d46083/opennurbs_evaluate_nurbs.cpp#L1231) discards this return, copies output and returns true. That prevents its outer success Boolean from serving as an independent denominator oracle. [`ON_BezierSurface::Evaluate`](https://github.com/mcneel/opennurbs/blob/eb92af3ba1806b0a34a99aba0d3bda83e3d46083/opennurbs_bezier.cpp#L2471) constructs clamped knot vectors and uses the general span evaluator; it is not a dedicated fast tensor Bézier path. These are static findings at the pinned revision; no library was built or executed.

### LTCSBezier: arithmetic ideas are stronger than inherited experiment labels

The implementation contains de Casteljau, constant-storage ladder, geometric and Seiler variants. Its [`two-FMA de Casteljau`](https://github.com/valasekg/LTCSBezier/blob/44fdd9838a1e87fe21de3118b1a79e7949fb7542/src/bezier_eval.hpp#L112) expresses interpolation without subtracting the two endpoints first. The [2026 paper](https://doi.org/10.1111/cgf.70403) supplies relevant error analysis and register motivation; its published polynomial results do not qualify a rational surface jet. The acquired derivative experiment near line 1201 fixes its derivative count at one, and the tree is not a complete rational six-jet implementation.

Several inspected source details require an owned harness:

- [`bezierDeCasteljauSUBFMA`](https://github.com/valasekg/LTCSBezier/blob/44fdd9838a1e87fe21de3118b1a79e7949fb7542/src/bezier_eval.hpp#L88) currently uses the same expression as the function labeled direct FMA. Names alone do not establish which arithmetic was measured.
- [`bezierLTCSNckLUT`](https://github.com/valasekg/LTCSBezier/blob/44fdd9838a1e87fe21de3118b1a79e7949fb7542/src/bezier_eval.hpp#L545) stores a mutable static binomial table and previous degree. It is unsuitable for unsynchronized concurrent invocations with different degrees. Our degrees can instead select immutable compile-time constants.
- [`main_benchmark.cpp`](https://github.com/valasekg/LTCSBezier/blob/44fdd9838a1e87fe21de3118b1a79e7949fb7542/test_performance/main_benchmark.cpp#L47) sizes its parameter and output arrays as `S`, currently one, then loops through `i<=S`. Static inspection finds an out-of-bounds read/write. Do not treat this snapshot's timings as a qualified baseline.
- [`main_precision.cpp`](https://github.com/valasekg/LTCSBezier/blob/44fdd9838a1e87fe21de3118b1a79e7949fb7542/test_precision/main_precision.cpp#L713) evaluates an exact rational ground truth, but on the original rational author inputs; floating algorithms receive rounded inputs. Its observed error combines representation error with algorithm error. The checked default is scalar polynomial points in `float`, degrees 2–5, not weighted surfaces and their derivatives. Our arithmetic qualification must also evaluate exact rational truth for the binary64 inputs actually provided.

These observations are based on reading, not execution or an exhaustive audit. They do not negate the paper's algorithms; they identify why source harnesses need scrutiny before reuse.

## GPU-NURBS: paper and public-browser study only

The public README combines a GPL description with academic-primary-user restrictions. Per project direction, no GPU-NURBS archive, checkout or local source copy was acquired. Browser pages were inspected on branch `main` on this date; unlike the three archived references, those pages are not a pinned source snapshot.

[Bao, Liu and Zou's 2023 primary paper](https://www.jcad.cn/cn/article/pdf/preview/10.3724/SP.J.1089.2023.2023-00003.pdf) was acquired and read: DOI `10.3724/SP.J.1089.2023.2023-00003`, volume 35(12), pages 1948–1957. It precomputes per-span polynomial matrices, obtains candidate parameters through adaptive subdivision and refines them with Gauss-Newton. The object of inversion is a point intended to lie on a carrier surface; this differs from finding the globally closest point on an arbitrary trimmed face. The paper's reported inverse throughput therefore does not qualify either our derivative error or our future closest-point contract.

Selected [public `Evaluate.cu` code](https://github.com/Qiang-Zou/GPU-NURBS/blob/main/src/NURBS/Evaluate.cu) makes `MatrixSurfacePoint` form parameter powers and contract four precomputed matrices using temporary arrays and device matrix multiplication. This demonstrates a concrete representation change that moves recursion out of per-query evaluation. It also shows why an owned fixed-degree path can simplify temporary storage without general matrix machinery. The [current build file](https://github.com/Qiang-Zou/GPU-NURBS/blob/main/src/CMakeLists.txt) targets architecture 86/`compute_86` and sets `-O0`: a CUDA 12.2 README claim does not make this source build P100-compatible or an optimized reference. Neither its source nor its experiments were executed.

The earlier [numerics reconnaissance](2026-10-13-gpu-numerics.md) covers recent derivative, subdivision and texture papers. Their main implications remain: retain Bernstein/homogeneous representation initially; treat faster recurrences as measured alternatives; avoid texture-filtered interpolation for FP64 results; and do not infer conservative geometry bounds from fast samples.

## Proposed first input and output contract

This is an explicit, conservative local numeric scope for the first design, not a claim to support all imported CAD surfaces:

- Degrees `p,q` are integers from 0 through 3. Degree zero is valid; absent homogeneous derivatives are exactly zero. A rational surface can still have nonzero Cartesian second derivatives when a direction has degree one.
- The patch supplies exactly `(p+1)(q+1)` binary64 homogeneous controls `H_ij=(A_x,A_y,A_z,W)`, indexed with `i` varying along `u` and `j` along `v`. The integrated design selects `i*(q+1)+j`, so `j` varies fastest.
- Every control is finite. For this first local envelope, `1<=W_ij<=256` and each `|A_ij,k|<=W_ij`. Thus Cartesian controls lie in the local cube `[-1,1]^3`. No automatic rescaling, recentering or clamping occurs in the evaluator.
- Query parameters are finite binary64 values in the **closed** square `[0,1]^2`, including edges and corners. Derivatives are with respect to these parameters and are ordinary derivatives, not Taylor coefficients divided by factorials.
- Emit `(S,Su,Sv,Suu,Suv,Svv)` in that order, 18 binary64 components plus status. Output identity/order matches input requests. Empty batches succeed with no outputs.

The power-of-two range cap is a proposed manageable first envelope, not a universal conditioning threshold. An importer may later apply explicit transformations and preserve their errors, but that is outside this primitive. Supporting arbitrary knot intervals would require parameter-map chain rules and one-sided semantics separately. Trim classification, normals, curvatures and topological validity are separate consumers.

Reject malformed buffers and invalid degrees/weights/control ranges structurally before dispatch. Per-query parameter errors emit their own status and invalid output coordinates. Use an explicit numerical-failure outcome if the evaluated denominator is nonpositive/nonfinite or any result is nonfinite; never replace the denominator, silently use the CPU, or reinterpret a launch failure as per-query success. A failed device batch must make its results unusable. Singularity of `Su×Sv` is not a failure of the six-jet itself.

### Why this envelope gives an honest range statement

These are **exact-arithmetic bounds** derived here, not measured forward-error guarantees. Nonnegative Bernstein basis functions sum to one, so `1<=W(u,v)<=256` and each Cartesian position component has magnitude at most one.

For degree at most three, homogeneous numerator derivatives satisfy `|Au|,|Av|<=1536`, `|Auu|,|Avv|<=6144`, `|Auv|<=9216` componentwise. Positivity of control weights gives `|Wu|,|Wv|<=768`, `|Wuu|,|Wvv|<=3072`, `|Wuv|<=4608`. Forward-difference formulas produce these conservative bounds directly. Applying the quotient identities below gives:

| Cartesian component | Conservative magnitude bound |
| --- | ---: |
| `S` | 1 |
| `Su,Sv` | 2,304 |
| `Suu,Svv` | 3,548,160 |
| `Suv` | 3,552,768 |

The intended operation graph therefore has enormous binary64 overflow headroom. This does **not** promise small relative error: coordinates and derivatives can be arbitrarily near zero after cancellation, and a Jacobian can be singular inside a positive-weight patch. Underflow is part of binary64 arithmetic and must not be hidden by a relative-error claim. Denominator stability is only one part of conditioning.

The primitive's first success status means that validated inputs produced finite arithmetic results under the stated operations. It does not mean a model-space tolerance or a conservative enclosure has been certified. Qualification must publish componentwise absolute/scaled errors for the corpus. Before using results to prune intersections or establish solid topology, derive outward error enclosures for the actual operation graph or use a separate bounded primitive. No unexplained `1e-6` or `1e-12` global geometry promise is justified by this study.

### Local core, named world/source-parameter adapter

Keep the first native/GPU API strictly local. Patch provenance records the source UV rectangle and an explicit world affine map `world(x)=origin+M*x`. A separately named native adapter such as `restore_world_source_jet` restores the caller's original coordinate and parameter conventions; it never infers conventions from an apparent model scale.

For source widths `du=u1-u0>0`, `dv=v1-v0>0`, and local normalized parameters `s=(u-u0)/du`, `t=(v-v0)/dv`, the adapter emits:

```text
world position = origin + M*S
world d/du    = M*Ss/du       world d/dv    = M*St/dv
world d²/du²  = M*Sss/du²     world d²/dudv = M*Sst/(du*dv)
world d²/dv²  = M*Stt/dv²
```

The integrated first design deliberately narrows this general affine proposal
to a uniform positive scale, bounded UV widths and bounded origins; see
[the reviewed adapter envelope](../docs/ITERATION_1_DESIGN.md#patch-and-query-contract).
General matrices and wider exponents remain future work.

The derivative transform uses only `M`; the origin contributes solely to position. Degree, source identity, source rectangle and the exact provided transform remain explicit native values. Invalid/nonfinite frames, nonpositive/nonfinite widths, or nonfinite mapped outputs fail the adapter separately from the local evaluator. Its elapsed time and conversion overhead are reported in benchmarks.

For a row of `M`, local component error budgets map through `sum_j |M_ij|*E`, multiplied by the appropriate inverse UV-width factors for derivatives. Add adapter rounding separately. If truth is defined at original source UV coordinates, also account for rounding when forming `s,t`; otherwise only the already-rounded normalized query was evaluated. Common recentering and weighting transformations need the same separation between author intent and provided binary input. A large world origin cannot be used to hide a small local positional error behind relative error on the translated coordinate.

### Initial qualification budgets

For the declared local envelope, propose the following **corpus qualification targets**, to be accepted or tightened during reviewed implementation. They are practical maximum absolute component errors against exact rational truth for the provided binary64 inputs, not a theorem for every possible patch:

| Grade | Components | Proposed maximum absolute error |
| --- | --- | ---: |
| `E0` | `S` | `2^-40` ≈ `9.095e-13` |
| `E1` | `Ss,St` | `2^-30` ≈ `9.313e-10` |
| `E2` | `Sss,Sst,Stt` | `2^-20` ≈ `9.537e-7` |

The higher-order budgets are looser because quotient recovery multiplies first-derivative errors by weight derivatives and can amplify denominator errors into second derivatives. Their severity is tied to the declared exact magnitude limits of roughly one, `2.3e3` and `3.6e6`, rather than interpreting one percentage across all orders. These deliberately allow margin for the bounded simple algorithm; they are starting targets, not measured errors or proven forward-error limits.

Every successful corpus case must meet the applicable grade on CPU and CUDA independently. A violation fails qualification, retains the input/output witness and prompts a design or scope correction; it must not disappear into an average or silently widen the threshold. Exact dyadic fixtures can additionally require exact expected components where the prescribed arithmetic permits them. Publish observed maxima and witnesses by order, including tiny expected values, endpoints, weight extremes and singular charts.

This scheme qualifies an initial primitive for further experiments. It does not permit a topology decision to assume that a runtime result is enclosed by `E0/E1/E2`: that requires analytic error enclosures. In particular, a tangent smaller than `E1` cannot support a claimed reliable normal from this budget alone, and curvature involves additional conditioning and division. The world/source-parameter adapter must publish its own transformed qualification outcomes rather than call local error a universal model-space tolerance.

## Six-jet mathematics and independent algorithms

Let `H=(A,W)` denote the homogeneous surface and its partial derivatives. Differentiating `A=W*S` yields:

```text
S   = A/W
Su  = (Au-Wu*S)/W
Sv  = (Av-Wv*S)/W
Suu = (Auu-Wuu*S-2*Wu*Su)/W
Suv = (Auv-Wuv*S-Wu*Sv-Wv*Su)/W
Svv = (Avv-Wvv*S-2*Wv*Sv)/W
```

Use direct division initially; replacing divisions with a shared reciprocal changes rounding and belongs in a separately qualified optimization. Explicit FP64 FMA can form subtraction/products intentionally, with compiler reassociation disabled. Validate each final component. Mixed terms require both first-derivative contributions, while pure second terms require the factor two.

**CPU model path:** evaluate Bernstein basis values and their first two derivatives explicitly for degrees 0–3, then accumulate each homogeneous jet component through a direct double sum over the control grid. For example, `B'_i=p(B_i-1^(p-1)-B_i^(p-1))` and the second derivative uses the second difference of the degree-`p-2` basis. This is structurally distinct from the CUDA tensor recursion. It is a useful model and baseline, with the exact-rational fixtures still checking its own arithmetic and quotient recovery.

**CUDA path:** start with one request per thread and a scalar homogeneous channel at a time. For each `v` row, evaluate along `u` the control values, degree-scaled first-difference controls and degree-scaled second-difference controls. This gives row arrays `r0,ru,ruu`, each with at most four scalars. Evaluate in `v`:

```text
H   = Bezier(r0,v)       Hu  = Bezier(ru,v)
Huu = Bezier(ruu,v)      Hv  = Bezier(q*diff(r0),v)
Huv = Bezier(q*diff(ru),v)
Hvv = Bezier(q*(q-1)*diff2(r0),v)
```

Here each derivative curve uses its actual reduced degree. Degree-zero and degree-one branches produce absent derivatives explicitly, without accessing nonexistent differences. Univariate evaluation uses owned nested-FMA interpolation `fma(t,b,fma(-t,a,a))`, not rounded `b-a` followed by a multiply-add. Degree dispatch selects bounded compile-time storage. Evaluate the six denominator values once, then recover Cartesian components from each numerator channel and write outputs incrementally.

Neither host nor device should allocate scratch dynamically per request, use mutable global degree caches or convert to a power basis by default. Alternative Bernstein direct sums, LTCS or Seiler recurrences may win later, but their arithmetic and true compiled behavior need comparison under this same output contract. For tiny degrees, asymptotic complexity alone does not decide the winner.

## Direct analytic and exact-rational fixtures

### Bilinear rational surface with an exactly representable full jet

Use Cartesian controls `P_ij=(i,j,ij)` and weights `1+i+j`, for `i,j` in `{0,1}`. In the integrated design's `i*(q+1)+j` order the homogeneous controls are `H00=(0,0,0,1)`, `H01=(0,2,0,2)`, `H10=(2,0,0,2)`, `H11=(3,3,3,3)`. Direct expansion gives:

```text
D = 1+u+v
S = (u*(2+v)/D, v*(2+u)/D, 3*u*v/D)
```

At `u=v=1/2`, direct differentiation gives all six vectors below. Every expected component is exactly representable in binary64, including the second derivatives:

| Output | Exact expected vector |
| --- | --- |
| `S` | `(5/8,5/8,3/8)` |
| `Su` | `(15/16,-1/16,9/16)` |
| `Sv` | `(-1/16,15/16,9/16)` |
| `Suu` | `(-15/16,1/16,-9/16)` |
| `Suv` | `(1/16,1/16,15/16)` |
| `Svv` | `(1/16,-15/16,-9/16)` |

This catches weight recovery, mixed-term omission, factor-two errors and the wrong assumption that a bilinear rational patch has zero Cartesian pure second derivatives. The table was checked with exact rational arithmetic during this study; it is a mathematical fixture, not a measured implementation result.

### Exact circle and cylindrical surface

Homogeneous quadratic controls `(1,0,1)`, `(1,1,1)`, `(0,2,2)` expand to `C(u)=((1-u²)/(1+u²),2u/(1+u²))`. Their weights are rational and unequal; no rounded square root is treated as exact conic geometry. Extruding with height `v` gives a degree `(2,1)` cylindrical patch.

With `D=1+u²`, analytic curve derivatives are `C'=(-4u/D²,2(1-u²)/D²)` and `C''=((-4+12u²)/D³,(-12u+4u³)/D³)`. At `u=1/2`, `C=(3/5,4/5)`, `C'=(-32/25,24/25)` and `C''=(-64/125,-352/125)`. Cylinder `Sv=(0,0,1)`, `Suv=Svv=0`. Independently check the circle equation and its differentiated identities, accounting for floating arithmetic in the checks.

### Polynomial and singular charts

Use a planar patch for constant first derivatives and zero second derivatives. Use a bicubic polynomial height graph

```text
S=(u,v,(u³*v²+2*u²*v+3*u*v³+u*v)/8)
```

with exact rational Bernstein controls obtained from the monomial-to-Bernstein coefficients `binom(i,k)/binom(3,k)` in each direction. Its nontrivial asymmetric mixed derivatives detect transposed control grids. Retain exact rational controls as author intent, and compute a second truth using their rounded binary64 homogeneous inputs; those answer different error questions.

The degree `(2,2)` map `(u²,uv,v²)` at `(0,0)` has zero tangents and second vectors `(2,0,0)`, `(0,1,0)`, `(0,0,2)`. Its six-jet succeeds although a normal cannot be normalized. Constant patches, degrees zero in either direction, endpoints, unequal degree pairs, supported weight extremes and invalid inputs belong in the same qualification corpus.

For generated cases, the exact model evaluates direct Bernstein sums and derivatives using exact rational values corresponding to the **provided binary64 bits**, not a binary64 copy of the CUDA algorithm. A small Python standard-library rational harness is sufficient for fixture generation; it is not the production geometry representation. Common weight scaling, affine transforms, parameter swap and subdivision/chain-rule checks add different metamorphic fault classes. Dense grids and finite differences alone cannot establish all the claimed derivatives.

## Register and storage tradeoffs to measure

A full bicubic patch uses 512 control bytes. Full Cartesian output is 144 bytes per query before status. Three row arrays of four homogeneous four-vectors would hold 48 doubles before outputs, corresponding to at least 96 32-bit register slots if all remained live. Storing six homogeneous vectors and all Cartesian vectors simultaneously increases pressure further.

The proposed scalar-channel schedule uses at most 12 row scalars plus a four-scalar univariate scratch array, and retains the denominator jet while processing one numerator channel. It trades some repeated basis work for smaller live state. These are source-level liveness estimates, **not compiler register counts or occupancy measurements**. CUDA's compiler may spill indexed arrays to local memory or increase live ranges when unrolling. Inspect generated register/local-memory counts on `sm_60`, and measure before choosing block sizes or forced register caps.

Begin with immutable geometry arrays and independent thread requests. Coherent access lets nearby threads reuse cached patch controls; random-patch access measures the other regime. A shared-memory block-per-patch path is an alternative when many queries share a patch, but synchronization, partial batches and occupancy determine whether it helps. Avoid putting all patch controls in CUDA constant memory: its benefit depends on warp address uniformity, and arbitrary patch access can serialize loads.

CPU and CUDA performance comparisons must emit identical requested outputs. Report resident device evaluation separately from upload, allocation, dispatch, download and complete API time. P100's CUDA 12/`sm_60` constraint and floating-point semantics are linked in the earlier [research report](2026-10-13-gpu-numerics.md). No throughput or modeling acceleration is established here.

## Choices ready for design review

Adopt the six-entry triangle and explicit homogeneous recovery as owned mathematics. Keep first scope local and normalized with a declared numeric envelope. Implement separate CPU and CUDA evaluation strategies with exact analytic/rational fixtures, errors and failure statuses. Use scalar-channel CUDA storage first and retain alternatives as experiments. Do not inherit denominator substitution, ignored error returns, mutable LUT caches, mislabeled arithmetic or out-of-bounds benchmark loops from the references.

The remaining design decision is the error guarantee level. A finite FP64 evaluator with measured fixture errors is a useful public first milestone. A query whose result can prune geometry or establish topology needs a conservative absolute error enclosure. That stronger contract requires its own operation-graph error analysis; successful status, positive weights and source parity do not supply it.

See [supplementary bibliography](first-iteration-numerical.bib), [source restoration TSV](first-iteration-numerical-sources.tsv) and [paper acquisition TSV](first-iteration-numerical-pdfs.tsv).
