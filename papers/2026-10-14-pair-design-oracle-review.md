# Pair discovery design: independent oracle review

Reviewed `docs/PAIR_DISCOVERY_DESIGN.md` against the independent pair study,
original depth-zero hierarchy inputs and current body identity model. Design
review only; no implementation, product import or native execution.

## P-OR1 — clarify multi-body face selection (medium; blocking API ambiguity)

`prepare_scene(cpu_engine,bodies,face_ids=None)` describes selected face IDs,
but face IDs are only unique within a body namespace: box face 1 and cylinder
face 1 are distinct sources. A bare integer cannot identify one of them in a
multi-body scene. Define selections as full `(body_id,face_id)` keys, or an
explicit per-body mapping with equivalent identity. Reject duplicate/unknown
full keys, preserve input-body/canonical-chart order, and allow empty selection.
Tests should include two bodies with repeated local face IDs and select only
one occurrence. This makes original-source checks meaningful and prevents
accidental face selection by globally assumed IDs.

## Independent truth and narrowness accepted

The remainder has no identified oracle blocker:

- Original submitted homogeneous Fractions establish whole-carrier convex-hull
  enclosure. Actual root downloads and actual world boxes are checked against
  both original extrema and exact transformed root endpoints. This binds the
  geometric claim to inputs used by the execution, rather than ideal bounds.
- Exact Fraction multiply/add followed by outward final rounding is valid.
  The earlier study's warning about separate outward primitives applies to
  rounded FP implementations; it does not prohibit this exact-expression route.
- Bit truth uses actual uploaded/downloaded finite world boxes, all three
  closed inequalities, all bits and tail padding. Equality, degenerate boxes,
  signed zero and subnormals are explicit, with no geometry-tolerance proxy.
- Page-local bits identify `start+k`, rather than absolute word alignment.
  Cursor progress counts examined pairs. Start==total/empty sides are valid;
  zero budgets and out-of-range starts are rejected. End flags and full-domain
  flags distinguish a terminal page from complete traversal.
- Actual source snapshots/frame/chart identities remain retained after public
  close; opaque native handles are not geometric IDs or device pointers.
  Failed download cannot publish an apparently qualified all-zero page.
- Known contact witnesses survive. Concentric cylinder and plate-hole false
  positives are required examples of the bounded carrier-box scope. No SSI,
  trim-overlap, unique intersection curve or Boolean classification is claimed.

For the implementation plan, include one bounded metadata/index arithmetic
case at 65,536-by-65,536: total is exactly 2^32, start=2^32−1 examines the final
pair, and start=2^32 produces an empty terminal page. This attacks the meaningful
uint64 boundary without allocating an all-pairs output. Also keep differing-frame
coincidence and local-equal/world-separated cases, unaligned start31/33, asymmetric
dimensions and a page concatenation check. These are direct fault classes rather
than another broad qualification framework.

## Disposition

Root integrated explicit `(body_id,face_id)` selection keys into the design.
P-OR1 resolved. Corrected design accepted with no remaining oracle blocker.
