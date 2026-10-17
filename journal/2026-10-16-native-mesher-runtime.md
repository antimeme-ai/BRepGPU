# Native runtime ownership and integration

The accepted runtime plan requires owner steps that return at child/barrier
boundaries, rather than occupying workers while waiting for queued children.
The first integration connects immutable source/geometry and shared-boundary
evaluation batches. It will become the same coordinator that runs face owners,
bound children and complete definition epochs; it is not a second mesher.

Before admitting a parent with children, reserve their bounded storage against
the run. `ResourceReservation` charges its full cap once and supplies prepaid PMR
storage to a child's private Budget. Actual child allocations consume that cap
without charging the run twice. The lease retains its parent resource and must
outlive its PMR allocations or device/pinned storage. Failed admission releases
all accounting; a child never waits for more memory held by its parent.

Direct RED cases specify prepaid allocation, nested Budget refusal, rollback,
parent lifetime and simultaneous bounded admissions. Standard C++ allocation and
thread primitives introduce no third-party dependency. Tests and builds run on
the fleet. Subsequent scheduler checks will use typed production boundary tasks
and literal original-geometry expectations, including reorder, failure and stop.

Prepaid reservations observed missing-header RED before implementation, then
strict Release and ASan/UBSan GREEN. The numerical owner independently reviewed
the actual capacity accounting, constructor rollback and allocator-control-block
lifetime and accepted the seam. Full cap remains charged while idle; child
allocations are not counted a second time. No global exact arithmetic budget is
shared between workers.

The native owner scheduler now has explicit ready/running/completed entries and
bounded ready/completion rings. A coordinator alone resubmits yielded owners;
workers never wait for children or barriers. One owner cannot have two active
steps, even at different generations. Each worker has a prepaid child arena and
each step receives a fresh private Budget. Its optional stop token is checked by
the existing exact-work charge path, allowing cancellation inside arithmetic
instead of a shell timeout. Existing callers retain the no-token default.
Authoring the continuation test first produced actual missing-header RED.

The actual boundary producer also observed missing-header RED, then CPU1/CPU4
literal original-line answers fed the same shared split transaction. It retains
its source/geometry and sealed parameter lease; job-local exact values die before
return, and incomplete result prefixes cannot be read. Review found the original
batch needed its SourceStore/definition lease to reject a foreign GeometryStore
with coincident slot numbers. The forest now retains that authority and the
producer rejects foreign pairing before work begins.

Strict integration, ASan/UBSan and ThreadSanitizer pass the real boundary producer
and owner scheduler. The scheduler cases directly exercise complete barrier
horizons, exclusive owners, a full outstanding window, continuation resubmission,
running arithmetic cancellation and queued cancellation without execution.
The geometry owner independently accepted the scheduler's actual queue/lifetime/
stop implementation. Actual-source lint and named format pass; two unchecked
optional accesses in the fixture were replaced by explicit admission checks.
This completes the native executor/point-child unit, not the complete mesher.
