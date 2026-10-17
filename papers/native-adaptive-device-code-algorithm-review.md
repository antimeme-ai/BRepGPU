# Resident adaptive CUDA module: scoped source review

Read actual `native/mesher/cuda/adaptive_cuda.cu` and `adaptive_api.h` after the
body was written, before initial device GREEN. No new geometry suite was
introduced. Module implementation/build closure belongs to root.

The warp-shared 64-coefficient net fits the admitted bicubic-or-lower homogeneous
shape; all active lanes participate in residual reduction, including lanes with
no coefficient. Warp-uniform family/tail exits preserve synchronization. The
recursive scan uses disjoint level ranges, and the checked frontier cap keeps
child-prefix doubling in uint32. Same-stream launches order point/control writes,
node installation, numerical bounds, marking, scan, compaction and readback.
Parent-read/destination-disjoint range admission prevents restriction batch races.
Sample destinations and node slots are unique before launches.

Pending allocation, old retired allocation and external reservation identities
are registered before CUDA acquisitions/free attempts. Failed frees retain the
pointer/token; destroy drains before releasing buffers, then event/stream, owner
and callback lease. Borrowed operation inputs are copied into owned pinned
staging before asynchronous use. Event-ready is required before result copying;
wrong tickets/capacities refuse without consuming the operation.

Concrete findings sent to root and resolved in a narrow actual-source reread:

1. Initial owner bytes must fit host_bytes, and grow must check live<=cap before
   unsigned cap-live; otherwise an undersized cap underflows.
2. Both root sets must admit known families and require rational controls.count
   exactly (degree_u+1)*(degree_v+1)*4 before sample kernels can read a net.
3. A published owner from a failed create must remain unavailable for operations
   until initialization succeeds; destroy still owns/drains it.
4. Finite tolerance_down zero must be allowed for a positive exact Rational
   allowance below binary64 minimum subnormal; native precise threshold handling
   retains the original positive allowance.

Current source rejects an undersized host cap before owner allocation, guards
live<=cap before subtraction, checks known families and both rational root shapes,
marks the owner broken until full successful creation, and accepts finite
nonnegative downward thresholds. All four findings are closed by source reread.
Scoped source ACCEPT, with actual nvcc/direct-device/integrated numerical GREEN
still owned by the module and geometry/runtime lanes. These are finite
API/ownership checks, not a replacement source geometry evaluator. No new test
campaign or claim of whole-watch completion follows from this review.
