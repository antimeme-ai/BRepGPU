# V1 oracle-geometry Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task (one unit per agent, at most three units at once, one independent review of the unit before it lands). Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the offline oracles' own view of source geometry, independent of the producer: evaluators for every carrier and curve kind, distance queries that return sound lower and upper bounds, the trim-domain builder of design §24.1, and a trimmed-face distance query. Oracles 1, 2 and 10 (units V3, V4, V2) build on it.

**Architecture:** A new standalone CMake project `tests/oracles` (namespace `bgor`) with one library, `brepgpu_oracle_geometry`, that links only `native/source_store` (input decoding) and nothing from `native/fast_mesher`. Every distance is computed by branch and bound over parameter boxes, with closed forms where a kind has them; every enclosure is inflated by a stated forward-error allowance; a query that cannot close its gap within its budget says so (`budget`), never guesses. The trim domain is a UV polyline per loop with a sound width per segment; points within the width are decided in 3D by distance to the boundary curves.

**Tech Stack:** C++20, pinned Clang 18.1.3 with libstdc++ 13, CMake 3.28 + Ninja, CTest; Python 3 fixture generators through `python/brepgpu/imported_source.py` (store version 3, from P1).

**Spec:** `docs/FAST_MESHER_DESIGN.md` §24 (independence, outcomes, "the oracle's trim domain"), §24.1 rows 1, 2, 10, §3.2 (distances are to trimmed faces, never to untrimmed carriers), §10.5 (seams, poles, quotients), §11.1 (the foot-point bound used here for the trim width); master plan `docs/fast-mesher-campaign/M7-plan.md` (calls 4, 5, 10; unit V1).

**Grounding:**
- Analytic parameterisations: OCCT 8.0.1 (`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`) `ElSLib` (`src/FoundationClasses/TKMath/ElSLib/ElSLib.cxx`, `PlaneValue`, `CylinderValue`, `ConeValue`, `SphereValue`, `TorusValue`) and `ElCLib`, because the adapter writes OCCT's frames and radii (`native/step_source/main.cpp:304-322`). Read as the definition of the parameterisation, not linked.
- B-splines: de Boor's algorithm on homogeneous points (Piegl and Tiller, *The NURBS Book*, 2nd ed., A2.2 and A4.1) for evaluation; Boehm knot insertion (A5.1) for Bézier extraction; de Casteljau subdivision with the convex-hull property for enclosures. The producer (P4, P5) evaluates on extracted Bézier spans; the oracle evaluates the full knot vector, so the two share no method.
- Forward error: Farouki and Rajan (1987), "On the numerical condition of polynomials in Bernstein form", CAGD 4(3): de Casteljau's error grows linearly in the degree; the allowance 2⁻⁴⁰·M covers degree 25 in binary64 with margin (master plan call 5).
- Projection: Ma and Hewitt (2003), CAGD 20(2), candidate elimination by control-polygon bounds, then Newton refinement; Hu and Wallner (2005), CAGD 22(3), for the local iteration. Only the elimination-plus-refinement structure is taken; the global bound comes from the enclosures.
- Fixtures: the exact projection fixtures A–F of `papers/2026-10-13-projection-oracle-study.md` (lines 62-196).
- Robust orientation: Shewchuk (1997), "Adaptive precision floating-point arithmetic and fast robust geometric predicates", DCG 18, two-sum and two-product expansions, written here independently of `native/direct_cdt/predicates.cpp`.
- Trim width: the lens bound of design §11.1 (Pav and Walkington, Lemma 1), with κ̄_uv = (σ̄²·κ̄_C + M_S)/σ̲ and ℓ̄ = ℓ̄₃/σ̲, computed from the oracle's own region bounds.

## Global Constraints

- Toolchain:
  - C++20 on the pinned Clang 18.1.3 with libstdc++;
  - CUDA 12.8, `sm_60`, SASS only;
  - new code stays C++26-lane-clean, and device-visible headers stay C++20.
- **No new libraries.** Any need for one becomes a proposal for the operator.
  Pinned OCCT 8.0.1 is the existing bridge.
- **No JSON for internal data:** use the tagged-table binary container. TSV and
  JSON appear only at presentation, on request.
