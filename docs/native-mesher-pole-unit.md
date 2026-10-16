# Analytic quotient construction unit

This implements geometry-plan section8 in the existing geometry owner. It does
not turn collapsed UV triangles into omitted physical triangles. Source admission
is one declared degenerate use on a sphere/cone, bounded angular interval, and
two adjacent constant-u meridians. All original use dispositions remain in the
snapshot; an unsupported trim or source mismatch returns a typed refusal.

The first executable seam is the original analytic fan bound, followed in the
same unit by boundary selection, shared cut records and ordinary-remainder
construction. `PoleFanRequest` retains the original carrier row, symbolic pole
kind, exact dyadic cut UV endpoints, the actual XYZ role triple
(pole,left-cut,right-cut), and explicit precision. No analytic pole constant is
rounded into a CDT coordinate. The result records the same map and actual role
to facet permutation. Its normalized map is the accepted xi/rho map, including
the full rho=0 collapsed edge; one nonzero physical facet realizes each fan.

The bound uses the existing original analytic jets over an exact rational box
containing the symbolic pole and cut. For a sphere, v_pole=+/-pi/2 from the
explicit precision profile; the ideal physical position is exactly O+/-rZ.
For a cone, v_pole=-r/sin(alpha), with sin excluding0; its ideal position is
O+v_pole*cos(alpha)*Z. Reuse the original enclosure arithmetic and rational
square-root norm bound. The normalized derivatives are

    F_xixi=Suu du²+2 Suv du rho dg+Svv (rho dg)²
    F_rhorho=Svv (g-v_pole)².

Their whole norm bounds divided by8, plus the maximum of the four ideal/cut
corner departures from the actual bilinear interpolant, bound the full fan.
The two ideal corners use the same physical node. Actual role facets are
strictly nonzero under exact binary64 cross products; output sense uses the
face orientation and sign(du*(g-v_pole)). Parameter distances, not physical
node distances, determine radial contraction. Any source finite-to-ideal band
is separately bounded before accepting positional epsilon.

The second seam consumes the settled original forest. Choose the closest
existing master samples on each adjacent meridian with proved same-side UV
heights and constant-u traces. Construct one face-owned cut chain at dyadic
fractions theta_j=j/2^k, using the SAME theta for u and g; angular pieces have
width<=pi/2. Endpoints preserve original node keys/XYZ, interior cut nodes use
face-site keys and once-rounded original source XYZ. The ordinary remainder
and quotient patches share these exact cut sites and `face_cut` BoundaryKeys.
Exact selected-domain/interval source checks exclude other trims from the cap.
No private source-edge nodes are manufactured.

The snapshot records cut sites and constraints, original quotient uses,
per-patch source map descriptor, nonzero physical-facet role triples and source,
face, boundary generations. The same FaceEngine processes ordinary remainder
cells and finite quotient map cells. Angular failure requests theta bisection;
radial failure requests master splits then a strict same-side new cut with
both endpoint parameter distances<=3/4 old. A changed cut rebuilds that face
and its ordinary neighbor, rather than entering the source-edge forest as a
face-cut key. Unsupported sectors and exhausted precision/resource budgets
produce explicit unresolved/unsupported outcomes.

Direct tests start with a genuine missing API RED. Independent long-double
analytic source/Phi point differences on fixed normalized grids attack map,
chain-rule, actual-corner and winding errors; exact source special points and
explicit known node offsets attack source/canonical departures. Metamorphic
angular/radial shrinking and translation test coverage/contraction without
using a producer flag as an oracle. Source-backed construction then checks
complete original use coverage, shared cut identity, ordinary/fan opposite
incidence and nonzero emitted facets. Root and numerical/topology peers review
actual bound math and the snapshot dispatch separately. All geometry execution
runs only on the fleet's isolated native-mesher checkout.

## Producer/consumer transport and first source-backed profile

The same snapshot retains the complete original use/segment loops, including a
`declared_quotient` disposition with literal original pcurve endpoint coverage
separate from spatial-t endpoints (which can be equal). Optional construction
cycles name the ordinary remainder; they do not overwrite the original loops.
`FaceConstructionSegment{site,constraint}` and
`FaceConstructionLoop{source_loop,outer,segments}` address the complete snapshot
site pool. An explicit construction-domain-present flag distinguishes an empty
ordinary remainder from the default original cycles.

Each `FrozenPoleCap` names its collapsed use, two adjacent meridian uses, consumed
original segment ranges, original mapped-pcurve interval, symbolic pole kind,
finite collapsed-source v enclosure, one proxy/pole site, cut revision and its
`FrozenPolePiece` range. Each piece names two shared cut sites, their face-cut
key, exact cut UV endpoints, canonical position/band bounds and the positive
chart role permutation of (pole,left-cut,right-cut). A single dyadic proxy uses
midpoint u and the mean of the two finite collapsed-use endpoint v locators;
its physical node is the declared quotient representative. It is bookkeeping
for connectivity, never an ordinary-cell source map or a rounded ideal pole.
Publisher applies original face orientation once; topology uses positive-chart
roles (the existing primitive called with orientation+1).

The initial finite profile is one declared collapsed edge on an original sphere
or cone, two adjacent constant-u meridians, one regular side and a convex cap
cut disjoint from every other selected trim. Actual source pcurve/meridian
interval bounds must justify the finite collapsed band and constant-u branch;
unsupported curved polar trims return a typed refusal. Choose existing master
samples only; if both are the opposite original endpoints, an empty ordinary
remainder is permitted. A failed fan can request actual meridian master splits
for radial contraction or face-owned dyadic angular subdivision. Radial source
requests use original edge keys; angular requests carry face-cut revision and
restart the face generation without entering the source forest. Both endpoints'
parameter distance to the symbolic pole must contract by at least3/4 on a radial
replacement, except a proved fixed source floor which is unresolved.

For distinct original endpoint IDs, the boundary forest owns a deterministic
component derived exclusively from declared degenerate edges, with min source
VertexID canonical key/XYZ and retained original aliases. Original V/Ce-to-carrier
node checks remain separate from actual canonical-Q departures; neither source
IDs nor their original values are erased. Every emitted pole facet has nonzero
exact-binary64 area and a full xi/rho source-coverage map, including rho=0.
