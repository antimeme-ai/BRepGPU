# Discovery exact-oracle and native benchmark source review

Date: 2026-10-14. Scope: the complete discovery qualification oracle and native
benchmark truth/consumption unit, independently of the numerical implementation.
This is source review; no native jobs, mutations or timing measurements were run
for this review. Target qualification belongs to the integration lane.

## Disposition

No numerical defect found in the Python exact original-input oracle, qualification
runner or public discovery corpus. Native benchmark analytic global distances are
correct and exactly representable for the generated inputs. Two native consumption
gaps were reported to the benchmark owner: missing numerical qualification of GAP
counts, and accepting a plausible global bracket paired with an unrelated feasible
UV/XYZ witness. The latter is narrower than the existing plan's explicit split:
native timed rows qualify global brackets, while the independent Fraction corpus
qualifies original-UV witness certificates. Neither issue establishes a defect in
the product solver. Both findings are resolved by the reviewed benchmark change
and explicit qualification-scope disposition below; no unresolved review blocker.

## Reviewed source checkpoint

| Source | SHA-256 |
|---|---|
| `python/brepgpu/discovery_fixtures.py` | `92a8401c42aef92086cf0303c41dc961ad918987857749081c4a856aed5a346c` |
| `scripts/qualify_discovery.py` | `85f03507ae070b8ffaf4e591730b95f54be2034138cd0cc66693786306036245` |
| `tests/test_discovery.py` | `13e16316944ba6d073162bcd8806f0fbbf88ecd38b8ecbb50253bf8d9e62fb40` |
| `benchmarks/discovery/fixtures.hpp` | `a18055c2227962c3543fa9250b5749e58cc47b9a02463e81b4991b90e8346e81` |
| `benchmarks/discovery/benchmark.cpp` | `53b0cccca12cddcb032e21ce18b94147dcb5f747ef63eada9bdc39e32bad8f63` |

## Exact oracle independence and correctness

`source_power` expands each supplied binary64 homogeneous control channel into
exact Fraction power coefficients. Its factors are the expansion of
`choose(p,i) s^i (1-s)^(p-i)` and the independent v basis. `_restrict` substitutes
the exact dyadic affine cell map into powers using the binomial theorem. `_bernstein`
converts powers through `choose(i,r)/choose(p,r)` and its tensor analogue. This is
different machinery from native repeated interval de Casteljau subdivision.

`base_coefficients` multiplies restricted power polynomials by ordinary exact
coefficient convolution, then converts degree-(2p,2q) results back to Bernstein.
It does not repeat the native ordered-pair Bernstein product coefficient routine
or its floating binomial quotient. The five objects are correctly C0=sum A_k²,
C1..3=A_k W, C4=W². Negative C0 coefficients remain negative, degree-dependent
active extents are separate from 49-slot padding, and active C4 enclosures must
stay inside the positive [1,65536] envelope. This attacks clipping, stride, degree
and product-factor mistakes directly.

`qualify_hierarchy` checks actual downloaded complete-tree cardinality, canonical
preorder, every node's dyadic identity and source slot, leaf sentinels, reachability,
Cartesian restricted-control hull containment, parent/child nesting, DOP support
containment and every active/padded Bernstein cache coefficient. Wrong extents add
violations rather than silently accepting an empty cache. Malformed values that
cannot become a Fraction raise and cause the runner to fail. An absent leaf set
returns no oracle. Empty forests remain a legitimate structural case.

`HierarchyOracle.node_lower` uses exact point-to-box distances, exact integer-normal
slab distances divided by the correct normal squared norm, or exact Bernstein
objective/positive-denominator coefficient ratios. `lower_limit` uses the original
root if the row is visit-limited, otherwise the minimum over *all* exact leaves.
It does not reproduce DFS, child ordering, pruning or refinement. This is a valid
independent ceiling on the native returned lower bound: native outward bounds are
no stronger than the exact corresponding bound; a pruned leaf has native lower>U
and therefore exact lower>U, and cannot invalidate the complete traversal's
`min(U, retained-leaf minimum)` output.

`qualify_row` evaluates the supplied original homogeneous patch exactly at the
actual returned binary64 UV. Its upper check is `U>=exact witness distance`, not a
distance formed from the descriptive rounded XYZ. It independently checks L<=U,
the flat/root lower ceiling, available analytic global distances and exact
`U-L<=tolerance_squared` for GAP. Status/flag consistency excludes arithmetic and
unknown bits, requires candidate validity, and requires exactly one of coverage
complete or visit limit for ordinary valid terminal rows. Work caps include a
lower as well as upper evaluator count; specified accounting cases check exact
counts. Approximate XYZ and distance alone receive explicit descriptive-error
allowances. These allowances do not leak into certificate inequalities.

The runner shuffles mixed degree/source cases, preserves target and source-lineage
snapshots, writes actual binary64 values in hex, writes exact witness/lower ratios,
and returns nonzero for any retained violation or exception. Public tests additionally
attack depths through the maximum, empty/single/stride-tail batches, caps independent
of gap success, named-patch invalid rows, resource lifetime, raw overlap rejection,
constructor/interrupt cleanup and per-thread CUDA device restoration. These are
direct product claims; they do not substitute CPU/GPU parity for geometry truth.

