# Independent truth for conservative world-space patch pairs

Study, 2026-10-14. Research/design input only; no product or test implementation,
product imports, native execution or fleet jobs. Read the existing depth-zero
hierarchy builder, interval arithmetic, public discovery header, body design and
topology/roadmap before defining this narrow oracle.

The proposed operation emits a deterministic page of **closed world-AABB overlap
bits** for two immutable patch collections. It retains false positives and does
not classify surface/trim intersections. Qualification must address two distinct
claims: every actual carrier is enclosed by its uploaded box, and every returned
bit agrees with the stated overlap predicate on those actual boxes. CPU/GPU
agreement alone addresses neither independently.

## Bounds tied to original submitted controls

Read the original binary64 homogeneous controls at each descriptor's record
offset, as exact Fractions. For every control `(Ax,Ay,Az,W)`, require the admitted
positive-weight envelope. Compute exact projected Cartesian coordinates `A/W`.
Positive rational Bernstein weights make the complete carrier a convex
combination of those projected controls; their componentwise minima/maxima are
therefore conservative over the entire source chart. This is a whole-domain
argument, rather than sampling a few UVs.

For the existing depth-zero root, verify its downloaded local endpoints contain
these exact projected-control extrema; verify root slot/cell/leaf identity and
one root per source patch. Qualify the **actual uploaded/downloaded world boxes**,
not reconstructed ideal boxes. For positive scale s and origin o, each returned
world interval must enclose both:

- Exact transformed original-control extrema: `o+s*min(A/W)` and
  `o+s*max(A/W)`.
- Exact transforms of the downloaded local root endpoints, interpreted as
  binary64 Fractions: `o+s*local_lower` and `o+s*local_upper`.

The second obligation checks the specified root-to-world transform even if an
unusually narrow wrong local bound happened to enclose some finite samples.
Outward multiply then outward add must be separate operations; ordinary
rounded multiplication/addition or a silent fused recipe is insufficient.
Use copied IEEE bits for finite/range admission and the existing supported CPU
floating environment before nonempty outward arithmetic. Reject unsupported
frames or environment for the whole unpublished snapshot. An empty collection
needs structural validation, without arithmetic/environment work.

An independent check can compare endpoint Fractions to the exact operations;
it need not duplicate the production rounding helper or require minimal box
width. If the design requires a particular deterministic per-primitive
nextafter recipe, separately compare that recipe using exact rational products
and sums. Keep the enclosure claim distinct from a recipe/width claim.

First fixtures can use the implemented body's finite offsets in [-16,16] and
positive power-of-two scales [.25,4], including different frames on each side.
The pair design must explicitly admit/reject any broader native frame family;
negative scale would require swapping extrema, rather than reuse of this proof.
No comparison of independent *local* boxes is geometrically valid when frames
differ. Small subnormal/one-ULP separations may remain false positives after
outward rounding and must not be forced to zero by an analytic-separation test.

## Exact closed-box bit truth

For each pair of actual stored finite boxes L_i/R_j, expected bit is one iff
for all three axes:

`L_i.lower <= R_j.upper && R_j.lower <= L_i.upper`.

Use exact Fractions or direct comparisons of finite binary64 endpoints. Equality
is overlap: reject only a strict axis separation. Check all bits, both zeros and
ones, including zero-width boxes and face/edge/corner touches. Closed-box truth
can keep analytically disjoint carriers; this is required behavior when their
boxes overlap, rather than a failed narrow-phase classification.

Each output pair is identified by deterministic Cartesian index `flat=i*R+j`.
Page-local bit k refers to global flat `start+k`; word k//32, bit k%32 is least
significant first. This page-local convention needs to be explicit even when
start is not divisible by 32. Candidate enumeration uses the retained counts
and source snapshots; it exposes neither borrowed host pointers nor device
addresses. Typed lineage contains side, source slot and complete optional
body/revision/entity-kind/entity/chart identity. Numeric IDs alone are not
unique across kinds/snapshots. Self-collection pairs include diagonals and both
ordered directions unless a separate contract explicitly removes them.

## Minimal useful analytic campaign

Derive every witness from original analytic maps and original submitted frames,
with exact rational UV/XYZ. For a known common world point, require its associated
pair bit survives; requiring all overlapping boxes is the stronger broad-phase
truth. Keep a compact set of independent fault classes:

