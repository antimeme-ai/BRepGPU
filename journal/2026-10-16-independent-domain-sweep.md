# Independent domain-reader candidate enumeration

Root reported actual complete-part construction (438 faces/191,688 triangles,
209.608s) followed by over 200s in independent qualification. A bounded live
stack sample located the Fraction AABB all-pairs loop in `_simple_domains`.
Root assigned replacement of that inadequate broadphase as one conceptual unit;
the production triangulator and answer predicates remain separate owners.

Read the exact current function and callers before design. Saved
`docs/IMPORTED_DOMAIN_ORACLE_SWEEP_PLAN.md` for independent review before code.
Proposed exact coordinate-rank closed boxes and an independently implemented
adaptive-axis sweep. Candidate predicates still use raw original Fraction
coordinates, and original loop/area/winding/owner/adjacency semantics stay intact.
The smaller projected-overlap count chooses X/Y generically; dense projections
can still produce quadratic active work and are reported honestly.

Proposed a scoped Fraction-comparison work-property RED rather than a wall-time
threshold, one private optional work dataclass, focused error-raising geometry
cases, and actual saved-domain/complete-part consumers. No new dependency,
production BVH/flag reuse or additional certification layer is selected.

The old fleet qualification remains alive and pinned. No fleet source upload
or geometry execution occurred during this design. Root must clear that worker
before its imported-oracle file is replaced.

Root and independent numerical reviewer accepted the written plan before code.
The old worker then ended with qualification TIMEOUT_CENSORED at 300s (whole
517.674s, construction 209.608s unchanged), and root cleared the fleet reader
for replacement. This is the historical outcome; it is not a result for the
new reader.

Authored the focused test first. Its selected work-property method against the
old actual reader was RED: 590,566 Fraction rich comparisons on the valid
512-site strip, exceeding the 32,768 bound, in .693s. Six semantic methods on
that old reader were GREEN in .186s before implementation. Then implemented
the reviewed exact-rank adaptive-axis broadphase and private per-call work
value; the eight focused methods were GREEN in .259s. No exact area,
intersection or hole-winding predicate was replaced.

Independent numerical code review ACCEPT with no findings: both heaps use
strict closed-bound expiration, original ordinal ordering restores the exact
neighbor exemption, and only integer ranks enter the broadphase. Root also
accepted the implementation. Root owns the subsequent committed source and
complete-part replay; the new fleet reader is pinned and remains untouched
while that replay runs.

Executed the affected consumer command on the Linux fleet, single CPU4, with the
existing ear-validation DSO explicitly selected (no native rebuild):

```sh
PYTHONPATH=python:tests:examples taskset -c 4 python3 -B -m unittest -v \
  test_imported_domain_sweep test_direct_cdt_oracle test_direct_cdt \
  test_face_domain_external test_native_face_domain_integration \
  test_imported_geometry test_imported_meshing \
  > context/domain-sweep-affected.log 2>&1
```

Result: all 80 methods PASS in 18.675s. This checks the new work property and
closed-touch cases plus actual direct/CDT/reference and imported-mesh consumers.

Replayed only the two already saved private original-domain snapshots with
`context/domain-sweep-replay.py`, `PYTHONPATH=python taskset -c 4 timeout 30s
python3 -B`, one fresh process each. The reader decoded original exact values,
then completed `_simple_domains` with outer-first/hole-following owner flags;
it did not load a producer's saved validity or construct a replacement mesh.

| Existing source | Sites / loops | Complete admission | Axis / active visits | Max active / exact tests | Process peak KiB |
| --- | --- | --- | --- | --- | --- |
| 165063 | 1,114 / 1 | PASS, .040738s | X / 2,233 | 4 / 0 | 27,940 |
| 167661 | 8,300 / 1 | PASS, .274259s | Y / 20,737 | 4 / 0 | 32,572 |

All boxes reaching the source predicate stage in these two domains are the
historical adjacent/closing-neighbor exemptions; hence zero exact nonadjacent
intersection tests. Actual area/collapsed-edge checks and whole-domain admission
still ran. Captured results are private `context/domain-sweep-165063.tsv` and
`context/domain-sweep-167661.tsv`. The two newly created operational JSON logs
were converted to TSV and removed; acquired original source bytes were untouched.
The replay script now writes TSV directly. These are original-domain reader timings,
not complete mesh qualification or product triangulation timings. Dense overlap
still admits quadratic work; the two saved domains do not establish a universal
bound. No additional workload campaign or acquisition was performed.

Public-safe results, check outcomes and actual fleet source pins are saved in
`results/domain-oracle-2026-10-16/`. Root owns adding the new complete-part result
and publication. No fleet pinned product source changed during these replays.
