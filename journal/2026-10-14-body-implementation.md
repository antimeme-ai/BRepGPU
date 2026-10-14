# Immutable body implementation

Root accepted BODY_SAMPLING_PLAN.md and explicitly authorized this lane to
implement python/brepgpu/body.py only, plus this journal. The fleet first
direct red created the existing Engine and then failed because brepgpu.body
was absent. No native machinery is changed. All product execution stays on
the fleet; local inspection is static/AST only.

Implementation subplan: factory-only canonical box/cylinder/plate snapshots,
frozen tuple records and explicit little-endian packed tables. One body frame
and monotonic process body namespace accompany typed IDs. Build shared outer
box edges, one closed edge per circular rim and one same-face two-use seam;
store explicit canonical-t UV maps and ordered loops. Four cylinder charts
reference one face and map one-to-one to native patch slots. Implement exact
integer submitted-UV trim signs, existing Patch/Lineage output and approximate
line/circle/UV evaluation. No sampler/native/oracle/caller files are owned here.

Canonical handoff agreed with coordinator/oracle/runtime owners: Curve line
data6XYZ doubles and circle(radius,z); UVMap line4UV doubles,
circle_plane(radius,half_width), circle_wall(v), seam_wall(a). Vertex.position;
face kinds rectangle/disk/rectangle_hole/cylinder. Plane axes top/bottom x,y,
front/back x,z,right/left y,z, all increasing; orientations +,-,+,-,+,-.
Chart slot equals patch slot; typed entity IDs are slot+1 within kind.

Whole body.py source is written. It supplies Frame validation, factory-only
BodySnapshot, canonical typed immutable geometry/topology records, exact integer
trim membership, canonical boundary evaluation, four-chart wall patches and
existing native lineage/frame descriptors. A single closed ring edge and its
anchor vertex are shared; cavity-wall traversal reverses the outer-wall loop.
Packed exports document every little-endian field/tag and adjacency range;
TABLE_LAYOUTS exposes those schema strings for independent exported-byte checks.
No runtime handles are retained inside the body. Body ID allocation is locked,
monotonic and does not recycle failed construction IDs.

AST parsing passed under Python3.11; product was not imported or executed
locally. Interface/schema handoff is ready for independent caller/oracle owners
and coordinator fleet integration. Implementation is pending executed direct
tests/source review; no geometric or hardware green outcome is claimed yet.
