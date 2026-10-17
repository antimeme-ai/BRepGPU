# Optional native progress trace, 2026-10-17

Plan docs/adaptive-model-progress-trace-plan.md: root and numerical peer ACCEPT
before body; numerical actual-source ACCEPT after body. No geometry, numerical
acceptance, source, kernel/CUDA ABI, allocator, Budget/Failure API or caps changed.
The boundary2epsilon hypothesis was falsified: prepare_affine_trace receives
selected UV plus captured actual Q endpoints, and its DIRECT carrier-trace-to-Q
upper+domain already must satisfy faceepsilon. No extra Ce error was charged.

Added boolean --trace-progress, defaultfalse, through trailing TrimFaceLimits
flag. One header-only synchronous diagnostic context is installed/restored by
RAII in each FaceTrim step. No pointer escapes its worker stack or reaches an
async task. Rows use retained identity/size/work counters only; no exact import
or geometry evaluation. Whole stderr rows serialized by std::osyncstream;
C++ formatting/I/O exceptions suppressed without replacing native exceptions
(this is not a promise against external OS signals).

Phase spans begin/end/failure retain originalfaceID,query,assignment,span,horizon,
revision,source/construction/pending/failed counts,anchors,leaves and work/peak.
No pending-poll row stream or retained log history. Region triangulate emits its
actual already-built N/B/component BEFORE calling CDT. Its paired end/failure
covers the wrapper through output-copy, not solely the core timer. Surface bound
subphase/unwind diagnostics copy effective worklimit before spent changes and
read local Budget counters before its destruction. Generic native errors remain
unchanged; root's failure-only Budget::charge GDB diagnosis needs no new API.

True old-operation RED: --trace-progress before another option returned1
`error options 0 unknown option`. A first placement at argv tail instead hit the
old generic missing-value check; only the actual unknown-option run is retained
as chosen RED in context/adaptive-watch/progress-trace-red.log.

Strict build GREEN. Focused existing model CPU1 port:12 original faces/180 trace
rows, balanced phase/CDT spans, IDs/counters checked; traceoff silent; traceon/off
both pass SAME original-source component/chain/facet/placement reader and all
native mesh/captured-boundary bytes agree. Tiny construction-work refusal has
identical original error with and without tracing and leaves no final/temp output.
Final log progress-trace-final-green.log; outputs progress-trace-final-ports.
No new geometry oracle, GPU numerical run, full replay or performance cohort.

Clang18/100column formatting precedes final strict build. A new private diagnostic
field placement triggered padding analysis; only those fields were regrouped in
existing scalar/bool runs. Final TrimFace/static PASS43 suppressed non-user
warnings; Surface PASS61 and model PASS58. Full Regions analysis retains3 existing
unchecked-optional warnings in cap metric/mapped code outside this diagnostic
change; no geometry edit or suppression was added. Scoped changed trace-line check PASS;58 warnings suppressed
(55 non-user,3 outside line filter), preserving the full report separately. Final strict build progress-trace-settled-build.log.

Stable tree build/adaptive-cap-construction-release handed free to root. Model
SHA216eabcb7228b0468e9b308eb72f1cfa58bd9d78197693dcc24fc2662f668d58.
Source SHA256:
- adaptive_trace.hpp:28873fef73b7b0c0a2f11002e0495f75b2c5ce54300a1a08fcfe925659d0c1c1
- adaptive_trimmed.hpp:be289f5c1f9dfb9f4957cc0be8a983ba9d6d77f5c1dfe1198a8b3f5df948bb05
- adaptive_trimmed.cpp:a83f18ba4c59fda06c512d1b378381a4850c6061a3bde87517548c4052f47a13
- adaptive_surface_cache.cpp:9a0e32612d57dd63fc92470492677443cd3446d85d65d53ba93789bbf514c35c
- adaptive_regions.cpp:82e1655c4e8922c32a4eb7feee42babc22e10902578970be9fa045396b911e13
- adaptive_model_main.cpp:8008f867e54e02f6bf8f5915e2c6f8d7e5dd7a83db0270ca7caf77d553fe5c24
- native_adaptive_model.py:4a8e805b82451893b9a880631d041b82ef57b7eaf3b984a9cca8b583637658f8

Fixed TSV columns (1based): native_trace,event,phase,originalface,query,assignment,
span,horizon,revision,source_sites,sites,facets,pending,failed,anchors,leaves,
cumulative_work,component,N,B,work,peak,byte_limit,work_limit,spent. Failed count is
from the last completed actual bound horizon; no invented classification of a
currently undecided batch. Root owns ONE selected-component diagnostic next.
