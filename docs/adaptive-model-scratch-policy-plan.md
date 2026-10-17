# Explicit native model scratch policy

2026-10-17. Narrow caller-only plan; no geometry/core/predicate/cap-source change.
The actual v8 remaining component still exhausts its64MiB child Budget at a
Quad-vector replacement (live64,918,076, peak65,770,300, max28limbs), after932.83s.
Lifetime phasing remains valid; it did not establish that this graph fits64MiB.
An explicit caller resource allowance is now selected rather than another blind
long replay or a hidden library-default increase.

Own adaptive_model_main.cpp, narrow existing native_adaptive_model.py policy
ports, this plan/journal. Add optional uint64 --scratch-mib. Unspecified preserves
exact old effective min(memory_mib,64). Explicit must be positive, representable
before shifting20bits, and <=memory_mib; never clamp an explicit value. For explicit
options reject impossible mandatory scheduler reservations by division before
OutputDirectory: threads*S for the external scheduler, plus threads*S for the
physical-curve scheduler when threads>1. Implicit options retain prior validation
and service-stage resource refusals. One effective bytes value sets surface and
curve child_bytes and external SchedulerLimits.child_bytes. Metadata records
memory_mib, effective scratch_mib/bytes and explicit-versus-default policy; help
names the option. No new owner or allocator.

Actual backing proof: resources.hpp RunResource::acquire/do_allocate enforces an
atomic run-wide limit. reservations.hpp ResourceReservation prepays its entire
capacity through acquire and its metered control block retains the resource.
scheduler.cpp Impl39–43 prepays every worker; workers' JobContext Budget uses that
reservation. AdaptiveBoundarySession builds a second scheduler for cpu_threads>1.
FaceTrimSession::step and SurfaceCache::step_facets/caps instead use child-sized
Budgets directly backed by the SAME RunResource; these allocations also count.
Surface cpu_threads=1 means no extra per-face worker pool. Thus12workers×256MiB
means3GiB prepaid externally and3GiB for curve workers, plus globally charged
live nested scratch/source/cache/output.32GiB remains hard;256MiB is a deliberate
bounded run configuration, not a proof that every input or concurrency fits.
Source setup can still consume the remaining capacity and refuse normally.

Before body capture genuine existing CLI RED: same authored two-definition model
operation with --scratch-mib256 returns unknown-option. Extend only existing
model driver with a focused policy port: three parser refusal classes (zero,
above-global including overflow, impossible mandatory reservations; unsigned
negative remains rejected), no final/temp output; finite actual source operation
with tiny global capacity still refuses resource exhaustion. Then SAME existing
original-source component/scene structural reader checks one small CPU mesh-only
assembly at implicit policy and explicit256, and complete native mesh/boundary
bytes must agree. Record metadata exact policy. No new geometry oracle, display
run, source search, GPU numerical cohort, sanitizer or performance campaign.
Strict build, one product clang-tidy18 and root/peer actual-source review close
this caller unit. Announce stable tree; root owns the next full CPU/P100 runs at
explicit256 under unchanged32GiB global cap, epsilon/delta/source unchanged.
