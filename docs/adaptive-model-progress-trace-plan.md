# Optional native progress trace

2026-10-17. Root/numerical review before body. No source/geometry/decision,
allocator, Budget/Failure API, work/precision/memory cap or CUDA ABI change.
The2epsilon hypothesis is falsified: adaptive_traces.cpp1531–1599 passes the
actual captured Q endpoints to prepare_affine_trace; complete_trace_intervals1803
already requires S(selected affineUV)−SAME Q chord+domain<=faceepsilon. The separate
Ce−Q bound must NOT be added again. No acceptance change is selected.

Own adaptive_trace.hpp (small header-only native diagnostic helper), narrow
adaptive_trimmed.hpp/.cpp, adaptive_surface_cache.cpp, adaptive_regions.cpp,
adaptive_model_main.cpp, existing native_adaptive_model.py and this journal/plan.
No core, geometry kernels, CMake, Failure or exact.hpp changes.

Add --trace-progress as a boolean model option, defaultfalse; set a trailing
TrimFaceLimits.trace_progress. Fixed TSV rows to stderr via std::osyncstream,
serialized whole rows, catch/suppress diagnostic I/O failure so it never masks
or changes native computation. No output artifact or numerical data dependency.
No new source evaluation, exact-coordinate import, arithmetic work charge or
callback deciding geometry. Off path returns before formatting. No log history.

A stack-owned TraceContext contains ONLY original sourcefaceID, query, assignment,
horizon and retained size/counter snapshots. RAII installs its address in an
inline thread_local slot during each FaceTrimSession::step, restoring prior slot
on every exit. The pointer cannot escape that synchronous native call; resumed
owners install a fresh context on their new worker. Surface/region helpers read
only the current context; async CPU/GPU jobs neither borrow nor emit it.

FaceTrim emits phase_begin/phase_end (failure end distinguished), with current
horizon, construction revision, source sites, construction sites/facets, pending
facet count, anchors, selected cache leaves, failed count when available, per-step
and cumulative work/peak. Keep only one active phase tag per owner to suppress
pending-poll rows; reset at assignment begin/retirement. A transition closes the
old phase; next step opens the new phase. Existing original exceptions rethrow.

RegionBuilder::triangulate emits cdt_begin BEFORE construct_region_cdt from its
ALREADY BUILT local.size and directed cycle lengths: actual N/B/component/revision,
caller Budget work/peak. cdt_end/failure adds workdelta; no extra topology scan.
SurfaceCache bound paths emit subphase transitions and exception-path local
Budget work/peak, configured childbytes and effective remaining worklimit BEFORE
that JobContext dies. A tiny scope guard detects unwind and logs only; it does
not wrap/change Failure. This disambiguates the1B bound horizon vs other owners.
Root's private failure-only Budget::charge GDB capture remains available.

RED: existing CLI --trace-progress is genuinely unknown. GREEN: extend only the
existing model driver with a focused CPU1 two-definition mesh-only trace port.
Traceoff stderr has no progress rows; traceon has balanced phase/CDT begin/end
rows for actual originalfaceIDs, nonnegative work/counts, and12 complete face
identities. SAME original-source reader checks both outputs and full mesh/
captured-boundary bytes agree. Existing finite source refusal preserves original
error and no final/temp output; trace rows do not turn failure into success.
Strict model build, product static checks and root/numerical actual-source review;
no new geometry oracle/cohort or full107 replay. Announce coherent source window.
Root then owns ONE selected remaining-component diagnostic using trace, not an
unobserved repeat of the entire assembly.
