# Direct CDT constructor: public tests and performance experiment

Status: proposed for root review before test/benchmark implementation. This
unit owns actual public-constructor tests and independently qualified workload
comparisons. Native construction and Python binding have separate owners.
All geometry, tests and benchmarks execute on the isolated fleet checkout.

## Consumer contract and missing-capability RED

The agreed public seam is

```
constrained_delaunay(loops, points, *, metric=(1.,1.), seed=0, token=0,
                     input_limit=1<<28, memory_limit=1<<30, work_limit=1<<34)
```

It returns tuples of original uint64 source-ID triples. `DirectCDT` owns the
same input/preparation arguments, `.triangulate()`, `.stats`, `.normalize_ns`,
`.encoded_bytes`, and deterministic close/context-manager lifetime. The separate
library is selected by `BREPGPU_DIRECT_CDT_LIBRARY`; the old ear library remains
available for a matched same-input baseline. No GPU operation is claimed.

Before product implementation is available, add `tests/test_direct_cdt.py` and
run it on the new isolated checkout. Retain genuine missing-public-entry RED;
do not import the future API in the already completed oracle-only test unit.

Actual constructor outputs are checked with the existing independent
`qualify_cdt`: original row identity, exact domain coverage/embedding, original
constraints, and full generic Fraction 4x4 local-circle determinants. Different
legal cocircular diagonals are accepted. Existing Python output is a timed
baseline, never an answer key.

Focused methods cover all seven authored families and five original external
domains, isotropic/anisotropic diagonal reversal, exact cocircular sites,
protected collinear sites, multiple independent seeds and reordered/remapped
source inputs. An identical seed/input is repeatable; different seeds or source
renaming need not choose identical legal zero diagonals. Include wrong/unknown
loops, crossing domain, nonpositive/nonfinite metric, unsigned-ID/range issues,
input/memory/work exhaustion, closed owner and successful independent owner
after another fails. Do not require an invalid operation to return a mesh.

## Complete large-output qualification without pairwise output scans

Keep the bounded pairwise embedding checker unchanged. For large authored
families, share its exact original-ID/positive-chain/incidence/count reader,
substitute independently checked family input admissibility, and omit pairwise
OUTPUT geometry enumeration on the following exact mathematical basis.

The sum of oriented positive triangle boundaries equals the oriented boundary
of the valid original domain. Winding number is additive on these finite chains.
At every point away from all edges, each positive triangle contributes exactly
its interior indicator, while the original domain contributes one inside its
material and zero elsewhere. Therefore output coverage multiplicity is exactly
one inside and zero outside. Any overlap or crossing of positive triangle
interiors has a nonempty open region of multiplicity at least two, and any
exterior triangle has an open region of positive multiplicity outside; both
contradict chain equality. This is a complete coverage/embedding argument,
not an area-only checksum.

The reader also requires every source boundary segment to be one identical
directed source-ID edge, and every other edge to have exactly two opposite
triangle uses. A hanging subsegment cannot cancel an unsplit edge in this
discrete exact edge map. For a putative nonshared vertex on an internal edge,
the two incident positive triangles occupy opposite open half-neighborhoods of
the edge at that point; another positive triangle incident to that vertex would
overlap one of them. At a source boundary segment, an interior source site would
already violate independently established input simplicity/protected segment
inventory. Thus neither endpoint-on-edge contacts nor collinear partially
overlapping internal edges can survive the exact chain/incidence conditions.
Unique exact original site coordinates preclude separate IDs hiding coincident
whole edges or endpoints. Triangle duplicate/inversion/collapse is rejected.

The large reader then performs the independent Fraction 4x4 determinant on
**every** unconstrained two-owner edge, with the actual metric. It never reads
producer topology flags or skips validation because construction returned OK.
Qualification time and memory remain reported separately from product time.

This substitution is only usable after independent input validity is checked
against actual coordinates, not inferred from a generator name. Four initial
large families have short exact checks:

- Strict convex parabola: loop x coordinates strictly increase, all y equal
  x squared; closing chord and strict convexity give a simple positive domain.
- Protected strip: two reversed copies of the same strictly increasing station
  sequence at two different fixed heights, closed by vertical sides. No samples
  collapse; actual coordinates establish a simple positive rectangle boundary.
- Concave comb/corridor: one flat lower chain and an upper chain monotone in x,
  with strict positive vertical separation and at most one vertical step per
  repeated x. Actual chain order and coordinates establish no crossing or touch.
- Separated holes: exact rectangular outer, clockwise unit square holes whose
  lower-left corners occupy distinct integral lattice cells with spacing four;
  all actual corners and strict interior bounds checked. Separation follows
  from those coordinates, without a quadratic hole-pair scan.

Reuse shared chain/incidence logic rather than create a second copy. Arbitrary
captured domains without a suitable independent input reader keep the bounded
checker or a visible INPUT_NOT_QUALIFIED result; never turn skipped input
validation into PASS. A general exact segment sweep can be a later separate
reader if arbitrary large nonconstructive domains require it.

## Fixed workload cohort and performance reporting

`scripts/benchmark_direct_cdt.py` runs fresh single-core subprocesses, with
CUDA hidden and fixed CPU affinity chosen by the invoker. Use exact same source
loops/points/metric in direct and native-ear→Python-Lawson comparisons. Include
both complete caller time and exposed normalization, validation, construction,
recovery, predicate, publication, and memory/work counters actually available.
Missing stage counters stay explicitly unavailable, never invented from sums.

Before tuning select the four scalable families above at target sizes
64, 256, 1024, 4096 and 8192 (actual count is reported when hole packing differs).
Use seeds 0, 1 and 2 for direct construction, isotropic metric for every family,
and a separate anisotropic strip/comb matrix with `(16.,.25)`. Run bounded
correctness first; then initial breadth timing across all four families; then
the complete selected scaling matrix. The watch is a separately identified
regression after those independent families, not a tuning denominator.

Each row records input generation/validation, product preparation/solve/output,
independent qualification, native high-water memory and whole-process peak RSS
where available. Baseline setup is included: native ear creation and output,
Python metric normalization, Lawson construction and output. Preserve earlier
results if a later case fails; every case gets an explicit outcome. A configured
per-worker deadline censors the last observed phase, retaining completed phase
times. A completed solve followed by a qualification timeout is QUALIFY_TIMEOUT,
never qualified PASS. No ratio is claimed from censored baseline wall time.

Generate TSV reports as presentation, retain stderr/logs privately, and make
timeouts/resource failures/unfavorable scaling visible. Inputs are owned exact
Python values; do not add JSON internal persistence or a dependency. A benchmark
is complete after meaningful correctness/performance evidence and independent
review; it does not start a certification campaign.
