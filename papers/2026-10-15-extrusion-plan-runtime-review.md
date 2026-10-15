# Extrusion implementation plan: runtime/sampler review

Reviewed complete `docs/EXTRUSION_PLAN.md` against the accepted design and actual
existing native/Patch/Boolean consumer helpers. **Accepted; no blocker.** No code
or execution. Implementation remains behind all-owner/root release and direct red.

The ownership units preserve whole profile/solid/history and independent truth,
with a separate native request/sampling consumer. Public ExtrusionRequest and
ExtrusionSamples fields match the existing type-agnostic helper requirements;
independent result/request guards keep Boolean/canonical APIs unchanged. Exact
input-grid validation precedes float control creation; actual candidate float
copy precedes trim labels. Original per-Patch frames, output Lineage, descriptors,
slots/UV bits, backend/count/order/statuses bind before rows are consumed.

The 1,048,576 total candidate cap handles34 charts without inheriting the old
ten-chart admission. Grid products are checked before generation, iterables stop
at cap+1, and empty/rejected rows are explicit. Borrowed Engine/per-call native
child ownership is clear; actual corrupted-descriptor rejection, late host failure
closing all three real handles and same-result/Engine healthy retry directly test
the new integration rather than crediting arbitrary exceptions. Optional native
profiling fields must retain their known flags/NaN behavior through reused checks.

The real caller distinguishes profile preparation, useful prepared-profile→solid/
reader work and complete input-points→consumed work. Both useful walls exclude
independent qualification/export/reference/startup explicitly; full child-process
wall remains available. Construction publication and native/topology setup and
cleanup are included. Genuine distinct profile sizes, fresh contexts and nested
nonadditive phases prevent scale-only or isolated-kernel speedup framing.

The optional installed reference uses pure copied-input linear_sweep semantics,
actual lower-plane endpoints and world sweep distance. Published dyadic recipes
must pass exact-world-endpoint representability before foreign construction.
Finite complete bounds and source-edge/face-interior witnesses are explicit;
foreign history/material and external alpha gaps stay separate. No dependency,
build, generalized adapter, native sanitizer rebuild or tuning lane is needed.
The plan is ready for coordinated test-first implementation.
