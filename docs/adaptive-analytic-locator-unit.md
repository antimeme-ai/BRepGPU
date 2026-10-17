# Reconstructed original analytic locator recovery

2026-10-17. DESIGN and finite implementation PLAN; product edits await review.

## Problem and contract

The current watch reaches a reconstructed cone use whose linear UV guide gives
an original Ce-to-S node enclosure upper of .00127181782942405977 mm, exceeding
the unchanged .001 mm source-node cap. Root's private samples find about
.00128779 mm guide departure but less than 4.5e-7 mm approximate nearest-cone
departure; those samples diagnose a poor proposal, not an acceptance proof.
Endpoints agree near roundoff. The declared edge tolerance is not the node cap.

Extend the existing FaceLocator proposal stage for cylinder, cone, sphere and
separated-ring torus. Keep exact plane recovery and the explicitly separate
reconstructed rational guide/natural-knot-rectangle rule. No source mutation,
source-ID selection, tolerance increase, bank expansion or general NURBS inverse.
Authoritative uses do not enter this recovery. Original mapped s, periodic_shift,
branch metadata, guide samples and raw curve values remain retained unchanged.

One proposal is eventually emitted into the existing carrier-sample task table.
ONLY complete_trace_sites establishes the original V/Ce/S/declared-alias guards;
Q remains excluded from those source-only guards. The existing physical chord,
selected affine trace-to-SAME-Q chord, authoritative policy and whole-use image
operations remain unchanged. A good node locator does not prove a whole trace,
source injectivity, domain coverage or a globally closest point.

## Sources actually read and selected differences

* OCCT 8.0.1, b8f597c677811d1f9f4d8a97f5ae2825c0353a42,
  ElSLib.cxx1558–1710: CylinderParameters, ConeParameters, SphereParameters,
  TorusParameters. Local inverse angles and cone generator projection are useful.
  gp_Ax3's orthonormal frame and normalization to [0,2pi) are not our contracts.
  Our literal original frames can be nonorthogonal, reflected and scaled, and an
  occurrence's lifted UV is retained. The torus reference's spindle branch is
  unnecessary: SourceCache currently admits abs(R)>abs(r)>0 only.
* SISL e0a82b1a851e932657f3e1af25033fbbcd33e907,
  s1773.c190–260 and s1773_s9dir368 onward: bounded tangent-plane iteration and
  its 2x2 Gram system. We use a bounded distance-decreasing proposal correction;
  we do not inherit its minimum status, direction-based acceptance, geometry
  epsilon or degenerate-branch arithmetic.
* Hu/Wallner2005, section3, as read in the retained projection study and original
  PDF (papers/projection-numerical-pdfs.tsv): regular-chart tangent projection is
  useful near an on-surface point; it supplies no singular/pole or global inverse
  guarantee. The selected first-order correction claims neither.
* Bertsekas1982 sections1–2 and Ferry/Gill/Wong/Zhang2024 projected-path discussion,
  retained in the same catalog: clipping a scaled step need not preserve descent.
  We shorten a feasible straight step and require actual proposal residual
  decrease; we do not borrow an asymptotic convergence theorem.
* Product source: adaptive_traces.cpp::FaceLocator/target_point/guide_point/
  prepare_trace_sites/complete_trace_sites; geometry_analytic.cpp300–342 and
  adaptive_numeric.hpp::evaluate_sample_rn64 define the literal source formula.
  SourceCache::admit_analytic admits nonsingular original frames and the finite
  analytic radii/sheet profiles. Existing seam-wedge construction owns the two
  pole sectors and subsequent meridian constant-U selection.

The improvement over the references is small and specific: preserve the source
chart/occurrence, use its actual frame metric, and let the already implemented
source bounds decide acceptance. We adopt no reference code or dependency.

## Literal source, proposal seeds and finite branch rule

Write S=O+A(v)(cos(u)X+sin(u)Y)+B(v)Z. The original families are

