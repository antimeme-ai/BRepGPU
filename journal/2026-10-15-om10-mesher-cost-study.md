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
