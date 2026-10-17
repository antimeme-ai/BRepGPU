# CDT bridge lifetime, 2026-10-17

Plan: docs/adaptive-region-cdt-lifetime-plan.md. Root and numerical peer accepted
before body; numerical actual-source review accepted after body. No dependency,
core storage/layout/topology, numerical predicate, metric, seed or cap change.

The actual v6 original-component failure requested3,670,016 bytes at alignment4
in DivideConquer::edge while Budget live64,301,215/limit67,108,864; peak65,202,847,
max integer width28 limbs. This matches the131,072-record Quad buffer allocation
with the65,536-record old buffer still live. Erased Quad slots already recycle.
No core/frame dump remains; actual point count, failed facet and refinement cause
were not captured. Do not infer them from vector capacity or claim giant integers.
The32GiB run cap was not exhausted: the failing owner was64MiB child scratch.
Raw private log: context/adaptive-watch/lazy-xyz-actual-1755-p1000.log.

Source inspection proved Predicates owns decoded coordinates, weights and lifts;
its constructor retains no encoding pointers. The bridge now ends admission-index
and sorting lifetimes before encoding, releases transformed coordinates and
serialized limbs immediately after predicate construction, releases predicates
and mappings after solve/remap, and starts adjacency only after those phases die.
All allocations and returned vectors retain the same Budget owner. See plan for
Triangle1.6 pool and CGALv6.1 record comparisons and phase working-set expression.

Genuine RED: preselected128×128 integer interior grid+4 protected square corners,
N16388, factor(2,1;0,1), seed0, unchanged16MiB Budget; strict fixture linked against
old bridge and returned1 `native memory budget exhausted`. Logs:
context/adaptive-watch/cdt-lifetime-red-build.log and cdt-lifetime-red.log.
No capacity search or workload escalation was used.

GREEN: same operation returns32770 triangles, peak16,647,712 bytes,
work39,974,286. SAME Python Fraction positive-chain/atomic-edge oracle PASS1;
normal return and an intentionally tiny-cap refusal restore caller live baseline.
The operation still charges exact arithmetic and vector growth; this is not a
universal promise that every production graph fits64MiB. Private log:
context/adaptive-watch/cdt-lifetime-green.log.

Strict compilation first caught the positive-cycle scalar scoped too narrowly;
its lifetime was corrected before a run. Existing real-face region CTest then
exposed a stale fixture SurfaceFaceIdentity with hardcoded face_generation1;
fixture now passes effective_face_generation(key), preserving production guards.
Final native_adaptive_regions CTest PASS0.23s, unchanged nested/disconnected/
rational/source-backed packet+band paths and SAME oracle. Clang18 formatting
(100columns) preceded final build; clang-tidy18 product PASS,25 suppressed
non-user-code warnings. Logs: cdt-lifetime-final-build.log,
cdt-lifetime-existing-oracle.log, cdt-lifetime-tidy.log in same private directory.
No additional cohort. Root owns the next single actual remaining-component run;
no claim of original1755 completion or performance improvement is made here.

Stable fleet tree: build/adaptive-cap-construction-release. Product archive and
fixture are settled and tree handed to root for model relink. SHA256:

- adaptive_region_cdt.cpp:43612288d05069703712a4b6d271aefb2c6055a23fccf5978dc527648ce18414
- native_adaptive_regions.cpp:d7be85c11e5191d78d3ed1e25ae105f1de0da2d4e8f4d12354ee8a59015b0c91
- native_adaptive_regions_oracle.py:0816b87e1d31d446e2f1086c7b5c5e7ceade0334967a5128ca2ffc5c9a53e55e
- libbrepgpu_mesher_adaptive_regions.a:234aaf0cd4ff4953b791e1e175f79267bada1fe51232b865aec214532e89d6f5
- brepgpu_native_adaptive_regions:55d9000f3a3bce6eee203c7cd39bb41b1e6de9a3b65ad5273a641cade07c8ccf
