# Body sampling plan: topology review

Accepted with no blocker. Reviewed docs/BODY_SAMPLING_PLAN.md against the
accepted design and body topology study. No product code or native execution.

Factory-only make_body keeps this a canonical analytic-body operation rather
than a hidden importer/general assembler. Frozen recursively immutable records
and owned packed bytes preserve the logical graph; existing Patch/Lineage and
ABI1 evaluation supply numerical execution. Runtime handles remain separate
from body IDs, revision and dense snapshot slots.

The plan retains one cylinder face/four charts, one closed edge per rim and
two opposed seam coedges on the same wall face. Independent graph, hole-sensitive
Euler, loop orientation and analytic edge/pcurve checks are appropriate direct
oracles. Exact submitted-UV membership is separate from approximate curve-map
evaluation. Local-chart derivative, frame and rounded trim-boundary cases remain
covered without assuming globally smooth atlas derivatives.

Prepared requests retain the exact snapshot, derive labels once and reject
foreign snapshots; gather/scatter keeps original candidate order and uses the
admitted count for SoA stride. Rejected/invalid samples have explicit unevaluated
status, while native/normal failures cannot publish successful output. Child
cleanup preserves primary errors. No new body GPU ABI, general normal solver,
mesh certificate, Boolean history or persistent naming is implied.

The module handoff should document ordinary concrete choices before client
implementation relies on them: Vertex.position; chart-slot/native-patch-slot
ordering; Curve/UVMap kind/data definitions; named packed-table field layouts.
These belong to the planned body.py implementation documentation and do not
require expanding the operation's scope or adopting another framework.

The 24-case measurement matrix separates CPU body/trim/normal work from native
evaluation, one cold observation and five reused calls, and actual caller
export/qualification costs. Host-code mutants attack the newly owned predicates,
orientation/seam/scatter rather than repeat unchanged native campaigns.

Ready for authorized implementation after canonical interface handoff. This
acceptance is a plan/source disposition, not an executed capability claim.
