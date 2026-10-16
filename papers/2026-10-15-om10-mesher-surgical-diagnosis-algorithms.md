# OM10 mesher: algorithm diagnosis before further work

The operator requested diagnosis and a report before proceeding. This is a
read-only analysis of the unchanged production mesher at `ea9b37a`, the
[earlier cost notes](2026-10-15-om10-mesher-cost-study.md), and the
[connectivity reading](2026-10-15-om10-return-pass-connectivity-reading.md).
It authorizes no implementation, restart, new dependency or full rewrite.

## What has actually been observed

Root reports V4 at about 4 h 17 min, with source preparation/comparisons under
one minute and the last progress marker at face 2,600. An earlier bounded stack sample found all
26 valid samples in circle-schedule boundary all-pairs work. A later
three-second nonblocking sample yielded 9 valid samples and 20 reader errors:
7 valid samples were in `_triangulate`'s occurrence containment loop, and
2 were in `_surface_error`/`_outward`. The invalid samples limit interpretation.
These are observations of separate phases, not percentages of the complete
run, a proof that every later face behaves similarly, or a measured attribution
of arithmetic versus Python iteration cost. They establish useful targets;
they do not establish a render speedup.

A subsequent successful nonblocking numeric-locals read places execution in
the boundary simplicity scan for **face167661, definition1756**, zero-based
face index 2688. Its single outer loop has **8,300 sampled nodes, no holes,
but only four original coedges**. Two long original cubic spatial curves have
102 spans; its cubic carrier has 102 spans, with a narrow/long UV domain of
approximately `[0,.15] × [0,74.1416]`. Thus original coedge counts badly
understate the current sampled problem size. Its all-pairs scan can enumerate
34,440,850 unordered pairs, or 34,432,550 after cyclic adjacency exclusions,
unless an earlier intersection terminates it. These are derived work counts,
not counted calls or timings. Root's same read shows all 25,871 edges completed.

The mesher's saved `sampled-begin` clocks give **5,232.435 s (87 min 12.435 s)**
for its combined validation/edge-refinement/circle-coordination/canonical-edge
phase. This is a measured owned pre-face stage; it is distinct from the short
native source preparation and is not solely circle-scheduling time. The
preceding face167660 is a plane in the same definition with long cubic
boundaries; it is a plausible source of the earlier ear stacks, but those
samples did not identify their face. Stale local variables from a completed
face must not be attributed to the current face. Root also identifies 1756 as
the part encountered by the NVIDIA reference failure investigation; that
comparison must retain both backends' actual outcomes, without asserting a
shared cause from this coincidence.

## Algorithmic and arithmetic causes visible in the code

**Ear clipping.** `_triangulate` maintains linked previous/next occurrences,
but every convex candidate still visits `range(n)` for the ORIGINAL bridged
polygon size. Dead occurrences are skipped only after visiting their slots.
For a convex polygon with every candidate accepted, the inner loop alone has
quadratic slot visits. A sequence of rejected convex candidates can produce
O(n³) visits in the worst case: O(n) potential candidates per removal, O(n)
containment work per candidate, O(n) removals. The linked list makes removal
constant time; it does not remove candidate rescanning. The ear's min/max
coordinates are also recomputed inside the point loop. That repeats exact
comparisons even for rejected bounding boxes.

David Eberly's retained *Triangulation by Ear Clipping*, revised 2024-08-03,
section 2, explicitly distinguishes the straightforward cubic construction
from a quadratic method maintaining convex/reflex/ear lists and updating
neighbors after removal. It does not justify blindly applying a simple-polygon
optimization to our bridged occurrence polygon: coincident bridge coordinates
are distinct occurrences and the code excludes points equal to an ear corner.
That behavior, inclusive boundary containment, and collinear samples matter.

**Planar exact arithmetic is intentional.** `_plane_uv` calls
`_frame_coordinates(exact=True)`, yielding exact `Fraction` coordinates from
the saved spatial values and original frame. Thus planar bounding comparisons
perform rational comparisons, and `_orient` takes its unconditional exact
rational path. This is not merely an occasional floating-filter fallback.
Those exact values preserve collinearity in oblique original planes; replacing
them by rounded doubles would change the geometric contract. Actual numerator/
denominator bit lengths and their runtime share require the live numeric
inventory, rather than an assumption that arbitrary precision dominates.

**Boundary pair generation.** `_boundary_intersection` enumerates all segment
pairs except same-loop incident occurrences, allocating a tail slice at each
outer iteration. Its inner `_segments_touch` ALREADY rejects disjoint AABBs.
For N sampled segments the enumeration remains O(N²), even when most pairs
fail that cheap test. Circle scheduling can repeat complete scans of affected
planes for candidate schedules. The first returned crossing chooses a circle
joint, so changing crossing order can change construction behavior.

