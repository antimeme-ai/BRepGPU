# Independent truth for signed cylinder carrier offsets

2026-10-15. Research only; no product code, test execution, new dependency or build. Read the active doctrine, alpha policy, original body construction/oracles, delivered fillet and feature consumers, pinned OCCT offset implementation interfaces/DRAW cases, and NVIDIA USD BRep wrapper/tests. The selected candidate is a complete sharp intersection-joined cylinder operation, not a general offset solver.

## Definition and selected family

Rossignac and Requicha distinguish solid distance offsets from normal displacement of surface carriers; their definitions use regularized solids, and their boundary discussion explains why surface offsets alone can contain unwanted geometry. Their figures and equations explicitly include singular boundary strata and self-intersection hazards. This distinction fixes the operation name and the expected corner behavior here. Read pp.129–135, especially §§2.1,2.4,2.7. [Original paper](https://doi.org/10.1016/0167-8396(86)90017-8), [author publication list](https://faculty.cc.gatech.edu/~jarek/papers/).

Recommend the actual immutable canonical cylinder source, local radius R0=1/4 and half-height H0=1/2, unchanged admitted Frame. Signed nonzero d is an exact 1/256 lattice value in [-1/8,1/8]. Define R=R0+d and H=H0+d, and output material

    x²+y² ≤ R²,   -H ≤ z ≤ H.

The operation moves the two plane carriers and the cylindrical carrier by signed d and intersects their extensions. R is at least1/8 and H at least3/8; the closed output is embedded and nonsingular throughout the admitted family. The radius-to-height ratio changes, so a uniform Frame rescale of the source is wrong.

For d<0 this solid is the exact Euclidean erosion: inside the original cylinder the distance to its complement is min(R0-sqrt(x²+y²),H0-|z|). For d>0 it is **not** a Euclidean dilation. The new rim point (R,0,H) is sqrt(2)*d from the original rim. That point belongs to the selected sharp solid but lies outside a radius-d distance dilation. A rounded outward dilation would require a toroidal join. Make this an explicit literal counterexample, not a documentation footnote.

A box is another viable sharp carrier-intersection family, with independently offset half-extents; its corners similarly differ from rounded dilation. It adds no curved-normal or periodic-seam exercise. Choosing the cylinder gives a more useful complete first unit while leaving boxes, arbitrary aspect ratios, rounded joins, general surfaces and self-intersection repair unsupported.

## Independent complete boundary and source maps

Construct expected truth solely from original source identity, actual source Frame, and exact d. Do not call an output body factory or copy constructor-generated geometry. The expected boundary has two complete disks and one complete cylindrical wall. Reconstruct all source roles and full curve/control/map payloads, then compare the complete result tables bijectively. Exact radial and axial inequalities certify the material, rather than counts, volume, render appearance or CPU/GPU parity.

For a cap with output normalized UV, local position is (-R+2Ru,-R+2Rv,±H). Its signed-normal foot on the **original analytic plane carrier** has the same XY and z=±H0. Original UV is

    u_source=1/2+(R/R0)*(u-1/2),
    v_source=1/2+(R/R0)*(v-1/2).

For output wall quarter q, let C(u)=(1-u²)/(1+u²), S(u)=2u/(1+u²), and rotate (C,S) by q quarter turns. Output is (R*Cq,R*Sq,-H+2Hv). Original analytic cylinder foot has radius R0, the same angular parameter, and

    v_source=-d+(1+2d)*v.

The face atlas uses angular tau=q+u, while chart UV uses local u. These distinct owners must remain explicit. At every regular carrier point, output minus original foot is exactly d times the oriented local unit normal; world displacement is Frame.scale*d times that normal. Grade this with exact rational identities before native rounding.

Positive offsets require analytic source-carrier extensions. Source-foot membership must refer to the **original effective face domain**, not just the original UV square: for example (.2,.2) is inside the source cap carrier square but outside its radius-.25 disk. A separate carrier-domain flag may be useful, but cannot replace the trim membership flag. For the wall, the original face condition is z in[-.5,.5]. Do not submit out-of-range UV to the native evaluator and call its rejection a geometry disagreement, or pretend an analytically valid extension is an admitted native patch query.

At the rim there is no single source face normal: its anchor displacement is (d,0,±d), a sum of cap/wall normal directions with length sqrt(2)|d|. Check both original incident carriers and both exact foot maps. Do not require a ring edge or anchor to move distance |d| along a unique face normal. Physical ring parameters remain intrinsic, while seam source axial parameters follow the wall affine map.

Every cap disk must be complete; every wall quarter must cover the whole selected axial interval, and the four quarter maps must join without gaps, overlaps or reversed normals. Source-oriented whole boundary maps, exact carrier equations and an exhaustive role-table bijection provide this bounded-family completeness argument. Finite samples attack public reader faults; they do not replace it.

## Topology and history truth

Expected graph: two anchors, three edges (two closed rings and one seam), six coedges, three loops, three faces and six charts, 23 entities across the six kinds. Ring edges have distinct cap/wall uses. The seam has two distinct opposite uses on the same wall face, retaining atlas tau0 and tau4 and the same physical source edge. Require every actual coedge exactly once in its owning loop, every edge exactly twice with opposed signs, complete chart ownership and exact source/world endpoint closure. At an anchor the ring has two endpoint germs and the seam one; counting only distinct neighboring vertex IDs misses this.

The graph is an embedded orientable genus-zero shell. A complete expected incidence/word comparison and source-map identities are the oracle; Euler and connectedness are useful diagnostics only. Raw ABI5 CSR bytes and actual public mate/neighbor/edge-kind readers must agree with independently collected typed memberships, including the same-face pair, under slot permutations.

For nonzero d every source entity has one modified same-kind image:23 modified inputs and23 modified origins, no retained/deleted/generated entities. A new namespace is still required, and equal geometry from another source must not match its lineage. History must bind actual original source entities and explicit map owners. It must not call a rim point a regular face-normal image, treat source UV extension as original trim inclusion, or silently attach source identity from another cylinder. Source payloads and caller-owned data remain unchanged on success and failure.

## Exact/native and public-reader qualification

Use exact Fraction arithmetic for all model positions, source maps and material labels. The existing asymmetric rational quarter basis admits exact dyadic weighted controls: quarter controls have XY (R,0),(R,R),(0,2R), weights1,1,2, rotated by q; Z coordinates are multiplied by the same weight at ±H. This avoids introducing sqrt(2) weights or another rounding promise. Cap controls are the literal square corners±R at±H. Preserve source Frame bits and verify every descriptor, lineage, slot, order, domain and control payload independently.

At actual copied binary64 query UVs, evaluate the rational quarter and plane formulas exactly. Cap normals are axial; wall normals are the exact rational (Cq,Sq,0). Direct component bounds can reuse the delivered position2^-36 and oriented-unit-normal2^-30 contracts over the admitted Frame range, subject to design confirmation. Check both signs, all quadrants, chart joins, seam ends, cap boundaries, non-axis rational angles, smallest/largest d, and Frame scale.25/4 with translated origins. Separate exact source-foot displacement from native finite-domain sampling. No ideal rational query may substitute for the actual submitted float UV.

Public material tests must include center, cap interior/exterior, wall, each rim with all supporting original/output face identities, seam points without invented material boundary, rational off-axis circle points and one-ULP radial/axial neighbors. Exact boundary input and its rounded binary64 neighbor need separate expected labels. Check trim readers at exact cap circle maps, corners outside disk, every wall atlas join, and original-domain source-foot flags at positive extensions and negative contractions.

Fault classes: wrong sign on lower cap; radius-only or height-only change; accidental uniform scale; radius-d diameter confusion; normal sign reversal; quarter reflection; dropped or merged seam occurrence; cap-square treated as disk; wrong original UV scale/shift; carrier-square membership substituted for trim membership; false unique-normal history at rims; stale source IDs/Frame/revision; float/int/bool typed aliases; copied result from another operation; corrupted raw adjacency and public mate reader. Direct injected witnesses should fail assertions in the corresponding real qualifier. Selected production mutants can demonstrate these individual sensitivities, but cannot characterize all testing quality or alpha readiness.

Admission should reject unsupported sources, zero, off-grid/over-limit distances, bool/nonfinite coordinates and foreign/forged identities before publication; d=-R0 would collapse the cylinder and is outside the family. Test rollback at the actual late validation boundary and native child-handle cleanup/retry through the existing consumer. No public partial solid or leaked new namespace handle is acceptable.

## Actual reference and community opportunity

OCCT offers separate arc and intersection joins; its simple offset routine does not compute intersections. The general API warns about self-intersections, incomplete self-intersection removal and non-C1 surfaces. It also exposes generated/modified/deleted history. Use the intersection join for a matching experiment, not the default arc join. These limitations are reasons to bound this unit, not evidence that a general offset has been solved. [Official API](https://dev.opencascade.org/doc/refman/html/class_b_rep_offset_a_p_i___make_offset_shape.html), [surface API](https://dev.opencascade.org/doc/refman/html/class_geom___offset_surface.html).

Pinned OCCT tests `offset/shape_type_i/A1,A2` really create radius5,height15 cylinders and offsets+1/-1, with volume targets1922.65/653.451. Their group begin sets complete intersection mode c and intersection join i; shared begin uses tolerance1e-7 and DRAW offsetload/offsetperform. Shared end checks shape validity, positive face areas, and absence of nonzero source/result section length. The target sharp analytic volumes are612*pi and208*pi. These original parameters have height/radius3, unlike the canonical source ratio4, so **an owned normalized canonical case is not an upstream replay**. A required adapter must preserve original geometry, runner semantics and assertions; current owned source admission cannot do that. `faces_type_i/A1–A4` remove one/two cap faces and test hollow/open constructions, outside this closed-solid unit. Keep those required scope decisions explicit before alpha.

OCCT's source DRAW suite needs an available compatible DRAW build, loaded topology/shape-healing plugins and the original group harness. We have read its pinned source, not built or run it. No pass is claimed. Pinned case links and exact read hashes are in offset-oracles-sources.tsv.

The installed isolated NVIDIA reference already wraps `offset_brep(...,extended_offset=True,self_intersection=True)` as a pure operation; its own wrapper documents sharp extension/intersection at convex edges. Its elementary offset test checks merely Brep type/nonempty faces, so it is not our geometry oracle. Its shell tests have stronger fixed volume/bounds/manifold/source-preservation assertions but implement a different hollowing operation. [Pinned wrapper](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SmPyLib/src/SmPyOffset.cpp), [tests](https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/source/SmPyLib/tests/test_omni_solid.py).

A feasible optional fleet-only experiment creates the same world cylinder using source origin/scale, radiusR0*scale,height2H0*scale and lower-z origin, then invokes the actual extended offset with distance d*scale. Check source preservation, manifold, complete finite bounds, volume2*pi*H*R²*scale³ and fixed original-boundary/interior witnesses through reference closest-point queries. Pin all source/runtime hashes and actual request/response data. Require the join behavior to match before comparing. No new dependency or reference build is needed. Reference success and finite distances do not prove our full maps/history. The unresolved revolution foreign-volume disagreement remains issue43; do not loosen a threshold to obtain green.

A whole-caller campaign should measure actual construction, original-domain foot readers, complete native sampling, topology build/readback, independent qualification and export with separate costs. This is one fixed23entity graph and six charts; changing d/sign/Frame changes geometry, not model size. Report three honest geometric recipes and separate CPU1/24/P1000/1 full-consumer placements. Do not inflate batches to imply larger modeling workloads. GPU construction, general offset repair and persistent residency remain future.

External alpha qualification remains incomplete. A matching same-world manual reference comparison is not the OCCT community suite. Required original cases with missing adapters/runners/data, unsupported required inputs, skips/xfails or unresolved discrepancies remain non-green under docs/ALPHA_QUALIFICATION.md. This study selects no new alpha scope or dependency.
