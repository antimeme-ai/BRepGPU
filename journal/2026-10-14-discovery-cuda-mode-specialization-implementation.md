# CUDA discovery compile-time mode delta

- Coordinator reported actual pre-change dynamic qualification on both P100s:
  three focused tests passed per card; each capture contains 309 corpus rows
  and 132 total batches. Actual preserved dynamic plugin SHA-256:
  8a6a861c17c7c2c22a0fafe40ce8cc2b46b33272322c8b48af9ce5af24d27efe.
  Scalar baseline queue finished before coordinator cleared product work.
- Changed only native/cuda_backend.cu: discovery_kernel<Mode> passes its unsigned
  constant into the unchanged shared discover_local; the kernel no longer has a
  runtime mode parameter. One private templated host helper packs the unchanged
  patch/hierarchy/query/result arguments and all six slabs for the three launches.
- Validated the retained hierarchy mode after context/count admission, before
  pending-result construction/allocation and the empty guard. A defensive switch
  default records an evaluation error and follows normal stream/event/device
  cleanup; it cannot publish a result or fall back to AABB.
- Retained 128 threads, grid cap 4,096, full-count grid-stride SoA indexing,
  numerical helper source, control offsets, branch precedence, allocation,
  profile event locations, launch-error collection, synchronization,
  restoration and publication. Empty admitted calls still skip events/launch.
- Source owner checked the complete argument mapping and diff. Shared math,
  CPU/core source, CMake, adapter, native benchmark and clients are unchanged.
  No local native execution, build, fleet job, deployment or commit. Independent
  source review and coordinator-only CUDA build/qualification follow; emitted
  resources, raw identity and performance outcomes remain unknown here.
- Independent numerical source review accepted CUDA source SHA-256
  0823748fbec55d21f8b3c76be06d101edaa0b3a8082b08d59a5af2a2488447bd,
  with no findings. Argument/extent/slab mapping, unchanged shared arithmetic,
  traversal counters and early validation/cleanup scopes were checked. Target
  compile, sanitizer, exact corpus, raw comparison and timing remain pending.
