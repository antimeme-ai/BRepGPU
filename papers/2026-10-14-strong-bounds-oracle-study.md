# Stronger discovery bounds: exact oracle and whole-operation experiment

Research/design input, 2026-10-14. The conservative AABB delivery continues. Compare an owned 13-direction/26-DOP slab variant against that verified baseline on the complete seed-free discovery operation. Introduce the scalar squared-distance Bernstein alternative only through the same oracle and delivered-value comparison. No product/test code or library adoption is authorized by this report.

## 13 integer directions and the actual lower bound

Choose one representative from each opposite pair:

    (1,0,0), (0,1,0), (0,0,1),
    (1,1,0), (1,-1,0), (1,0,1), (1,0,-1), (0,1,1), (0,1,-1),
    (1,1,1), (1,1,-1), (1,-1,1), (1,-1,-1).

Their exact norm squared is respectively1,2 or3. Do not normalize stored directions or use approximate sqrt normals. For each exact restricted Cartesian control P_i and direction n, construct outward support [l_n,h_n] enclosing all n·P_i. Positive rational convex combinations imply every point of the exact subpatch satisfies l_n <= n·S <= h_n. The 26 halfspaces therefore enclose the original carrier, including degenerate and nonunique images.

For an exact target X, distance to this individual slab is

    d_n² = max(l_n-n·X, 0, n·X-h_n)^2 / (n·n).

For outward target projection [qlo,qhi], use conservative gap max(0,down(l_n-qhi),down(qlo-h_n)); square and divide downward by exact1,2,3. Negative axes select signed interval endpoints; every sum/subtraction is outward. A nearest-rounded q can produce an unsafe separation at a boundary. Support values must be computed from the actual restricted controls with enclosing arithmetic, not from sample points, and not by projecting the already loose AABB if the intended benefit is control-hull correlation.

**Keep the baseline AABB distance sum.** Max individual slab distance² alone is not guaranteed stronger than it: for a singleton at0 and X=(3,4,0), AABB lower²=25, while the maximum over these13 directions is49/2=24.5. The variant lower is max(AABB_lower², all additional slab_lower²). Use the same retained AABB values and baseline arithmetic so the enhancement never discards that available lower information.

This maximum is a valid lower bound on distance to the intersection of slabs; it is not the exact point-to-26-DOP distance. Jointly solving the polytope distance could tighten it, but a numerical feasible QP solution is an upper bound to that polytope distance and unsafe for culling. A reviewed dual/separating certificate would be needed. That is a separate method, not silently supplied by naming a DOP.

Intersect child directional supports with parent supports to maintain nesting. The intersection remains sound; empty intersection is an invariant failure, never evidence of an empty exact carrier. Fixed integer axes form a closed set under coordinate permutations/sign changes, useful exact metamorphic tests. Arbitrary rotations do not preserve this13-direction set and cannot be assumed performance invariant.

## Direct oracle, not parity

Reuse the [independent restriction proposal](2026-10-14-discovery-oracle-design.md): exact original Bernstein controls -> power basis -> dyadic affine restriction -> exact child Bernstein controls. For every node and all13 directions, calculate exact Fraction support min/max of n·(A_i/W_i). Require downloaded l_n <= exact min and h_n >= exact max, with no tolerance. This validates actual child hull enclosures, including cancellation/subnormal errors which sampling would miss.

Require retained AABB axis supports consistent with the canonical direction layout and parent nesting. Using emitted supports and the exact binary target, calculate exact slab distances and AABB distance. For complete coverage, an independent flat leaf reduction over max(AABB,slab distances) gives the method-specific lower oracle. For visit-limit fallback, use the exact root combined bound. Exact original-patch value at returned UV remains the upper witness oracle; the projector and witness arithmetic are unchanged. Analytic truth checks global brackets and squared-gap accuracy. Node IDs, coverage flags, cap semantics and source binding remain baseline obligations.

Download all cached support intervals through a reviewed qualification/API capability bound to the same hierarchy and integer direction table; no copied private pointer graph. If only AABB export is available, it cannot qualify the additional support enclosure. A CPU/DOP numerical match does not fix this missing evidence.

