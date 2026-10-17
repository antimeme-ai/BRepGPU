# One complete native definition over bounded face owners

2026-10-17. Concrete subplan under review. Implements the component horizon and
physical-output handoff in adaptive-component-unit.md, using its accepted fixed
source-bank/assignment architecture. Existing face numerics, cache subdivision,
physical forest and contact predicates remain the actual machinery.

Scoped construction and runtime plan reviews accept this unit. Grounding is
NVIDIA USD/BRep c9c5979f04cea8c38213004d4f04df0dcfb40d05,
SmTess::TessellateEdge / Phase1SetupBrep / Phase2CreateFacePolygons; OCCT 8.0.1
b8f597c677811d1f9f4d8a97f5ae2825c0353a42, EdgeDiscret::process/Tessellate2d,
ModelPreProcessor::SeamEdgeAmplifier and FaceDiscret::performInternal/process.
Those stages motivate one physical chain, occurrence-specific charts and face
parallelism. The complete immutable request horizon, exact source bounds and
bounded task ownership are our selected mechanism, not an upstream correctness
claim. Existing ContactEpochOwner/FacetBVH/classify_contact are the owned exact
contact mechanism; no additional triangle checker is introduced.

## Literal native API

`adaptive_component.hpp/.cpp`, namespace bgmesh::adaptive:

* ComponentLimits: TrimFaceLimits face, CurveLimits curves; uint32 face_owners;
  uint64 maximum_boundary_rounds; ContactEpochLimits contacts; contact_batch.
  Each face owner uses cpu_threads1; outer NativeScheduler supplies actual face
  parallelism. The component service never creates one permanent owner per face.
* ComponentQuery: uint64 generation, Slot<DefinitionTag> definition,
  CurveSourceCache curves, ExactTable values, ExactRef positional/node_source.
  The curve cache and the pool's fixed SourceCache must retain SAME SourceStore.
* ComponentResultData: retained source bank, original definition, query/quality
  values, final SharedCurveSnapshot, every complete FaceTrimResult in original
  FaceID order, and stage/work/contact counters. Each face keeps its EXACT older
  boundary snapshot when an unrelated split leaves it valid.
* ComponentMesher(resource,SourceCache,ComponentLimits,optional CacheDevice,
  stop_token). `mesh(ComponentQuery,NativeScheduler&,JobContext&)` returns one
  immutable ComponentResult, or typed failure. `close()` drains owners; readable
  host results survive it. Lifetime of fixed source bank/device face owners
  spans multiple complete definition calls. A definition boundary session is
  local to one call and uses that query's requested curve bank/quality.

Global quality may repeat across different definitions in one assembly but
cannot go backward. Pool assignment counter is checked monotone across all
owners/calls. A boundary session uses that SAME quality generation, not a fake
per-face query number. No JSON/native IPC or production Python is introduced.

## Complete horizon algorithm

Check resource/source, explicit original definition, every requested exact cap,
positive capacities and idle scheduler before opening mutable work. Canonical
face inventory comes from SourceData definition CSR sorted by original FaceID;
empty/missing inventories refuse. Every face's carrier must have roots in the
fixed source bank. One physical AdaptiveBoundarySession prepares the entire
definition under the SAME tau/delta.

Allocate at most min(face_owners,face_count,scheduler capacity) reusable face
owners. A task owns exactly one face assignment and advances one existing
FaceTrimSession step. Submit bounded waves, drain each wave, fold completions in
task-key order, and resubmit ready tasks. Neither workers nor the coordinator
block on queued child work. Each task returns complete or first needs_boundary;
capture its host result or copy its BoundaryKeys before retiring the assignment.
Fill freed owner slots with remaining original faces. A one-face owner must
handle more faces than its resident node/point capacity can accumulate.

After ALL current faces arrive, union/sort/deduplicate requests, then refine the
physical forest once. Settle all occurrence traces. Read each face's actual
boundary_generation from the new snapshot and invalidate only results whose
captured generation changed; unaffected immutable results remain. No mutation
of the forest occurs merely because the first face reports a request. Retry
affected faces through fresh assignments with retained immutable source data.
Bound round count, work and memory explicitly; a stalled/unsupported source is
not accepted. There is no guessed topology repair or old-mesher fallback.

