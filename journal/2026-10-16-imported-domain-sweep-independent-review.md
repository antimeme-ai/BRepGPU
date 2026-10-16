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
