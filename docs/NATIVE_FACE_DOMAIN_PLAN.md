# Native face-domain implementation plan

Implements the [design](NATIVE_FACE_DOMAIN_DESIGN.md) after independent design
and plan review. No product/test code precedes acceptance. Root owns branches,
fleet checkout/build profiles, integration, commits and final delivery.

1. Review exact arithmetic, ABI/lifetime, occurrence identity, hole bridge
   behavior, deterministic crossing/ear ordering, budget statuses and measured
   performance denominator. Resolve findings in design and journal before code.
2. Write focused specification/model tests and a fleet case runner against the
   proposed C ABI and thin binding. Literal expected turns, crossings and exact
   source-boundary output are the oracle. Capture actual fleet RED caused by
   missing native capability; absence/build failure is only the initial seam
   RED, not evidence of numeric or topology fault sensitivity. Preserve original
   reference fixture bytes/license and extract no upstream implementation.
3. Implement minimal native exact integers, safe C decode/lifetime and binding
   normalization. Exercise direct signs/order against Python Fraction, malformed
   offsets/limbs/IDs and forced budget exhaustion. Bounded seeded arithmetic
   comparisons attack carry/borrow/cancellation; hand-derived extrema identify
   the expected carry chain. Establish focused green before geometry indexing.
4. Implement exact rank compression, closed BVH and first crossing. Compare the
   complete first tuple with the old model across authored/external boundaries,
   including touches and exclusions; direct sparse scaling counts establish the
   candidate reduction. Forced exhausted traversal returns a resource error.
5. Implement strict domain validation and exact strict-convex/no-hole cursor
   fast path, then exact bridge-distance ranking, indexed
   visibility and unchanged occurrence ear cursor with indexed blockers. Check
   model equality on no-hole/prior-exact-order cases; for declared bridge-ranking
   changes use independent exact domain/embedding/source-node checks. Preserve
   duplicate bridge occurrences and original source IDs. Do not add Lawson or
   refinement to this conceptual unit.
6. Run release behavior, focused ASan/UBSan malformed/reuse/lifetime cases, the
   actual compiled-unit/header static checks provided by root, then an independent
   adversarial implementation review. TSan is deferred until this unit actually
   contains shared concurrent mutation. Fix every valid finding; record dismissed
   findings with concrete code/oracle reasoning. No mutation/certification job.
7. Run optimized single-core fleet trials on authored scaling, selected external
   cases and actual8300-node face after reproducing/capturing its full snapshot
   (only live counts/source metadata exist today). Report normalization and whole native
   call, stage walls, actual work, exact bit width, output size and peak RSS;
   counters and timing configuration match across ratios. Baseline timeout300s;
   native practical budget300s/2GiB. Sparse strip crossing-only node visits plus leaf box tests <64*N at8192.
   Three fresh trials where affordable; no full-watch retry to discover scaling.
8. Return code + focused tests + actual correctness/performance report to root.
   Root selects the separately reviewed production integration and complete-part
   oracle pass. Preserve old live watch job/pinned sources until root decides its
   disposition. Do not edit imported_meshing.py, imported_oracles.py, production
   source/helper binaries or qualified display/CUDA modules in this unit.

Owned files after code GO: `native/face_domain/*`,
`python/brepgpu/face_domain.py`, new focused `tests/test_face_domain.py` and
`scripts/benchmark_face_domain.py`, plus this design/plan and unit journal.
Coordinate exact target/CMake/test naming with root before editing shared build
files. New persistent formats are owned binary values; TSV is external reporting.

Root and independent algorithm review ACCEPT the amended design/plan. The
operator authorized pursuing the fixes and C++ transition. Proceed with the
focused fleet RED before implementation; root owns checkpoint commits.