## Native analytic truth

All generated control, target and global-distance operands are dyadic with small
integer numerators. Generated UV sampling coordinates are multiples of 1/16;
radial lambda is a multiple of 1/8; parabolic lambda is 1/16. The highest-degree
distance expressions remain well within 53 significant bits, including the
squared rational-cylinder radial gap. Thus the stored binary64 global distance
is the exact answer for the supplied binary64 inputs, not an approximate ideal
model answer.

* Plane S=(s,t,0): admitted x/y targets are inside the box and z=±1/2, so d*²=1/4.
* Diagonal S=(s,s,t): target (a+1/4,a-1/4,b) projects to (a,a,b), so d*²=1/8.
* Three-axis plane S=(s-1/2,t-1/2,1-s-t): image lies in x+y+z=0 and contains the
  origin; target (1/4,1/4,1/4) has d*²=3/16.
* Rational cylinder: homogeneous channels are A=(1-u²,2u,v(1+u²)), W=1+u².
  Its xy image is the unit quarter circle. Target xy=lambda(1-a²,2a) has exact
  radius lambda(1+a²), admitted angle, and d*²=(lambda(1+a²)-1)². Axis targets have
  d*²=1 and admitted height. The capped case changes traversal budget, not truth.
* Parabola S=(a,v,a²), a=2s-1: normal target (b-2bλ,t,b²+λ), λ=1/16, is closest
  at a=b. The squared-distance difference from that point is
  `(a-b)²((a+b)²+1-2λ)`, positive for a!=b. Its d*²=λ²(4b²+1).
  Twin target (0,t,3/4) has d*²=1/2 at a=±1/2.

## Findings for the benchmark owner

### R3-OR1 — GAP counts accept a bracket wider than tolerance

Severity: material benchmark precision-reporting gap. `consume` checks agreement
between status GAP_SATISFIED and the GAP flag, but never qualifies that flag against
the returned squared bracket width. A row with a valid wide bracket, feasible UV,
finite fields, bounded counters and status/flag GAP passes and contributes to
`gap_rows`. The Python oracle correctly rejects this row class.

Fix: check the numerical gap against the requested squared tolerance in the native
consumer. An independently outward-rounded subtraction is a practical conservative
check under the qualified native FP environment; do not silently use an arbitrary
epsilon or merely inspect the status. This belongs before publication of precision
counts. No standalone oracle-for-oracle test is requested; target product fault
qualification remains owned by integration.

### R3-OR2 — global brackets do not validate the reported witness

Severity: bounded benchmark-consumption gap; explicit qualification-scope limit.
`consume` only requires UV in [0,1], finite XYZ/descriptive distance, and
L<=known-global-distance<=U. An unrelated feasible UV with arbitrary finite XYZ
and approximate distance can pass with an attractive analytic bracket. This does
not falsify the analytic global bracket test, but it means the benchmark cannot
claim that every consumed timed witness received independent geometric validation.

Practical fix: independently evaluate the fixture's analytic map at returned UV
and check descriptive XYZ/distance with stated rounding allowances. A truly exact
arbitrary-binary64 rational upper check needs exact arithmetic or a defensible
enclosure; ordinary double/long-double agreement alone must not be described as
that certificate. Keep the existing Fraction original-UV certificate qualification
as the exact mechanism, and state precisely what native timed consumption checks.
If a native interval comparison is used, distinguish a lower-distance contradiction
check from proof that U contains the true witness distance. No dependency adoption
or second implementation of the full native solver is necessary.

## Timing and work interpretation

`resident_discover` includes public result allocation, traversal/refinement and
synchronization, excluding download/consumption/close. `download_prepared` measures
download into allocated output arrays. `cached_hierarchy_host_to_host` includes
query snapshot/upload, discovery, download and full `consume`; its Stats histogram
map allocations and checks are therefore included. Result/query cleanup is
separate. Setup includes patch snapshot and hierarchy build/upload; retained
hierarchy reuse is explicit. Profiled kernel runs use a distinct profile-enabled
context and warm-up, and actual info must carry a finite profiled kernel value.

`mean_interval_witnesses=1+mean_refinements` matches the reviewed algorithm for valid
rows: root interval witness plus one fresh original-UV interval witness per returned
projector candidate. Visits count actual node-bound calculations, including both
ordered children once; pruned/refinement/evaluator counts describe actual work,
not successful minima. The native consumer's current cap checks are weaker than
the Python qualification's lower evaluator-count/refinement-limit assertions; this
is a reporting limitation rather than evidence of wrong product counters. Raw
work histograms and exact direct accounting cases provide the relevant independent
mechanisms.

## Fix re-review and final dispositions

Reviewed `benchmarks/discovery/benchmark.cpp` SHA-256
`708f728f48563f2d5f866469b939f1c7711ff8467b4d13170413c5973244efd7`
and `benchmarks/discovery/fixtures.hpp` SHA-256
`8608fe42dca542dc00f42ae42fa5595efed938dd05bc9b6a9454baba77a7e002`.
This is source re-review, not a new execution or qualification claim.

