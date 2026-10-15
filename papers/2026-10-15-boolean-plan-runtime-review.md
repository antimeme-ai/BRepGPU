# Boolean delivery plan: native consumer/runtime review

Reviewed `docs/BOOLEAN_PLAN.md` against the accepted design and existing
Engine/JetBatch/child cleanup APIs. **Accepted, no blocker.** No code or execution.

The whole CPU result/history unit and independent whole-geometry oracle remain
separate from the sampling consumer. Distinct result/request types preserve
canonical admission. Candidate conversion precedes exact trim classification;
the original cutter V band and per-Patch frame survive sampling and conversion.
Native returned slots, UV bits, counts/order, descriptors, output Lineage, statuses
and backend must bind to the actual request before rows/normals are published.
This is geometry consumption, not GPU Boolean construction.

The complete sample operation owns patch/query/result children and borrows Engine;
late host failure closes all children and leaves the same result/Engine available
for a healthy retry. Existing primary-error-preserving child helper can be reused.
Candidate limits are enforced while copying, before unbounded input materializes;
invalid/rejected rows are explicit without submission. Per-call patch creation
and cleanup belong in sample timing, with Engine creation/closure in caller wall.
Independent truth, output and optional reference costs remain outside the useful
operation and explicit. No native build or dependency is needed.

Sent field coordination to root before coding: CutRequest retains result,
candidates, labels and admitted_indices; CutSamples retains result, request, rows,
timings, observed-validated backend string and requested device, with count and
evaluated_count. Named sample phases include patch snapshot/upload in addition
to gather/query/evaluate/download/convert/normal-scatter/cleanup and complete
sample wall, because this API owns one PatchSet per call. These clarify the
already selected contract without adding a reusable residency/session lane.

Test-first actual red, complete sampler green on CPU/P100, adversarial source
review, healthy late-failure retry and affected compatibility are proportionate
to the new Python ownership path. Source/geometry/history qualification remains
independent of status and CPU parity. Plan is ready for coordinated implementation
after all owner approvals and the actual-red gate.
