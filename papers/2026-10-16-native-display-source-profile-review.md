# Native display source-profile review

Independent actual-source review of the changes in native/om10/math.hpp,
scene.hpp, types.hpp, cpu.cpp, cuda.cu and main.cpp. Root owns the direct CPU,
both-P100 and ABI-version tests; this review adds no execution campaign.

The profile feature/version checks preserve existing ABI structure sizes and
reject new semantic flags through the legacy factory. New source-profile
vertices require stored XYZ and cofactor normal transport. Position evaluation
uses stored mesh coordinates while analytic carrier derivatives retain the
original finite frame, rather than normalizing its axes into another surface.

For a sphere, the regular normal tends to north +cross(X,Y) and south
-cross(X,Y), times the declared regular-sheet sign. For a cone with
E=cos(u)X+sin(u)Y, the radial limit is
sin(alpha)*cross(E',E)+cos(alpha)*cross(E',Z), times the declared sign of
radius. These formulas remain correct for admitted nonorthogonal frames; face
sense is applied separately. The cofactor rows cross(row1,row2),
cross(row2,row0), cross(row0,row1) correctly transport an oriented source
normal under the admitted positive determinant instance. Shared positive
scaling changes no direction and avoids overflow in the cofactor calculation.
The normal-unit helper divides components by their maximum directly, avoiding
an overflowing reciprocal for tiny finite vectors.

Finding resolved: the initial source-profile admission required an independent
third Z axis even for plane carriers, although S=O+uX+vY never uses Z. A valid
plane with Z=0 was rejected and a huge irrelevant Z could make a well-formed
plane fail the shared scale guard. The new independent_plane_axes checks only
the scaled X/Y cross product, with a conservative uncertain-rank refusal; other
carriers retain their three-axis admission. Actual source reread confirms the
plane dispatch uses this helper.

ACCEPT for this finite display profile. Conservative frame guards can refuse
poorly scaled/near-singular inputs; they do not establish that an uncertain
rounded determinant is zero. This review does not approve mesher domain,
source-error or physical-embedding obligations.
