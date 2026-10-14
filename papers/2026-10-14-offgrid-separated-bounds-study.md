# Exact extrusion structure and separated off-grid bounds

2026-10-14. Research/design input only: no product implementation, native execution,
fleet activity, tests, builds or benchmark changes. The proposed first value unit
is **exactly recognized local Z extrusion with a separated Bernstein lower bound**.
Retain the current generic bound when recognition fails. Curvature/Taylor bounds
remain a follow-on hypothesis, not part of the first implementation proposal.

## What retained measurements show

The controlled [off-grid comparison](../results/discovery-2026-10-14/figures/offgrid-comparisons.tsv)
has 4,096 requests at depth 8, eight refinements and squared tolerance 2^-24.
Every row is coverage complete and none attains GAP. Bernstein axis mean width is
approximately 0.000732421875 plus outward-rounding effects; cylinder mean width is
approximately 0.001518 and maximum approximately 0.002070. The
[independent performance review](2026-10-14-discovery-performance-review.md)
records the different work patterns: axis exhausts the refinement budget, while
the off-grid cylinder does not. This is not evidence that increasing projector
iterations will resolve the lower-bound relaxation.

There is an exact explanation for the axis value. The supplied cylinder has
Ax=1-u², Ay=2u, W=1+u², Az=vW, hence its XY squared radius is exactly 1.
At depth 8 the alternating hierarchy has four splits per axis, so a V interval
has width h=1/16. An odd/64 target height sits at h/4 or 3h/4 in its nearest V cell.
For delta=a-Xz, the degree-2 Bernstein coefficients of the height term are
`delta², delta(delta+h), (delta+h)²`. The middle coefficient is -3h²/16=-3/4096.
Therefore the exact tensor coefficient lower on that cell is 1-3/4096, despite
the exact true minimum being 1. Separation restores the exact height lower 0.
The observed floating widths are not asserted to equal this rational value exactly.

A local exact Fraction calculation using the existing independent power-convolution
oracle verifies the endpoint identities and this explanation. For X=(0,0,15/64),
the root tensor lower is 3361/4096; a nearest depth-8 cell gives 4093/4096; both
separated exact lowers are 1. [Exact algebra record](offgrid-bounds-exact-algebra.tsv).
This calculation is paper algebra, not product or floating-enclosure qualification.

## Primary reading and reference limits

