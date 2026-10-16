# Live owned qualification phase observation

The durable complete nonvisual V4 qualifier remained active near 43 minutes,
using one CPU core and approximately 361 MiB RSS. The last regular edge marker
was 25,000/25,871. Rather than infer a hang or restart it, observed its Python
stack without pausing the process or editing its pinned inputs/binaries.

Acquired the isolated py-spy 0.4.1 wheel/executable; restoration and SHA-256 are
in QUARANTINE.md. No system packages or Python environment were changed.
A single nonblocking dump returned an incomplete-frame error, so one bounded
three-second, 10 Hz nonblocking raw-stack sample followed. It returned 26 samples
and three errors. All 26 samples show `_coordinate_circle_schedules` calling
`_boundary_intersection`, predominantly its exact `_segments_touch` AABB
comparisons. This identifies the observed quiet phase as planar circle-schedule
boundary checking after the full edge loop. It does not time that complete
stage, identify the current face or establish a whole-run dominant hotspot.

Private output stays in context/. The qualifier kept running; no source decode,
mesh, benchmark, image, production change or second qualification was launched.
The previously deferred candidate-generation/indexing notes now have one actual
phase observation; no optimization is selected from this short sample.

## Later progress observation

At an operator status check the same durable V4 process had elapsed 4 h 09 min,
used approximately one full CPU core and 1.57 GiB RSS, and had advanced past
circle scheduling into face meshing. The latest emitted marker was
2,600/9,162 faces. No exception or final mesh/native qualification output had
been emitted. Face counts are progress markers, not an elapsed-time forecast.
The independently closed SDK rectangle export now passes the actual date disc
under both policies; two bridge and two nonsimple polygon failures remain.
No complete owned or reference watch image exists at this observation.
