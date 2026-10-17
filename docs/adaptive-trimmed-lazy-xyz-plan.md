# Deferred exact virtual XYZ

2026-10-17. Narrow implementation PLAN; review before body.

The sampled v5 fallback caller is the virtual SAME-L dot product: each positive
clip triangle eagerly seals9 exact XYZ values using27 Rational multiply/adds.
Clipping already has exact UV and barycentric triples relative to the actual
parent. Retain that expression and defer its exact XYZ evaluation. No geometry,
triangulation, cap, source bound, physical Q, tolerance, kernel or device ABI
changes. Pinned v5 runs continue; no new watch/performance campaign.

Grounding: the existing exact clipping/SAME-L contract is in
docs/adaptive-trimmed-numerics-unit.md178–214 and the existing bound kernels.
CGAL6.2.1 [Lazy_exact_nt definition/approx/exact](https://doc.cgal.org/latest/Number_types/classCGAL_1_1Lazy__exact__nt.html)
explicitly describes interval approximation with exact evaluation on demand
(read2026-10-17). We use that scheduling pattern for ONE fixed linear expression,
not its expression-DAG machinery or a library adoption. Existing
adaptive_numeric.hpp::add/mul and Rational::bounds provide outward containment;
this is the SAME mathematical L used by the current source/cached proof.

Own adaptive_trimmed_geometry.hpp/.cpp, existing native_adaptive_trimmed_work.cpp
and focused journal. No SurfaceCache phase, shared wire, CUDA or CMake change.
Keep TrimPieceGeometry::whole_parent unchanged. Replace clipped piece's9-entry
affine_xyz range with a9-entry barycentric range; original_uv remains6 entries.
Keep15 sealed scalars/piece and existing exact-entry/work caps. No permanent
exact XYZ, mutable numerical cache, second authority or extra point registry.

1. Existing minted work fixture first rejects the old eager-XYZ payload when
   checking the selected barycentric representation. Observe real old-operation
   RED, then replace piece sealing with exact lambda0/1/2 for each virtual corner.
   UV, clipping, orientation, piece/parent ordering, limits and fold stay intact.
2. One common interval helper resolves parent as
   work.facets()[piece.parent].construction_facet, then reads captured actual
   parent Q bits. This is essential after fallback merge rewrites piece.parent.
   For each virtual corner and axis form, in canonical parent-corner order,
   sum_j numeric::mul(exact_interval(lambda_j),numeric::point(Q_j)). All adds/muls
   round outward. Do not normalize/clamp rounded lambda, replace Q, source-sample
   a virtual point, or claim the interval midpoint is actual L. whole_parent
   reads direct point(Q). Both base/source ordinary packers consume this helper;
   source normalization/UV arithmetic and kernel inputs remain unchanged.
3. The precise piece route alone decodes exact lambdas and forms exact
   sum_j lambda_j*Rational::from_double(Q_j). Both cached_bilinear and
   original_source precise methods use it. Its temporary values remain in the
   requesting JobContext Budget; captured work remains immutable. Unrepresentable
   or nonfinite interval-dot output throws typed unresolved at the common pack
   helper; it cannot reach current kernel admission as a valid task and does not
   get automatic precise recovery. Finite nonaccept keeps existing routing.
   It never implies source invalidity, acceptance or a source floor. Floor/refinement consumers need only
   UV/leaf/parent and remain unchanged.
4. Update the SAME existing fixture export to request/compute exact SAME-L only
   for its direct oracle output. Its Python Fraction reader remains the one
   independent whole-piece oracle. Add ordinary packed interval endpoints to
   that same output and check they contain its independently reconstructed
   Fraction L at EACH virtual vertex, for base and source packs. Include a
   captured source with signed physical coordinates/cancellation in this bounded
   fixture path; no authored fake view. Exercise one existing precise piece
   method and ensure its source/parent/UV/Q capture is unchanged. Do not expect
   identical upper bounds: intervals can be wider than a fully exact dot.

Checks: strict fleet-only build and the single existing native_adaptive_trimmed_work
original-source oracle; source review by root and runtime/numerical peer. No
second certifier, sanitizer/numerical cohort or full-leaf prefilter. Announce
coherent hpp/cpp/fixture uploads; root may relink model only after stable handoff.
