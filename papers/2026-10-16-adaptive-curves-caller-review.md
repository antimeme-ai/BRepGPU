# Shared-boundary caller and independent reader review

2026-10-16. Scoped ACCEPT after rereading fixes in
`native/mesher/adaptive_curves_main.cpp` and
`tests/native_adaptive_curves_caller.py`.

Initial actual-source findings were concrete: cache requests used a descending
literal physical edge range despite cache admission requiring increasing bounds;
global edge/coedge incidence could pull guide uses from another definition;
unknown map kinds could be used to guess a guide domain; the independent reader
parsed but did not verify line-map payloads, original use traversal or full
plane selected-trace/chord error.

The caller now normalizes only the cache request range, preserving literal source
edge and use traversal. It filters guides by the owning face's definition and
refuses unimplemented maps explicitly. The same independent reader checks line
covector/scalar/parameter/traversal payloads from original curve values, exact
successive physical/use incidence, and endpoint errors for the whole affine
plane selected trace against the SAME actual chord. Rational and polynomial
physical chord checks use exact Fraction source coefficients and Bernstein
residuals; source node checks use ORIGINAL Ce versus chosen UV on the authored
plane. The finite line-image case remains independent of equal-parameter speed.

Wire output contains all physical and occurrence rows rather than presenting an
untrimmed patch as a complete BRep. No-replace output publication and stage timing
reuse the existing caller mechanism. New root-owned seam/refusal/integration
runs are outside this source-only review; no duplicate campaign was requested.
