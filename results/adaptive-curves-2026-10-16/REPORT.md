# Complete cached shared-boundary operation

Original definition → shared physical edge chains + every incident occurrence
trace now works in owned C++/CUDA on CPU1, CPU4 and both P100s. The complete caller
passes **200 queries in80 processes across10 authored families**: polynomial,
strong positive weights, degree32, stationary nonlinear straight curve,
descending t/piecewise map, wire, C0/nonclamped knots, two faces sharing one
carrier/physical chain, and a cylinder seam using one edge twice. Fine/coarse/
fine/repeat constructions match across backends; fresh coarse output matches
warm coarse output. These are boundary snapshots, not complete face meshes or
the watch. [Interface and source contracts](../../docs/ADAPTIVE_SHARED_CURVES.md).

## Original-output truth

The single independent reader derives whole rational chord bounds from literal
original homogeneous coefficients and actual stored XYZ using Fraction
arithmetic. Exact affine plane residuals, analytic circle interpolation and
original map/incidence checks supplement it. Actual line scalar/covector payloads,
use traversal, source-node guards and distinct seam UVs are checked. Placement
parity is separate. The numerical service's [81-record reader](../adaptive-curves-geometry-2026-10-16/REPORT.md)
checks original point enclosures and whole chord/trace bounds on CPU/both P100s.

Source refusal/publication checks pass for fixed original vertex mismatch,
endpoint-invisible off-carrier bulge, resource exhaustion, unknown map, existing
output preservation and original units metadata. A collapsed original guide map
and a zero-range declared wire quotient have positive actual-output checks:
original endpoint aliases survive, with no fabricated positive physical interval.

The twelve current curve/trace CTest entries pass over the affected runs. Six
surface compatibility entries also pass after the shared CUDA/native owner
change, including the original complete patch caller. Selected host ASan/UBSan/
leak and TSan checks, real caller instrumentation, named source lint/format and
actual-source peer reviews pass. CUDA protocol memcheck/leak reports zero errors/
leaked bytes, and racecheck reports zero hazards. The later convex arithmetic
correction changed no device lifetime/memory mechanism and passes the actual
CPU/P100 source readers. No additional certification campaign or watch retry.

## Observed costs

One observation per case/backend on a shared fleet machine, with other scoped
qualification activity. Four-query process is fresh numerical/session setup,
quality .02/.08/.02/.02mm, actual output, close and process completion. The
original-node cap is1e-8mm throughout. Independent reader time is excluded from
these process measurements. The CTest caller's83.63s includes that exact reader.

| Family | CPU1 process | CPU4 process | P1000 process | P1001 process |
|---|---:|---:|---:|---:|
| Degree32 planar curve | 15.857ms | 18.075ms | 347.568ms | 374.365ms |
| Two faces, shared carrier/edges | 25.215ms | 24.569ms | 367.616ms | 356.278ms |
| Cylinder seam | 57.731ms | 60.381ms | 408.739ms | 401.492ms |

These outputs are small: the first two fine results have11 boundary-node rows /
7 physical intervals, while the seam has36 /33. **CPU wins these complete
operations.** Device open alone takes roughly256–294ms in these observations;
actual first/warm constructions and transfers remain separately recorded.
No large-batch or whole-mesher GPU speedup is established here.

The degree32 P100 operation nevertheless performs real work across the four
queries:30 original point evaluations,36 physical/trace bound tasks and3 paired
control restrictions; uploads37,748bytes and downloads6,872bytes. The all-analytic
seam has no rational control restrictions to perform:80 original point tasks and
119 bound tasks,89,592bytes uploaded /22,440bytes downloaded. Counts are cumulative
per retained session. Upload/download means host↔GPU traffic, not server network.
Source, query caps, inventories, work, memory, backend and quality are retained in
[queries.tsv](queries.tsv); [processes.tsv](processes.tsv) retains all80 process
observations and [stages.tsv](stages.tsv) the separate native wall scopes.

## Findings resolved and remaining dependency

The degree32 caller exposed interval-dependency inflation in a+t(b−a), producing
a2.13e-7 source enclosure against a valid1e-8 cap. The curve-only convex DAG fixes
that numerical mechanism without loosening tolerance or changing patch policy.
Original endpoint source checks moved before positional subdivision after a real
mismatch repeatedly split to depth64. The same check now covers wires. Requests
from faces sharing one carrier are united by source carrier, retaining all uses.
Exact threshold equality is accepted only after precise comparison, and its
proven exact upper remains exact in output. Journals retain the direct failures.

Next: connected trimmed-face regions, holes, side schedules and SAME actual-facet
bounds. Boundary authoritative-domain uppers must not be used as whole-face
interior maps. Read-only [construction](../../papers/native-adaptive-boundary-regions.md)
and [numerical](../../papers/native-adaptive-trimmed-facet-bounds.md) proposals
record those dependencies. No complete component/watch/alpha claim follows.
NVIDIA USD/BRep has not been run on these new scenarios; the existing post-watch
reference backfill requirement remains live.

The tested caller SHA256 is
`d079cc241ba4e97632573702f31cced607f01002688af1e6158d194b54f09be1`;
CUDA module SHA256 is
`196314e96c337d34689cdc22ef72cdb1d4fbdf42e353bd5527c4898eb05915e3`.
Implementation is the reviewed curve geometry/trace/owner/device/session commits
plus the caller/build integration accompanying this report. Clang18.1.3,
CUDA12.8.1, sm60; test fixtures are public generated original SourceStore values.