At a settled no-request horizon, verify complete original face inventory and
current captured per-face boundary generations. Build actual contact input once
for this final component. The reused ContactEpochOwner receives every original
face, canonical actual node slots and current actual facet XYZ. PhysicalNodeKey
is authority; bit-different XYZ under one key refuses. Generated sites have
face-qualified keys, not coordinate welds. Source boundary edges declare their
actual original EdgeID and persistent interval; other edges are face-owned
canonical physical-node pairs. Packet rims are internal face edges.

Only USED nodes from each retained face are imported: an older snapshot can
contain stale unrelated boundary rows. Generated node keys use original FaceID,
global quality generation and canonical construction-site ordinal in this ONE
immutable final horizon. They never use worker slot/assignment epoch. There are
no provisional contact-scene edits in this unit; future edits require an explicit
logical face generation rather than reinterpreting these final-horizon keys.
The boundary session receives the SAME fixed source bank via its explicit-bank
constructor; selected trace coordinates must stay within its prepared rectangle.
Contact input is a bulk immutable horizon: gather used site records in flat
arrays, sort once by PhysicalNodeKey, check equal-key XYZ words and assign
canonical NodeSlots. This avoids a heap-allocated tree entry per vertex and makes
node numbering independent of parallel completion. The older mutable optimizer's
incremental node hash remains appropriate there and is not duplicated here.

Use its existing BVH, filtered/exact contact pairs and full face horizon.
Unpermitted contacts return precise unresolved with original face/facet keys;
they do not publish a component. This first integration does not claim an
unimplemented contact-driven refinement route: if an admitted original workload
needs one, implement nomination through the SAME cache/central-anchor mechanism
under its own actual-source subplan. No second triangle intersection checker.
The direct contact service already checks its own full horizon and stale handles.

Only then return a complete native ComponentResult. Any error cancels and drains
active owners/tasks before destruction. A failed owner is terminal; it must not
silently reset a cancelled stop_source for later work. No output directory is
published by this service. The actual model caller stages all final export and
publishes only once its requested component/full assembly is complete.

## Semantic export and first actual caller

Root adds a `brepgpu-adaptive-component` native caller. Arguments mirror the
selected-face caller, replacing --face with a complete explicit --definition.
It prepares source/assembly once from prepare_adaptive_assembly, builds the fixed
source bank from the SAME retained carrier profiles, obtains local tau/delta
from AssemblyStore and executes ComponentMesher. It writes original source bytes,
complete per-face BGATRM and exact paired BGACRV files plus an external artifact
index, actual source/quality/backend counters and native timing scopes. Snapshot
files deduplicate by captured lease/epoch; never reinterpret stale row ordinals.
No display or watch success is implied by this component-only caller.

BGATRM currently serializes every SourceCache original root. A multi-face bank
must export only the selected face carrier's COMPLETE original root closure,
remapping surface-site and packet-owner original ordinals consistently. It must
not repeat every other carrier in every face artifact or drop outside branches
of the selected carrier. Wire1 remains the SAME literal fields, now correctly
local to this published face. The existing independent reader is the oracle.

## Direct RED and finite checks

Before bodies, a new actual-source fixture calls the proposed complete component
API and demands all its original faces. Missing API/constructor is the initial
RED. The independent component reader extends the SAME source/whole-face reader
with explicit selected original face, not a new triangulation certifier.

Preselected cases are an authored closed box with six independent original plane
charts and twelve shared physical edges; a periodic cylinder seam component; a
definition with an actual coarse closed-curve domain requiring a physical forest
request; and malformed/source-incompatible input which publishes nothing. Keep
complete source inventory, original edge-use incidence, actual node identity and
original whole-facet bounds. Check the explicit component scope against full
source definition IDs; never silently filter the product input. Compare CPU1 /
parallel CPU /both P100 actual artifacts and counters after source checks.

Scope sanitizers to NEW task/pool/result/export ownership and host concurrency.
Reuse completed numerical and device retirement qualification. A complete
imported component run with unchanged quality follows these authored connections;
the three singular source-vertex wedges remain a separate concrete capability
before the complete movement. Save actual failures/costs and stop after direct
claims are answered. Whole assembly/SceneView integration is the next dependency,
already selected in the parent design, not another meshing algorithm choice.
