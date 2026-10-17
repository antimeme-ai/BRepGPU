# Stocktake and proposed second breadth pass

The [post-watch roadmap proposal](POST_WATCH_ROADMAP.md) updates the next
sequence using the delivered composition chain, active OM10 work and required
README-wide NVIDIA CPU comparisons. This earlier stocktake retains its history.

2026-10-15, after the matched NCSX rerun. This is a repository-grounded roadmap
proposal, not an accepted implementation design, a shipping-language decision,
an alpha scope change or a promise of speedup. Individual units still require
source study, reviewed design/plan and direct qualification before implementation.

## First selected connection delivered

The operator authorized proceeding. The first bounded unit of B is now
[delivered](CUT_MESHING.md): actual through-bore CutResult to a closed bounded
mesh, preserving original cutter frames/UV, source history and shared boundaries.
The canonical mesher remains factory-only; the new dedicated API consumes the
actual retained result. The first feature-result connection, [actual convex extrusion meshing](EXTRUSION_MESHING.md),
is also delivered. The first [actual composition chain](FEATURE_BORE.md) now
consumes that extrusion, constructs a strictly interior axial bore, and meshes
the result with ordered ancestry back to the original profile and actual cutter.
The [watch-inspired assembly](WATCH_TARGET.md) is its concrete visual consumer.
External adapters, runtime/Python study and owned NCSX geometry remain
next-tranche work; further feature families are separate units.
No language/dependency or alpha scope change was made.

## What the first pass established

Every theme in the [breadth map](ACCELERATION_ROADMAP.md) has a working beachhead.
Five stages have owned CPU/CUDA implementations: rational patch jets, resident
seeded projection, conservative single-patch discovery, conservative surface-pair
discovery and edge-use adjacency construction/reading. Exact CPU work supplies
canonical bodies/trims, analytic intersections, classification, arrangements,
canonical meshing, narrow carrier interchange, a through-bore Boolean and four
feature families: extrusion, revolution, fillet and offset. GPU consumers of
CPU-constructed solids do not imply GPU solid construction.

Typed immutable identity, source parameters and occurrence multiplicity are a
substantive foundation. The topology work avoids pointer-defined identity; it
does not yet provide a general editor or persistent naming. The first bounded
extrusion/bore composition now preserves actual ordered source-history steps.
Each delivered operation retains its declared narrow input family. The canonical
mesher admits factory BodySnapshot values; the separate through-bore result
mesher now admits its actual CutResult. Arbitrary feature/Boolean results remain
unsupported.

The major performance lesson is the difference between accelerated stages and
whole operations. Cached pair discovery already has a measured complete-shortlist
benefit for its large case. Small fresh-process feature consumers lose to CPU
because host work and GPU initialization/cleanup dominate. Actual results and
different timing scopes are retained in [RETURN_PASS](RETURN_PASS.md); these
observations do not establish a universal backend choice.

The [NCSX rerun](../results/ncsx-rerun-2026-10-15/REPORT.md) reproduced all nine
P100 artifacts and the composed hero exactly; CPU geometry pixels matched.
Evaluation was 16.310 ms CPU1 / 1.430 ms P1000, but cold native work was
119.862 / 570.840 ms. Full qualified callers took 41.024 / 40.379 s, including
roughly 28 s independent exact verification and 9 s CPU display work. These are
one observation each, without a historical timing baseline. The newly delivered
feature constructors are absent from this fitted-carrier scene.

## Deferred work, and what is now eligible

The broad-pass scheduling condition has been satisfied. The following work is
now eligible for selection; that does not select every recorded optimization.

| Deferred area | Actual remaining work | Proposed place |
| --- | --- | --- |
| Result consumption and meshing | Mesh actual trimmed CutSolid/feature output; current canonical mesher cannot consume it | First connection between existing capabilities |
| Source maps and history | First actual extrusion/bore chain delivered; arbitrary edit graphs remain | Further composition selected from a blocked consumer |
| Python's delivered role | Compare native application/core, optional thin binding and Python orchestration using actual consumers | Study and measured prototype, then architecture decision |
| GPU setup and residency | Engine/module ownership, separate adjacency contexts, reusable inputs/scratch/output and explicit shutdown/recovery | First retained workflow after ownership design |
| Host representation and bulk consumption | Exact request generation, material/trim decisions, normals/scatter, typed adjacency/history reads and mask decoding | Relocate only stages identified in the selected complete workflow |
| Community qualification | Pinned applicable case/data inventory; adapters checking actual BRepGPU output; unported/missing cases remain visible | Start now, alongside operation work |
| Reference disagreements | Revolution volume #43, cylinder source/offset #48 and sphere chord discrepancy #4 | Separate bounded investigations; no threshold relaxation |
| NCSX | Engineering source revision, owned source conversion/trim provenance, closed geometry and bounded mesh | First source-to-owned component, with its own contract |
| Wider geometry | Multispan extraction/imported trims; curved contacts; clipped/tangent arrangements; broader SSI/Booleans/sweeps/blends/offsets | Select from a concrete blocked consumer |
| Other hardware | Open Apple Silicon request #22; explicit backend/toolchain/semantic and benchmark scope | Separate platform proposal, not implied CUDA portability |

