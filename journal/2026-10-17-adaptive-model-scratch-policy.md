# Explicit model scratch policy, 2026-10-17

Plan docs/adaptive-model-scratch-policy-plan.md accepted by root and numerical
peer before body. Caller only: adaptive_model_main.cpp and focused existing
native_adaptive_model.py policy port. No geometry/core/math/source tolerance,
source bank, cap domain, memory backend or work limit changed.

Actual v8 remaining original component still exhausted64MiB after932.83wall/
1205.33user seconds. Same3,670,016-byte Quad growth, live64,918,076,
peak65,770,300, work367,355,660, max28limbs. The previous lifetime unit remains
correct but did not prove production fit. No additional isolated long replay.

Added optional --scratch-mib; absent preserves old effective min(memory_mib,64),
including previously legal small global limits. Explicit value is nonzero,
representable before shift20, <=memory_mib, and sufficient global allowance for
mandatory prepaid scheduler width. The outer scheduler reserves threads*S;
physical curves reserve another threads*S when threads>1. Division-based parser
admission avoids multiplication overflow. All3 child settings use the same value.
Metadata records globalMiB, effective scratchMiB/bytes, explicit/default policy;
help names the option. There is no hidden default increase or explicit clamp.

Actual backing read: ResourceReservation prepays RunResource.acquire; per-worker
Budget uses the reservation. Nested FaceTrim/Surface budgets instead use SAME
RunResource directly, so their live allocations also count.12×256MiB is3GiB
externally prepaid and3GiB for curve workers, plus live nested/source/cache/output
allocation under unchanged32GiB. This is a selected bounded configuration, not a
promise that every input/concurrency fits256MiB or32GiB.

True existing CLI RED: same authored model operation with --scratch-mib256
returned1 `error options 0 unknown option` before body. Private log:
context/adaptive-watch/scratch-policy-red.log.

Strict model target GREEN. Existing two-definition/12-face source CPU1 mesh-only
runs at implicit64 and explicit256 both pass SAME original-source component,
chain/facet/placement metadata reader. All emitted mesh and captured-boundary
bytes agree across the two policies. Metadata exactly records64/67108864/default
versus256/268435456/explicit. Parser zero, above-global and impossible2poolwidth
refusals leave no final/temp output; explicit overflow1<<44 was added on root
source review and checked separately with the same CLI/no-artifact assertion.
Valid1MiBglobal/1MiBscratch options reach service.construct then refuse code2
`run memory reservation exhausted`, proving backing remains active. No image,
GPU numerical test, performance cohort or extra geometry checker.

Private logs context/adaptive-watch/scratch-policy-{build,green,overflow,tidy}.log;
outputs scratch-policy-ports. Clang18/100column format precedes final strict build.
clang-tidy18 PASS,58 suppressed non-user warnings. Root/peer actual-source ACCEPT.
Tree handed free to root for full CPU/P100 runs; no whole-watch success claim.

Stable build/adaptive-cap-construction-release SHA256:
- adaptive_model_main.cpp:704162aaa5616476d01e948cd08c3f5492b8494894d52c77df60a8b629440dd9
- native_adaptive_model.py:1e02d5f1f17c749aa47bd74970ae20bd5f09d0b4fc657f07aafd6cf4a2ea7e67
- brepgpu-adaptive-model:f460f284c15c3d47fb99f500289b3fdbfd543f3d59c3dddd37f7e70f63a95a6e
