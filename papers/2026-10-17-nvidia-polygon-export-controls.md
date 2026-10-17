# NVIDIA polygon-export controls: primary-source correction

Reference: NVIDIA USD/BRep c9c5979f04cea8c38213004d4f04df0dcfb40d05,
[SmApiTessellationParams.h](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SM_API/inc/SmApiTessellationParams.h),
[SmApiBrep.cpp](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SM_API/src/SmApiBrep.cpp),
[SmBrep.cpp](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/src/SmBrep.cpp),
[SmPoly.cpp](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SMLib/src/SmPoly.cpp),
[SmuTessellate.cpp](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/BREP_SM_USD/src/SmuTessellate.cpp).
Read the actual local primary source, not archived agent instructions as policy.

The shipped C++ defaults enable25° curve tangent and25° surface-control-polygon
turning limits. Our reference caller overrides both to zero (disabled). The
absolute .005mm chord remains enabled, but it is not the same control as angular
subdivision. All enabled constraints contribute, with curves and surfaces using
separate drivers. Thus prior observed crossed SDK rings are genuine outcomes
of the explicit0/0 policy, not evidence of failure under the documented standard
controls. A predeclared25/25 control is the smallest meaningful usage correction
to examine before writing a new polygon exporter.

The official planar triangulator excludes self-intersection; its nonplanar
counterpart expressly warns against spirals and skips holes. The official USD
mesh converter retains arbitrary polygon counts and only the outer loop, with
float output. Neither source establishes a valid triangle-only escape from an
invalid ring. No fan, source-ID dispatch or owned-mesher substitution follows.
The documented15° cap/fillet workaround is a separate known issue, not selected
as a tuned second angle here. The finite plan first tests the single documented
25/25 control on both retained components and healing policies, preserving
actual face ancestry/normals and existing complete-output/physical gates.

### Finite-plane import divergence

At pinned c9c5979f04cea8c38213004d4f04df0dcfb40d05,
OcctStagingDriver.cpp::ComputePlaneLoopDomainFrom3dBoundary (1531–1585)
projects25 samples per whole original3D edge. DriveFace (2275–2315) uses this
box unconditionally for Planes and ignores source pcurve extents. In both
retained inputs, actual imported boundary samples extend2.3–2.4mm beyond two
finite plane patches while their support-plane residuals remain rounding-size.
The SDK's own lazy normalized UV evaluations return success but exhibit the
same millimetre-scale Ce/S discrepancy. This precedes tessellation and explains
why tightening angular constraints alone cannot restore the missing domain.
No universal root-cause claim about all SDK polygons follows.

SmuConvert.cpp deliberately defers UVTrimCurve creation; absent cached curves
are therefore expected. The original-source comparison preserves directed use
orientation and distinguishes a rationalized circle clock from a different
physical curve: all four circle images and angular endpoints agree within
rounding in the finite sample. An external conservative plane-envelope control
requires review; a sampled-box expansion or crossed-ring triangulation is not
justified by these observations.

### Selected conservative-envelope control

The actual repair changes only Plane face:range entries, before import. The
native helper exports untouched positive homogeneous B-spline controls and the
final represented SmPlane canonical frame. Exact rational H/W control bounds
contain each admitted natural-domain curve by the positive-basis convex hull
property. A definition-wide3D AABB also includes source vertices. Its8corners
bound each affine plane-coordinate functional, computed using the exact Gram
dual of that represented frame. Union with the previous range and outward
binary64 endpoints cannot narrow the mathematical support domain.

Pinned SmuConvert.cpp's Plane branch uses SetCanonical, CreateCanonical,
UVScale1 and AdjustSTEPUVDomain. The latter updates the finite support domain
without Reparameterize's affine coordinate scaling. The helper captures the
final frame after CreateCanonical's second canonicalization. This mathematical
range enclosure is not a proof about every rounded SDK dot/projection routine;
the finite reprojection and unchanged strict mesh gates still apply.

All four retained input/healing controls now pass unchanged raw export and the
existing closed-shell/source-Euler mechanism. The public helper/entry repeated
the two heal0 controls successfully. One independent authored cylinder with
positive rational cap boundaries exercises insufficient support→restored
literal circle support→actual strict closed mesh. No triangle-admission rule,
SDK binary, source support plane, curve, topology, ID or tolerance changed.
Only finite carrier domains widened. The requested .005mm reference policy is
still distinct from independent whole-model distance qualification.
