# Cut-result mesh caller/export: independent semantics review

2026-10-15. **ACCEPTED; no blocking findings before fleet measurements.** Read
`examples/mesh_cut.py`, `benchmarks/cut_meshing/run.py` and the bounded shared
`examples/mesh_body.py` extraction against actual source record definitions and
the accepted design/plan. Static review only; no product/reference/test execution.

The caller constructs actual box/cylinder operands and difference, passes that
same CutResult into meshing, and checks retained result/body/history identity.
The indexed reader consumes actual exact/source/corner/edge/coedge records and
packs the saved rounded XYZ, indices, result face IDs and geometric facet normals.
History consumption resolves all original source entity records by typed
namespace/kind/ID, traverses forward and reverse relations, and reads complete
origin fields, including source ranges/domains and section/event/map references.
It reads those metadata and references; it is not another Boolean construction
or a GPU history traversal.

Exports keep original operands in distinct directories, including actual original
factory packed tables and controls. The result has its own actual record tree,
per-patch frames, raw band and controls, followed by retained section, arrangement,
selection and all-kind forward/reverse history trees. Source-body references in
trees resolve through the separately retained original namespaces. No plate
factory or fabricated packed result supplies geometry. The tree writer handles
the actual tuple/dataclass/Fraction/native scalar structures and fails on an
unsupported field rather than silently stringifying arbitrary objects.

PLY reads actual indexed rounded arrays and uses round-trip binary64 decimal
coordinates. SVG derives its two views from the same arrays, retains the actual
hole and marks the one physical seam with two occurrences. Labels distinguish
result topology from original operands and retain original carrier UV versus
intrinsic result-edge parameter meaning.

The shared helper extraction leaves canonical source writing, wrapper behavior,
default shape/namespace, TSV headings and SVG captions unchanged. Cut-specific
shape/namespace values are explicit arguments; no canonical product admission
or geometry implementation is widened.

Each benchmark case prepares one actual result, then measures a first mesh,
one excluded warmup and five repeated complete mesh/indexed/history consumers.
The useful wall is directly wrapped, not a sum of medians. Independent first
qualification and full later mesh/buffer/history correspondence happen outside
that wall and receive separate timing fields. Export retains the first qualified
output after all useful repeat samples. Operand/Boolean/export phases are single
observations, and case variation changes resolution/frame rather than Boolean
complexity. Source pins are checked before and after the campaign. Metadata
explicitly excludes imports/final output bookkeeping/shutdown and makes no native,
GPU or full-CAD speedup claim.

Saved artifacts and numeric interpretation still require inspection after actual
fleet execution; this acceptance authorizes neither unobserved pass results nor
an external-suite/alpha-green claim.
