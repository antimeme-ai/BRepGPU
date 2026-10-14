# CUDA mode-specialization numerical plan review

Reviewed [implementation plan](../docs/CUDA_DISCOVERY_MODE_PLAN.md) against the
accepted design and current shared arithmetic/CUDA dispatch. Report:
[plan numerical review](../papers/2026-10-14-discovery-cuda-mode-specialization-plan-numerical-review.md).

Accepted with no blockers or required numerical corrections. Raw capture is an
execution-identity client, while the existing exact Fraction qualifier runs once
per new plugin/card; focused repeated tails use analytic assertions. Early mode
validation, same arithmetic/flags and complete raw output/metadata comparison
preserve semantics. The matched 28-case GPU-only matrix and native-phase threshold
with reverse-order confirmation on both cards provide a bounded acceptance rule.
Record the same confirming condition/phase on both cards when publishing; preserve
individual samples and spread. No product/fleet edits, tests/builds or timing jobs;
the coordinator's scalar queue remains undisturbed.
