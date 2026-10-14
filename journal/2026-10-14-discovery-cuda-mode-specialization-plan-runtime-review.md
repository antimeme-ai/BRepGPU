# CUDA specialization implementation-plan review

Reviewed docs/CUDA_DISCOVERY_MODE_PLAN.md against the accepted corrected design
and current callback/runtime/benchmark sources. No blockers. The plan keeps
the mode template and selection minimal; invalid private modes are rejected
before construction and the empty guard, and defensive switch failure must
follow normal event/stream/Pending/device cleanup. Existing empty/profile rules,
all-mode byte identity and actual device restoration are explicit.

Preserved dynamic plugin/core/benchmark staging, pre-delta baseline clients,
one unchanged exact qualifier per new plugin/card, focused sanitizer/ABI smoke
and28-condition GPU comparisons are coherent. Core/benchmark identity stays
equal; changed plugin identity is recorded provenance rather than equal-output
metadata. Native scopes and measured task-value acceptance remain separate
from resource counts or event-only improvement.

Accepted disposition saved at
papers/2026-10-14-discovery-cuda-mode-specialization-plan-runtime-review.md.
No product edits, tests/builds/copies, target execution or Git operations were
performed. The scalar fleet queue remains owned by the coordinator.
