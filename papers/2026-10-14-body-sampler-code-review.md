# Body sampler: independent source review

Reviewed python/brepgpu/body_sampling.py against the accepted body design,
actual body records and existing ABI1 adapter. Read the example's boundary/
normal visualization for scope only. No native/public test, build or fleet
execution was performed. One cleanup finding requires disposition before final
acceptance; binding and geometry consumption otherwise fit the narrow contract.

## BS-C1: child cleanup can replace an active primary exception

sample() scopes query/result with the shared _Resource context managers. Their
__exit__ catches Exception rather than BaseException and adds cleanup notes
without a guard. If conversion/download/normal construction raises the primary
error and child.close raises KeyboardInterrupt or SystemExit, cleanup replaces
that primary error. A failure during add_note can also replace it. This conflicts
with the sampler plan's preserve-primary cleanup rule. BodySampler's own
__exit__ already guards BaseException/notes, but does not protect those child
scopes.

Fix inside the owned sampler operation rather than changing unchanged native
machinery: attempt both child closes, catch BaseException, retain the active
primary exception object and attach guarded cleanup notes; when there is no
primary, raise the first cleanup failure with secondary notes. Publish only
after cleanup succeeds. A direct failure test should use actual query/result
wrappers and verify exception identity plus both attempted closes. This is
resource-failure conformance, not a new geometry reference or audit framework.

## Source conclusions

SampleRequest copies candidate values, bounds iteration before oversize buffer
creation, derives exact trim labels/admitted indices once and retains the
snapshot. Grid selection validates unknown/duplicate faces and respects snapshot
face/chart ordering. BodySampler checks exact snapshot identity, not equal
counts/slots or a coordinate-based correspondence. Native input slots/UV bytes
and lineage are compared to gathered values, preserving signed zeros/source
identity.

First-jet shape uses the admitted count for its SoA stride. Native failures
retain original candidate index, face/chart/UV/status details and cannot become
trim rejection. Frame conversion occurs once. Normals use converted first
derivatives, corresponding face orientation and a finite positive norm check.
Uniform positive frames and unit source-chart widths preserve direction; local
quarter derivative-speed differences do not produce a global smooth-derivative
claim. Scatter writes the original row order and distinguishes invalid/rejected
NaN rows with native-status−1 from successful admitted rows.

Published rows contain tuple values and retain immutable body records rather
than native handles. Sampler close preserves retryable OOM ownership and treats
consumed handles as closed. Existing engine/resource binding prevents hidden
backend fallback. Host timing separates gathering, allocating/provenance
download, conversion and normal/scatter from native evaluation; internal reused
wall ends after child cleanup, with publication overhead stated separately.

The example draws actual samples and normals in a normalized body view and
CPU-owned canonical edges. It retains one cylinder face/four charts and does
not draw quadrant splits as body edges. Sample thinning is stated; gray curves
are reference boundaries, without mesh/chord/embedding claims. No connector
bridges rejected candidate rows. These source conclusions are not render or
hardware qualification results.

## Disposition

BS-C1 is the only concrete blocker found. Await its direct test and corrected
source; no change is requested to body geometry, native ABI or supported scope.

Focused re-review: BS-C1 is resolved. The sampler-owned _child_scope catches
BaseException from child cleanup, guards attribute/string/note attachment and
bare-rethrows the exact active primary. Its nesting attempts result and query
closes; without an active primary the first cleanup failure propagates while
the enclosing scope still closes its child. Successful batches publish only
after both scopes complete. Shared native/adapter files were not changed.

Coordinator reported actual fleet red: chosen ArithmeticError was replaced by
cleanup KeyboardInterrupt/SystemExit. Corrected direct green preserves the
same primary object, leaves both handles0 and permits a healthy retry. No new
campaign was requested or run by this reviewer. Final disposition: accepted,
no remaining sampler source blocker.
