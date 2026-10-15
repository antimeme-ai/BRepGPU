# Canonical meshing implementation: geometry semantics review

2026-10-15. **Product source accepted after M3 correction.** Read complete
`python/brepgpu/meshing.py` and the lazy `BodySnapshot.tessellate` adapter against
the accepted design. No product execution or production edits occurred in this
review. Independent oracle/test source review was subsequently completed and
accepted after O1 typed-field correction; see the separate meshing-oracle
semantics review. This disposition does not claim fleet green.

## M3 — source vertex slots were mistaken for IDs

Blocking finding: the original `Edge.start/end` fields store zero-based source
vertex slots, as the original factory directly stores its `edge(start,end)`
arguments. Initial mesh code subtracted one for line polylines, closed ring
anchor endpoints and planar corner resolution. This introduced -1 or another
incorrect physical index and broke source geometry/topology correspondence.
Original `Vertex.id` remains one-based, a different namespace.

Reported to root before green/performance execution. Root corrected all three
conceptual sites to use source endpoint slots directly. Re-read actual code:
line indices use `(edge.start,edge.end)`, circle start/end use those same slots,
and corner resolution binds those slots at original parameters 0/1. M3 resolved.

## Geometry, orientation and error reasoning

The exact reader consumes original line/circle, UV-map, planar controls and
frame records as rational values. Uniform full-edge t=k/(4n) produces the exact
quarter geometry. First permitted dyadic n uses the declared conservative
R/(2n²) half-tolerance budget; the actual maximum exact L1 binary64 perturbation
is computed over every returned physical vertex. The final sum is checked and
failure publishes nothing. Source/frame preparation is not silently rounded.

Complete box/outer rectangle, cap-center fan, plate corner/side fan and wall
strip templates match the accepted family. Plate fan triangles have positive
canonical plane orientation and preserve the polygonal hole. Every negative
source face swaps both triangle indices and associated corner UV records.
Cylinder wall strips use matching top/bottom rings, consistent diagonals and
the same orientation reversal for the inward plate wall. Expected vertex and
triangle counts guard complete construction, without replacing qualification.

The rounded guard verifies a strict single quadrant traversal in addition to
positive polygon turns, paired top/bottom XY, common ring planes, ordered
heights and strict plate-hole nesting. It checks actual triangle normal signs,
nondegeneracy and common planar coordinate planes before publication. These
are the bounded prerequisites of the canonical embedding argument; independent
whole-domain boundary-chain qualification remains required.

## Identity and occurrences

All original physical vertices are allocated once. Each original edge adds its
interior samples once; ring closure aliases its actual original anchor. Typed
source IDs remain original one-based IDs while mesh indices remain dense slots.
Opposite coedge uses reverse canonical parameters/indices exactly once and read
their original lifted UV maps. Cylinder seam u=0/4 occurrences remain separate
in `BoundaryUse`, while wall triangle corners retain chart-local s and shared
physical vertex IDs. Face orientation swaps keep these attributes attached to
their correct physical vertices.

The immutable factory family supplies original edge/face/chart order and
canonical orthogonal planar controls; no arbitrary reordered/imported snapshot
contract is asserted. Bounds, recursive immutable tuples, factory-only mesh
publication and the CPU-only narrow scope match the design. No other product
geometry, source identity, bound or rounded-output finding was found.
