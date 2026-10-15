# Independent native CPU/CUDA review

2026-10-15. Read all owned native display math, binary decode/validation, CPU
workers/tile bins, CUDA kernels/scan/storage, CLI/output/timing, plugin values
and isolated CMake configuration. Reviewed against the accepted design and
independently written source-jet/pixel fixtures.

Accepted the geometry and ownership implementation. Explicit count/placed-slot
limits bound32-bit bin indices;8192sample dimensions bound the second scan level
to512 block sums. Count/scan/fill retains every candidate. Tile scatter ordering
cannot change maximum-depth/lowest-ID winners. Integer edge signs/top-left
ownership use unbiased weights for interpolation. Source first jets, boundary
canonical XYZ, face signs, rigid normal rotation and alpha0 bypass are retained.

CPU workers finish each immutable generation before task replacement or shutdown.
CUDA allocation/resize ownership survives partial failures, launch/stream errors
surface before accepting output, and teardown attempts all owned releases and
restores the prior device. CPU has no linked CUDA path; optional loading occurs
only for requested CUDA. Fresh output directories prevent accepting old images.
Cold/session and retained frame walls have explicit nesting; process shutdown
remains outside the internal wall and inside the caller's external process wall.

One small timing finding was sent to the owner: CPU projected-diagnostic copying
was included in the complete wall but absent from named download attribution,
while the corresponding GPU copy was attributed. Assign the CPU copy to its
download row. This affects diagnostic stage accounting, not geometry or the
whole wall; the existing project-only fixture exercises it.

The first two direct methods predated implementation and failed because the
executable was absent. The later expanded nine-method direct family passed on
CPU1/CPU24/P1000/P1001; the native owner also reported relevant host sanitizer
and device memory results. A tenth actual owned thin two-hole mesh→native scene
writer→source jets→assembled/exploded project-only integration case subsequently
passed on all four modes, checking the complete provenance/geometry/placement
reader that the actual OM10 caller will use. These synthetic checks do not
substitute for complete actual-source mesh and native qualification.

No general GPU solid construction, source validity, global deflection certificate,
alpha readiness or actual watch render is established by this native review.
