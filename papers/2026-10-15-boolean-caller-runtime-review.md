# Boolean actual caller/reference/controller source review

Reviewed `examples/cut_bore.py`, `benchmarks/boolean/run.py` and
`scripts/reference_boolean.py` as one complete consumer/cost unit after the
sampler's direct CPU/P100 greens. **Accepted; two bounded findings were fixed.**
This review performs no modeling/reference/benchmark execution or source edits.

Reference finding 1: first-eight admitted grid rows mostly repeated source outer
edges/seams. Root changed the helper to up to eight deterministic spread native
INTERIOR rows per face and requires all seven faces. Ninety exact edge witnesses
remain separate. This now compares actual interior source-carrier samples as
well as complete physical boundary representatives, rather than disguising a
second edge-only set as face-interior correspondence.

Reference finding 2: zip alone could accept a truncated bounds return. Root added
finite exact 2×3 shape admission before component comparisons. Re-read both fixes;
accepted. Reference construction uses the identical recorded frames/dimensions,
copies actual box/cutter before difference, and checks original input volumes
before/after. Actual public volume(relative_accuracy), bounding_box(tight),
is_manifold_solid() and get_closest_point returning (point,distance) match the
available reference API. Relative volume and scale-aware boundary limits are
declared. Owned/reference volume, manifold/bounds and closest-boundary checks
remain distinct from missing foreign material/history comparisons and external
alpha corpus qualification; those limitations are explicit.

The useful caller timer begins after separately recorded operand construction
and includes the actual owned difference, material/edge/UV/history reads, request
preparation, Engine creation, per-call native sampling/normal/checksum consumer,
Engine cleanup, actual topology packing/library load, synchronous ABI5 build and
actual mate/neighbor reads. CPU constructs every result; CPU/P100 choice applies
only to the declared native consumers. Exact result/sampling/topology truth work
and all record/SVG output are outside that timer. Nested construction/sampling/
adjacency phase rows overlap their enclosing stages and must not be summed with
them. Fresh processes/contexts, excluded warmups and exploratory three-sample
ranges do not establish resident throughput or whole-CAD speedup.

The command binds returned result operands and sample owner/request/placement,
checks all returned native statuses inside sample(), consumes every admitted
position/normal and resolves all actual adjacency pairs/mates/neighbor faces.
Source and pre/post native hashes, requested/loaded library path agreement,
controller before/after hash, exact operand packed-table hashes, original graphs/
carriers/source maps, full history, candidate bits/rows and actual input/CSR reader
binary arrays support direct inspection. No canonical packed result tag appears.

The source-derived SVG is emitted after independent qualification: exact physical
edge maps and actual admitted native samples/normals feed its wireframe/point
display. It states that no result tessellation is implemented. This scope fits the
existing capabilities; the graphic does not establish occluded surface coverage,
an exported triangle solid or a new meshing operation. Final numerical/cost claims
remain pending the actual qualification/campaign records.
