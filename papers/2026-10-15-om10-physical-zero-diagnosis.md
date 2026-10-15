# Actual physical-zero diagnosis

Independent nonvisual investigation on the fleet, using reviewed OM10 source
SHA256 c60675abe3314dca55a31da0406deef851e128e2ee8608536525f926f56d61e3.
The full old scan preceded owned cylinder trim inversion. Representative faces
were rerun against current production source; exceptions exposed actual mesh
records without changing the mesher. Expected carrier/curve positions use the
independent original coefficient evaluators.

## Plane166230: exact original collinearity, foreign UV sliver

Original vertices68803,68826,68800 have saved XYZ respectively
(3.14824785219045,-.87,-11.7860620895614),
(3.14824785219045,6.6441495727456e-17,-11.7860620895614), and
(3.14824785219045,-.3,-11.7860620895614).
Their exact rational cross product is zero. These are three original physical
vertices, not generated interior values or a nearby-vertex merge.

Foreign UVs on original plane161844 have U=.22937457609191964 for the first
two, but U=.2293745760919197 for the third. The one-ULP difference creates a
positive UV sliver whose physical image is exactly collinear. Independent
original plane evaluation at these UVs also rounds all X/Z to the same values.

Source-based plane affine inversion is the appropriate correction before
triangulation. Exact rational affine inversion preserves original saved-XYZ
collinearity in the owned UV partition; ordinary float inversion can recreate
small noncollinearity on oblique planes. Keep original point identity and all
boundary occurrences. Removing the resulting triangle after triangulation
would break the positive-domain claim and is not an accepted repair.

## Cone166156: foreign seam gap causes sub-ULP interior construction

Original cone1217 has origin(0,4.53175,0), Zaxis(0,1,0), radius.38675 and
semiangle.78539816339745. The upper source circle6549/edge86914 has constant
saved Y=4.5935; lower circle6550/edge86916 has constant saved Y=4.47.
Their original cone inverse V values are respectively +/−.08732768747653874.
Measured source radial residuals in the diagnostic are below2.3e-16mm.

The foreign upper-circle pcurve instead has V=.08732768747653862, while the
adjacent foreign seam ends at V=.08732768747653857. That few-ULP UV mismatch
creates a sliver near the original constant-height circle. Refinement produced
two distinct interior rational UVs differing by2^-52 in U and3*2^-60 in V;
both round to the same binary64 UV and evaluate to identical physical XYZ
(-.37291412111769145,4.5935,.24917324950929182).

Thus the observed physical zero is an interior representation collapse induced
by foreign trim inconsistency. Original-source cone inversion at original
vertices and original C(t) boundary samples produces consistent constant height
before triangulation/refinement. Rerun that correction before considering any
extra construction rule. No collapse exemption or dropped cell is justified.

## Cylinder166811: stale physical-zero result

Current owned-cylinder-inverse code does not reproduce the old physical-zero
exception. It fails earlier with edge parameter subdivision exhausted binary64.
That separate current interval/bound issue is being investigated by the source
owner. It must not be described as current evidence for physical-zero handling.

Subsequent correction removed foreign-only guide knots from the owned cylinder
sampler while retaining original spatial knots and exact whole-interval span
ownership. Face166811 then passed with14,218 UV cells and corrected residual
3.5747e-5mm. Plane166230 and cone166156 subsequently passed their source-based
recovery, respectively6 and99,384 cells, without a physical-collapse exemption.

## Later source-owned axial cylinder cases

Current scan faces171110/171115 contain original LINE edges100910/100925.
Three shared spatial samples have exactly identical saved X/Y and differing Z;
owned floating atan2 inversion gives one-ULP U variation, so this is no longer
foreign UV data. The generic bound used XY-operator-norm times full3D LINE
speed, even though its actual projected XY direction is negligible. For original
LINEs, q'=dualXY*sourceDirection is known and constant, and q''=0. Its actual
projected speed supplies the angular bound directly. This removes artificial
curvature-driven interior samples without snapping U or exempting zero cells.
Shared-edge sampling is a union across incident carriers; other incident needs
must still be checked. Physical-zero rejection remains unchanged.

The five construction-cap failures170998/170999/171000/171002/171003 are ordinary
thin full-period rings with radii4.986–5.45mm, not unusual singular geometry.
Their last reported triangles have angular widths roughly.058–.089rad, only
modestly above the required angular interpolation resolution. The million-node
cap remains. Investigate actual initial triangulation, selected edges and error
reduction before choosing curvature-aware refinement or connectivity improvement;
raising the cap would hide the problem.

The collapsed-cell production guard must test every repeated physical key, not
whether any corner happens to be a declared pole. The independent mesh checker
already requires every repeated key to be an original supported singular vertex.

Read primary triangulation guidance while reviewing the connectivity remedy:
Shewchuk, [Triangulation Algorithms and Data Structures](https://www.cs.cmu.edu/~quake/tripaper/triangle2.html),
1996, explicitly represents boundary constraints separately and prevents flips
from removing them. Its triangle-based adjacency discussion fits the existing
active-triangle/edge-incidence machinery; no new general mesh framework is needed.
[Correct Adaptive Tests](https://www.cs.cmu.edu/~quake/tripaper/triangle6.html)
explains why orientation and incircle determinant signs need reliable arithmetic,
and distinguishes robust predicates from representability of constructed points.
For owned saved rational UV, uncertain or rational-coordinate predicates should
use exact arithmetic. A metric-Delaunay flip should only replace the diagonal of
a strictly convex pair of positive cells, preserving their union, boundary chain,
source points and orientation. Cocircular ties need no flip; work must be bounded.
This is an investigated direction, pending actual initial-fan diagnosis and
reviewed implementation plan, not a selected dependency or performance claim.
