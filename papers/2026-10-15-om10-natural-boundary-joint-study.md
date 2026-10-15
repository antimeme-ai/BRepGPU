# OM10 natural spline boundary joint169982

Independent read-only original-source study, 2026-10-15, on the fleet. This is
diagnosis, not a selected recovery algorithm. Private input bytes remain ignored.

The reported nonclosing joint lies on face169982/carrier4256, a degree(3,3)
original tensor spline with7x4 unit-weight controls. V is clamped with endpoint
multiplicity4 at0.00999999999999996 and0.035. Control XZ coordinates across V
vary only by roughly1e-15mm; the four Y rows are-1.6,-1.68333333333333,
-1.76666666666667,-1.85mm. These saved coefficients must remain unchanged.

Original vertex76059 joins coedge138092/edge98001/curve2575 to
coedge138093/edge98002/curve2576. The former's active endpoint parameter is
0.025588974014147316; the latter's natural starting parameter is
0.0255992662053717. The source has a genuine parameter gap at the declared
shared vertex, rather than just differing binary64 UV evaluations. Retained
translated vertex tolerance is0.00021186340422014706mm.

Independent original-knot evaluation gives:

| Source endpoint | Distance to original vertex76059 |
| --- | ---: |
|C2575(active end)|0.000185231372716mm|
|C2576(active start)|0.000211863192357mm|

At the natural lower V boundary, source U values0.43225527967847305 and
0.4324291379188916 reproduce those spatial endpoints. Source tangent norms are
approximately0.591237mm per U and10mm per V, so this is not an ill-conditioned
inverse. A diagnostic one-dimensional closest-point iteration using independent
original-knot jets finds U approximately0.432255279678475, with minimum residual
0.000185231372719mm. That numerical observation is not a uniqueness proof or
an adopted product solver.

Whole active-curve correspondence was checked without trusting extracted spans
or point probes alone. Take the original carrier's clamped lower-V control row,
with original U knots, as a cubic boundary curve. Compose U with each retained
affine recovered guide U(t), regarded as a proposed map. Split the original
active spatial parameter range at both its original knots and the mapped
original carrier U knots. Exact Fraction Cox evaluation at four rational points
per interval recovers the cubic Bernstein coefficients of the spatial-minus-
boundary difference. Their maximum coefficient-vector norms are approximately
4.24e-15mm for curve2575 over three pieces and6.98e-15mm for curve2576 over five
pieces. Positive Bernstein partition of unity bounds each polynomial difference
by its maximum coefficient norm. Reported square-root norms are rounded numeric
values; the coefficient calculations themselves use exact saved rationals.

The two whole spatial curves therefore agree with the original natural boundary
to saved-coordinate precision under their distinct affine parameter maps. Their
shared canonical vertex departs from that boundary within retained source
tolerance and the selected0.001mm source-residual limit. Any accepted joint
construction must still account for endpoint UV displacement in the trim bound
and canonical XYZ displacement in the physical triangle bound. Choosing a UV,
admitting an inverse family, or changing global numerical grades requires the
owner's written design and review; none is selected by this study.

Review also exposed a separate existing world-bound omission. Bounding original
C(t) against the chord joining ideal C(a),C(b) does not bound it against a chord
whose endpoints are canonical original vertices P(a),P(b). The necessary direct
triangle-inequality term is max(||C(a)-P(a)||,||C(b)-P(b)||), added to the ideal
spatial chord bound. The face cell's canonicalXYZ-minus-S(UV) term does not
generally cover this: take a plane at z0, a source LINE at z=epsilon, and declared
endpoint vertices on the plane. Ideal curve curvature, UV chord departure and
surface-canonical displacement all vanish, while the original spatial edge is
epsilon away. Source residual is an input-consistency limit, not an extra mesh
error allowance. Add actual measured endpoint departure if this world-bound
contract is completed; do not add the selected source tolerance wholesale.

The reviewed implementation subsequently selected only between the two retained
admissible endpoint UVs under original clamped-cell/natural-V/source guards,
with explicit original directed-joint provenance and pre-sampling trim endpoint
corrections. It also completed the measured original-world endpoint chord term.
No closest-point solver was adopted. Independent checks use original-knot
evaluation and actual source displacement, rather than the product metric bound.
Seven direct/affected geometry-native methods pass, including deliberate wrong
candidate/identity/raw-record and hidden original-edge faults. This bounded
result leaves complete original-source/closed-solid/native qualification
pending before the actual watch render.
