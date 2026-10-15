# Convex extrusion design: runtime/integration review

Reviewed complete `docs/EXTRUSION_DESIGN.md` against pinned sweep studies and
actual owned Patch, profile/body records, Boolean consumer and ABI5 contracts.
**Accepted; no blocker.** Source review only, no implementation or execution.

The fixed source square carrier preserves actual original UV and XY control bits
on both translated caps. Lattice endpoint coordinates and centered height produce
exactly representable bilinear side controls inside the existing local envelope;
all patches retain the original bounded Frame. Positive h and minimum lattice
edge length avoid arbitrary vanishing wall normals. Exact global support tests
reject winding stars/concavity rather than assuming adjacent left turns imply
convexity. Vertex and coordinate iterables are bounded before allocation grows.

Distinct factory-only profile/solid/result types keep canonical and Boolean
admission unchanged. Source IDs determine copied layers, vertical shared edges
and swept sides; intrinsic source t/UV/axial maps keep occurrence sense separate.
The explicit lower/upper cap and side loop words give opposed edge uses and
outward material sense. Connected shell/vertex-link checks supplement whole-domain
embedding; area/volume/counts are not substituted for geometry. Six-kind histories
are internally complete:3N+3 modified source records each have two same-kind cap
images,6N+6 modified output entities and8N generated side entities.

Inspected the actual existing Boolean sampling private helpers: `_check` reads
request.result.solid, actual patches, indices and metadata; `_rows` reads the same
typed chart/face records and orientations. Neither embeds a Boolean geometry
predicate or canonical frame assumption. Reuse through a separate extrusion
request/public sampler guard is valid, without weakening existing `_result` or
public sample type checks. The same status/provenance/backend/profile-field
admission and primary-error-preserving cleanup apply. One fixed complete-query
budget accommodates up to34 charts without inheriting an accidental ten-chart
limit. Exact classify requests remain distinct from native float candidates.

Independent integer-delta sqrt intervals provide source normal truth without
using production normalization. Position/normal grades are stated directly at
actual copied UV. Native source snapshots, frames, Lineage and query bits bind
to the retained feature result; existing synchronous topology input has no new
pointer/native ownership requirement. Construction publication and later sample
batch failure remain separate, with prior result/Engine healthy retry preserved.

Actual triangle/chamfer/many-sided recipes vary topology rather than only scale.
Useful complete costs include CPU construction/publication, actual material/
history/boundary readers, native setup/transfers/normal rows/adjacency and cleanup;
independent checks/reference/export/startup remain explicit. Nested spans and
medians are nonadditive. Optional foreign curve inputs are documented copied,
direction/distance semantics are correct, and actual world-endpoint rounding is
recorded or avoided by dyadic recipes. No native build, dependency, mesher,
generalized architecture, GPU construction gain or external alpha waiver is
implied. Ready for the implementation plan.
