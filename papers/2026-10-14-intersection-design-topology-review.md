# Intersection design: exact construction and topology review

2026-10-14. **Accepted after the corrections below; ready for plan review.**
Reviewed [INTERSECTION_DESIGN.md](../docs/INTERSECTION_DESIGN.md) against the
current doctrine, owned canonical geometry and
[intersection topology study](2026-10-14-intersection-topology-study.md).
Source/design inspection only; no implementation or native execution.

## Findings and integrated decisions

**IT1 — high: merging admitted cells could erase a trim tangency event.**
Supported counterexample: a radius .25 wall centered at (0,0), intersected at
z=0 by a scale-four plate whose hole center is (.75,1) and radius is 1. The
entire section circle lies outside that open hole but touches its boundary at
quadrant 0, s=1/2. Adjacent admitted intervals therefore belong to one component,
yet their shared named trim event must remain. **Resolved:** pieces stay split
at every admitted constraint root; graph joins preserve component connectivity
and a degree-two event. A full circle has four chart pieces only when there
are no additional constraint events; this fixture has five pieces.

**IT2 — medium: isolated points need representation incidence, not fake edges.**
The original one-row wording conflicted with two-sided evaluation, while an
ordinary lo/hi incidence for lo==hi could manufacture a self-loop. **Resolved:**
point pieces have their own kind and a single `point` incidence; each point
occurrence yields one output candidate and two native rows. Duplicate chart
occurrences share the physical component/event rather than adding point edges.

**IT3 — medium: exact map and slot semantics must be explicit before the plan.**
Both operands retain original body chart slots; the sampler separately remaps
them into its packed PatchSet. **Resolved:** original slots, inspectable maps
and factory-only result/request binding are specified. Each map independently
uses one denominator common to its own coordinates, positive on its whole
piece domain. The three maps need not share a denominator. Dimensions are 3/2/2.

**IT4 — medium: preserve all source domain coincidences.** Horizontal sections
on a wall rim and generators on a planar lateral boundary have named boundary
meaning even without a newly isolated plane-trim polynomial root. **Resolved:**
axial, lateral and endpoint domain constraints are retained alongside plane
trim constraints. Boundary-coincident curves remain included and annotated.

## Exact scalar and completeness assessment

The two-radical comparison is sound when implemented with exact sign checks:
first establish the sign of z=K+u*sqrt(D); when it opposes v*sqrt(F), compare
their squared magnitudes. Squaring after checking signs avoids extraneous
ordering, and the remaining expression has only one radical. Equality also
handles different noncanonical radicands. Scalar hashing must stay disabled;
exact equality does not imply identical representation fields.

Same-field arithmetic, exact primitive quadratic roots and rational witnesses
between distinct algebraic events suffice for the declared cell construction.
There is no need for a sampled topology decision or a bounded precision loop
that silently loses close events. Generic mixed-field arithmetic may reject;
the prepared sampler uses rational interior parameters and exact endpoints
whose maps remain within supported fields. A useful implementation choice for
generator UVs is quadrant-local s=y/(R+x), derived directly from the exact
generator point, keeping its radical field rather than introducing redundant
root representations. This is a construction suggestion, not an additional
design prerequisite or an adoption.

Enumerating all quadratic roots, checking each root and every root-free interval,
and exact joining at known chart endpoints gives a complete supported closed
face intersection. It distinguishes isolated contacts, retained tangent
generators and identically-zero boundary constraints. Arbitrary partial pair
coverage cannot establish scene completeness; the face operation itself needs
no broad phase. Unsupported plane/plane, wall/wall and general geometry fail
before any result. Allocation failure also fails construction rather than
publishing an empty or partial set.

Chart subdivisions are separate from connected components. The seam's a=0/4
lifts share one physical event, while vertical seam generators retain two
chart occurrences. Source orientation remains metadata; canonical spatial
direction is not a promised Boolean coedge orientation. Per-result IDs and
explicit source references avoid both coordinate welding and pointer-defined
topology. No edited body, persistent naming or Boolean edge is implied.

## Agreed minimal interface

* `FaceRef(snapshot, face_id)` retains an exact owned snapshot/face.
* Factory-only `IntersectionSet` exposes `left`, `right`, immutable `components`,
  `pieces`, `events`.
* Components expose `id`, `kind`, `closed`, `tangent`, `piece_ids`.
* Pieces expose `id`, `component_id`, `span_id`, `kind`, original `left_slot`, `right_slot`,
  exact `lo`, `hi`, `spatial_map`, `left_map`, `right_map`; delegated
  `position(parameter)`, `left_uv(parameter)`, `right_uv(parameter)`.
* Frozen `ExactMap` exposes `numerators` as coordinate tuples of ascending-power
  Scalar coefficients and `denominator` as an ascending-power coefficient tuple;
  `evaluate(parameter)` returns exact Scalar coordinates. Positive denominators
and source-map correspondence belong to construction, not sampled inference.
* Events expose `id`, all named `constraints`, chart `joins` and
  `incidences(piece_id,end)`, with a single point tag for point occurrences.
* `Scalar(a,b,d)` supports rational/Fraction coercion, exact comparison,
  supported arithmetic, `float()` and `bracket()`; it is unhashable.

This interface gives the sampler and independent oracle inspectable geometry,
source identity and topology without reading a private object graph. The
sampler's numeric target does not decide connectivity or exact trim membership.
Independent qualification must still attack graph components/events and whole
piece correspondence, including IT1's fixture. No remaining design blocker.

Final integrated re-review also accepts the independent oracle lane's
`span_id` correction: overlapping chart occurrences of a cardinal/seam
generator share one physical span. Graph degree and closedness count that
span once, while raw event incidences retain both UV occurrences. Point
occurrences share one span but add no graph edge. Horizontal interval pieces
each have their own span. This prevents two occurrence records from appearing
as parallel physical edges or a false cycle. The implementation owner must
preserve this distinction throughout construction and sampling provenance.
