# Native complete-component CLI

2026-10-17. Finite caller subplan for root review before body implementation.
This lane owns only new adaptive_component_main.cpp and coordinated caller-mode
ports in the existing component driver. Root owns ComponentMesher, its typed
statistics, writer, CMake and original-output oracle. No display, new geometry
policy, old GeometryStore or dependency is introduced.

## Grounding and operation

Read adaptive-component-coordinator-unit.md and the actual component.hpp,
adaptive_assembly.hpp/.cpp, assembly.hpp/.cpp, adaptive_caller_io,
adaptive_trimmed_io, scheduler and OutputDirectory. Existing selected-face CLI
supplies checked options, staged close-before-no-replace publication and typed
stage errors. Its completed external failure fixture supplies those failure
expectations; service cancellation/stale work remain in their direct fixtures.
The existing component plan supplies the pinned numerical/topological literature
and actual shared-horizon contract. The CLI does not implement another controller.

The executable is brepgpu-adaptive-component. Options mirror the selected-face
caller except --face: required --source, --definition, --output, repeatable
--positional-mm, --node-source-mm, --threads. Optional --face-owners defaults to
threads; positive bounded integer, with actual active count limited by component
inventory/scheduler. Retain --backend cpu|p100, explicit --cuda-module/--device,
--memory-mib, --max-nodes, --max-depth and --construction-work. Preserve existing
qualified defaults, including construction-work1<<42, and checked products.
CPU never opens a module; P100 failure never falls back to CPU.

1. Parse/validate options before mutable owners. Open one OutputDirectory and
   timing/index streams. Decode SourceStore once; resolve the original selected
   DefinitionID and its complete face inventory. Retain the entire SourceStore.
2. Prepare AdaptiveAssembly once for the first requested exact quality. Its
   original definition boxes and frozen per-carrier cache profiles do not depend
   on quality. Build the fixed source bank from only the selected definition's
   distinct carrier rows, retaining their EXACT admitted cache_domain; gather
   its complete spatial/pcurve CurveRequests through the existing caller helper.
   Compile both immutable banks once.
3. For later requested qualities reuse those SAME original boxes/profiles and
   banks, but rerun existing prepare_assembly transport. Eta depends on epsilon
   through the expanded original box, so reusing the first query's allowance
   would be wrong. Import retained definition boxes into caller-owned Rational
   arrays and pass borrowed DefinitionEnvelopeViews synchronously. Do not rescan
   original source/control geometry or expand a cached profile. Keep the initial
   AssemblyStore for query1 and use a new unique allowance table per later query.
4. Locate the selected DefinitionAllowance by its source slot and pass its SAME
   AssemblyStore ExactTable and positional/node_source refs to ComponentQuery.
   No conversion through double/text, source-unit rescaling or invented defaults.
   Requested CLI doubles become exact original binary64 Rational inputs once.
5. Keep one ComponentMesher, fixed bank and outer NativeScheduler across the
   query sequence. Configure actual outer CPU width from --threads; each face
   owner has surface.cpu_threads1. Root's component service owns forest union,
   face assignment, contacts and cancellation/drain. The caller only invokes its
   mesh(query,scheduler,context) and writes a returned COMPLETE ComponentResult.
   Each quality has monotonically increasing generation, even when values repeat.
6. Write the complete query while retaining its result. Release it before the
   next query; no mutable face owner/forest handle is used for export. At the end
   close ComponentMesher, stop/drain scheduler, close every stream and publish
   through the existing no-replace rename. Any detected failure unwinds the same
   OutputDirectory transaction and returns the precise stage/error class.

## External files and lifetime

Write source.bgsource from SourceStore::original_bytes verbatim, never reread a
possibly changed path. External component.tsv names the explicit definition
scope, full source digest, numerical policy, backend and requested qualities.
For each query create a directory with face-<OriginalFaceID>.bgatrm plus a
faces.tsv index pairing every face to its EXACT retained BGACRV snapshot.

Deduplicate snapshot files by retained SharedCurveSnapshot object identity, not
by the latest forest epoch, row ordinal or a content hash. Iterate canonical
OriginalFaceID order; first semantic occurrence assigns snapshot0,1,... so
allocation address/hash iteration does not determine filenames. Retain all
snapshot handles until that query's writes close. Include source/query/boundary
epoch and original face IDs in the external index. An unaffected face may
legitimately pair with an older snapshot than ComponentResult.boundary. Export
the final complete boundary snapshot as a named index entry even if no retained
face references it. No JSON, private absolute paths or hostnames in output metadata.

The existing BGATRM writer emits local complete original carrier roots and
remaps source slots; use it unchanged. Original source bytes/index/paired wires
retain the whole original topology while this operation reports ONE definition.
This is not an assembly-complete or image-producing result.

## Timings and exact quality metadata

Retain native source decode, original-envelope/initial assembly, immutable bank
compilation, scheduler/mesher construction, per-query assembly transport,
complete component operation, artifact write, close and prepublication horizon.
Stdout after successful publication includes actual elapsed native wall. External
runner process-exit wall includes process launch/exit and remains separate.

queries.tsv includes requested global quality and exact local quality as
`r:<signed hexadecimal numerator>/<hexadecimal denominator>` read directly from
the retained ExactView limbs (also serialized by the paired face wires), complete face/site/facet counts,
assignments/boundary rounds/retained faces, contacts and actual CPU/P100 work.
Root is adding cumulative per-owner surface snapshots/deltas to ComponentStats;
use the actual declared fields rather than infer transfers or device setup from
mesh time. Distinguish per-query curve/contact counters from cumulative persistent
surface-owner counters, actual RunResource peak including device reservations,
and mesher construction/close scopes. Do not sum overlapping child scopes.

Only immutable source/service state is warm across qualities: pooled face pages
retire and each component call creates its physical authority. Metadata must not
claim persistent per-face numeric cache reuse or return a prior mesh unchanged.

## Direct RED and bounded closure

Before body, invoke the proposed executable through the existing authored
component driver and record actual absent-caller RED. Root supplies target/deps
after main.cpp exists. Link adaptive_component, adaptive_assembly, caller-I/O,
trimmed-I/O and Threads/dl through the existing strict CMake profile.

The existing original-output component reader qualifies the actual closed-box
caller, complete source incidence/face inventory, exact local allowance transport,
paired boundary identities and geometry. Reuse its authored complete definitions
for CPU1/parallel CPU/both P100; use a repeated quality sequence to exercise the
same retained bank and a fresh component authority. No second mesh certifier.

Add only CLI-specific refusal ports not already supplied by that driver: wrong
definition, malformed source, insufficient resources and existing output left
unchanged; require structured stage/code and no final/temporary artifacts.
Service cancellation/stale/failure ownership stay with direct runtime fixtures.
Root's module/assignment numerical checks are reused. Static and one relevant
instrumented actual caller target NEW caller ownership/publication, followed by
actual-source root/peer review. No broad corpus or watch run in this unit.

## Delivered interface and closure

The actual output uses one top-level `faces.tsv`, with query ordinal and relative
query-directory paths, plus each query's `final-boundary.tsv`. This preserves
all original face/boundary pairings without repeating the same index schema.
The complete native caller and `--caller` driver mode are implemented. Root's
actual-source review accepted ownership/transport/publication; the query timing
scope finding is resolved. See the journal for the bounded six-mode original
reader closure, exact refusal diagnostics, sanitizer scope and the explicit
CPU4 cumulative cover-revision wire difference. No general performance result
or imported component claim follows from these authored fixtures.