Hamadneh/Athanasopoulos/Ali 2019 section I.B bounds a rational polynomial by its
Bernstein numerator/denominator coefficient ratios when all denominator coefficients
are positive; section II explains why a positive function can have negative
coefficients. Endpoint interpolation supports selecting a V endpoint polynomial.
Its real-arithmetic results do not account for construction rounding.
[Institution paper](https://www.zuj.edu.jo/wp-content/uploads/IT_Research/IT_61.pdf).

Garloff/Schabert/Smith 2012 treats rational range dependency loss and replaces a
quotient with a linear approximant plus residual quotient. This supports studying
algebra that preserves structure, without making a claim that ordinary interval
cancellation will recover exact dependencies.
[Author paper](https://www-home.htwg-konstanz.de/~garloff/pamm_gss.pdf).

Boyd/Vandenberghe section 9.1.2, equations 9.7–9.9, derives a quadratic minorant
from a Hessian lower bound and the gradient-squared lower value. A positive Hessian
at one point does not establish its whole-domain premise.
[Author book](https://web.stanford.edu/~boyd/cvxbook/bv_cvxbook.pdf).

All three retained primary PDFs were re-read in the relevant full-text sections
and preserved in ignored worktree `papers/pdfs`; [catalog](offgrid-bounds-pdfs.tsv)
records identities/read scopes. [Bibliography](offgrid-bounds.bib). No new archive,
dependency, install or reference-code execution was needed. Earlier interval
arithmetic/source study remains in [bounds numerical study](2026-10-14-bounds-numerical-study.md).

The pinned openNURBS source is a warning about recognition semantics:
`ON_Surface::IsIsoparametric` accepts nearly linear curves with tolerance adjustments,
and `ON_NurbsSurface::IsPlanar` derives a nominal plane and biases coordinate-axis
directions using tolerances. Those are useful CAD recognition operations, but do
not prove the exact identities below. Reviewed source revision
`eb92af3ba1806b0a34a99aba0d3bda83e3d46083`,
[surface source](https://github.com/mcneel/opennurbs/blob/eb92af3ba1806b0a34a99aba0d3bda83e3d46083/opennurbs_surface.cpp),
[NURBS surface source](https://github.com/mcneel/opennurbs/blob/eb92af3ba1806b0a34a99aba0d3bda83e3d46083/opennurbs_nurbssurface.cpp).
Pinned SISL `s1958.c` uses boundary candidates then a heuristic interior seed/Newton
iteration; it does not supply the missing global interval certificate.
[Source](https://github.com/SINTEF-Geometry/SISL/blob/e0a82b1a851e932657f3e1af25033fbbcd33e907/src/s1958.c).
These intact references remain historical study material, not current instructions.

## Exact original-input recognition

For an already admitted finite patch of degree (p,1), p=0..3, read original
homogeneous records in canonical order `i*2+j`. Require, for every i:

```
Ax[i,0] == Ax[i,1]
Ay[i,0] == Ay[i,1]
W [i,0] == W [i,1]
Az[i,0] == 0
Az[i,1] == W[i,0].
```

These are exact finite-binary64 value equalities, never approximate tolerances,
samples or interval overlaps. Signed zeros can both represent exact real zero;
NaNs are already excluded by admission. Positive weights remain [1,256]. Source
control offsets must be retained; metadata lineage and a named-cylinder label
are not recognition evidence.

The linear V basis gives Ax(u,v)=Ax(u), Ay(u,v)=Ay(u), W(u,v)=W(u) and
Az(u,v)=vW(u), so **S(u,v)=(Ax(u)/W(u),Ay(u)/W(u),v)**. Consequently

`distance²(u,v)=Fxy(u)+(v-Xz)²`,
`Fxy=((Ax-Xx W)²+(Ay-Xy W)²)/W²`.

This holds for arbitrary admitted curve controls, folds, singular/constant curves
and weights; it is not unit-circle recognition. It is deliberately local-frame
Z extrusion with the original normalized V parameter. A Y-extruded parabola,
q=3 polynomial patch, translated/scaled height law or almost-extrusion falls back.
Coordinate permutation/general extrusion may be separate future scopes.

Use one shared recognition definition for CPU and CUDA, applied to original input
records. A simple first implementation can recognize once per query using at most
five comparisons per U record (20 at p=3), then pass the result to node bounds.
That avoids changing public/private ABI or cache metadata, at a measurable control
read/branch cost. Alternatively a private precomputed tag requires a reviewed
core/plugin handoff; never overload public patch flags or node leaf flags. Check
recognition parity on all modes/backends. Strengthen mode 2 only; modes 0/1 retain
their existing geometry recipe and nonrecognized mode-2 rows retain generic behavior.

## Reuse the unchanged five-net cache safely

Let a=`v_index/2^v_level`, b=`(v_index+1)/2^v_level` be the **global normalized**
V cell endpoints. With local eta in [0,1], v=a+(b-a)eta; this scaling must not be
confused with the node-local Bernstein parameter. The restricted homogeneous
height is `(a+(b-a)eta)W(u)`. At eta=0, the exact degree-(2p,2) cache coefficient
identities are

```
C0[r,0] = B_r(Ax²+Ay²) + a² D_r
C1[r,0] = B_r(Ax W)
C2[r,0] = B_r(Ay W)
C3[r,0] = a D_r
C4[r,0] = D_r = B_r(W²) > 0.
```

Here B_r means the univariate Bernstein coefficient of the U-restricted
polynomial, not evaluation at r/(2p). V endpoint interpolation proves the slice;
exact extrusion makes XY/W independent of V. Thus the proposed expression
`C0-2(Xx C1+Xy C2)-2a C3+(Xx²+Xy²+a²)C4` is valid, and proven C3=aC4 simplifies it to

`Nxy_r=C0[r,0]-2(Xx C1[r,0]+Xy C2[r,0])+(Qxy-a²)C4[r,0]`.

Recommended first recipe: precompute outward Qxy=Xx²+Xy² once/query; for each
node form exact dyadic a and a² (level<=12, integer square needs at most 24 bits),
enclose Qxy-a², then stream r=0..2p using existing outward interval multiply/sum/
subtract and positive-denominator ratio rule. The exact integer/exponent route
for a² must be named and checked, rather than assuming arbitrary multiplication
is exact. Using ordinary outward multiplication is also safe but can be wider.

For q=1 the active coefficient index is `r*3+0`. Packed endpoint pair address is
`node_id*490 + polynomial*98 + 2*(3*r)`. Read polynomials 0,1,2,4 only, and require
positive C4 lower. The existing cache export/layout remains 490 doubles/node;
unread padding stays unread. There are 2p+1 active ratios instead of 3(2p+1), with
eight coefficient doubles per ratio instead of ten. Cache construction still
encloses the exact restricted polynomials even if duplicate V controls gained
interval width during subdivision. Do not replace those intervals with nominal
coefficients or cancel interval endpoints as if they were equal exact scalars.

Take `Lxy=max(0,min_r ratio_lower(Nxy_r,D_r))`. Negative polynomial coefficients
are allowed; only the resulting squared-objective lower is clipped to zero.
Let `gap=max(0,down(a-Xz),down(Xz-b))` and `Lz=down(gap²)`, clipped to zero.
Then `Lsep=max(0,down(Lxy+Lz))` bounds every point in that node. The exact box
is a product in u,v, so the separate minima may be combined. Return
`max(current_generic_node_lower,Lsep)` to preserve the old per-node strength.

The first comparison should retain generic mode-2 evaluation before the maximum;
it isolates bound correctness but adds work. An alternative separated-only path
retaining AABB can lose generic strength through FP cancellation, and needs its
own measured/reviewed decision. Do not hide that tradeoff by claiming fewer ratios
while computing both full recipes. The exact separated lower may still be loose
off-axis because the U coefficient relaxation remains. Existing large cache bytes
do not shrink merely because the new query reads fewer elements.

## Oracle, fallback and compatibility changes

The existing flat generic lower ceiling is intentionally weaker than this new
recipe and will reject valid strengthened output. Revise the oracle for recognized
rows rather than relaxing comparisons. Independently derive the original input
power polynomials and verify zero V dependence of Ax/Ay/W and Az-vW identically.
Form exact XY numerator by direct power convolution of `(Ax-XxW)` and `(Ay-XyW)`,
restrict only U, convert to Bernstein, and add exact distance from Xz to [a,b]².
This does not mirror cache cancellation or traversal. Exact combined node ceiling
is max(old exact node bound, exact separated bound); final ceiling is the root
under cap or minimum over all complete leaves. Original-UV upper checks remain
exact and unchanged. Exported cache coefficients retain their current oracle.

Attack one-ULP changes to each recognition identity, wrong original offset,
nonzero/global V cells, p=0..3, weight extremes, negative XY coefficients, outside
heights, -0, folds and continuum minima. Near-extrusions must use the generic path;
use raw output identity there when the recipe truly stays unchanged. Both CPU and
CUDA must qualify the strengthened bound independently, not by parity. Recognition
alone does not tighten a witness: p=0 vertical lines can retain a rank-unresolved
projector seed and therefore a loose U. Candidate/root seed changes are outside
this first bound proposal.

Unrecognized geometry falls back normally; invalid original inputs remain admission
errors. Nonpositive denominator, nonfinite required interval or empty invariant
intersection in the selected mandatory recipe is arithmetic failure, never fake
GAP. Global V identities/degrees must be validated before exact-cell arithmetic.
All original gap/coverage/cap/status semantics remain. Stronger pruning can change
refinement seeds, UV witnesses, work counters and output bits: the projector may
leave its seed cell, so a per-node stronger lower does not promise identical final
UV or monotonic final U for a fixed refinement budget.

Keeping the cache meaning and ABI unchanged can make this a compatible numerical
strengthening, but it is a changed algorithm and needs its own design/plan review.
An actual old plugin may still return weaker yet valid results for recognized
rows; compatibility must accept containment/unresolved status, not require new
GAP population or equality to the new method ceiling. Do not reinterpret its data
as a new cache mode. Any new public mode, flags/counters or private tag handoff
must be explicitly designed/versioned rather than waved away.

## Small proposed measured-value experiment

After design/plan/oracle review, compare current generic mode 2 with exact-recognized
separation at depths 0/2/8 for axis_offgrid and cylinder_offgrid, N=4096, unchanged
targets, tolerance, visit/refinement settings, CPU1/CPU24 and each P100. Start with
root-only axis to test the specific height-relaxation hypothesis. Include a small
near-extrusion/generic polynomial control group and constants/folds for correctness;
do not expand into a new degree Cartesian timing campaign.

Measure attained squared widths/GAP population, visits/refinements/evaluations,
recognition and bound work, resource use, unchanged cache bytes/build/upload,
resident allocation+operation+sync, cached host consumption and setup. At N4096 the
axis inputs have 32 distinct heights, cylinder 1024 targets repeated four times;
retain that limitation. Root-only axis could require only one 80-byte node plus
3920-byte mode-2 cache and root array, but only actual qualified timings and U
justify selecting it. No synthetic speedup, guaranteed closed gap or default
depth selection follows from the mathematical identity.

## Curvature/Taylor follow-on, deliberately excluded initially

For F=N/D and D>0, whole-cell interval bounds can derive
`F'=(N'-D'F)/D`, `F''=(N''-D''F-2D'F')/D`. Bernstein finite differences bound
N/D derivatives in node-local U; derivatives scale by cell width under a global
parameter change. Anchor gradient and curvature must use the same parameter;
the scale cancels in g²/m only when both are consistent. A pointwise metric or
projector Hessian is not a whole-cell curvature lower.

If finite m_lower>0 is certified on the whole U cell and a feasible anchor has
F0>=F0_lower, g in [glo,ghi], a safe bound is
`F0_lower - max(abs(glo),abs(ghi))²/(2*m_lower)`, using outward upper penalty and
downward subtraction. The unconstrained quadratic minimum is still a lower bound
when its minimizer lies outside the cell, though it can be weak at boundaries.
Zero/negative/unresolved curvature falls back; arbitrary tiny positive m can
overflow the penalty, so a reviewed optional path must skip unusable certificates
rather than publish a nonfinite lower. A unit-circle axis objective is constant,
so this strict-positive-curvature hypothesis does not address the first axis win.
Direct interval quotient dependency may fail to certify positive curvature even
where the true function is convex. Higher-degree exact polynomial certificates
would add per-query work/storage and require a distinct independent oracle.

For the cylinder the XY objective need not be convex over the full parameter
domain, despite a positive second derivative near a radial minimum. Boundary,
folded and flat cases rule out treating local Newton success as a convexity proof.
This is a useful later hypothesis for the remaining off-axis U relaxation, with
its own proposed cost/value comparison. It is not needed to deliver or assess the
first exact-extrusion separation unit.
