# CUDA mode-specialization clients: independent source review

Reviewed tests/test_discovery_modes.py and
scripts/check_discovery_mode_identity.py against actual adapter/fixture APIs,
accepted design and plan. Accepted with no blocker. Only source, AST and exact
input inspection were performed; no public test, native callback, fleet job or
benchmark was executed by this lane.

Reviewed source SHA256:

| File | SHA256 |
| --- | --- |
| tests/test_discovery_modes.py | b7e081c66a39309f6429394b8baa90bdd0ddaf033bf3cff46bb7bed5c233b740 |
| scripts/check_discovery_mode_identity.py | 10864127b5deb9bfb9dcbdbb0bc295c128250f9149fc952242f41b2201d97f31 |

## Focused public clients

All three modes exercise counts0/1/127/128/129/257 with full-count field strides.
Mixed invalid slot/NaN target uses BAD_PATCH precedence; valid slots with NaN,
infinity and just-outside envelope targets require BAD_TARGET. Every invalid
field is compared to its canonical NaN bits, and all work/flags must be zero.
Valid plane answers require GAP, analytic distance1/4 and the stated XYZ grade.
The finalized valid-row check requires exactly one coverage/visit-cap flag and
rejects arithmetic/unknown flags, resolving the small initial assertion omission.

Cap/root cases use the existing independently specified status/flag/work
expectations. Refinement and visit combinations check requested bounds and
two-child reservation fallback. Degree-zero rational1/3 exercises a nonzero
exact distance despite rounded descriptive coincidence. The mixed source-offset
batch includes nonuniform-weight degree(3,3) constant geometry. Exact Fraction
input inspection confirms its3/32 distance, plane1/4 and constant fixture truths.
The depth12 AABB varying-axis case forces complete maximum-depth traversal
without a degree/depth Cartesian campaign.

Profile-on/off slabs are compared as bytes, and profile metadata checks admitted
backend, empty NaN duration, finite nonnegative nonempty CUDA duration and exact
profile flags. Parent handles close before downloading retained children in
every mode. Calling-thread and worker-thread device checks run actual selected
backend calls; only explicitly CPU-selected device behavior skips. Requested
unavailable CUDA is an error. Nested resource scopes and closing the observation
generators release native ownership on successful iteration or assertion failure.
The new family stays on its existing public resource graph rather than exposing
device/native pointers as identity.

ABI1 plane jets and ABI2 seeded projection use actual existing method/field
names and analytically specified outputs. This checks translation-unit smoke
behavior without a new numerical reference. No silent numerical fallback,
timing assertion or production token-corruption hook is added.

## Capture and comparison

Capture uses fresh processes with explicit core/plugin selection, checks native
width/byte order and preserves actual downloaded data buffers, six discovery
integer slabs, deterministic info/source/options, slots/targets/seeds, original
homogeneous input bits and case labels. Existing corpus capture totals309 rows;
reordering mixed source slots changes control offsets while preserving their
recorded labels. It does not rerun a solver, traversal or cache oracle. The
unchanged standalone Fraction qualifier supplies geometry independently.

Native/core/benchmark SHA and client-source identities are recorded without
private paths. Actual selected binary hashes must remain unchanged during
capture. Comparison requires matching core/benchmark/client-source/device/input
metadata and indexed shapes, while plugin hash and human variant label may
deliberately differ. It compares bytes rather than hashes or float tuples, so
NaNs and signed zero are preserved. Only kernel duration is excluded from
operation identity; its empty/nonempty shape, profile flags and finite-grade
rules are still checked. The first differing data field/row or integer slab is
reported. Index loading checks duplicate records, extents, widths, payload
lengths, case consistency and complete duration evidence.

A proposed inherited-fields finding was dismissed after inspecting the actual
adapter declarations. The reviewer initially inferred that named_record's
direct _fields_ iteration omitted ABI fields from a base class. That inference
was incorrect: _Versioned, _ProjectionVersioned and _DiscoveryVersioned define
initializers, without fields. PatchDesc/ResultInfo
[declare their ABI fields directly](../python/brepgpu/__init__.py#L46), as do
ProjectionOptionsDesc/ProjectionInfo
[at the projection declarations](../python/brepgpu/__init__.py#L403) and the
hierarchy/discovery options/info
[at the discovery declarations](../python/brepgpu/__init__.py#L501).
named_record already includes version and size without copying padding. No MRO
machinery or implementation change is required.

## Disposition

The clients implement the reviewed compatibility and focused callback checks
with coherent analytic expectations. AST and exact mixed-source input checks
passed. Baseline dynamic execution, new-plugin identity, exact geometry and
sanitizer outcomes remain coordinator-owned future evidence. No performance,
hardware green result or geometric certificate follows from this source review.
