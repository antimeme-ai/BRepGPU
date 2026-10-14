# CUDA mode-specialization numerical design review

Read active repository instructions and current shared doctrine. Reviewed the
proposal against actual shared directed interval arithmetic, scalar/projector,
CUDA kernel/output stride, retained mode, callback events and publication path.
Report: [numerical review](../papers/2026-10-14-discovery-cuda-mode-specialization-numerical-review.md).

No numerical/output/counter blocker found in three template-mode instances calling
unchanged force-inlined helpers. Reported two concrete clarifications: raw bits
must implement NaN/signed-zero output identity, and start-before-dispatch CUDA-event
duration can include host enqueue gaps. Existing independent exact geometry and
old/new raw identity address distinct claims without a fake functional red or
repeated CPU/mutation campaign. No product edits, tests, native compilation, target
operations, benchmark changes or timing claims were made.

Re-read the owner's integrated clarifications. Both are resolved: the design now
requires raw buffer-byte identity and explicitly allows submission/idle gaps in
the unchanged stream-event bracket. Appended corrected design hash and accepted
disposition to the report. No unresolved review blocker; implementation-plan
review remains the next phase.
