# Next native unit: fixed-vertex weighted Lawson connectivity

Read-only proposal after the first native domain slice exposed another active
Python stage. Root reports a three-second nonblocking sample with210 valid
stacks and89 reader errors:167 valid stacks in `_improve_connectivity`,5 in plane
UV work and1 in surface error. These samples identify executing code, **not**
whole-run percentages. The438-face definition1713 trial subsequently censored at
its predeclared300s deadline, with last progress200/438 faces and no completed
part qualification. This proposal starts no replacement run, benchmark or
implementation; no new native connectivity implementation is selected here.

## Bounded recommendation

Extend the owned C++ core with **fixed-vertex weighted Lawson flips**, reusing
the exact integers and source-owned point context. Keep current metric values,
constraint keys, initial triangle order, FIFO edge schedule, strict positive
incircle rule and original occurrence IDs. Use packed triangle/edge arrays and
a bounded index rather than translating the Python dictionary/set/deque objects.
Do not bundle native refinement, new Delaunay construction, filters, a new metric,
parallel flips or a source-model rewrite into this unit.

The current function already changed rational arithmetic into an integer view.
Therefore repeating that optimization in Python would not remove its present
interpreter/container work. Native ownership is a concrete next slice, while
actual predicate counts and limb widths must determine whether arithmetic or
topology dominates the resulting native version.

## Exact numerical contract

Let original exact UVs be `(X/Du,Y/Dv)` with positive common axis denominators.
Let `mu,mv` be the actual existing positive finite binary64 metric values,
interpreted exactly through Fraction; **do not recompute the metric natively**.
The relevant Euclidean coordinates are `(mu*u,mv*v)`, not raw integer XY.

Choose positive integer weights using the existing rule:

`Rx = exact(mu)^2 / Du^2`, `Ry = exact(mv)^2 / Dv^2`,
`W = lcm(denominator(Rx), denominator(Ry))`,
`wx = W*Rx`, `wy = W*Ry`.

For positive triangle `(a,b,c)` and opposite point d, define integer differences
`(xi,yi)=integer_point(i)-integer_point(d)` and `li=wx*xi^2+wy*yi^2`.
Then evaluate

`I = la*(xb*yc-yb*xc) + lb*(xc*ya-yc*xa) + lc*(xa*yb-ya*xb)`.

This equals the weighted-lift determinant in original UV axes times the positive
`W*Du*Dv`. The ordinary incircle determinant in metric-space coordinates has
the additional positive cross-column factor `mu*mv`; equivalently it equals
`mu*mv*I/(W*Du*Dv)`. Thus sign and exact zero are unchanged. The existing domain engine's
**bridge weights `Dv^2,Du^2` are not these metric weights**. Give the native
operation separate explicitly named weights/descriptor, and bound their
normalization/encoded bytes before allocation as in the corrected domain input.
No new native gcd/division or numeric dependency is needed.

Preserve all existing guards: four distinct node identities; opposite-side
orientation; both proposed triangles strictly positive; proposed diagonal absent
from all current incidence; selected edge unconstrained with exactly two owners.
Flip only for `I>0`. Exact cocircular zero retains the current diagonal, with no
epsilon or symbolic tie-break introduced. The Python model remains independent.

Existing tests already exercise anisotropic metric behavior, constrained
diagonals, concavity, same-side overlapping input, exact shifts and cocircular
zero. Reuse those fault classes. If a new low-level API deliberately rejects
malformed embedding instead of returning an unchanged soup, declare its narrower
admission rather than claiming parity on malformed inputs.

### Fixed-point lift cache can remove arithmetic, too

Because all vertices stay fixed, compute once
`Li=wx*(Xi-X0)^2+wy*(Yi-Y0)^2` for one fixed source-point anchor `(X0,Y0)`.
In the incircle expression use `Li-Ld` instead of the per-query translated
square `li=wx*(Xi-Xd)^2+wy*(Yi-Yd)^2`. Write `dxi=Xi-Xd`, `dyi=Yi-Yd`,
`xd=Xd-X0`, `yd=Yd-Y0`. Expanding gives
`Li-Ld = li + 2*wx*xd*dxi + 2*wy*yd*dyi`.
The three-row incircle matrix has columns `[dx,dy,l]`. Replacing its third
column by `l + (2*wx*xd)*dx + (2*wy*yd)*dy` leaves the determinant unchanged:
multilinearity adds two determinants with repeated columns, both exactly zero.
No approximation, division, sign convention or zero decision changes.

