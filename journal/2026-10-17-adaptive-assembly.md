# Original cached-mesher assembly envelopes, 2026-10-17

Parent assigned one whole source-envelope/assembly unit after the read-only
component study. Saved docs/adaptive-assembly-unit.md and obtained root approval
before product implementation. Read actual source/control layout, original
analytic formulas, old envelope producer, exact assembly transport and pinned
OCCT pole-hull source. No dependencies, eager GeometryStore, old meshing callback
or source-specific dispatch were introduced.

Review removed a proposed unconditional ideal cone apex extension. Verified
against assembly.cpp::transport that kappa>=1, eta>=0 and local tau<=requested
epsilon. Accepted actual L lies within tau of original source x in B0, so L is
inside the existing B0+epsilon placement-error box. Intermediate ideal-map points
need no additional extent. Later cap/domain maps must bound the complete original
finite source→actual facet relationship inside the retained source profile.
Root and construction peer agreed; removed apex trig/flags/span from the unit.

Authored adaptive_assembly.hpp and direct native fixture first. Fleet missing
prepare_adaptive_assembly link RED is retained in ignored context. Implemented
the source-only producer: exact positive homogeneous control ratios, original
natural knot domains, literal analytic row bounds, V/Ce boxes, mapped raw guide
hulls, plane/cylinder inverse directions, shared-carrier UV union then outward
binary64 profile used for BOTH B0 and subsequent cache requests. Memoized source
curve domains as well as hulls to avoid repeated knot scans for shared curves.
Reused the existing exact AssemblyStore transform/allowance mechanism unchanged.

The external original-source Fraction oracle passes eight definition envelopes,
seven carrier profiles and eight placements. It includes nonorthogonal frames,
literal unequal circle/ellipse radii, a nonclamped rational surface and curve,
false saved span/UV hints, a shared carrier with distinct guide envelopes, mapped
descending t, an original vertex outside the carrier hull, nested once-rounded
scale/shear transforms, an unplaced definition and explicit unsupported lift/map3.
The native fixture attacks resource/key/item/work admission on the SAME producer.

Root actual-source review accepted the formulas/ownership with one concrete
finding: an exact finite guide ratio can exceed binary64, and the profile boundary
must report unresolved rather than a low-level conversion error. Added the huge
positive homogeneous guide case to the SAME external fixture; it reproduced RED
on the old body. Fixed by comparing exact endpoints with maxFinite before outward
conversion, checking converted finiteness and returning ErrorCode::unresolved.
No source bound is clipped. Scoped static findings were resolved with named axis
shape and explicit private optional-domain checks; no suppressions.

Final affected strict Release and ASan/UBSan/leaks pass the SAME authored oracle
after the representability finding. Named 100-column formatting and actual-source
clang-tidy also pass, without suppressions. Qualification stopped: one authored
operation, no GPU or imported movement run. Root actual-source review accepted
the unit after its finite representability correction; ready for the coherent
source-envelope checkpoint and component caller integration.
