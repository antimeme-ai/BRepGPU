# Explicit source UV preparation subplan

2026-10-15, reviewed before code. Whole model no-processing source
extraction fails at original coedge109547. The complete audit finds24,602
nonplanar missing-pcurve uses, all original SameParameter flags true and all
original edge bindings preserved. All375 multi-loop faces are planes; all635
spline surfaces are open/nonperiodic. Raw neutral input remains NVIDIA's
deliberately supported missing-UV format; this unit serves owned extraction and
OCCT query faces only.

Design: `papers/2026-10-15-om10-explicit-uv-source-preparation.md`.

1. Root/oracle/native adversarial design and bounded-plan review. Read exact
   selected methods/callees, not class names. No whole-shape healing.
2. Genuine fleet authored extraction red before implementation. Existing real
   whole-source missing109547 failure is retained. Extend the literal source
   fixture with a bounded pole/seam case only where needed, preserving existing
   box/cylinder fixture and its NVIDIA producer tests.
3. Add one shared `native/source_uv/source_uv.hpp/.cpp` capability: deep detached
   geometry/topology, original-use chain validation, missing-PC projection,
   direct checked projector then single-PC attachment; fresh existing-PC-only
   FixAddPCurve for native dual seams; direct FixSeam/FixShifted; whole-basis
   true-period/readback/source-correspondence checks. Preserve raw foreign PCs
   for owned extraction. Optional detached query copy applies narrowly admitted
   existing source-column/finite-joint/pole2D corrections before natural-domain
   admission, with actual endpoint/control perturbation bounds unchanged.
   Guard3D changes directly; use existing original geometry qualifier once for
   actual emitted coefficients. No audit framework or generic repair engine.
4. Integrate prepared shape mappings into `native/step_source/main.cpp` and its
   CMake target. Keep raw graph matching separate. Add only optional query-BRep
   output from those same prepared solids; preserve BGSRC001 fields/IDs and
   default positional CLI. Raw NVIDIA exporter does not invoke preparation.
   Emit preparation wall times only to stdout/external timing rows; keep
   BGSRC001 bytes deterministic across fresh CPU/GPU benchmark callers. Parent
   integrates one query export in qualify_om10.py's query-source directory and
   binds it to source snapshot/original SHA with existing input/report hashes;
   the NVIDIA finite qualifier consumes that prepared query set, not raw neutral.
5. Focused source formula/parameter/seam/pole/unsupported failures green, one
   affected source/reference reader run as justified, independent code review.
6. Fresh complete owned source +107 query V4 files, existing independent raw
   geometry/placements/166 occurrence checks and old/new actual delta. No V4
   full meshing until parent reviews the actual source. Native query owner uses
   these explicitly prepared files and retains strict missing-pcurve preflight.

Initial direct projector research used copies on one representative of each14
actual nonplanar carrier/curve families. All returned2D output;17 probes per
representative have maximum8.75e-5mm residual. This does not qualify untested
edges or universal projection error. A probe initially reused CurveOnSurface's
null output interval and failed on zero-range TrimmedCurve; corrected the
diagnostic to retain original3D interval separately. No production source was
altered by that experiment. Mature projector cache/status/alias/sphericalV and
coincident-UV nudge hazards are recorded in the independent method review.

Root and independent oracle accepted the revised design/subplan before implementation. Native review accepted the checked-projector/existing-PC-only seam path. Genuine authored fleet red is the next step; no reconstruction implementation at acceptance.

Bounded new-projector known-input probe confirms three beyond-grade natural-U
excursions on172494/172983/173010 and the existing169982 endpoint gap;173009's
9.24e-14 excess is within128ulp(4). Root correctly identified that an adapter
hard-domain gate would block already reviewed source-based owned recoveries.
The revised design preserves raw projected PCs for that consumer and explicitly
prepares a query copy, without another STEP parse or projection. Native review
found unchecked null seam dereference in FixAddPCurve's missing-PC branch;
selected direct projector checks plus existing-PC-only seam attachment remove
that failure path. Oracle review requires whole-basis period translation and
complete ordered original-use ownership; both are now explicit. Await revised
root/peer acceptance before genuine red or production.

Root revised review accepts source bindings and raw/query split, with two
required integration details now written: no wall times in deterministic
BGSRC001 fields; one optional query export in the existing qualification caller
and existing source/report hashes, consumed by the reference finite qualifier.
No extra persistent protocol or receipt mechanism.

Final root/oracle design and bounded-plan ACCEPT recorded before code. The
focused source fixture scope is existing authored box/cylinder plus a literal
three-great-circle spherical octant through the north pole, with original use/
period failure cases. Source owner owns the shared module, adapter and existing
qualification caller; reference owner owns consuming prepared query files.
Include native/source_uv in existing source pins. Whole V4 remains gated on
focused green/code review and actual complete original source/query export.

## Authored red before production

The existing reference box/cylinder STEP includes explicit SURFACE_CURVE/PCURVE
representations, so it does not exercise this source gap. The first test attempt
correctly exposed that fixture mismatch: extraction succeeded, then the original
geometry oracle rejected unsupported SURFACE_CURVE wrapper ID139. Retained that
initial run; it is not evidence of a missing-PC failure.

Added separate authored `tests/fixtures/source-uv.step`: same box/cylinder and
source topology/product IDs, with all15 EDGE_CURVE references pointing directly
to the existing spatial basis curve of their SURFACE_CURVE/SEAM_CURVE wrapper.
The original reference fixture and its NVIDIA tests stay intact. Unreferenced
2D records are retained; no source geometry, units or topology is fabricated.

The corrected fleet command `PYTHONPATH=python python3 -m unittest discover
-s tests -p test_source_uv.py -v` is genuinely RED before any production code:
original cylinder coedge400 has missing recovered UV; the optional query CLI is
absent; the new literal octant native fixture entry is absent. Three methods,
0.115s, two failures/one missing-entry error. The ignored
`context/om10-source-uv-direct-spatial-red.txt` retains the exact output.
The tests check existing raw original coefficient/assembly oracles, independent
original C(t)/S(UV) evaluation, deterministic BGSRC bytes, V4 query presence and
literal spherical octant curves/analytic surface. Negative native fixture modes
must actually mutate native input and traverse production guards; mode-name
refusal in the fixture would not establish the failure claim.
