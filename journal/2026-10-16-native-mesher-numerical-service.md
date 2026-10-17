# Persistent numerical service

Parent approved `docs/native-mesher-numerical-service-unit.md` before code. The
source-backed fixture was authored first. Its initial fleet syntax compilation
failed because `native/mesher/numerical_service.hpp` did not exist: genuine
missing-API RED, before implementation.

The service owns one persistent BoundRuntime and bank across ordinary curve
batches and FaceCellWork requests. Preparation imports only into current worker
scratch and freezes actual-cell pieces. Cross-cell batches retain route ranges;
the narrow CellBoundFold range overload validates the complete selected range
before modifying its existing fold. Preparation table IDs use a distinct run
service owner with a monotone table ordinal, separate from face seed/engine IDs.

The first strict Release run passed CPU source-backed curve/C0/analytic work and
source identity, resource, cancellation checks. An additional reversed-key face
packet then produced a genuine semantic RED: the existing runtime requires
sorted task keys, while the face frontier's geometric priority order is not key
order. Preparation records now sort by parent key while retaining original
packet indices for result publication. The reversed packet remains the fixture.

Actual qualification now passes on the isolated fleet checkout:

- Strict Release CPU: service fixture 0.07s; affected original actual-cell
  fixture 0.11s. Curve zero/equality and failure, C0 multi-piece actual maps,
  analytic plane departure, original packet-order restoration, mixed-cell
  packing, complete headers and coordinator request identity are checked.
- Both actual P100s: the same source-backed service reuses one session across
  curve and cell requests and produces the same independent expected decisions.
  Scoped device memcheck reports zero errors.
- Host Debug ASan/UBSan with leak detection: full service fixture passes 3.23s.
- Source/geometry identity, duplicate/source-stale packet, configured capacity,
  private scratch exhaustion and cancellation fail with their expected types.
  Caller handle release and retained immutable outputs are checked; healthy
  service destruction returns RunResource live bytes to its prior baseline.
- A selected response containing a valid first answer and stale second answer
  is rejected atomically: the first answer remains available to a subsequent
  valid range. Empty/out-of-range and repeated ranges are rejected, and missing
  pieces prevent final publication.

Static analysis initially rejected unchecked optional-state assumptions. These
were replaced with explicit request/packet/ticket invariants; no suppression or
numerical policy change. Final production/fixture clang-tidy and formatting are
GREEN. Independent review closure remains pending. No complete-mesher
performance or watch claim follows from this unit.
