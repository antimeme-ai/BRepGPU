# Discovery CUDA mode specialization study

- Read the current CUDA kernel/callback/ownership/profiling path, shared bound
  and traversal math, CPU range, numerical flags, direct qualification corpus
  and public mode/invalid/tail tests at main 1d0762d.
- Proposed a CUDA-only mode-template kernel and immutable-hierarchy host dispatch,
  keeping shared arithmetic, CPU binary, ABI, cache format, allocation/lifetime,
  stream/device handling and profiling scopes unchanged. No speedup inferred
  from the dynamic kernel's reported 254 registers / 448-byte stack / zero spills.
- Identified removable mode-specific paths and common remaining projector/DFS/
  interval scratch. Current mode is launch-uniform, so this is not a proposed
  fix for per-query divergence or unresolved geometry.
- Saved papers/2026-10-14-discovery-cuda-mode-specialization-design.md with exact
  cross-mode invalid/empty/degree-zero/cap/refinement/profile qualification and
  controlled old/new plugin measurements using preserved CPU/core/benchmark.
- No product/performance-source edits, builds, target execution, deployment,
  dependencies or Git mutations. Current CPU1 measurements remain undisturbed;
  any implementation follows design/plan disposition.
