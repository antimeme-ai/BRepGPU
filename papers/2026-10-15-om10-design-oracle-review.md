# Independent review: complete OM10 implementation design

2026-10-15. Read `docs/OM10_IMPLEMENTATION_DESIGN.md` and the native display
study's complete byte layouts, source formulas, ownership and timing rules.
Reviewed against the original source inventory and independent-oracle study.

The design has the right complete ownership: isolated foreign source extraction,
owned arbitrary-loop trimmed meshing, all definitions/faces/occurrences, actual
useful native source evaluation, native CPU baseline, deterministic raster
fixtures, and source-linked labels. No product triangles come from OCCT and
no silent fallback or face omission is permitted. No expansion into alpha or
general functional watch claims is made. Exact byte arithmetic independently
checks the80-byte scene header,56-byte vertex,192-byte carrier and192-byte view.

Two findings must be made concrete in the accepted plan:

1. **Pre-render actual native geometry qualification needs an executable mode.**
   The whole source's actual CPU/P100 source positions/normals must qualify
   before the first OM10 product render. A zero-frame request which still
   evaluates/dumps source jets, or an explicit evaluate-only mode, achieves that
   without a dummy product image. It must exercise the same consumer used by
   rendering, not a second validation evaluator.
2. **Permitted whole-image edge/depth differences need a stated diagnostic.**
   Fixed snapped-input raster coverage is exact, but analytic libm results can
   change snapped vertices and very close FP64 depth winners across backends.
   A vague edge-band exception would allow unexplained visibility defects.
   Retain per-backend projected/snapped data and attribute differences to
   measured source/projection uncertainty or depth ambiguity; unexplained
   differences fail. Tiny exact specification fixtures still require expected
   winning IDs. No universal bit-identical image promise is needed.

Coordinated both with the native owner and parent. Smooth per-face source
normals are accepted and the study wording was corrected from per-facet. This
is useful geometry consumption; known rotated-normal/interpolation fixtures
check the independent behavior. The separate source/mesh lane retains spatial
edge→face residual budgets and physical singularity quotients as already agreed.

Disposition: accept the overall design subject to the two concrete plan/interface
resolutions above. No production or test implementation was started.

Subsequent source-schema cross-review identified concrete fields to reconcile:
source styles/materials/occurrence overrides were promised but absent from the
exact records; physical-edge parameter direction must retain EDGE_CURVE sense
or explicitly orient its range; SameParameter=false needs a real edge-parameter
to pcurve-parameter mapping; and the interior MeshVertex face/UV alternative
needs named fields. Sent these to the source owner before implementation.
The88-byte source header arithmetic independently checks. Parent/native owner
accepted zero-frame evaluate-only and projected-data diagnostics for the two
display findings; exact final output filenames/headers remain in the native
owner's single lane subplan.

Reviewed `docs/OM10_IMPLEMENTATION_PLAN.md`: accepted. It obtains actual source
extraction early, broadens fixtures from observed families without falsely
claiming they all predated implementation, qualifies complete source/mesh and
actual native jets before product rendering, compares a strong CPU baseline,
and stops after direct obligations. Source schema findings remain assigned
integration corrections before dependent code; they do not delay a focused
missing-API/native executable red.
