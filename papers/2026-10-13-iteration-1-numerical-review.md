# Iteration 1 numerical adversarial review

2026-10-13. Reviewed [iteration-1 design](../docs/ITERATION_1_DESIGN.md), [acceleration roadmap](../docs/ACCELERATION_ROADMAP.md), [topology model](../docs/TOPOLOGY.md) and the linked [numerical source study](2026-10-13-first-iteration-numerical-study.md). Scope is the complete numerical input/output contract, algorithms, adapters, direct qualification and fair measurement. No product implementation or runtime benchmark was examined.

**Assessment:** the core six-jet mathematics and bounded local primitive are coherent. The initial adapter had a concrete silent-error blocker, corrected during review by narrowing its exponent range and specifying sequential factors. Remaining findings below concern a conflicting fixture ordering, mandatory coverage of the supported numeric scope, cumulative error grades and resident timing semantics. These should be resolved before the implementation plan relies on this design.

## N1 — high, original adapter could emit silently wrong finite derivatives; corrected during review

The first draft accepted arbitrary finite positive scale and source widths, with derivative formulas containing `du²` and `du*dv`; checking finite inputs and final outputs cannot detect overflow or underflow of those intermediates.

Direct witness: local `S(s,t)=(s²,0,0)`, at `s=1/2`, with world scale `2^600`, source width `du=2^600` and `dv=1`. Correct world position is `(2^598,0,0)`, first source derivative `(1,0,0)` and pure second source derivative `(2^-599,0,0)`. All are finite; the second derivative is a normal binary64 value. Forming `du²` overflows, and dividing local `Sss` by it before multiplying by scale returns a wrong zero that still passes final finiteness.

The updated design now restricts scale and widths to `[2^-20,2^20]`, origins to magnitude at most `2^40`, and local jets to the declared magnitude envelope. It computes `scale/du`, `scale/dv`, `(scale/du)/du`, `(scale/du)/dv`, `(scale/dv)/dv`. Those factors stay within `[2^-60,2^60]`; combined with local second components below `2^22`, they have ample range margin. The earlier witness now returns unsupported conversion. This resolves the blocker without requiring a broad exponent-aware adapter in iteration 1.

Direct oracles: reject the `2^600` witness; accept exponent-boundary widths/scales within the new envelope and compare restored polynomial/cylinder derivatives to exact rational expressions. Test large supported origins separately from local error; relative error on a translated position must not hide lost local detail. Conversion remains empirically qualified, not a universal enclosure. Nonlinear analytic-angle conversions are explicitly outside scope and must remain unsupported.

## N2 — medium, linked direct fixture has a different flattened order

The design defines homogeneous controls by `i*(q+1)+j`, so `j` varies fastest. The linked numerical study proposes `i+(p+1)j` and lists the bilinear controls as `(0,0,0,1)`, `(2,0,0,2)`, `(0,2,0,2)`, `(3,3,3,3)` in that different ordering. Reusing the literal list under the design's layout swaps the first and second parameter directions. Position at the symmetric half/half query happens to agree, while the first and second derivative vectors disagree with the stated direct oracle.

Practical fix: standardize the fixture list to the design ordering or label every control by `(i,j)` and make packing explicit. Under `i*(q+1)+j`, the correct list is:

```text
H00=(0,0,0,1), H01=(0,2,0,2),
H10=(2,0,0,2), H11=(3,3,3,3).
```

Direct oracle: preserve the six dyadic expected vectors already derived in the study, including `Ss=(15/16,-1/16,9/16)` and `St=(-1/16,15/16,9/16)` at half/half. Also query an asymmetric parameter pair and use the closed rational expressions. A symmetric position-only witness is insufficient for grid orientation.

## N3 — medium, qualification needs the supported degree and numeric boundaries explicitly

