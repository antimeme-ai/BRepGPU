# Iteration3 implementation plan: independent oracle review

2026-10-14. Reviewed [implementation plan](../docs/ITERATION_3_PLAN.md) against the accepted [design](../docs/ITERATION_3_DESIGN.md), [oracle design input](2026-10-14-discovery-oracle-design.md), [three-mode math review](2026-10-14-discovery-design-oracle-review.md) and [strong-bound experiment](2026-10-14-strong-bounds-oracle-study.md).

**Disposition: accepted in this lane, no blocker.** The plan delivers one seed-free named-patch operation, all three owned bound modes, direct exported-cache and returned-UV qualification, actual bounded result semantics, and a useful caller/full-operation comparison. It preserves research/design/plan/direct-red ordering and fleet-only native execution. No product/test implementation occurred during this review.

## Why the direct mechanism is adequate

The oracle unit owns fixtures, public conformance and standalone qualification together. Original binary64 homogeneous inputs become exact Fractions. Independent Bernstein-to-power expansion, exact dyadic affine restriction and conversion back to Bernstein compute each source cell without reproducing the production de Casteljau splitter. Exact power-polynomial convolution and conversion compute C0..C4 expectations without reproducing native Bernstein-product loop order. The alternative binomial identity remains a mathematical derivation, not an extra test layer for the implemented oracle.

For every downloaded node, directly check exact integer cell/root/child topology and complete source coverage. AABB endpoints enclose exact restricted Cartesian control ratios; DOP supports enclose their exact integer-direction projections; all active Bernstein C0..C4 intervals enclose exact coefficients, active C4 remains positive and inactive padding is0. Actual CUDA arrays are downloaded, so qualification checks both construction and resident storage. One faulty cached ratio/sign/stride cannot hide behind CPU parity.

The returned binary64 UV names an exact point on the original carrier; calculate its exact squared distance to the retained target and require U>=D with no allowance. Flat method-specific leaf lower reductions qualify complete traversal; exact root lower qualifies visit-cap fallback. Analytic models independently check true global L<=d*²<=U and squared-objective precision. Empirical XYZ allowances remain separate from these certificate inequalities. There is no reference iterative solver or sampled enclosure oracle.

The plan fixes the earlier small design details: root split U-first/alternating, left-first lower ties, preorder topology, coefficient packing, stack13 invariant failure, strict prune and alias/no-write semantics. Degree-(0,0) always has one root/leaf; one degree-zero axis splits only the varying axis. The oracle must not apply a generic full-tree count to constant-degree carriers.

## Direct red and conformance targets

The red cases call the absent/new public capability rather than fabricate a failure. Runtime supplies canonical header/Python classes and status names first; root can invoke genuine old-core missing-capability red on the qualified target. Local editing or import failure is not native CPU/P100 evidence. Requested missing backend must fail standalone qualification rather than skip.

Whole cache corpus includes all16 degree pairs, asymmetric/nonuniform weights1..256, default-depth repeated restriction, simple maximum-depth extent, opposite-sign and subnormal numerators. Specific fault witnesses remain:

- Constant1/3 with target round(1/3): ordinary XYZ distance0 is a false upper certificate.
- Subnormal linear midpoint2^-1075: rounded child-net replacement loses exact enclosure even when distance squares underflow.
- DOP norm2 diagonal plane d²1/8 and norm3 plane d²3/16 versus root AABB0; singleton target(3,4,0) exposes weak max-slab replacement of AABB sum.
- Negative C0 line Ax=s-1/2,W1,target1/4: coefficients C0=(1/4,-1/4,1/4), N=(9/16,-3/16,1/16), true minimum0.
- Rational quarter-cylinder axial target: continuum minimum1, scalar t coefficient ratios(5/4,3/4,5/4) before split and minimum1 on both t halves; depths0/2/4/8 expose actual precision benefit and cost.

Boundary-local competition, symmetric ties, parabola twin minima/saddle/quartic-flat, folds and rank-refused projection all remain valid discovery cases. Root-leaf duplicate solve is skipped; zero refinement budget still yields an initial witness and bounds. Child-pair visit admission and exact-cap cases must preserve root fallback when incomplete. Four gap/coverage/visit combinations and independent refinement-limit flags distinguish squared precision from traversal work.

Strict > pruning now has an actual mechanism: a real CUDA constant-point row with exact zero lower/upper exposes incorrect equality rejection through missing retained-terminal state; direct native qualification exercises the same real predicate where CPU outward slack prevents equality. A single displayed witness/bracket alone is not treated as adequate mutation sensitivity. Mutants execute only on the fleet.

## Runtime and actual caller coherence

The plan extends the same registry, fault seam, allocation/publication cleanup and Python BaseException guards; no parallel runtime framework. Valid allocated raw buffers attack extents/alias/no-write, source identities, close-retention and wrong kinds. Actual host arithmetic environment tests restore rounding/FTZ/DAZ/masks and require documented refusal before publication. Both P100 callback families restore calling-thread device; actual worker guards cover thread-local state. Sanitizers and compatibility probes address resource/ABI fault classes separately from geometry.

Root's native benchmark and seed-free caller consume the same certified result fields and source/option snapshots. The chosen analytic global values can be exact dyadic quantities; construct admitted target/control inputs exactly and check representability before claiming epsilon-free native truth. Exact Fraction cache/witness qualification complements that timed-row analytic truth and does not duplicate it.

All three modes receive identical bits, depth, budgets and squared tolerance. Native scopes include actual resident output allocation/traversal/refinement/synchronization; setup/upload, ingestion, download, host consumption/cleanup and Python/export are separately named. Seven repeated samples, counter distributions, gap/coverage/unresolved populations and preprocessing/cache amortization support selection. Different witness order/culling is legal; differing delivered precision cannot be hidden by only comparing latency.

SVGs draw actual cached enclosures and returned witnesses, preserve visible unresolved rows, and avoid inventing traversal traces or minimizer enumeration. This completes the user's material caller/graphics request while keeping scope at untrimmed local carriers. Default versus opt-in bound mode remains a measured whole-operation choice.

## Implementation handoff

After parent marks this plan accepted, oracle owner can implement only discovery_fixtures.py, qualify_discovery.py and test_discovery.py plus journal, coordinated with canonical runtime/Python handoff. Root owns native benchmark/caller/SVG and all deployment/Git operations. Direct red precedes numerical implementation; completed artifacts, qualification and source review are returned as the whole oracle/caller conformance unit. This review itself does not authorize code before that disposition.