The current expression uses approximately21 integer multiplications per query:
12 for weighted squared differences,6 for cross products and3 for lift/cross
products. Cached lifts use9, after4 per input point. The anchor avoids inflating
the cache with a large irrelevant UV origin; it does not require storing another
coordinate table. Cached lift differences may still have greater bit widths
than local translated squares, and storing them consumes memory. Compare full
native solve and peak bytes/limbs rather than assuming the operation-count ratio
is the runtime ratio. This is a concrete exact option inside the same fixed-
vertex unit, distinct from later adaptive filters or a general expression DAG.
Bind the cache to the exact point/preparation identity, metric weights and chosen
anchor. Invalidate/rebuild it if any changes; mixing anchors between point lifts
does not satisfy the column argument. Original UVs and source records never
change to create this predicate view.

## Packed topology with stable ownership

Store an immutable point table; fixed triangle slots with three NodeSlots and
three EdgeSlots; and dense edge records with two endpoint NodeSlots, up to two
triangle-owner slots, constrained/queued flags. Strong NodeSlot, TriangleSlot and
EdgeSlot types separate identity classes. Source IDs remain original uint64s;
no coordinate rounding, source-map rewriting or vertex insertion occurs.

Build three edge uses per input triangle, sort canonical keys once and group
them. Reject repeated triangle nodes, nonpositive triangles and more than two
owners. Mark constraints through the input set. Record explicitly what happens
to a supplied constraint absent from the input mesh. Input triangles keep their
original list positions; a flip updates its two existing slots in place.

Use an owned flat open-address index from canonical dense endpoint pair to
EdgeSlot for proposed-diagonal existence and key replacement. A sorted vector
with O(E) insertion per flip would reintroduce avoidable work. A pointer-per-node
map is an easier baseline but is a poor final high-density ownership model.
Allocate index capacity once from a checked edge-count/load bound: the number
of live edges stays fixed across admitted flips. Deletion must support bounded
back-shift or a declared bounded tombstone rebuild; never retain every historical
diagonal or silently accumulate O(flips) storage. Count probe work in the same
work budget. This is an engineering choice to review, not imported code.

For each pop, choose `left,right` by increasing original triangle ordinal, as
the existing function does. Rotate their current words to
`left=(a,b,c), right=(b,a,d)`. If an orientation guard fails, retain the edge.
For a legal positive-incircle flip emit exactly
`left=(c,d,b), right=(d,c,a)`.

Four perimeter edge slots survive. Only `ca` changes owner left→right and `db`
changes right→left; `bc` and `ad` keep their owners. The new triangle edge words
are `[newDiagonal,db,bc]` and `[newDiagonal,ca,ad]`. Store owner triangle IDs,
not pointers or fragile owner-local-edge indices; finding a popped EdgeSlot in
one triangle's three slots is bounded constant work. If storing local indices
instead, **all four perimeter locators** need updating even when their triangle
owner ID stays the same, because triangle rotations changed their positions.

### Queue identity without a generation maze

In this serial operation only the currently popped diagonal disappears. Four
perimeter edges remain. Since pending membership is cleared on pop, the deleted
diagonal's EdgeSlot is unqueued and can be reused for the new diagonal. Other
queued slots keep their keys. Thus a fixed-capacity FIFO of EdgeSlots with one
queued bit per live edge replaces both the Python deque and pending set;
queue occupancy cannot exceed the edge count.

This relies on the stated serial local-flip invariant. If later code deletes
other queued edges or performs parallel mutation, it needs explicit stale-entry
generations; do not assume the argument carries into a general topology editor.

Initialize the FIFO in lexicographic **original source-ID edge-key order**.
After a flip, sort the four perimeter keys and new diagonal by that same order,
then enqueue live unconstrained edges not already queued. The removed old key
is skipped, exactly as in the current affected-set loop. Dense NodeSlot order
may differ from source-ID order; an arbitrary-ID permutation case must catch
accidentally sorting by dense slots. Repeated visits and zero tests remain part
of the measured operation.