The primitive supports all sixteen degree pairs in `0..3`, including constant directions, but its named demonstration carriers exercise only part of that matrix. “Mixed-degree batches” alone does not require `(0,3)`, `(3,0)`, `(1,2)`, `(2,1)` and other asymmetric paths. Likewise, nonuniform weights do not by themselves exercise the declared `1`/`256` endpoints or cancellation near a zero coordinate. A few smooth grids can pass while a degree-zero path reads a nonexistent derivative control or an asymmetric path transposes row scratch.

Practical fix: make the small sixteen-pair qualification matrix mandatory. Include corner/edge endpoints, adjacent representable parameters inside the square, actual weight endpoints and mixed large/small weights, nearly canceling numerator components, and a singular chart whose jet still succeeds. Check all requested orders `0/1/2` on the same supported data. Unsupported/nonfinite input cases remain explicit failures and cannot be folded into qualified timings.

Direct oracles: use polynomial monomials with known derivatives and degree elevation to populate all degree pairs; constant patches test missing-direction derivatives. Nonuniform positive-weight data are checked against exact represented-input rational sums. The bilinear rational fixture already supplies nonzero pure second derivatives below polynomial degree two. Use next representable values immediately outside the closed square to test refusal, keeping them distinct from the inside boundary fixtures.

Metamorphic common-weight scaling and affine transformations must either keep both inputs inside the supported envelope or expect unsupported creation; a transformation outside the contract cannot be used as an unconditional “both runs succeed” invariant. Subdivision tests must separate floating extraction/transform error from evaluation error on the resulting controls.

## N4 — medium, cumulative order targets should be stated explicitly

The table has separate position, first and second targets, but “each corpus request must meet its output grade” can be read as applying the batch's largest-order target to every returned field. That would permit position error near `2^-20` in an order-2 request despite the intended `2^-40` positional target.

Practical fix: require order 0 to meet `E0`; order 1 to meet `E0` on position and `E1` on first derivatives; order 2 to meet `E0`, `E1` and `E2` on the corresponding fields. Reject unsupported derivative orders structurally before allocating output. Compare shared fields across order-0/1/2 calls to detect branch-specific errors without requiring bitwise equality between differently scheduled algorithms.

Direct oracle: a known exact position plus nonzero first/second polynomial or rational derivatives provides separate component error grades. A returned position with absolute error between `E0` and `E2` fails an order-2 request even if its other fields meet their budgets. Qualification compares the returned fields directly to their mathematical expected values.

The design correctly states that these empirical corpus targets are not bounds emitted at runtime. `ok` means validated input and finite arithmetic, not certified geometry. No new blanket precision or topology-success rule is required.

## N5 — medium, prepared-output timing is not supplied by the public result API

The API creates a new owned result handle from `evaluate(query_set,order)`. The benchmark prose says resident reruns use prepared buffers, but does not provide a reusable result target or allocator policy. Excluding result allocation from a metric labeled public resident evaluation would understate the operation that callers can actually perform.

Practical fix without enlarging the API: clarify that patch/query inputs are prepared; result allocation, required checks and synchronization remain in public resident-call wall time. Raw kernel-only repeats may use private preallocated outputs and CUDA events, explicitly labeled as a component. Their native CPU comparator uses equivalent preallocated outputs and requested fields. Host-to-host timing includes all required copies and transformations. If a future allocator pool or `evaluate_into` API is added, it needs explicit ownership and timing semantics, not an assumption here.

Direct measurement oracle: count and report the bytes and allocations performed by the actual timed public calls, and show their synchronized wall time beside kernel-only events. Keep output handles alive according to the measured scenario and include retained storage in memory results. CPU scalar Python binding overhead is already correctly separated from native arithmetic throughput.

## N6 — low, failed arithmetic output contents should match the query-failure rule

The failure table explicitly makes requested numeric fields NaN for invalid query input, but its arithmetic-failure row only denies a successful geometry claim. This leaves partial finite coordinates beside an arithmetic status as a possible implementation interpretation.

Practical fix: specify that every non-`ok` request emits NaN in all requested Cartesian components, whatever the per-query failure cause. Batch failure still publishes no usable result at all. The status must remain visible; NaN filling is not a substitute for it. This gives downstream samplers one consistent failure representation and prevents accidental reuse of a plausible partial position.

