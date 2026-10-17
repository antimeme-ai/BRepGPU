# Imported plane/cylinder component reader

2026-10-17. DESIGN and finite test-only subplan for root review before body.
This extends the SAME original-face/component readers; it introduces no product
geometry, source editing, second chain checker, renderer or native rerun.

## Source and claim

Read `native_adaptive_component.py`, `qualify_face`/`prove_cylinder` and the
original-source `cylinder_trig` helper in `native_adaptive_trimmed_caller.py`,
`native_adaptive_curves_caller.py` point/chord/line-map helpers, and the original
source types/loader. The existing chain reader and directed physical incidence
remain the sole topology oracle. The existing rigorous original Taylor/Bezier
math remains the independent source geometry oracle. The curvature/variance
argument is the one already used in `prove_cylinder` and the adaptive geometry
reports; generalization replaces the unit orthogonal frame constant with a
conservative exact L1 coefficient bound. No implementation interval/kernel is
imported or called.

The operation reads the unchanged retained `source.bgsource`, explicit original
DefinitionID, query index, exact local caps and paired artifact index from a
completed native component output. It selects inventory by original IDs without
constructing/re-exporting a reduced source. Current saved output paths are
`context/adaptive-component/imported-first-cpu4` and `imported-first-p1000`.
Original source remains opaque retained bytes as well as the parsed source.
No private coordinates or private source paths enter tracked results.

Initial finite source-family contract: selected faces have original plane or
cylinder carriers; original spatial families line/circle/ellipse and original mapped pcurve
metadata; the existing rational-curve reader retains its normalized original
clock contract and is not generalized by this imported analytic operation; source map identity/explicit affine
piecewise maps. Unknown source families/maps are named oracle-unsupported, not
accepted by sampling. Actual selected-definition inspection found four faces (three
planes/one cylinder), six spatial curves (four lines/two circles),12 original
uses, all reconstructed identity maps. Frames are neither replaced nor assumed
orthogonal/unit; the cylinder has a nonunit original radius. Its stored U extent
exceeds the old authored trig helper's |u|<=7 assumption. No original coordinate
or frame values are published by this reader/report.

## Original analytic point and facet bounds

All original binary64 frame/radius/parameter values become exact Fraction.
Plane: S=O+uX+vY. Cylinder: S=O+r*cos(u)X+r*sin(u)Y+vZ.
Original analytic curves retain both independent harmonic radii:
C=O+a*cos(t)X+b*sin(t)Y; lines use O+tX. No phase reduction or pi-equivalence
changes original arguments. When comparing two original expressions, combine
coefficients at the SAME literal argument before interval evaluation so exact
common terms cancel; distinct phases are never merged. Enclose sin/cos by direct original-argument rational
Taylor series with its explicit omitted-term bound once term ratios decrease.
Extend the existing helper beyond its authored |u|<=7 profile using a bounded
series loop (maximum512 terms, target absolute tail2^-192). Retain old32-term
results inside the old profile; outside that profile, failure to reach a proved
remainder is named oracle-unresolved. No libm assertion supplies truth.

Add an OPTIONAL original-facet proof hook to `qualify_face`; defaults preserve
all existing unit cases. The hook receives the literal carrier, exact UV3,
actual XYZ3 and exact accepted upper. Plane residual is affine: max corner
squared residual <=upper^2 proves the whole facet, including equality.
For cylinder let M=abs(r)*(||X||_1+||Y||_1), using original coefficients exactly.
For a triangle of U range h, Taylor/Jensen variance gives
||S-sum(lambda_i*S_i)|| <= M*h^2/8. Add max original-corner-to-SAME-actual-L
Euclidean departure, proved by squared interval-coordinate maxima against the
nonnegative remainder. If inconclusive, bisect UV and actual L together into
four subtriangles; never resample S into virtual L. Bounded recursion reports a
counterexample when a rigorous lower bound exceeds the cap, otherwise unresolved
at the limit. This proves all facet interiors, not sample agreement. Existing
positive chart/physical area/packet source associations remain unchanged.

## Shared source authority and boundaries

Extend existing analytic point-box helper to arbitrary original frames/radii and
dimension, reusing original rational curve_h only under its existing normalized-clock
contract where applicable. The first imported case needs only lines/circles; no
new rational extraction, arbitrary maps or clock convention is added. Compare original
V vs original Ce(t), and original V/Ce(t) vs original S(chosenUV), to the retained
node_source_upper and exact local node cap. Actual Q is excluded from delta.
Check constructed Q vs Ce separately against positional/chord bounds. Preserve
original endpoint vertex words, exact original t, use traversal, mapped pcurve s,
periodic lift/branch metadata and separate occurrence UV roles. Existing line
image-map checker handles implemented straight-image correspondence; no bare
image tag substitutes for a descriptor.

Use the same source curvature formula for whole Ce->actual chord and
S(selected affineUV)->actual chord when needed, with recursive SAME-L subdivision
if the L1 estimate is too conservative for the retained bound. A tighter native
bound does not make a larger oracle estimate a counterexample. The first scope
does not re-certify unrelated diagnostic bounds or authoritative maps not used
by these reconstructed outputs.

`check_output` accepts explicit definition/node cap and optional original geometry
hooks. It reuses `qualify_face`, `original_boundary`, `incident_chains`, and the
same physical-key frontier accounting. General open/closed source incidence
replaces authored assumptions that every face orientation is+1 or the source has
exactly12 box edges. Apply source face orientation once. Every original selected
face/use/master occurrence remains required. Compare only selected-face incident
chains in captured vs final snapshots; unrelated stale rows are permitted.
Forest epoch and per-face generation remain separate: the index epoch is parsed
routing metadata (BGACRV has no epoch field), while face generation matches the
BGATRM envelope. Actual captured chains establish pairing.

## Execution and stopping

After root review, implement only test-reader changes and an explicit saved-output
CLI mode. Read the existing saved CPU4 output first using the actual source,
then matched P1000 output. No native meshing rerun, source retry, broad family,
new contact checker or dependency. Require complete original inventory, actual
facet bounds/node guards and shared frontier. Only after both original readers
pass compare all paired BGACRV/BGATRM bytes; report routing-only differences
explicitly if present. Reader cost is external and excluded from native timing.
Do not publish a general imported-source or performance claim beyond these
finite analytic source contracts and this completed component.

## Implemented checkpoint

`--saved-output --source-file --definition` reads completed output without
invoking native geometry. The saved CPU4 and P1000 imported analytic components
both pass the SAME original chain/facet/node reader (four faces,32 facets each).
Their five paired BGACRV/BGATRM artifacts are byte-identical. The actual source
review and finite limitations are recorded in the journal; this does not extend
rational-clock, singular, authoritative-domain or general imported contracts.
