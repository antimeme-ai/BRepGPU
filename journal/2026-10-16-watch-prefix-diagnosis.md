# Bounded whole-definition construction diagnosis

Root's unchanged direct whole-watch trial ended at a 30-minute deadline, after
all source edges had been sampled and the last reported face checkpoint was
5,400. The diagnostic question is whether that point reflects ordinary prefix
cost or an unusually expensive complete definition near the checkpoint. The
whole watch will not be rerun unchanged for this unit.

Reuse the existing private complete-part harness with a generic `--definition`
selector and construction-only exit. Keep the exact original whole-definition
closure, original quality and source provenance, and pinned direct/validation
libraries. Reject foreign edge incidents or carrier faces rather than removing
them. No product, default quality, triangulation, or qualifier changes are part
of this diagnosis. No new correctness/qualification claim follows construction.

Run complete definitions 1774, 1775, 1776, 1777, 1778, 1779 in source order, one
fresh process each on one CPU, with a 120s total process budget per candidate.
Stop at the first failure or censor. Report CONSTRUCTED, FAIL, or CENSORED,
original closure counts, product stage times and last progress. If a candidate
remains active past 45s, take at most one 3s read-only py-spy sample while it is
alive; retain profiler errors as limits, not invented stack evidence. Retain
private source bytes and TSV operational records. Root owns publication/commits.

This subplan is written before adapting or executing the private harness. Root
accepted it, and clarified that the fresh direct whole-watch trial is stopped
while the historical original job remains alive read-only. That separate job
and its original worktree are untouched by this diagnostic unit.

Adapted only private `context/direct-cdt-diagnosis/diagnose_definition.py` from
the existing complete-part harness. The complete closure/source-row identity
checks and retained original-provenance checks remain. The generic selector
also checks the expected original face/edge counts for each candidate. Removed
the qualifier import/call entirely from this diagnostic harness; success writes
CONSTRUCTED, never a qualification pass. Parent deadline is 120s for the total
process, not a reset at each phase. The optional 3s read-only profiler uses root's
existing isolated executable via `sudo -n`, with stderr retained. No candidate
reached its 45s trigger.

First command (fleet CPU4, unchanged pinned release DSO for validation/direct):

```sh
python3 -B -u context/direct-cdt-diagnosis/diagnose_definition.py \
  --definition 1774 --backend direct \
  --library build/direct-cdt-release/libbrepgpu_direct_cdt.so \
  --direct-library build/direct-cdt-release/libbrepgpu_direct_cdt.so \
  --cpu 4 --seconds 120 \
  --output context/direct-cdt-diagnosis/definition-1774-v1
```

Then ran exactly the remaining five definition IDs in order with the same
arguments, one fresh process at a time, stopping on any nonzero exit. All six
exited zero with CONSTRUCTED; no censor/failure occurred, so all planned
candidates were reached. The first result was sent immediately to root before
the remaining candidates finished.

| Definition | Original faces/edges | Triangles | Complete mesh call seconds |
| --- | --- | --- | --- |
| 1774 | 72/210 | 4,400 | 5.468420 |
| 1775 | 13/30 | 1,620 | 1.826275 |
| 1776 | 9/13 | 3,668 | 2.673123 |
| 1777 | 10/24 | 68 | .132953 |
| 1778 | 37/102 | 13,748 | 14.558760 |
| 1779 | 6/12 | 10,624 | 3.107557 |

Whole original source decode took 7.10–7.27s in each process, distinct from
these mesh calls. The sum of independent part mesh calls was 27.767087s,
147 original faces, 34,128 triangles, not a measured whole-watch stage. Closure
asserted zero foreign incidents/carrier faces removed,
and checked original row identity. All actual pinned sources were verified again
on successful construction. DSO SHA remains `605e0b21c58093fe5270a30099b9338d18c1b2880de7465d8188553db2a7f968`.

Public-safe stage outcomes and source/binary pins are in
`results/watch-prefix-diagnosis-2026-10-16/`. Private provenance/closure/worker
logs and selected original-source snapshots remain under ignored context.
No unusually slow complete definition near the checkpoint was reproduced in
isolation. This result does not isolate earlier prefix costs or whole-source-size
effects. Qualification/display/normals are NOT_RUN. No product changes, new
tests, profiler sample or whole-watch retry was performed for this diagnosis.
