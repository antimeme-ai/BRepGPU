# Physical curve-owner extension: actual-source review

2026-10-16. Scoped ACCEPT of the `boundary.hpp/.cpp` shared-curve extension and
`tests/native_adaptive_curve_forest.cpp`. Reviewed owner-only wire requests,
combined split/promotion staging, expected logical key and constructed revision,
original vertex exclusion, immutable revision export and optional exact-table
namespace.

All promotion rows are validated before sole-owner mutation. Split and promotion
requests share the full face horizon; wire requests need no fictitious face.
Failed shape/evaluation keeps installed Q unchanged and permits the documented
retry. Once a commit mutates owner state, resource failure poisons that owner
rather than exposing half a transaction. Previous snapshots remain immutable.
The source sample key is not replaced by constructed coordinate proximity.
Production sessions supply a unique table owner and retain the existing monotone
ordinal allocator. No additional execution requested.