**Connectivity is a separate stage.** The existing Lawson pass already converts
coordinates to exact integers with positive common denominators once per
fixed mesh. It has an incidence map and local queue; repeated Fraction GCD
arithmetic was removed previously. Historical ring/spiral times identify a
known cost family, not a current profile of this run. Neither sampled hot
phase above establishes that a new Lawson rewrite is warranted.

**Surface bounds.** `_surface_error` computes coordinate ranges and four
`_outward` conversions before returning zero for a plane. `_outward` constructs
a Fraction of the rounded value even for an ordinary float. Spline derivative
bounds are cached, but overlap selection still linearly scans all carrier
spans per triangle. Two sampled stacks identify execution there; they do not
separate these costs or justify changing the approximation bound.

## Bounded intervention choices to review

1. **Prune ear candidates, retain current construction.** Compute each current
   ear's exact AABB once. Query an immutable owned point index, then apply the
   existing alive, corner-coordinate and exact containment rules. Return
   candidate occurrence IDs in their original order to preserve the same
   blocker/ear choice and output. A simple axis-sorted index can reduce work
   to range searches plus candidates; its worst case remains quadratic or
   worse, and boxes containing most points will limit the gain. Dead entries
   alone are a constant-factor issue; changing only their container does not
   solve repeated all-point searches. This option attacks the late sampled
   phase without replacing hole bridging, Lawson flips or refinement.

2. **Change arithmetic representation, not coordinates.** Study a per-face
   exact integer predicate/comparison view, using the existing Lawson
   positive-denominator conversion argument. Preserve original Fraction UVs,
   physical/source IDs, traversal and emitted triangles. Both axes may have
   different positive scale factors; orientation signs and coordinate order
   remain identical. Inspect denominator growth first: blindly taking an LCM
   of unrelated rational coordinates can be expensive. This option attacks
   arithmetic cost, not the O(N²) candidate count. It should not be bundled
   with option 1 until their separate value and scope are understood.

3. **Prune segment pairs with the same narrow predicate.** An owned conservative
   segment index can avoid generating disjoint pairs. Preserve closed-box
   endpoints and all equal/collinear/touching cases; visit surviving pairs in
   the same lexicographic original `(i,j)` order, so the chosen first crossing
   remains identical. This attacks the earlier sampled phase. Adding another
   AABB test inside `_segments_touch` would duplicate existing work.

4. **Tiny surface-bound fast paths, only if worthwhile.** Returning the plane's
   exact zero before unnecessary rounding work, or returning an already-float
   input directly from `_outward`, could preserve the mathematical result for
   valid inputs. Check exceptional/nonfinite behavior and actual callers before
   changing either helper. This is bounded housekeeping with unmeasured total
   benefit; it is not the answer to hours of all-pairs/ear scanning.

The current 8,300-node observation strengthens the case for reducing generated
pairs/point candidates before changing arithmetic representation or language.
It does not establish which index wins. A maintained ear/reflex-list algorithm
is a somewhat larger alternative grounded in Eberly; it changes candidate
maintenance and requires careful handling of bridge duplicates, rather than
being a one-line replacement for `range(n)`.

## Evidence required for the selected option

Reuse actual changed-output oracles, rather than adding a certification layer.
For a candidate-pruning or arithmetic-only change, bounded authored polygons
and one retained problematic face can compare exact triangle ID/order or
first-crossing tuples to the current algorithm. Include collinear samples,
duplicate bridge occurrences, the existing Eberly nearest-reflex counterexample
and many-hole case. The independent `qualify_domain` checks positive exact
triangles and the full source-derived boundary chain; whole-mesh qualification
already checks physical shell/vertex links, original source maps and unchanged
surface/chord/rounding limits. An arithmetic view additionally needs direct
exact sign/order comparison on the actual saved rational inputs, including
zero, rather than agreement after rounding them first.

Measure one bounded problematic face's wall and direct work reduction, keeping
its input, grade and complete output checks fixed. A full unchanged multi-hour
watch run is not the first diagnostic or the first performance experiment.
No test, mesh, benchmark, restart or implementation was launched by this study.

Primary references were read from the retained author documents listed and
hashed in the import study: Eberly sections 2–5; Shewchuk's robust-predicate
paper (saved-value exactness and filtered signs); Triangle sections 2, 3 and 6
as summarized by the existing connectivity study. Their algorithms are
reference material, not adopted libraries or a claimed forecast of speedup.