Specific new attacks: n norm divisor omitted or wrong (1/2/3); dot sign reversal; axis-table permutation; positive/negative endpoint swap; cancellation of large opposite control components; zero-width support classified empty; nearest-rounded target dot; root/child supports computed from nominal rounded nets; stale DOP storage after source replacement; a weak max-slab replacement of the stronger AABB sum; support array tail/stride omission; narrowed float projection; and a tighter box method incorrectly credited for failures caused by a different refinement budget.

## Analytic workloads where strength pays, or does not

- Diagonal plane S=(s,s,t), X=(3/4,1/4,1/2): target is inside the root AABB, so AABB lower0. Exact support for n=(1,-1,0) is0 and slab lower²=(1/2)²/2=1/8, the true global minimum. Correct UV=(1/2,1/2). This should reduce whole-operation uncertainty/work at shallow depths.
- Three-axis plane S=(s-1/2,t-1/2,1-s-t), X=(1/4,1/4,1/4): AABB lower0; n=(1,1,1) support0 gives3/16, exactly global d² at UV=(1/2,1/2). It directly attacks norm3 arithmetic.
- Constant origin and X=(3,4,0): exact d²25. Pure max slabs49/2 expose failure to retain baseline AABB sum.
- Tilted generic planes whose normals are *not* in the13 directions test finite-direction limitations; there is no arbitrary orientation accuracy theorem. Near-carrier and exact on-surface targets also show when extra support traffic buys little.
- Rational quarter-cylinder axial target X=(0,0,1/2): root AABB lower0, n=(1,1,0) support min1 gives at least1/2, true d²1. The minimizer continuum remains valid. Direction supports improve rejection but cannot generally capture the exact curved carrier merely by intersecting a fixed finite set of halfspaces.
- Quarter-cylinder exterior/interior-angle targets, unequal endpoint-local minima, symmetric endpoint ties, parabola twin minima/saddle/quartic-flat and fold/rank-deficient carriers reuse the established exact fixtures. Include arbitrary bounded rational bicubic weights and high-survivor curved targets so the experiment is not only an axis-aligned tutorial win.

## Scalar squared-distance Bernstein alternative

For a restricted exact homogeneous net, let b_i=A_i-W_i X. Product Bernstein rules construct degree-(2p,2q) numerator N=||sum B_i b_i||² and denominator D=(sum B_i W_i)². At bicubic degree there are49 coefficient pairs. Every D coefficient is positive. The rational scalar coefficients N_i/D_i enclose the entire squared-distance range by the positive-weight hull argument; max(0,min_i N_i/D_i) is safe. Some N coefficients can be negative despite the nonnegative function. Clamping the *lower result* to0 is safe; silently dropping negative coefficients is unsafe. Strictly positive lower coefficients of N-U D can also prove exclusion without ratios. All product/combinatorial factors, denominator and subtraction require interval enclosure in product.

Independent oracle uses power coefficients for each restricted A_x,A_y,A_z,W, forms exact polynomials sum(A_k-X_k W)^2 and W² by exact convolution, then converts to Bernstein degree(2p,2q). This differs from production Bernstein-product loops. Check every exposed coefficient interval and positive denominator, then calculate exact rational coefficient lower. Combine with AABB/DOP lower if those cheaper bounds are already available. The witness upper and traversal/cap contract stay unchanged.

There is a concrete rational nonlinear win: quarter-cylinder X=(0,0,1/2) has exact distance function1+(t-1/2)^2, independently of rational s. In t's degree2 Bernstein form its root scalar ratios are5/4,3/4,5/4. After one split at t=1/2, both halves have minimum coefficient1, exactly global d²; the u-dependent denominator factors cancel in the ratio. Thus a shallow hierarchy can provide a near-exact lower once t is split, while hull/slab bounds still approximate the arc. It supplies an actual workload hypothesis rather than a claim that coefficients are always superior.

