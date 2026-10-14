# Intersection consumer/caller source review

2026-10-14. Reviewed sampler and root caller/campaign as one exact-result,
source-provenance, lifetime and graph-consumption invariant. Saved
[sampler review](../papers/2026-10-14-intersection-sampler-topology-review.md) and
[caller review](../papers/2026-10-14-intersection-caller-topology-review.md).

Accepted the stable sampler source: exact windows/caps, result binding,
side/original-slot remapping, actual source bytes, exact model grades, detached
immutable output and BaseException cleanup are coherent. Reported IC1 timer
boundary and IC2 missing imported helper source to root. Source graph/span,
SVG and byte-count semantics otherwise agree with the literal recipes.

No product imports, native/fleet/performance execution, tests, builds,
dependencies, agents or Git operations. Only these review/journal files written.

Re-read root's caller fixes: IC1 includes all non-timing data/quality/input/source/
binary exports before the whole wall ends; final timing and console writes are
explicitly excluded. IC2 includes the imported helper and body oracle hashes.
Accepted both corrected source units. Sampler reviewed SHA-256 is
`8691091fa8ad411713aa180bf1ad7000edc9301e4ec57cbb06c2092de87de316`.
