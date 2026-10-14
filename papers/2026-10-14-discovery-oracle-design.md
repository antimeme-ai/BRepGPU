# Iteration 3: independent discovery oracle and caller proposal

Design/corpus input, 2026-10-14. No product or test implementation is authorized by this document. Parent owns the reviewed product design/plan; numerical and runtime owners own their respective implementation. This proposal narrows the earlier [bounds study](2026-10-14-bounds-oracle-study.md) to one supplied patch per local target, a complete CPU-built dyadic hierarchy uploaded once, resident automatic witness discovery/refinement, and a squared-distance bracket. There is no public candidate enumeration or patch-union/world-metric selection in this increment.

## Contract to qualify

The exact object is the rational carrier defined by the accepted binary64 homogeneous controls, degrees 0..3, positive weights in [1,256], local Euclidean controls in [-1,1]^3 and closed normalized UV square. Targets are exact binary64 local coordinates under the reviewed target envelope. Chart metadata does not trim this carrier. Curves, constant surfaces, singular/folded carriers, repeated XYZ, multiple minima and minimizer continua remain valid inputs to discovery even when the existing seeded projector refuses rank.

Hierarchy admission is transactional: invalid patch or aggregate storage/depth/size cap rejects the whole hierarchy; no missing-domain hierarchy is published. Depth zero is valid and means one root/leaf for each admitted patch. The retained patch snapshot and its exact revision identity determine the hierarchy. Packed slots and node offsets are storage indices; source lineage stays queryable after public source handles close, through retained dependencies.

Queries bind this hierarchy and one patch slot, retain original local targets, and produce one best valid binary64 UV witness, approximate XYZ/descriptive distance, certified [lower2,upper2], work counters, and independently meaningful outcome flags. Projector status is refinement information, not bracket authority. A valid failed/rank-unresolved refinement can still supply a witness UV after rigorous original-patch enclosure evaluation. A nonempty valid closed patch always has some feasible UV; initial witness selection must not require solver convergence.

DFS excludes a closed cell only when its conservative lower2 is strictly greater than the current certified upper2. Equality is retained. A visit cap stops traversal and returns a conservative root lower fallback; no dropped frontier can be hidden behind a partially reduced lower. A refinement-call cap prevents more local solves but does not stop bounding/traversal. Bounds remain valid under either cap.

The parent-selected success rule is deliberately separate from complete traversal: main `GAP_SATISFIED` means only that upward-computed upper2-lower2 meets the squared-distance tolerance. `VISIT_LIMIT` may coexist with this status when the root fallback already gives sufficient precision. `COVERAGE_COMPLETE` means traversal accounted for the whole named patch without visit-limit abandonment; it is absent in the capped-but-tight case. No global UV isolation, exact closest point, all stationary roots, or minimizer enumeration follows. Complete coverage includes safely pruned regions; it does not mean every node was visited. Exact runtime spellings and status precedence remain subject to canonical design/header handoff.

If 0 <= L <= d*² <= D(witness) <= U and upward(U-L) <= delta2, then D(witness)-d*² <= delta2. This is squared-objective accuracy. It does not establish the same numeric tolerance in Euclidean distance, world distance or UV. It remains meaningful for continuum minima. A reversed bracket or nonfinite endpoint cannot pass success by clamping its gap. Invalid rows must not publish valid-looking zero brackets or valid-witness flags.

## Independent exact restriction oracle

Use Fraction.from_float on original controls and downloaded endpoints. Do not repeat the production de Casteljau triangle with Fractions. Derive each node independently from its exact dyadic cell by a Bernstein-to-power, affine restriction, power-to-Bernstein transform.

For one homogeneous channel, write H(s,t)=sum c_ab s^a t^b. Obtain c_ab directly from input Bernstein controls H_ij:

    c_ab = sum_{i<=a,j<=b} H_ij
           binom(p,i) binom(p-i,a-i) (-1)^(a-i)
           binom(q,j) binom(q-j,b-j) (-1)^(b-j).

