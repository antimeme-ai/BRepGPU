# Independent imported-domain oracle: exact ranked sweep

Status: root and independent numerical reviewer ACCEPT before code. This replaces candidate
enumeration inside the existing `_simple_domains`; it does not add another
qualification layer or consume a production triangulator's validity flags.

## Actual problem and preserved contract

The actual 438-face part constructed 191,688 triangles in 209.608s, then remained
in independent qualification for over 200s. Root's bounded nonblocking stack
sample found the original-domain reader enumerating all segment pairs and doing
Fraction min/max AABB comparisons. This is actual consumer cost, not a proposed
certification exercise.

Retain the current reader's return value, error classes/messages, loop-owner
meaning, minimum loop size, original signed-area checks, collapsed-edge checks,
exact `_intersect`/`turn`/`_on_segment`, hole containment and hole nesting logic.
Retain the precise same-loop immediate/closing-neighbor exemption. Nonadjacent
equal-coordinate endpoints and all inter-loop touches remain forbidden. Do not
change the current adjacent-collinear admission rules as part of optimization.
Different traversal order can find a different offending pair; this private
reader does not return a crossing witness.

Original sites remain untouched finite exact source values. The existing area
and winding code uses those values exactly as before. Rank coordinates represent
only equality/order and closed boxes; they never represent orientation, distance,
area, interpolation or a replacement source coordinate.

## Owned broadphase

1. Build the same original segment records `(loop_index, edge_index, a, b)` after
   existing loop/area/collapse checks. Their list ordinal is stable identity.
2. Independently sort distinct original coordinate values on each axis and map
   each value to its exact order rank. Equal values receive equal ranks; no
   float conversion, quantization or production BVH call occurs. Build each
   segment's closed integer `[xmin,xmax] x [ymin,ymax]` once.
3. Before enumeration, count projection-overlap pairs for each possible sweep
   axis using sorted starts and a heap of active ends. Expire **only ends less
   than** the next start. Add current heap size before inserting each event.
   Choose the axis with fewer projected pairs, breaking ties in favor of X.
   This general ordering decision avoids a fixed-axis cliff on a rotated narrow
   strip without changing source geometry. It is not keyed to source names,
   dimensions or watch sample counts.
4. Sort events by selected-axis minimum rank, then original segment ordinal.
   Keep an active dictionary by ordinal and a min-heap `(maximum_rank,ordinal)`.
   Remove expired entries only when maximum rank is strictly less than the new
   minimum rank. Same-coordinate starts/ends and zero-width vertical boxes
   coexist, preserving closed touching contacts.
5. Visit active records, reject disjoint closed intervals on the other axis
   using integer comparisons, and restore original ordinal ordering before the
   current adjacency exemption. Equivalently adjacency is symmetric by original
   owner/edge indices; never assume sweep order equals source loop order.
6. For remaining pairs call the existing exact Fraction `_intersect(a,b,c,d)`
   on original endpoints. A true result raises the current crossing/touch error.
   Insert the new segment after querying active records: each candidate pair is
   visited once, including same-start events. No pair list is materialized.
7. Run the unchanged original outer/hole winding and nesting checks.

Completeness: if two closed segment boxes overlap, their projections overlap on
the selected axis. At the later start (ordinal breaks ties), the earlier box has
not expired. The other-axis closed test then passes, so the original exact
intersection predicate receives the pair unless it has the existing adjacency
exemption. Any skipped pair is disjoint in at least one original coordinate.
Thus candidate enumeration preserves the existing geometric acceptance/rejection.

The active scan is intentionally simple and independent of the producer's BVH.
Preprocessing is O(N log N) comparisons/storage O(N); enumeration is O(P), where
P is the smaller closed projection-overlap count, plus actual exact candidate
work. P can still be quadratic for dense projections. This is not a universal
O(N log N) intersection algorithm. An augmented interval tree or full segment
sweep is a later contender if measured active work demands it; do not silently
add that machinery or claim its bound for this implementation.

## Minimal work observation

Add one private owned dataclass `_DomainSweepWork` with segments, sweep_axis,
projected_pairs, active_visits, box_candidates, exact_tests and max_active. An
optional keyword `work=None` on `_simple_domains` fills/reset this value when
provided. Existing callers need no change and still receive None on success.
`max_active` includes the current segment after insertion. `projected_pairs` is
the complete selected-axis count, whereas `active_visits` can stop early if an
invalid contact raises. Counts reset per call rather than accumulate implicitly.
No log stream, telemetry system, persistence format or per-predicate callback is
introduced. Actual captured-input replays may request these counts once.

## Reviewed RED → implementation → actual consumer

1. Review this plan, particularly closed expiration, ordering/adjacency, adaptive
   axis selection and unchanged Fraction geometry. Root/numerical ACCEPT precedes
   product edits.
2. Author focused `tests/test_imported_domain_sweep.py`. Before implementation,
   run a real work-property RED on a known valid protected strip: temporarily
   count Fraction rich comparisons using scoped standard-class method wrappers
   while calling the unchanged reader on actual Fraction inputs. At N512 require
   fewer than 64*N comparisons. Old all-pair AABB comparison exceeds this; this
   is an operation-count claim, not a guessed microsecond cutoff or a test of
   min/max syntax. Restore comparison methods immediately after the call.
3. Test independent valid/invalid domains: rotated strip; vertical/equal-X and
   zero-width boxes; nonadjacent endpoint touch, proper crossing, collinear
   overlap/backtracking, separate-loop touch, outer/hole order, nested/outside
   holes, multiple outer components, arbitrary exact translations/wide rational
   values. Preserve original assertions rather than requiring changed witnesses.
   Existing authored/reference geometry checks remain actual consumers.
4. Implement only the owned ranked sweep/work value. GREEN the relevant new
   work/geometry cases and affected independent domain/direct-CDT/imported-mesh
   methods on the fleet. No unrelated GPU/mutation/certification campaign.
5. Independent review; fix findings and rerun affected cases. Measure complete
   input admission, raw exact candidate work and process memory on the actual
   saved 1,114-/8,300-site domains when available; retain censored outcomes. A
   valid return needs the whole unchanged area/crossing/winding reader, not only
   a producer's saved validity statement.
6. Root replays actual complete-part qualification with the new reader and
   unchanged actual constructed mesh/source. Report qualifier time separately
   from product time. The old pinned worker's result is historical, not relabeled
   as the upgraded reader.

## Live-run discipline

The current main-plate qualification still owns its old code pins. Do not upload
or mutate its fleet `python/brepgpu/imported_oracles.py` while that worker is
alive. Local design/tests are safe. Fleet RED can use a private isolated copy
of the current reader without replacing the pinned file; product upload waits
for root to confirm that the old worker completed or censored. All geometry
execution remains fleet-only. Root owns Git commits and deployment timing.
