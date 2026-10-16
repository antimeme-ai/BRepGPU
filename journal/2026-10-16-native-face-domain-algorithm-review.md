# Native face-domain design and plan: algorithm review

Independent numerical/algorithm review of `docs/NATIVE_FACE_DOMAIN_DESIGN.md`
and `docs/NATIVE_FACE_DOMAIN_PLAN.md`, after reading production predicate,
bridge, ear and Lawson behavior and the retained/new primary-source studies.

Disposition: **ACCEPT the amended design and implementation plan.** This is
permission to proceed to genuine fleet RED and the bounded owned unit, not a
claim of implemented correctness, measured speed or complete watch readiness.

The exact per-axis integer view preserves comparisons/orientation. Its squared
distance uses the required opposite-axis denominator weights. Doubled midpoint
winding avoids division. Signed base-2^32 multiplication can use the stated
uint64 bound; dynamic limbs and checked budget errors preserve the numerical
contract rather than silently narrowing it. Exact rank boxes preserve closed
touching/zero-width cases. Traversal-order-independent minimum successful j
preserves first crossing; boolean ear blockers need no sorting. Distinct bridge
occurrences remain distinct despite equal coordinate ranks.

Findings resolved in the design:

- Added a proven strict-convex/no-hole fast path; a point BVH alone can still
  scan broad boxes for fan ears. All turns must be strictly positive. A weakly
  convex triangular outline with a point on an ear diagonal shows why inclusive
  blockers prevent an all-nonnegative shortcut.
- Work accounting names limb operations and checks multiplication effort before
  entering its product loop. Dynamic memory accounting names bigint temporaries
  and vector capacities; complete-process RSS remains separate. Implementation
  must preserve that allocator ownership and measure accounting overhead.
- Structural minima, original-slot incidence, ID0 validity and ABI POD
  version/size/reserved validation are explicit.
- Exact squared bridge ordering intentionally changes any rounded-sqrt collision,
  not only exact-equal distances. Complete-domain oracles, rather than claimed
  old triangle equality, govern those changed outputs.

The practical performance plan measures normalization/build and complete call,
not just native predicate throughput. Sparse segment, strict convex, dense
concave and many-hole families distinguish the main index failure modes. The
actual spring snapshot remains to be reproduced; it is not already retained.
The predeclared isolated budget is a useful early rejection of an ineffective
implementation. There is no numerical basis yet for a whole-watch speedup.

Remaining implementation review focus: carry/borrow extrema, canonical limbs,
resource cleanup after exceptions, allocator propagation on copies/moves,
closed-box touching, bridge endpoint exemptions, source-node boundary chains,
ear stall/final ordering, and real measured candidate work. No test or CAD run
was performed by this read-only review.

Also reviewed `docs/NATIVE_CPP_RIGOR_PLAN.md`: ACCEPT its target-scoped pinned
compiler/warnings, separate release and instrumented profiles, actual compiled
header analysis, bounded setup probes and meaningful-oracle requirement for any
future fuzzer. It starts no additional geometric qualification campaign and
does not change the live watch files or CUDA configuration.