The costs are substantial. Starting from interval homogeneous nets costs up to16*4*2 doubles=1,024 bytes/node, versus208 support bytes/node. Caching query-independent AA=sum A_k², three AW_k, and W² scalar coefficient maps is5*49*2 doubles=3,920 bytes/node. Computing query-specific N,D and restricting them instead requires up to98 interval coefficients (1,568 bytes) of scratch before traversal state. GPU local-memory/register pressure and repeated conservative products may erase pruning gains. Polynomial/preweighted cases can use less storage; rational cases cannot be declared polynomial by dropping W. The reviewed experiment must choose the concrete residency/construction path before implementation, not report a coefficient microbenchmark as complete discovery value.

## Whole-operation experiment and selection rule

Keep identical source inputs, hierarchy depth/cell identities, target bits, squared tolerances, visit/refinement caps and initial witness/refinement policy. For DOP retain baseline AABB storage/arithmetic. Different culling may legitimately change subsequent witnesses/work; exact witness equality is not required, but every result must independently satisfy its method's certificate and shared semantics. Fully qualified row populations must accompany performance; compare achieved gap and unresolved populations, not only latency of whichever rows happened to finish.

Report preprocessing time, retained/download/upload bytes, repeated resident traversal+refinement+certified witness evaluation, native complete calls and actual Python caller consumption. At depth8 a full binary tree has511 nodes: baseline80-byte POD ~39.9KiB/patch; straightforward DOP240-byte POD ~119.8KiB/patch. The support endpoint traffic grows from48 to208 bytes/node. A separate extra-support array may avoid changing baseline ABI, but does not remove bandwidth or admission costs.

Run CPU1/24 and both P100s on batches32 through1,048,576 where admitted, with repeated samples and counter histograms. Include diagonal planes, curved unique targets, endpoint competitions, minimizer continua, degenerate carriers, far easy targets and cap-limited rows. Measure break-even repeated query count for preprocessing and extra upload; one could outperform resident work but lose for a single caller. Do not assume fewer visits entails less total time or that tighter mathematical boxes imply narrower outputs under changed witness order.

Choose the stronger method when independent exact qualification passes and actual task latency/achieved precision justifies its storage/work cost on named workloads. Retain AABB when it wins; a benchmarked option or documented workload policy is legitimate. No claimed P100 speedup exists before these complete-call measurements. Baseline delivery must not be blocked by an unimplemented strongest-method search.

## Primary literature and acquired bytes

[Klosowski, Held, Mitchell, Sowizral and Zikan (1998)](https://doi.org/10.1109/2945.675649), [free university-hosted paper](https://www.cs.cmu.edu/afs/cs/project/pscico-guyb/realworld/www/slidesF10/00675649.pdf): read sections3–4 and fixed6/14/18/26-DOP descriptions. They make explicit the competing costs of bound tightness, overlap work and updating, on polygonal collision hierarchies. This supports the direction representation and complete-operation cost comparison, not outward FP64 rational geometry or GP100 speedups.

[Zachmann (1998), Rapid Collision Detection by Dynamically Aligned DOP-Trees](https://user.informatik.uni-bremen.de/zach/papers/vrais98.pdf): read sections3–5. Opposite direction pairs form slabs and interval overlap tests; rigid realignment is a separate construction. Our patch-local metric needs no dynamic alignment, and its polygonal collision results do not certify point-distance lower arithmetic.

[Chen et al.(2008)](https://doi.org/10.1016/j.cad.2008.06.008), acquired in the earlier [bounds PDF catalog](bounds-oracle-pdfs.tsv), supplies squared-distance rational Bézier coefficient exclusion for curves. Its one-dimensional termination claims are not applied to surfaces.

[MiSo, Sichetti et al.(2025)](https://doi.org/10.1145/3731207), [author preprint](https://fsichetti.gitlab.io/papers/miso.pdf), already acquired/read by the numerical lane and reread here in sections4–5: polynomial inclusion maps and expression decomposition trade coefficient construction cost against tightness; conservative interval execution is distinct from its faster ordinary backend. Rational expression handling does not automatically supply specialized rational objective collapse. This supports measuring our specialized coefficient option without adopting its DSL/NFG implementation.

See [strong-bound bibliography](strong-bounds-oracles.bib) and [PDF catalog](strong-bounds-oracle-pdfs.tsv). No new source dependency or code execution was required. These sources ground the experiment, while exact restrictions and analytic formulas independently qualify its actual arithmetic and geometry claims.
