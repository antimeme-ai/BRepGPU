# Signed sharp cylinder offset: runtime/source/reference study

2026-10-15. Research only. Read current doctrine/AGENTS, native Patch admission,
source Frame/cylinder records, complete feature sampler/caller and acquired
NVIDIA/OCCT/Truck sources. Read-only fleet introspection confirmed installed
usd-brep1.0.0 API/docstrings. No geometry/offset executed, no product/test
implementation, new build, installation or production dependency adoption.

## Actual API and join semantics

Installed offset_brep(brep,distance,*,extended_offset=True,
self_intersection=True) documents a PURE new Brep, input unchanged. Positive
means outward along face normals; negative inward. extended_offset=True extends
convex offset faces to SHARP intersections. False is not selected; the Python
doc warns of gaps while acquired executive comments describe fillet-like joins.
Either way its complete rounded-solid coverage is not established here. The
selected sharp operation matches the actual default flags explicitly, rather
than inferring Euclidean ball dilation from the word 'offset'. shell_brep creates
a hollow wall and is a different operation. [NVIDIA API](https://nvidia-omniverse.github.io/usd-brep/latest/docs/python.html).

SmApiOffsetBrepFull delegates to SmOffsetExecutive::OffsetBrep. That executive
sets null output, stores signed inset/extension/merge choices and builds new
geometry. The selected extended path allocates an intermediate Brep, offsets
source faces, intersects/rebuilds incidence and publishes a separately allocated
result while cleaning intermediate storage. A legacy parameter comment says
input consumed; the inspected selected path does not delete the original, and
public binding/live contract is pure. Do not silently infer stronger lifetime
behavior from either comment: actual original body/edge scalar before/after
probes must establish the advertised retention scope. A fresh subprocess bounds
foreign failures; no stale source handle should be queried after a documented
consuming operation. No source-copy requirement is found for this public pure
API, unlike the previous mutating fillet.

OCCT MakeOffsetShape explicitly distinguishes GeomAbs_Arc pipes/spheres from
GeomAbs_Intersection extended face intersections. Positive/negative are outside/
inside. Its Intersection flag concerns considering all parallels and is distinct
from the Join enum; it is not the selector for sharp joins. Its SelfInter path is
documented incomplete and defaults false. PerformBySimple avoids intersection
computation and cannot substitute for an independently closed solid. Generated/
modified/deleted outputs and tolerances remain reference behavior, not selected
owned semantics. [OCCT offset](https://occt3d.com/dev/doc/refman/html/class_b_rep_offset_a_p_i___make_offset_shape.html).

Truck's acquired Offset(entity,NormalField(entity,scalar)) decorators evaluate
point and derivatives of entity plus a normal-vector field. NormalField uses
cross-product normalization and higher derivatives, showing general offsets are
not simply modified rational control points. The inspected decorator/tests are
curve/surface machinery, not complete joined solid/history construction. Surface
normal and curve left/right orientation are distinct conventions; an oriented
B-rep face sense cannot be omitted. [Truck source](https://github.com/ricosjp/truck/blob/88ed005249e5e3a6b07f62425399435905cd3ab6/truck-geometry/src/decorators/offset/surface.rs).

## Selected complete cylinder family

Root selects actual canonical factory cylinder, retained source identity/revision/
Frame, with signed nonzero local distance d on1/256 grid[-1/8,+1/8]. Source
radius r0=1/4, half-height h0=1/2. Result radius R=r0+d and half-height H=h0+d,
with sharp cap-wall intersections. R∈[1/8,3/8], H∈[3/8,5/8] stay positive and
bounded. Exclude zero rather than invent no-op history; exclude radius collapse,
partial/multi-surface selection, other bodies/fillet results and generalized
rounded offsets. No canonical cylinder reconstruction via make_body may replace
the actual source or hide changed dimensions.

The source/result have2 anchor vertices,3 edges(two full rings+seam),6 uses,
3 loops,3 faces and6 charts=23 six-kind entities; the seam has two opposed uses
on the same wall face with distinct atlas0/4 occurrences. Keep fresh output
namespace while complete six-kind ancestry identifies original actual entities,
geometry and domains. D is a carrier-normal displacement; it is not a promise
all output boundary points are Euclidean distance abs(d) from the bounded source.
At an expanded rim displacement combines radial and axial components and has
norm sqrt(2)*abs(d); positive sharp cylinder differs from a rounded Euclidean
parallel body. Negative cylinder erosion happens to share this geometric family,
but that fact must not redefine the positive operation.

Output caps need real controls spanning[-R,R]^2 at z=±H, with disk trim; four
wall rational quarter charts use R and z endpoints±H, weights1/1/2. All source
values/d remain dyadic so emitted controls are exact binary64 coefficients with
no fitting/sqrt-weight conversion. Retain original Frame and deliberate raw
parameter convention, fresh descriptors/Lineage. Native runtime requires weights
in[1,256], every abs(homogeneous numerator)<=weight and bounded metadata; here
local absXY<=3/8 andabsZ<=5/8, comfortably inside the envelope. Original source
Frame admits power-of-two scale[1/4,4] and origin components[-16,16]. No native
build, new ABI, relaxed admission or rebasing is required.

UV metadata alone does not enlarge an unchanged control net. Output carriers
must represent actual new domains. Exact affine pullback into original SUPPORT
carriers is cap sourceUV=-2*d+(1+4*d)*outputUV per component; wall source tau is
unchanged and sourceV=-d+(1+2*d)*outputV. Positive offsets yield sourceUV/sourceV
outside original bounded trims. Do not force these maps through source.classify
admitted-face labels, call them spatial coincidence, clamp them or silently
replace original source parameterization. Distinguish source carrier extension
from trim membership, and explicit normal correspondence from topological edge/
vertex correspondence. Cap support points differ by signed normal d; wall support
points differ by d times exact radial unit normal. The original rim itself does
not have one unique normal.

Exact material is X²+Y²<=R² and -H<=Z<=H after exact retained Frame inversion,
returning all bounded supporting cap/wall faces. Exact volume is pi*R²*(2H)*scale³;
complete bounds origin+scale*((-R,-R,-H),(R,R,H)). Whole rational edge/bothUV/
carrier identities, opposite paired uses, seam multiplicity, links and domains
must supplement these measures. Every map owner/source namespace and effective
carrier domain must be explicit and independently qualified.

## Native consumer and whole costs

Separate OffsetResult/Solid/Request/Samples factories preserve existing canonical
and feature guards. Current genuinely generic metadata/row/child helpers fit
actual patches/charts/faces and frames; use existing ABI1 first jets and ABI5
explicit topology retaining the actual result, including same-face seam readers.
Copy Candidates to actual binary64 UV BEFORE exact cap/wall admission. Keep
rejected/invalid rows status-1/NaNs, full descriptor/querybits/Lineage/status/order/
backend binding and native-derived oriented normals. Bound arbitrary iterable
requests cap+1 and grids before allocation. Construction validates privately and
publishes once; native/late host failure closes all3 children, preserves borrowed
Engine/valid result and allows healthy retry. No fallback/partial batch or new
resident lifecycle is selected.

Root selects three fixed-size cases: inset d=-1/8,Frame0/scale1; outset+1/8,
Frame(2,-3,1)/scale1/2; odd+17/256,Frame(-2,1,1/2)/scale2. Six-chart grid17 yields
1734 candidates per case with actual cap trim rejections, not replicated patches
or increasing CAD complexity. Complete caller consumes exact material/edge/UV,
all source history and actual support-carrier maps, native first jets/oriented
rows and actual typed paired/seam CSR readers. Timed useful walls include source
publication/request, Engine/context/packing/transfers/download/checks/Frame/
normal/scatter/cleanup and topology construction/readers. Publish prepared-source,
source-factory and full child walls; independent qualification/artifacts/SVG/
foreign/import work remains separately timed. Nested phases/medians are not
additive and unprofiled wrapper evaluation is not kernel time. GPU construction,
residency tuning, Python-role investigation and matched rendering remain deferred.

## Same-world foreign comparison and limits

Construct actual source cylinder using public create_cylinder(base_center,radius,
height): world base origin+(0,0,-scale/2), radius scale/4, height scale, signed
worlddistance=d*scale. Check all exact source bounds/endpoints/distance survive
binary64 input. Call offset_brep(source,worlddistance,extended_offset=True,
self_intersection=True), with no shell or all-edge fillet substitution. Retain
original body volume/manifold/bounds and all3 edge endpoint ports/domains/three
scalar point probes before/after; the closed rings' two endpoint occurrences and
wall seam must not be collapsed merely because coordinates agree. Original pure
input and new result ownership differ from previous fillet mutation.

Compare manifold, complete finite bounds, analytic pi-volume and63 actual
boundary witnesses:15 exact edge samples plus48 native interior points(eight per
chart, covering both cap charts and allfour wall quarters). Record actual
source/input/adapter pins and method/flags/requested accuracy. Keep unchanged
volume relative1e-8, bounds1e-8*scale and returned-distance/recomputed-boundary-
residual1e-7*scale criteria unless an independently reviewed design selects them
before data. Volume API clamps below1e-8, not a certified final error enclosure;
mass_properties clamps at1e-4 and is not a tighter replacement. Save every actual
outcome before nonzero failure, with structural/nonfinite results hard failures.
Do not switch join type/API/accuracy to chase pass. Earlier revolution issue43
and required external alpha qualification stay unresolved by selected optional
passes. No foreign material/history or general Euclidean offset is established.

Pinned source hashes: offset-runtime-sources.tsv; bibliography: offset-runtime.bib.
