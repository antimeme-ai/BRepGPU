# Native scheduler ownership and cancellation review

Scoped independent source review of `native/mesher/scheduler.hpp/.cpp`,
`reservations.hpp` and the Budget stop-token seam. **ACCEPT**, no blocking
finding. This review adds no test campaign or separate acceptance layer.

The bounded outstanding-entry table accounts for queued, running and untaken
completions together. Each entry contributes at most one ring item, so neither
ready nor completion rings can overflow through admitted submission. Original
owner identity stays excluded until the coordinator takes its completion.
Workers publish every result under the mutex before completion notification;
ready and completion predicates use separate condition variables.

Stop prevents new admission and wakes idle workers. Already queued entries are
dequeued, but initial `Budget::charge(0)` observes cancellation before their
step. Errors become keyed failed completions. Destructor order stops, drains
ready/running work and joins workers before Impl/resource destruction. The
partial thread-construction failure path likewise stops/notifies/joins the
threads already started before rethrowing.

Per-worker private Budget allocation uses a prepaid retained reservation, while
queue/control blocks use the RunResource. A task must not retain references or
Rational values allocated by the temporary JobContext after its step returns;
the header states this contract and concrete producer steps need to respect it.
Retained values must use owner arenas or sealed run-owned tables. Work/peak in
a completion are per step, so the coordinator must accumulate work across owner
continuations to enforce/report a whole Run allowance. No claim of correctness
for arbitrary user callbacks follows from the executor's queue invariants.
