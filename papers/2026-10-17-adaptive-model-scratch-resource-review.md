# Explicit model scratch policy: resource review

Scoped ACCEPT of docs/adaptive-model-scratch-policy-plan.md after reading
RunResource, ResourceReservation, JobContext/Budget, NativeScheduler, the model
caller and the actual adaptive curve/surface/face child-context construction.
No build, geometry execution or resource-policy implementation by this lane.

RunResource counts reserved/allocation blocks through atomic acquire/release;
the limit is the shared run accounting cap, not a measured process RSS bound.
ResourceReservation acquires full prepaid capacity from that same run, retains
its owner and requires its allocations to die before release. Scheduler workers
construct owner-affine Budgets on their prepaid reservation. A larger child
Budget therefore does not bypass global admission.

The model caller has an external scheduler and, when physical cpu_threads>1,
the boundary session has another scheduler even on a P100 run. Twelve workers
and 256 MiB scratch prepay 3 GiB in each pool. Face/Surface nested child contexts
instead allocate directly through SAME RunResource, so their actual allocations
can coexist with conservative prepaid worker capacity. Source/cache/output also
consume that same capacity. Global refusal remains necessary; scratch<=memory
alone does not prove every requested concurrent operation fits.

The literal plan preserves the existing unspecified effective
min(memory_mib,64), validates explicit nonzero/shift/global bounds without
clamping, and rejects impossible combined mandatory reservations by division
before output construction. The one effective value flows to existing surface,
physical and scheduler child limits. Metadata distinguishes default and explicit
policy. No work-counter reset, new allocator, device-cap substitution, tolerance
change, source change or topology change is selected. Existing worker draining,
exception capture and resource release stay in place.

256 MiB under a 32 GiB run cap is a coherent bounded caller configuration, not
a guarantee that the remaining imported component completes. The direct parser,
tiny-global refusal and actual small-model byte-identity ports in the plan
exercise resource admission and unchanged geometry using existing readers.

Actual adaptive_model_main.cpp and focused native_adaptive_model.py source
reread: ACCEPT. Explicit validation follows validated memory/thread bounds and
precedes run/OutputDirectory. Division tests combined mandatory pool width
without multiplying potentially overflowing MiB values. The effective value
flows to surface, physical and outer scheduler child limits; source/run/device
ownership is unchanged. Metadata/help record policy. The existing driver checks
actual implicit/256 mesh and boundary byte identity, exact effective metadata,
parser refusal without output, and a valid tiny-global resource refusal. No
ownership or numerical finding, and no run from this review lane.
