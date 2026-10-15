# Signed sharp-cylinder offset

`brepgpu.offset.offset_solid(source,distance)` builds a complete immutable owned
solid from an actual canonical cylinder. Distance is local, nonzero, on the exact
1/256 grid in[-1/8,+1/8]. The actual original bounded Frame remains; world distance
is distance times scale. Accept int/Fraction/finite binary64 excluding bool.
Other sources, zero, off-grid and wider/collapsing distances are unsupported.

```python
from fractions import Fraction
from brepgpu.body import make_body
from brepgpu.offset import offset_solid

source = make_body('cylinder')
result = offset_solid(source, Fraction(1,8))
solid = result.solid
assert result.join == solid.join == 'intersection'
assert solid.classify_point((Fraction(3,8),0,Fraction(5,8))).boundary_faces == (1,3)
foot = solid.source_foot(0, Fraction(3,4), Fraction(3,4))
assert foot.in_source_square and foot.source_trim == 0
```

The original local radius/half-height are1/4,1/2. Result radius R=1/4+d and
half-height H=1/2+d define the exact material x²+y²<=R²,|z|<=H. The join is sharp:
three normal-offset analytic carriers meet at their extended intersections.
For d<0 this equals Euclidean erosion in the admitted convex family. For d>0
it is not rounded Euclidean dilation: a new rim moves radially and axially by d,
with distance sqrt(2)*d to the original bounded solid. No general offset repair,
rounded rim blend, GPU feature construction or result mesher is established.

Two actual disk caps and four rational wall charts have changed exact dyadic
control nets. Original rawUV/Frame binary64 bits remain, with fresh result
Lineage. All new vertex/Curve/UVMap coefficients are Fractions. Source stays
unchanged; a fresh namespace owns2vertices/3edges/6coedges/3loops/3faces/6charts.
Two rim circles are ordinary opposed pairs, one seam has distinct same-face uses.
Circle start/end ports remain distinct at the same anchor. Complete words and
three-port vertex cycles close one genus-zero shell. Old canonical guards and
packed tags remain unchanged.

`classify(zero_based_chart_slot,u,v)` uses exact submitted values and returns
REJECTED/INTERIOR/BOUNDARY/INVALID. Nonfinite/out-of-unit-square is INVALID. Slots
0/1 are top/bottom caps,2..5 wall quarters. Actual physical rims/seam are boundary;
internal wall chart joins are interior. `classify_point(worldpoint)` returns
exact query and every bounded supporting face: top1,bottom2,wall3. At a rim both
cap and wall appear; the seam is an occurrence, not a second material face.

`edge_point(one_based_edge_id,t)` returns exact world points. Lower circle1,
upper circle2 and upward seam3 use intrinsic t∈[0,1]; circles have four rational
quarter pieces. `coedge_uv(one_based_coedge_id,t)` returns exact intrinsic raw
output UV at the same physical-edge parameter. Apply occurrence signs only for
loop traversal. `topology_input()` exposes actual result ABI5 sources/incidence
with snapshots empty. Exact world volume is Measure(0,2*H*R²*scale³); tight bounds
are original origin±scale*(R,R,H).

`result.history` has23 original records and23 modified output origins: each own
same-kind original has one image and no births. Source E/CO parameter maps are
`boundary_parameter` correspondence at intrinsic t, not spatial coincidence or
a single face-normal translation of the whole sharp boundary curve. Source UV
maps are `normal_foot` on an explicitly named original face/chart. Contextual
owners are not extra causal image parents. All maps retain absolute material/
traversal senses without applying them twice.

`UVMap2(kind,data,domain)` has exact required target argument domain. Affine maps
store six coefficients; circle maps store one radius ratio and use the full
rational circle phase. `.evaluate(u,v)` returns original RAW UV and permits output
outside original bounds. Wall face/loop map arguments use rawtau∈[0,4]; chart/CO
arguments use normalized/intrinsic[0,1]. Descriptor input and domain iterables
are read with fixed width+1 limits. Unknown kinds reject before consumption.

`source_foot(slot,u,v)` accepts only an admitted exact output-face query and
returns factory-immutable Footpoint with original face/chart keys, output chart
ID/query, raw sourceUV, exact WORLD source/target points, outward unit normal,
signed world distance, original-square membership and exact original trim label.
Its complete carrier relation is point=source_point+distance*normal. It evaluates
the actual original carrier coefficients with exact arithmetic, including their
analytic extension; it never submits extended UV to native code or uses the old
float-coerced source classifier as exact foot truth.

An outward cap foot may be inside the original square but outside its disk
(source_trim REJECTED); an outward wall foot can lie outside original V
(source_trim INVALID and in_source_square False). Inward feet remain bounded
original face points. At the sharp rim, the cap and wall have different source
normal feet; neither implies coincidence with the original physical rim. Original
square/trim admission is always reported separately from the carrier relation.

Publication validates strict original tables/native/packed bits, complete changed
carriers, literal whole circle/seam/bothUV identities, orientation/ports/links and
all original causal/map owners. Closed analytic substitution of full exact
descriptors proves normal-foot identities; sampled fits do not. Late failure
publishes no result and preserves source/prior outputs. Independent qualification
and native sampling/ABI5 consumers remain separate. Constructor timings name
admission, assembly/history, validation and total CPU cost; full useful callers
also consume feet/history/material, native setup/transfers/cleanup and topology.
Read OFFSET_DESIGN.md and OFFSET_PLAN.md for scope and qualification details.
