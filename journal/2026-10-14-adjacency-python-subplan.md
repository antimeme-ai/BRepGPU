# Prepared adjacency interface subplan

Root owns preparation, source mapping, synchronous ctypes invocation and immutable
result access as a complete interface unit. Purpose: connect existing immutable
body records to a pointer-free native constructor, retain source identity and
let callers consume actual mate/neighbor outputs. Input validation is once at
preparation, while native memory/slot admission remains per call. No JSON or new
library is used; frozen Python records and owned little-endian bytes carry data.

Source fields are instance/body/revision/entity, with typed containing tables.
The input enforces uint64 validity, uniqueness and per-instance lineage, freezes
numeric inputs as exact packed bytes, and retains original snapshot occurrences
when created from bodies. Repeated snapshots get distinct occurrence ordinals.
A storage permutation changes slots and references together, preserving keys and
original provenance. Caller lookup reads actual binary outputs and returns typed
keys; it never re-derives mates from the input to conceal a native reader fault.

The wrapper loads a separate topology library, verifies ABI5 layouts, allocates
private ctypes destinations and keeps borrowed input buffers alive throughout
the synchronous call. Failure raises with native diagnostics and discards every
partial destination. Successful output is immutable owned bytes with timings
including wrapper allocation/native call/publication; original sources remain
alive. CPU/P100 inputs and the original four ABI libraries are independent.

Implementation waits for accepted plan reviews and the direct fleet cylinder
red. Direct greens precede full independent model qualification and interface
adversarial review. Root runs all product execution only on the fleet.
