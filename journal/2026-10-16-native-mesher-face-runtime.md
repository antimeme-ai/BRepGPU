# Native face owner continuations

The accepted whole-mesher runtime gives each face generation one persistent,
single-writer exact arena and the same FaceEngine. This step connects that owner
to NativeScheduler: sealed cell/candidate requests cross child boundaries;
complete matching answers resume the parent; boundary and contact horizons yield
to the definition coordinator. No worker waits for another queued worker.

The arena reservation precedes admission and outlives the Budget, geometry
context and topology. A fresh scheduler step Budget does not own retained face
values. Work accounting reports the persistent owner's delta separately from
scheduler scratch. The coordinator may transfer answers only while the task is
not submitted; the scheduler's existing owner exclusion enforces execution.

Direct checks exercise actual original geometry, complete child packet identity,
one-thread/four-thread continuations, a real refinement candidate, stale/partial
answer rejection and cooperative cancellation. Geometry-owned feature protection
and the whole definition coordinator remain required product connections; the
authored interior fixture cannot substitute for either.

The direct executable first failed to link against the absent FaceOwnerJob.
Implementation then passed CPU1/CPU4 in the strict profile, ASan/UBSan with leak
checks and ThreadSanitizer. The first fixture incorrectly treated the scheduler's
nonblocking take as a wait; it now drains the current bounded step wave before
taking its completion. Actual-source lint passes after replacing unchecked
optional fixture accesses with an explicit completion admission helper.

Geometry's actual-source peer review accepted the ownership and complete-reply
seam and found the constructor's default empty stop token could disconnect the
persistent Budget from scheduler cancellation. The default is removed; callers
must supply the submitting scheduler token. No second cancellation mechanism was
introduced. Persistent exact work is reported independently of fresh scheduler
scratch and folded once per completed step.
