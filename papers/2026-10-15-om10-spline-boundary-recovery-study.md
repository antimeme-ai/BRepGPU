# Original spline natural-boundary recovery

Reviewed OM10 source SHA256
c60675abe3314dca55a31da0406deef851e128e2ee8608536525f926f56d61e3.
Independent fleet investigation uses original-knot rational surface answers and
retained original spatial LINE coefficients, not product inverse helpers.

The four current failures172494/172983/173009/173010 use carriers4415/4747/4760/
4761, degree(2,1), shape6x2, clampedU=[0,4], V=[-.006,.01], allunitweights.
Each U row's two V controls share saved X/Z and have Y=.19/.35. At either
clamped U endpoint the original tensor surface selects its first/last U row.
It is therefore an exact affine map of V along that original control column.

Eight incident sideLINE occurrences were checked independently. Original LINE
spatial directions are exactly(0,1,0); their saved X/Z and original endpoint
vertex X/Z exactly equal the selected first/last source-control column. Recover
V by the exact original column affine inverse, not by assuming the intended
parameters are0/.002. The actual recovered V values are approximately
6.93889390390723e-19, .0010000000000000018, .0020000000000000026 at endpoint,
midpoint and endpoint. Independent original-knot S(Uendpoint,V) recovers their
saved original spatial coordinates with zero binary64 comparison residual.
Both original LINE C(t) and this surface-boundary S(Ufixed,V(t)) are affine, so
endpoint norm residuals bound the entire source-t interval. This is a direct
whole-edge mechanism, not evidence for a general spline inverse.

The triggering foreign recovered U lies1.17e-13..1.52e-13 beyond the original
U domain, exceeding the existing128ULP admission grade. Some triggers occur on
an adjacent degree2/3 spatial-curve endpoint, rather than the sideLINE itself.
That endpoint is the same ORIGINAL vertex incident to the proven sideLINE and
shares its original column coordinates. Its canonical source-vertex UV and
joint provenance must therefore be recovered too. Merely recognizing LINE
interiors would leave those endpoint failures unresolved.

Keep generic outside-grade rejection. A typed original natural-boundary LINE
profile may own its U/V samples after a source-coefficient affine endpoint proof;
a typed original-vertex/joint recovery must retain incident original coedges,
original endpoint parameters, raw foreign UV and the canonical source-column UV.
Require unique source-column selection, clamping multiplicities, degree1/two
unitweight V controls, nonzero column direction, V inside original domain and
unchanged world residual. Existing full positive UV chains, shared physical
keys, source approximation and native rounded source checks remain the output
obligations. No source vertex is moved and no cell is dropped.