| Fixture | Independent expected geometric fact / purpose |
|---|---|
| XY plane z=0 and YZ plane x=0 | Complete transverse line segment; exact interior witness (0,0,0) survives. |
| Two partially overlapping coplanar planes | Coincident carrier region retained; no unique-curve classification inferred. |
| Parallel planes separated by 2^-10 | Clean strict separation, large enough to exceed outward rounding padding. |
| Cylinder r=.25 and plane z=0 | Every quarter chart intersects the horizontal circle; all four relevant pairs survive. |
| Cylinder r=.25 and tangent plane x=.25 | Vertical tangency at y=0; endpoint charts retained by closed comparisons. |
| Two equal cylinders with x-centers separated by .5 | Tangent vertical contact; touching quarter pairs survive. |
| Equal cylinders with center separation 1 | Clear spatial separation; corresponding boxes reject. |
| Concentric cylinders from scales 1 and 2 | Same-quadrant boxes overlap although cylindrical carriers are disjoint: explicit retained false positive. |
| Canonical box/cylinder/plate chart collections | Shared-edge, cap/wall and same-face seam occurrences remain typed chart pairs; no deduplication by rounded coordinates. |
| Plate cap carrier and small patch wholly inside its hole | Carrier-box overlap retained despite disjoint trimmed material; trims are outside broad-phase classification. |
| Constant degree-(0,0) carriers at touching/separated points | Degenerate boxes, empty/rank-independent bound support; not a surface-intersection promise. |

Use independently framed equivalent surfaces: translation/scale compensation
can make different local patches coincide in world space. Also keep local boxes
identical while world translations separate them. These two complementary cases
catch accidental comparisons in local coordinates. A common dyadic translation
and positive power-of-two scaling of both sides must preserve geometric
intersection witnesses; exact *box* bitsets may widen differently at precision
limits, so demand witness survival rather than unjustified general bit invariance.

## Page and ownership truth

Cap requested page pairs at 2^20; total Cartesian product and start/end arithmetic
must be checked in uint64 before any allocation. Recommend start in [0,total],
start==total producing an empty page; start>total rejected. Define the budget-zero
rule explicitly rather than silently letting it change progress. Return actual
count `min(budget,total-start)` and stable source/count/start metadata. Full count
can be much larger than a page, without an all-pairs allocation.

Check dimensions 0xN, Nx0, 1x1, 3x11 and asymmetric collections. Check starts
0,1,31,32,33,total-1,total; budgets 1,31,32,33 and the cap, plus invalid start,
cap+1 and arithmetic overflow admission. Unused high bits in the final u32 word
must be zero. Concatenating pages must cover exactly [0,total) with no duplicated
or omitted Cartesian indices; direct per-bit expectation uses each page's actual
start, without relying on another pagination implementation as truth.

Retained results must remain readable after public source/query close; their
metadata must resolve into exact retained input snapshots, including each side's
frames/chart order. Reject foreign contexts, wrong-kind/stale handles and output
alias/capacity failures before publication under existing runtime rules. A failed
upload/kernel/download cannot publish an apparently complete all-zero page.
CPU-only operation and both P100s use the same geometric scope; no CPU fallback
may conceal a requested CUDA failure. These resource tests reuse the existing
registry/publication machinery rather than invent a second lifetime framework.

## Primary owned source anchors and limits

- `native/discovery_cpu.cpp`: `Builder::box`, original offset-aware root input
  loading, positive weights and depth-zero construction.
- `native/discovery_math.hpp`: downward/upward arithmetic and positive interval
  division; these are production references, not independent oracle truth.
- `include/brepgpu_discovery.h`: actual hierarchy downloads, root slots and
  non-overlap/exact-capacity diagnostic contracts.
- `docs/BODY_SAMPLING_DESIGN.md`, `docs/TOPOLOGY.md`: canonical maps, distinct
  chart/entity identity, common world frames and closed seam occurrences.

No external library, general SSI solver, imported body, branch completeness,
trim intersection or changed Boolean algorithm is selected. A qualified fast
pair page is useful preparation for a later independently qualified intersection
consumer; it does not complete that consumer.
