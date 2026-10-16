# Direct CDT native boundary

This delivery adds a separate owned host-only C++20 constructor and a thin
Python binding. It does not replace the running watch worker or change the
existing sampled-domain ABI layouts. The constructor design and algorithm plan
are in DIRECT_CDT_DESIGN.md and DIRECT_CDT_PLAN.md as they are reviewed.

## Source and numerical contract

`brepgpu.face_domain.DirectCDT` owns one immutable face input. Its constructor
accepts the existing loop words and exact rational source points, a positive
constant diagonal metric, a uint64 seed and the existing input/native/work
budgets. `triangulate()` returns positive source-ID triples; `stats` reports
actual native work and memory. `constrained_delaunay(...)` is the owning
one-call convenience function. IDs are actual Python integers in uint64 range;
bools and float aliases are rejected before dictionary lookup. The public
domain remains one simple positive outer loop and strictly contained disjoint
negative holes, every distinct site appearing exactly once in the loop words.

The binding uses the existing lossless common-denominator preparation. For
coordinates `(X/Du,Y/Dv)` and exact positive metric scales `(mu,mv)`, it encodes
positive integral weights proportional to `(mu**2/Du**2,mv**2/Dv**2)` using the
least common multiple of those rational denominators, with a common integer
factor removed. Equivalently cross-cancel the factors in
`(numerator(mu)*Dv*denominator(mv)) /
(numerator(mv)*Du*denominator(mu))` before forming the two squared integers.
Check aggregate product bit/encoded-byte bounds before multiplication and
squaring, then count the actual metric integers in the input byte budget.
These are metric lift
weights, separately computed from the old bridge-distance weights. Input
packing, metric preparation and output copying remain in complete-call timing.

## Native lifetime and failure

`libbrepgpu_direct_cdt.so` is selected by `BREPGPU_DIRECT_CDT_LIBRARY` or the
explicit direct-CDT build directories. Its independently checked ABI has sized
POD input/error/statistics records, an opaque owning context, and create,
construct, copy-result, read-statistics and destroy operations. It reuses the
existing exact integer descriptor and node/loop records. No Python callback
crosses a predicate or topology operation. All exceptions are caught at the C
boundary. No result is published after construction fails, and destruction is
null-safe. A context attempts construction once; success caches immutable
connectivity and repeat calls copy it, while failure is terminal. Neither path
resets its cumulative work budget. Copying a completed result never mutates
its connectivity. Seeds, tokens and budgets require actual uint64 integers.

Before direct construction, an additive `bgfd_validate_domain` export checks
the original sampled boundary with the existing exact indexed engine, without
ear triangulation. It additionally requires a complete source-site inventory.
That temporary context is destroyed before allocating the direct point owner.
Validation work is subtracted from the remaining operation budget and reported
alongside constructor work; native peak is the maximum of these sequential
lifetimes, including the context objects. Destroy the temporary validation
context on every failure path as well. Whole-process RSS separately includes
simultaneously live Python packing arrays. Decode/preparation/validation costs
remain visible. The constructor uses a shared bounded PMR resource for owned
points, arithmetic, topology and output. No source-dependent admission or
capacity is introduced.

## Implementation and direct checks

1. Review this boundary plan against the constructor and predicate interfaces.
2. Run a real missing-public-entry RED on the fleet before implementation.
3. Add the validation export and direct C ABI, exact metric packing and owning
   Python interface. Add a standalone strict native build and extend the scoped
   actual-source analysis to the reached direct sources and headers.
4. Run the existing independent CDT oracle against actual constructor output
   on authored and original external cases, metrics, seeds and permutations.
   Exercise raw ABI layout, malformed records, resource failures and result
   invalidation with instrumented native fixtures.
5. Review production code independently. Resolve findings, then measure full
   calls across the selected general families and retained real inputs. Record
   qualification time separately; unqualified/censored output is not a pass.
6. Connect to the real mesher only after those results justify changing its
   connectivity path. Retain the old constructor as an explicit baseline.

All native builds, tests and benchmarks run on the Linux compute fleet. This
unit adds no third-party production dependency or GPU timing claim.
