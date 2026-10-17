# Point-only fixed trigonometry: arithmetic and enclosure argument

This implements the accepted point filter in
`docs/native-mesher-point-trig-filter.md`. Source formulas remain shared with
the original exact enclosure path. Explicit jets/metrics are unchanged.

For profile P in {128,192,256}, W=P+32 fractional bits and one32-bit integer
word are held in bounded stack arrays. Signed-magnitude addition is exact;
overflow refuses the filter. Multiplication forms the full unsigned product
before discarding W low bits. Nonzero discarded bits increment positive upper
and negative lower magnitudes; the other endpoints truncate. This is exact
floor/ceiling for either sign, including underflow to a signed-magnitude zero.
All carries and high discarded words are checked. Interval products select
their extrema from exact endpoint signs: two products for a monotone sign
case, four candidates when both intervals cross zero. Square includes zero.
No FP rounding mode, FMA or subnormal assumption appears in this arithmetic.

`tests/native_mesher_point_trig_vectors.py tables` generates pi bounds using
Machin's identity and exact alternating arctangent remainders. It proves a
single512-bit floor cell containing pi. A prefix at R bits has the same exact
floor property. With B an upper bound on the argument's integer bits, use
R=W+B+8 only within both the512-bit constant and configured working cap.
Arguments outside |x|<=2^128 refuse this filter. The argument may have an
arbitrary rational denominator; the owned Integer division handles it directly.

Choose integer k by rounding2x against the lower pi prefix. Uniqueness of the
nearest quadrant is unnecessary. The exact identity is r=x-k*pi/2 for ANY
integer k. Signed exact integer cancellation over the common denominator
D*2^(R+1) and sign-correct pi endpoints enclose r, then directed division puts
it on the W-bit grid. Requiring the whole r interval inside[-1,1] establishes
the subsequent domain. Choose j by rounding32 times its midpoint and REQUIRE
the whole delta=r-j/32 interval inside[-1/32,1/32]. Both locator outputs are
checked; neither rounded locator is treated as a geometric fact.

For each analytic function, an exact Taylor parent on t=32*delta in[-1,1]
has coefficients a_n=(-1)^k/(32^n*n!). Its parity permits the Taylor polynomial
one degree higher with a zero coefficient, giving remainder
1/(32^(m+2)*(m+2)!) for parent degree m. The generator converts this polynomial
exactly to Chebyshev coefficients b_k. Removing terms above degree d incurs
at most sum(abs(b_k),k>d), because |T_k(t)|<=1. Adding the parent Taylor tail
gives a uniform bound; the selected degree meets2^(-P-16). The retained
polynomial is converted exactly back to powers of delta. This is certified
Chebyshev truncation, not an unproved Remez/minimax result. Taylor controls
with their own rigorous tails remain available for direct comparison.

Every retained coefficient is enclosed by floor/ceil on the W-bit grid.
Horner evaluation uses interval multiplication/addition and adds the explicit
uniform error rounded upward to that same grid. Anchor sin(j/32)/cos(j/32)
enclosures are generated independently by exact Taylor/Lagrange bounds before
grid rounding. Angle-addition formulas and exact quadrant swaps/signs then
enclose the original sin/cos. Intersecting with[-1,1] uses their mathematical
range. Guard bits provide useful widths; directed arithmetic supplies proof.

The original source coefficient/frame expression consumes those enclosures
through the same formula body, with its existing exact rational outward
compression. A point is accepted only when BOTH endpoints of EACH final XYZ
coordinate round to the identical nearest-even IEEE word. Difficult rounding,
out-of-profile arguments or excessive width try the fixed next profile and
then the original exact point path. Cancellation/resource errors propagate.
Filter success adds no pi cache or source-dependent global mutable state.

The independent vector oracle uses original-argument Taylor with repeated-angle
identities for large inputs; it does not use quadrant reduction, anchors or
either production polynomial. Tests cover arbitrary rational inputs, signs,
grid boundaries, very large angles, quadrant neighborhoods, subnormal-scale
arguments, signed discarded products, full-limb carry/overflow, all interval
sign combinations and actual correct-nearest source circle coordinates.

Reproduction on the qualified fleet checkout:

```
python3 tests/native_mesher_point_trig_vectors.py tables native/mesher/point_trig_tables.hpp
clang-format-18 -i --style=file:tooling/native-cpu.clang-format native/mesher/point_trig_tables.hpp
python3 tests/native_mesher_point_trig_vectors.py vectors context/point-trig-vectors.bin
```

The formatting exclusion encloses generated literal data only. Production
compiler/static-analysis diagnostics remain enabled. Cost evidence compares
complete source-point APIs, not just polynomial instructions, in
`benchmarks/native_mesher/point_filters.cpp`.

Independent parent review accepted the actual fixed integer arithmetic,
reduction, generated constants and remainder transformations, independent
original-argument oracle, shared formula integration and final IEEE-cell gate.
The reduction integer does not need nearest-quadrant status: the checked
residual/domain interval is the proof obligation. No numerical finding remains.
See journal/2026-10-16-native-mesher-point-trig.md for the final checks; measured
complete-point results are in results/native-point-filter-2026-10-16/.
