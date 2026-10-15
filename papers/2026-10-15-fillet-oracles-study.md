# Genuine one-edge fillet: independent oracle study

Research only. Read current AGENTS/doctrine and actual canonical box records,
exact circle/native consumers, feature-history oracles, pinned OCCT fillet source
and assertions, and existing USD BRep bindings/tests. Read acquired Sauer notes
on tangent planes/geometric joins and rational conics; read Karčiauskas/Krasauskas
introduction and construction scheme from the primary journal PDF. No product,
test, native build, dependency adoption or reference experiment was executed.
Pinned paths/hashes are in `fillet-oracles-sources.tsv` and citations in
`fillet-oracles.bib`.

## Selected complete operation

Parent selected ONE complete canonical-box vertical edge, supporting all original
edge IDs9..12, via `fillet_edge(source,typed_edge_key,radius)`. Source is the actual
owned immutable box, half-width a=3/4 and Z in[-1/2,1/2], with its existing Frame.
Require actual namespace/revision/edge kind and original endpoint identity;
geometry-equal foreign keys do not select the source edge. Radius is exact local
1/256-grid in[1/16,1/2], without approximation. No arbitrary/partial edge,
multiple-edge propagation, variable radius, concave blend, corner patch or repair.
Canonical/body/Boolean/feature type guards stay scoped. A separate result type
owns the changed trim/topology/history; do not relabel a carrier preview a solid.

For selected corner signs(sx,sy), reflect local coordinates p=sx*x,q=sy*y and set
c=a-r. The complete cross-section D is the original square satisfying

    p<=c OR q<=c OR (p-c)^2+(q-c)^2<=r^2.

The material is D×[-1/2,1/2]. The reflected selected-corner square outside the
quarter disk is removed. This direct rational inequality model independently
checks the operation without constructing a Boolean or calling a fillet helper.
All source corner orientations share this model, with explicit handedness.

The boundary consists of two WHOLE rounded cap domains, four original side
rectangles (two shortened), and one WHOLE quarter-cylinder band. Selected old
corner vertices/vertical edge disappear. Both cap loops replace their old corner
by a true circular quarter arc with exact tangent endpoints. This supplies end
closure; a cylinder whose ends/neighboring trims are unchanged is incorrect.

## Complete geometry, tangency and actual source UV

Keep all six original plane carrier controls, raw UV and per-Patch Frame bits.
Faces/charts with changed domains are still modified history even when carrier
coefficients remain identical. Adjacent sides retain their full original plane
carrier and use exact shortened trim domains. For corner(+,+), the x=a and y=a
side threshold in the appropriate original UV coordinate is d=1-2r/3. General
signs choose the corresponding lower or upper UV endpoint. Radius is dyadic but
d and the cap tangency UVs generally are **not** binary64. Preserve their exact
Fractions; copying Candidate float UV before trim classification remains crucial.

Selected parameterization uses a CCW rotation, rather than reflecting the
parameter map. For source corner k0(-,-),k1(+,-),k2(+,+),k3(-,+), put
q=(k+2)%4 and center=R_q(c,c), where R_q is q right-angle CCW rotations.
Let C=(1-u²)/(1+u²),S=2u/(1+u²). The blend is

    B(u,v)=(centerXY+r*R_q(C,S),-1/2+v), (u,v) in[0,1]^2.

Its new exact degree(2,1) homogeneous controls follow the existing asymmetric
quarter representation (X,Y,W)=(1,0,1),(1,1,1),(0,2,2): emit
(centerX*W+r*R_q(X,Y).x,centerY*W+r*R_q(X,Y).y,z*W,W)
at each original Z endpoint. All are dyadic;
weights1/1/2 and Cartesian controls stay within existing native admission.
No fitting, sqrt weights, circle approximation, control clipping or source-frame
change is needed. Its outward unit normal is exactly(R_q(C,S),0). Natural
du×dv points outward for every q, so all new blend faces have orientation+1.
The reflected p,q material predicate above is unchanged. A direct reflected
circle parameter would reverse handedness in two corners; it is not the
selected map.

