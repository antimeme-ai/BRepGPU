# OM10: diagnosis before proceeding

Operator direction: diagnose and return before implementation. This report
changes no product code, quality setting, source input or running job. The
existing V4 process remains live. No replacement mesh or benchmark was launched.

## Findings

**The delay is in the production CPU mesher.** At the initial diagnostic check
V4 had elapsed about4h17m with roughly one core active. STEP translation,
source decode and initial original geometry/assembly checks together took about
57s. The mesher's saved live clocks show5232.435s (87min12.435s) from its own
start through edge validation/refinement, circle coordination and canonical
edge materialization, before its per-face loop. That combined stage wall does
not isolate circle scheduling. The remainder so far is per-face construction.
CPU24 and P100 selection apply to the later native consumer; this Python mesher
itself is serial.

**Two quadratic searches are observed and visible in the code.** An earlier
three-second nonblocking sample returned26 valid stacks, all in circle-schedule
boundary intersection checking. A later sample returned9 valid stacks and
20 read errors: seven in ear containment scanning and two in surface error/
rounding work. These short samples identify executing phases, not whole-run
percentages or exact arithmetic cost shares.

A subsequent successful nonblocking locals read caught boundary simplicity
checking at zero-based face index2688: original face167661, definition1756.
Read-only mmap of the immutable snapshot identified a102-span spline carrier
and one outer loop with four original coedges; two long edges are102-span cubic
curves. The live boundary has8300 sampled nodes, with no holes or chart closures.
The current all-pairs checker can visit34,432,550 nonadjacent segment pairs on
that one face before triangulation, unless an earlier crossing ends the scan.
It already rejects disjoint AABBs, but only after generating each pair.

The ear routine keeps linked neighbor slots, yet each convex candidate scans
all original `n` slots, including removed points. Successful ears alone on a
no-hole8300-node input entail68,865,100 inner slot iterations. Rejected candidates
can add substantially more work. These are code-derived counts, not instrumented
counts from this live face. Exact plane UVs are Fraction-valued, making many of
those operations rational comparisons; the current spline face's UVs must not
be mislabeled as the previous plane's exact UVs. Stale locals from a finished
face do not identify the face in the earlier ear sample.

**Refinement and representation may amplify the cost.** The surface-bound
helper scans all carrier spans for each triangle's overlap query, even though
derivative values are cached. It also performs range/rounding work before its
plane-zero return. Shared-edge refinement can produce large boundaries from
small source topology. Sampling quality may be conservative, but neither excess
sampling relative to the chosen contract nor its total runtime share has been
measured. Dropping shared samples or weakening quality is not a diagnosed fix.

**The reference is faster but incomplete on the hard case being processed.**
The fresh NVIDIA attempt used12 native worker processes on12 physical cores;
ours uses one Python meshing core. Each policy emitted103/107 definitions and
212,832 triangles passing the existing physical shell/original Euler checks.
Four definitions failed. The current owned face belongs to1756, one of those
failed definitions. Requested0.005mm chord height does not establish equal
actual output quality; finite source-boundary admission is still pending.
Thus7.23s/75.42s worker batches and4h+ incomplete owned qualification cannot be
turned into a complete matched speedup. The observed owned enumeration cost
is nevertheless concrete and independently warrants repair.

## Proposed first intervention, awaiting the operator

Reduce candidate generation before changing language or geometry semantics:

1. A conservative segment index for boundary checks, retaining the existing
   exact touch predicate, adjacency exclusions, closed-box behavior and original
   lexicographic first-crossing order. Circle scheduling depends on that order.
2. A separately reviewed ear-point query index and cached current-ear bounds,
   retaining alive/duplicate-bridge rules, exact containment and current ear
   cursor order. This changes searches rather than introducing a new mesher.

Each is a bounded conceptual unit. A per-face exact integer arithmetic view is
another possible follow-up, and a new constrained triangulator is a larger
alternative. Neither is required to begin removing the observed enumeration.
Native ownership, definition parallelism and GPU work remain measured placement
options after these algorithmic costs are understood.

After operator direction, capture/reproduce the problematic sampled input,
check candidate coverage and the exact first-crossing/triangle result against
the current routines, and reuse the existing domain/source/physical oracles.
Measure the isolated changed path before another full watch attempt. Separately
choose a common reference-successful definition for actual same-quality CPU
comparison. The current NVIDIA-failed spring cannot supply that passing pair.
No new corpus, mutation or certification campaign is required for this work.

Detailed algorithm/source analysis:
[algorithm diagnosis](2026-10-15-om10-mesher-surgical-diagnosis-algorithms.md).
Reference scope and existing per-definition stage/count analysis:
[reference diagnosis](2026-10-15-om10-mesher-surgical-diagnosis-reference.md).
Original [source and cost notes](2026-10-15-om10-mesher-cost-study.md) and
[connectivity reading](2026-10-15-om10-return-pass-connectivity-reading.md)
remain relevant. Numeric locals, raw stacks, source tables and operational logs
stay private in ignored context/. This is the requested report, not a go-ahead.
