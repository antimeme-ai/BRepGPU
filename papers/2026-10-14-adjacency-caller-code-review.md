# Adjacency actual caller and benchmark code review

Independently reviewed `examples/build_adjacency.py` and
`benchmarks/adjacency/run.py` against accepted design/plan, wrapper/oracle and
actual CUDA transfer sites. Accepted on static review, no blocking finding.
No caller execution or measured result is claimed by this review.

## Actual consumption and independent truth

The caller constructs actual owned snapshots, packs their records and globally
permutes edge/face/coedge slots when requested. Repeated snapshots remain
distinct source occurrences, not a connected newly constructed solid.
`consume` reads every actual native edge kind and CSR occurrence, then every
emitted mate and neighbor through the public typed readers. Its source sum
is explicitly a consumption marker, not geometry or relation truth.

`build_and_consume` measures native/wrapper build and the complete typed reader.
The external combined wall includes both and the helper return bookkeeping.
Each initial/warm/repeated output is independently qualified afterward from
raw arrays. Literal source projection runs on the initial output; subsequent
generic qualifications check complete incidence and mate relations on unchanged
prepared immutable input without repeatedly reproving primitive geometry.
Result/input object binding and full consumption counts are retained. Qualified
outputs do not come solely from a checksum, status code or CPU/GPU equality.

## Publication and topology provenance

The actual caller exports all three submitted slot/sign arrays and all five
actual returned CSR/reader arrays, complete typed edge/face/coedge source TSVs,
actual coedge memberships, reader slots and typed mate/neighbor identities.
Original loop order, hole designation and UV occurrence maps are resolved by
original body instance/coedge ID, preserving provenance after permutations.
The incidence SVG reads actual downloaded memberships and reader answers;
same-face pairs receive a distinct color. Placement is diagrammatic and the
footer explicitly disclaims a geometric embedding interpretation.

Caller wall includes preparation/load/build/typed consumption, independent
qualification, raw/table/SVG export and manifest comparison. Directory creation
precedes that clock; final timing TSV/console publication follows it, explicitly
named in source. An export-only change after measurements would alter source
pins and require clear campaign/checkpoint accounting rather than silently
claiming the newer caller source produced older data.

## Cost scopes, sizes and exact metadata

The controller serially launches 24 fresh-process cases: mixed1, plate512 and
plate32768; ordered and globally permuted; CPU1/CPU24/P1000/P1001. The larger
case contains491,520 edges/229,376 faces/983,040 coedges. Each worker records
preparation/packing/permutation and load separately, then cold first build/
reader and cold-to-first-consumer including device initialization. Python
process/import startup is outside that narrower cold clock; results must not
describe it as full fresh-process wall.

One warmup and five repeats consume every result, with all independent
qualification outside each measured combined wall. Independent cost is itself
recorded. Repeats reuse host input/library/runtime only; native allocations,
uploads, construction, reader, readback and cleanup happen per call. Native
host phase spans are not kernel-only events. Wrapper publication's narrower
scope and full wrapper/build/typed-reader clocks remain separately named.

Controller admission compares every input metadata field against independently
written expected count/byte formulas, rejects duplicates, unexpected/missing
phases, bad iteration IDs and nonfinite/negative durations. Each setup phase
must occur once and every repeated phase five times. Median/min/max data retain
spread; separate native and complete-build-reader SVGs avoid confusing either
scope. Small mixed counts30/16/60 and both large plate sizes match declared
primitive counts. The independent source sum is schedule/order-insensitive,
while arbitrary within-row order is not imposed as equality.

Slot/sign upload is12C bytes. Actual returned offsets(E+1), kinds(E), and
members/mates/neighbors(3C) give8E+12C+4 readback bytes. Checked these formulas
against actual CUDA memcpy sites. Typed source/geometry/UV records stay CPU-side
and contribute zero extra DMA. The formulas refer to host↔device transfer,
not network traffic; host payload sizes are separately labelled for CPU.

## Source, binary and environment controls

The manifest pins actual caller, benchmark, wrapper, independent oracle, body
records, headers, CMake and all included topology native source/hook files;
the separate used topology shared library is hashed. Controller checks these
before and after every worker plus final campaign, while the caller checks
before/after its own operation. The used old numerical libraries are not loaded
by this topology-only benchmark; they need no fictitious measured-work pin.

Environment export records hardware/software/build variables while omitting
invocation paths and private operational logs. Worker stdout/stderr are retained
only in ignored context. Source hashing/environment export sit outside measured
worker spans. Root must still serialize fleet workloads externally: the code
does not claim an automatic whole-fleet exclusion lock. No performance tuning,
new dependencies or alternate native lifetime scheme was introduced here.

Next review is of actual serialized artifacts and measured raw/summary results,
after fleet qualification and campaign execution. This static review does not
certify missing future execution, memory checks or speedup.
