# Watch chain design review: actual caller and display

2026-10-15. Reviewed `docs/WATCH_CHAIN_DESIGN.md` against the existing
extrusion/mesh records and source-grounded visual study. No blocking finding.

The admitted cylinder and feature Frames are sufficient to build real pierced
watch-like parts. Cutter radius is world scale/4; with power-of-two Frames the
available radius ratios are discrete. Recipes should select those ratios
explicitly, rather than treating radius as an arbitrary art parameter. A
structural rim with feature scale2, outer profile radius approximately.72 and
cutter scale4 gives world outer radius approximately1.44 and inner radius1.
The height minimum gives a thick rim; a consciously separated sculptural
assembly avoids an untruthful flattened display transform.

An initial layout inside that rim can place a barrel-like wheel at(-.40,.35),
a smaller train wheel at(.35,.30), another at(.60,-.25), and a larger balance-like
assembly at(-.35,-.45). These are starting design coordinates, not guaranteed
mechanical engagement or interference clearance. Actual source recipes and
result counts will supersede them. Separate bridges, bearings and tooth-like
lugs can be owned convex extrusions. All remain separate assembly pieces.

The mesher's actual `.bore_result`, `.body`, `.history` and `.composed_history`
bindings provide everything the scene needs. The caller should walk the actual
origin/path records, retain source instances, export namespace/revision/type
information, and aggregate indexed triangles by part. Rendering consumes the
actual rounded array. Only camera projection/lighting modifies screen output.

A display cannot establish geometric correctness; it can expose the cut walls,
depth and empty regions after the chain's independent qualifier passes. The
dedicated display checks address wrong indices, unusable geometry and lost
parts. PNG output, count correspondence and attractive appearance must not be
described as a second geometry certificate.

API names, same-Z admission, exact containment, source UV ownership and timing
scopes are explicit enough to proceed to the implementation plan. No native
dependency, new ABI, GPU rendering, functioning mechanism or watchmaker's
caliber reproduction is selected.
