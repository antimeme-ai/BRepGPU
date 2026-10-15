# From breadth beachheads to a usable GPU modeling workflow

2026-10-15. Proposal prepared while the complete OM10 nonvisual qualification
runs. The operator requests roadmap work alongside that compute. This updates
the sequence proposed in [the earlier stocktake](POST_BREADTH_ROADMAP.md);
it does not select an implementation, change alpha scope or promise speedups.
The watch remains the active delivery.

## Where we actually stand

The first breadth sweep is complete. Five owned GPU stages cover patch jets,
seeded projection, conservative patch discovery, conservative surface-pair
culling and edge-use adjacency construction/reading. Narrow solid/feature
construction and trim-aware meshing run on CPU. Actual extrusion → bore → mesh
composition with original ancestry is delivered on CPU. Separate GPU consumers
elsewhere in the project establish numerical/topology stages; GPU Boolean and
feature construction remain future work.

OM10 is exercising real imported carriers, curves, trims, assembly and source
correspondence. Complete source/query conversion and independent comparison of
all 9,162 carriers, 25,871 spatial curves and 166 occurrence placements pass.
Whole meshing, CPU/P100 qualification and images remain pending. A three-second
nonblocking observation located the current quiet phase in circle-schedule
boundary intersection checks; it does not establish the full runtime profile.
The actual NVIDIA reference produced 102/107 definitions under each declared
policy before the separately reviewed rectangle-export continuation. No complete
reference or watch render is claimed by those component results.

The next useful milestone is a repeated modeling workflow: load a real part,
perform an admitted geometric edit, inspect its source/history, and update its
mesh/view while retaining unaffected state. Measure fresh and retained work on
CPU and P100, and execute the actual NVIDIA CPU counterpart where supported.
This gives numerical kernels, topology, residency and host representation one
concrete consumer. Wider geometry remains an independent axis of progress.

## Finish the watch, then the required comparisons

**Current delivery, issue60:** finish the owned whole-source mesh/backend
qualification, inspect assembled/exploded pilots, run the reviewed full callers,
and publish images with scoped owned CPU/P100 and actual NVIDIA CPU outcomes.
Missing-part reference failures cannot supply a complete reference image or a
successful full-watch speedup ratio. Keep both predeclared reference policies;
fix our exporter boundary without silently repairing vendor geometry or changing
input preparation. Preserve original correspondence, source privacy and credit.
If the running job exposes a real blocker, address that bounded prerequisite;
roadmap work adds no new watch gate.

**Required next tranche, issue63:** enumerate every scenario in the delivered
README revision and execute its NVIDIA USD/BRep CPU counterpart with comparable
source, quality and operation/output scope. Retain complete caller and useful
stage times, versions, hardware, repeated observations where appropriate and
all unsupported/failed outcomes. The reference adapter must invoke the actual
operation; timing our CPU implementation does not supply the external baseline.
Publish the scenario/result map beside the README claims. New README scenarios
inherit this obligation. Begin this work after the watch concludes, as requested.

Completing this tranche means accounting for the whole advertised inventory,
including genuine unsupported outcomes; it does not require implementing every
unsupported NVIDIA operation or repairing every reference failure. Those
outcomes remain separate from our community-suite alpha obligation.

## Next broad cycle: one useful advance in each lane

Dependencies order these deliveries. Each row is a bounded beachhead to design
and review separately, rather than a promise to finish its entire field. Start
with the external comparisons above, use their measurements and the completed
watch to choose concrete performance units, then revisit every lane before
successive refinements in one lane.

| Lane | Proposed first delivery | What would establish useful progress |
| --- | --- | --- |
| Preparation and meshing | Remove one demonstrated dominant cost from the actual watch path; candidate generation for exact boundary checks is a current candidate | Same source, shared boundaries, holes, quality and error obligations; direct candidate-coverage/predicate oracle; actual whole preparation wall and peak memory. Profile once before choosing between pair pruning, triangulation or tighter refinement bounds |
| Runtime and Python | Compare Python orchestration, a thin optional binding and a native caller on one complete retained workflow; implement one reusable owner after the decision | Explicit load/update/execute/shutdown costs, live-output ownership, bounded memory and failure recovery; fresh/repeated best-CPU/P100 comparison. Keep research/oracle language independent of the shipping decision |
| Imported geometry and edits | Select one actual source part and one geometric edit; publish a new body revision, source history and derived mesh | Real material/trim change with original correspondence, shared edge/UV consistency and independent geometric checks. Preserve old readers and inputs on failure. Assembly explosion alone does not complete this lane |
| Intersections and regions | Broaden the first stage that blocks that edit: a declared transverse surface-pair family, clipped/source-hole arrangements or supported region selection | Complete admitted components/regions, spatial and both-UV correspondence, explicit unresolved cases. Intersection, arrangement and Boolean assembly remain separate implementation units |
| GPU topology construction | GPU assembly of one admitted result's packed topology from already established geometric events/classified regions | Actual new vertex/edge/coedge/loop/face tables, complete ancestry and candidate validation; CPU/both-P100 semantic correspondence under slot permutations. State explicitly which geometry/classification still runs on CPU |
| Community oracles | Execute one applicable pinned upstream case family through owned operation/result adapters, with the complete relevant inventory visible | Checks read BRepGPU's actual output and retain original case identity/expected semantics. Required unported or failing cases remain non-green; this is independent of NVIDIA performance comparisons |
| NCSX and wider features | Select one sourced engineering component and the smallest missing geometry capability needed for it | Owned closed/trimmed geometry with a stated source/fit/error contract and a comparable result. If a sweep is required, deliver it as its own unit. Preserve the existing illustrative coil/plasma baseline |