## Positive conclusions and implementation cautions

The degree/range envelope, quotient equations and exact component magnitude bounds are sound as stated. Positive weights keep the denominator away from zero mathematically, while the design correctly declines relative-error guarantees near canceled coordinates, small derivatives or singular charts. CPU Bernstein sums and CUDA tensor de Casteljau have different execution structures; exact and analytic truth remain necessary because both still share the quotient mathematics.

The normalized local API is a useful owned operation. The plane, asymmetric bicubic, rational cylinder and weighted bilinear generator provide real sampling callers without claiming trim-aware meshing or SMLib Boolean acceleration. Source rectangles identify the returned chart, and the optional adapter is allowed to refuse nonlinear/unsupported correspondence. The roadmap does not advertise the existing hole-cut result as accelerated.

The component-major output layout and four-double homogeneous controls are coherent; full jets are 144 bytes per request. One query per thread is a defensible initial schedule, but a general per-thread array of all homogeneous row jets can create register pressure. Keep bounded scalar-channel scratch as an implementation-plan candidate; record compiled registers/spills and actual timing before choosing shared tiles or a forced register cap. No hardware speedup follows from these source-level estimates.

The updated query-set binding retains one immutable patch set and provenance, preventing slots from silently referring to another same-sized set. This agrees with the topology document's snapshot/slot distinction. No further numerical identity contradiction was found in the reviewed draft.

This review is source/design analysis only. Findings N2–N6 require a document decision; N1 was corrected during the review. Product behavior, qualification outcomes and runtime failure handling remain work for implementation and its direct tests.

## Re-review disposition

Re-read the integrated iteration-1 design and linked numerical-study revisions after the owner resolved the findings. **No numerical design blocker remains for writing the implementation plan.** This conclusion concerns the reviewed contract; it does not certify unimplemented code or future benchmark results.

| Finding | Disposition | Integrated behavior checked |
| --- | --- | --- |
| N1 — adapter intermediate range | Resolved | Scale/width/origin limits and sequential conversion factors are explicit; the `2^600` witness is unsupported. Wider exponent support is deferred. |
| N2 — direct fixture packing | Resolved | Design and source report both select `i*(q+1)+j`; the report's literal list is now labeled `H00,H01,H10,H11` in that order. The dyadic derivative oracle still matches the stated rational map. |
| N3 — supported scope qualification | Resolved | All sixteen degree pairs and three requested orders are mandatory, with endpoints/inward neighbors, cancellation, weight endpoints, singular charts and invalid outward/nonfinite/slot cases. |
| N4 — cumulative error grades | Resolved | The design explicitly applies `E0` to position, `E1` to first derivatives and `E2` to second derivatives in an order-2 result; unsupported orders fail structurally. |
| N5 — resident output allocation | Resolved | Public resident calls reuse inputs but allocate a new result and pay allocation/check/launch/synchronization/publication costs. Private preallocated kernel timing is labeled separately with an equivalent CPU setup; cleanup and retained memory are reported. |
| N6 — failed numeric payload | Resolved | Every non-`ok` request fills all requested fields with NaN and retains mandatory status; batch/runtime failures have separate unusable-result behavior. |

The numerical execution choices are also clearer: CUDA uses the explicit nested-FMA interpolation primitive; implicit contraction and unsafe math are disabled; direct quotient division remains initial behavior, with a shared reciprocal reserved for separately qualified optimization. CPU direct basis sums and CUDA tensor evaluation remain different paths under one output contract.

The linked source study distinguishes its broader affine-matrix research proposal from the integrated uniform positive-scale adapter. That narrower first adapter is a coherent scope decision. It still separates local evaluator errors, parameter-map rounding and world restoration errors, while avoiding a claim that empirical corpus budgets enclose runtime geometric results.

Implementation-plan review should carry forward this exact local layout, cumulative grades and caller-visible timing. Register/spill inspection and operation-specific direct tests will decide implementation details; no additional numerical redesign or generic proof exercise is requested by this re-review.
