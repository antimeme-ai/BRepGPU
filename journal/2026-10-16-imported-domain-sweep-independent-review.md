# Independent imported-domain sweep review

Read the existing `_simple_domains` and exact `_intersect`, `_on_segment`,
`turn` and `_winding` helpers before the optimization plan. ACCEPT
`docs/IMPORTED_DOMAIN_ORACLE_SWEEP_PLAN.md` before product code. No geometry
execution or fleet-file mutation was performed by this reviewer.

The proposed exact coordinate ranks preserve equality and ordering only; source
endpoints still feed the unchanged exact geometry. Choosing either projection
axis by actual closed-overlap count changes work order without changing the
candidate set. The broadphase's active scan remains quadratic when both
projection counts are dense, as the plan states.

The key invariant is that at a segment's start the active set contains exactly
earlier segments whose closed maximum has not expired. Expiration must use
strict less-than in both projection counting and actual enumeration. Therefore
every closed box overlap reaches the other-axis test exactly once, including
equal starts, touching endpoints and zero-width vertical boxes.

Canonical original list ordinals must precede the existing adjacency exemption:
sweep order is not original loop order. Only immediate and closing neighbors
of the same loop remain exempt. Nonadjacent equal-coordinate touches and all
inter-loop touches stay forbidden. Area, collapsed-edge, Fraction intersection,
hole winding and nesting checks remain unchanged.

Asked the owner to make the counters precise: `projected_pairs` is the complete
chosen-axis count; `active_visits` may stop early when invalid geometry raises.
`max_active` must state whether it includes the newly inserted current segment.
These are small work observations, not a second qualification system.

## Actual code review

Reviewed `_DomainSweepWork`, `_domain_box_candidates`, the changed
`_simple_domains` and all eight focused methods. ACCEPT; no code findings.
Both counting and enumeration heaps retain equal-coordinate ends. The chosen
other-axis box components are correct for both X and Y sweeps. Current insertion
occurs after earlier active pairs are yielded, and no active dictionary mutation
occurs during its iteration. Ordinal canonicalization restores the exact old
loop-owner/neighbor exemption. Original Fraction geometry and nesting source
are unchanged. Counter reset, complete projected count, early-stopped visits,
pre-adjacency box candidates and occupancy agree with the documented meanings.

Root/owner report the genuine old-reader RED: 590,566 Fraction comparisons for
the actual 512-site protected strip against the predeclared 32,768 operation
bound. Six semantic methods passed the old implementation; the eight focused
methods passed the new implementation in 0.259s on the fleet. Those runtime
observations are the owning lane's work; this reviewer did not repeat them.
The input fixtures exercise nonzero-area proper crossing, vertical/closed
touches, exact wide translations, arbitrary loop starts, inter-loop ownership
and unchanged outer/hole containment behavior. Root was cleared to replay the
actual complete-part qualifier with newly recorded oracle pins.