The imported-part edit is a **selection task**, not an already admitted API.
Choose it from the watch BOM/geometry and existing operation contracts after
whole-model delivery. A through-hole or local feature change is a candidate;
importing a part does not turn it into the canonical fixture expected by current
constructors. First state the geometric contract and map the missing stages.
Use a minimal authored counterpart with independent truth alongside the actual
private source; preserve genuine unsupported cases rather than widening guards.

Community adapters and source/geometry study can progress independently once
the reference tranche begins; they need not wait for a retained runtime or a
general SSI solver. New external reference dependencies still require the
operator discussion specified in AGENTS.md. Existing selected OCCT/NVIDIA
bridges do not select an additional kernel dependency.

## GPU construction and the topology map

Continue the [existing identity architecture](TOPOLOGY.md): logical entity IDs,
revision-bound packed slots and runtime handles have different lifetimes.
Geometry and topology live in typed arrays; seams keep their distinct oriented
uses. Device work reads slots and ranges, never a host object graph. Source
maps retain original curve/UV parameters, owners and generated/deleted ancestry.

First establish the admitted CPU result candidate and history semantics as the
model oracle. The next construction beachhead can consume those CPU-established
events and region choices, use count/scan/fill to size and emit a separate candidate snapshot, and
validate its incidence, loops, geometry and material before publication. This
extends the delivered CSR construction to actual result topology. It does not
establish GPU intersection, GPU classification or a complete GPU Boolean.
Its first input can be the already delivered through-bore family, so wider
imported editing is not a prerequisite for this construction beachhead.
Within supported single-body edits unchanged logical IDs survive; Boolean output
uses its own body namespace plus explicit operand history. Slot relocation
rewrites dependent references and invalidates stale revision-bound requests.
Failed allocation, unresolved geometry or invalid candidate output leaves the
old immutable body readable. Design those semantics before device allocation
or incremental adjacency tuning.

Later cycles move the costly remaining stages into resident geometric queries,
intersection branches, arrangements and classification. The target is a complete
admitted modeling operation whose result stays usable on the device across
subsequent operations. General rational SSI, tangencies/coincidence, interacting
fillets/offsets and broad editing each need their own contracts and literature.
They are visible longer-term work, not implied by the first imported edit.

## How to choose and stop each performance unit

Use the completed target's stage walls, work counts and memory to select one
mechanism. The short live-stack sample motivates investigating exact boundary
candidate generation; it does not justify a Python rewrite or an assumed GPU
mesher. Existing NCSX and adjacency results already show that fast kernels can
sit inside expensive fresh callers; retain both scopes in the next comparison.

Compare the actual same inputs and declared quality. Retained source/mesh work
must be labeled retained; fresh source-to-output includes preparation and final
shutdown. Report setup, transfer, useful native work and host consumption without
silently excluding a shared cost. Preserve independent qualification as a
separately named cost. Accept a CPU win or no crossover as a result.

Stop after the selected consumer, meaningful direct tests, relevant compatibility
checks, independent review and measured outputs. Test the actual fault classes;
add no general certification campaign to a bounded delivery. Record the next
opportunity and move to the next lane. P100 remains the current GPU target;
Apple Silicon issue22 is a separate platform proposal.

## Alpha and the horizon

The next cycle aims at useful measured modeling, reusable ownership and one
actual GPU topology-construction operation. A later cycle can expand interacting
features and move more of the same modeling path onto the GPU. This ordering
keeps both geometry coverage and GPU construction advancing without a universal
kernel rewrite as a prerequisite.

Alpha still requires every applicable case in the agreed external corpus to be
green on the advertised operations/backends. Missing fixtures/adapters, required
unsupported cases, skips, failures and unresolved oracle disagreements are
non-green. Issues4/43/48 retain their actual discrepancies; no roadmap milestone
relaxes them. See [the unchanged alpha contract](ALPHA_QUALIFICATION.md).

Repository basis: [original breadth map](ACCELERATION_ROADMAP.md),
[measured/deferred work](RETURN_PASS.md), [topology design](TOPOLOGY.md),
[Python-role brief](../papers/2026-10-14-python-role-return-pass.md),
[mesher cost study](../papers/2026-10-15-om10-mesher-cost-study.md),
[live phase observation](../journal/2026-10-15-om10-live-mesh-phase.md),
[actual NVIDIA component outcomes](../journal/2026-10-15-om10-actual-nvidia-geometry.md)
and [open issues](../ISSUES.md). This document adds no product execution or new
performance claim. The running watch delivery and its reviewed plan remain active.