A cell identifies s in [alpha,alpha+delta], t in [beta,beta+epsilon], where alpha=u_index/2^u_level, delta=1/2^u_level and analogously for v. Substitute s=alpha+delta*x, t=beta+epsilon*y. Its power coefficients are

    e_rk = delta^r epsilon^k
           sum_{a>=r,b>=k} c_ab binom(a,r) alpha^(a-r)
                                     binom(b,k) beta^(b-k).

Convert these powers to a degree-(p,q) Bernstein net:

    K_ij = sum_{r<=i,k<=j} e_rk
           binom(i,r)/binom(p,r) * binom(j,k)/binom(q,k).

All operations are exact Fractions; the formulas include degree-zero axes. Repeat for A_x,A_y,A_z,W. Positive W and each Cartesian ratio K_A/K_W are exact. Independent basis substitution is a direct specification oracle for the source cell, not a second traversal solver or a sampling approximation. It deliberately takes each cell from the original input rather than inheriting accumulated numerical children.

For every downloaded node require each reported lower coordinate <= minimum exact restricted control ratio and each upper coordinate >= maximum ratio. This attacks the stronger specified control-hull enclosure directly, and implies surface enclosure by positive rational Bernstein weights. It can catch errors far smaller than any visible position tolerance, including exact 2^-1075 midpoint coordinates. It does not demand minimally tight boxes. Require finite ordered box endpoints, known [-1,1] enclosure invariants, and child nesting if the reviewed algorithm promises parent intersection.

The proposed all-node download is an 80-byte POD: six FP64 lower/upper endpoints, then eight u32 fields left/right/patch_slot/u_index/v_index/u_level/v_level/flags; roots are separate u32 entries indexed by patch. Decode all fields exactly. The canonical leaf sentinel/flags and binary split-axis rule must be supplied by the runtime header, not guessed by the oracle.

Check roots identify [0,1]^2 for each original patch. Child indices are in range, acyclic and rooted; every node is reachable exactly once from its root and belongs to the correct patch. Every internal node has both children; children split the parent at its exact dyadic midpoint along the declared axis, preserving the other coordinate and closed boundary. Rooted leaf paths form a prefix-free complete cut. Reject duplicate cell IDs where the canonical fixed tree does not permit them. Area totals alone are insufficient because duplicated area can conceal gaps. Verify declared depth/count formulas for the complete binary build: at total binary depth d, each varying root has 2^d leaves and 2^(d+1)-1 nodes; the reviewed degree-(0,0) special case remains one root/leaf at every requested depth. A degree-zero axis splits only the varying axis. Admission rejects aggregate multiplication/index overflow before output publication.

## Independent bracket/witness oracle without a DFS reference

At the returned binary64 UV, evaluate the original homogeneous polynomial by the exact expansion above (or the existing independent exact Bernstein value oracle); divide exact A by W. Calculate exact D=sum(S_k-X_k)^2. Require feasible closed UV, exact retained patch identity/target correspondence, upper2 >= D, and approximate point/descriptive distance within their reviewed finite numerical allowances. An approximate XYZ is never substituted for S in the upper check.

For each downloaded AABB, compute its exact squared distance to the binary64 target using Fractions: sum max(lower_k-X_k,0,X_k-upper_k)^2. This treats the emitted outward box as the actual enclosing object and checks rounding of query-side lower computation independently from hierarchy construction.

When traversal completes under the AABB-only first design, a flat exact reduction over all leaves supplies B_leaf=min exact distance(X,leaf_box)^2. Require 0 <= returned lower2 <= B_leaf. This does not replay DFS, incumbent scheduling, pruning or local solves. Sound strict pruning against a valid upper cannot hide a leaf whose exact box lower would be below that upper; pruned ancestor and nested descendants retain the needed implication. If the reviewed algorithm adds a genuinely stronger mathematical lower criterion later, replace this AABB-specific comparison with its independent certificate oracle rather than falsely rejecting a stronger result.

