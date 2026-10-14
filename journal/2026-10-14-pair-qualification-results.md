# Qualified pair beachhead and serial measurements

Fleet host ASan/UBSan/leak checks passed60 native cases including 18 newpair
scenarios. CPU completed83 publictests (five CUDA-onlyskips) and13CPU24 pairtests
(one CUDA-onlyskip). Reviewed scope refinement stopped the GPU deep unchanged
Bernstein continuation; completed CPU checks were retained. Each P100 passed
the 68-test focused family suite under Compute Sanitizer with0 errors/0 leaks.
Eight actual old-artifact compatibility cases passed, and four isolated real
production mutants were caught by direct assertions. No unresolved review
finding remained.

Exported actual CPU24/P1000 four-body and P1001 sixteen-body callers. Independent
standalone original-control/frame/mask/serialization review checked15,024 bits,
314 survivors and1,624 controls with0violations. SVG crop/source identity reviewed.
Added source/control export without changing numerical work; final callers used
the resulting source. Protected discovery-release bytes remained unchanged.

Only after all qualification/build/copy/export work finished, ran eight serial
fresh-process performance cases, one excluded warmup and five reused calls.
All 304 rawobservations/112 summaries and17 sourcepins independently reviewed.
Large879,840-pair shortlist medians:CPU1 19.182 ms,CPU24 9.631 ms,P1000 7.776 ms,
P1001 6.920 ms;1,823 survivors. CPU1wins small; cold setup favors CPU. Host mask
conversion/decoding dominate cached GPU costs. Published full scopes/ranges,
CUDA payloads and shared-host limitations; no whole-CAD or intersection claim.
Preparing README/roadmap publication and commit-preserving PR18 merge.
