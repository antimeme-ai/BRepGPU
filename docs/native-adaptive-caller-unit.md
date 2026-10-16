# Complete native patch caller

Purpose: make the first adaptive cache operation concrete and inspectable.
This caller consumes an original SourceStore, a requested original carrier and
rectangle, and the actual PatchSession/PacketMesh operation from the reviewed
adaptive plan. It never invokes the previous mesher to fill missing geometry.
It is an untrimmed patch diagnostic/benchmark, not whole-component delivery.

Build native/mesher/adaptive_main.cpp as brepgpu-adaptive-patch. Required options
are --source, --carrier, --output, --positional-mm and --threads; CPU is default,
P100 requires an explicit --cuda-module and selected --device. Accept a declared
--memory-mib/--max-nodes/--max-depth cap, with shared supported depth64 profile.
An optional original --u0/--u1/--v0/--v1 rectangle must be complete and finite;
otherwise the requested patch is the explicitly retained carrier rectangle
(natural knot domain for a rational surface). This does not claim the face trims
or other uses of that carrier. Unknown/foreign IDs and missing modules refuse.

Support repeated --positional-mm values as separate fresh queries of the same
retained session. The first has empty adaptive caches. Later queries reselect
from the original roots and the same base numerical policy, reuse only demanded
tiers, and emit fresh triangle output. They do not merely return a prior mesh.
The backend, query tolerance, active source rectangle and retained cache scope
are recorded for each result. Source decoding, source compilation, session/
device setup, query/adaptive work, packet construction/bounds, output write and
close are separate elapsed scopes. Keep worker work and overlapping stage time
distinct from wall. An external runner measures process exit.

Write actual indexed triangles, actual XYZ and exact original chart/UV values
to an owned little-endian patch artifact, with source/root associations retained
by the returned PatchMesh. Do not fabricate a complete NativeMesh inventory for
a patch or call its cells LEPP births. The external format is BGAPCH01, version1:
header with point/facet counts and original carrier/source identifiers; each
point has three binary64 XYZ and two canonical Rational word records
(sign,numerator/denominator lengths and32-bit little-endian limbs); facets name
three checked point indices. Exact parameters are not rounded solely for output.
Cache association/cost tables are human-readable TSV with explicit scopes;
runtime pool offsets are not semantic source IDs. No JSON internal model.

Use a fresh output directory and the existing owned no-replace publication
semantics; a successful final directory appears only after required artifacts
close. Existing paths cannot be replaced. Failure writes its typed stage error
and removes only its own temporary output. Close/drain device/session leases
under the runtime owner's healthy/poisoned lifetime rules.

Direct test first: actual original independently authored source -> this native
caller -> patch artifact -> independent Python integer/Fraction reader and
POWER/rational/analytic source formulas on actual emitted facet interiors.
Compare CPU1/parallel CPU/P100 semantic geometry for the same requested query,
including repeated and fine/coarse/fine cache history. Require named useful GPU
geometry work and concrete cache/control/sample reuse from actual counters.
Counts/image appearance alone do not establish source accuracy. This unit adds
no rasterizer or second geometric certifier. Relevant refusal/publication paths
and a focused sanitizer/source review close the caller; broader scenarios remain
the complete operation's staged comparisons.

Root owns this caller, build wiring and direct whole-patch test. Geometry,
runtime and packet owners provide their actual reviewed native interfaces.
The finite v1 header is `<8sIIQQQ32s`: magic BGAPCH01, wire revision1,
geometry policy revision1, original carrier ID, point count, facet count and
the original 32-byte SourceDigest, in that order (72 bytes). Each point emits
XYZ `<3d>` followed by original U then V; each rational is `<iII>` sign,
numerator word count, denominator word count, followed by their LE uint32 words.
Zero is sign0, no numerator words and denominator1. Nonzero records are reduced,
have sign +/-1, positive denominator and no high zero word. Facets are `<3I>`.
Every size/index conversion is checked; no trailing data is emitted. Query N
writes `query-N.bgapch`; `queries.tsv` records query index, binary64 allowance,
source digest, numeric profile, rectangle, backend and cumulative work counters.
The timing table names elapsed scopes and does not sum overlapping worker work.
The independent reader tests the actual artifact, exact UV coverage/incidence,
source-to-actual-facet discrepancies and query-history/backend correspondence.
No placeholder adapter or production fixture
supplies an absent cache/query/packet implementation.
