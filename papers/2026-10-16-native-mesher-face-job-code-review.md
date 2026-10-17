# Face owner scheduler wrapper review

Scoped independent actual-source review of native/mesher/face_job.hpp/.cpp and
the consumed FaceEngine/frontier completion semantics. No additional execution
was requested; root owns strict CPU1/CPU4 and sanitizer/runtime validation.

The retained face Budget/GeometryContext and reservation outlive engine state
and staged topology edits across child yields. Immutable child packets retain
their own source/table leases. Complete result count, duplicate rejection and
full-header matching establish a bijection before resumption. Result arrival
order only updates named frontier rows; semantic selection remains the existing
key-ordered frontier. Stale or failing consumed input makes the owner unusable.
Settled contact processing and explicit finish preserve the definition horizon.
Owner work deltas are separately consumed once; scheduler scratch is separate.

Finding: a default empty constructor stop token left a valid-looking submitted
owner's persistent Budget disconnected from scheduler cancellation; step does
not use JobContext scratch for this arithmetic. Root removed the default and
documented that callers supply the submitting scheduler's token. The existing
real scheduler fixture already supplies it. A separately stopped private token
exercises owner cancellation intentionally. No second cancellation mechanism
was introduced.

ACCEPT with that API correction. This review covers scheduling, lifetime and
state transfer, not correctness of authored test geometry or unfinished face
domain/correspondence production.
