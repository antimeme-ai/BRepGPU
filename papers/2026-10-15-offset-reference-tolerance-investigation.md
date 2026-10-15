# Offset optional reference: tolerance and carrier source investigation

2026-10-15. Source-only follow-up after root's actual outset foreign run failed
its existing volume and boundary criteria. No geometry rerun, API/flag/accuracy/
threshold change, product edit, native build or dependency adoption. The saved
private probe reports63 witnesses, volume absolute error approximately5.03e-7
and boundary maximum approximately1.08e-6; final precise per-recipe exports will
remain authoritative. Both criteria remain FAIL. This investigation does not
reinterpret either as a pass or resolve earlier revolution issue43.

For outset, original local r=1/4,h=1/2,d=1/8 and world scale1/2 independently give
R=3/8,H=5/8 and world volume coefficient2*H*R²*scale³=45/2048 multiplying pi.
The selected comparison limit is relative1e-8, and boundary distance/residual
limit1e-7*scale=5e-8. No observed source/owned analytic arithmetic error is
identified by this failure. A returned reference volume is an adaptive numeric
integral over its actual reconstructed boundary, not exact truth for that formula.

One concrete query limitation is visible directly: public get_closest_point
calls SmApiGetClosestPoint, which calls topology BrepPointSolve(MINIMIZE,SINGLE)
with **hardcoded SM_ZONE_TOL_3D**. SmMath.h defines that constant as1e-5. The
Python function exposes no tolerance argument. Thus a returned closest point can
carry solver error much larger than the selected5e-8 outset limit. Recomputing
Euclidean residual to that returned point checks the actual returned answer;
it does not magically establish the exact mathematical distance to the foreign
boundary. A failed boundary distance/residual criterion can reflect query solve,
assembled geometry/trimming or both; it does not uniquely prove the exact foreign
carrier is displaced by the reported maximum. No alternate closest API was tried.

The construction path also is tolerance-based. SmOffsetExecutive::OffsetBrep
reads input Brep tolerance into SmOffsetGeometryCreation, chooses inset for
negative d and delegates extended offset. FaceuseOffset calls original surface's
virtual CreateOffsetSurface with requested maximum approximation distance equal
to that Brep-derived tolerance/10. The extended assembly creates/extends supports,
intersects and trims them, lifts curves with edge tolerance/3 and stitches/merges
using Brep tolerances. SmMath.h additionally defines approximation default5e-6
and intersection default2e-5, but this source inspection does not measure actual
runtime body/context tolerances or certify a global output error bound. Changing
volume integration request cannot tighten those separate construction/query
controls, and the selected Python offset call exposes no geometric tolerance.

Do not call every cylinder path a generic fitted surface. Public
SmApiCreateCylinder(solid) delegates equal-radii CreateCone; its full360 primitive
branch calls the analytic cone factory only under USE_NEW_ANALYTICS, otherwise
uses rotational sweep. SmCone::CreateOffsetSurface explicitly copies/updates
analytic radius and scales homogeneous radial control positions while preserving
parameter domains. In contrast the generic SmSurface offset wraps a copied
base in an implicit SmOffsetSurface; other B-spline paths can approximate when
required. Offset geometry attempts analytic recognition where possible. The
inspected source contains these branches, but neither an installed build flag
trace nor the actual runtime carrier type/selected branch was captured by this
bounded investigation. Therefore no unique fitting/extension/intersection/
integration/query component is identified as the cause of the observed failure.

ReplaceImplicitOffsets approximates implicit offsets to B-splines in the
shelling path; it must not be claimed as automatically executed by the selected
pure offset_brep path merely because it exists nearby. Extended offset itself
has explicit tolerance-based curve/SSI/lift/merge steps even when an underlying
support is analytic. Finite closest witnesses and an approximate volume alone
cannot distinguish support geometry from trim or query/integration error.

The volume API's separate requested relative accuracy clamps at1e-8 as established
in the previous investigation. Its nested area-derived integration scales are
not an enclosure of final global volume error. Neither that clamp nor the
larger source construction/query tolerances waive the declared comparison.
Preserve source retention/manifold/bounds/volume/boundary measured outcomes,
exact input/flags, all63 witnesses and nonzero overall failure. Observe all final
recipes once as planned, publish failed criteria honestly and track the owned
repository discrepancy. Do not tune a reference or change owned exact output to
match an approximate answer. Any later alternative adapter or explicit tolerance
experiment needs its own preselected scope and cannot retroactively green this
campaign. Required external alpha suites remain incomplete/non-green.

Pinned source additions are recorded in offset-runtime-sources.tsv; bibliography
in offset-runtime.bib. Public API source: [NVIDIA API](https://nvidia-omniverse.github.io/usd-brep/latest/docs/python.html).

Subsequent source-stage reporting finding: final odd recipe initially stopped at
finite original-source manifold/volume preflight before any offset call. Root's
reviewed adapter correction now preserves all3 source criteria, world inputs,
original probes and pins, then marks offset_attempted=False/output criteria
NOT_RUN on failure. This is an unqualified foreign source factory, not observed
offset output geometry. Final per-recipe reruns/precise outcomes are reported
separately; no threshold, API or owned geometry changed.