NCSX timing is the one formerly deferred comparison already completed. Improving
its CPU display and improving its geometric fidelity are separate possible units.
The current inner surface is plasma, not the vacuum vessel, and coil sections
remain illustrative. Existing extrusion/revolution do not implement a general
nonplanar coil sweep. See [goal object](NCSX_GOAL_OBJECT.md).

## Proposed tranche: connect, reuse and independently challenge

Keep the operator's broad-first intent: one bounded advance in each lane before
deepening any one lane. Dependencies order the units; this is not one combined
rewrite or a calendar schedule.

| Unit | Concrete user-visible delivery | Completion claim |
| --- | --- | --- |
| A. External case inventory and first adapters | Publish the pinned relevant upstream inventory and actual comparisons for currently admitted geometry | Original case identity/checks retained; all non-pass dispositions visible; no passing subset represents complete alpha qualification |
| B. First result mesh | Turn the actual through-bore result into a closed bounded-error mesh; follow with one feature family as a separate unit | Cover actual trims, share physical boundaries and preserve source/coedge/chart identity; reject unmet budgets |
| C. First composable modeling chain | Admit one chosen operation result to one next operation, with inspectable composed history | Correct final material/topology and original-source correspondence, including generated/deleted ancestors and stale revision rejection |
| D. Runtime/language decision and retained caller | Compare complete fresh and reused operation costs; prototype retained native consumption for a chosen real workflow | Include ownership, immutable live outputs, memory, failure recovery and final shutdown; compare best CPU and each promised GPU |
| E. NCSX connection | Convert or construct one selected sourced component into an owned closed/trimmed result and consume its bounded mesh | Explicit source/fit/error contract and actual topology; preserve the original comparison scene and label geometry changes |

A can start on existing supported families without waiting for B--E. Existing
reference adoption policy still applies: propose benefit, scope and costs before
adding any new reference dependency. Inventory/adapter design requires no new
dependency by itself. #43/#48 investigations can proceed separately; absent an
established cause, neither our output nor a reference is presumed faulty.

B starts with CutSolid because its construction already retains real section,
arrangement and trim provenance. It must read those records rather than relabel
the output as a canonical plate. Extending B to an extrusion or other feature is
a separate conceptual delivery, not evidence that a universal mesher exists.

C's first pair is delivered: actual convex extrusion followed by one strictly
interior, fully through axial circle. Off-center cuts are admitted, with exact
support tests, real cap-hole domains and retained original carrier Frames/UVs.
The watch study uses actual meshes from that chain alongside separate convex
extrusions. Multiple cuts, nonconvex profiles and a functional watch remain
outside this delivery. Further pairs must build their real domains rather than
widen a type check to bypass canonical assumptions.
Compose maps with explicit owners/domains: revolution quarter coordinates are
not uniform angles, fillet spine correspondence is not spatial coincidence, and
offset carrier feet need not lie in the original source trim. Preserve every
seam occurrence and ancestry from deleted entities. General persistent naming
and an arbitrary edit graph are later work.

D begins with the already requested Python-role investigation, using existing
callers while B/C develop. A native consumer/session prototype may use a current
caller; it need not wait for a general operation graph. Retaining engines and
inputs may help without moving exact arithmetic into C++. Conversely, moving
packing/rows into native code does not alone settle Python's product role.
Select migration only from complete workload measurements and ownership needs.
Independent qualification remains separate from useful production work and must
stay visible in reported qualified walls; speed claims cannot silently omit it.

E starts with source/representation selection. It must not wait for the entire
NCSX assembly, a general importer or arbitrary rational SSI. If the chosen
component requires a new sweep or bounded approximation, design that actual
prerequisite rather than claiming existing analytic constructors already supply
it. Display-only tuning can be a separate measured improvement; it is not CAD
geometry progress.

## What follows this tranche

Broaden the first real workflow where its inputs expose a limit: source holes
and clipped/interacting arrangements, curved contact and general transverse SSI,
multispan extraction and imported trims, or partial/holed sweeps and interacting
features. Each remains a complete bounded operation, with CPU/GPU placement a
separate choice. GPU construction can then move an established costly stage
while retaining the same geometry/topology semantics; full GPU residency is the
longer-term direction, not a prerequisite for delivering these connections.

Keep certified predicate filters, sparse pair pruning, tighter off-grid bounds,
scan/queue tuning, persistent workers and output pooling as measured candidates.
None automatically outranks missing consumer semantics or a demonstrated bug.

Alpha is a separate release milestone. The complete required external corpus
against advertised operations must be green, including data/adapters/backends;
missing, unsupported, skipped, xfail and unresolved disagreements remain
non-green. This proposal changes no promised scope. The next tranche can finish
while alpha remains blocked. See [release contract](ALPHA_QUALIFICATION.md).

Source basis: [return-pass measurements](RETURN_PASS.md), [issue index](../ISSUES.md),
[Python study brief](../papers/2026-10-14-python-role-return-pass.md), and the
Boolean/extrusion/revolution/fillet/offset and adjacency round-two notes linked
there. The original stocktake performed no product execution or benchmark.
Subsequent delivered B/C units are linked above; none selected a new dependency
or language migration.