| Family | A(v) | B(v) |
| --- | --- | --- |
| cylinder | r | v |
| cone | r+v sin(alpha) | v cos(alpha) |
| sphere | r cos(v) | r sin(v) |
| torus | R+r cos(v) | r sin(v) |

Once per face, form the exact cofactor dual of F=[X Y Z], using Rational source
bits and the already admitted nonzero determinant. Convert its action on the
ORIGINAL target to finite binary64 local coordinates q=(x,y,z). The target is
the existing exact original Vertex or midpoint of the original Ce enclosure,
never constructed Q. Exact plane Gram projection remains its existing path.

Retain the guide candidate first. Analytic inverse seeds use ordinary atan2,
hypot and trigonometric proposals; they do not establish source equality:

* cylinder: u=atan2(y/r,x/r), v=z;
* cone: consider signed radial sheets through u=atan2(y,x) and u+pi. For each,
  E=cos(u)X+sin(u)Y, D=sin(alpha)E+cos(alpha)Z and
  v=((P-O-rE) dot D)/(D dot D). This is projection onto that physical generator,
  including a nonorthogonal original frame. The corresponding sheet is not
  inferred from a positive-radius convention;
* sphere: divide q by signed r, then (u,v)=(atan2(y,x),atan2(z,hypot(x,y))) and
  the reflected chart representative (u+pi,pi-v);
* torus: sign of its nonzero radial factor is sign(R). Set u=atan2(sign(R)y,
  sign(R)x), a=x cos(u)+y sin(u), and v=atan2(z/r,(a-R)/r).

For every trigonometric coordinate choose the nearest representative to the ORIGINAL
lifted guide, with its two adjacent winding candidates. Period arithmetic here
is a binary64 proposal only, never an exact symbolic-period proof. Choose the
nearest finite representative in the fixed original bank rectangle R; exact
Rational comparison of final stored binary64 coordinates gates eligibility.
Sphere V also has equivalent trigonometric seeds even though a V shift is not
an admitted periodic coedge operation; this never changes the semantic periodic
axis flags. Do not enumerate an unbounded winding set. Keep the existing argument-range
admission. Do not normalize stored U/V to a principal interval or rewrite shifts.
For sphere's reflected alternative select the guide-nearest chart seed, using
lexicographic UV as a deterministic tie. Cone's two sheets follow the same
guide-nearest rule. The original guide is always the fallback.

An inverse outside R is skipped, not clamped to a supplied narrow bank. No later
operation widens R. A valid guide at an exact profile endpoint can survive while
an inverse adds only rounding noise. The rational natural-R projection remains
its separate existing policy; this analytic unit does not generalize that clamp.

## Bounded correction, candidate selection and pole sectors

Use at most two starts: the original guide and the selected analytic seed. Each
has at most8 accepted Gauss–Newton steps and8 halvings per step. Evaluate only
the literal analytic proposal formula above and its D1 factorization:
Su=A(-sin(u)X+cos(u)Y), Sv=A' E+B' Z. This arithmetic is a private proposal
calculation, not a second accepted source evaluator or a new numerical profile.
Finite checks and work charges apply to every trial. Scale the two tangent
columns by their finite norms, solve the 2x2 normalized Gram system when positive
and finite, and otherwise try its two coordinate descent directions separately.
No small determinant implies invalid source. A failed direction just stops that
start. Take a feasible straight step, shorten before hitting R or the guide's
nearest angular branch window (guide +/-pi), and accept only strict decrease of
the ordinary squared physical residual. No UV snapping to R, source tolerance
stopping criterion, convergence label or computed normal is used as acceptance.
No motion, nonfinite arithmetic or the finite trial cap stops improvement.

