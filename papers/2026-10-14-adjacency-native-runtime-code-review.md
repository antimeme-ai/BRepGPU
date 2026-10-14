# Native adjacency runtime code review

Root reviewed the standalone header, topology.cpp, topology_cuda.cu, internal
operation/cleanup helpers and native checks against the accepted design/plan.

NR1 valid: the first CPU1 implementation used atomic increment for serial count
and fill, imposing an avoidable penalty on the fair CPU baseline and diverging
from the direct-counter design. Corrected before fleet green: CPU1 owns direct
uint32 counters; CPU24 owns atomic counters with bounded worker groups. Partial
thread-launch failure joins every launched worker before buffer cleanup.

Checked structural admission before descriptor/input accesses and diagnostics:
all provided pointer extents/alignment and pairwise output aliases, exact payload
counts, uint64 byte overflow, reserved uint32 range and +/-1 signs. Diagnostic
is unwritten on unsafe structural aliases. Accepted per-call inputs stay borrowed
only synchronously. Runtime failure may partially modify output, while timings
are withheld and the Python wrapper discards every destination.

The recursive 256-lane scan reaches every barrier with zero-padded lanes, scans
block totals on device and adds their exclusive prefixes. Count totals remain
at most admitted C<=2^22. Final offset is written on device; no unreported host
sentinel copy is added. Fill atomics reserve every occurrence once, and the
reader runs on the same stream only after fill. Each degree-two opposed row
writes its two distinct coedges and mate faces; higher degree preserves members
and no unique mates. Sentinels cover boundary/conflict/multiple-use cases.

Checked allocation ledger capacity against the admitted hierarchy, immediate
tracking of successful allocations, synchronization before cleanup, attempted
free/destroy/device restoration and primary error retention with bounded
secondary diagnostics. Cleanup-report injections happen after actual cleanup;
reports must distinguish those from observed driver failures. Requested a direct
previous-device observation in tooling to substantiate restoration. Native/host
and both-device sanitizer results remain a separate required qualification.

Phase clocks synchronize actual CUDA stages and include launch/sync overhead;
they describe the instrumented synchronous operation, not isolated kernel time.
CPU scan remains serial and worker launches are included. No general CAD topology
editing or Boolean identity construction is claimed. NR1 is resolved; a second
independent topology review and memory/fault qualification follow these greens.