When `VISIT_LIMIT` is set and the reviewed fallback is the root bound, require lower2 <= exact distance(X,root_box)^2. If the design specifies its precise downward arithmetic, targeted exact-representable cases can require the exact resulting value; generic rows assert the necessary inequality, not an invented tightness policy. The hierarchy's full immutable coverage plus the conservative root fallback protects the interval without exporting a frontier.

For analytic fixtures additionally require L <= known d*² <= U and D-d*² <= delta2 whenever GAP_SATISFIED. For arbitrary bounded homogeneous fixtures, the exact box lower, exact witness upper and complete hierarchy topology already attack the interval claim without solving the global minimization problem. Check success from exact rational endpoints: U-L <= requested delta2 is necessary; the production upward gap predicate must be checked in equality/nextafter boundary cases. No epsilon may excuse a false lower/upper inequality.

Strict-pruning observability matters: with one witness and no traversal trace, a >= tie-pruning mutant may leave identical brackets and answers. Require a deterministic equality fixture with specified prune/leaf/refinement counters, or a direct native qualification of the real predicate on valid equal lower/upper operands. Distance truth alone cannot distinguish that fault. Do not introduce public candidate enumeration merely for this test.

Counters have separate jobs: visits bound traversal work; refinement_calls bound invoked local solves; evaluator work counts actual attempts if exported. Refinement failed/rank-limited attempts still consume their cap. No more-than-cap work is permitted before a flag appears. The exact iteration/update counters of the internal projector are not automatically discovery counters. A deterministic algorithm can provide small exact-counter witnesses after the design fixes ordering; do not invent arbitrary order expectations in advance.

## Proposed direct corpus

Build admission and hierarchy checks:

- All 16 degree pairs, three original-input patterns (constant, asymmetric bounded polynomial/unit weights, nonuniform rational weights including 1 and256), at depths0,1,2,5. With the degree-(0,0) single-leaf exception, this yields 3,342 downloaded nodes for direct ratio checks. Select eight especially useful models for default depth8 (4,088 additional nodes for eight varying models), including bicubic mixed-sign numerators, weight asymmetry, one degree-zero axis and subnormal controls. Add one simple degree-(1,0) cache at the admitted maximum depth12 for exact large-tree identity/extent qualification without duplicating the costly bicubic matrix. Cap corpus by arithmetic fault coverage rather than redundant depth sweeps.
- Degree(0,0), repeated controls, rank-one/folded images and weight256 must build. Invalid degree, nonfinite/invalid homogeneous controls, missing record tails, out-of-envelope values and cap overflow must reject the entire build with no published partial root set. Empty patch-set hierarchy behavior must be explicitly chosen; invalid named query slot remains invalid, never a zero-distance answer.
- Subnormal homogeneous numerator line H0=(0,0,0,1), H1=(2^-1074,0,0,1): exact first midpoint 2^-1075 and repeated smaller controls directly checked. Negative counterpart, opposite signs, signed zeros and normal/subnormal transition attack FTZ/DAZ and rounding direction. These are enclosure cases even when d² underflows.
- Weighted line H0=(0,0,0,1), H1=(256,0,0,256) has midpoint x=256/257. Cartesian splitting, forgotten W and ratio corner errors are exposed. Constant H=(1,0,0,3) yields exact x=1/3; target X=round(1/3) has positive exact d², despite rounded XYZ distance zero. Require a positive-enough upper, never an exact zero certificate.

Query geometry and meaningful outcomes:

