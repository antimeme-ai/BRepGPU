# Deferred GPU lifetime reuse study

Read current owned Engine/plugin/context/stream/buffer creation and teardown
source, pair caller timer boundaries and actual 39/79 ms Engine cleanup records.
Read pinned CUDA 12.8.1 primary docs on runtime primary contexts, lazy loading,
stream drain, frees and stream-ordered pool capability/threshold. Distinguished
owned stream/context lifetime from CUDA shared primary contexts, and identified
unload/module teardown as unmeasured rather than an assumed cause.

Saved explicit long-lived per-device session/resident-input, plugin ownership
and bounded leased-buffer options plus one staged attribution experiment in
papers/2026-10-14-gpu-lifetime-reuse-study.md. User explicitly deferred all such
integration, pooling and new measurements until after the broad pass big rocks.
No product code, optimization lane, native/fleet execution, new agents,
dependency adoption or Git mutation.