- **Fleet only:** every build, test, render and benchmark runs on the fleet; the
  laptop edits and reads git. Every GPU test runs on both GPUs. Transfer figures
  are never compared across GPU0 (x8) and GPU1 (x16).
- **Original bytes stay private:** OM10 and corpus bytes are never copied into
  tests. Fixtures are small and authored.
- **No overfitting:** nothing keyed to OM10 IDs, names, sizes or counts.
- **Bounded review:** each unit gets one independent review, its findings fixed,
  then it lands. Edge cases go to beads.
- **Bugs:** fix every bug in the phase it is found, and file a bead.
- Mutation testing waits until the verification stack settles.
- **Privacy:** no hostnames, addresses, absolute paths or personal names in
  tracked files. Commits end with the Co-Authored-By line used in this
  repository.

Unit-specific, from the master plan:
- `tests/oracles` includes and links nothing from `native/fast_mesher`, `native/mesher`, `native/direct_cdt`, `native/face_domain`, `native/projection_solver.hpp` or `native/evaluator*`. Its CMake adds only `native/source_store` (through `add_subdirectory(../../native/source_store source_store)`).
- The oracle never reads a producer UV, a reconstructed pcurve (`pcurve_presence` 2), a target, a threshold or a budget from producer output. It uses a coedge's pcurve only when `pcurve_presence` is 1 (file) and `master` is 2 (this coedge's pcurve is the declared master).
- Every distance result carries `lower ≤ true ≤ upper` after the allowance; an unresolved query is `budget`, never a guess.

## Review Focus

1. **A target whose nearest point is on a patch boundary, or two equally near points.** A person expects the global distance, not a local one and not a seed-dependent one. Test: fixtures B (clamping is wrong), D (the farther endpoint is a local minimum only) and E (two equal minima) in `projection_fixtures_a_to_f`, Task 4.
2. **A point near a pole of a sphere, or the apex of a cone, where the chart is singular.** Expected: a finite distance with closed bounds, the trim width marked unbounded only on the segments touching the pole, and classification there decided in 3D. Tests: `sphere_cap_with_pole_loop` (Task 5), `distance_at_singular_chart` (Task 4).
3. **A face whose loop crosses the seam of a periodic carrier.** Expected: one closed polyline lifted across the period, the same membership for a point and its image one period away. Test: `cylinder_band_with_seam` and `periodic_image_same_membership`, Task 5.
4. **A coedge with a reconstructed pcurve and no declared master.** Expected: the oracle ignores the pcurve and projects the 3D curve. Test: `reconstructed_pcurve_ignored`, Task 5.
5. **A point exactly on a hole's boundary, or within the width of it.** Expected: `near` from `classify`, and `distance_to_face` equal to the 3D distance to the boundary curve within its bounds. Test: `on_hole_boundary_is_near_and_measured_in_3d`, Task 6.

---

### Task 1: Project scaffold, oracle types and evaluators