| Model/target | Independent expected meaning |
| --- | --- |
| Orthoplane S=(s,t,0), X=(a,b,c) | d*²=gap(a,[0,1])²+gap(b,[0,1])²+c². Interior, all edges/corners, exact on-surface zero and nextafter boundaries. Nearest UV on a hierarchy split boundary must remain valid. |
| Tilted plane S=(s,t,(s+2t)/4), X=(-1/4,1/2,1/4) | Unique constrained UV=(0,1/2), d*²=1/16; free-UV clipping gives a different answer. |
| Quarter-cylinder S=((1-s²)/(1+s²),2s/(1+s²),t), X=(3/4,1,1/4) | Unique UV=(1/2,1/4), d*²=1/16. The exact controls already support independent rational truth. |
| Same cylinder X=(-3/4,-1,1/4) | Global endpoint UV=(0,1/4), d*²=65/16; other endpoint is local minimum with d²=73/16; interior stationary saddle has d²=81/16. Query discovery must escape merely trusting one seeded answer. |
| Cylinder X=(-1,-1,1/2) | Two equal endpoint minima, d*²=5. Either representative is legal; strict pruning is independently needed even though the API does not enumerate ties. |
| Cylinder X=(0,0,1/2) | d*²=1, continuum [0,1]×{1/2}; no unique UV check. Complete fixed-depth traversal can remain wide under a tight squared-gap request. |
| Parabola S=(a,t,a²), a=2s-1, X=(0,1/2,3/4) | d²=1/2+(a²-1/4)²+(t-1/2)². Global UVs(1/4,1/2),(3/4,1/2), d*²=1/2; center saddle d²=9/16. |
| Same parabola X=(0,1/2,1/2) | d²=1/4+a⁴+(t-1/2)²: unique quartic-flat minimum; rank/curvature heuristics cannot exclude it. |
| Parabola normal family | a0=±1/2,±1/4, lambda=1/16; X=(a0-2a0lambda,t0,a0²+lambda). Unique UV=((a0+1)/2,t0), d*²=lambda²(4a0²+1), useful nonlinear throughput targets. |
| Fold S=((2s-1)²,t,0), X=(1/4,1/2,c) | Two UVs map to the same nearest XYZ, d*²=c². At X_x=0 the nearest point is rank deficient. Both remain supported bound inputs. |
| Rank-one S=(s,0,0) and constant patches | Exact line/point distance, nonunique unused parameter; discovery obtains upper witnesses without requiring rank-valid projector success. |
| Linear S=(2s-1,t,0), X=(0,1/2,0) | Both endpoint controls outside a radius1/2 sphere while carrier reaches its center; nearest-control distance is not a valid lower bound. |

Outcome/flag attacks must include all four combinations:

1. Complete traversal and tight bracket: GAP_SATISFIED plus COVERAGE_COMPLETE, no VISIT_LIMIT. Use analytically simple plane/constant cases with tolerance above unavoidable enclosure slack.
2. Complete traversal but wide bracket: COVERAGE_COMPLETE, no GAP_SATISFIED, no VISIT_LIMIT. Use depth0 or shallow curved carrier with a sufficiently strict positive tolerance. Expect the reviewed unresolved status/reason; no invented uniqueness failure.
3. Visit cap and tight fallback bracket: GAP_SATISFIED plus VISIT_LIMIT, no COVERAGE_COMPLETE. A constant or simple plane with a generous squared tolerance is a deterministic attack. Do not require mathematically zero width, because outward evaluation can add slack.
4. Visit cap and wide fallback bracket: VISIT_LIMIT, no COVERAGE_COMPLETE and no GAP_SATISFIED. Low cap plus a curved patch/tight tolerance preserves valid root bounds.

Refinement cap0 must still produce an initial certified witness and perform bounding. An exhausted refinement cap must not create VISIT_LIMIT or stop domain accounting. With complete traversal it may still satisfy the gap. Test cap exhaustion through valid rank-refused refinement attempts as well as successful solves. Options at zero, exact limit and maximum must have canonical validity/precedence defined by the design. Test squared-gap tolerance equality and predecessor/successor when exact box/witness quantities permit it; use the exact Fraction predicate for nonrepresentable values.

Repeated rows, mixed valid/invalid patch slots/targets, N=0,1,32,257 and4096, same-size foreign hierarchies, independent options/results and slot permutations exercise full-N stride and source binding. Invalid-slot precedence must be consistent with the reviewed public boundary. CPU and both explicitly selected CUDA devices run the same copied binary inputs; requested unavailable backend fails qualification, with only actual CUDA-current-device behavior skipped on an explicit CPU run.

## Arithmetic environment and runtime qualification

