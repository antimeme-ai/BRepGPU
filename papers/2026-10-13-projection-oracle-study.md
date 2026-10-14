# Seeded point-to-carrier projection: independent oracle study

2026-10-13. Research for iteration two; no solver or test implementation.
Read current project instructions and the existing iteration-one evaluator
contract. References are research snapshots, not selected dependencies.

## Contract that the oracles can establish

One request supplies an exact patch-snapshot slot, a local-frame target `X`,
and a seed `(s0,t0)` for the closed parameter rectangle `[0,1]²`. The useful
operation keeps its bounded iterative work on the selected backend and returns
a locally resolved constrained candidate, or a meaningful unresolved/failure
status. It does not search all patches, establish a global nearest point,
classify face trims, or infer a nonlinear imported source-UV chart.

The design must decide whether an accepted result means only first-order KKT
stationarity, or also a checked isolated local minimum. Those are different
claims. Recommend accepting a regular candidate only after KKT and a suitable
second-order check; otherwise expose nonminimum/curvature-unresolved/singular
outcomes. A first-order-only candidate must be named accordingly. Finite local
curvature tests are numerical checks, not universal error certificates.

For a returned binary64 UV, the independent oracle evaluates the actual
homogeneous input exactly using the existing `Fraction` jet machinery. It
then derives `S`, distance, gradient, metric and distance Hessian without
calling either native solver. Analytic fixtures below separately supply known
UV/3D/distance answers. CPU agreement cannot replace those expectations.

## Objective, normal equations and box KKT

Use `r = S(s,t)-X`, `f = dot(r,r)/2`, and `d² = 2f`. Direct differentiation gives

```text
g = (r·Ss, r·St)
G = [[Ss·Ss, Ss·St], [Ss·St, St·St]]
H = G + [[r·Sss, r·Sst], [r·Sst, r·Stt]]
```

The signs and the factor-one-half are explicit. At an interior candidate,
`g=0`; at a regular chart this is equivalent to the residual lying on the
surface normal line. At a lower bound `g_i >= 0`, at an upper bound `g_i <= 0`,
and at a free coordinate `g_i=0`. An edge or corner minimum need not satisfy
full residual/normal orthogonality. A useful first-order residual is
`u - clip(u-g,0,1)` with a declared scale/tolerance; the exact fixture oracle
also checks the individual free equations and active signs. Near-boundary
snapping must be part of the contract, not silently applied by the oracle.

`G` describes chart rank; `H` describes distance curvature. Positive `G` does
not make a stationary point a minimum. With box feasibility, the critical
cone consists of feasible tangent directions `v` satisfying `g·v=0`.
Strictly active coordinates with nonzero KKT multipliers are fixed in that
cone. Weakly active coordinates with zero multiplier retain their one-sided
directions. Negative `vᵀHv` in this cone disproves a local minimum; positivity
on its nonzero directions provides the ordinary strict second-order condition.
Rejecting every indefinite full Hessian at a strict boundary is too strong;
discarding every boundary coordinate before curvature testing is too weak.

