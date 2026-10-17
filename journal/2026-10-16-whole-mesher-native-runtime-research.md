# Whole-mesher native architecture and runtime research

2026-10-16. Parent assigned the coherent native runtime research unit after the
operator selected a whole mesher design/plan/delivery. Read-only source and
literature research; no production/test edits, CAD, build or performance run.

Read current source snapshot reader/model, extractor/source-UV adapter, imported
mesher stage/data contracts, exact/domain/direct-CDT interfaces and allocator,
native CPU/CUDA display math/session code, binary scene reader, and pinned native
rigor settings. Read retained NVIDIA/SMLib tessellation/cache phases and concurrency
API notes, OCCT edge/face task stages, Triangle pools and its original data-layout
discussion, and CUDA 12.8 directed double-intrinsic documentation. Shared snapshot
references remain in their existing quarantine manifests; no foreign dependency
or copied source selected.

Saved findings and proposed interfaces/ownership in
`papers/2026-10-16-whole-mesher-native-runtime.md`. Key distinctions: source_uv is
the foreign adapter, not owned runtime geometry; BGMESH01 is a display view, not
the complete mesh/source witness; current exact Budget is not thread-safe; source
extracted spans need original-coefficient discrepancy/enclosure semantics; a new
owned native geometry/decision contract must serve CPU and sm_60 bulk bounds,
with host exact fallback and no Python production callbacks. Native fixed-site
CDT and display kernels can be reused without copying the existing serial Python
algorithm as the whole architecture.

Coordinated shared-edge barrier/cell-bound semantics with algorithm owner and
source/corner/provenance/publication fields with oracle owner. Remaining whole
design decisions are named explicitly, especially canonical-boundary conformity,
actual-cell residuals, certified GPU decisions, full mesh schema and failure/
resource behavior. Parent owns synthesis, design review and later plan review.

Root review accepted native ownership, geometry/cache and P100 direction and
rejected the proposed required native independent checker. Valid finding: native
CLI ownership alone does not justify a second certification layer. Revised the
paper to remove `check_mesh` from required interfaces, make direct constructor
invariants/bound checks/inventory/atomic publication the production mechanism,
and retain Python/Fraction independent actual-output qualification off the product
path. A new native checker would need a distinct fault class and real consumer
need. Full-mesh fields carry geometry/source/approximation semantics, not a new
receipt protocol. BGSRC001 admitted degrees remain surfaces <=3 and curves <=32;
BGMESH01 remains a lossy display view. No product/test code changed.