At u0 the whole vertical join has position(center+r*R_q(1,0),z),
normal(R_q(1,0),0); at u1 position(center+r*R_q(0,1),z),
normal(R_q(0,1),0). These are the source CCW incoming/outgoing side contacts.
In corner order k0..3 their normal pairs are(-X,-Y),(-Y,+X),(+X,+Y),(+Y,-X).
Compare each whole join's exact
world curve and both original incident UV maps, plus normal identities. These
are geometric tangent-plane joins, not equality of arbitrary parameter speeds.
The blend still meets the cap planes at a sharp right angle; no endpoint spherical
blend or G2 continuity is implied. Sauer's tangent-plane discussion makes this
separation explicit. [Geometric Modeling, §3.2 and §§6.3–6.4](https://www.fim.uni-passau.de/fileadmin/dokumente/fakultaeten/fim/lehrstuhl/sauer/geyer/Vorleseskripteab2012/GeometricModelling-SS2015.pdf).

Cap arcs use original plane UV obtained by substituting their exact circle into
(x+a)/(2a),(y+a)/(2a). The shortened original straight edges keep intrinsic source
parameter subranges; traversal orientation is applied once. A new tangent edge
may map its Z parameter to the old selected edge but its spatial position is an
explicit offset, not the old source curve. New cap arcs have new angular
parameters; do not pretend they were subranges of old line edges. A generated
fillet interior does not lie on either old side plane: any source-plane contact
map must be explicitly a contact projection, not a false coincidence UV map.

## Strong complete material/topology/history oracle

Using exact submitted int/Fraction/finite-float values and original Frame,
classify D×Z directly. Determine all supporting output faces from their actual
bounded domains. At a vertical tangent line return BOTH adjacent plane and blend
IDs even though normals agree. Its endpoint has cap+plane+blend supports. Cap
arc interior has cap+blend; selected old corner is exterior. Artificial p=c or
q=c lines are not material boundaries. Circle equality outside the selected
quarter does not name the blend: e.g.(p,q)=(c-r,c) lies on the full circle but is
interior. This is an essential false-boundary witness for a naive circle test.

Complete source-domain/curve bijections and exact whole rational identities
establish boundary coverage beyond a finite sample, Euler or volume. D is a
simple convex rounded square, its straight pieces have positive length at the
selected radius bounds, and its quarter arc is inside the original square and
touches only the two declared side endpoints. Product with the positive original
height establishes embedding/no hidden sheets. Require unique exact outer cap
words, complete side/blend rectangles, every edge in two opposed occurrences,
one shell and cyclic vertex links. No same-face periodic seam exists on this
single quarter patch. Native ABI5 must read15 ordinary paired edges, zero
boundary/conflicting/multiple/same-face edges, under typed slot permutations.

The exact measure is

    scale^3 * ((9/4-r^2) + pi*r^2/4).

It follows from removing corner square area r² minus quarter-circle area pi*r²/4
through the original unit height. Counts/measure/bounds support the complete
geometry proof rather than replacing it. Original outer bounds remain attained.

Complete six-kind input/output roles are independently derivable:

| Kind | Input retained | Input modified | Input deleted | Generated output |
|---|---:|---:|---:|---:|
| Vertex |6|0|2|4|
| Edge |7|4|1|4|
| Coedge |14|8|2|8|
| Loop |2|4|0|1|
| Face |2|4|0|1|
| Chart |2|4|0|1|

Input62=33retained+24modified+5deleted. Output76=33retained+24modified+19generated;
V10/E15/C30/L7/F7/CH7, Euler2/genus0. Modified source cap edges are the four
straight edges incident to the selected endpoints; BOTH their incident coedges
change. Caps plus two adjacent side loops/faces/charts are modified. Deleted
means no boundary image; selected source edge/vertices can still generate new
fillet descendants. State that distinction rather than equating deletion with
no descendants. Parent design must fix exact ordered parent lists/maps/roles.

Require each retained or modified target's actual typed original source key,
original retained geometry/carrier bits, shortened source ranges, whole domain
changes and separate occurrence signs. Generated tangent vertices/edges, cap
arcs/uses and blend loop/face/chart must bind to the selected source edge and
actual incident source vertices/faces/coedges as appropriate. The result retains
radius, exact source edge and operation context. Every source disposition,
forward image/generated link and every reverse origin must agree with this
independently reconstructed role, not just reciprocal tables. Wrong but
consistently remapped ancestry must fail. No foreign history API is an oracle.

## Native/direct failure contract and measured workloads

At actual copied normalized float UV use exact rational original planes or B,
then original per-Patch Frame. Direct component position<=2^-36 and outward unit
normal<=2^-30 remain plausible explicit observed output contracts, not an error
bound inferred solely from partial grades. Blend non-axis expected normals are
rational, so no production sqrt/normalization is used. Grade both incident native
join occurrences, preserving distinct source IDs and seam-free topology. Check
actual descriptors, source/querybits, backend/order/count/statuses, gather and
scatter, explicit rejected/invalid NaN rows and real handle cleanup/healthy retry.
CPU construction and P100 sampling/topology remain separate operations.

Tests should cover all4 selected edge IDs, minimum/maximum and odd-grid radii,
all Frame scales/translations, cap non-dyadic tangency UVs and their actual float/
nextafter neighbors, rational off-axis arc points, subnormal material offsets,
all support strata and tangent endpoint closures. For selected(+,+), include
removed old corner, circle center interior, circle's non-blend half interior,
arcs at Zcaps, tangent vertical lines and their three-face endpoints. Mirror
these through the other corners. Unknown/foreign/non-edge/stale keys, invalid
radius/grid/bool/nonfinite and unsupported source types must fail atomically.
Late final-validation/sampler exceptions preserve source and prior result and
permit retry with no partial publication.

Fault witnesses: wrong radius/center/corner, handedness/sense flip, unchanged
old side/cap trim, missing cap arc/end closure, full-circle boundary false face,
UV rounded before exact admission, shortened source range applied twice,
wrong source parent despite reciprocal links, native/query metadata corruption,
normal flip and artificial weld/extra face. Direct named witnesses are not a
broad mutation score or a community alpha pass. Actual selected mutants may be
used if they address a remaining concrete claim.

The three caller recipes vary edge/radius/Frame, **not topology size**: every
result still has7 charts. Report this honestly and do not inflate repeats into
larger models. Useful wall consumes construction/publication, material and exact
boundary/history reads, actual first jets/normal rows, ABI5 topology and real
mate readers including all setup/transfers/checks/cleanup. Separate qualification,
SVG/export/reference and fresh-process startup; nested timings are nonadditive.
No GPU construction or whole-operation speedup is presumed.

## Actual primary references, suites and unresolved external evidence

The rational rolling-ball paper warns that general fixed-radius blending is
usually irrational and develops variable-radius rational canal surfaces. Its
contact-curve/envelope formulation is useful background; it does not grant exact
rational support for arbitrary fixed-radius fillets. Our two perpendicular planes
are the direct exceptional exact cylinder case.
[Karčiauskas/Krasauskas, 2000, introduction and §3](https://journals.vilniustech.lt/index.php/MMA/article/view/9948).

Read actual pinned OCCT MakeFillet header/source: rational representation,
continuity settings, generated/modified history and partial/faulty result APIs.
Its Build can expose partial geometry; the owned contract should instead publish
only the complete valid result. Pinned IsDeleted consults a result-face map and
split states. Upstream issue1036 reports unreliable edge/vertex deletion history;
we observed the face-only map population in source, so copying those labels is
not independent six-kind truth. [API](https://occt3d.com/dev/doc/refman/html/class_b_rep_fillet_a_p_i___make_fillet.html),
[upstream history issue](https://github.com/Open-Cascade-SAS/OCCT/issues/1036).

Read `tests/blend/begin,end`, `simple/X1,X2,U1`, and bug29334/25979/28596_2/28354.
The group checks actual checkshape and views; X1 has a surface-property scalar.
Bug28596_2 additionally runs bopcheck; bug25979 is a missing-closed-wire regression
with external data/TODO; bug28354 addresses seam-touch failure and retains TODO;
bug29334 checks analytic face kinds after offsetting a box fillet. None supplies
our full exact fillet material/history/tangency oracle. Missing external fixtures
or upstream TODOs must remain visible; do not auto-xfail required alpha cases.

Existing USD BRep `fillet_edges` MUTATES its body, uses cross-section integer1 for
circular and requires actual edges owned by that same body. Its pinned direct
example only checks face count>6, insufficient geometry truth. A feasible optional
isolated fleet comparison constructs the identical dyadic world box, selects the
vertical edge geometrically using endpoint/curve probes rather than assuming
foreign edge order, mutates only that disposable foreign body, and checks finite
manifold/bounds, exact analytic volume and physical-edge/interior-surface witnesses.
Check original owned input unchanged; do not demand the foreign mutator preserve
its disposable target or retain borrowed child handles across mutation.
[Public fillet/volume contracts](https://nvidia-omniverse.github.io/usd-brep/latest/docs/python.html).

The prior revolution reference volume disagreement remains OPEN
[#43](https://github.com/antimeme-ai/BRepGPU/issues/43); its integration accuracy
parameter is not a certified final volume enclosure. Do not widen tolerances,
exclude the failed criterion or inherit an assumed foreign-volume pass here.
All actual comparison outcomes must be retained, even when a diagnostic finishes
other checks before raising. No fillet reference run occurred in this study.
External community alpha qualification remains required and non-green; define
complete applicable required cases against advertised scope before observing
passes. Owned exact truth and selected foreign examples cannot waive that gate.