**Files:**
- Create: `tests/oracles/CMakeLists.txt`, `tests/oracles/geometry/types.hpp`, `tests/oracles/geometry/eval.hpp`, `tests/oracles/geometry/eval.cpp`
- Test: `tests/native_oracle_geometry_eval.cpp`
- Modify: `scripts/check_native_face_domain.py` (add `--unit oracles` with source directory `tests/oracles`; the driver's `ROOT/'native'/module` mapping becomes a unit-to-directory table so a unit may live outside `native/`; lint globs `tests/native_oracle_*.cpp`)

**Interfaces:**
- Consumes: nothing.
- Produces:
  - `namespace bgor { using Vec2 = std::array<double, 2>; using Vec3 = std::array<double, 3>; struct UvBox { Vec2 lo, hi; }; struct Bound { double lower, upper; }; enum class Outcome : uint8_t { resolved, budget, unsupported, nonfinite }; }`
  - `enum class CarrierKind : uint8_t { plane, cylinder, cone, sphere, torus, bspline, revolution, extrusion };`
  - `enum class CurveKind : uint8_t { line, circle, ellipse, hyperbola, parabola, bspline };`
  - `struct OracleCurve { uint8_t dimension; CurveKind kind; std::array<double, 12> frame; Vec2 radii, range; uint32_t degree; std::vector<double> knots /* expanded */; std::vector<double> controls /* homogeneous, dimension + 1 per point */; };`
  - `struct OracleCarrier { CarrierKind kind; std::array<double, 12> frame; Vec2 radii; std::array<uint32_t, 2> degree, shape; std::vector<double> knots_u, knots_v; std::vector<double> controls; uint32_t flags; std::optional<OracleCurve> profile; Vec3 sweep; UvBox domain; std::array<bool, 2> periodic; };`
  - `struct Jet { Vec3 p, du, dv, duu, duv, dvv; };`
  - `Jet evaluate(const OracleCarrier&, Vec2 uv);` `std::array<Vec3, 3> evaluate(const OracleCurve&, double t);` (point, first, second derivative; dimension 2 curves return z = 0)
  - `double magnitude_bound(const OracleCarrier&);` (the M of the allowance: the largest absolute coordinate of the frame origin plus radii, or of the Cartesian controls)
- Parameterisations follow `ElSLib`: plane (u X + v Y), cylinder (R(cos u X + sin u Y) + v Z), cone ((R + v sin α)(cos u X + sin u Y) + v cos α Z), sphere (R cos v (cos u X + sin u Y) + R sin v Z), torus ((R + r cos v)(cos u X + sin u Y) + r sin v Z); revolution S(u, v) = C + Rot_Z(u)(λ(v) − C), angle-linear u; extrusion S(u, v) = λ(u) + v·V with V the raw vector (design §5.4).

- [ ] **Step 1: Write the failing tests**

```cpp
// tests/native_oracle_geometry_eval.cpp
void analytic_points() {
  const auto cyl = cylinder(identity_frame(), 2.);                 // R = 2
  require(near(evaluate(cyl, {std::numbers::pi / 2, 3.}).p, {0., 2., 3.}, 0x1p-50), "cylinder");
  const auto sph = sphere(identity_frame(), 1.);
  require(near(evaluate(sph, {0., std::numbers::pi / 2}).p, {0., 0., 1.}, 0x1p-50), "sphere pole");
  const auto tor = torus(identity_frame(), 3., 1.);
  require(near(evaluate(tor, {0., std::numbers::pi}).p, {2., 0., 0.}, 0x1p-50), "torus inner equator");
  const auto cone = cone_carrier(identity_frame(), 1., std::numbers::pi / 4);  // R = 1, α = 45°
  require(near(evaluate(cone, {0., -std::numbers::sqrt2}).p, {0., 0., -1.}, 0x1p-50), "cone apex");
}
void rational_quarter_circle_on_circle() {
  // degree 2, controls (1,0), (1,1), (0,1) with weights 1, √2/2, 1: every t in [0,1] at 1/64 steps has |P| = 1 within 0x1p-50
}
void bspline_matches_bernstein_at_dyadics() {
  // a clamped cubic with one interior knot; at t = k/16 the de Boor value equals the hand-expanded
  // Bernstein value of the containing span (expansion written in the test from the knot vector)
}
void derivatives_by_closed_form();   // cylinder du, dv, duu at authored uv equal the closed forms exactly
void revolution_and_extrusion();     // a line profile revolved about Z gives a cylinder point for point (0x1p-48);
                                     // an extrusion of a circle by V = (0, 0, 2.5) at v = 1 lies 2.5 above the circle
void hyperbola_parabola_points();    // x = a cosh t, y = b sinh t; parabola x = t²/(4f), y = t
```

- [ ] **Step 2: Run them to make sure they fail**

Run (fleet lane `lanes/V1/`): `python3 scripts/check_native_face_domain.py --unit oracles --profile debug`
Expected: build fails: `geometry/eval.hpp` not found.

- [ ] **Step 3: Implement `bgor::evaluate` in `tests/oracles/geometry/eval.cpp`**

Analytic kinds in closed form; B-splines by de Boor on homogeneous points with the quotient rule for derivatives; swept kinds through the profile curve's evaluator.

- [ ] **Step 4: Run them to make sure they pass**

Run: `python3 scripts/check_native_face_domain.py --unit oracles --profile debug --lint`
Expected: `100% tests passed`.

- [ ] **Step 5: Commit**

```bash
git add tests/oracles tests/native_oracle_geometry_eval.cpp scripts/check_native_face_domain.py
git commit -m "V1: oracle project, types and independent evaluators

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 2: Exact orientation, Bézier pieces and enclosures with the error allowance

**Files:**
- Create: `tests/oracles/geometry/expansion.hpp`, `tests/oracles/geometry/bezier.hpp`, `tests/oracles/geometry/bezier.cpp`, `tests/oracles/geometry/enclosure.hpp`, `tests/oracles/geometry/enclosure.cpp`
- Test: `tests/native_oracle_geometry_enclosure.cpp`

**Interfaces:**
- Consumes: Task 1 types.
- Produces:
  - `int orient2d_sign(Vec2 a, Vec2 b, Vec2 c);` (exact sign by expansions)
  - `struct BezierPatch { std::array<uint32_t, 2> degree; std::vector<double> controls; UvBox domain; };` `std::vector<BezierPatch> extract(const OracleCarrier&);` (Boehm insertion to full multiplicity), `std::array<BezierPatch, 4> split(const BezierPatch&);` (de Casteljau at the midpoints); the same for curves (`BezierCurve`, `extract`, `split`)
  - `struct Box3 { Vec3 lo, hi; };` `Box3 enclose(const OracleCarrier&, const UvBox&);` (analytic kinds: closed-form arc and generator bounds over the box; B-splines: the box of the projected controls of the pieces meeting the box) and `Box3 enclose(const OracleCurve&, Vec2 t_range);` — each inflated by `allowance(M) = 0x1p-40 * M`
  - `double lower_distance(Vec3 p, const Box3&);`

- [ ] **Step 1: Write the failing tests**

```cpp
void orient_exact();                   // a, b, c collinear in exact arithmetic but rounding-adversarial
                                       // (Shewchuk's grid-perturbation family): sign 0; perturbed by one ulp: ±1
void extraction_reproduces_surface();  // for each extracted patch, its de Casteljau value at 9×9 points equals the
                                       // de Boor value of Task 1 within 2·allowance
void enclosures_contain_samples();     // property: for every kind, 200 random UV boxes (seeded), 1,024 samples each:
                                       // every sample lies in enclose(carrier, box)
void enclosures_shrink_under_split();  // the box of each split child lies inside the parent's box
void allowance_is_positive_and_scaled(); // allowance(M) == 0x1p-40 * M; M from magnitude_bound
```

- [ ] **Step 2: Run them to make sure they fail**

Run: `python3 scripts/check_native_face_domain.py --unit oracles --profile debug`
Expected: build fails: `geometry/enclosure.hpp` not found.

- [ ] **Step 3: Implement `orient2d_sign`, `extract`, `split`, `enclose` and `lower_distance`**

Analytic enclosures: a cylinder or cone patch over `[u0,u1] × [v0,v1]` is enclosed by the boxes of its two boundary circles' arcs over `[u0,u1]` (arc box: endpoints plus every axis extreme the angle range contains) joined; the sphere and torus likewise per parameter line, plus the radial extreme inside the box.

- [ ] **Step 4: Run them to make sure they pass**

Run: `python3 scripts/check_native_face_domain.py --unit oracles --profile debug --lint`, then `--profile asan`.
Expected: `100% tests passed`.

- [ ] **Step 5: Commit**

```bash
git add tests/oracles/geometry tests/native_oracle_geometry_enclosure.cpp
git commit -m "V1: exact orientation, Bezier pieces and inflated enclosures

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 3: Region bounds for the trim width

**Files:**
- Create: `tests/oracles/geometry/region_bounds.hpp`, `tests/oracles/geometry/region_bounds.cpp`
- Test: `tests/native_oracle_geometry_region.cpp`

**Interfaces:**
- Consumes: Tasks 1–2.
- Produces:
  - `struct RegionBounds { double sigma_lo, sigma_hi, m_s; };` `RegionBounds region_bounds(const OracleCarrier&, const UvBox&);` — σ̲ and σ̄ bound the singular values of DS over the box, M_S bounds ‖D²S‖; σ̲ = 0 when the box meets a singular locus (pole, apex, interior pole line, irregular point).
  - `double curvature_bound(const OracleCurve&, Vec2 t_range);` (κ̄_C) and `double arc_length_bound(const OracleCurve&, Vec2 t_range);` (ℓ̄₃: control-polygon length of the pieces; closed form on lines, circles, conics)
- Analytic carriers in closed form (cylinder σ ∈ {1, R}; cone σ ∈ {cos-scaled radius, 1}; sphere σ ∈ {R cos v, R}, zero at a pole; torus σ ∈ {R + r cos v, r}). B-splines: σ̄ from the hodograph nets (‖S_u‖ + ‖S_v‖ bounds the spectral norm), σ̲ ≥ min|N|/σ̄ with min|N| the distance from the origin to the box of the Bernstein coefficients of S_u × S_v (rational: the homogeneous form of design §6.2), M_S from the second-difference nets with their degree factors.

- [ ] **Step 1: Write the failing tests**

```cpp
void analytic_closed_forms();       // cylinder R = 2 over any box: sigma_lo == 1, sigma_hi == 2, m_s == 2
void sphere_pole_box_is_singular(); // a box touching v = π/2: sigma_lo == 0
void bounds_hold_by_sampling();     // property: B-spline families (bicubic, rational with weights in [1, 256],
                                    // degree 6 multi-span with multiplicities p − 1 and p): at 1,024 seeded points of
                                    // each of 100 seeded boxes, the sampled singular values lie in [sigma_lo, sigma_hi]
                                    // and the sampled second derivatives are ≤ m_s
void curve_bounds_hold();           // sampled curvature ≤ curvature_bound; sampled arc length ≤ arc_length_bound
```

- [ ] **Step 2: Run them to make sure they fail**

Run: `python3 scripts/check_native_face_domain.py --unit oracles --profile debug`
Expected: build fails: `geometry/region_bounds.hpp` not found.

- [ ] **Step 3: Implement `region_bounds`, `curvature_bound`, `arc_length_bound`**

- [ ] **Step 4: Run them to make sure they pass**

Run: `python3 scripts/check_native_face_domain.py --unit oracles --profile debug --lint`
Expected: `100% tests passed`.

- [ ] **Step 5: Commit**

```bash
git add tests/oracles/geometry/region_bounds.* tests/native_oracle_geometry_region.cpp
git commit -m "V1: region bounds for singular values, second derivatives and curve curvature

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 4: Distance bounds to carriers and curves

**Files:**
- Create: `tests/oracles/geometry/distance.hpp`, `tests/oracles/geometry/distance.cpp`
- Test: `tests/native_oracle_geometry_distance.cpp`

**Interfaces:**
- Consumes: Tasks 1–3.
- Produces:
  - `struct Budget { uint64_t max_nodes = 1u << 20; double target_gap; };`
  - `struct PointBound { Bound distance; Vec2 foot_uv; Vec3 foot; Outcome outcome; };`
  - `PointBound distance_to_carrier(const OracleCarrier&, Vec3 p, const UvBox&, const Budget&);`
  - `PointBound distance_to_curve(const OracleCurve&, Vec3 p, Vec2 t_range, const Budget&);`
- Method: a best-first queue of parameter boxes ordered by `lower_distance(p, enclose(box))`; the upper bound is the least distance from `p` to any evaluated point, refined by at most 8 Newton steps from each popped box's centre; stop when `upper − lower ≤ target_gap` (`resolved`) or after `max_nodes` boxes (`budget`). Closed-form unrestricted answers for plane, sphere and cylinder are used when the box is the whole domain, and checked by the same enclosure test.

- [ ] **Step 1: Write the failing tests**

```cpp
void projection_fixtures_a_to_f() {   // papers/2026-10-13-projection-oracle-study.md, exact expectations
  // A: plane (s,t,0), target (1/4,3/4,1/2): d² = 1/4; (-1/4,5/4,1/2): d² = 3/8 at corner (0,1)
  // B: tilted plane (s,t,(s+2t)/4), X = (-1/4,1/2,1/4): d² = 1/16 at (0,1/2), never (0,11/21)
  // C: rational quarter cylinder, target (3/4,1,1/4): d² = 1/16 at (1/2,1/4)
  // D: same carrier, X = (-3/4,-1,1/4): global d² = 65/16 at (0,1/4), not 73/16
  // E: parabolic cylinder, X = (0,1/2,3/4): d² = 1/2 (two minima; either foot)
  // F: S = (s², t, 0), X = (0,1/2,1/4): d² = 1/16 ... and the constant carrier: d = |X − P|
  // each: lower ≤ exact ≤ upper, upper − lower ≤ target_gap = 0x1p-30, outcome resolved
}
void distance_at_singular_chart();    // sphere R = 1, p = (0, 0, 2): d = 1 exactly within bounds; cone apex p on the axis
void curve_closed_forms();            // circle R = 2, p at its centre: d = 2; line segment endpoint clamping
void budget_is_reported();            // max_nodes = 4 on a degree-9 patch: outcome budget, lower ≤ upper still true
```

- [ ] **Step 2: Run them to make sure they fail**

Run: `python3 scripts/check_native_face_domain.py --unit oracles --profile debug`
Expected: build fails: `geometry/distance.hpp` not found.

- [ ] **Step 3: Implement `distance_to_carrier` and `distance_to_curve`**

- [ ] **Step 4: Run them to make sure they pass**

Run: `python3 scripts/check_native_face_domain.py --unit oracles --profile debug --lint`, then `--profile asan`.
Expected: `100% tests passed`.

- [ ] **Step 5: Commit**

```bash
git add tests/oracles/geometry/distance.* tests/native_oracle_geometry_distance.cpp
git commit -m "V1: branch-and-bound distance bounds to carriers and curves

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 5: The trim-domain builder

**Files:**
- Create: `tests/oracles/geometry/trim_domain.hpp`, `tests/oracles/geometry/trim_domain.cpp`
- Test: `tests/native_oracle_geometry_trim.cpp`

**Interfaces:**
- Consumes: Tasks 1–4.
- Produces:
  - `struct OracleCoedge { OracleCurve curve3d; Vec2 range; int8_t sense; std::optional<OracleCurve> master_pcurve; bool degenerate; Vec3 start, end; };`
  - `struct OracleLoop { bool outer; std::vector<OracleCoedge> coedges; std::optional<Vec3> pole; };`
  - `struct OracleFace { uint64_t id; OracleCarrier carrier; int8_t orientation; std::vector<OracleLoop> loops; };`
  - `struct TrimSegment { Vec2 a, b; double width; uint32_t loop; };` (width = +∞ on a segment whose UV box meets a singular locus)
  - `struct TrimDomain { std::vector<TrimSegment> segments; std::vector<uint32_t> loop_offsets; Outcome outcome; };`
  - `TrimDomain build_trim_domain(const OracleFace&, const Budget&, double width_target);`
  - `enum class Membership : uint8_t { inside, outside, near };` `Membership classify(const TrimDomain&, Vec2 uv);`
- Method: per coedge, the master pcurve when declared (master-plan call: presence 1, master 2), otherwise samples of the 3D curve projected by `distance_to_carrier` with the previous foot as the seed region, each parameter unwrapped to the lift nearest the previous one; each segment is split at its dyadic midpoint until its width ½·ℓ̄·tan(τ̄/2) ≤ `width_target`, with τ̄ = κ̄_uv·ℓ̄, κ̄_uv = (σ̄²κ̄_C + M_S)/σ̲, ℓ̄ = ℓ̄₃/σ̲ over the segment's UV box (Task 3), or until `Budget` (`budget`); a pole loop becomes the iso-line segment at the pole; membership is by winding number with `orient2d_sign`, `near` within a segment's width.

- [ ] **Step 1: Write the failing tests**

The test builds each case below as an `OracleFace` by hand (test-local helpers for lines, circles and the carriers), so this task needs no store; Task 6 writes the same cases as version 3 stores and checks the loader against these hand-built faces.

```cpp
void plane_with_two_holes();            // square [0,4]², circular holes r = .5 at (1,1) and (3,3):
                                        // (2,2) inside, (1,1) outside, (1.5,1) near (on a hole), (5,5) outside
void cylinder_band_with_seam();         // R = 2, v ∈ [0,3], loops: two circles and the seam used twice:
                                        // (1, 1.5) inside, (1, 3.5) outside
void periodic_image_same_membership();  // the same band: classify(u, v) == classify(u + 2π, v) for 64 seeded points
void sphere_cap_with_pole_loop();       // R = 1, latitude circle v = π/4 and a vertex loop at the north pole:
                                        // (0, π/3) inside, (0, 0) outside; segments at the pole have infinite width
void cone_through_apex();               // the apex as a vertex loop: points on the generator inside
void torus_both_seams();                // a full torus face, seams in u and in v: every interior point inside
void closed_bspline_with_seam();        // a bicubic net closed in v (coincident boundary rows), seam edge twice: inside
void master_pcurve_followed();          // a coedge declares its own pcurve as master, offset 1e-3 in UV from the
                                        // 3D curve's projection: membership follows the pcurve
void reconstructed_pcurve_ignored();    // the same pcurve marked presence 2 (reconstructed): membership follows the projection
```

- [ ] **Step 2: Run them to make sure they fail**

Run: `python3 scripts/check_native_face_domain.py --unit oracles --profile debug`
Expected: build fails: `geometry/trim_domain.hpp` not found.

- [ ] **Step 3: Implement `build_trim_domain` and `classify`**

- [ ] **Step 4: Run them to make sure they pass**

Run: `python3 scripts/check_native_face_domain.py --unit oracles --profile debug --lint`, then `--profile asan`.
Expected: `100% tests passed`.

- [ ] **Step 5: Commit**

```bash
git add tests/oracles/geometry/trim_domain.* tests/native_oracle_geometry_trim.cpp
git commit -m "V1: the oracle's own trim domain with sound segment widths

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 6: Distance to a trimmed face, and the store loader

**Files:**
- Create: `tests/oracles/geometry/face_distance.hpp`, `tests/oracles/geometry/face_distance.cpp`, `tests/oracles/geometry/load.hpp`, `tests/oracles/geometry/load.cpp`
- Test: `tests/native_oracle_geometry_face.cpp`, `tests/native_oracle_geometry_load.cpp`, `tests/native_oracle_geometry_fixture.py` (authors Task 5's cases as version 3 stores through `imported_source.write_source(..., version=3)`)

**Interfaces:**
- Consumes: Tasks 1–5; `bgsrc::Store` and `bgsrc::read_store` from P1 (`native/source_store/store.hpp`). If P1 has not landed, rebase onto P1's branch at its Task 3 commit (master plan §8).
- Produces:
  - `PointBound distance_to_face(const OracleFace&, const TrimDomain&, Vec3 p, const Budget&);` — lower bound: the least of the box lower bounds over parameter boxes not classified `outside` and the lower bounds to every boundary curve; upper bound: the least of the distances to points certified `inside` and the upper bounds to every boundary curve (boundary curves lie in the face's closure).
  - `struct SkippedFace { uint64_t id; Outcome outcome; std::string_view reason; };`
  - `struct OracleDefinition { uint64_t id; std::vector<OracleFace> faces; std::vector<SkippedFace> skipped; };`
  - `OracleDefinition load_definition(const bgsrc::Store&, uint64_t definition_id);` — maps version 3 rows to oracle types for every kind; a face that is refused in the store, or uses a refused entity, is `skipped` with its reason.

- [ ] **Step 1: Write the failing tests**

```cpp
void on_hole_boundary_is_near_and_measured_in_3d(); // plane with holes: p on the hole circle, 1e-3 above the plane:
                                                    // classify near; distance_to_face == 1e-3 within bounds
void foot_outside_trim_uses_boundary();             // p above the hole centre at height h: distance = hypot(h, r)
                                                    // (the nearest face point is on the hole's rim), never h
void interior_point_is_carrier_distance();          // p above (2,2) at height h: distance h within bounds
void cylinder_band_outside_axial_range();           // p on the axis line beyond v = 3: distance to the rim circle
void loader_maps_every_kind();                      // the version 3 fixture store of each Task 5 case loads to the
                                                    // OracleFace the test constructed by hand (field by field)
void loader_skips_refused();                        // a face whose carrier is refused in the store is skipped with
                                                    // reason prerequisite; the others load
```

- [ ] **Step 2: Run them to make sure they fail**

Run: `python3 scripts/check_native_face_domain.py --unit oracles --profile debug`
Expected: build fails: `geometry/face_distance.hpp` not found.

- [ ] **Step 3: Implement `distance_to_face` and `load_definition`**

- [ ] **Step 4: Run them to make sure they pass**

Run: `python3 scripts/check_native_face_domain.py --unit oracles --profile debug --lint`, then `--profile asan`.
Expected: `100% tests passed`.

- [ ] **Step 5: Commit**

```bash
git add tests/oracles/geometry/face_distance.* tests/oracles/geometry/load.* tests/native_oracle_geometry_face.cpp tests/native_oracle_geometry_load.cpp tests/native_oracle_geometry_fixture.py
git commit -m "V1: trimmed-face distance bounds and the version 3 store loader

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 7: Trim-domain census on the watch and the corpus

**Files:**
- Create: `tests/oracles/geometry/main.cpp` (`brepgpu-oracle-geometry census STORE [--tsv] [--width-target MM]`)
- Test: `tests/test_oracle_geometry.py`

**Interfaces:**
- Consumes: Tasks 1–6.
- Produces: `brepgpu-oracle-geometry census` builds the trim domain of every face of every definition and prints per face its outcome, segment count, largest finite width, count of infinite-width segments and wall time; text by default, TSV on request. Exit 0 when every face was attempted, 4 on a file-level store fault.

- [ ] **Step 1: Write the failing test**

```python
# tests/test_oracle_geometry.py  (fleet only; BREPGPU_ORACLE_GEOMETRY, BREPGPU_STEP_SOURCE)
class Census(unittest.TestCase):
    def test_void_block_every_face_resolved(self):
        # adapter --store-version 3 on tests/fixtures/void-block.step; census --tsv:
        # every face resolved, no infinite width (no singular locus on a block)
    def test_apex_cone_marks_only_apex_segments(self):
        # tests/fixtures/apex-cone.step: the apex face resolved; infinite width only on segments touching the apex
```

- [ ] **Step 2: Run it to make sure it fails**

Run: `PYTHONPATH=python:tests BREPGPU_ORACLE_GEOMETRY=build/oracles-debug/brepgpu-oracle-geometry python3 -m unittest tests.test_oracle_geometry -v`
Expected: FAIL (`brepgpu-oracle-geometry` not built).

- [ ] **Step 3: Implement the census subcommand**

- [ ] **Step 4: Run it to make sure it passes, then the census**

Run: the Step 2 command (PASS); then, lane-bound,
`python3 scripts/measurement_lane.py run --node 1 --context out/v1-census-context.tsv --label v1-watch -- build/oracles-release/brepgpu-oracle-geometry census --tsv "$BREPGPU_WATCH_STORE"`
and the same per admitted corpus store (label `v1-<stratum>`), at `--width-target` 1e-4 mm and at 3.1e-5 mm (η/10, with η = ε_ref/16 at ε_ref = 0.005 mm); the census records both.
Expected: the unit report lists, per stratum and carrier kind, faces resolved, `budget`, `unsupported` and skipped, with each reason other than `resolved` filed as a bead unless it is a store refusal. Every admitted face of the watch resolves or the reason is filed.

- [ ] **Step 5: Commit and write the unit report**

Write `results/v1-oracle-geometry-<date>/REPORT.md`: grounding, red and green evidence per task, suites and counts, the census tables, fleet-relative paths only.

```bash
git add tests/oracles/geometry/main.cpp tests/test_oracle_geometry.py results/v1-oracle-geometry-*/REPORT.md
git commit -m "V1: trim-domain census over the watch and the corpus

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

## Self-review

- **Spec coverage.** §24 independence (Global Constraints, unit-specific lines; CMake boundary in Task 1); outcomes `pass`/`fail`/`not_evaluated` are V2–V4's, built on this unit's `Outcome` and bounds; "the oracle's trim domain" (Task 5: own projection, own density, the master pcurve where the file declares one); §3.2 distances to trimmed faces (Task 6); §10.5 seams, poles, interior pole lines (Tasks 3, 5); oracle 1's needs (Task 6), oracle 2's (Task 5), oracle 10's (Task 4).
- **Step scan.** Each code step names its file and signature; each test step carries its expected values.
- **Type consistency.** `bgor::OracleFace`, `bgor::TrimDomain`, `bgor::PointBound`, `bgor::load_definition`, `bgsrc::read_store` match the master plan's V1 and P1 blocks.
- **Review focus.** Each line has its test in the task named.
- **Proportion.** Signatures, methods in one line, and assertions; no function bodies.
