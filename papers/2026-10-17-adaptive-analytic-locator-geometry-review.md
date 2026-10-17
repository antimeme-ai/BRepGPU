# Analytic locator source review

Scoped ACCEPT of docs/adaptive-analytic-locator-unit.md and the actual private
FaceLocator implementation in adaptive_traces.cpp. No numerical run belongs to
this review.

The exact original frame dual is converted once per face. Unrepresentable dual
coefficients and failed finite starts retain the original guide. Signed-family
cylinder/cone/sphere/torus seeds and physical original-frame D1 corrections are
construction proposals. Finite source-bank and guide-relative branch bounds
apply to starts and rounded trials. Bounded damping accepts only a decreasing
proposal residual; no libm result accepts a source node or source interval.

The candidate passes through the existing common original-source RN sample,
and can replace a valid guide only when its residual upper is below the guide
residual lower. The captured final source-node and whole-trace tasks remain the
acceptance mechanism. The extra host calls are explicitly proposal work.

One actual-source finding was fixed: locked U initially reconstructed a rounded
binary64 guide coordinate. The sealed candidate now copies exact guide[0] when
locked, before common sampling, preserving the source seam sector even when the
exact guide coordinate is not representable in binary64. The final source was
reread with this correction and the consistent rounded branch checks.
