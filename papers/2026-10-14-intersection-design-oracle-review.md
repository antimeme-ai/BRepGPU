# Intersection design: independent oracle review

Reviewed corrected `docs/INTERSECTION_DESIGN.md` against the analytic component/
exact-event study. Design only; no product/test code or native execution.
**Accepted after the geometric-span clarification below; no remaining blocker.**

## I-OR1 — distinguish geometric spans from chart occurrences (medium; resolved)

A vertical seam/cardinal generator has two source-chart occurrences. If each
occurrence is an ordinary Piece sharing the same endpoint events, counting all
incidences as physical graph edges makes one open segment resemble two parallel
edges/a cycle. Point occurrences have the analogous distinction.

Root added `Piece.span_id`: physical degree and closedness count each geometric
span once, while incidence records remain occurrence-specific. Two generator
pieces can represent one span/open segment; multiple point occurrences are one
span and zero graph edges. Ordinary horizontal intervals each define one span.
This gives the oracle direct physical graph truth without discarding UV seam
representations. Re-read corrected clause; finding resolved.

## Exact construction and coverage

The selected pair is one factory plane and one wall, so tilted planes and
plane-plane/wall-wall cases are explicitly unsupported. Frame/control binding
must precede using literal canonical equations. Horizontal construction enumerates
all quadratic constraint roots, endpoints and constant/coincident constraints;
exact root-free interval signs plus point-event membership cover the closed face
domains. Transverse, trim-tangent and boundary-coincident cases are represented,
not silently rejected as empty. Vertical exact discriminant branches and closed
span/height clipping separately cover empty, two-generator, tangent-generator
and singleton results.

Preserving admitted trim roots as events splits representational pieces while
retaining component connectivity; it avoids losing an internal degree-two trim
contact. Named quadrant/seam identities join known atlas occurrences exactly.
A full unclipped circle has one four-piece cycle; additional retained trim events
may split a circle without adding components. No proximity welding or finite
sample residual establishes this graph.

The proposed two-radical comparison formula is sound: after determining signs,
opposed terms are ordered by exact squared magnitudes, and its remaining
expression contains one radical. It resolves equality across differently
represented radical fields without factorization. Perfect-square/zero/linear/
double-root normalization, exact midpoint ties-to-even conversion and rejection
of unneeded mixed arithmetic fit the supported map/parameter operations.
Sampling endpoints use their own root field; interior parameters are rational,
avoiding accidental mixed-field evaluation. Unbounded exact construction and
bounded sampling precision are explicitly different contracts.

## Sample error and near tangency

The updated design directly checks finite outputs, each source position against
the exact curve and their componentwise difference before publication. Empirical
local evaluator targets are not substituted for these model checks. Construction
and algebraic topology use no sample tolerance.

The stated budget has ample conditional margin for the admitted maps. On normalized
[0,1], nearest binary64 error is <=2^-54. Each planar world-coordinate slope is
<=6; cylinder coordinate slope <=2 and height slope <=4. Both-side UV rounding
therefore costs at most 12*2^-54 per coordinate. Taking the design's conservative
conversion slack 2^-48 per side and scaled local evaluation targets gives a
pair budget `2^-37 + 2^-47 + 12*2^-54 < 2^-36`. The local 2^-40 quantity remains
a target rather than a universal theorem; direct actual model checks enforce the
published sample grade regardless. Original world coordinates stay below32,
and power-of-two scale makes the conversion allowance conservative.

Near-tangent discriminant/root conditioning is not an extra sample-error factor:
exact algebraic construction precedes nearest rounding of bounded native UV.
The maps have the derivative bounds above even at chart endpoints. Tiny distinct
components may round to indistinguishable sample positions; exact spans/events
remain distinct. Irrational rounded UV need not be trim BOUNDARY. The 4096-step
inward-window cap can explicitly fail sampling without changing complete exact
geometry. Points contribute one candidate/two native rows; total cap2^19 keeps
native rows bounded. No fitted approximate continuous curve is being certified
from those discrete samples.

## Independent verification boundary

The oracle must inspect raw Scalar coefficients and ExactMap numerator/denominator
coefficients, rather than call production Scalar arithmetic as truth. Whole-piece
cross-products against original homogeneous controls establish coupled spatial/
UV identities. Independent rational root/isolation or literal algebraic values
check fixture event positions and ordering. Literal complete expected cases
cover full circle, four clipped arcs, seam crossing, lens/hole/trim contact,
generators/tangency/points, swapped sources and unsupported inputs. Generic map/
graph validity alone must not be advertised as an independent component-count
oracle for arbitrary unknown configurations.

The root caller recipes are appropriate: the interior z=0 circle and the
half-width3/16 rectangle clipped against radius1/4 give respectively one closed
component and four disjoint quadrant arcs with irrational endpoints. Both are
complete analytic expectations, not inferred from a broad-phase prefix or a
render. No new native kernel, general SSI, Boolean topology or uniform evaluator
error theorem is implied.
