# Adjacency design: runtime adversarial review

Reviewed docs/ADJACENCY_DESIGN.md on 2026-10-14 against current runtime ownership,
ABI admission conventions, CUDA 12.8.1 stream semantics and the scan study. No
product execution occurred. The proposed dedicated synchronous ABI5 library is
appropriate: it avoids expanding the existing Engine resource graph and keeps
retained sessions out of the broad-pass work.

## Findings

**ADR1 — C=0 does not imply E=0.** The design admits nonzero edge count with zero
coedges but later describes empty input as returning the single zero offset.
Required disposition: E=0 emits one zero; C=0/E>0 emits E+1 zero offsets and E
unused kinds. Do not launch count/fill kernels with zero blocks. The reader may
still produce unused kinds, or equivalent initialization may do so. Test both
cases, including E near a scan tile boundary.

**ADR2 — diagnostic publication must follow pointer/alias admission.** The design
requires diagnostic, descriptor, timings and payload buffers to be nonoverlapping.
Writing a default diagnostic before admitting these ranges can modify an input
payload when the diagnostic pointer aliases it. Required disposition: read only
the minimal structurally admitted input descriptor, validate known pointer
extents/alignment and alias constraints, then permit diagnostic writes. When the
diagnostic range itself is inadmissible, return the status without writing that
buffer. ABI-info query and operation entry points need separate explicit rules.
No pointer-range checker proves that arbitrary native addresses are mapped;
valid/stable memory remains a caller obligation.

**ADR3 — cleanup can fail.** The design says all resources are reclaimed on all
paths while also admitting cleanup failures. Required disposition: the operation
attempts synchronization, each owned free, stream/event destruction and device
restoration on all relevant paths, retaining the first failure and noting
secondary failures. A failed CUDA free or a poisoned context cannot support an
unconditional no-leak guarantee. Successful paths and injected recoverable
failures should have direct resource qualification; cleanup failure prevents
publication and is reported without pretending reclamation succeeded.

## Accepted design points and implementation pressure

Count/scan/fill has O(E+C) work under bounded dimensions. Recursive block sums
reduce geometrically; there is no CPU scan readback. With C<=2^22, all prefix and
count intermediates fit uint32. Check lengths and byte products in uint64/size_t
before narrowing. Padding adds a fixed tile-factor amount of work. Every padded
lane reaches every barrier. No inter-block polling or implicit grid barrier is
needed, avoiding a new forward-progress contract.

Atomic fill does not order rows. The explicit unspecified-order contract is
sound because membership retains original coedge slots and the reader's
classification/mate mappings do not depend on order. For degree two, each edge
reader owns exactly those two coedge destinations, so separate readers cannot
race. Degree >2 must not inspect only two members and emit a mate. All unpaired
mate/neighbor values must be initialized even when the reader skips a row.

Use one stream, check each launch error, and complete outstanding operations
before freeing scratch or borrowed host inputs. Page-locked host memory is not
assumed; asynchronous-copy API calls do not imply useful overlap. Private-stream
host phase timing may synchronize each phase, so report it as measured instrumented
host spans and separately retain full native/caller wall time. Do not call it
kernel-only or subtract overlapping phase figures from the full wall.

CPU24 launch failure must join every already-launched worker; exception cleanup
must not let joinable thread destructors terminate the process. Worker functions
should not throw, and each phase joins before any shared buffer is repurposed.
Serial CPU prefix scan is honestly documented, not disguised as all-parallel.

Keep preflight output-write refusal distinct from runtime failure: native private
destinations may be partially written after a GPU failure, but the Python wrapper
publishes no result. Shape/ABI sizes must be checked before native reads. Python
source validation and retained immutable lineage complement slot-only native
validation rather than being recertified by CPU/GPU parity.

The separate build target avoids changes to v1-v4 factories and protected
qualification binaries. Old binaries are not used as a source-version certificate
for the new operation. Warm repeated stateless calls still include allocation,
upload, construction, reader, download and destruction; reserve the word resident
for an operation that actually retains device topology.

Recommendation: accept after ADR1–ADR3 have explicit dispositions. Subsequent plan
review should name the concrete structural-admission order, empty cases, partial
worker launch cleanup and CUDA error/restore ownership. No dependency adoption,
sort stage, general lifetime framework or extra certification layer is required.
