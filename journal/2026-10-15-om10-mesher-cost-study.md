# Read-only OM10 mesher cost study

Requested as deferred round-two work while complete nonvisual qualification
V4 runs. Read the current mesher, the existing circle-schedule design and
private earlier/current progress logs. Used a low-priority read-only mmap of
the completed V5 BGSRC tables for original boundary/span counts; no source
decode, mesh, test, benchmark, profiler attachment, dependency, helper edit or
binary change was performed. No actual images were rendered.

Saved findings and concrete deferred candidates in
`papers/2026-10-15-om10-mesher-cost-study.md`. The important distinction is
algorithmic pair enumeration versus the AABB rejection already implemented
inside `_segments_touch`; the latter is not a new opportunity. Existing
progress does not separately expose pre-edge spline-joint preparation or
post-edge schedule/canonical-vertex phases. No dominant hotspot or complete
watch runtime has been measured by this study.

No production implementation selected. Root owns integration and git.

After the operator requested diagnosis before further work, performed a second
read-only algorithm review and re-read the retained Eberly/Shewchuk material.
Saved the specific complexity, exact-arithmetic and bounded intervention/oracle
analysis in `papers/2026-10-15-om10-mesher-surgical-diagnosis-algorithms.md`.
Root supplied the nonblocking live samples and numeric locals; no further
process attachment, execution, implementation or restart was performed here.
The current 8,300-node/four-coedge face demonstrates why actual sampled counts
must replace original topology size when diagnosing this run. Recommendations
remain proposals pending the operator-facing report.

For the operator's subsequent C++ phaseout study, inspected imported source/
mesh records, exact plane construction, packed display ownership and ABI5
source/revision/slot conventions. Saved the bounded native-domain recommendation
and numerical/identity/concurrency/oracle seams in
`papers/2026-10-15-cpp-migration-numerical-seams.md`. No implementation,
execution, benchmark, installation or dependency adoption; the diagnostic
hold remains in force.