The reviewed arithmetic unit must choose required rounding/gradual-underflow environment and rejection or save/restore. Before publication test CPU rounding-mode/FTZ/DAZ attacks using real supported host controls on the execution target, restoring the test's original environment in finally. Require either the documented rejection with no token/payload publication or valid outward bounds and exact restoration; never silently accept corrupted boxes. Test build and query-side bounds, plus CPU worker threads if environment assumptions are thread-local. CUDA subnormal controls and direct exact enclosure rows attack device behavior; compile flags and intrinsics still require numerical review rather than inferring all-operation correctness from this corpus.

Caller/resource obligations extend the existing ABI: raw descriptor/version/extent/alignment rejection using valid allocated buffers; no forged pointers; exact output extents; all output/diagnostic alias relations; stale/wrong-kind/foreign tokens; immutable source/option/input snapshots; retained dependencies after closing public parents; context closure invalidates dependents; concurrent calls retain source lifetime; backend errors publish no partial result. Python native-success constructor/allocation/BaseException faults must close the returned token while preserving the original exception. Both new CUDA creation/query/download/provenance/close callbacks must restore the calling thread's previous device, including a host worker. Reuse existing direct mechanisms for these fault classes, extend them to the new family rather than adding parallel resource frameworks.

Hierarchy download shape must expose each canonical u32 cell field and six doubles exactly, with per-patch roots. Result download must expose original request source/targets/options and certificate/flags/counters separately from descriptive XYZ. These are the necessary handoff points for a direct caller; no backend-private pointers or mutable borrowed buffers cross the boundary.

## Actual caller, visualization and benchmark value

Root owns a real seed-free point-cloud caller and benchmark. Use the exact cylinder/parabolic normal families above plus boundary, ambiguous/degenerate and intentionally capped rows. Caller consumes result UV/witness and [L,U]/flags to decide whether a distance bracket is sufficiently precise. Export original target, returned UV/approximate point, certificate endpoints, source lineage and work/outcome fields. Any displayed candidates are the one returned witness per request, not an undisclosed all-minimizer list.

A visualization may draw selected downloaded hierarchy boxes, target-to-witness segments and labels with squared bounds/gap/coverage/visit-limit state. Label boxes as cached enclosures. Without traversal trace data, it cannot label them actual visited/pruned/frontier nodes. An unresolved bracket is shown visibly rather than removed from the plot. This material task demonstrates automatic seeding and bounded geometric uncertainty without claiming trimmed CAD faces.

Measure complete CPU preprocessing/download/upload and cache memory separately from repeated resident queries; target ingestion, traversal+refinement+certificate evaluation, synchronization, compact download, host/native complete call, Python caller and export scopes stay explicit. CPU1/24-thread and each P100 use identical named-patch inputs and options. Include visit/refinement histograms and gap/coverage/limit populations; several CPU samples show dispersion. Both easy exclusions and difficult surviving/refining cases are needed. Fast rejection of invalid rows is not discovery throughput. No additive inference from independently timed medians or borrowed tutorial speedups.

## Review gates and limitations

Before product code, the design/plan must fix build-depth/cap admission, initial UV policy, node topology/sentinels/flags, traversal ordering, visit/refinement counter definitions, fallback lower and all outcome precedence, squared tolerance domain, arithmetic environment and output/source download layout. This proposal intentionally does not invent a second ABI.

The direct exact mechanisms qualify distinct claims: restriction/control-hull enclosure; complete immutable topology; lower arithmetic and original-UV upper arithmetic; analytic global answers; gap/coverage/cap separation; and independent lifetime/device failures. They do not implement a reference iterative nearest solver, and no sampling or CPU parity substitutes for enclosure. Finite exact conformance is strong evidence for its targeted fault classes, not a theorem over every binary64 execution. Numerical containment rules must carry their own reviewed argument.

Primary source grounding and acquisitions remain in [bounds bibliography](bounds-oracles.bib), [bounds PDF catalog](bounds-oracle-pdfs.tsv), [source map](bounds-oracle-sources.tsv) and the existing numerical/runtime studies. No new library, installation, source adoption, reference execution, product/test source or Git mutation was performed for this proposal.
