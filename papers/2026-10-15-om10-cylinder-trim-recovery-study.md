# Original cylindrical trim versus foreign pcurve diagnosis

Source: private OM10 STEP, SHA256
`c60675abe3314dca55a31da0406deef851e128e2ee8608536525f926f56d61e3`,
reviewed BGSRC snapshot already independently compared against all original
carrier/curve coefficients. Investigation executes on neuroses, not the laptop.
No production evaluator was used for these answers.

Face165590 uses original cylinder14608, radius2mm, source edge85311 and original
spatial cubic B-spline2098. Coedge112915 has a foreign reconstructed degree1 UV
curve over source t=[1.07550994781079,1.1269469671525], endpoints
(3.3876720513589067,~0) and(3.141592653589794,.22). The source supplied no pcurve.
The imported SameParameter flag is true and its edge tolerance is .006609455mm.
Neither establishes agreement at the selected .001mm source residual grade.

Independent original-knot rational Cox evaluation at the actual failing
`t=1.1207636976165989` gives original XYZ
(2.5115849061308473,.9050054694954496,.1943695920909052), matching the physical
source sample. Foreign pcurve(t)=(3.1711739797068934,.19355369118763882) has
original-cylinder residual .002472328592687374mm.

Invert the original cylinder from the original spatial point: subtract original
origin, project onto original X/Y/Z, take atan2(Y,X), lift by the integer period
nearest the same original branch, and retain projected Z. This gives
(3.172340819963917,.1943695920909052), with radial residual
.0000268980830712478mm. Thus this particular failure is foreign trim recovery,
not an original spatial edge that cannot satisfy the selected residual.

A1001-point independent source-t survey measured max nearest-cylinder residual
.00014095925176580893mm; the same-t foreign UV line instead reached
.006610206697880476mm. These are measured extrema, not global certificates.

Reparameterizing the foreign line is insufficient. Independent one-dimensional
minimization over its full parameter interval gives best residual
.0016974398432522274mm at the actual failing source t. At source
`t=1.0991709767079767`, same-t error is .006610206697880476mm, nearest-cylinder
residual .00013670660998044365mm, and best foreign reparameterization still has
.003741749257286026mm residual. The line's angular interval is only .2461rad;
for these points its squared cylinder distance is strictly convex because the
cosine term remains positive throughout, plus a positive quadratic height term.
The separated minima are therefore sufficient to reject scalar parameter
correspondence as the fix.

Required design direction: retain original C(t), raw foreign UV and actual
parameters, recover cylindrical UV from original C(t) with explicit provenance,
validate the unchanged source residual and preserve branch continuity. Bounds
for derived UV segments must be reconsidered: trusting the old degree1 pcurve's
zero UV chord error would be wrong. Original spatial-curve chord bounds plus
cylinder UV-linear chord bow and endpoint residuals can bound distance to a
recovered trim segment; own adaptive recovery must retain its source interval
and fail on budget/branch/residual violations. Original-vertex joint inversion
is the endpoint case, not a nearby-vertex weld.

Reviewed derived cylindrical UV chord bound: let B1/B2 bound the planar XY
projection of original spatial speed/acceleration on the retained source-t
interval, and rmin=|XY(C(mid))|−B1*dt/2. Reject/refine if rmin<=0 or total angular
variation B1*dt/rmin>=pi. Differentiating atan2 gives
`|u''| <= B2/rmin + 2*B1²/rmin²`; original-axis height gives
`|v''| <= ||Zcovector||*A2`. The carrier first-partial metric therefore bounds
UV-linear chord displacement by `(Lu*|u''|bound + Lv*|v''|bound)*dt²/8`.
Original endpoint-vertex incidence perturbations contribute the maximum metric
UV endpoint displacement. Bounds must be uniform over original source spans;
C1 piecewise-C2 curves admit the integral remainder, while genuine derivative
jumps need splitting. Branch/radius/finite checks fail closed. This replaces
trust in the foreign UV line's zero chord error without changing the quality.