**R3-OR1 resolved.** `exact_gap_within` implements Knuth two-sum with operands
U and -L. Under the admitted RN/gradual-underflow environment and strict benchmark
compile options (`-fno-fast-math -ffp-contract=off`), `sum+error` is the exact
binary-input difference. After the existing finite/bracket checks,
0<=L<=known-global-distance<=U ensures subtraction and two-sum intermediates cannot
overflow. Gradual underflow preserves the low part; cancellation into the subnormal
range is exactly representable for these binary64 operands.

For representable tolerance T, RN `sum<T` implies exact U-L<T, and `sum>T` implies
exact U-L>T: a distinct representable T lies beyond the RN midpoint interval for
sum. If `sum==T`, the residual sign decides whether exact U-L<=T. The code's
`sum<T || (sum==T && error<=0)` therefore handles exact equality and a rounded-to-T
difference just above T correctly. It does not add an epsilon or blindly expand
an already exact equality. A GAP row now passes only after this independent exact
squared-width predicate; unresolved rows need not be labeled GAP merely because
their exact width happens to satisfy tolerance.

**R3-OR2 resolved with the stated scope and additional corruption checks.**
`Fixture::point` uses expanded analytic maps for each delivered corpus, independent
of native Bernstein and CUDA tensor evaluation. Cylinder uses
((1-s²)/(1+s²),2s/(1+s²),t), parabola uses (2s-1,t,(2s-1)²), and the plane maps
match the original supplied controls. The denominator is bounded away from zero
over admitted returned UV. The consumer now checks returned XYZ against that
map at the returned binary64 UV, checks descriptive distance using the propagated
2^-40 position allowance plus explicit arithmetic allowance, and rejects a
candidate distance that exceeds U by more than
`512*long_double_epsilon*max(1,candidate_distance)`.

The comment correctly says this last comparison is a contradiction/corruption
check, not an exact original-UV upper certificate. The long-double arithmetic
margin may admit a small upper error; it must remain excluded from claims of exact
certificate qualification. Exact native timed global bracket containment and exact
binary tolerance checks remain unrelaxed. The standalone Fraction original-input
oracle and public caller rows remain the mechanism that exactly qualifies the
original-UV upper inequality. This matches the reviewed plan's deliberate division
of work without adopting a dependency or adding a mirrored solver.

The added analytic map and distance checks execute inside `consume`, so they are
included in `cached_hierarchy_host_to_host` and excluded from `resident_discover`
and prepared-download scopes. Source review found no new timing-scope ambiguity.

## Off-grid native fixture extension re-review

Reviewed `benchmarks/discovery/fixtures.hpp` SHA-256
`813ccb0063782a0886be2a55807a71bab3252e66284ff9991b5c8404a8276735`.
The additional named corpora `cylinder_offgrid` and `axis_offgrid` are correctly
recognized by both control generation and independent expanded point evaluation.
No product arithmetic changed. The existing common comparison corpora remain
separate from this precision-study extension.

For cylinder_offgrid, let m=2(i mod 32)+1, n=2(floor(i/32) mod 32)+1, k=8+(i mod 4).
Then s=m/64, t=n/64, lambda=k/8, and the generated binary64 inputs are exactly

```
Xx = k(4096-m²)/2^15
Xy = km/2^8
Xz = n/64
R  = k(4096+m²)/2^15
d*² = [k(4096+m²)-2^15]²/2^30.
```

Target/radius numerators use at most 17 bits, the radial-gap numerator at most 16
bits, and the squared numerator at most 32 bits. Intermediate generated arithmetic
is therefore exact in
binary64, and d*² remains an exactly representable minimum for the actual supplied
inputs. The angle lies inside the admitted quarter circle, height inside [0,1],
and maximum target y is less than 2.71, so target admission remains valid.
Axis_offgrid uses (0,0,n/64) and exact d*²=1. The updated analytic point method
evaluates the same original rational cylinder for both names.

At depth 8 with alternating u/v subdivision, both parameter levels are 4: leaf
boundaries are multiples of4/64 and centers are (4j+2)/64. Odd/64 coordinates are
neither, making these corpora an intentional off-grid check of the originally
aligned study. The `_offgrid` names accurately describe that relationship; no
literal midpoint claim is made. One 1024-row cycle contains 1024 distinct cylinder
angle/height pairs with lambda linked to the angle index. Axis targets have only
32 distinct heights because angle is immaterial. N4096 repeats four cycles; a
larger N is an amortized throughput study, not more independent geometry cases.

Integration reported the arbitrary-target 1024-row cylinder and polynomial clouds
were UNRESOLVED for Bernstein depth8, while coarse aligned native targets reported
GAP. This is consistent with the valid conservative fixed-depth contract and
does not establish a geometry defect: unresolved brackets still require exact
containment. It prevents general precision claims from the aligned corpus alone.
Off-grid target execution and gap/width/timing statistics remain integration work;
this source re-review makes no claim about their future observed outcomes.
