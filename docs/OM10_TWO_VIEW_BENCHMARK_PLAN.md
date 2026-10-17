# Measure the complete two-view OM10 workflow

The operator requests the complete assembled movement followed by an exploded,
labeled view, on CPU and P100, with actual costs in the graphics. The existing
caller already constructs one complete source model and emits those two views
in order. Measure that real workflow once per backend instead of reconstructing
identical geometry separately for each pose. This changes measurement granularity,
not geometry qualification, rendered output, quality or backend coverage.

The native owner studied the actual caller/driver/poster code and recommended
this scope. Root design review ACCEPT: two fresh complete callers directly enclose
the requested two-view workflow. The tradeoff is explicit: there will be no
independent fresh STEP-to-single-pose wall or full-task per-pose speedup claim.
The repeated native matrix continues to measure each pose separately.

## Bounded implementation plan

1. Keep every product/helper/binary pinned by the running nonvisual qualification
   unchanged, including render_om10.py and om10_scene.py. Change only the benchmark
   driver, poster compositor and their documentation. Do not rasterize before
   complete nonvisual qualification passes.
2. Run CPU24 then P1000 as two fresh complete render_om10.py processes with
   `--view both`, default repeat_frames=1. Record exactly one external
   `cold_complete_process` observation per backend, view=`both`, n=1, through
   actual STEP, both PNG/labeled-SVG outputs and interpreter exit. Compare full
   source snapshot/BGMESH/BGVIEW/source-pin hashes and metadata source hashes
   across those two callers. Never duplicate
   the same observation as two pose rows or divide it into inferred totals.
3. Require the actual two-view BGVIEW01 to have count2/400 bytes and the existing
   assembled-alpha0 then exploded-alpha1 order. Copy each exact192-byte row under
   a count1 header into a private per-pose request file for the unchanged repeated
   fresh native matrix. Do not repack floating-point fields. Retained native frame
   batches concatenate those same rows. Continue three repeats per pose for
   CPU1/CPU24/P1000/P1001, with separate process and resident-frame scopes.
4. The final poster uses the fixed P1000 complete caller's `000-assembled` and
   `001-exploded` images/callouts, and the corresponding actual view row. Keep CPU
   images too. Read the shared complete observation keyed `view=both` for both
   posters. Caption it **FRESH STEP → BOTH LABELED VIEWS · n=1**, with an explicit
   shared-total note. Per-pose native medians retain their separate panel/caption.
   Individual native/PNG/SVG stage rows remain diagnostics; no common cost is
   apportioned between views. Final static SVG rasterization remains separate.
5. Review command construction, row copying/order/counts, output names, unchanged
   native matrix and scope labels. Existing CLI checks plus the actual matched
   outputs/byte comparisons directly exercise the change; do not add a synthetic
   benchmark-certification framework or repeat geometry/memory campaigns.

Independent plan review ACCEPT. Fresh process does not mean flushed filesystem
caches: report the fixed CPU24→P1000 order and n=1 without a statistical complete-
workflow speedup claim. After implementation, root reviews the
complete diff and native owner runs CLI checks on the fleet. Actual pilot and
benchmark execution still wait for the complete nonvisual PASS and inspected
pilot images. No source reuse is represented as a fresh source-to-image result.

Implementation review ACCEPT: complete observations remain one row per backend
with view=both; metadata and original snapshot/scene/request/source pins match;
actual row offsets16/168 select alpha/flags before exact copying. The poster
uses frame0/frame1 and the shared total, while native fresh/resident scopes stay
separate. Both CLI checks passed on the fleet. No pinned product/helper changed.