All quantities above are our direct derivation. The standard KKT treatment
distinguishes the special sufficiency available for convex objectives; general
surface distance is nonconvex. [Boyd and Vandenberghe, §5.5](https://web.stanford.edu/~boyd/cvxbook/bv_cvxbook.pdf).

## Exact fixtures inside the iteration-one patch envelope

Controls below have positive weights in `[1,256]` and Cartesian controls in
`[-1,1]³`. Tensor records use `i*(q+1)+j`. Unless stated otherwise all weights
are one; extrusion in `t` uses the two endpoints `t=0,1` at each `i`.

### A. Orthogonal plane: unique interior, edge and corner answers

`S=(s,t,0)`, degree `(1,1)`, controls `(i,j,0,1)` for `i,j=0,1`.
For target `(a,b,c)`, the unique rectangle answer is
`s*=clip(a,0,1)`, `t*=clip(b,0,1)`;
`d²=(s*-a)²+(t*-b)²+c²`, `G=H=I`, oriented unit normal `(0,0,1)`.

| Target | Expected UV | Expected d² | Expected g |
| --- | --- | ---: | --- |
| `(1/4,3/4,1/2)` | `(1/4,3/4)` | `1/4` | `(0,0)` |
| `(-1/4,1/2,1/4)` | `(0,1/2)` | `1/8` | `(1/4,0)` |
| `(-1/4,5/4,1/2)` | `(0,1)` | `3/8` | `(1/4,-1/4)` |

Perturb seeds throughout the rectangle, include exact solutions as seeds, and
use all endpoint/inward-neighbor cases. Check actual UV and position, not just
a residual scalar: a solver that ignores its target can pass an on-surface
residual check when the seed itself lies on the surface.

### B. Tilted plane: clamping an unconstrained solution is wrong

`S=(s,t,(s+2t)/4)`, degree `(1,1)`, the existing exact plane fixture.
Its constant Gram/Hessian is
`[[17/16,1/8],[1/8,5/4]]`. For `X=(-1/4,1/2,1/4)` the unconstrained solution is
`(-5/21,11/21)`. Component clipping would return `(0,11/21)`, which violates
the free `t` equation. The actual rectangle answer is `(0,1/2)`,
`S=(0,1/2,1/4)`, `d²=1/16`, `g=(1/4,0)`.
Its oriented unit normal is `(-1,-2,4)/sqrt(21)`.

For arbitrary exact targets, an independent complete plane oracle can evaluate
the feasible interior linear solution, each of four one-dimensional edge
quadratic minima, and the corners, then compare exact squared distances.
That is a small exact analytic oracle, not another iterative projection solver.

### C. Rational quarter-cylinder: nonlinear interior and boundary solutions

Use the existing degree `(2,1)` carrier:
`C(s)=((1-s²)/(1+s²), 2s/(1+s²))`, `S=(C(s),t)`.
The three homogeneous circle rows are `(Ax,Ay,W)=(1,0,1),(1,1,1),(0,2,2)`;
each extruded record is `(Ax,Ay,tW,W)` at `t=0,1`.
For `d=1+s²`,

```text
Cs = (-4s/d², 2(1-s²)/d²)
Css = ((-4+12s²)/d³, (-12s+4s³)/d³)
Gss = 4/d², Gtt=1, Gst=0; oriented unit normal = (C,0).
```

For a target with positive planar coordinates `(a,b)`, let
`rho=sqrt(a²+b²)`. The interior arc answer is
`s*=b/(rho+a)`, `C*=(a/rho,b/rho)`; `t*=clip(z,0,1)`.
When `t` is interior, `d²=(rho-1)²` and `Hss=rho*4/(1+s*²)²`.
Choose dyadic target `(3/4,1,1/4)`: `rho=5/4`, expected UV `(1/2,1/4)`,
point `(3/5,4/5,1/4)`, `d²=1/16`, `g=0`, `H=diag(16/5,1)`.
Every control and target bit is exact; the expectation uses rational ratios.

Boundary targets include `(3/2,-1/4,1/2)` → UV `(0,1/2)`, `d²=5/16`,
`g=(1/2,0)`, and `(-1/4,3/2,5/4)` → UV `(1,1)`, `d²=3/8`,
`g=(-1/4,-1/4)`. These test bound signs and a `t` corner independently of
interior orthogonality.

The general analytic rectangle oracle maximizes `a cos(theta)+b sin(theta)`
on `theta∈[0,pi/2]` and solves the independent `t` interval. Candidate values
are both arc endpoints and the interior radial maximum when `a,b>0`.
Naively clipping an `atan2` angle is incorrect in the negative quadrant.

### D. Cylinder: two unequal local minima and an exact interior saddle

On the same carrier, target `X=(-3/4,-1,1/4)` gives:

| UV | d² | Classification and curvature |
| --- | ---: | --- |
| `(0,1/4)` | `65/16` | Strict lower-bound local minimum; `g=(2,0)`, `Hss=-3` |
| `(1,1/4)` | `73/16` | Strict upper-bound local minimum; `g=(-3/4,0)`, `Hss=-1/4` |
| `(1/2,1/4)` | `81/16` | Interior saddle; `g=0`, `H=diag(-16/5,1)` |

Both endpoint seeds are legitimate locally constrained solutions. Returning
the upper endpoint cannot be described as globally nearest because the lower
endpoint is closer. Both strict-boundary minima have indefinite full `H`,
but their critical cone fixes `s` and has positive curvature in `t`.
The midpoint is a regular-chart saddle, not a singular surface.

For `X=(0,0,1/2)`, every `(s,1/2)` has `d²=1` and `g=0`, while
`Hss=0` and `Gss>0`. This is nonisolated local ambiguity at a regular chart.
Returning one seed-selected stationary candidate is possible, but neither
unique UV nor an isolated-minimum result is justified.

### E. Parabolic cylinder: exact multiple interior minima

Let `a=2s-1`, `S=(a,t,a²)`, degree `(2,1)`.
The degree-two `x` controls are `(-1,0,1)` and `z` controls `(1,-1,1)`;
extrude in `t`. With `X=(0,1/2,h)`,
`d²=a²+(t-1/2)²+(a²-h)²` and
`g_s=2a(1+2a²-2h)`, `g_t=t-1/2`.
The chart is everywhere regular, `Gss=4+16a²`, `Gtt=1`.

At `h=3/4` there are two exact minima:
UV `(1/4,1/2)` and `(3/4,1/2)`, points `(-1/2,1/2,1/4)` and
`(1/2,1/2,1/4)`, `d²=1/2`, `H=diag(4,1)`.
Their oriented normals are `(1,0,1)/sqrt(2)` and `(-1,0,1)/sqrt(2)`.
The midpoint `(1/2,1/2)` has `d²=9/16`, zero gradient and
`H=diag(-2,1)`: never accept it as a minimum merely because a Newton step
vanishes. Near-left/right seed runs check local branch selection only within
a documented fixture basin; exact-minimum seeds require no basin inference.

At `h=1/2` the midpoint is the unique minimum but is quartically flat in `s`:
`d²=1/4+a⁴+(t-1/2)²`, `H=diag(0,1)`.
A conservative second-order solver may return curvature unresolved. It must
not call the chart singular or claim UV accuracy from gradient alone.

Weak-boundary attack: `S=(s,t,s²)` has controls `x=(0,1/2,1)`,
`z=(0,0,1)`. For target `(0,1/2,3/4)`, seed `(0,1/2)` is lower-bound KKT with
zero multiplier and `Hss=-1/2`. Feasible positive `s` reduces distance.
It must not become a minimum by removing all boundary coordinates from `H`.

### F. Singular and poorly conditioned charts

| Carrier | Witness | Independent expectation |
| --- | --- | --- |
| `S=(s²,t,0)`, degree `(2,1)` | `X=(0,1/2,1/4)`, seed `(0,1/2)` | Unique geometric closest point, but rank-one chart at the answer; normal/regular inverse claim fails |
| `S=((2s-1)²,t,0)` | `X=(1/4,1/2,1/4)` | UV `s=1/4,3/4` represents the same closest point; two local chart uses, not coordinate-welded identity |
| `S=(s,0,0)`, degree `(1,0)` | `X=(1/2,1/4,1/4)` | Rank-one carrier, nonunique `t`; a regular surface solver reports singular/unsupported |
| Constant carrier, degree `(0,0)` | Any finite target | Rank zero; no invented normal or isolated UV |
| `S=(s,epsilon*t,0)`, `epsilon=2^-k` | `X=(1/4,epsilon/2,1/4)` | Exact UV `(1/4,1/2)`, `d²=1/16`, `G=diag(1,epsilon²)`; mathematically regular but condition-dependent admission |

The last family requires a declared numerical regularity threshold. The report
should distinguish an ill-conditioned chart refused by the implementation
from a mathematically singular surface. At a singular seed a zero projected
gradient is not sufficient: that vanishing can be caused by a zero tangent.

## Failure oracles and output semantics

Status spelling belongs to the reviewed ABI, not this study. Distinct semantic
outcomes should cover invalid patch slot; invalid/nonfinite/out-of-domain seed;
invalid or unsupported target; nonfinite arithmetic; singular/ill-conditioned
chart; KKT with negative critical curvature; unresolved zero curvature;
iteration budget exhausted; and line-search/stagnation failure. Multiple distant
isolated minima cannot generally be detected by one seed, so do not promise
a global ambiguity detector. Nonisolated local curvature is detectable here.

A zero/small budget must not turn an unresolved seed into success. If the API
permits a zero-update budget, a nonstationary plane seed gives an exact budget
failure, while an already resolved minimum may succeed after its initial
checks. If budgets start at one, the design must specify counted evaluations/
accepted updates and select a fixture whose independent final residual remains
outside tolerance; do not duplicate the solver's iteration path as the oracle.
Failure fields must be deliberately defined: either NaN numerical answer
fields with separate finite diagnostics, or explicitly labeled last-iterate
diagnostics that callers cannot mistake for accepted projections. Preserve
request order, slot, seed/target snapshot and lineage through all statuses.

The same exact geometric outcomes should be exercised through Python on CPU
and each GPU. Residency tests should measure an actual projection batch with
geometry/queries already uploaded, then report setup/upload/download/Python
and failure counts separately. A speedup consisting of fast rejected queries
is not useful projection throughput. Analytic target/seed families should be
published with the result, rather than sampling only targets the solver itself
generated. Runtime ownership/device/fatal-error checks remain independent of
these geometric oracles.

## Primary literature and source inspection

1. **Hu and Wallner (2005), _A second order algorithm for orthogonal projection
   onto curves and surfaces_, CAGD 22(3), 251–260**, DOI
   [10.1016/j.cagd.2004.12.001](https://doi.org/10.1016/j.cagd.2004.12.001).
   [Author PDF](https://www.geometrie.tugraz.at/wallner/sproj.pdf) acquired.
   Read surface regularity, tangent-plane/curvature-circle iteration and
   experiments. Its geometric method is a relevant second-order alternative;
   empirical robustness does not establish rectangle boundary completeness or
   a global-nearest contract.
2. **Boyd and Vandenberghe (2004), _Convex Optimization_**, author-hosted
   [book](https://web.stanford.edu/~boyd/cvxbook/); PDF acquired, §5.5 read.
   Its convex sufficiency cannot be transferred to nonconvex surface distance.
3. **Botbol, Busé, Chardin and Yildirim, _Fibers of multi-graded rational maps
   and orthogonal projection onto rational surfaces_**, [arXiv:1903.08107v2](https://arxiv.org/abs/1903.08107v2),
   revised 9 April 2020; DOI [10.1137/19M1289522](https://doi.org/10.1137/19M1289522).
   Acquired/read v2 introduction, normal congruence and §6 algorithm. Matrix
   preprocessing and numerical algebra recover finite normal fibers under
   stated assumptions; root verification removes possible ghost candidates.
   This is a future multi-candidate research direction, not a trimmed-domain
   global-distance solver or selected dependency. The artifact filename uses
   first-submission year 2019; the read version is explicitly 2020 v2.
4. **Bao, Liu and Zou (2023), _GPU Parallel Inverse Evaluation of NURBS Surfaces
   Based on Matrix Representation_**, DOI
   [10.3724/SP.J.1089.2023.2023-00003](https://www.jcad.cn/cn/article/pdf/preview/10.3724/SP.J.1089.2023.2023-00003.pdf).
   Previously acquired/read primary PDF: adaptive candidate subdivision and
   Gauss–Newton refinement inform batching. Surface inversion for a point
   intended to lie on a carrier is a different contract from distance to an
   arbitrary off-surface target.
5. **Ma and Hewitt (2003), _Point inversion and projection for NURBS curve and
   surface: Control polygon approach_**, CAGD 20(2), 79–99, DOI
   [10.1016/S0167-8396(03)00021-9](https://doi.org/10.1016/S0167-8396(03)00021-9).
   [Author institution record](https://research-portal.uea.ac.uk/en/publications/point-inversion-and-projection-for-nurbs-curve-and-surface-contro/)
   inspected; full PDF not acquired/read in this lane. Candidate extraction
   followed by Newton refinement is relevant background; no detailed
   exclusion or convergence theorem is adopted from an abstract.
6. **Li et al. (2019), _A Geometric Strategy Algorithm for Orthogonal Projection
   onto a Parametric Surface_**, JCST 34(6), 1279–1293, DOI
   [10.1007/s11390-019-1967-z](https://jcst.ict.ac.cn/article/doi/10.1007/s11390-019-1967-z).
   Publisher abstract inspected; full text/theorem assumptions not obtained.
   Its initial-value-independence assertion is not accepted as a general
   guarantee for our singular, nonunique and constrained fixtures.

Pinned source read alongside the literature:

- **OCCT V8.0.1**, revision `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`.
  `src/ModelingData/TKGeomBase/Extrema/Extrema_FuncPSNorm.cxx` directly forms
  the correct normal equations and `G+r·D2` Hessian.
  `Extrema_GenLocateExtPS.cxx/.hxx` separates a seeded normal-root route from
  a distance route; `IsMinDist` samples a small neighborhood independently.
  `Extrema_GenExtPS.cxx` samples/presearches then invokes bounded roots.
  `src/ModelingAlgorithms/TKGeomAlgo/GeomAPI/GeomAPI_ProjectPointOnSurf.cxx`
  chooses the nearest among computed extrema. It is not our independent oracle.
  [Pinned normal-equation source](https://github.com/Open-Cascade-SAS/OCCT/blob/b8f597c677811d1f9f4d8a97f5ae2825c0353a42/src/ModelingData/TKGeomBase/Extrema/Extrema_FuncPSNorm.cxx).
- **OCCT distance-gradient caution.** In the inspected
  `Extrema_FuncPSDist.cxx`, `Value` returns squared distance while
  `Gradient/Values` form `gp_Vec(surfacePoint,target)·Si`. The pinned
  `gp_Vec` constructor is `P2-P1`: this is the negative half-gradient of that
  objective. This is a source-reading inconsistency, not an executed reference
  regression. Do not port the branch's sign/scale. Derive owned descent from
  the explicit objective above.
- **Truck**, revision `88ed005249e5e3a6b07f62425399435905cd3ab6`.
  `truck-geotrait/src/algo/surface.rs` uses a normal-line `(u,v,w)` Newton
  system for 3D nearest-parameter search; `search_parameter` instead uses
  `Jᵀr` with Gram approximation and then checks point agreement.
  `truck-geometry/src/nurbs/nurbssurface.rs` documents local hint dependence.
  `truck-base/src/newton.rs` stops on a small parameter update. These patterns
  do not supply our box KKT or minimum classification.
  [Pinned source](https://github.com/ricosjp/truck/blob/88ed005249e5e3a6b07f62425399435905cd3ab6/truck-geotrait/src/algo/surface.rs).
- **SISL**, revision `e0a82b1a851e932657f3e1af25033fbbcd33e907`,
  `src/s1773.c`: seeded iteration, explicit local/global warning, bounded
  parameter correction and 30-iteration cap. Actual updates use first
  derivatives/Gram; final distance/degeneracy status is not a complete KKT
  test. Its degeneracy branch contains `/l2*l2` where the corresponding
  one-direction squared-norm denominator would be `(l2*l2)`. No implementation
  is copied or reference success used as truth.
  [Pinned source](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s1773.c).

New PDF acquisition hashes/read scope are in `projection-oracle-pdfs.tsv`.
The annotated source/paper references above accompany `projection-oracles.bib`;
the already verified Bao et al. entry remains in `first-iteration-numerical.bib`.
Existing pinned restoration records remain in
`acceleration-roadmap-reference-restoration.md`,
`reference-catalog.tsv` and `first-iteration-numerical-restoration.md`.
Access-limited papers above are literature leads, not inspected authority.
