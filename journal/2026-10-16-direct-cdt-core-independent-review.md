# Independent native direct CDT core review

Reviewed `native/direct_cdt/core.hpp/.cpp` during constructor implementation,
with source findings sent directly to its owner and root. This source review
does not execute geometry or replace the independent actual-output reader.

Found a blocking lower-tangent defect: the right-child endpoint advanced with
`origin_prev(rdi)`, which rotates around the same origin instead of advancing
along the right hull. The correct operation in this representation is
`next(sym(rdi))`. The owner confirmed the stalled tangent in an actual fleet
failure and fixed it. The separate right merge candidate's `origin_prev(base)`
is correct and was retained.

Found inconsistent hash-probe accounting in backward-shift deletion: the final
key lookup and final empty shift probe were not counted, unlike get/set. The
owner changed the loops to charge every examined entry. Backward-shift movement
correctly compares cyclic distances from each entry's home bucket, preserving
search chains across wraparound. Empty entries use a value sentinel, so valid
source key values are not reserved accidentally.

Asked about incident hints invalidated while other incident faces remain. The
owner gave a valid caller lifecycle argument: no star query occurs during an
erase/rebuild transaction; every main strip site is a boundary site recreated
by completed cavity output, while scratch queries concern freshly inserted u.
This avoids adding a hidden all-mesh scan to repair hints. The argument is a
between-transaction caller invariant, not a generic Mesh erase guarantee.

The following checked out under the selected representation:

- Alternating `(X,Y)` and `(Y,-X)` comparators preserve orientation. Positive
  metric/denominator scaling preserves their coordinate order. Exact tie order
  and the reconstructed hull extrema handle collinear child chains.
- Manual quad-edge triangle/chain derivation checked rotation, left-next,
  origin-previous, splice, connect, exterior hull traversal and face traversal.
  Complete four-record groups are recycled; checked handle bounds prevent
  sentinel/rotation overflow.
- Global original-point ranks and `+,-,+,-` height-column cofactors consistently
  refine true cocircular zero. Raw predicates still return geometric zero;
  mapped duplicate point rows and all-collinear rows remain zero. No XY turn
  is perturbed.
- Fixed-capacity directed-edge indexing, backward-shift deletion, face/free-slot
  reuse and checked planar capacities avoid storage proportional to historical
  flips. Shared-budget allocations and explicit work charges remain in the
  actual topology paths.
- Constraint strip side chains have the required orientation after reversing
  the left chain. Protected original constraints can be indexed before their
  insertion because input admission excludes crossings. Void flood uses the
  original directed boundaries, including holes, after randomized processing.
- The secondary cap retains cyclic signed facets, uses raw lifted visibility
  through the constructor tie helper, reconstructs saved neighbors in reverse
  removal order, and maps original point identity consistently. It does not
  normalize clockwise facets or add the SB XY fold guard to Chew insertion.
- Marking before recursive task execution is equivalent to marking after it:
  marks accumulate as a union and fan repair occurs only after the entire
  top-level insertion. Every temporary new triangle before that repair contains
  the freshly inserted occurrence u.

No additional algebraic defect was found in the signed-cap implementation.
Its all-four-collinear/repeated-projective-vertex behavior is still explicitly
qualified by actual completion checks and external semantics, rather than a
borrowed unconditional theorem. The initial broad small cohort had no secondary
calls. Root assigned the oracle owner bounded independent fixture discovery;
the reviewer avoided duplicating that search. A retained fully qualified actual
secondary-path input remains required before presenting that branch as tested.
Final positivity alone can reveal a valid-input defect; it cannot make such a
defect acceptable or establish full advertised coverage.

Suggested direct owning-cap inputs with old apex `(0,0)`: the valid DT neighbor
fan `[(10,0),(1,1),(0,10),(-10,0),(0,-10)]` can retain a clockwise seed whose
first three sites have XY orientation -80 but lifted visibility 1800 toward
the apex. Replacing `(1,1)` by `(5,5)` gives a valid protected-collinear neighbor
fan: its first three sites have XY orientation zero but lifted visibility 5000.
The latter is a genuine vertical lifted facet, not a collapsed 3D triangle.
Both have simple original domains and four cocircular cardinal sites. A direct
owning-cap fixture can execute this exact product subroutine without claiming
that a public whole-constructor run reached secondary repair.

Root's oracle owner independently attempted broader radial/scaling discovery,
reported no secondary calls and stopped. This reviewer ran no duplicate search.
Those no-hit runs do not establish the signed-cap branch's correctness.