## API, errors and publication

Expose one coarse connectivity call on an owned exact point context or an
equivalently bounded native operation. Inputs include the ordered initial
triangle triples, constrained pairs, separate metric weights and source token.
Decode original IDs to dense slots once. Preserve output triangle order and
original IDs. A combined domain→connectivity call may eventually avoid packing,
but the first migration must measure its actual boundary cost rather than hide
it. Do not shoehorn metric weights into the bridge-distance fields.

Keep actual native input/scratch/work/allocation failures distinct from a
geometric no-flip answer. Preserve the current `8*N^2` flip cap with checked
count arithmetic and declare resource rejection if it cannot be represented;
never wrap or silently widen it. Budget queue/probe/limb work and high-water
array/index/temporary capacities. On failure publish no partial optimized mesh
and make earlier completed output unavailable under the established owner policy.
Do not overwrite the immutable initial triangles, which are needed for retry
and independent comparison. Existing point/source ownership outlives the call.

## Capture the actual operation before porting it

A source-face name and point count are insufficient. Retain the exact values
at `_improve_connectivity` entry: point IDs and rational UVs, **ordered initial
triangle list**, constrained edge pairs, original metric binary64 bits, node
loops/source token and source/preparation/quality/code pins. The active mutated
triangle list sampled midway is not the original caller input.

After the current trial finishes or censors at its existing deadline, a bounded
controlled replay can capture that entry and deliberately stop before the old
expensive Lawson call. Mark it INPUT_CAPTURED, not complete-part success. This
is preferable to inventing a synthetic mesh from stack counts or changing the
running pinned process. Reuse the existing lossless face snapshot for coordinates
and owned binary arrays for ordered triangles/constraints/metric; no JSON model.
Original CAD-derived input stays private. Capture time is separate from replay
construction and complete-part time.

## Correctness and brutal performance experiments

1. Before code, review the exact metric/queue/local-update contract and bounded
   plan. Genuine authored RED targets the missing native connectivity capability.
2. Direct hand-derived weighted incircle signs, zero and wide cancellation;
   the existing metric/constraint guards; arbitrary original IDs; a queued
   perimeter edge surviving several flips; and reused diagonal slots changing
   keys. These target exactness, scheduler identity and ownership faults.
3. Compare ordered native output with the unchanged Python model on identical
   fixed inputs. Independently check positive original-domain boundary chains
   and complete unchanged constraints; a matching wrong model is insufficient.
   Source/physical/quality checks consume the actual eventual complete part.
4. Measure fresh optimized one-core normalization/weight packing, native decode,
   index construction, queue/flip solve and output. Record queue pops/max
   occupancy, flips, convexity/duplicate-diagonal/zero skips, exact incircle
   calls, maximum limbs, lookup probes and peak bytes/RSS. Explain counts with
   whole wall; sanitizers are separate. Keep hardware and counter settings fixed.
5. Include the existing thin-strip fan at increasing sizes, a cocircular grid
   stressing zero retention, constrained diagonals, anisotropy and a retained
   actual pre-Lawson caller. A strip fan can require many local flips; native
   execution must report that work rather than claim a new asymptotic algorithm.
   Compare complete connectivity, then complete-part costs. Baseline timeout is
   censored; no numeric speedup against an unfinished case.

Select a practical isolated budget against the captured workload before running
it. Only then repeat the complete part under its unchanged quality and existing
independent oracles. A faster connectivity kernel may reveal refinement or
another stage; preserve the bounded diagnosis cycle rather than promising the
whole watch from a microbenchmark.

Primary literature reread: Shewchuk's [triangle records/constraint layout](https://www.cs.cmu.edu/~quake/tripaper/triangle2.html)
and [exact orientation/incircle discussion](https://www.cs.cmu.edu/~quake/tripaper/triangle6.html),
1996. These motivate locality and exact signs; neither supplies a speed forecast
for this implementation or licenses erasing seam/bridge occurrences. Existing
source/restoration notes are in the retained import/connectivity studies.