Rank the final heuristic candidates by physical residual, guide-distance, then
lexicographic UV. Before replacing an in-bank guide, compare it and the proposed
winner using the EXISTING common RN source sample kernel with the SAME original
descriptor and ORIGINAL target enclosure. Replace only if the winner's residual
Euclidean upper is below a valid guide residual lower (maximum per-axis interval
separation suffices). This comparison protects a valid near-exact endpoint from
libm-only noise; it still does not accept a node. If the comparison is unresolved,
keep the guide. An out-of-bank guide may be replaced by an in-bank inverse;
otherwise the existing later profile admission refuses it. There are at most
two extra common RN sample evaluations per source site, not a PatchSession,
numerical cache rebuild or exact transcendental search.

Identify the existing possible three-use wedge's source endpoint incidence
before analytic proposal iteration. Freeze U to EACH original occurrence's
guide for its two shared pole endpoint sectors. Never infer a sector from
atan2(0,0), merge the two UVs or replace the actual finite endpoint by an ideal
pole. If local x=y=0 on another analytic source endpoint, also keep its guide U;
rank-deficient tangent iteration may still improve V. Existing wedge processing
then takes each meridian's constant U from its recovered regular-side endpoint,
checks strict V order, and captures the actual finite connector. All cap map,
sheet, source-role, finite-band and BOTH-side radial history guards remain in
their current producers. No cap admission is relaxed by this locator operation.

## Ownership, interface, implementation order

Own adaptive_traces.cpp's private FaceLocator and preparation sequence, the
existing native_adaptive_traces.cpp fixture and native_adaptive_trace_fixture.py,
this plan, a focused paper and journal. No public staged API, result row, CUDA
ABI, runtime phase, bank factory, cap domain or renderer change is needed.
Keep Rational owner-local and emit the same one final carrier task per merged
ordinary joint/explicit wedge sector. Exact table identities and stage ordinals
remain unchanged. Existing ordinary junction policy still merges candidates
before source sampling and validates all incident original obligations.

1. Review this complete unit. Add finite source fixtures and observe actual old
   prepare/complete trace source-guard refusal before body. No absent-symbol
   fiction is needed; missing behavior is visible through the existing operation.
2. Factor existing wedge endpoint incidence before proposal recovery without
   changing its source admission. Implement exact original-frame dual, seeds,
   bounded physical correction, bank/branch eligibility and guide preference.
3. Execute the same staged source operation on the fixed direct cases below,
   then request source/math review from the existing numerical/runtime peers.
4. Upload one coherent traces/fixture group. Root rebuilds/pins the model and
   retries the SAME original watch/quality; no private tolerance or source edits.

## Direct oracle and bounded checks

Extend the SAME original-source trace fixture with four analytic rectangles,
one per family, using original analytic iso curves/spatial generators and a
reconstructed guide with a deliberately different nonlinear clock. Endpoints
agree; interior raw-guide error exceeds delta while original spatial points lie
on the literal original carrier. Request one canonical original interval split
through the real DefinitionBoundaryForest before the direct trace operation so
the tested inventory includes an interior node; endpoints alone cannot expose
the missing behavior. The original source sample enclosure and final
node_source_upper<=unchanged delta are the direct oracle, followed by existing
whole selected-trace-to-SAME-Q bounds. Use signed radii/arbitrary dyadic affine
frame in the fixed sphere/torus examples, not an orthonormal-only assumption.
Cylinder uses two literal seam occurrences and a nonzero shift; assert distinct
chosen U sectors and identical shared physical keys. Existing cone/north/south
wedge fixtures attack pole-sector preservation without new cap families.

Retain explicit negative cases: the SAME bad guide marked authoritative must
not be repaired; a genuine spatial source departure greater than delta must
still refuse; a recovered true point outside a supplied narrow analytic bank
must refuse, not snap inward. Existing rational natural-R and narrow-bank checks
must keep their existing outcomes. Inspect actual resulting chosen UV and raw
guide/mapped-s/lift retention, not just process completion. No new geometry
certifier, broad benchmark, sanitizer campaign or both-device numerical rerun.
The numeric kernels are unchanged. Root's bounded watch retry is a subsequent
whole-operation observation, not a substitute for these direct fault classes.
