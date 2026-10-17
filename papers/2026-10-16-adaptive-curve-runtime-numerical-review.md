# Shared curve runtime: scoped numerical/cache review

Reviewed actual `native/mesher/adaptive_curves.cpp` numerical cache and fold paths
against the selected source/point/curve contract. Root separately reviews the
whole owner state machine and device lifetime. No additional geometry execution
was requested for this source review.

Point identity includes original source slot, canonical exact parameter limbs,
fixed-override presence and all fixed XYZ binary64 words. The cache retains the
source lease; equality is not a hash alone. Original/prepared root identities and
initial restrictions stay separate. Node tiers include actual endpoint point
identities and numerical-stage signature. Fresh quality queries restart BASE
stage even if tighter values remain cached, while original endpoint guards reset
per query. Device resident stages must match or the selected cached sample is
reinstalled; query history does not privately choose a different Q.

The exact allowance comparison uses the original Rational, while downward
binary64 tolerance is only a device filter. One concrete review finding was
fixed: a READY conservative upper just above exact tau did not enter the precise
same-task comparison when endpoint departure was small. Current source uses a
canonical degree/operation-count near-threshold band only to choose exact work,
limits ambiguities per horizon, and accepts equality only from the exact
comparison. Large failures still subdivide. A certified exact tau is retained as
a valid positional upper instead of re-rounding it above itself.

Cross-reading this route also exposed a geometry response omission: the precise
response did not retain the already-computed endpoint floor. Geometry now fills
numerical_upper, using the existing exponent-relative exact norm bound, so the
promotion path neither loses an actual floor nor invents an absolute 2^-192
floor for a true 2^-600 value. Geometry's direct line equality checks cover both
.125 and 2^-600. The sole physical owner receives sorted promotion rows with old
logical identity/revision and all incident faces rebuild across its transaction;
fixed original vertices remain fixed.

Scoped numerical/cache ACCEPT after these fixes. This does not independently
approve scheduler/device lifetime or claim a complete trimmed surface mesh.
