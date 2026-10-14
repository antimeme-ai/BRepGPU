# Pair discovery design runtime review

Reviewed the whole native/CUDA/Python pair design against current owned source.
Found PR-R1: actual resident index download had no private table callback.
Sent the concrete omission; root added the sixth callback and 72-byte table.
Re-read and accepted, with public ABI info40 unchanged and its ten-u32 field
count checked. No unresolved blocker. Checked bit-order/empty/tail/cursor flags,
same-context admission, retained objects, publication cleanup, lazy capability
handling, CUDA stream/device restoration and BaseException-safe host consumer.
Saved papers/2026-10-14-pair-design-runtime-review.md. No implementation, native
or fleet execution, dependencies, new agents or Git mutations.
