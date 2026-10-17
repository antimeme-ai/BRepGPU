# Display the new mesher's stored geometry

The accepted runtime plan requires an explicit source-geometry display profile:
all corners use stored canonical XYZ, original source axes remain intact,
approximately rigid instance normals use the actual cofactor, and pole/apex
normals follow declared symbolic branch/sheet values. The existing fixed scene
record widths stay intact; header feature1 selects profile2 and its required
CPU/CUDA API2. Legacy API1 rejects new vertex flags before evaluation.

This is the display consumer of the whole native mesher, not a meshing fallback.
Direct authored binary inputs check stored off-surface points, independently
computed affine normal transport, nonorthogonal source axes, symbolic sphere
limits and malformed flag combinations. CPU and both P100s consume the same
records. Actual watch publication still requires the complete native mesh.

The direct binary/pixel/normal suite passes five authored methods on CPU and
each P100. The explicit API fixture passes CPU and both devices: API1 refuses
the source flags, API2 refuses legacy-profile flags, and successful API2 output
uses the retained physical words. CPU ASan/UBSan with leak detection passes the
five methods and direct API fixture. The affected legacy direct/failure suite
passes eighteen methods with its one expected skip. These finite checks cover
the changed display path; they establish no complete-watch mesh or render.

The independent actual-source review in
papers/2026-10-16-native-display-source-profile-review.md found an irrelevant
plane-Z admission requirement. A literal Z=0/huge-Z test failed before the fix;
admission now checks only X/Y for planes. Review accepts the corrected source
profile. Poorly scaled uncertain frames remain explicitly refused.
