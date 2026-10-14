# Three owned bounds modes: comparative numerical design

2026-10-14. Complements the [discovery numerical design](2026-10-14-discovery-numerical-design.md).
No implementation precedes design and plan review. The operator requested
serious performance trials informed by computational geometry literature.
This delivery therefore implements and qualifies mode0 AABB, mode1 DOP26 and
mode2 BERNSTEIN, then measures all three before choosing a default. Stronger
methods that lose remain explicit opt-in experiments with source/results;
operation counts alone do not settle the question. No dependency is adopted.

## Actual primary-source inspection

Klosowski/Held/Mitchell/Sowizral/Zikan1998 sections3.2/3.3.3 and5 describe
fixed opposing support planes, six/14/18/26-DOP directions, interval projection
tests and construction/query/memory tradeoffs. Their26-DOP normals use entries
{-1,0,1}, avoiding normalized-direction multiplication. Our outward rational
support construction and point-distance lower bound extend this structure;
their triangle collision implementation is not an FP64 enclosure oracle.
The printed IEEE identity was checked in the acquired full-paper mirror after
direct publisher acquisition failed. [DOI](https://doi.org/10.1109/2945.675649),
[paper mirror](https://scispace.com/pdf/efficient-collision-detection-using-bounding-volume-3we401iylh.pdf).

Hamadneh/Athanasopoulos/Ali2019 sectionI.B andII state the rational
Bernstein coefficient-ratio enclosure under strictly positive denominator
coefficients and show nonnegative functions with negative coefficients.
Real-arithmetic subdivision/degree-elevation convergence does not excuse
rounded construction errors. [Primary institution PDF](https://www.zuj.edu.jo/wp-content/uploads/IT_Research/IT_61.pdf),
[DOI](https://doi.org/10.1109/JEEIT.2019.8717503).

Garloff/Schabert/Smith2012's two-page paper highlights dependency loss from
separate numerator and denominator range division, then constructs a linear
approximant plus residual quotient. The proposed first objective mode uses
coefficient-wise rational ratios; a residual approximant remains a distinct
candidate after this concrete comparison. [Author PDF](https://www-home.htwg-konstanz.de/~garloff/pamm_gss.pdf).

Damkjær/Erleben2009 section2 selects AABBs partly for their memory footprint
in blocked GPU BVHs. This cautions against assuming tighter geometry bounds
always win; their old-GPU tandem triangle traversal differs from this named
rational-patch query. [Author PDF](https://erleben.github.io/pubs/2009/damkjaer.erleben.09/damhjaer.erleben.09.pdf).

[Acquisition catalog](discovery-strong-bounds-pdfs.tsv) records byte identities,
hosts, failures and actual read scopes. Entries are appended to the existing
[numerical bibliography](bounds-numerical.bib). Existing MiSo/TIGHT studies
supply the elementary interval construction distinction.

## Common experiment contract

The existing80-byte hierarchy node, root array, traversal, witness construction,
fixed projector settings, cap behavior, fields and counters apply to all modes.
Mode is enum-valued in build flags:0/1/2;3 and other values reject. Auxiliary
cache contains respectively0/20/490 doubles per node, with checked extents and
generic actual-resident cache download. Modes are mutually exclusive. CPU builds
all caches under its checked RN/gradual-underflow environment and uploads the
same bytes to each CUDA engine. Query arithmetic uses CPU expanded-RN or CUDA
directed primitives; independently qualified endpoints need not be identical.
Every stronger node lower is `max(AABB_lower,mode_lower)`.

## DOP26: homogeneous control projections

Use coordinate directions plus, in order,
`(1,1,0),(1,-1,0),(1,0,1),(1,0,-1),(0,1,1),(0,1,-1),`
`(1,1,1),(1,1,-1),(1,-1,1),(1,-1,-1)`.
Squared norms are exactly2 for the first six,3 for the last four. No square
root/normalization is needed.

For each enclosed homogeneous child control compute interval `n·A`, using exact
sign negation/endpoint selection and outward x/y/z addition, then divide by
its strictly positive W interval. Min/max of those quotient intervals encloses
every exact Cartesian control projection. Positive rational hull weights put
the entire carrier inside these supports. Intersect with parent supports and
known `[-sum(abs(nk)),sum(abs(nk))]`. Project homogeneous numerator before
division: projecting Cartesian AABB corners is safe but loses the intended
control-hull correlation. Empty intersections are arithmetic failure.

Store20 doubles/node as ten lower/upper pairs. Coordinate supports use the
existing AABB. At query setup outward-enclose each target dot product once:
`n·X in [xlo,xhi]`. For a node support `[a,b]`, compute

`g=max(0,down(a-xhi),down(xlo-b))`,
`slab_lower=down(down(g*g)/norm_squared)`.

Take `max(AABB_lower,max_slab slab_lower)`. The target endpoint nearest the
slab is necessary for a lower gap. Summing nonorthogonal slab distances is
unsafe; several directions can describe the same residual. This is a
separating-slab bound, not an exact distance to the DOP/control hull. Retaining
the combined-coordinate AABB bound also avoids weakening it to the maximum
of three separate coordinate gaps.

DOP adds160 bytes/node: depth8 adds81760 bytes/patch; depth12 adds1310560.
Ten target projections require14 real additions/subtractions, with outward
endpoints, once/query. Each node adds up to20 lower subtractions, ten squares,
ten norm divisions and maxima, plus20 double loads. Norm2=2 permits exact
power-of-two scaling subject to underflow semantics; norm2=3 uses directed
division. CPU nextafter overhead and CUDA instruction cost must be measured.
Visits still count one complete node-distance calculation, not one slab.

## Bernstein: five cached quadratic bases

For each node's child homogeneous tensor polynomial cache interval Bernstein
coefficient nets at degree `(2p,2q)`:
`C0=Ax²+Ay²+Az²`, `C1=Ax W`, `C2=Ay W`, `C3=Az W`, `C4=W²`.
There are `K=(2p+1)*(2q+1)<=49` active coefficients. Pack490 doubles/node as
five nets of49 lower/upper pairs, active coefficient index `r*(2q+1)+s`.
All padding pairs are[0,0] and never enter query arithmetic; C4 padding must
not enter positivity checks or minima.

Use the tensor product identity
`B_i^p B_k^p = [choose(p,i)choose(p,k)/choose(2p,i+k)] B_(i+k)^(2p)`
and its q counterpart. Compute binomial integers exactly (degree<=6), form
the complete factor numerator/denominator as small exact integers, convert
exactly to doubles and outward-divide to enclose each rational factor. A
rounded literal such as9/20 is not an exact coefficient. Degree0 gives1.
Accumulate ordered input-control pairs in increasing canonical index order,
with outward product/multiply/sum into index `(i+k,j+l)`. Construction uses
interval homogeneous nets enclosing the original restriction, never rounded
nominal children or Cartesian box corners.

For each active product coefficient, the exact nonnegative Bernstein product
weights sum to1. Thus exact C4 coefficients are convex combinations of
`W_i W_j`, lie[1,65536] and stay strictly positive. Intersect their enclosures
with[1,65536]; fail if empty. Exact C1..3 coefficients lie[-65536,65536] and
C0 lies[-196608,196608], allowing those invariant intersections. **C0 may have
negative coefficients:** degree1 numerator controls(-1,1) produce squared
coefficients(1,-1,1). Never clip C0 to nonnegative despite its nonnegative
polynomial function.

At query setup outward-enclose `Q=Xx²+Xy²+Xz²` in x/y/z order and intersect
with[0,48]. For every active coefficient evaluate

`N_i=C0_i-2*(Xx*C1_i+Xy*C2_i+Xz*C3_i)+Q*C4_i`

with general interval multiplication and displayed fixed add/subtract order.
Factor2 is exact but the multiplication still rounds outward. It encloses the
exact coefficient of `sum(A_k-X_k W)^2`; denominator is `D_i=C4_i>0`.
This expansion loses dependencies, especially near cancellation, but remains
conservative. No invariant can turn those lost dependencies into a narrow
interval by empirical padding.

The ratio lower can use four downward endpoint divisions/minimum or the
equivalent reviewed sign formula:
`r_i=down(Nlo/Dlo)` for Nlo<0; otherwise `r_i=down(Nlo/Dhi)`.
Recommend the latter one-division query implementation. Denominator lower
must remain positive. Then `mode_lower=max(0,min_active_i r_i)`.
Proof: exact positive D coefficients normalize Bernstein basis values to
nonnegative weights summing to1, so the exact objective is a convex combination
of exact N_i/D_i. Interval lower endpoints bound each ratio. Clamp only the
resulting objective lower to0, not the polynomial coefficients. Coefficient
upper ratios are not feasible witnesses and do not replace original-patch U.

Stream one coefficient at a time with short interval temporaries/running
minimum; no per-thread49-coefficient output/scratch net is required. A node
reads ten doubles per active coefficient; its query needs at most four
general interval products, a small sum/subtraction chain and one ratio division
per coefficient. The256 pair convolution is setup work performed once/node.
Record its construction cost, temporary scratch and allocated cache bytes.

Bernstein auxiliary cache costs3920 bytes/node: depth8 adds2003120 bytes/patch
(about1.91MiB); depth12 adds32108720 (about30.62MiB). A total-node cap1048576
allows4110417920 auxiliary bytes before80-byte nodes, roots, original controls,
queries/results. Checked byte admission/device allocation failure are material.
No silent depth reduction, partial cache or mode fallback is allowed. Interval
cancellation/high weights can make a Bernstein bound loose; cache size and
lower tightness are both experimental quantities.

## Small validated comparisons and default selection

Land the correct AABB path first; then implement both owned stronger modes
behind the explicit enum after review. Exact exported-child supports and
quadratic product coefficients qualify setup; exact target/ratio calculations
qualify query lower bounds. Qualify original UV witnesses and final brackets
independently for each mode before timing. Root/leaf/cap/projector counts retain
the same meanings. Arithmetic failure cannot masquerade as fast rejection.

Compare depths0/2/4/8 on axis-aligned planes; diagonal planes whose hull
separates a target inside their AABB; supplied rational cylinders with axis and
diagonal targets; folds/equal minima/saddles; and constants/continuous minima.
A direct diagonal fixture is `S=(s,t,s+t-1)`, `X=(1/2,1/2,1/2)`. All controls
are in the cube and AABB contains X. Direction(1,1,-1) has support1, target
projection1/2, hence exact slab lower1/12; the orthogonal projection is within
the patch and exact global minimum is1/12. This directly attacks the claimed
hull improvement. Degree1 `S=(2s-1,t,0)` with normal targets attacks negative
middle squared coefficients; curved/folded cases test differences between
spatial support and objective bounds.

Hold targets/order, witness budgets and gap semantics constant. Measure CPU1,
CPU24 and each P100: setup, actual cache bytes/upload, full resident discovery
including fresh result allocation/sync, host consumption, visits/refinements,
gap-qualified fraction, unresolved reasons and bracket widths. Show repeated
query break-even from measured setup differences and full-call savings,
including cases with no positive break-even. Modes/backends may perform
different useful work because pruning differs; disclose those counts. Neither
prune count nor a kernel-only speed alone chooses the default. Source,
resource reports and qualified comparative benchmark artifacts land in this
iteration even if the stronger modes lose; no quality/speed winner is assumed.
