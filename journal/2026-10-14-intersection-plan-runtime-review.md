# Intersection consumer plan review

Reviewed the complete intersection plan, accepted design, current JetBatch and
resource wrappers. No blocking finding. Supplied concrete prepared-request,
row/batch, native interleaving and phase interfaces to the coordinator before
caller work. Explicitly retained span_id plus piece_id and original slots.

Implementation must compare model grades exactly using actual binary64 inputs,
validate complete descriptors and slot/UV bytes, and own BaseException-safe
child cleanup. The current generic child context manager remains unsuitable for
that primary-exception contract. These are implementation details of the
reviewed design, not new research or qualification gates.

Saved papers/2026-10-14-intersection-plan-runtime-review.md. No product/native,
fleet, benchmark, new dependency/agent or Git execution. Await coordinator's
accepted interface and genuine missing-method red before implementation.
