# Bounded GeomProjLib projection: selected source review

Read-only study, 2026-10-15, prompted by actual face170920/carrier4257's
ellipse boundary and its projected U excursion. Reviewed retained OCCT8.0.1
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`: GeomProjLib::Curve2d with explicit
curve range and surface U/V bounds; ProjLib_ProjectedCurve; the selected B-spline
path through ProjLib_ComputeApproxOnPolarSurface; GeomAdaptor_Surface loading;
Geom_TrimmedCurve construction. Restoration uses the existing quarantine
manifest. No probe, production adoption, source edit or tolerance change here.

## What explicit bounds do

GeomProjLib.cxx58–137 constructs a curve adaptor on the supplied First/Last and
a surface adaptor on the supplied rectangle. GeomAdaptor_Surface::load stores
that rectangle in adaptor fields; it does not trim or modify the underlying
surface. ProjLib_ProjectedCurve's B-spline branch (cxx455–535) sends that adaptor
to ComputeApproxOnPolarSurface. Initial Extrema searches use these bounds.
The later Function_Value search clamps U0/V0 seeds to this rectangle and builds
a bounded neighborhood for local/global point searches (polar cxx320–470).
Thus bounds do affect the search; they are more than unused hints.

They **do not guarantee an in-domain final pcurve**. Function_Value retains its
original initial-curve point and returns it when both searches fail its
orthogonality/improvement tests. The seed clamp does not clamp that returned
point. The final Approx_FitAndDivide2d is an unconstrained curve fit; its poles
are assembled into a new spline and smoothed by knot removal, without a final
rectangle/control-hull check (polar cxx2170–2300). Even in-domain sampled
projection points do not imply in-domain fitted coefficients or every curve
value. An actual domain check remains necessary; these source facts do not
authorize promoting a successful return to a bounded inverse.

## Original parameter and geometry

The general nonpole fit evaluates original C(t) at the function parameter t.
Its function FirstParameter/LastParameter return the input curve adaptor's
range (polar cxx482–522), and fitted segment parameters become output knots.
This is an intended original-t approximation, not arc-length retiming. The
initial point sequence also retains actual curve parameters. Source C/S
correspondence must still be measured at original t.

Reviewed branches read source geometry and produce new2D curves/adaptors;
no source curve/surface setter or original topological edge range write occurs.
TrimC3d changes the **working adaptor handle**, not the source curve. Where a
temporary Geom_TrimmedCurve is created, its constructor copies its basis.

There are two range hazards:

* At detected degenerate surface bounds, the projected-curve B-spline branch
  can trim the working curve ends by1% of the original range. ExtendC2d then
  adds derivative-directed2D line pieces to boundary lines and SameRange maps
  the resulting2D range back to original First/Last. This preserves source
  geometry, but does not establish exact C(t)/S(UV(t)) near the extension.
* GeomProjLib's final wrapper treats an input Geom_TrimmedCurve specially:
  its own trim bounds determine the returned2D wrapper, intersected with the
  new nonperiodic curve's bounds. Those input trim bounds need not equal the
  separately passed First/Last. Check actual result coverage/range; do not
  rewrite the original edge interval to accommodate the output.

## Tolerance and failure contract

Curve2d floors its in/out tolerance to PConfusion; ProjLib_ProjectedCurve then
floors its working tolerance to Confusion (cxx371). The polar initial projection
search admits a distance of100 times that working tolerance unless an explicit
MaxDist is set; this wrapper exposes no MaxDist argument. Its reached tolerance
comes from fitting/error conversion and is not the caller's independently
measured original C/S residual. Keep original edge/source budgets separate.

Null output is an ordinary documented failure. Other geometry/solver exceptions
can propagate; Curve2d supplies no comprehensive status object. Some early null
returns occur before the reached-tolerance assignment. A non-null result and
finite returned tolerance therefore still need original-t, finite, source
residual, branch/domain and topology checks. The actual ellipse experiment may
show whether this path is useful, but the supplied bounds alone cannot justify
domain recovery or relaxing the existing endpoint grade. A selected change
still needs its bounded written design and direct red/green implementation.
