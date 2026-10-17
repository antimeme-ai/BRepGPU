# Local chosen-domain recovery

The actual saved chosen plane loops contain proper crossings near a common original
junction of two unequal-radius tangent arcs. `prepare_face_domain` currently discards
`DomainSegmentIndex::crossing()`'s exact pair and requests every positive-bound interval.
The trace shows neighboring valid faces repeatedly rebuilt as these unrelated intervals
double. This unit changes only recovery proposals; exact graph admission, original source,
chosen trace guards, physical point authority, quality and all caps remain unchanged.

Grounding: the existing `domain_index.cpp::crossing` exact segment witness and
`adaptive_regions.cpp::prepare_face_domain` source-word construction provide the selected
mechanism. The subdivision-based boundary recovery discussed in
`papers/2026-10-15-om10-tangent-boundary-study.md` motivates local refinement rather than
uniform source subdivision. The direction below is an owned finite heuristic, not a
universal topology or convergence theorem.

Retain a private edge-origin row (trace and actual interval indices) alongside every
constructed boundary edge, reversing/inserting it with that edge. Record a typed first
failure: short/zero-area loop, zero edge, exact crossing pair, or invalid loop nesting.
Choose ONE existing captured BoundaryKey from that witness. For a crossing between
original uses adjacent at a common original junction, inspect the first three distinct
captured physical Q sites away from that junction on each use. Compare squared
circumcircle curvature `4*|a cross b|^2/(|a|^2 |b|^2 |b-a|^2)` by exact cross products.
If both estimates are positive, refine the crossed interval on the flatter side. If
exactly one estimate is zero, choose the curved side. Require all three lengths nonzero;
missing/degenerate triples use the ordinary local fallback. This avoids choosing a fixed
straight chord as the flatter side. Physical Q is read from the same captured boundary;
there is no evaluator or private point construction.

Fallback chooses one eligible actual interval within the named witness/loop by largest
retained `carrier_trace_to_actual_chord_upper`, with canonical BoundaryKey ties. A
connector has no invented source key. No eligible interval gives typed unresolved.
Zero-area/short-loop failures use only the implicated loop; zero edges use
only their actual interval. Genuine straight crossings cannot be declared valid: every
retry still passes the same exact no-contact, winding and nonzero-edge/area checks.
Nonconstant curvature and distorted UV charts may misrank the proposal; existing finite
limits bound attempts, and failure never produces an invalid accepted domain.

Own `adaptive_regions.cpp`, existing native region fixture and its source/oracle adapter,
plus this plan/journal. No public result schema, geometry kernel, ABI or cap changes.
First run the actual domain operation on a source-backed tangent instance: old behavior
must request unrelated keys, which is the precise RED claim (old graph rejection is
correct). Then use fixed radius/sampling/rigid-reflection variants and a curved-versus-line
case; run requests through the real boundary authority until the same exact domain
operation admits. Assert one witnessed key per horizon, preserve the complete original
use word, no unrelated-key request, and feed accepted output to the existing exact simple
polygon/chain oracle. Fixed non-simple straight source must remain refused/capped.
Run only this affected strict fixture/oracle, format/tidy and actual source review. Parent
then runs the actual previously blocked component on CPU/P100 once; no broad campaign.

Review amendment: preserve the previous bounded all-use fallback for nesting failure.
Winding alone does not identify whether a hole or its coarse enclosing outer rim needs
refinement; restricting it to the hole could regress existing behavior. The one-key
claim applies to crossing and the other localized witnesses, not nesting. No new nesting
algorithm or convergence claim is introduced. Three collinear Q samples are a zero
curvature estimate, not a proof the whole original use is straight.

Fixture observation: the three fixed rational tangent inputs and fixed curved/straight
input already admit their initial graph. They remain controls, not recovery evidence.
The unchanged original five-arc source subset supplies the direct crossing RED and
recovery oracle at three predetermined initial uniform subdivision levels (0,1,2).
Each accepted output passes the SAME independent exact simple-domain/chain reader;
the actual curved/straight direction branch is source-reviewed, not claimed exercised.
A literal straight bow-tie source must still refuse with no accepted output.
