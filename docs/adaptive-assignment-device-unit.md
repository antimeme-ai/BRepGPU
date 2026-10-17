# Retire mutable CUDA face pages without reopening the source bank

2026-10-17. Runtime peer identified actual pinned validity arrays in
adaptive_cuda.cu::DeviceSession/create/samples/restrictions/control_range. Clearing
only native arrays would leave old point/child-control residency admissible.
The component design therefore needs one real module retirement operation on
the SAME surface owner. Existing base, curve and trim ABI layouts stay unchanged.

Select a separate AssignmentCudaAPI v1 extension with abi,size and
retire(DeviceSession*,expected_assignment,next_assignment,DeviceError*).
Its initial assignment is0. A surface owner must be healthy, have no outstanding
operation, match expected_assignment and advance to a strictly larger epoch.
Validate before mutation, synchronize its stream at this assignment barrier,
charge the actual validity bytes against the existing work cap, then clear all
point-valid flags and child-control-valid flags beyond the immutable source
control prefix. Retain original root/control bytes, all allocated capacities,
stream/event, cumulative stats, ticket monotonicity and native reservation leases.
The module must reject a restriction destination overlapping that source prefix.
This is owned lifetime/immutability reasoning, not a numerical theorem attributed
to a paper. Native captured-view assignment checks remain a separate responsibility.

Extend the EXISTING native_adaptive_device fixture and its current independent
protocol model, rather than adding a second device tester. Direct old-module RED
is the absent extension. Check stale/nonmonotone and busy retire refusal; complete
the actual outstanding work; retire successfully; demand stale point/child reads
refuse but original controls still restrict and original-source points still
evaluate. Verify exact authored plane XYZ after overwrite/repopulation. Repeat a
retirement then close and require reservation release. Existing device test's
prefix/scan/refusal/drain checks remain intact. Run both P100s and one scoped
memcheck, not the full numerical campaign. Root owns this extension/test/header;
runtime owner owns native assignment/page retirement and the real face fixture.
