# Test plans of mature mesh-generation and triangulation libraries

2026-10-17. Research lane on branch `research/fast-mesher-study`. It studies the test suites of public mesh generators, triangulators and mesh-robustness libraries. The purpose is to catch fault classes that BRepGPU's bootstrap coverage plan (journal/2026-10-17-fast-mesher-direction.md) would otherwise miss. A sibling lane covers the CAD/B-rep tessellators: OCCT, Gmsh, Netgen, FreeCAD, SALOME, truck, SolveSpace, openNURBS and usd-brep.

**Method.** Every finding comes from reading pinned source, test files, CI configuration, corpora and, where noted, retained PDFs. No project was built, run or installed. No geometry ran on the laptop. Every per-project claim cites file:line.

**Who did what.**
- Six read-only sub-lanes wrote the per-project sections:
  - CGAL 2D/kernel
  - CGAL 3D/PMP
  - classic meshers and predicates
  - wild-input meshers
  - metric remeshers
  - 2D polygon triangulators
- The lane owner wrote the GPU/cross-platform section and the synthesis.
- The lane owner then re-read the source for these consequential claims and confirmed each one:
  - Blender `expect_coord_near<mpq_class>` compares x twice.
  - The Blender polyfill shift/reverse loop passes the unshifted polygon.
  - The libigl Hausdorff `break` makes its assertion unreachable.
  - fTetWild calls `exit(0)` on inverted tets and returns `EXIT_SUCCESS` with faces still uninserted.
  - CGAL records a non-terminating seed, 1473902576.
  - geogram has a commented-out Delaunay assert, and its "Emergency exit" breaks out with a possibly non-Delaunay result.
  - spade's CDT sanity check skips the Delaunay property whenever constraints exist.
  - CDT has a fast-math CI job.
  - earcut's `splitEarcut` silently returns when it finds no diagonal.
  - Manifold's polygon fuzzer counts a timeout as a failure.
  - OpenSubdiv's `-pass 2` and its CPU-only backend comparison.

**Citation roots.**
- Paths are relative to `BRepGPU/quarantine/sources/` unless a section says otherwise.
- artem-ogre/CDT is cited relative to `quarantine/` (`direct-cdt-study/CDT-53eff288a4e7/...`, sometimes abbreviated `CDTr/`).
- JIGSAW is cited relative to `quarantine/` (`whole-mesher-study/jigsaw/...`).
- `BL/` means `blender-selected/source/blender/blenlib/`.
- In this workspace `quarantine/sources` and `quarantine/archives` are symlinks into `../quarantine_proj/`.

**Acquired this lane.** Pinned archives are in `quarantine/archives/` and clean extractions in `quarantine/sources/`. Appendix A has catalog rows in `reference-catalog.tsv` format, ready to merge. Appendix B lists what was not acquired.
- **GitHub archives:** CGAL v6.2.1, fTetWild, TetWild, Mmg v5.8.0, libigl v2.6.4 plus libigl-tests-data, OpenSubdiv v3_7_0, earcut v3.2.4, spade v2.15.1, Thingi10K tooling v1.5.0, gDel3D, georust/robust v1.1.0, poly2tri.
- **Non-GitHub sources:** the Mmg CI corpora (mmg, mmg2d, mmgs), Triangle 1.6 (netlib), Shewchuk `predicates.c` (CMU), TetGen 1.6.0 (WIAS), and 10 selected Blender blenlib files.
- **Already in quarantine:** Manifold, geogram, gCDT, OpenSCAD, fidget, NVIDIA Warp, JIGSAW and artem-ogre/CDT were re-read in place.

## Most decision-relevant findings

1. **Mature projects ship broken oracles, and their CI does not notice.** The confirmed cases:
   - libigl's Hausdorff test cannot fail.
   - Blender's exact-coordinate check never compares y, and its polyfill cyclic-shift loop never shifts.
   - geogram's CDT Delaunay assert is commented out.
   - Triangle's `-C`, TetGen's `-C`/`-CC`, gDel3D's checker and gCDT's checker all print and still exit 0.
   - CGAL has a vacuous test because a `.cmd` file is missing, and dead Alpha_wrap_3 asserts.
   - artem-ogre/CDT has a duplicated test, and georust/robust has a tautological symmetry assert.
   - OpenSubdiv passes when *any* two shapes fail.

   Our oracles must change the exit status. Their sensitivity must be measured by mutation testing on the fleet (doctrine Testing item 5), and known-bad inputs must carry exact expected verdicts.
2. **The parameter-space CDT output properties we most need are left unchecked almost everywhere.** None of the six 2D libraries checks every one of these:
   - constraint presence: every input segment appears as a chain of output edges;
   - the *constrained* empty-circle property, which spade skips whenever constraints exist and poly2tri tests in unconstrained form;
   - orientation independent of topology, since CDT's topology check passes inverted triangles;
   - coverage without overlap, since an area balance like earcut's scores an overlap plus an equal-area gap as zero;
   - inserted intersection and split points lying on their parent constraints. CGAL snaps them within 4 ulp silently, spade allows 3e-2, and CDT's golden files store no coordinates.

   Only TetGen counts missing segments. Our bootstrap oracles are 3D (deviation, watertightness). These 2D properties need their own exact oracles on every face.
3. **Device and compiler arithmetic is a fault class that no corpus will expose.**
   - Only artem-ogre/CDT tests that error-free transformations survive `-ffast-math`/FMA in CI.
   - geogram defends with `-ffp-contract=off` plus a startup contraction probe.
   - gDel3D's CUDA predicates use `__dmul_rn` so nvcc cannot fuse them.
   - CGAL reruns its interval suite in a round-to-nearest mode.

   nvcc contracts mul+add by default. Metal's defaults need checking against Apple's documentation; that was not done here. A per-device predicate conformance suite has to come before any GPU CDT claim.
4. **Hitting a termination bound is not the same as producing a valid result.** Five projects hit a cap or error and still report success or emit output:
   - geogram's `Delaunayize_new_edges` "Emergency exit" breaks out and continues;
   - JIGSAW's `MESH_ITER` cap exits the loop silently;
   - TetWild outputs unrounded vertices, logged at debug level only;
   - fTetWild calls `exit(0)` on inverted tets;
   - earcut silently drops geometry.

   The projects that handle it properly return structured incomplete status: spade's `refinement_complete`, CDT's give-up counters, CGAL Mesh_3 error codes, TetGen's refusal codes and mmg's LOWFAILURE. Our "termination within bound" oracle must force every cap to fire and run full property checks on whatever comes back.
5. **Sampled deviation is only a lower bound unless corrected.** TetWild and fTetWild shrink epsilon by the sampling lattice's covering radius (`d/sqrt(3)`). CGAL's bounded-error Hausdorff gives two-sided bounds. Our "dense deviation vs exact geometry" oracle needs one of these, or it can pass meshes that exceed tolerance between samples.
6. **Shared-edge identity is its own oracle.** Two existing checks are the models:
   - mmg's `compare-para-tria.c` checks that frozen interface triangles survive.
   - libigl's `remesh_at_points` checks that points on a shared or flipped non-manifold edge stay in a consistent order.

   For us, a B-rep edge shared by two faces must discretize bit-identically and in matching order on both sides, even when the faces traverse it in opposite directions. Welded watertightness checks hide the mismatch.
7. **Exact-valid output can become invalid when written at output precision.** CGAL's snap-rounding fixtures (`intersection_when_rounded_*`) contain meshes that are valid exactly and self-intersect after rounding to double. Our main consumers (STL to slicers) take float32, so this family belongs in the plan.
8. **The best cross-platform and cross-backend practices to adopt:**
   - Manifold requires a bitwise SHA-256 match of six hard Boolean outputs across Linux, macOS and Windows, and gates performance on both time and peak RSS.
   - fidget instantiates one canonical property suite per evaluator backend, and checks interval-enclosure soundness by interior sampling.
   - Warp runs the same analytic-spec tests on every device.
   - OpenSCAD compares backends against a shared golden image, with a commented list of the cases allowed to differ.

   OpenSubdiv shows the failure mode: six GPU backends, and none of them is compared with the CPU in its regression tests. GPU parity has to be designed in.
9. **Highest-yield adoptable inputs for parameter-space CDT** (paths in Synthesis §2):
   - Manifold's 100-case polygon corpus (Sliver, Precision, CoincidentHole, KissingZigzag, ...);
   - artem-ogre/CDT's 44 inputs with 562 canonical golden files;
   - earcut's 59 fixtures plus its 119,680-polygon integer-grid tile corpus;
   - CGAL's near-degenerate CDT issue regressions and Polygon_repair's 36 malformed-ring WKT files;
   - Blender's non-zero-winding cases;
   - spade's epsilon-grid generator;
   - the cocircular, collinear and thin-shell distributions of gDel3D and gCDT;
   - geogram's dyadic SOS ray-parity test, which compares inside/outside classification with an analytic answer.

## Per-project findings

### Cluster 1: CGAL 2D triangulation, Mesh_2, kernel and predicates

#### CGAL v6.2.1 (commit 28811b671a12b5caa9e3688569dadbc6b3728fe6)

Scope: Triangulation_2 (constrained), Mesh_2, Kernel_23, Filtered_kernel, Number_types, Interval_support, Intersections_2, Polygon, Polygon_repair, Boolean_set_operations_2, and the testsuite/CI machinery. Cartesian_kernel has no `test/` directory; its coverage comes from the Kernel_23 tests. Everything below comes from reading the files. Nothing was built or run.

- **Test inventory**
  - *Constraint intersection semantics per Itag.* `_test_cdt_throwing` inserts two constraints and checks whether `Intersection_of_constraints_exception` is thrown, against the expected class: NO_INTERSECTION, INTERSECTION_WITHOUT_CONSTRUCTION, or INTERSECTION (`cgal/Triangulation_2/test/Triangulation_2/include/CGAL/_test_cls_constrained_triangulation_2.h:37-89`). It is fed 18 hand-built cases: shared endpoint, collinear overlap, containment, the same constraint reversed, a zero-length constraint, a T-junction, and a proper crossing (`...:353-374`). The cases run for No_constraint_intersection_tag, the default `..._requiring_constructions_tag`, Exact_predicates_tag, and Exact_intersections_tag on both a Quotient<MP_Float> kernel and EPECK (`cgal/Triangulation_2/test/Triangulation_2/test_constrained_triangulation_2.cpp:47-89`).
  - *Dimension degeneracy.* A zero-length constraint (0,0)-(0,0) must give dimension 0 with 1 vertex (`_test_cls_constrained_triangulation_2.h:142-146`). Collinear point constraints and collinear segment chains must give dimension 1 (`...:152-165`). Removing a constraint and then a vertex in 1D and 2D is covered at `...:266-339`.
  - *Near-degenerate floating-point constraint crossings (user-bug regressions).* Five near-coincident 3D points projected to xy (`cgal/Triangulation_2/test/Triangulation_2/issue_4405.cpp:32-41`). Eleven constraints with endpoints about 1e-6 apart (`issue_5055.cpp:23-77`). A point at x=-1.56e-21 joined to four nearly collinear targets, then a vertical constraint at x=0 (`issue_5080.cpp:64-84`). Four constraints whose insertion "can lead to an infinite loop, that actually ends with a segfault once the stack overflows" (`test_cdt_degenerate_case.cpp:27-36`). An EPECK copy followed by reinsertion of an overlapping constraint (`issue_7328.cpp:15-26`). Bug #2999, where a computed intersection "should be snapped to p4" (`include/CGAL/_test_cls_const_triang_plus_2.h:121-133`).
  - *Constraint-hierarchy bookkeeping (CDT_plus_2).* Collinear polylines, a self-looping polyline, a polyline that repeats a subconstraint, and two polylines sharing a middle segment. Each has exact expected subconstraint counts (`issue_4025.cpp:57-190`). Enclosing-constraint contexts are checked at `_test_cls_const_triang_plus_2.h:48-82`. Inserting the same polyline twice is at `issue_3447.cpp:31-33`.
  - *CDT in an arbitrary 3D plane.* Ten points on the plane x=-0.375039, one of them with an x that differs by 1 ulp (`test_cdt_2_projection_traits_special_case.cpp:14-24`). The segment-intersection taxonomy covers overlap, inclusion, shared vertex, and a segment that collapses to a point under projection (`...:77-121`).
  - *Cocircular and duplicate points.* A 5x5 integer lattice (`include/CGAL/_test_cls_delaunay_triangulation_2.h:72`). A 28-point symmetric set containing exact duplicates, for example (91,-312) twice (`...:77-88`). `move_if_no_collision` crossing dimension changes (`...:210-327`).
  - *Mesh_2 refinement.* fish-and-rectangle.poly with 3 seeds, refined at three size bounds (`cgal/Mesh_2/test/Mesh_2/test_meshing.cpp:70-95`). Equivalence of `refine_Delaunay_mesh_2`, `refine_mesh()`, and the `try_one_step`/`step_by_step` loops (`...:119-180`). Gabriel and Delaunay conforming (`test_conforming.cpp:91-108`). A very tiny edge (`issue_781.cpp`, input `issue_781_very_tiny_edge.txt`, wired with an argument at `CMakeLists.txt:23-25`). A single triangle that once hit a "vertex very close to a constrained edge" assertion (`test_meshing_user_bug_test_case.cpp:1-25,49-61`). Gabriel conforming on polylines read from OBJ, with convex-hull closure (`test_mesh_obj.cpp:92-154`).
  - *Determinism.* Meshing the same input 20 times must give byte-identical serialized CDTs (`reproductibility.cpp:80-87`). Refinement plus 20 Lloyd iterations, run twice, must give identical face lists (`issue8719.cpp:139-146,183-188`). Meshing under `CGAL::Random_allocator`, which places nodes at random addresses so code that depends on pointer order is exposed (`test_mesh_terrain.cpp:1-9`; allocator at `cgal/STL_Extension/include/CGAL/Random_allocator.h:30-35`).
  - *Lloyd optimizer.* After optimization the vertex count must be unchanged and the constrained-edge count preserved (`test_lloyd.cpp:62-74,106-119`).
  - *Filtered predicates.* For orientation_2/3, side_of_oriented_circle/sphere and compare_squared_radius, the inputs are random points, points collinear, coplanar, cocircular, or cospherical up to rounding, and the same points perturbed by 2^-45 or 2^-40. Each case is evaluated through `Kernel_checker<static-filtered, dynamic-only-filtered>`, which asserts that the signs agree (`cgal/Filtered_kernel/test/Filtered_kernel/Static_filters.cpp:110-241,289-337`; assertion at `cgal/Kernel_23/include/CGAL/Kernel_checker.h:44-88`).
  - *Interval arithmetic and IEEE edge cases.* Fixed points of repeated sqrt, x*x, and division. Overflow saturates to [MAX_DOUBLE,+inf]. Underflow goes to [0,MIN_DOUBLE]. 1/0 gives `largest()`. is_valid and is_finite are checked on NaN and inf intervals (`cgal/Number_types/test/Number_types/Interval_nt.cpp:87-124,137-173,183-200,208-229,318-370`). The same suite reruns under `CGAL_ALWAYS_ROUND_TO_NEAREST` (`Interval_nt_nearest.cpp:1-4`). Get/set of all four FPU rounding modes is in `rounding_modes.cpp:60-100`. NaN/inf validity on raw doubles is in `_test_valid_finite_double.cpp:39-55`.
  - *2D intersections.* Every object pair (point, segment, ray, line, triangle, iso-rectangle, circle, bbox) is tested on touching, overlapping, collinear, and degenerate-bbox cases (`cgal/Intersections_2/test/Intersections_2/test_intersections_2.cpp:234-241,386-439,520-640`). The suite runs on five kernels (`...:928-933`).
  - *Polygon repair (CDT-based even-odd/non-zero labeling).* 36 WKT inputs: bowtie, spikes, back-and-forth, hole-outside, hole-touching-twice, overlapping-edge, not-closed, point-double, spiral (`cgal/Polygon_repair/test/Polygon_repair/data/in/`, driver `repair_polygon_2_test.cpp:23-81`). Validity diagnosis on valid and invalid multipolygons, for example bad orientation and touching vertices (`repair_is_valid.cpp:25-35`). Issue 9325, a mislabeled triangle (`issue9325.cpp:218-253`).
  - *Simple-polygon test.* 12 `.dat` polygons in exact rationals, each with its expected is_simple answer (`cgal/Polygon/test/Polygon/SimplicityTest.cpp:20-52,76-87`). Polygons with 0, 1, or 2 vertices (`...:55-69`).
  - *Booleans on polygons with holes.* 15 bop and 18 aggregate-op cases (`cgal/Boolean_set_operations_2/test/Boolean_set_operations_2/test_bop.cmd`, `test_bop.cpp:138-200`).

- **Oracles**
  - CDT `is_valid()` is tier 5, but it checks less than its name suggests. It checks TDS combinatorics, CCW faces, a convex hull (`cgal/Triangulation_2/include/CGAL/Triangulation_2.h:973-1030`), that constraint flags agree across each shared edge (`Constrained_triangulation_2.h:1556-1567`), and that no unconstrained edge is flippable (`Constrained_Delaunay_triangulation_2.h:1066-1078`). **It does not check that every input constraint appears as a chain of constrained edges, and it does not check that intersection vertices lie on both input segments.** The intersection regression tests (issue_4405, 5055, 5080, test_cdt_degenerate_case) assert nothing at all. They are tier 2 (no crash, no stack overflow), relying on the Debug-build postcondition `is_valid()` in the constructors (`Constrained_Delaunay_triangulation_2.h:173,183`).
  - `_test_cdt_throwing` is tier 7. The answer comes from the documented tag semantics.
  - issue_4025 subconstraint counts and the Polygon simplicity answers are tier 7. They are hand-derived from the geometry.
  - Mesh_2 vertex counts are tier 3 with a tolerance band: `64 <= n <= 72`, `190..210`, `580..640` (`test_meshing.cpp:75-95`), and exact `== 36`, `== 17`, `== 29` (`test_meshing.cpp:189`, `test_conforming.cpp:102,108`). These numbers were recorded from earlier runs. **No Mesh_2 test checks the advertised output guarantees.** None checks the minimum angle against `B`, edge size against the size bound, that every face is in the domain, or that seeds carved holes. API-equivalence checks (`test_meshing.cpp:142,161,179`) are tier 4 (metamorphic: the same result by different driving loops). `is_conforming_Gabriel()` / `is_conforming_Delaunay()` (`test_conforming.cpp:94,98`) are tier 5. Determinism tests are tier 4, run-to-run identity. test_lipschitz is tier 1: it only does `set_K(2)` and `get_K()==2` (`test_lipschitz.cpp:30-32`).
  - Static_filters is tier 6, a differential between two filter paths that should both end in exact evaluation, on adversarial near-degenerate inputs. `Kernel_23/test/Kernel_23/Kernel_checker.cpp` is weak: K1 and K2 are the same kernel, per the comment "Does not fully work yet, so I choose a simple case K1 == K2" (`:12-16`). It checks nothing.
  - Interval_nt tests are tier 7. Expected fixed points are derived analytically, for example sqrt converging to [1-2^-52, 1+2^-52] at iteration 54 (`Interval_nt.cpp:124`).
  - Intersections_2 is tier 7 against hand-written answers. Approximate kernels compare with 1e-3 relative tolerance, for example `P(3.42105,-0.657895)` (`test_intersections_2.cpp:17,69-79,428`), so on EPICK this is loose. It is also tier 4 metamorphic: argument swap, plus `do_intersect` agreeing with `intersection` (`...:125-179`).
  - Polygon_repair WKT is tier 3 golden output with an exact `rmp == refmp`, plus a tier 5 orientation property: outer boundaries CCW, holes CW (`repair_polygon_2_test.cpp:73-81`). issue9325 is tier 6: an independent winding-number implementation on the raw rings is compared with point location in the repaired output at witness points (`issue9325.cpp:94-123,191-200`).
  - BSO2 is tier 3 plus tier 4. The goldens were produced by the library itself (`bop_test_suite_generator.cpp:38-66`). The operand swap p1,p2 versus p2,p1 is checked (`test_bop.cpp:146-200`).

- **Corpora**
  - Everything is small and in-tree. Mesh_2: `fish.poly` (40 lines), `fish-and-rectangle.poly` (86 lines; header "produced by the CGAL::Mesh_2 package", so it is self-generated), four `triwild-10070-min{1..4}.obj` files of 6-15 lines, minimized from a TriWild case, plus `mini.obj` and `issue_781_very_tiny_edge.txt` (12 lines, coordinates at about 1e-7 relative spacing near 44.37). Only `mini.obj` runs by default (`test_mesh_obj.cpp:170`). The triwild minimizations are not wired into ctest.
  - Shared `cgal/Data/data/2d_segments/` holds 15 `.edg` files, including `norway.edg` (40,578 segments) and `clusters.edg`. Of these, Mesh_2 tests use only `fish.edg` (`test_conforming.cpp:55`). `norway` appears only in the manual benchmark `cgal/Triangulation_2/benchmark/Triangulation_2/CDT_insert_on_constraints.cpp:12`.
  - Triangulation_2: `data/layers.xy` (502 points on vertical lines x=10,... at 0.01 spacing, which is highly cocircular and collinear).
  - Polygon_repair: 36 input/reference WKT pairs. Polygon: 16 `.dat` files. BSO2: 56 data files.
  - All of this is hand-built or minimized from user reports. There is no external corpus.

- **Pathology handling**
  - *Tests.* There is no explicit hang test. The only guard is ctest `--timeout 1200` per test in the nightly runner (`cgal/Scripts/developer_scripts/run_testsuite_with_ctest:371`), reported as status 't' (`cgal/Testsuite/test/parse-ctest-dashboard-xml.py:92,119`). The infinite-recursion CDT case (`test_cdt_degenerate_case.cpp:35-36`) is caught only as a segfault or a timeout. No test feeds NaN or inf into any geometric predicate, triangulation, or mesher; NaN/inf appear only in Number_types tests. There are no memory-limit tests.
  - *Runtime: CDT intersections.* With No_constraint_intersection* tags, CDT refuses explicitly by throwing `Intersection_of_constraints_exception` (`cgal/Triangulation_2/include/CGAL/Constrained_triangulation_2.h:131,349,726,1036,1048`). With Exact_predicates_tag it builds the crossing point inexactly (`insert_intersection`, `:1155`). If that point falls outside the two triangles adjacent to the crossed edge, it recomputes with the exact kernel. If the exact point is still outside, it snaps to the nearer endpoint of the crossed edge (`:1169-1216`). Floating-point results are also snapped to any input endpoint within a 4-ulp dilated bbox (`:1806-1815`, tunable through `CGAL_CDT_2_INTERSECTION_SNAPPING_ULP_DISTANCE`). If no point can be built, `limit_intersection` picks the input endpoint nearest the other line (`:1882-1903`). So constraints are silently bent toward existing vertices: output is no longer exactly the input PSLG, and no flag is raised.
  - *Runtime: predicates.* The static filter falls back to the dynamic filter when magnitudes are below 1e-146 or at least 1e153 (`cgal/Filtered_kernel/include/CGAL/Filtered_kernel/internal/Static_filters/Orientation_2.h:75-89`). The interval filter falls back to exact evaluation on `Uncertain_conversion_exception` (`cgal/Filtered_kernel/include/CGAL/Filtered_predicate.h:107,180`). A NaN input reaches the exact number type, where only an assertion stops it (`cgal/Number_types/include/CGAL/Mpzf.h:447` "Creating an Mpzf from infinity or NaN"; `GMP/Gmpq_type.h:164`). These checks disappear in release builds.
  - *Runtime: Mesh_2 termination.* The refine loop is `while(!is_algorithm_done())` with no cap (`cgal/Mesher_level/include/CGAL/Mesher_level.h:283,392`). Termination rests on theory: B >= sqrt(2) (20.7 degrees), and input angles of 60 degrees or more for full guarantees (`cgal/Mesh_2/doc/Mesh_2/Mesh_2.txt:150-192`). Near small input angles, the Clusters logic (`Mesh_2/include/CGAL/Mesh_2/Clusters.h:535-575`) accepts bad triangles instead of looping. Nothing checks a user-supplied B that is too aggressive. The Lloyd optimizer does have `max_iteration_number` and `time_limit` caps (`Mesh_2/include/CGAL/Mesh_2/Mesh_global_optimizer_2.h:98,152-205`).

- **Fuzzing / mutation / property-based / metamorphic / differential / performance-regression**
  - Fuzzing: none found. A search for libFuzzer, OSS-Fuzz, AFL, and honggfuzz found only a bib entry.
  - Seeded randomized testing is real. In testsuite builds `get_default_random()` seeds from wall-clock time and prints the seed (`cgal/Random_numbers/include/CGAL/Random.h:233-241`, `Random_impl.h:50-58`), so each nightly run explores new inputs while staying reproducible. Static_filters accepts `loops` and `seed` on argv (`Static_filters.cpp:291-293`). The Triangulation_2 tests use unseeded `std::rand()`, which is deterministic (`_test_cls_delaunay_triangulation_2.h:333`).
  - Test-case minimization: `CGAL::bisect_failures` (`cgal/STL_Extension/include/CGAL/bisect_failures.h`) is a delta-debugging reducer over points and constraints. It is wired into `issue_781.cpp:235-241` and `test_mesh_obj.cpp:203-205`, and it produced the triwild-min files.
  - Differential: Kernel_checker (Static_filters) and issue9325's winding-number cross-check.
  - Metamorphic: argument swap in Intersections_2 and BSO2, API-path equivalence in Mesh_2, and run-to-run identity.
  - Performance regression: none. `Triangulation_2/benchmark/` and `Number_types/test/Number_types/bench_interval.cpp` time things and print, with no thresholds. `Filtered_kernel/test/Filtered_kernel/tst10.cpp` is an old benchmark with commented-out timings.

- **CPU/GPU or cross-platform consistency**
  - No GPU code. Cross-platform consistency comes from the nightly testsuite: `autotest_cgal_with_ctest` downloads LATEST, builds each `PLATFORM` from the reference platform list, and uploads results to cgaltest.geometryfactory.com (`cgal/Scripts/developer_scripts/autotest_cgal_with_ctest:11-14`). Builds default to **Debug**, so all `CGAL_assertion`/postconditions are live (`cgal/Installation/cmake/modules/CGAL_SetupFlags.cmake:47-53`). The runner machines (bonnard, cgal, friedrich, rubens) run `cgal/testsuite-docker:*` images (`list_test_runner_machines:6-11`, `cgal_testsuite_report.py:112`). The image definitions are not in the archive (they live on Docker Hub under cgal/testsuite-docker) and were not read, so the compiler/OS matrix is unknown from this tree.
  - GitHub Actions runs only ubuntu-latest + clang with `ctest -L Installation` (build-system tests, not package tests) (`cgal/.github/workflows/cmake-all.yml:18-20`), plus merge checks and Lab demo builds (`checks.yml`, `demo.yml`, `.github/test.sh`).
  - Sanitizers: `CGAL_USE_ASAN` adds `-fsanitize=address` (`cgal/Installation/cmake/modules/CGAL_SetupCGALDependencies.cmake:113-116`). No workflow or script in the tree turns it on. UBSan, TSan, and valgrind: none found.
  - Rounding-mode portability: the Interval_nt suite runs both with directed rounding and with `CGAL_ALWAYS_ROUND_TO_NEAREST`, where `IA_up` uses `nextafter` instead of hardware round-up (`cgal/Number_types/include/CGAL/FPU.h:347-358`). The expected answers differ per mode (`Interval_nt.cpp:33-37,98-124`).

- **Known-bad / expected-failure handling**
  - `WILL_FAIL TRUE` for the gdb pretty-printer test when gdb is absent (`cgal/Kernel_23/test/Kernel_23/CMakeLists.txt:39-45`).
  - EPECK_with_sqrt meshing is commented out (`test_meshing.cpp:203-207`).
  - One circle-point case is commented out (`test_intersections_2.cpp:333`).
  - Polygon_repair silently skips inputs that have no reference file (`repair_polygon_2_test.cpp:53-57`). `exact_test.cpp` asserts nothing; it only draws.
  - `test_RT_or_FT_predicates` and `atomic_compilation_test` build only when `CGAL_KERNEL_23_TEST_RT_FT_PREDICATE_FLAGS` is set (`Kernel_23/test/Kernel_23/CMakeLists.txt:73-87`).
  - `issue_3301.cpp` and `issue_129.cpp` only print or catch.
  - Triangulation_2 I/O tests are serialized with `RESOURCE_LOCK` (`Triangulation_2/test/Triangulation_2/CMakeLists.txt:21-33`).
  - There are no xfail lists or tolerance whitelists beyond the tolerance bands above.

- **Worth adopting for BRepGPU**
  1. **The issue-regression set as parameter-space CDT trim adversaries.** `issue_4405/5055/5080/7328.cpp` and `test_cdt_degenerate_case.cpp` are exactly near-tangent, near-coincident trim-loop endpoints in double precision. The x=-1.56e-21 against x=0 case is a seam or pole trim. Import them verbatim and upgrade the oracle to tier 5/6. Also import `issue_781_very_tiny_edge.txt` (tiny edge beside a large domain) and `triwild-10070-min*.obj`.
  2. **The `_test_cdt_throwing` truth table (`_test_cls_constrained_triangulation_2.h:353-374`).** Make an equivalent per-policy table for BRepGPU's "refuse vs. split" trim-intersection policy. CGAL's tag-selected throw is a good model for explicit refusal.
  3. **Fill the constraint-recovery gap CGAL leaves open.** Add an oracle that every input trim segment is a union of output edges, and that every inserted Steiner/intersection point lies on both source segments, checked exactly. CGAL's snapping (`Constrained_triangulation_2.h:1169-1216,1806-1815`) bends constraints with no flag. BRepGPU should either forbid that or record it as a measured deviation.
  4. **Static-filter differential with constructed degeneracies (`Static_filters.cpp`).** Points collinear or cocircular up to rounding, then perturbed by 2^-45, sent through two predicate paths that must agree. Run this as a CPU-exact vs. GPU-filtered differential on P100 and Metal. Include magnitudes below 1e-146 and above 1e153, because the filter changes regime there.
  5. **Round-to-nearest interval mode (`FPU.h:347-358`, `Interval_nt_nearest.cpp`).** Metal has no directed rounding, so BRepGPU's GPU filters need the nextafter-style approach. Port the analytic fixed-point tests (`Interval_nt.cpp:87-229`) as tier 7 checks of the GPU interval library.
  6. **Determinism and address-independence:** `reproductibility.cpp`, `issue8719.cpp`, and `Random_allocator` (`test_mesh_terrain.cpp`). GPU atomics and hash ordering make this fault class likely. Run each case N times plus once with randomized allocation, and compare serialized output.
  7. **Seed-logged randomized runs plus `bisect_failures` minimization**, the pattern behind the triwild-min files. This fits the planned mutation fuzzer: log the seed, then auto-reduce failing STEP/B-rep parameter sets.
  8. **The issue9325 independent-classifier oracle.** Compute even-odd or winding containment of witness points (triangle centroids) directly from raw trim loops and compare with the tessellator's face-in-domain labeling. This is a cheap tier 6 check against inside/outside mislabeling of trimmed regions. Also import the Polygon_repair `data/in/*.wkt` cases (spikes, back-and-forth, hole-touching, overlapping-edge, not-closed) as malformed-trim-loop inputs.
  9. **Add the Mesh_2 output-guarantee checks CGAL lacks:** minimum angle at least asin(1/(2B)) except near small input angles, the size bound, domain coverage, and an iteration/time cap on refinement. CGAL's refine loop has no cap (`Mesher_level.h:283`).

**Blind spots this project reveals** (fault classes CGAL tests for that the bootstrap plan would miss):
- Constraint-intersection policy as a specified truth table (overlap, containment, T-junction, zero-length, duplicate constraint), checked for each policy.
- Constraint-hierarchy bookkeeping under shared or repeated sub-segments and self-looping trim polylines (`issue_4025.cpp`).
- Run-to-run and allocation-address determinism.
- Filter-regime boundaries (1e-146 and 1e153) and rounding-mode dependence of interval filters, which matters directly for Metal.
- Cocircular lattices and exact duplicate points as Delaunay tie-breaking stress (symbolic perturbation in `is_flipable(...,perturb)`, `Constrained_Delaunay_triangulation_2.h:725`).
- Automatic test-case minimization so that fuzz findings become small, permanent regressions.
- Even-odd versus winding-number inside/outside classification checked by an independent implementation.
- Self-intersecting, unclosed, or spiked trim loops as an input class (the Polygon_repair corpus), distinct from geometric degeneracy.
- Conversely, CGAL itself misses: NaN/inf geometric inputs, hang tests with tight bounds (only a 1200 s ctest timeout), constraint-recovery checks, mesh-quality guarantee checks, sanitizer CI, and fuzzing. BRepGPU's plan already covers some of these and should not assume CGAL validates them.

### Cluster 2: CGAL Mesh_3, surface meshing, Polygon_mesh_processing, Alpha_wrap_3

#### CGAL v6.2.1 (commit 28811b671a12b5caa9e3688569dadbc6b3728fe6)

Scope note: at this revision PMP is split into `Polygon_mesh_processing` (core: self-intersection, distance, predicates), `PMP_Mesh_repair`, `PMP_Boolean_operations`, `PMP_Remeshing`. `Surface_mesher` is present but deprecated in favour of Mesh_3 (`cgal/Surface_mesher/include/CGAL/make_surface_mesh.h:17-20`). Read only; nothing built or run.

What actually runs: GitHub CI only configures and runs `ctest -L Installation` (`cgal/.github/workflows/cmake-all.yml:20,33`); `checks.yml` runs merge/dependency/header scripts. The package tests run in GeometryFactory's nightly testsuite on private Docker hosts (`cgal/Scripts/developer_scripts/list_test_runner_machines:6-11`) with `ctest --timeout 1200` per test (`cgal/Scripts/developer_scripts/run_testsuite_with_ctest:371`). Results pages are not in the archive. The testsuite build defines `CGAL_TEST_SUITE`, and the build fails if `NDEBUG` is also set (`cgal/Installation/include/CGAL/config.h:52-54`), so the plain `assert`s these tests rely on are live. Under `CGAL_TEST_SUITE`, `CGAL::get_default_random()` is seeded from `std::time` and prints its seed (`cgal/Random_numbers/include/CGAL/Random.h:233-241`, `Random_impl.h:39-59`). Every test that uses the default RNG therefore runs on a different seed each night. ASan is opt-in only (`CGAL_USE_ASAN`, `cgal/Installation/cmake/modules/CGAL_SetupCGALDependencies.cmake:113-116`). No package-level WILL_FAIL, TIMEOUT or skip properties exist in any test CMakeLists in scope.

- **Test inventory**
  - *Mesh_3 shared harness (refinement, idempotence, optimizer invariants)*: `Tester::verify` (`cgal/Mesh_3/test/Mesh_3/test_meshing_utilities.h:88-192`) does the following.
    - Asserts V/F/C counts lie in a band.
    - Re-runs `refine_mesh_3` on the finished mesh and requires the vertex count to stabilise within 10 passes (`:131-144`).
    - Then runs exude, perturb, ODT and Lloyd. After each it asserts:
      - V (and F for exude) unchanged;
      - min dihedral angle non-decreasing (`:237-247`);
      - volume within 5% (`:283-293`);
      - Hausdorff distance (facet centroid to input AABB tree) at most 1.01x the pre-optimisation value, 4x in parallel (`:396-412`).
    - Used by `test_meshing_polyhedron.cpp:89-103` (sphere.off, bands 119-121 V / 200-204 F / 350-360 C sequential, wider in parallel), `test_meshing_implicit_function.cpp:44-87` (implicit unit sphere, error bound 1e-3), `test_meshing_polyhedron_with_features.cpp:104-136` (cube.off with `detect_features`, no count bands), and the image tests (`test_meshing_3D_image.cpp:68-69`).
  - *Surface completeness (no holes)*: `test_mesh_3_issue_1554.cpp:75-102` runs on `data/thepart-simplified.off` (from the `.cmd`) with features. For every in-complex cell whose neighbour is out of the complex, the shared facet must be a surface facet. Otherwise it prints the facet and exact weighted circumcenters and exits 1.
  - *Feature-edge protection (protecting balls)*: `test_meshing_polylines_only.cpp:145-189` runs on `polylines_3/couplingdown-polylines.txt`. Two vertices form a complex edge iff their protecting balls intersect, and a dimension-1 vertex must lie on the same curve as its incident complex edge. `test_domain_with_polyline_features.cpp:129-201` covers hand-built corners and curves, cycle detection, and signed geodesic distance along polylines, checked with `near_equal`.
  - *Criteria semantics*: `test_criteria.cpp:208-298` builds cells and facets by hand and asserts bad/good classification and badness ordering for angle, radius and distance bounds. `test_labeled_mesh_domain_3.cpp:90-251` asserts initial points lie on the unit sphere within the error bound, plus is_in_domain and intersection answers.
  - *Termination guards*:
    - `test_min_size_criteria.cpp:111-129` (liver.inr.gz): re-refines with size targets 100x smaller but `*_min_size` 1.00001x the current largest element, and asserts zero new vertices.
    - `test_meshing_polyhedral_complex_with_manifold_and_min_size.cpp:72-82`: `manifold()` combined with `edge_min_size` / `facet_min_size` on intersecting sphere patches. It only has to finish.
    - `test_min_edge_length.cpp:50-64` (dragknob.off, `edge_min_size=0.1`): only has to finish.
  - *Edge-distance criterion*: `test_max_edge_distance.cpp:84-101` (dragknob.off, quadDomainCube.inr). With `edge_distance` set, there must be more vertices, smaller min and average edges, and a smaller summed chord-to-geodesic-midpoint error than without it.
  - *Determinism*: `test_meshing_determinism.cpp:76-155` runs make_mesh_3, Lloyd, ODT, perturb and exude twice in one process. MEDIT and OFF outputs must be byte-identical, both Sequential and Parallel with 1 thread. There is also a no-features twin (`test_meshing_without_features_determinism.cpp:140-145`).
  - *IO round trip*: `test_meshing_polyhedron_with_features.cpp:141-149` (binary save/load triangulation equality) and `test_mesh_3_issue_8773.cpp:123-125`.
  - *Surface-only vs volume*: `test_remeshing_polyhedron.cpp:56-79`. With `surface_only` there are facets and zero cells. Without it, cells exist iff the input is closed.
  - *PMP self-intersection / do_intersect*:
    - Inputs: `self_intersection_surface_mesh_test.cpp:44-79,150-201` (elephant false, mannequin-devil true, overlapping_triangles true, degtri_single true).
    - The enumerator and `does_self_intersect` must agree, under EPICK (parallel) and EPECK; `maximum_number(40)` is exact sequentially.
    - The soup convention is at `self_intersection_triangle_soup_test.cpp:190-238`. A shared vertex by index is not an intersection; the same geometry with duplicated point indices is.
    - `pmp_do_intersect_test.cpp:358-428`: nested spheres with bounded-side tests.
  - *Repair*:
    - Degenerate edges/faces on the 16 `meshes/degeneracies/*` files: exact before/after counts, `is_valid_polygon_mesh`, EPICK and EPECK, Surface_mesh and Polyhedron (`cgal/PMP_Mesh_repair/test/PMP_Mesh_repair/test_pmp_repair_degeneracies.cpp:283-363`).
    - Non-manifold vertices are injected by merging vertices of valid meshes, then `duplicate_non_manifold_vertices` must return exactly the injected count and leave none (`test_pmp_manifoldness.cpp:102-167`).
    - Stitching: exact pair counts, and refusal (0 pairs) on non_stitchable / non_manifold (`test_stitching.cpp:172-215`).
    - Soup repair and orientation: `test_repair_polygon_soup.cpp:116-592`, `orient_polygon_soup_test.cpp:59-179`, `test_is_polygon_soup_a_polygon_mesh.cpp:132-150`.
    - Volume / orientation: `test_does_bound_a_volume.cpp:29-33`, `test_split_volume.cpp:30-163`, `orient_polygon_mesh_test.cpp:34-72`.
  - *Hole filling and triangulate_faces*:
    - Patch from an n-gon must have n-2 triangles with valid indices; collinear and null-edge polylines must return empty (`triangulate_hole_polyline_test.cpp:95-129,224-227`).
    - Non-manifold or collinear holes must return empty (`triangulate_hole_Polyhedron_3_test.cpp:161-184`).
    - CDT2 planar, concave and curved holes (`triangulate_hole_with_cdt_2_test.cpp:83-174`).
    - `cgal/PMP_Remeshing/test/PMP_Remeshing/triangulate_faces_test.cpp:237-346` includes the dual of elephant: warped, non-planar polygon faces.
  - *Hausdorff*: `cgal/Polygon_mesh_processing/test/Polygon_mesh_processing/test_hausdorff_bounded_error_distance.cpp:397-789` (analytic synthetic cases), `:361-362` (translation), `:315` (symmetric = max), `:849-895` (vs sampling), `:1072` (parallel = sequential).
  - *Boolean / corefinement / autorefine / snap rounding*:
    - `cgal/PMP_Boolean_operations/test/PMP_Boolean_operations/test_autorefinement.cpp:100-166` (28 cases, 8 stored integers each, expected `Triple_intersection_exception`, output soup must not self-intersect).
    - `test_snap_rounding.cpp:83-86`.
    - `test_corefinement_bool_op.cpp:172-243`: an expected per-op success flag, run over 21 output-aliasing scenarios.
    - `test_corefine.cpp:60-87`.
    - `test_degenerate_pmp_clip_split_corefine.cpp:84-102`.
    - `test_mesh_kernel.cpp:278-442`.
  - *Remeshing*:
    - `remeshing_test.cpp:243-291`: sharp-feature isotropic remeshing on cheese, which must not self-intersect.
    - `delaunay_remeshing_test.cpp:91-111`: three constraint APIs must give the same counts.
    - Smoothing: constrained vertices must stay bit-identical (`test_mesh_smoothing.cpp:113,144`).
  - *Alpha_wrap_3*: all checks are in `cgal/Alpha_wrap_3/include/CGAL/Alpha_wrap_3/internal/validation.h`. Used by `test_alpha_wrap_3_mesh.cpp:136-152`, `test_AW3_manifoldness.cpp:56-92`, `test_AW3_multiple_calls.cpp:65-86` and `test_AW3_cavity_initializations.cpp:26-95`.
  - *Surface_mesher / AFSR*:
    - `cgal/Surface_mesher/test/Surface_mesher/implicit_surface_mesher_test.cpp:141-191`: near-tangent nested spheres (gap 0.01) under 3 manifold tags.
    - `test_robust_circumcenter.cpp:16-25`: a 4x4x4 cospherical lattice.
    - `cgal/Advancing_front_surface_reconstruction/test/Advancing_front_surface_reconstruction/lowdim.cpp:48-51`: inputs of 1 point, 2 points, a triangle and a planar set.

- **Oracles**
  - *Mesh_3 count bands* (`test_meshing_polyhedron.cpp:101-102`, `test_meshing_unit_tetrahedron.cpp:106-110`, `test_meshing_implicit_function.cpp:80,86`): tier 3. These are recorded numbers widened by hand. Because the seed changes nightly, they double as a weak randomized regression.
  - *Volume checks*: hard-coded volumes such as sphere 0.479171765761454 (`test_meshing_polyhedron.cpp:89`) and image volumes are tier 3. The unit tetrahedron's 1/6 +/-5% (`test_meshing_unit_tetrahedron.cpp:103-104`) is tier 7.
  - *Re-refine idempotence and optimizer invariants*: tier 4 (metamorphic). The Hausdorff check is relative to the pre-optimization value, not to `facet_distance`. No Mesh_3 test checks the facet_distance, facet_angle or facet_size bound on the output. The criteria are tested only in isolation on hand-made cells (`test_criteria.cpp`, tier 7).
  - *Strongest Mesh_3 property oracles*: issue_1554 boundary completeness and the protecting-ball adjacency property (tier 5); min_size zero-new-vertex (tier 4/5); geodesic distances (tier 7).
  - *Determinism*: tier 4. It is same-process only, not cross-platform.
  - *Weak Mesh_3 tests*:
    - Both polyhedral-complex tests: the verify call is commented out ("does not work because of Hausdorff distance", `test_meshing_polyhedral_complex.cpp:104-107`). Tier 1.
    - `test_min_edge_length.cpp`, `test_mesh_capsule_var_distance_bound.cpp:52-70` and `test_without_detect_features.cpp`: tier 1.
    - `test_meshing_polyhedron_with_features.cpp` passes `manifold()` but never checks manifoldness.
  - *PMP repair*:
    - Degeneracy counts on tiny hand files: tier 7. Counts on larger files: tier 3.
    - Inject-then-repair manifoldness: tier 4.
    - Soup classification: tier 7.
    - The `remove_degenerate_*` return value is deliberately not asserted (`test_pmp_repair_degeneracies.cpp:156-162`).
    - Snapping tolerance-abuse (`test_pmp_non_conforming_snapping.cpp:89-166`): tier 1, writes files only.
    - Polyline simplification never checks its Fréchet bound: tier 1.
  - *Self-intersection*: tier 7 booleans plus a tier-6 cross-check between two APIs and two kernels.
  - *Hausdorff*: the bounded-error test is the best oracle in scope. It is tier 7 for analytic cases within 1e-4, tier 4 for translation, tier 6 against the sampler. `test_pmp_distance.cpp:258-320` only prints (tier 1).
  - *Corefinement / Booleans*:
    - The accept/refuse flag per operation is tier 7: hand-derived manifoldness of the result (`cgal/PMP_Boolean_operations/include/CGAL/Polygon_mesh_processing/corefinement.h:258-262`).
    - Output checks are only `is_valid()` (halfedge combinatorics) plus visitor bookkeeping: weak tier 5. No closedness, volume or self-intersection check after corefinement.
    - `test_coref_epic_points_identity.cpp` (corefine twice and require no change, `:30-42`) reads `argv` but has no `.cmd` file, so its loop runs zero times. It is vacuous.
  - *Autorefine / snap*: the output must not self-intersect (tier 5) plus 8 stored counts (tier 3).
  - *Remeshing*:
    - Isotropic remeshing checks only no-self-intersection. Nothing checks edge length, constraint survival or deviation.
    - `remeshing_quality_test.cpp` asserts nothing.
    - triangulate_faces checks only `return true && is_triangle_mesh`: no area-coverage, flip or overlap check.
  - *Alpha_wrap_3*: `is_valid_wrap` (`validation.h:132-217`) is a strong tier-5 stack. It requires non-empty, valid halfedge structure, triangle mesh, closed, `does_bound_a_volume`, no degenerate faces, no non-manifold vertices and no self-intersection. Non-manifold edges are caught only indirectly via the self-intersection test (admitted at `:33`).
    - `is_outer_wrap_of_triangle_mesh` (`:229-282`): no intersection with the input, and every input vertex strictly inside.
    - `has_expected_Hausdorff_distance` (`:107-129`): bound alpha+offset, a spec-derived tier 7.
    - Weaknesses:
      - If degenerate faces survive pre-cleaning, all validity asserts are skipped (`test_alpha_wrap_3_mesh.cpp:56-63`).
      - The Hausdorff and edge-length asserts there are dead code behind a constant flag (`:46,61-66`).
      - Volume growth from manifold repair only warns (`test_AW3_manifoldness.cpp:91-92`).
      - An empty wrap is accepted in the cavity test (`test_AW3_cavity_initializations.cpp:93-95`).
  - *Surface_mesher and AFSR*: tier 1-2, no output assertions. The exception is `test_c2t3_iterators.cpp:63-67`, which checks structural counts (tier 7).

- **Corpora**
  - Shared corpus `cgal/Data/data/`: 183 files under `meshes` (32 MB), 11 under `images` (1.6 MB, e.g. liver.inr.gz, skull_2.9.inr), 2 under `polylines_3`, 35 under `points_3`.
    - `meshes/degeneracies/` has 16 files: all_degen, caps_and_needles, deg_on_border, degtri_* (single, four, nullface, edge, sliding variants), existing_flip, fused_vertices, small_ccs, trihole.
    - Other named pathologies: non_manifold.off, non_manifold_face_graph.off, degenerated-sd.off, tetrahedron_flat.off, spheres_intersecting.off, tetra_intersected_by_triangle.off, *-shuffled.off (vertex-order permutations), and open/holed meshes.
    - CAD-like meshes with sharp features: fandisk, cube, joint, rotor, pinion, couplingdown, ALSTOM_TEST4, cheese.
  - Mesh_3 local `test/Mesh_3/data/` has 3 files: sphere.off, elephant.mesh, thepart-simplified.off.
  - PMP_Mesh_repair data: data-polygon_soup 8, data-repair 35, data-snapping 20, data-stitching 17. `data-stitching/deg_border.off` is unreferenced.
  - PMP_Boolean_operations data: data-autoref 28, data-clip 15, data-coref 244 (140 of them under `coplanar_triangles/`, including `deg/collinear_edges` and `deg/vertex_on_edge`; plus `star_tgt*` tangency grids and `nm/`), data-snap_rounding 10, data-slicer 1.
    - Standout cases: `intersection_when_rounded_1.off` has a vertex at x=1e-30, which rounding moves onto a plane and so creates an intersection. `collapse_*` and `no_collapse_*` test rounding-induced collapse.
  - PMP_Remeshing data: data-remeshing 5, data-decimation 26. `data-decimation/sphere_selection.off`, read by `test_decimation_of_planar_patches.cpp:460`, is missing from the archive, so that sub-test runs on an empty mesh.
  - Alpha_wrap_3 `test/Alpha_wrap_3/data/` has 17 hand-authored defect files (about 780 KB): tetrahedron_degenerencies (faces `0 1 0`, `0 0 0`), duplicates, non_manifold, combinatorial_manifold*, self-intersecting / twisted / open tips, plane.off (zero volume), overlay_triangulation (275 isolated triangles), two_knives / three_knives (thin blades with near-coincident x), and random_perturbation.
  - Missing data:
    - The repair-self-intersection test data (brain, flute, dinosaur, hemispheres) is absent; the test is disabled anyway.
    - Thingi10K is used only by the manual Alpha_wrap_3 benchmark (`cgal/Alpha_wrap_3/benchmark/Alpha_wrap_3/benchmarking.sh:45,68`, chart titles at `Robustness/generate_robustness_benchmark_charts.py:130`). The Mesh_3 charting script expects a local `Thingi10k/raw_meshes` directory (`cgal/Mesh_3/benchmark/Mesh_3/Charting/benchmarking.sh:7,33`). There is no URL in the archive. The public source is https://ten-thousand-models.appspot.com; it was not read.
    - `cgal/Mesh_3/benchmark/Mesh_3/benchmark_inputs.txt` points at private paths (`D:/INRIA/Data/_Models/BigOFF/lucy-100kt.off`).

- **Pathology handling**
  - *Termination theory*: the Mesh_3 docs guarantee termination when facet_angle <= 30 degrees for smooth surfaces (`cgal/Mesh_3/doc/Mesh_3/Mesh_3.txt:412-413`) and when cell_radius_edge_ratio > 2 (`:441`). Protecting balls give termination "whatever small dihedral angles" (`:1385-1387`). Acute polyline corners are forced to be terminal vertices "to ensure termination" (`cgal/Mesh_3/include/CGAL/Mesh_3/polylines_to_protect.h:336-340`). `detect_features` refuses to run twice to "prevent from not-terminating" (`cgal/Mesh_3/include/CGAL/Polyhedral_mesh_domain_with_features_3.h:468-470`).
  - *Runtime stop mechanisms*: `Mesher_3::forced_stop` (`cgal/Mesh_3/include/CGAL/Mesh_3/Mesher_3.h:319-335`) and `Protect_edges_sizing_field::forced_stop` (`Protect_edges_sizing_field.h:198-215`) check an atomic stop flag and `maximal_number_of_vertices`, and set `CGAL_MESH_3_STOPPED` / `CGAL_MESH_3_MAXIMAL_NUMBER_OF_VERTICES_REACHED` (`cgal/Mesh_3/include/CGAL/Mesh_error_code.h:27-31`).
    - Optimizers take `time_limit` (`Mesh_global_optimizer.h:372-374`). The test harness uses 0 or 5 s (`test_meshing_utilities.h:156-188`).
    - Only `test/Mesh_3/surface_mesh_to_tetrahedral_mesh.cpp:96-213` exercises the stop path. It pre-checks the input for non-manifold vertices and self-intersection, picks `manifold` / `manifold_with_boundary` / `non_manifold` accordingly, sets `edge_min_size = diag*0.05/1e6`, wires SIGINT to the atomic stop flag, and exits with failure if interrupted.
    - No test asserts that the vertex cap fires.
  - *Min-size floors*: `*_min_size` is the user-facing termination floor for protecting balls (`Protect_edges_sizing_field.h:484-505,1137-1249`). It is tested metamorphically by `test_min_size_criteria.cpp`.
  - *Predicate robustness*: circumcenters are computed in double, verified with an exact `side_of_oriented_sphere`, and recomputed exactly if the check fails or the denominator is zero (`cgal/Triangulation_3/include/CGAL/Robust_weighted_circumcenter_filtered_traits_3.h:56-81`). Mesh_3 uses `Robust_intersection_traits_3` with an EPECK converter (`Robust_intersection_traits_3.h:1-40`). Alpha_wrap_3 forbids exact kernels because it bisects for Steiner points (`Alpha_wrap_3.h:194-196`). Corefinement computes intersection nodes in an exact kernel and throws `Self_intersection_exception` on degenerate faces (`internal/Corefinement/intersection_callbacks.h:392-397`) and `Triple_intersection_exception` (`intersection_impl.h:1335-1396`).
  - *Bounded loops with explicit failure*:
    - Snap rounding caps at 5 iterations and returns false with "no guarantee of validity" (`internal/triangle_soup_snap_rounding.h:263,545`, `autorefinement.h:1717-1722`).
    - `experimental::remove_self_intersections` caps at 7 steps with a visitor stop (`repair_self_intersections.h:2388-2459`).
    - Hausdorff stops when upper - lower <= error_bound (`cgal/Polygon_mesh_processing/include/CGAL/Polygon_mesh_processing/distance.h:1560-1612,1783`).
  - *Unbounded loops*:
    - `remove_almost_degenerate_faces` is `for(;;)`; it exits when its queues drain or a pass makes no progress, and returns false in the latter case (`repair_degeneracies.h:802-1124`).
    - Alpha_wrap_3's main loop is `while(!m_queue.empty())` with only a visitor hook (`Alpha_wrap_3.h:1272`, default `go_further()` true at `:99`).
    - Surface_mesher's loop is `while(!is_algorithm_done())` (`Surface_mesher.h:776-779`).
  - *Known hang*: "when get_default_random()::get_seed() = 1473902576 the last test (on trihole.off) does not terminate" (`test_pmp_repair_degeneracies.cpp:16-18`). The only guard is the 1200 s ctest timeout.
  - *Silent bad outputs in release*: Alpha_wrap_3 returns an empty mesh with only a `CGAL_warning_msg` when extraction fails (`Alpha_wrap_3.h:877-881`), and its postconditions compile out (`:809-811`). Degenerate-CC removal failure only prints a WARNING (`repair_degeneracies.h:2640`). Autorefine's self-check is debug-only (`autorefinement.h:1630-1639`).
  - *Refusals*:
    - Stitching uses exact `Equal_3` on points, so only bit-identical coordinates stitch, and it refuses stitches that would make a non-manifold edge (`stitch_borders.h:73-91,713`).
    - Hole filling returns empty for collinear or non-manifold holes.
    - CDT2 face triangulation falls back on collinear points, zero normal, non-planarity beyond bbox/4, a non-simple projection or constraint intersection, then goes to DT3 search, then to the cubic search (`cgal/PMP_Mesh_repair/include/CGAL/Polygon_mesh_processing/internal/Hole_filling/Triangulate_hole_polyline.h:1359-1611`, `triangulate_hole.h:210-218,766-798`).
    - Self-intersection reports a degenerate face as the pair (f,f) (`self_intersections.h:398-410`).
  - *Tolerance abuse*: too-large and too-small snapping tolerances are exercised but unchecked (`test_pmp_non_conforming_snapping.cpp:89-135`). Denormal-scale plane coefficients (1.67e-19, -2.22e-16) are used for degenerate clipping, which only has to stay a triangle mesh (`test_degenerate_pmp_clip_split_corefine.cpp:64-102`).
  - *NaN/inf*: no NaN or inf input test found anywhere in scope.

- **Fuzzing / mutation / property-based / metamorphic / differential / performance-regression**
  - *Fuzzing*: none found. There is no libFuzzer, AFL or mutation corpus.
  - *Randomized*:
    - Time-seeded nightly RNG with the seed printed (above).
    - Alpha_wrap_3 draws random alpha/offset in 2^[0,6] of the bbox, 2 draws per file (`test_alpha_wrap_3_mesh.cpp:103-110`), and random cavity seeds.
    - `orient_polygon_soup_test.cpp:31-37` flips faces with unseeded `std::rand`.
  - *Metamorphic*:
    - Mesh_3: re-refine idempotence; optimizer invariants; min-size no-op; with vs without edge_distance; determinism.
    - PMP: Hausdorff translation; orientation reversal (`test_does_bound_a_volume.cpp`, `orient_polygon_mesh_test.cpp`); the cavities orientation grid (`test_corefinement_cavities.cpp:93-122`); rotation invariance of the mesh kernel (`test_mesh_kernel.cpp:348-401`); pinch-injection round trip; constraint-API equivalence (`delaunay_remeshing_test.cpp:91-111`).
    - Alpha_wrap_3: re-wrap with doubled alpha (`test_AW3_multiple_calls.cpp:65-86`); its monotonicity assert is commented out (`:81`).
  - *Differential*:
    - EPICK vs EPECK, and Surface_mesh vs Polyhedron, across repair, self-intersection and corefinement (`test_corefine.cpp:85` compares subface counts).
    - Bounded vs sampled Hausdorff.
    - Non-manifold vs manifold Boolean output paths via `match_faces` (`test_corefinement_nm_bo.cpp:47-63`).
    - Forced fallback paths via compile flags: `triangulate_faces_hole_filling_dt3_test.cpp` and `..._all_search_test.cpp` re-include one test with `CGAL_TRIANGULATE_FACES_DO_NOT_USE_CDT2` and `CGAL_HOLE_FILLING_DO_NOT_USE_DT3`.
    - FP environment: `test_corefinement_and_constraints_nearest.cpp` recompiles with `CGAL_ALWAYS_ROUND_TO_NEAREST`.
  - *Performance and robustness benchmarks (manual, not CI)*:
    - Alpha_wrap_3 `Robustness/robustness_benchmark.cpp:18-110` has exit-code taxonomy 1-9: invalid input, not triangle mesh, non-manifold, borders, degenerate faces, self-intersections, not bounding a volume, not containing input, distance too large. `compute_robustness_benchmark_data.py:24-73` runs it under `/usr/bin/time -v` with a wall timeout, maps SIGSEGV/SIGABRT/SIGFPE, and records TIMEOUT and peak RSS.
    - Commit-vs-commit performance and quality charts (`benchmarking.sh:72-82`). They are charts only, with no thresholds.
    - Mesh_3 `benchmark/Mesh_3/benchmark_mesh_3.cpp` with XML/charting.

- **CPU/GPU or cross-platform consistency**
  - No GPU code.
  - Sequential vs TBB-parallel checks:
    - Mesh_3 runs both, with wider bands and a 4x Hausdorff slack in parallel (`test_meshing_utilities.h:403-410`).
    - Mesh_3 determinism is tested only with Parallel_tag at 1 thread (`test_meshing_determinism.cpp:164-166`).
    - Hausdorff parallel must equal sequential (`test_hausdorff_bounded_error_distance.cpp:1072`).
    - Autorefine and snap rounding expect the same stored counts under both tags.
    - Self-intersection `maximum_number` is exact sequentially and only a lower bound in parallel.
  - Cross-platform consistency is implicit: nightly runs on multiple OS/compiler containers against the same banded counts. There is no cross-machine byte comparison.

- **Known-bad / expected-failure handling**
  - Disabled tests:
    - `test_pmp_repair_self_intersections` is commented out of the build (`cgal/PMP_Mesh_repair/test/PMP_Mesh_repair/CMakeLists.txt:30,56-57`) and its body is `#if 0`.
    - The Mesh_3 polyhedral-complex verify is commented out.
    - The naive Hausdorff reference ("takes too long") and elephant-vs-shark are disabled (`test_hausdorff_bounded_error_distance.cpp:1150-1164`).
    - Alpha_wrap_3 skips `overlay_triangulation.off` in multiple_calls (`test_AW3_multiple_calls.cpp:150`).
    - The DT hole-filling mode skips hole1.txt (`triangulate_hole_polyline_test.cpp:220`).
    - Polyhedron cannot read torso.off, so that case is Surface_mesh-only (`test_pmp_manifoldness.cpp:297`).
  - Expected failures as data:
    - Per-operation 0/1 flags in `test_corefinement_bool_op.cmd`.
    - The `Triple_intersection_exception` flag column in `test_autorefinement.cmd`.
    - `nested_cubes_invalid_volume.off 0` in `test_does_bound_a_volume.cmd`.
  - Manual-only full suites: `test_corefinement_bool_op_full.cmd` and `test_snap_rounding_full.cmd` (cubes.off, 26k triangles) run only through `run_test_corefinement_bool_op_full.sh`.
  - Tolerance whitelists:
    - Mesh_3 count bands; volume +/-5%; Hausdorff 1.01x sequential and 4x parallel; dihedral angle `1+1e-15`.
    - Hausdorff tests use error_bound 1e-4.
    - Shape smoothing is checked to 1e-10.

- **Worth adopting for BRepGPU**
  1. **Alpha_wrap_3 `validation.h` predicate stack** as the watertightness oracle for each tessellated solid: closed, `does_bound_a_volume`, no degenerate faces, no non-manifold vertices or edges, no self-intersection, plus a spec-derived bounded-error Hausdorff bound (alpha+offset there; chord tolerance for us). Add the non-manifold-edge check that it admits it lacks.
  2. **Bounded-error Hausdorff harness** (`test_hausdorff_bounded_error_distance.cpp`): analytic synthetic cases, translation metamorphic, symmetric = max, bounded vs sampled. Its two-sided upper/lower-bound subdivision (`distance.h:1560-1612`) is the right model for an offline "deviation vs exact geometry" oracle with a provable error bar.
  3. **Construct-then-verify-exactly circumcenters** (`Robust_weighted_circumcenter_filtered_traits_3.h:56-81`). This applies directly to parameter-space Delaunay refinement: Steiner points of skinny triangles near poles or seams, computed in float on the GPU, verified with an exact in-circle predicate, recomputed exactly on failure.
  4. **Protecting-ball adjacency property** (`test_meshing_polylines_only.cpp:145-189`) and the acute-corner terminal rule (`polylines_to_protect.h:336-340`). These form an oracle and a termination mechanism for trim-curve discretisation where trim loops meet at small angles, which is the classic infinite-refinement case for CDT.
  5. **Min-size no-op metamorphic** (`test_min_size_criteria.cpp:111-129`), and re-refine idempotence with a pass cap (`test_meshing_utilities.h:131-144`). These are cheap termination and stability tests for any refinement loop.
  6. **Correct-output-or-explicit-refusal encoded as data**: per-case expected success flags (`test_corefinement_bool_op.cmd`), expected-exception flags (`test_autorefinement.cmd`), must-return-empty cases (collinear and null-edge hole polylines), and snap rounding's capped-iterations-then-false contract. Add the Alpha_wrap_3 robustness exit-code taxonomy plus the timeout, signal and peak-RSS runner for the ABC sweep.
  7. **Rounding-induced-invalidity fixtures** (`data-snap_rounding/intersection_when_rounded_*.off`, `collapse_*`). Exact-valid output becomes self-intersecting after rounding to double. This is the same hazard as emitting float32 vertices from GPU tessellation of near-tangent trims. Pair them with the `CGAL_ALWAYS_ROUND_TO_NEAREST` recompile pattern to test FTZ, fast-math and FMA variants.
  8. **Compile-flag path forcing** (triangulate_faces CDT2 -> DT3 -> cubic): force every fallback tier of our trimmed-face triangulator through the same oracle. The CDT2 guards (planarity, simple projection, non-collinear, no constraint intersection; `Triangulate_hole_polyline.h:1359-1490`) are a ready checklist for when parameter-space CDT must refuse.
  9. **Shared-vertex-by-index vs duplicated-geometry self-intersection convention** (`self_intersection_triangle_soup_test.cpp:190-238`) and exact-equality stitching (`stitch_borders.h:73-91`). Seam and shared-edge vertices must be emitted bit-identical or index-shared, or the watertight check will (correctly) flag them.
  10. **`meshes/degeneracies/*` (16 files) and the Alpha_wrap_3 defect set (17 files)**, adapted as post-tessellation inputs to our validators. They test the validators themselves, which needs known-bad meshes with exact expected verdicts.
  11. **Avoid** CGAL's weak patterns:
      - tests that need a missing `.cmd` or a missing file and pass vacuously;
      - validity asserts skipped when preprocessing fails;
      - dead asserts behind constant flags;
      - warn-only regressions;
      - a documented non-terminating seed with no pinned regression test.

#### Blind spots this project reveals
- **Rounding of an exactly-correct mesh to output precision** can create self-intersections or collapses (snap-rounding fixtures). The bootstrap plan checks the mesh, but not "exact-valid becomes float-invalid" as its own family.
- **Validator self-tests**: we need known-bad meshes with exact expected verdicts (degeneracy counts, stitch refusal, soup-vs-mesh classification). Otherwise a buggy watertight checker passes everything.
- **Output-encloses / no-intersection-with-reference** (Alpha_wrap_3 `is_outer_wrap`): one-sided containment and mesh-vs-exact-surface crossing, not just deviation magnitude.
- **Idempotence and re-run stability**: re-tessellate or refine again and expect no change. In-process byte determinism across sequential and parallel-1-thread (our analogue: CPU vs GPU with identical scheduling). Seed-randomized nightly runs with the printed seed.
- **Termination floors as a tested contract**: min-size floors, acute-corner terminal vertices, vertex caps with error codes, a cooperative stop flag. Our plan has "termination within bound" but no metamorphic test that the floor actually stops refinement, and no test that the cap fires and is reported.
- **FP-environment recompiles** (round-to-nearest assumption, and for us FTZ, fast-math and FMA contraction) as separate test configurations.
- **Fallback-path forcing**: each degraded triangulation path must be exercised under the same oracle, not only the fast path.
- **Sequential-vs-parallel tolerance policy**: CGAL tolerates 4x Hausdorff slack in parallel. Decide explicitly whether GPU output may differ from CPU, and by how much.

### Cluster 3: Classic meshers and robust predicates: Triangle, Shewchuk predicates, TetGen, georust/robust, geogram

Scope: Triangle 1.6, Shewchuk predicates.c, TetGen 1.6.0, georust/robust, geogram.
All paths are relative to `quarantine/sources/`.
Everything here was read, not run. No project was built or executed. The only tool
run on a PDF was `pdftotext`, on the retained Shewchuk 1997 paper
(`papers/pdfs/shewchuk-1997-robust-predicates.pdf`, sha256 5f047fd4...), with its
text output written to the scratchpad.

---

#### Triangle (1.6, netlib, acquired 2026-10-17)

- **Test inventory** -- There is no test suite. The distribution is
  `README, triangle.c, triangle.h, tricall.c, showme.c, makefile, A.poly`
  (`triangle/README:55-61`). `tricall.c` is a sample caller; it has no
  assertions or checks (a grep for check/assert/verify in it finds nothing). `A.poly` is one
  sample PSLG with no expected output. The checking that does exist is built into the program:
  - Final-mesh topology and orientation check `checkmesh()` (`triangle/triangle.c:6694-6781`):
    it fails a triangle when exact `counterclockwise()` <= 0 (flat or inverted, `:6727-6732`), when a neighbor
    bond is not reciprocated (`:6737-6748`), or when the two triangles on an edge disagree about its
    endpoints (`:6751-6760`).
  - Final-mesh constrained-Delaunay check `checkdelaunay()` (`triangle/triangle.c:6793-6886`): it runs
    an exact `nonregular()` (incircle) test on every interior edge pair except edges that carry a subsegment
    (`:6829-6850`), and skips edges that touch the bounding "infinite" vertices.
  - Both run only under `-C` (`triangle/triangle.c:15996-15999`). `-C` turns exact arithmetic
    back on even if `-X` was given (`:6711-6712`, `:6813-6814`, doc `:1931-1935`).
  - A compile-time `SELF_CHECK` (`triangle/triangle.c:224-230`) adds in-algorithm invariant checks:
    no flip on a boundary or segment (`:7938-7952`), no clockwise triangle before or after an edge
    insertion (`:8422-8454`, `:8716-8744`), and a vertex being deleted must have degree >= 3 (`:8983-9000`).
    One check is commented out at `:8724-8733`, with the note "constrainededge() sometimes generates
    inverted triangles that insertvertex() removes". So the invariant is allowed to be violated for a while
    in the middle of an operation.
  - `-V` prints statistics: predicate call counts (`:712-716`, `:5253`, `:5905`) and histograms of
    angle and aspect ratio (`quality_statistics`, `:15342`; doc `:2942-2946`).
- **Oracles** -- `checkdelaunay` is tier 5: a universally quantified local-Delaunay property, evaluated
  with exact predicates, and locally Delaunay everywhere implies globally CDT. `checkmesh` is tier 5 for
  orientation and adjacency. Both are weak in three ways:
  (a) They only print. They return `void` and the exit status is unchanged
  (`:6771-6779`, `:6873-6884`), so a script must grep stdout for "!!".
  (b) Nothing checks that every input segment is present as a chain of subsegments, i.e. that the constraints
  were recovered.
  (c) Nothing checks that the `-q` angle bound or `-a` area bound actually holds in the output. The
  `-V` histograms are tier 1 (shown to a human).
  `SELF_CHECK` is tier 2 or 5 depending on the check, and most of its failures print and keep going.
- **Corpora** -- None. `A.poly` is a single input of 1045 bytes. The paper corpus (see the predicates section) is not shipped.
- **Pathology handling** (runtime behavior, from source):
  - *Duplicate vertices*: after sorting, exact-equal duplicates are dropped with a warning and marked
    `UNDEADVERTEX` (`:9997-10010`; also `:10245-10249`, `:10857`, `:10906`). The output numbering
    then changes (doc `:3103-3106`).
  - *Coincident segment endpoints*: a zero-length segment is dropped with a warning (`:12536-12541`).
    A segment with an invalid endpoint index is dropped with a warning (`:12521-12532`). Under `-Q`
    these warnings are suppressed, so the input is changed silently. This is a "silent bad mesh" path
    that our plan forbids.
  - *Segment intersections*: an inexact intersection point is constructed (`:11720-11738`). If
    `denom == 0`, i.e. the segments are parallel, Triangle calls `internalerror()` and exits (`:11727-11731`). The
    docs warn that three segments crossing at one point produce several nearby points and can
    "fail to terminate at all" (`:2286-2298`). Inexact intersection roundoff "might actually lead to an
    inverted triangle and an invalid triangulation" (`:2969-2976`).
  - *Precision exhaustion*: if a segment split point rounds onto an endpoint, Triangle prints
    "Ran out of precision" and calls `triexit(1)` (`:13403-13413`). If a circumcenter lands on an
    existing vertex, it only warns: "You can be sure of this if I fail to terminate" (`:13575-13597`).
    One step of iterative refinement makes split points more collinear, and has a NaN guard
    (`multiplier == multiplier`, `:13378-13391`). That NaN guard is the only NaN handling in the file.
    Input coordinates are never checked for NaN or Inf.
  - *Termination*: refinement is mathematically guaranteed to terminate for min angle <= 28.6 degrees
    "assuming infinite precision arithmetic" (`:1645-1655`). Termination with small input angles comes from
    the concentric-shell segment splitting (`:13278-13292`, `:13346`) and from the Miller-Pav-Walkington
    rule that leaves skinny triangles at small input angles unsplit (`:7313-7322`). The only resource bound
    is `-S` (Steiner-point cap: `:1876-1891`, loop guards `:13264-13267`, `:13662`). There is no
    wall-clock or iteration bound. If `-S` runs out under `-D`, Triangle only prints a warning (`:13683-13697`).
  - *Small-angle caveat*: the output can contain angles below the bound "between input segments
    separated by small angles". The max angle is still <= 180 - 2*theta (`:2870-2885`).
  - *Precision and range*: the predicates are claimed correct only when every input exponent is in
    [-148, 201] (`:2990-2997`). Underflow "can silently prevent" exactness and overflow raises a
    floating exception. To keep precision, put the origin inside the data (`:3030-3036`).
  - *Memory*: `trimalloc` prints "Out of memory" and calls `exit(1)` (`:1428-1435`). `triexit` is
    `exit()` even in `TRILIBRARY` builds (`:1411-1418`), so a library caller cannot recover.
  - *Exact-arithmetic toggle*: `-X` turns off adaptive exactness (`:1831-1846`, `:2951-2964`). The
    docs say it is about 8% faster and has "rare difficult inputs (having many collinear and cocircular
    vertices)" where it errs.
  - *x87*: `exactinit()` sets the FPU control word to 53-bit precision under `-DLINUX`/`-DCPU86`
    (`:4893-4911`). The makefile default is `-DLINUX` (`triangle/makefile:44-45,76`). `INEXACT volatile` is
    the fallback (`:255-273`).
- **Fuzzing / mutation / property-based / metamorphic / differential / performance-regression** --
  None found. There are three independent Delaunay algorithms (default divide-and-conquer; `-i` incremental;
  `-F` Fortune sweepline; `-l` vertical cuts only: `:1519-1522`, `:1897-1910`), and the docs suggest
  trying `-i` "if the divide-and-conquer algorithm fails". That is a ready-made differential oracle, but
  nothing in the archive uses it.
- **CPU/GPU or cross-platform consistency** -- None. The only portability guidance is the x87 and
  extended-register warnings (`triangle/README:93-111`).
- **Known-bad / expected-failure handling** -- None. The troubleshooting docs (`:3000-3165`) list
  known failure modes in prose: vertices lying on segments, long collinear vertex rows on one segment,
  intersections close to vertices, nearly collinear hull vertices under `-q` with `-c`.
- **Worth adopting for BRepGPU**
  - `checkmesh` plus `checkdelaunay`, with exact predicates switched on for the check even when the run
    used a fast path (`:6711`), as a tier-5 offline oracle for our parameter-space CDT. Add the two checks
    Triangle lacks, constraint-edge presence and the angle/area bound, and make a violation fail the run.
  - Triangle's troubleshooting prose (`:2266-2305`, `:3016-3165`) as a family of adversarial inputs for
    trimmed-region CDT: three segments crossing at one point; a vertex on a segment interior; a long
    collinear row covered by one segment; segments meeting at a tiny angle; nearly collinear hull vertices;
    coordinates in a narrow band far from the origin (6247133..6247134, `:3034-3036`). Each one maps directly
    to trim loops (seam vertices, near-tangent trims, offset parameter domains).
  - Triangle as an independent model (tier 6) for our CPU CDT on parameter-space PSLGs. Run it `-pC`, so its
    own checks run, never `-X`, with `-S` as a hang guard.
  - The "Ran out of precision" refusal (`:13403-13413`) as a pattern: detect that a constructed point
    collapsed onto an existing vertex and refuse explicitly, instead of looping.
  - Treat Triangle's silent changes to the input (dropped duplicates and zero-length segments) as an
    anti-pattern. For us these must be explicit, reported repairs or refusals.

#### Shewchuk predicates.c (May 18 1996 header; quarantine copy acquired 2026-10-17)

- **Test inventory** -- The file has no test driver and no `main` (4262 lines). Its test-supporting
  machinery is:
  - Non-adaptive exact reference versions of every predicate: `orient2dexact/slow`
    (`shewchuk-predicates/predicates.c:1446,1491`), `orient3dexact/slow` (`:1709,1790`),
    `incircleexact/slow` (`:2389,2491`), `insphereexact/slow` (`:3346,3603`). The header says these are
    "for timings" (`:56-61`, `:1419-1420`). In practice they are model oracles for the adaptive versions.
  - Random generators with a random significand and random exponent: `doublerand` (exponent in [0,511]),
    `narrowdoublerand` ([0,7]), `uniformdoublerand`, and float versions (`:491-620`). Bit printers
    `doubleprint`/`expansion_print` (`:372-486`).
  - `exactinit()` (`:643-683`) measures `epsilon` and `splitter` at runtime by halving until
    `1.0 + epsilon == 1.0`. It stops if the sum equals the previous sum, which catches round-up machines
    (`:654-667`). It does not report a non-IEEE machine and does not set the FPU control word (Triangle's
    and TetGen's copies do). It does not check for extended precision. On an x87 register machine the loop
    can quietly measure the wrong epsilon.
  - The test approach documented in the paper (local PDF, read via pdftotext). Timings use "uniform
    random", "geometric random", and "nearly collinear/coplanar/cocircular" inputs: two random points, then a
    third chosen to be approximately degenerate. Each timing averages 60+ inputs, 10,000 iterations per
    predicate call (paper Tables 2-5, text near "Table 2: Timings for ORIENT2D"). Application tests run
    Triangle and Pyramid on uniform random points, points near a circle or sphere, and a tilted square or
    cubic lattice, "chosen for their nastiness". The lattices are tilted with approximate arithmetic so the
    exponents vary (paper section 4.5). With approximate arithmetic, the 3D program "failed to terminate on the
    tilted grid": point location looped forever on a mesh made inconsistent by roundoff (paper section 4.5,
    Table 9). Correctness is argued by proof (the theorems in sections 2-4), not by a test suite. The paper
    does not describe any comparison of adaptive against exact results over a corpus. So I do not claim one.
- **Oracles** -- No automated oracle ships. The `*exact` functions make a tier-6 model check
  possible (adaptive sign == exact sign). The paper's evaluation is performance (tier 1 for correctness),
  plus one observed failure of the non-robust variant (a hang, tier 2).
- **Corpora** -- None shipped. The paper's generators are partly in-file (`doublerand` etc.). The tilted
  grid generator is not shipped.
- **Pathology handling** -- Overflow and underflow are out of scope. The paper says the four predicates
  "will not overflow nor underflow" only when input exponents are in a stated range for IEEE double. The
  range symbol was lost in text extraction; Triangle restates it as [-148, 201] at
  `triangle/triangle.c:2990-2997`. NaN and Inf are not handled; `Absolute()` is a plain compare (`:145`).
  Zero is returned exactly for degenerate input once the exact stage is reached. The paper says
  determinants that get past stage C "are nearly always zero".
- **FMA / x87 / extended precision, and GPU ports** (from source and paper; my analysis is labeled):
  - The macros assume every operation is rounded separately to 53 bits, with round-to-even
    (`:148-160`, `:691-692`, `:936`; paper section 5). Extended registers break the tail computations, and
    double rounding of `volatile` values can produce errors that 53 bits cannot express (paper section 5).
    `INEXACT volatile` is the only in-file workaround (`:118-131`).
  - *My analysis, not stated in source*: implicit FMA contraction breaks `Split`
    (`:200-204`). `c = splitter*a; abig = c - a` contracted to `fma(splitter,a,-a)` drops the
    rounding of `c` that the algorithm depends on, so `ahi`/`alo` are wrong and `Two_Product_Tail` stops being exact. The
    `x - ahi*bhi` steps are products of half-width values. Those are exact, so contracting them is harmless. The
    stage-A error bounds (`ccwerrboundA` etc., `:670-682`) were derived for the unfused operation sequence.
    For CUDA, nvcc contracts mul+add by default (`--fmad=true`), so a port must compile with `--fmad=false`
    or use explicit `__dmul_rn`/`__dadd_rn`. Alternatively, rewrite `Two_Product` as `x=a*b; y=fma(a,b,-x)`,
    which is exact with hardware FMA. Apple Metal GPUs have no fp64, and Metal compiles with fast-math on by
    default. Both need verification against current toolchain docs before we rely on them. I did not check
    them in this archive.
  - TetGen's copy compiles predicates at `-O0` (`tetgen/makefile:24-26,54-55`), defending against
    "too smart" optimizers (`predicates.c:634-637` warns about this).
- **Fuzzing / mutation / property / metamorphic / differential / perf** -- None in the file. The paper has
  performance tables (tier 1 for correctness).
- **CPU/GPU or cross-platform consistency** -- None, beyond the warnings about extended precision and
  compilers.
- **Known-bad / expected-failure handling** -- None.
- **Worth adopting for BRepGPU**
  - A tier-6 harness for our GPU and CPU predicates: adaptive sign == `*exact` sign == a big-rational
    sign. Feed it `doublerand` (wide exponent), `narrowdoublerand`, and constructed near-degenerate inputs
    (two random points plus a third rounded onto their line). Most random inputs never leave stage A, so the
    near-degenerate family is the one that matters.
  - The tilted-lattice input (paper section 4.5) as a termination and robustness fixture for our CDT. It
    produced an actual infinite loop with non-robust predicates, which is exactly our "no hangs" fault class.
  - A startup self-test that checks `epsilon == 2^-53` and `splitter == 2^27+1` and verifies
    `Two_Product` exactness on known pairs, run on every target (CPU, each CUDA arch, Metal). This catches
    contraction or extended precision at build time, rather than through a silently wrong sign.

#### TetGen (1.6.0, wias-berlin tarball, released 2020-08-31)

- **Test inventory** -- No tests, no corpus, no CI. Files: `README, tetgen.cxx, tetgen.h, predicates.cxx,
  makefile, CMakeLists.txt, example.poly`. The built-in checks are:
  - `check_mesh(topoflag)` (`tetgen/tetgen.cxx:31731-31866`). It flags a tet with `orient3d >= 0`
    ("Inverted"/"Degenerated", `:31761-31769`), reciprocity of tet-tet bonds, edge/face/apex agreement,
    and "two identical tetra" (`:31793-31840`). It also checks for leftover marker flags (infected/marktested/
    facemarked/edgemarked), i.e. scratch state that escaped (`:31771-31782`, `:31846-31862`).
  - `check_shells()` (`:31873-32048`) checks the face ring at each subface edge, duplicated subfaces
    (`:31926-31934`), dead subsegment and subface pointers, and subface-tet bonds.
  - `check_segments()` (`:32060-32322`) checks tet-seg and seg-tet bonds, seg-seg chains at
    `FREESEGVERTEX`, and counts "missing segments" (unrecovered constraints, `:32242-32246`, `:32322-32324`).
  - `check_delaunay(perturb)` (`:32329-32391`) runs exact `insphere`, or `insphere_s` with symbolic
    perturbation, on each interior face, and skips subfaces (constrained). `check_regular(type)` (`:32404-32483`) does
    Delaunay or regular, with or without symbolic perturbation, using `orient4d`/`orient4d_s`.
    `check_conforming(flag)` (`:32493-...`) checks the diametral ball/lens of subsegments and subfaces, with
    `b->epsilon` rounding (`:32550`).
  - Dispatch: `-C` runs check_mesh, plus shells and segments for `-p`/`-r`. `-CC` adds check_delaunay
    (`:36480-36489`). check_regular and check_conforming are not reached from `-C`.
  - `predicates.cxx` adds `test_double()` (`tetgen/predicates.cxx:475-521`). It measures machine epsilon
    and checks that it equals 2^-52, and checks that the smallest positive number is 2^-1074 or 2^-1022.
    It is called from `exactinit` (`:584-587`).
- **Oracles** -- check_delaunay and check_regular are tier 5, using exact or symbolically perturbed
  predicates. check_mesh, check_shells and check_segments are tier 5 topology properties. The missing-segment
  count is the only constraint-presence check in this whole scope. Weaknesses:
  (a) Return values are discarded (`:36481-36488`). Like Triangle, results are only printed and the exit
  code is unchanged.
  (b) `test_double`'s pass flag is ignored (`predicates.cxx:586`). A non-IEEE result prints
  "[not IEEE 754 conformant] !!" and keeps going. Its `dstore()` (`:470-473`) is a plain function, not a
  `volatile` store, so after inlining it may not force rounding.
  (c) The `-q` radius-edge and dihedral bounds are never verified on output; `qualitystatistics()` only
  prints (`:32651`).
- **Corpora** -- `example.poly` only (one PLC). None in the archive.
- **Pathology handling** (runtime behavior):
  - *Explicit refusal taxonomy* (`tetgen/tetgen.h:2487-2525`): 1 out of memory, 2 internal error
    ("report this bug"), 3 self-intersecting input surface, 4 very small input feature (suggests `-T`),
    5 two very close facets (suggests `-Y`), 10 input error, 200 Steiner points on the boundary under `-YY`.
    `TETLIBRARY` builds `throw x` instead of `exit` (`:2489-2491`), so a library caller can recover.
    Call-site counts: about 120 `terminatetetgen(this, 2)` internal-error sites, 23 code-3, 1 code-4, 4 code-10.
    Many code-2 sites carry comments such as "to debug", "Check this case", "assert(0)". These are refusals
    that stand in for unhandled geometry.
  - *Self-intersection detection*: segment recovery refuses with code 3 when the segment path hits a vertex,
    a crossing subsegment, or a subface (`:14960-14998`). A new segment point near an existing segment vertex
    refuses with code 4, printing "Two segments are very close" (`:15305-15322`). A coincident point refuses with code 3 (`:15323-15330`).
    Under `-d` (diagnose), coplanar-facet merging is turned off (`:14688`). Failed segments and facets go to
    `skipped_segment_list`/`skipped_facet_list` (`:20440-20460`, `:21327-21331`), are written out
    (`out_intersected_facets`, `:36060-36080`), and the run exits non-normally with code 3 (`:36225-36239`).
    A clean `-d` run prints "The input surface mesh is correct" (`:36243-36248`).
  - *Tolerance `-T`* (default 1e-8, `tetgen/tetgen.h:816`; parse `tetgen/tetgen.cxx:3576-3589`): it is
    relative to the bounding-box diagonal (`minedgelength = longest * epsilon`, `:10786`). It is used to
    reject an all-identical, all-collinear or all-coplanar point set with code 10 (`:12034-12090`), and a trivial set (`:10781-10784`).
    It also drives about 30 "rounding" snaps (`:13443-13456`, `:18537-18636`, `:19555-19565`). When a near-vertex
    intersection is found, TetGen prints the exact `-T` value that would accept it (`:20262-20271`). That
    makes the tolerance a user-abusable knob: a looser `-T` hides features.
  - *Exact arithmetic*: `-X` turns off exact predicates and `-X1` turns off the static filter
    (`:3404-3410`). The static filters are derived from the bounding-box maxima at `exactinit`
    (`predicates.cxx:629-643`) and used in orient3d/insphere (`:2225-2228`). Symbolic perturbation is used
    for insphere and orient4d (`tetgen.cxx:5127-5212`).
  - *Termination safeguards*: `-S` caps Steiner points (default -1 = unlimited, `tetgen.h:799`;
    decremented at many sites, e.g. `tetgen.cxx:15299`, `:17681`). `-r#` caps element growth (`elem_limit`,
    `:29175-29178`, checked `:27165`, `:28021`, `:29053`). Optimization iteration caps are `opm.maxiter = 100`
    (`:19415`) and 30 (`:22959`), with fixed `maxiter` 2-3 loops (`:29278`, `:29471`, `:29582`). A
    nearly-coplanar 2-2 flip is allowed "only if all new faces are locally Delaunay. Otherwise, this routine
    may not terminate" (`:29981-29985`). There is no wall-clock bound.
  - *Quality bounds*: `-q` default radius-edge 2.0 and min dihedral 3.5 degrees (`tetgen.h:811-814`). `-o/`
    sets the optimization max dihedral, default 177 degrees (`tetgen.h:815`; parse `tetgen.cxx:3460-3500`).
    Faces that cannot be split are queued as "unsplit" instead of looping (`:28038-28046`). There is no
    termination theorem in the source; usage text shows `-pq1.414` as an example (`:3053-3057`).
  - *Duplicates*: "Point #%d is coincident with #%d. Ignored!" (`:12140`). `-M` turns off merging of
    coplanar facets and close vertices (`:3390-3403`).
  - *Memory*: allocation failures go to `terminatetetgen(NULL,1)` (`:82-99` and others).
- **Fuzzing / mutation / property / metamorphic / differential / perf** -- None found.
- **CPU/GPU or cross-platform consistency** -- None. One build inconsistency: the makefile compiles
  predicates at `-O0` (`tetgen/makefile:24-26`), but `CMakeLists.txt:5,9` compiles both files with the
  same flags. A CMake build therefore runs the predicates under whatever optimization the toolchain applies.
- **Known-bad / expected-failure handling** -- None. The `report_selfint_edge` diagnostics at the
  segment-recovery refusal sites are commented out (`:14970`, `:14976`, `:14989`), so those code-3
  refusals print only the generic message from `terminatetetgen`, with no location.
- **Worth adopting for BRepGPU**
  - The refusal taxonomy (`tetgen.h:2487-2525`) as a model for our explicit-refusal contract:
    distinct codes for self-intersecting input, a feature below tolerance, near-coincident facets, input
    error, and internal error. Library builds throw instead of exiting. Our refusals should also carry the
    location, which TetGen 1.6 no longer does.
  - The stale-marker checks in `check_mesh`/`check_shells` (`:31771-31782`, `:32023-32035`). GPU kernels
    that use per-element scratch flags should assert that every flag is clear after each phase.
  - The missing-segment count (`:32242-32246`) as a tier-5 "every constraint edge present" oracle. Our trim-
    loop CDT needs exactly this, and Triangle lacks it.
  - The `-T` hint (`:20262-20271`) as a warning sign: tolerance-coarser-than-features inputs should refuse,
    and never be "fixed" by loosening the tolerance automatically.
  - TetGen as a model or reference for a 3D watertightness check: tetrahedralizing our output surface with
    `-pd` checks that it does not self-intersect. Code 3 together with `out_intersected_facets` names the
    intersecting facets. Usable only offline, as a third-party oracle.

#### georust/robust (rev c2183a06cc4e68f24836f981acb08e28d4d5f3d3, crate 1.1.0)

- **Test inventory** (all in `robust-predicates-rs/src/lib.rs`, `mod test` at `:2234`):
  - Fixture sign tests for orient2d, incircle, orient3d and insphere: `test_*_fixtures` (`:2269-2465`)
    read `fixtures/{orient2d,incircle,orient3d,insphere}.txt`, 1000 lines each, and assert
    `res.signum() == fixture_sign.signum()`, reporting the line number on failure (`:2285-2296`).
  - Hand cases at the underflow edge: `test_orient2d` (`:2467-2492`) puts points at
    `(+-f64::MIN_POSITIVE, +-MIN_POSITIVE)` against the line (-1,-1)-(1,1), and expects 0 for the collinear ones and +-1 for the off-line ones.
    `test_orient3d` (`:2494-2536`) does the same with a plane. `test_incircle` (`:2538-2554`) and `test_insphere` (`:2556-2601`)
    include an exactly cospherical point that must return `== 0.0` (`:2600`).
  - Regression against the C reference: `test_issue48_a/b` (`:2603-2650`) take near-cocircular points
    (coordinates within about 1e-14 of the unit circle). They assert that the result is not the value the
    previous buggy port returned, and that it is exactly the `predicates.c` value (`-8.0140565430358e-30`,
    `7.226864249343135e-30`).
  - Fuzz: `fuzz/fuzz_targets/orient2d.rs` and `incircle.rs` (cargo-fuzz/libfuzzer, `fuzz/Cargo.toml`) feed
    arbitrary `[(f64,f64); 3|4]` and assert `result.is_finite()`.
  - Visual: `examples/predicate-map/main.rs` evaluates naive and robust orient2d/incircle over a 256x256
    grid of consecutive floats after 0.5 (`nextafter`) and writes a PNG (`examples/predicate-map/README.md`).
- **Oracles** --
  - Fixture tests: tier 6 by provenance. The signs come from the Nanevski et al. "Staged Geometric Predicates"
    (2001) test set, copied through mourner/robust-predicates (`lib.rs:2237-2239`). The model is
    another implementation, and the source of the sign is not re-derived in-repo. They are weak on degeneracy: every fixture sign is
    +-1 (orient2d 538/-462, incircle 496/504, orient3d 491/509, insphere 507/493 by my count), and there is
    not one zero case. The coordinates are random with wildly varying exponents (1e-3 to 1e33). These
    probably resolve at stage A, but nothing records which stage was reached.
  - The orient3d "symmetry" assert (`:2389-2390`) calls the same predicate with the same arguments. It is a
    tautology, effectively tier 1.
  - Hand cases: tier 7. The answers are analytic, including exact zero.
  - issue48: tier 3 golden, but the golden value came from the reference C model (tier 6 in origin). It checks
    the bit-exact value, not just the sign. It is the only test that would catch a port divergence at the
    near-degenerate stages.
  - Fuzz: tier 2 (an "is_finite" property). It does not compare signs against exact arithmetic. Also, the
    targets do not filter NaN or Inf inputs, which the algorithm does not support, so as written they would
    flag those as findings.
  - predicate-map: tier 1 (a human inspects a PNG). The expected image is fully specified (README: gray on
    the diagonal, black lower-left, white upper-right), so it could be tier 7, but nothing asserts it.
- **Corpora** -- 4 fixture files x 1000 cases (`fixtures/`, 148-359 KB each). Provenance: Nanevski et
  al. 2001, originally at `cs.cmu.edu/afs/cs/project/pscico/.../compiler1/test/` (`lib.rs:2239`); that original
  location was not read. The fixtures are hard because of exponent spread, not degeneracy.
- **Pathology handling** -- The constants are hardcoded instead of measured at runtime ("They should? be the
  same in all IEEE-754 environments", `lib.rs:49-64`). There is a deliberate deviation from predicates.c: the
  orient2d stage-A filter uses Ozaki et al.'s `THETA * |detleft+detright|`, with the underflow guard omitted
  (`lib.rs:88-100`). No test targets that filter boundary. There is no NaN or Inf handling. f32 inputs are widened
  exactly to f64 (`lib.rs:24-27`). `no_std` uses `ieee754` for `abs` (`Cargo.toml:17-21`). The fixture tests are
  `cfg(not(feature="no_std"))`, so the no_std configuration runs only the 8 hand tests.
- **Fuzzing / mutation / property / metamorphic / differential / perf** -- Fuzzing exists (2 targets,
  tier 2 oracle). CI does not run it. Differential testing is the fixtures plus the issue48 goldens. No
  metamorphic tests: permutation antisymmetry such as `orient2d(a,b,c) == -orient2d(b,a,c)` and translation
  invariance are not tested. No benchmarks.
- **CPU/GPU or cross-platform consistency** -- CI is `ubuntu-latest` only, `cargo build-all-features --examples`
  plus `cargo test-all-features` (`.github/workflows/rust.yml:15-22`). Examples are built but not run. There
  is one platform and no fuzz job. bors gates merges on "ci result" (`bors.toml`).
- **Known-bad / expected-failure handling** -- None.
- **Worth adopting for BRepGPU**
  - The issue48 pattern: pin bit-exact predicate values from a reference implementation on near-cocircular
    and near-collinear inputs. For a GPU port, value equality with the CPU reference at the
    near-degenerate stages is a much stronger test than sign agreement on random inputs.
  - The ulp-grid predicate map (`examples/predicate-map/main.rs`), turned into an assertion. The exact sign
    of `orient2d((12,12),(x,y),(24,24))` is `sign(y-x)` on that grid (tier 7). Run it on CPU, CUDA and Metal and
    require identical sign maps. This is a cheap cross-device consistency test.
  - The `MIN_POSITIVE` cases as underflow-boundary probes. Parameter-space trims of tiny faces next to huge
    ones (our 1e6:1 family) push differences toward this range.
  - Avoid three weaknesses: fixtures with no zero cases, a tautological symmetry assert, and fuzzers whose
    only oracle is is_finite. Our predicate fuzzer should compare against an exact rational sign and include
    constructed degenerate inputs.

#### Geogram (390b14d5b9df949c45d07d5af208700f6be3a616)

Test harness: Robot Framework suites (`geogram/tests/*.robot`) drive command-line programs through `geogram/tests/lib/VorpatestLibrary.py`. A test passes when the program exits with status 0 (`subprocess.check_output`, `geogram/tests/lib/VorpatestLibrary.py:336`); a failing test keeps its run directory for inspection (`:270-280`). No golden files, no stored counts, and no output diffing anywhere. Every oracle lives inside the C++ program, as a `geo_assert` or as a nonzero return.

- **Test inventory**
  - 2D CDT topology, with and without Delaunay, plus external-triangle removal. Inputs are 16 constraint files: `constraints_100_1..11`, `constraints_100K`, duplicated vs non-duplicated constraints on a triangle, and `constraints_finally_2/3`. See `geogram/tests/CDT_2d.robot:12-74` and `geogram/tests/CDT_no_Delaunay_2d.robot:82` (`delaunay=false`). The driver is `geogram/src/tests/test_CDT_2d/main.cpp:78-124`.
  - Symbolic perturbation (SOS) orient3d correctness on degenerate rays: `geogram/tests/Orient_3d_SOS.robot:12-18`, `geogram/src/tests/test_orient_3d_SOS/main.cpp:333-384`.
  - Expansion arithmetic exactness on a recorded failing binary64 case: `geogram/tests/ExpansionNT.robot:12`, `geogram/src/tests/test_expansion_nt_2/main.cpp:56-121`.
  - Surface self-intersection, internal-shell removal and resulting topology, on 13 meshes (cubes, cylinders, icosphere, a wing): `geogram/tests/Intersect.robot:13-58`.
  - Mesh booleans/CSG on 20 `.scad` inputs: `geogram/tests/CSG.robot:13-78`. Four more sit in `geogram/tests/CSGplus.robot:13-30`.
  - 3D/2D/periodic Delaunay and convex hull: `geogram/tests/Delaunay.robot:13-79`, `geogram/tests/DelaunaySequential.robot`.
  - Remeshing/reconstruction/tetrahedralization: the output is compared against its own input (`geogram/tests/Remesh.robot:11-56`, `geogram/tests/Tetrahedralize.robot:13-23`).
  - Malformed input refusal. There are 19 corrupted or truncated OBJ/OFF/MESHB/PLY/STL files, and each must make the program exit nonzero (`geogram/tests/FileInput.robot:12-67`). Bad command-line parameters must also be refused (`geogram/tests/CmdLine.robot:12-35`).

- **Oracles**
  - `test_CDT_2d` (tier 2, plus a weak tier 5). After insertion it calls `check_consistency()` before and after `remove_external_triangles` (`geogram/src/tests/test_CDT_2d/main.cpp:120-124`). That runs `check_combinatorics()`, which uses `Tcheck` to confirm no repeated vertex in a triangle and symmetric adjacency (`geogram/src/lib/geogram/delaunay/CDT_2d.h:1115-1129,1149-1153,1189-1192`). It also runs `check_geometry()`, but the empty-circle assertion in it is commented out, so it checks nothing (`geogram/src/lib/geogram/delaunay/CDT_2d.cpp:1118-1126`).
    - So the CI CDT tests never verify the Delaunay property. They also never check that every constraint is present in the output, or orientation, or that constraint-intersection points are correct. Debug builds add `geo_debug_assert`s on walk invariants (`geogram/src/lib/geogram/delaunay/CDT_2d.cpp:622-649`). CI runs Debug, and Debug enables `GEO_DEBUG` (`geogram/src/lib/geogram/basic/common.h:197-203`).
    - `check_edge_intersections` (a brute-force check that segment-crossing edges are exactly the queued ones, `geogram/src/lib/geogram/delaunay/CDT_2d.cpp:1156-1177`) exists but runs only under `CDT_DEBUG`, which is commented off (`:85-87`).
  - `test_orient_3d_SOS` (tier 7, and the strongest test here). Grid points with spacing 0.25 over [-0.25, 1.25]^3 are classified against a CSG "cube minus corner(s)" mesh. Each point gets ray-parity counting with `orient_3d_SOS`, using 26 ray directions of length 1024 with components in {-1, -0.5, 0, 0.5, 1}. The result is compared to an analytic box-membership `where_is` (`geogram/src/tests/test_orient_3d_SOS/main.cpp:333-384`).
    - The dyadic spacing makes rays pass exactly through mesh vertices and edges, and lie in face planes. Points exactly on the boundary are excluded (`UNDETERMINED == ONBORDER`, `:362-364`). The program exits nonzero if any count is wrong (`:384`).
  - `test_expansion_nt_2` (tier 7). It checks the identities `u == r + h` and `det(u,h,g) == det(r,h,g) == det(r,u,g)`, which must hold exactly in exact arithmetic. The inputs are hex-float literals taken "from the failing case" (`geogram/src/tests/test_expansion_nt_2/main.cpp:56-121`).
    - The bigint dyadic reference oracle (tier 6) is compiled out: `// #define WITH_BOOST` at `geogram/src/tests/test_expansion_nt_2/main.cpp:10`. Its implementation is `geogram/src/tests/test_expansion_nt_2/exact_dyadic.h`.
    - The older `geogram/src/tests/test_expansion_nt/main.cpp` only prints values and always returns 0 (tier 1). No `.robot` file runs it.
  - Intersect (tier 5, coarse). After `intersect remove_internal_shells=true`, `vorpastat out.meshb SPHERE` requires 1 connected component, 0 borders and Euler characteristic 2 (`geogram/src/bin/vorpastat/main.cpp:153-166`).
    - Weak points: `vorpastat` first runs `mesh_repair(M1, MESH_REPAIR_TRIANGULATE)`, which always dissociates non-manifold vertices (`geogram/src/bin/vorpastat/main.cpp:145`, `geogram/src/lib/geogram/mesh/mesh_repair.h:60-61`). More importantly, the output is not re-checked for self-intersections, and its geometry is not compared to the input.
  - Remesh/Reconstruct/Tetrahedralize (tier 6, very loose). `vorpastat` compares output to input by symmetric sampled Hausdorff distance with a pass threshold of 5% of the bbox diagonal (`geogram/src/bin/vorpastat/main.cpp:63-110`). A topology mismatch is only a warning (`:186-189`).
  - Delaunay 3D (mixed). Cases that pass `dbg:delaunay_verbose=true` (`geogram/tests/Delaunay.robot:15,19,27,31`) turn on a brute-force O(n·t) empty-sphere check over every tet/vertex pair, plus a combinatorics check (tier 5). See `geogram/src/lib/geogram/delaunay/delaunay_3d.cpp:132-135,227-229,1164-1190` and `geogram/src/lib/geogram/delaunay/parallel_delaunay_3d.cpp:2629-2631,2859-2867`. The 2D, fandisk and periodic cases pass no debug flag, so they are tier 2.
  - CSG (tier 2). The test is only that `geocsg` exits 0 (`geogram/tests/CSG.robot:78`). The output is never inspected.
  - FileInput and CmdLine (tier 7 for refusal). Each test expects `CalledProcessError ... non-zero exit status` (`geogram/tests/FileInput.robot:78-81`, `geogram/tests/CmdLine.robot:5`).

- **Corpora**
  - All mesh data comes from the separate repo `BrunoLevy/geogram.data`, checked out into `tests/data` (`geogram/.github/workflows/continuous.yml:37-41`, `geogram/.github/workflows/nightly.yml:33-37`). **It is not in the archive and was not read.** `geogram/tests/data` does not exist.
  - File counts below come only from the `.robot` references: CDT2d 16, Intersections 13, CSG 24, FileInput 19 corrupted or truncated, Small about 15.
  - What makes them hard can only be inferred from names. `constraints_100_*` and `constraints_100K` are presumably random crossing segments; `triangle_duplicated_constraints` covers duplicated constraints; `three_cubes`, `cucubes` and `rot_seven_cubes` cover coplanar/rotated overlaps; `cube_with_8_spheres` and `sphere_icosa` cover many curved intersections. None of this was verified.
  - The Orient_3d_SOS and ExpansionNT inputs are synthetic and live in source (fully read).

- **Pathology handling**
  - **Termination (tests).** Only `geogram/tests/CmdLine.robot:5` (10 s) and `geogram/tests/ConvexCell.robot:5` (15 min) set a Robot `Test Timeout`. CDT, Intersect, CSG and Delaunay have none and rely on the GitHub job limit. No test is designed to catch a hang.
  - **Termination (runtime).**
    - CDT2d caps loops "NASA programming style" at `10*nT()` iterations. The caps are inconsistent:
      - `Delaunayize_vertex_neighbors` warns and then assert-fails (`geogram/src/lib/geogram/delaunay/CDT_2d.cpp:659-671`).
      - `Delaunayize_new_edges` warns and **breaks out, then continues with a possibly non-Delaunay triangulation** (`:700-712`). That is a silent-degradation path.
    - The flip queue in `constrain_edges` asserts rather than flipping forever when the last remaining quad is non-convex (`:604-611`).
    - Point location has only a debug-build infinite-walk assert (`:765-767`) and asserts if a point lies outside the enclosing boundary (`:771-772`).
    - The outer `while(i != j)` loop in `insert_constraint` and the walk in `find_intersected_edges` have no cap (`:260-296`, `:404-415`).
    - Delaunay3d `locate` uses a `max_iter` countdown (`geogram/src/lib/geogram/delaunay/delaunay_3d.cpp:395,479`).
    - `mesh_remove_intersections` runs a fixed `max_iter` passes with no convergence test (`geogram/src/lib/geogram/mesh/mesh_surface_intersection.cpp:2290-2299`).
  - **Assert behaviour.** `geo_assert` throws `std::runtime_error` in release builds and aborts in `GEO_DEBUG` builds (`geogram/src/lib/geogram/basic/assert.cpp:61-65,114-120`). Release callers can therefore catch it and turn it into a refusal.
  - **Duplicates.** CDT insert detects a duplicate vertex (two zero orientations), returns the existing index and rolls back (`geogram/src/lib/geogram/delaunay/CDT_2d.cpp:182-198`). MeshSurfaceIntersection removes degenerate (`aligned_3d`) triangles, colocates vertices exactly (epsilon 0), and removes bad facets before intersecting. It then switches SOS to lexicographic mode (`geogram/src/lib/geogram/mesh/mesh_surface_intersection.cpp:245-254`; `geogram/src/lib/geogram/mesh/mesh_repair.h:202-204`).
  - **Non-exact CDT2d (the class under test).**
    - Its constraint-intersection points are computed in plain double with `t = det(D,V)/delta`, and `delta` is never checked for zero (`geogram/src/lib/geogram/delaunay/CDT_2d.cpp:1370-1376`).
    - It sets `exact_intersections_ = exact_incircle_ = false` (`:1301-1304`) and guards flips with `is_convex_quad`.
    - The exact variant, `ExactCDT2d`, uses rational intersection points built from the original constraint endpoints (`:1717-1760`) and caches orient2d results (`:1663-1704`). **No dedicated test drives ExactCDT2d.** It is exercised only indirectly through Intersect and CSG.
  - **Precision.**
    - PCK filters are generated, semi-static error bounds. They return `FPG_UNCERTAIN_VALUE`, falling back to the exact path, when magnitudes fall outside [5e-147, 1.7e153] (`geogram/src/lib/geogram/numerics/predicates/orient2d.h:42-66`).
    - In the open-source build (no proprietary geogramplus `exact_nt`), coordinates are `expansion_nt`. The source says exact_nt has "no risk of underflow / overflow", which implies the default does (`geogram/src/lib/geogram/numerics/exact_geometry.h:66-72`).
    - `ExactCDT2d::incircle` uses *approximated* lifted coordinates, so the result is a regular (weighted) triangulation rather than a strict Delaunay one. Flips therefore need an extra convexity check (`geogram/src/lib/geogram/numerics/exact_geometry.cpp:275-322`, `geogram/src/lib/geogram/delaunay/CDT_2d.cpp:1534-1543`).
    - `scale_fast` does not check for overflow (`geogram/src/lib/geogram/numerics/multi_precision.h:1063`).
    - `expansion::initialize` is Shewchuk's exactinit loop. It computes epsilon and the splitter but does not assert they match binary64 (`geogram/src/lib/geogram/numerics/multi_precision.cpp:550-576`).
  - **NaN/Inf.** No `isnan`/`isfinite` guard exists in `predicates.cpp`, `multi_precision.cpp` or `expansion_nt.cpp` (searched). With NaN, every filter comparison is false, so control falls to the exact path. No test covers NaN.
  - **FMA/x87.**
    - Every platform file adds `-ffp-contract=off` (for example `geogram/cmake/platforms/Linux-gcc.cmake:40-43`, `geogram/cmake/platforms/Darwin-clang.cmake:47-48`).
    - `GEO_FP_CONTRACT_OFF` becomes a pragma on clang and MSVC. On GCC it becomes a static-init **runtime self-test**: it computes `a*a+b` with `a=0x1.0000002p0, b=-0x1.0000004p0`, which is nonzero only if the compiler fused it, and asserts if so (`geogram/src/lib/geogram/basic/numeric.h:419-452`). The macro is applied at `geogram/src/lib/geogram/numerics/predicates.cpp:46` and `geogram/src/lib/geogram/numerics/multi_precision.cpp:46`.
    - Explicit `fma()`-based `two_product` is used only under `FP_FAST_FMA` (`geogram/src/lib/geogram/numerics/multi_precision.h:162-171`, `geogram/src/lib/geogram/numerics/multi_precision.cpp:192-199`).
    - x87 is avoided on 32-bit Linux with `-mfpmath=sse`, commented "In the default 387 mode, rounding is unpredictable" (`geogram/cmake/platforms/Linux32-gcc/config.cmake:7-8`).
    - `interval_nt` switches rounding mode with `fesetround` (`geogram/src/lib/geogram/numerics/interval_nt.h:77-90`).

- **Fuzzing / mutation / property-based / metamorphic / differential / performance-regression**
  - Fuzzing, mutation, property-based and metamorphic testing: none found. The corrupted-file refusal list in FileInput is the closest thing to fuzzing, and it is hand-made.
  - Differential: the Orient_3d_SOS test (SOS ray-parity vs analytic membership); `test_expansion_nt_2` against the boost dyadic reference, which is disabled; and one Delaunay case run through the bundled Triangle (`geogram/tests/Delaunay.robot:49-51`), but its output is not compared.
  - Performance: `dbg:delaunay_benchmark=true` logs tets/second (`geogram/tests/Delaunay.robot:67`), but nothing asserts on it. The CGAL benchmark in `geogram/src/tests/test_expansion_nt/CGAL/README.txt` is manual. No performance regression gate.
  - Coverage: the nightly job runs gcovr (`geogram/tools/runpybot.sh.in:26-36`).

- **CPU/GPU or cross-platform consistency**
  - No GPU code.
  - The smoke CI runs on every push to main across {ubuntu-26.04, macos-15, windows-2025-vs2026} x {Debug, Release} x {Standard, Gargantua (64-bit indices)} (`geogram/.github/workflows/continuous.yml:15-17,78-79,98-99`).
  - Nightly runs Linux-only valgrind and gcov (`geogram/.github/workflows/nightly.yml:14-17,73-77`). Emscripten is built but never tested (`geogram/.github/workflows/emscripten.yml`).
  - Nothing compares outputs across platforms. Consistency is only "each platform passes its own asserts".

- **Known-bad / expected-failure handling**
  - Expected failures are written as `xfail ...` test names wrapped in `Run Keyword And Expect Error` (`geogram/tests/FileInput.robot:12-81`, `geogram/tests/CmdLine.robot`).
  - Several things are disabled without saying so:
    - `geogram/tests/CSGplus.robot` carries only the tag `CSGplus`, which no workflow includes.
    - `geogram/tests/Vorpaline/*.robot.vorpaline` and `geogram/tests/Exploragram/*.robot.exploragram` have non-`.robot` extensions, so Robot does not collect them.
    - Suites tagged only `daily` (RVD, NL, BNN/CNN_Search) run only for the cases that also carry `daily_valgrind`.
    - `weekly_valgrind` cases (fandisk) have no weekly workflow.
  - **Valgrind does not gate.** The wrapper writes XML (`.memcheck`) without `--error-exitcode` (`geogram/tests/lib/VorpatestLibrary.py:78-85`). ValgrindCI is installed but never invoked (`geogram/.github/workflows/nightly.yml:49-51`), so leaks and invalid reads only show up as uploaded artifacts.
  - The pass/fail of a whole run is the existence of `TESTS_SUCCESS` (`geogram/tools/runpybot.sh.in:6-8`).
  - Removed or commented-out checks: the CDT Delaunay assert (`geogram/src/lib/geogram/delaunay/CDT_2d.cpp:1122`); the "number of charts is even" assert (`geogram/src/lib/geogram/mesh/mesh_surface_intersection.cpp:1206-1209`). The degenerate-triangle sanity check is debug-macro only, and its own comment admits "Actually this happens sometimes..." (`:763-790`).

- **Worth adopting for BRepGPU**
  1. **The `test_orient_3d_SOS` pattern** (`geogram/src/tests/test_orient_3d_SOS/main.cpp`). Pick a dyadic lattice of query points and rays aligned so they hit vertices, edges and coplanar faces exactly, and compare against an analytic oracle. For us: point-in-trimmed-region classification of a parameter-space grid against analytic trim loops (polygons, circles at dyadic radii), with grid lines through trim vertices and along trim edges. This attacks inside/outside decisions at trim degeneracies, the classic source of flipped or missing triangles.
  2. **The GCC FMA-contraction runtime probe** (`geogram/src/lib/geogram/basic/numeric.h:426-450`). nvcc contracts `a*b+c` into FMA by default (`--fmad=true`), and Metal's fast-math does the same. Port this probe as a GPU kernel self-test run once per build/device in the test harness. Our predicates either need `__fmul_rn`/`__fadd_rn` or must be written to use explicit `fma` in `two_product`, as `multi_precision.cpp:192-199` does.
  3. **Hex-float regression literals from real failures** (`geogram/src/tests/test_expansion_nt_2/main.cpp:56-80`), checked against algebraic identities that must hold exactly (tier 7) rather than against stored outputs. Combine this with geogram's disabled bigint dyadic oracle (`geogram/src/tests/test_expansion_nt_2/exact_dyadic.h`) as a tier-6 reference for our predicates.
  4. **The CDT2d input families** `triangle_duplicated_constraints` and `constraints_100K` (crossing constraints at scale). Fetch them from `BrunoLevy/geogram.data` (CDT2d directory). Strengthen the oracle well beyond geogram's own: brute-force empty-circle check for non-constrained edges, every input constraint present as a union of output edges, consistent CCW orientation, and exact verification of intersection vertices.
  5. **Bounded loops that refuse rather than degrade.** Copy the `10*nT()` cap idea (`geogram/src/lib/geogram/delaunay/CDT_2d.cpp:665,705`), but make every cap produce an explicit refusal. Geogram's `Delaunayize_new_edges` silently breaks out (`:707-712`). Each cap needs a test that forces it to fire.
  6. **Corrupted/truncated-file refusal tests** (`geogram/tests/FileInput.robot:12-67`). This is the template for STEP-reader refusal tests: invalid counts, truncation, bad indices, and the expectation of a nonzero exit (an explicit refusal, not a crash).
  7. **Brute-force geometry checker switchable from the command line** (`dbg:delaunay`, `geogram/src/lib/geogram/delaunay/delaunay_3d.cpp:132-135,1164-1190`). An O(n²) offline Delaunay/constraint verifier can be turned on per test without a debug rebuild.
  8. **ExactCDT2d design lessons.** Build intersection points from the original constraint endpoints rather than from derived vertices, which keeps expansion growth bounded (`geogram/src/lib/geogram/delaunay/CDT_2d.cpp:1727-1735`). Also note the explicit admission that the expansion-based incircle on constructed points is only approximately Delaunay (`geogram/src/lib/geogram/numerics/exact_geometry.cpp:310-322`). If we construct trim/trim intersection points, we must decide whether we need strict Delaunay or "valid CDT plus convexity", and test for whichever we choose.

**Blind spots this project reveals (geogram-specific)**
- **Constraint intersections with constructed (non-input) vertices.** Exact rational intersection points and the expansion growth or overflow they cause are a fault class of their own. Our plan's adversarial families (near-tangent trims) generate them, but no oracle checks the constructed point's exactness or its predicate consistency.
- **Compiler- or device-induced FMA contraction silently breaking exact predicates.** This needs a build/device self-test, not a geometry test. None of ABC, NIST, OCCT or fuzzing would reveal it reliably.
- **"Emergency exit" degradations.** Iteration caps that log a warning and return a plausible but invalid mesh (`geogram/src/lib/geogram/delaunay/CDT_2d.cpp:707-712`). A termination-within-bound oracle passes these. We need a property check after every capped loop, plus tests that force the cap.
- **Leak/memcheck results that do not gate.** Geogram runs valgrind but never fails on it. Our tier-2 sanitizer runs must use failing exit codes (`--error-exitcode`, ASan abort).
- **Expansion overflow/underflow at extreme coordinate ranges** (filter guard at 1e-147 and 1e153, and `scale_fast` unchecked). Our 1e6:1 scale family does not reach this. Add inputs near the exponent limits and subnormal parameter deltas.
- **Self-check code that is present but disabled.** Commented-out asserts make a "checks consistency" claim hollow. Audit that our oracles actually assert.

---

#### Blind spots this project set reveals (fault classes the bootstrap plan would miss)

1. **Checkers that do not fail.** Triangle `-C`, TetGen `-C/-CC`, TetGen `test_double`, geogram's
   commented-out CDT Delaunay assert, and geogram's non-gating valgrind all report a problem and still
   exit 0. Every oracle in our plan must change the exit status, and we need a test that plants a known
   violation and confirms the oracle fails on it.
2. **Missing constraint-presence check.** Triangle never checks that input segments survive, and geogram's
   CI CDT tests do not either. Only TetGen counts "missing segments". Our oracle list has "every constraint
   edge present", but only as a property phrase: it must be a concrete check on every CDT output,
   including subdivided constraints and Steiner points added at intersections.
3. **Silent input mutation.** Triangle drops duplicate vertices, zero-length segments, and segments with
   invalid endpoints, with only a warning (suppressed under `-Q`). TetGen merges coplanar facets and
   near-coincident points under the `-T` tolerance. "Correct output or explicit refusal" also needs "every
   input repair is reported", or the mesh can be valid but describe a different input.
4. **Constructed-point degeneracy.** Inexact segment-intersection points (Triangle, geogram `CDT2d`) can
   invert triangles or loop. Three segments through one point become three points. Neither
   deviation-vs-geometry nor watertightness checks catch an intersection vertex that is off both
   constraints by an ulp. Add an exact check that every constructed vertex lies on its parent constraints,
   or is represented exactly.
5. **Toolchain and device arithmetic.** FMA contraction (nvcc's default, Metal fast-math), x87 extended
   registers, and optimizer "simplification" can silently break exact predicates. TetGen defends with
   `-O0` in the makefile but not in its CMake build; geogram uses a runtime contraction probe. ABC, STEP,
   OCCT corpora and mutation fuzzing will not reliably expose this. It needs a per-build, per-device
   predicate self-test plus CPU/GPU sign-map equality (robust's ulp-grid map is a ready template).
6. **Predicate exponent range.** Shewchuk's guarantees hold only for input exponents in a bounded range
   (Triangle restates it as [-148, 201]). Geogram's filters fall back outside [5e-147, 1.7e153]. Our 1e6:1
   scale family is far inside this range. Parameter-space deltas on tiny trims near huge ones, subnormal
   differences, and NaN or Inf coming out of upstream evaluation need their own family. No project here
   validates NaN or Inf input.
7. **Degeneracy-free predicate corpora.** robust's 4000 fixtures contain no zero-sign case, and random
   inputs almost always resolve at stage A. Predicate tests must be built from constructed degenerate and
   near-degenerate inputs: the paper's "nearly collinear" construction, tilted lattices, ulp grids,
   and geogram's dyadic SOS rays.
8. **Termination by cap versus termination with validity.** The only bounds are on Steiner points (Triangle
   `-S`, TetGen `-S`/`-r#`) or iterations (geogram `10*nT()`, TetGen `maxiter`). At least one of these exits
   with a warning and a possibly invalid mesh (geogram `Delaunayize_new_edges`, Triangle `-D` out of Steiner
   points). Our "termination within bound" oracle must be paired with full property checks on the output
   that came back at the cap, and with tests that force each cap to fire.
9. **Tolerance-as-escape-hatch.** TetGen prints the exact `-T` value that would make a rejected
   near-vertex case pass. The tolerance-coarser-than-features family should assert refusal, and should
   also assert that no automatic tolerance loosening ever happens.
10. **Library-mode recoverability.** Triangle calls `exit()` even when built as a library, while TetGen
    (`throw`) and geogram (`geo_assert` throws in release builds) let the caller recover. We need tests that a
    refusal deep inside the mesher returns control to the caller with its state intact (no leak, no
    partial output marked valid), on CPU and on GPU.

### Cluster 4: 2D polygon and constrained-Delaunay triangulators: earcut, poly2tri, artem-ogre/CDT, spade, Blender, gCDT

Scope: earcut, poly2tri, artem-ogre/CDT, spade, Blender (selected files), gCDT. Everything was read only. Nothing was built or run.

Citation roots. Paths are relative to `quarantine/sources/` unless marked otherwise. Two abbreviations:
- `BL/` = `blender-selected/source/blender/blenlib/`
- `CDTr/` = `direct-cdt-study/CDT-53eff288a4e7/CDT/`. This one is relative to `quarantine/`, not `quarantine/sources/`, because the tree lives in `quarantine/direct-cdt-study/`.

---

#### earcut (mapbox/earcut v3.2.4, 9e70986eb94054c621c926888dd05bc26bffb902)

- **Test inventory**
  - Exact index output on trivial input, in 2D and 3D strides (`earcut/test/test.js:9-17`). Empty input (`:19`).
  - The main family is the 59 fixtures listed in `expected.triangles`, each run at rotations 0/90/180/270 (`earcut/test/test.js:100-149`). Fixture names mark the fault classes:
    - self-tangency: `self-tangent-1..4`
    - touching holes: `touching-holes1..6`, `hole-touching-outer`
    - shared points: `shared-points`
    - collinear diagonals: `collinear-diagonal`
    - degenerate rings: `degenerate`, `empty-square`, `issue83`, `issue107`
    - bowtie: `hourglass`
    - Steiner points as one-vertex holes: `steiner`
    - recorded hangs: `infinite-loop-jhl`
    - large real-world inputs: `water-huge3` (1444 rings, 12,864 verts)
  - Hang regression (`earcut/test/test.js:151-153`): a hard-coded ring with duplicate vertices plus a one-vertex hole. It only has to return.
  - Collinear-only polygon: must return 0 triangles and deviation 0 despite shoelace roundoff (`:155-160`).
  - `refine()` (Lawson-flip post-pass): fixes a bad quad diagonal (`:162-172`), leaves a good diagonal alone (`:174-182`), preserves a concave polygon (`:184-195`). It must terminate on near-cocircular points under a 5 s `{timeout: 5000}` (`:197-212`). It must leave zero non-Delaunay convex interior edges on `earcut.json` (`:214-223`).
  - 119,680-polygon MVT tile corpus: zero deviation before and after refine, same triangle count, perimeter ratio < 0.72 (`:225-292`).
  - Hole-bridge block-index regression (#183): a collinear-rich integer-grid outer ring with two holes, all four rotations (`:298-318`).
- **Oracles**
  - Triangle count at rotation 0 must equal `expected.json` (`earcut/test/test.js:126-128`). **Tier 3.** The numbers were recorded from earlier runs.
  - `deviation` is `|sum|tri area| - (|outer| - sum|hole|)| / polygon area` (`earcut/src/earcut.js:816-847`). It is checked against per-fixture thresholds (`test.js:130-132`). **Tier 5, and weak.** An area balance is necessary but not sufficient. Taking the absolute value of each triangle means an overlap and a gap of equal area cancel. Triangles that lie outside the polygon, or overlap each other, can still score 0. Orientation is never checked, and neither are T-junctions (the README says T-junctions are by design, `earcut/README.md:57-59`). When there are no triangles, the function returns 0 if polygon area <= `data.length * max^2 * EPSILON` and 1 otherwise (`earcut.js:841-845`). That is an implicit degeneracy tolerance.
  - The rotation loop is **tier 4 (metamorphic)**, but only the deviation bound carries across rotations. The count is checked only at rotation 0. Rotation uses exact integer matrices (`test.js:105-117`), so the coordinates do not change.
  - `countIllegalEdges` (`test.js:44-84`) is a **tier 5** check of the local Delaunay property. It uses non-robust float predicates, and the test's `inCircle` has no tie margin (`test.js:94-98`), unlike the library's (`earcut.js:995-1006`).
  - The near-cocircular refine test is **tier 1 to 2** (it terminates within 5 s) plus a tier 5 area check.
  - The MVT test asserts `polys.length == 119680` and perimeter < 0.72 of the base. **Tier 3.**
- **Corpora**
  - 59 JSON fixtures in `earcut/test/fixtures/`, from about 6 to 12,864 vertices. Sources: real GIS data (water, rain, building, simplified US border), GitHub issue repros (`issue*`), Eberly's paper examples (`eberly-3`, `eberly-6`), and a Hilbert curve (1,027 verts).
  - `earcut/bench/tiles-fixture.bin` (4.7 MB): packed-varint MVT geometry decoded into 119,680 polygons (`earcut/bench/tiles-fixture.js:4-17`). It was made by "a one-off, author-machine-specific script; regenerating it is intentionally out of scope" (`earcut/bench/bench-tiles.js:1-3`). MVT coordinates are integer-grid, so the corpus is rich in collinear and coincident points.
  - What makes these hard: self-touching rings, holes touching each other and the outer ring, repeated points, zero-area spikes, and holes outside the outer ring (`outside-ring`).
- **Pathology handling**
  - The README disclaims correctness: "does not guarantee a correct triangulation on arbitrary input... overlapping triangles, gaps, or triangles outside the polygon" (`earcut/README.md:46-56`). Earcut never refuses.
  - The runtime escalates when no ear is found: filterPoints (repeat while nodes are removed), then cureLocalIntersections once, then splitEarcut (`earcut/src/earcut.js:151-169`). If splitEarcut finds no valid diagonal, it returns with nothing pushed (`:249-272`). **The rest of the polygon is silently dropped.** That is a silent bad mesh.
  - Duplicate and collinear points are removed by exact `===` / `area()===0` (`earcut.js:103-124`, `:656-658`). There is no epsilon.
  - Predicates are plain double, with no robust orient. `refine`'s inCircle uses a Shewchuk-style tie margin `1e-13*s^2` so that Lawson flipping provably terminates (`earcut.js:995-1006`). That was the fix for issue #205's infinite flip loop.
  - There is no NaN/inf validation. z-order hashing quantizes to 32767 per axis (`earcut.js:68-69`).
- **Fuzzing / mutation / property / metamorphic / differential / perf**
  - Fuzzing, mutation, property-based testing, and differential testing against another library: none found.
  - Metamorphic: the 90-degree rotations above.
  - Performance: `bench/bench-tiles.js` reports the median of 5 runs, per-zoom breakdowns, and a position-weighted index checksum "drift signal" (`bench-tiles.js:28-37`). The checksum is printed, not asserted. `bench/bench-refine.js` does the same for refine. Neither runs in CI.
- **CPU/GPU or cross-platform consistency**: none. CI is ubuntu-latest, Node 20, `npm test` only (`earcut/.github/workflows/node.yml`). `pretest` runs eslint and tsc (`earcut/package.json` scripts).
- **Known-bad / expected-failure handling**
  - `expected.errors` is a per-fixture tolerance whitelist, and some fixtures are known-imperfect. `issue142` is allowed 13% area error. `bad-hole` is allowed 1.9%, and 3% under rotation. `water-huge2` is allowed 0.3%, and 3% under rotation (`earcut/test/expected.json:63-82`).
  - Fixtures with expected count 0 skip the deviation check (`test.js:130`).
  - The test prints an "improved" diagnostic when error drops below 1/3 of the threshold (`test.js:134-146`). That is a ratchet aid.
- **Worth adopting for BRepGPU**
  - Fixtures `self-tangent-*`, `touching-holes*`, `hole-touching-outer`, `shared-points`, `collinear-diagonal`, `degenerate`, `hourglass`, `infinite-loop-jhl`, `filtered-bridge-jhl`, `bad-diagonals`. These are compact parameter-space trim-loop pathologies: loops touching at a vertex, a hole tangent to the outer loop, zero-area spikes. Convert them to our trim-loop input format.
  - `tiles-fixture.bin` as a speed and robustness corpus of integer-grid polygons with heavy collinearity. It is analogous to trims snapped to a parameter lattice.
  - Technique: an area-balance oracle is cheap, but it must be paired with orientation, pairwise-overlap, and edge-coverage checks, because area alone misses overlap-plus-gap.
  - Technique: a tie-margin on inCircle with a proven roundoff bound guarantees flip termination (`earcut.js:995-1006`). If BRepGPU ever runs float Lawson flips on GPU without exact predicates, this is the minimum.
  - Anti-pattern to avoid: the last-resort path that silently drops geometry (`earcut.js:249-272`). BRepGPU must refuse explicitly in that case.

---

#### poly2tri (jhasse/poly2tri 1008179917fd38c1824b8fe810f020fef0e5f6b5)

- **Test inventory** (Boost.Test, 9 cases total)
  - Triangle containment and circumcircle predicate (`poly2tri/unittest/TriangleTest.cpp:7-18`).
  - Triangle and quad (`poly2tri/unittest/main.cpp:16-48`).
  - Narrow quad, a former failure (`main.cpp:50-68`). Same data as `testbed/data/deadly_quad.dat`.
  - Concave-by-1e-15 boundary plus hole plus Steiner points. It used to throw "EdgeEvent - null triangle" (`main.cpp:70-113`).
  - Issue #10 polygons with near-equal x/y coordinates (`main.cpp:116-189`).
  - Polygon with hole (`main.cpp:191-224`).
  - Four testbed files (`main.cpp:226-263`).
- **Oracles**
  - `BOOST_CHECK_NO_THROW(Triangulate())` is **tier 2**.
  - Exact triangle counts (1, 2, 2, 18, 10, 11, 5, 13) are **tier 7** for the simple polygons, since n-2+2h is derivable. For ConcaveBoundaryTest's 18 with Steiner points the count is also derivable: 8 + 4 + 3 = 15 points, with h = 1. In practice these were recorded (**tier 3**).
  - `IsDelaunay(result)` (`poly2tri/poly2tri/common/shapes.cc:394-409`) is **tier 5**, but it is the wrong property. It is an O(n^2) unconstrained empty-circumcircle test over all triangle pairs, ignoring constraint visibility. It uses a plain-double strict `< 0` incircle (`shapes.cc:370-385`), so cocircular points pass. It is not applied to PolygonTest01-04.
  - TestbedFilesTest asserts only `result.size()*3 > polyline.size()` (`main.cpp:257`). **Weak.** It is roughly tier 2 to 3.
- **Corpora**: `poly2tri/testbed/data/` holds 22 `.dat` files and one `.dae`: bird (275 pts), nazca_heron (1,036), nazca_monkey (1,204), debug2 (10,000), sketchup, tank, dude, and others. **Only 4 are used by tests**: custom, diamond, star, test (`main.cpp:228`). The testbed binary is an interactive GLFW viewer with a `random <n>` mode that places Steiner points uniformly in a box (`poly2tri/testbed/main.cc:120-125`, `:314-330`). CI does not build or run it.
- **Pathology handling**
  - Documented as unsupported: repeat points "within epsilon", non-simple polygons, and holes touching each other or the boundary (`poly2tri/README.md:1-12`). There is no input validation.
  - Runtime: on exact duplicate endpoints, the `Edge` constructor throws `runtime_error("Edge::Edge: p1 == p2")` (`poly2tri/poly2tri/common/shapes.h:130-143`). Other failures also throw: "collinear points not supported" (`poly2tri/poly2tri/sweep/sweep.cc:137`, `:153`), "Opposing point on constrained edge" (`sweep.cc:811`), and null-neighbor throws (`sweep.cc:83`, `:120`, `:749`, `:819-830`). So the failure mode is an exception, which is good. But `assert(0)` remains in shapes.cc and advancing_front.cc (`shapes.cc:65`, `:157`, `:170`, `:230`, `:244`; `advancing_front.cc:87`), and those become UB in release builds.
  - `Orient2d` is plain double with exact-zero collinearity. The epsilon was deliberately removed because it "fails on concave-by-subepsilon boundaries" (`poly2tri/poly2tri/common/utils.h:66-80`). `InScanArea` still uses EPSILON = 1e-12 absolute (`utils.h:51`, `:117-128`), which is not scale-aware.
  - EdgeEvent and FlipEdgeEvent recurse with no depth bound (`sweep.cc:126-170`, `:772-777`), so stack exhaustion is possible on big inputs. There is no timeout test.
- **Fuzzing / mutation / property / metamorphic / differential / perf**: none found. The testbed random mode is manual.
- **CPU/GPU or cross-platform consistency**: CI runs a Fedora 32 container, builds with GCC and Clang, runs clang-tidy, then `ctest` (`poly2tri/.github/workflows/linux.yml:9-34`). Linux only. No sanitizers.
- **Known-bad / expected-failure handling**: no xfail mechanism. The 18 unused `.dat` files act as an informal "may fail" set. Not testing them is the only handling.
- **Worth adopting for BRepGPU**
  - `deadly_quad.dat` and NarrowQuadTest: a 1e-5 x 3e-7 sliver quad. This is a parameter-space aspect-ratio pathology.
  - ConcaveBoundaryTest: a boundary concave by 1e-15. It is a direct analogue of trim curves that are almost straight after tessellation (`poly2tri/unittest/main.cpp:70-113`).
  - PolygonTest01-03: mirror-symmetric coordinates that differ in the last few ULPs (`main.cpp:119-131`). Exactly the issue seen when trim loops are mirrored or periodic-shifted.
  - Lesson: an unconstrained `IsDelaunay` check is the wrong oracle for a CDT. BRepGPU's oracle must test the constrained property, meaning the empty circumcircle as seen across non-constraint edges only. It must use exact predicates.

---

#### artem-ogre/CDT (53eff288a4e77ad1c0afe1a10f60bd2b57f877b8)

Paths are relative to `quarantine/`. `CDTr/` = `direct-cdt-study/CDT-53eff288a4e7/CDT/`. This revision includes Ruppert refinement code and tests.

- **Test inventory** (Catch2; `CDTr/tests/cdt.test.cpp`, 2,291 lines; `CDTr/tests/predicates.test.cpp`, 1,494 lines)
  - Ground truth constrained triangulation (`cdt.test.cpp:483-596`).
    - Inputs: 28 input files.
    - Parameter matrix: {float, double} x {AsProvided, Auto insertion order} x {NotAllowed, TryResolve intersecting edges}, each compared at 4 erase stages (_all, _super, _outer, _auto).
    - Fault classes: hanging edges (Hanging, Hanging2, double-hanging, triple-hanging); boundary overlap (issue-42-*); overlapping constraints; points on constraint edges; a crack; corner cases; the wrong-hull regression (#38).
  - Ground truth conforming triangulation: 14 files (`cdt.test.cpp:598-686`).
  - Crossing edges, constrained and conforming: crossing-edges, issue-148 (`:714-752`).
  - Insertion in two batches must give the same topology as one batch (`:754-779`).
  - Regressions:
    - Don't flip a constraint while resolving an intersection (`:816-842`).
    - Hanging edge in a pseudo-polygon (`:844-872`).
    - debug2, 10,000 vertices and 10,000 edges (`:901-909`).
    - #154(1): y = 1e38 breaks the super-triangle (`:911-928`).
    - #154(2,3): loops in pseudo-polygons (`:930-988`).
    - #174: bounding box 1 ULP wide (`:999-1013`).
    - #204: vertex on a fixed edge, checking `pieceToOriginals` provenance (`:1451-1491`).
    - #212: float near-endpoint intersection (`:1493-1532`).
    - #211: near-degenerate, minDist 5e-17 (`:1534-1548`).
    - A duplicated constraint edge with intersecting constraints, "found by the fuzzing harness and minimized", which used to loop forever (`:1550-1579`).
    - #230: vertex 4e-17 inside a hull edge (`:1581-1598`).
    - A vertex near an edge but outside its diametral circle, which "would loop forever" (`:1600-1623`).
  - Callbacks, compiled only with `CDT_ENABLE_CALLBACK_HANDLER` (`:1013-1355`): exact counts of callback events (`:1015-1103`), and abort mid-insertion via `isAbortCalculation` (`:1105-1164`). Refinement reports vertex types (`:1166-1219`). Refinement inserts no encroaching circumcenter: a brute-force diametral-circle scan inside the callback (`:1221-1293`, `:1295-1353`).
  - Ruppert refinement:
    - Reaches the requested minimum angle and area (`:1625-1687`).
    - Scanning helpers `findEncroachedFixedEdges` and `findUnrefinedTriangles` (`:1689-1729`).
    - Zero threshold is a no-op (`:1731-1762`).
    - Respects the vertex budget (`:1764-1791`).
    - Concentric-shell split lengths at a 10-degree corner (`:1793-1846`).
    - Terminates near close parallel edges, with and without minEdgeLength (`:1848-1914`).
    - Rejects invalid split vertices without modifying the mesh (`:1953-2003`).
    - No flat triangle made from two pieces of one input edge (`:2052-2075`).
    - Circumcenter that lands on a vertex (`:2077-2101`).
    - Erase marks kept through refinement (`:2103-2200`).
    - Small-angle corner neither degenerates nor inverts triangles at 15/20/25/30 degrees (`:2202-2228`).
    - FinalizedError on misuse (`:2230-2262`).
    - Constrained Sweden refined, ground truth (`:2264-2291`).
  - KD-tree nearest neighbor against brute force, 1,000 random queries with mt19937(42) (`:1412-1449`). This one is tier 6.
  - Predicates (`CDTr/tests/predicates.test.cpp`):
    - The build's FP model preserves error-free transformations (`:296-338`).
    - Expansion arithmetic exact against int64 for 100k random pairs (`:340-385`).
    - Sign conventions and degenerate inputs (`:387-665`).
    - Argument-swap antisymmetry (`:667`).
    - Small-grid exact integer oracles for orient2d, incircle, and indiamcircle (`:728-891`).
    - Near-collinear ULP grid (`:1051`).
    - incircle over the full magnitude range (`:1087`).
    - Adaptive filters against an exact path on near-degenerate (`:1153`) and random Shewchuk-style inputs (`:1203-1249`).
    - float against double (`:1251`).
- **Oracles**
  - The "ground truth" files are **tier 3 golden files.** `topologyString(cdt) == topologyString(expected)` (`cdt.test.cpp:555-595`) compares a canonical text form. Triangles are rotated so the smallest index comes first, the triangle list is sorted, neighbors are remapped, and fixed edges, overlap counts, and pieceToOriginals are sorted (`:258-399`). Regeneration uses a compile-time `constexpr bool updateFiles = false` (`:23`). Flip it, rerun, commit. **Coordinates are not stored**, so the position of Steiner vertices created by intersection resolution is never checked. Separate `f32_`/`f64_` goldens exist for 5 files (`:516-527`), so output depends on precision.
  - `verifyTopology` (`CDTr/extras/VerifyTopology.h:31-75`) is **tier 5 but shallow.** It checks vertex-triangle incidence and neighbor-link symmetry only. The header says so: inverted and degenerate triangles "are topologically consistent, so #verifyTopology misses them" (`:77-79`). A test comment shows this happened in real life: "silently corrupting neighbor links... Topology stayed symmetric, so verifyTopology could not see it" (`cdt.test.cpp:1956-1960`). `verifyWinding` (strict CCW via orient2D, `VerifyTopology.h:80-95`) was added later and is used only in refinement tests. `eachVertexHasNeighborTriangle` (`:98-108`) is also tier 5.
  - `countNonDelaunayEdges` with the robust incircle (`cdt.test.cpp:663-686`) is **tier 5 with exact predicates.** It is used only in the conforming test "Conforming triangulation is Delaunay" (`:688-712`). The constrained-Delaunay property is **never** checked on the ground-truth corpus. It is checked only indirectly through the golden files.
  - Two batches against one batch is **tier 4.**
  - Refinement tests: `findUnrefinedTriangles(...).empty()` and `findEncroachedFixedEdges().empty()` are **tier 5**, but they are evaluated by the library's own functions, not an independent oracle. The encroachment callback test uses an independent dot-product test (`:1271-1283`), so it is **tier 5 with an independent oracle.**
  - Exceptions: `REQUIRE_THROWS_AS(..., InvalidEdgeSplitVertex)` (`:1520`, `:1547`, `:1578`) is **tier 7.** The spec says to refuse.
  - Predicates against exact integer arithmetic or an exact expansion path are **tier 6.** The Nanevski-Blelloch-Harper fixtures are **tier 7**.
  - Weak tests:
    - "Regression: multiple hanging edges" (`cdt.test.cpp:874-899`) is a verbatim copy of the previous test. It uses the same `HangingIntersection.txt` input and the same golden file, so it tests nothing new.
    - "Regression test" on debug2 (`:901-909`) and "hanging edge in pseudo-poly" (`:990-997`) check only verifyTopology.
    - Callback counts (`:1096-1102`) are tier 3.
- **Corpora**
  - `CDTr/tests/inputs/`: 44 input files plus `predicates/` (4 fixture files of 1,000 lines each, with a README of provenance). Largest: debug2 (10,000 verts), Constrained Sweden (2,619 verts and edges), island (95).
  - `CDTr/tests/expected/`: 562 golden topology files.
  - `direct-cdt-study/CDT-53eff288a4e7/visualizer/data/`: 39 files for the interactive viewer. Only some overlap with tests. Sweden, LakeSuperior, Orange County, "Sweden with duplicate points", and cdt_duplicates are visualizer-only.
  - Text format: `nVerts nEdges`, then x y lines, then index-pair lines (`cdt.test.cpp:87-118`).
  - What makes them hard: overlapping and duplicate constraints, hanging edges inside holes, constraint edges passing through vertices, boundary overlaps, near-degenerate intersections.
  - The predicate fixtures come from Nanevski et al., ICFP 2001, via mourner/robust-predicates (`CDTr/tests/inputs/predicates/README.md`).
- **Pathology handling**
  - Duplicate vertices: insertion throws `DuplicateVertexError` (`CDTr/include/Triangulation.hpp:2031`). Callers are expected to run `RemoveDuplicatesAndRemapEdges` first, which only standards_check exercises (`CDTr/tests/standards_check.cpp:36-38`). Every ground-truth test asserts `FindDuplicates(...).empty()` first (`cdt.test.cpp:541-545`). **No test checks DuplicateVertexError or IntersectingConstraintsError**, and the `DontCheck` strategy is never exercised (it appears only in `to_string`, `:471`).
  - Intersecting constraints: under NotAllowed, `IntersectingConstraintsError` (`Triangulation.hpp:643`, `:860`). Under TryResolve, a vertex is inserted at the intersection. If the rounded point breaks the topology, it throws `InvalidEdgeSplitVertex` (`:662`, `:882`). That validity check `isEdgeSplitVertexValid` (`:467-475`) was added after an out-of-bounds corruption.
  - `minDistToConstraintEdge` snaps vertices onto edges within a distance (#212 non-zero section, `cdt.test.cpp:1522-1531`; #230, `:1581-1598`). It is an absolute tolerance, so tolerance abuse is possible.
  - Huge coordinates: the 1e38 super-triangle regression (`:911-928`). A tiny bbox: the #174 fix uses `max(2r, 1)` (`CDTr/include/Triangulation.hpp:1100-1105`). **No NaN/inf validation** was found in Triangulation.hpp, and no test feeds NaN.
  - Termination: refinement is bounded by a caller-supplied `VertInd` budget (`cdt.test.cpp:1764-1791`), and refinement returns counters of what it gave up on (shortEdgeTriangles, circumcenterOutside, sharpFixedCorner, splitVertexInvalid; `:1745-1752`). Insertion can be aborted through the callback's `isAbortCalculation` (`Triangulation.hpp:2298`, `:2318`, `:2473`). Two former infinite loops became exceptions or kept edges (`cdt.test.cpp:1550-1579`, `:1600-1623`). There is no wall-clock timeout in the test runner.
  - Predicates are Shewchuk adaptive. The CI fast-math job checks that error-free transforms survive `-ffast-math`, `/fp:fast`, and `-march=haswell` (FMA), with `CDT_ENSURE_PRECISE_MATH_IN_CONSTRUCTIONS=ON` (`direct-cdt-study/CDT-53eff288a4e7/.github/workflows/ci_builds.yml:138-167`; `predicates.test.cpp:296`).
- **Fuzzing / mutation / property / metamorphic / differential / perf**
  - Fuzzing: done externally. The README credits Som1Lse for "fuzzing CDT: finding, reproducing, and reporting bugs" (`direct-cdt-study/CDT-53eff288a4e7/docs/README.md:364`). One test is a minimized fuzzer find (`cdt.test.cpp:1553`). **The harness is not in the tree.**
  - Metamorphic: batch split (`:754`). Property: the Delaunay edge count and refinement predicates.
  - Differential: KD-tree against brute force; predicates against exact integer and expansion arithmetic.
  - Several refinement tests are ported from spade (`cdt.test.cpp:1733-1734`, `:1766-1767`) and DelaunayTriangulation.jl (`:1797-1800`). That is cross-library test reuse, not differential execution.
  - Performance: Catch2 BENCHMARK on Constrained Sweden, hidden with the `[.]` tag (`:781-814`). Not a regression gate.
- **CPU/GPU or cross-platform consistency**: CI covers Windows 2025, Ubuntu 24.04 x64 and arm64, and macOS 15 arm64 (`ci_builds.yml:18-26`), with six Conan configurations including header-only, shared/static, Boost/no-Boost, and callbacks on (`:45-56`). It also compiles C++98 through C++26 (`:58-136`) and runs the fast-math job on MSVC, GCC, and Clang (`:138-167`). Golden files must therefore agree across OS, architecture, and compiler. That is a strong cross-platform determinism check. No sanitizers. Catch2 is a git submodule (`direct-cdt-study/CDT-53eff288a4e7/.gitmodules`), not present in this archive.
- **Known-bad / expected-failure handling**
  - Float-specific golden files for 5 inputs (`cdt.test.cpp:516-527`, `:625-628`).
  - Benchmarks hidden with the `[.]` tag (`:781`).
  - Callback tests compiled out unless `CDT_ENABLE_CALLBACK_HANDLER` is defined (`:1013`, `:1355`).
  - "Sensitive: a zero count needs a new input, not a relaxed assert" (`:2080-2082`) is a written anti-relaxation rule.
  - No xfail list.
- **Worth adopting for BRepGPU**
  - Input files to port as parameter-space trim cases:
    - `CDTr/tests/inputs/issue-42-*` (boundary or hole edges overlapping), for trim loops sharing seam segments.
    - `overlapping constraints*.txt`, `points_on_constraint_edge.txt`, `square with crack.txt` (slit or zero-width region), `Hanging*.txt` / `double-hanging.txt` / `triple-hanging*.txt` (dangling constraint edges; for us these are degenerate trim edges).
    - `issue-211.txt` and the #212 float vertices (near-degenerate intersections that must refuse rather than corrupt).
    - `issue-230.txt` (vertex 4e-17 off a hull edge).
    - `refine-*.txt`.
  - Technique: canonicalized topology golden files (`cdt.test.cpp:258-399`) stored per precision and checked on 4 OS/arch combinations. Pair them with stored coordinates, which CDT omits.
  - Technique: a separate winding check, because neighbor symmetry alone misses inverted and degenerate triangles (`VerifyTopology.h:77-95`). This is directly relevant to watertightness oracles.
  - Technique: a written policy that a numerically ambiguous intersection throws a typed exception (`InvalidEdgeSplitVertex`) and leaves the structure unmodified (`cdt.test.cpp:1998-2002`). This is our "explicit refusal".
  - Technique: refinement returns a structured account of what it gave up on, plus a hard vertex budget. Our tessellator should report the same kind of give-up counts.
  - Technique: a CI job that builds under `-ffast-math` and FMA and asserts the error-free transformations still hold (`predicates.test.cpp:296-338`). On GPU, nvcc's default FMA contraction is the same hazard. Adopt this verbatim as a gate for CUDA and Metal builds of our predicates.
  - Predicate fixtures `CDTr/tests/inputs/predicates/*.txt` (4,000 hard cases): import them as tier 7 predicate tests for both the CPU and GPU predicate implementations.

---

#### spade (Stoeoef/spade v2.15.1, c8befc96bbbc1898a89cb19f9f3104a848936374)

- **Test inventory** (Rust `#[test]`; counts per file: cdt.rs 54, triangulation_ext.rs 39, bulk_load.rs 16, dcel_operations.rs 14, math.rs 12, interpolation.rs 9, refinement.rs about 12, bulk_load_fuzz_tests.rs about 22)
  - Fuzzer regressions on bulk load: duplicate points, axis collinear runs, subnormal and 1e-40 coordinates (`spade/src/delaunay_core/bulk_load_fuzz_tests.rs:177-197`), points exactly at ±MIN_ALLOWED_VALUE (`:218-233`), clusters of about 2.7e-11 differing in the last bits (`:316-370`).
  - An epsilon grid: 20x20 points made of the first consecutive f64 values at or above MIN_ALLOWED_VALUE, shuffled 30 times (`spade/src/delaunay_core/bulk_load.rs:1192-1234`). The CDT version uses a zig-zag constraint chain (`:1236-1250`).
  - CDT: constraints overlapping existing edges (`spade/src/cdt.rs:1405-1445`), constraints through a point (`:1616`), degenerate cases (`:1656`, `:1784`, `:1908`).
  - Seeded random tests: random constraints on a single line (`cdt.rs:1690-1713`) and 2,000 grid-aligned constraints on a 60x60 grid (`:1715-1753`).
  - Intersecting a constraint panics: `#[should_panic]` (`cdt.rs:1755-1782`). An infinite-loop regression (#98) (`:1918-1950`).
  - Locate with NaN panics (`spade/src/delaunay_core/triangulation_ext.rs:1007-1022`).
  - Insertion and removal: removal down to empty and down to degenerate (`triangulation_ext.rs:1407-1461`), points on a line (`:1154-1181`), increasing distance (`:1197-1224`).
  - Coordinate validation (`spade/src/delaunay_core/math.rs:460-481`) and underflow mitigation (`:483-500`).
  - Refinement (`spade/src/delaunay_core/refinement.rs:1032-1300`): zero angle limit is a no-op, sharp angle with a budget of 10, outer-face exclusion monotone across iterations, f32 rounding breaking CCW on edge split (#96, `:1225-1253`), and a refinement-hang regression with a 1,000,000 vertex budget that asserts `refinement_complete` (`:1256-1290`).
  - Interpolation NaN regression (`spade/src/delaunay_core/interpolation.rs:1045`).
- **Oracles**
  - `sanity_check` (`triangulation_ext.rs:891-912`) is **tier 5.** It runs DCEL consistency checks (next/prev/rev/cw/ccw identities, face and vertex back-pointers, `spade/src/delaunay_core/dcel.rs:385-433`), CCW orientation of every inner face with the robust side query, no self-loop edges, Euler-style counting (inner directed edges = 3 x inner faces), convex hull convexity, and the **empty circumcircle on every edge** with robust incircle (`robust` crate, `math.rs:263-287`). That is strong.
  - **Weak spot:** `cdt_sanity_check` skips the Delaunay test whenever any constraint exists. It falls back to `basic_sanity_check` (`cdt.rs:737-751`). **The constrained-Delaunay property itself is never asserted on a CDT.** Constraint count consistency is the only CDT-specific check.
  - `fuzz_test` in bulk_load_fuzz_tests asserts sanity plus `num_vertices == dedup(input).len()` for both `bulk_load_stable` and `bulk_load` (`bulk_load_fuzz_tests.rs:5-18`). That is **tier 5 plus a tier 7 count.**
  - `add_constraint_and_split` split vertices must lie within **3e-2** of the original segment (`cdt.rs:2634-2637`). The comment says it is an "arbitrary epsilon that will just barely pass". That is a loose tolerance whitelist. Split points are rounded constructions, so the constraint geometry does drift.
  - Refinement: `refinement_complete` plus sanity. Tier 5 for validity. The angle bound is not independently re-checked.
  - `#[should_panic]` tests are tier 7 for "must refuse", but the refusal is a panic, not a recoverable error.
- **Corpora**: no file corpus in the crate. Inputs are seeded generators (`spade/src/test_utilities.rs:8-27`, fixed 32-byte seeds `SEED` and `SEED2`) and inline fuzzer minimizations. `spade/delaunay_compare/examples/Europe_coastline.shp` and `.dbf` (2,912,812 vertices, 2,837,094 constraint edges, per `spade/delaunay_compare/README.md`) are used only by a benchmark example. This archive has `.shp` and `.dbf` only; I did not check whether the companion `.shx` is needed.
- **Pathology handling**
  - Input validation: `validate_coordinate` refuses NaN (`InsertionError::NAN`), refuses |x| > MAX_ALLOWED_VALUE = 2^201 (`TooLarge`, which includes ±inf), and refuses nonzero |x| < MIN_ALLOWED_VALUE = 2^-142 (`TooSmall`) (`spade/src/delaunay_core/math.rs:20-31`, `:69-108`). These bounds come from Shewchuk's no-overflow/no-underflow conditions (`:60-66`). `mitigate_underflow` flushes tiny values to 0 (`:156-169`). Insertion returns `Result`, so bad input is refused explicitly.
  - NaN in a query (`locate`) panics (CHANGELOG notes the behaviour was made consistent; `triangulation_ext.rs:1007-1022`).
  - Duplicates are merged on bulk load (the vertex count equals the dedup count).
  - Constraint intersection: `add_constraint` panics. `try_add_constraint` returns empty. `add_constraint_and_split` inserts rounded intersection points (`cdt.rs:553-558`). `try_bulk_load_cdt` was added "for loading CDTs without panicking" (`spade/CHANGELOG.md:17`).
  - Refinement termination: the default budget is `num_initial_vertices * 10` extra vertices (`refinement.rs:498-503`). The main loop breaks with `refinement_complete = false` when the budget is reached (`:533-537`). Optional min/max area (`:244`, `:264`), angle limit (`:228`), outer-face exclusion (`:319`). Split points are snapped to the nearest power of two of the half-length (Ruppert concentric shells, `:809`) so they don't cascade near small input angles. The CHANGELOG records "Prevents potential endless loop during refinement (#134)", "infinite loop" #98, and "panic during refinement due to rounding errors" #96 (`spade/CHANGELOG.md:12`, `:115`, `:131`).
  - Predicates are robust (Shewchuk via the `robust` crate) for orientation and incircle. Constructions such as split points and intersections are plain float.
- **Fuzzing / mutation / property / metamorphic / differential / perf**
  - cargo-fuzz with libFuzzer and `arbitrary` (`spade/fuzz/Cargo.toml:8-38`) has three targets:
    - `bulk_load_fuzz` (`spade/fuzz/fuzz_targets/bulk_load_fuzz.rs:7-18`): `Vec<f64 point>`, filtered to valid coordinates and **|x|,|y| <= 20**. Then bulk_load and sanity_check.
    - `bulk_load_int_fuzz` (`bulk_load_int_fuzz.rs:6-21`): i32 points in [-1024, 1024].
    - `bulk_load_cdt_fuzz` (`bulk_load_cdt_fuzz.rs:8-53`): points and edges. It builds an incremental reference, keeps the edge prefix that `can_add_constraint` accepts, bulk-loads the CDT, and runs `cdt_sanity_check`. The reference is built but **never compared** to the bulk-loaded result. A differential oracle was within reach and not taken.
    - The custom `Debug` formatters print `Point2::new(...)` (`fuzz_shared.rs:16-20`), so crashes paste straight into regression tests. That is how `bulk_load_fuzz_tests.rs` came about.
  - Property-based: proptest on `add_constraint_and_split` (1,000 cases) and `bulk_load_cdt_stable` (200 cases), but **both are `#[ignore]`** (`cdt.rs:2453-2462`, `:2644-2665`). Their minimized finds are kept as plain tests (`cdt.rs:2419`, `:2594`).
  - Differential: none. `delaunay_compare/` benchmarks spade, the `cdt` crate, and delaunator for speed only (`spade/delaunay_compare/README.md:1-30`, `spade/delaunay_compare/src/lib.rs:9-10`). It produces no outputs to compare.
  - Performance: criterion benches (`spade/benches/*.rs`). `spade/benches/README.md` holds hand-copied results and is not a gate.
- **CPU/GPU or cross-platform consistency**: CI is ubuntu-latest only (`spade/.github/workflows/spade_actions.yaml`): `check --no-default-features --features=serde` (no_std build), `cargo test`, rustfmt, clippy `-D warnings`, and **`cargo fuzz build` only. The fuzzers are compiled, never run in CI** (`:67-84`). No miri, no sanitizers.
- **Known-bad / expected-failure handling**: `#[ignore]` on both proptests. `#[should_panic]` for documented refusals. The 3e-2 split-distance tolerance (`cdt.rs:2637`). The fuzz targets silently discard inputs beyond |20| (`bulk_load_fuzz.rs:12`), which hides large-magnitude and wide-dynamic-range behaviour from the fuzzer.
- **Worth adopting for BRepGPU**
  - The `validate_coordinate` contract: refuse NaN, inf, and values outside [2^-142, 2^201]; flush underflow (`math.rs:69-108`, `:156-169`). Our parameter-space front end should derive and enforce a similar bound for our own predicates, and have a test for each boundary value (`math.rs:460-500`).
  - Epsilon-grid generator (`bulk_load.rs:1192-1234`): adjacent-ULP coordinates around a magnitude. This is the classic parameter-space torture case, such as a trim at u ≈ 0 next to a pole. Generalize it to any magnitude, such as 1e6 with ULP neighbours, to cover our 1e6:1 scale family.
  - Copy the minimized fuzz-repro inputs as golden regressions: `bulk_load_fuzz_tests.rs` `_10`, `_11`, `_13`, `_19`, `_20`, `_21`, and `bulk_load.rs:1488` (`test_cdt_fuzz_1`).
  - `repro_refine_hang_*` (`refinement.rs:1256-1300+`): a fan of nearly collinear constraints in a unit square that used to hang refinement. A direct termination case.
  - Technique: a fuzz `Debug` formatter that emits ready-to-paste test code.
  - Technique: run the fuzzers in CI or nightly. spade only builds them. Use an incremental-against-bulk differential oracle, which spade computes in `bulk_load_cdt_fuzz.rs:25-47` and then discards.
  - Gap to avoid: assert the constrained-Delaunay property, not just DCEL consistency, when constraints exist.

---

#### Blender blenlib, selected files (blender/blender main 3f4216e1bad1e76e9c2d4a00b4a9d09954593f47)

Only these 10 files were fetched: 5 tests and 5 intern sources. Not present: CMake test registration, `testing/testing.h`, `BLI_math_mpq.hh`, `delaunay_2d.hh`, `polyfill_2d.cc`'s header, CI (Blender uses buildbot; nothing under `.github` was fetched), and `convexhull_2d.cc`. Statements about which tests actually run therefore rest on the in-file `#define` switches only.

- **Test inventory**
  - `BL/tests/BLI_delaunay_2d_test.cc` (5,097 lines; prefix path `blender-selected/source/blender/blenlib/`). Every geometric case is a template run twice: `delaunay_d` (double, epsilon 1e-5) and `delaunay_m` (GMP `mpq_class` exact rationals, epsilon 0) (`:94-104`, `:3774-4102`, `:4104-4402`). There are about 75 cases:
    - Empty, 1, 2, and 3 points; mixed points; quads (`:432-650`).
    - Line in square, line hole (`:653-735`).
    - Nested holes; even-odd nesting, boundary disagreement, coincident polygons (`:738-1010`).
    - CW input face ids (`:1011`).
    - Looping-back faces 2x and 3x (`:1053-1150`).
    - About 20 non-zero winding cases: cancel-to-zero, high count, fan, self-intersect, deep nest, shared subsegment, island in hole, coincident verts, many crossings, negative-only, T-junction, exact shared edge (`:1202-2716`).
    - Crossing segments, cuts across a triangle, diamonds crossing, many crossings (`:2718-2960`).
    - Overlapping faces, squares, and edges (`:2963-3262`).
    - Repeated edge and repeated triangle (`:3343-3407`).
    - Shared split boundary (`:3409`).
    - Letter B fill and degenerate interior faces (`:3480-3618`).
    - Dissolving a pendant edge (`:3620`).
    - A stale symedge crash (`:3648-3678`).
    - Two minimized fuzz repros (`:3680-3723`).
    - Edge-id provenance through intersections (`:3725-3772`).
  - Text-outline stress tests (`text_test`, `:4406-4749`): a letter B with N arc points, M letters per line, and L lines, with inverted copies overlapping. Variants up to B200 and B10x20x20, in double and mpq. **Compiled out: `#define DO_TEXT_TESTS 0`** (`:18`).
  - Random tests (`rand_delaunay_test`, `:4751-4970`): random points, segments, polygons, tilted grids (tilt 0, 1, 0.01), points on a circle, and triangles between circles with radius 0.25 or 1e-4. Sizes are 2^1..2^7, seed `BLI_rng_new(0)`, double and mpq (`:4973-5091`). **Compiled out: `#define DO_RANDOM_TESTS 0`** (`:19`).
  - `BL/tests/BLI_polyfill_2d_test.cc` (790 lines): about 30 polygons. Triangle, CCW/CW square, starfleet with a repeated point, 3 and 4 collinear points, unordered collinear, plus, star, U, spiral, self-intersect, self-touch, self-overlap, libgdx issues 815/207/1407, and Blender bugs #40777, #41986, #52834, #67109 a/b/c, #103913 (`:402-790`). Each is run through polyfill and then beautify.
  - `BL/tests/BLI_mesh_intersect_test.cc`: mpq3 triangle meshes from string specs (`:30-115`). Tri-tri coplanar, corner crosses, tet-tet, cube-cube step, rect cross (`:165-815`). Perf tests sphere-sphere (512 rings), sphere-grid, grid-grid, print timing only. **Compiled out: `DO_PERF_TESTS 0`** (`:20`, `:815`). The whole file requires `WITH_GMP` (`:22`).
  - `BL/tests/BLI_mesh_boolean_test.cc`: tet-tet, cube-tet, coplanar, inside, beside, degenerate triangles, cube-cube, cube-cone (`:117-963`). Requires `WITH_GMP` (`:20`).
  - `BL/tests/BLI_convexhull_2d_test.cc`: convexity and CCW of the hull, minimum-area AABB fit angle on lines, octagons, near-duplicates over 10^-15..10^15 scale (`:358-409`), near-collinear, coincident topmost x, random transforms (`:411-478`). Circle and Random are under `#if 0` (`:519-574`).
- **Oracles**
  - Delaunay tests check exact vertex, edge, and face counts, original-id provenance (`get_orig_index`, `output_face_has_input_id`), and some intersection coordinates. For example, CrossSegs: 5 verts, 8 edges, 4 faces, intersection at (0,0) (`BLI_delaunay_2d_test.cc:2718-2755`). This is **tier 7** for small hand-derivable cases and **tier 3** otherwise. Running the same case in double and mpq is a **tier 6 model check in spirit**, but each instantiation is compared to the same hand-written expectations, not to each other.
  - **Bug in the oracle:** `expect_coord_near<mpq_class>` compares `testco[0]` with `refco[0]` twice and never compares y (`BLI_delaunay_2d_test.cc:416-421`). Exact-arithmetic y coordinates are unchecked.
  - No Delaunay empty-circle check, no orientation check, and no "every constraint present" check in the delaunay test file. No `validate_cdt` or self-check function exists in `BL/intern/delaunay_2d.cc` at this revision. A grep for validate/sanity/check found nothing relevant. Debug dumps (`dump_crossings`, `:1996`) are print-only.
  - Fuzz repro tests only call the function and discard the result (`(void)out;`, `:3698`, `:3720`). **Tier 2.**
  - Random and text tests assert only `out.vert.size() != 0` and print timings (`:4955-4967`, `:4608-4617`). **Tier 1 to 2, and disabled anyway.**
  - Polyfill harness (`BLI_polyfill_2d_test.cc:47-213`) is **tier 5**:
    - every triangle index set, no repeated index within a triangle, every vertex used;
    - the edge set has exactly n + (n-3) edges, each boundary edge used once, every edge used once or twice;
    - all non-degenerate triangles have the same winding;
    - total area within 1e-5 relative;
    - optionally no triangle area < 1e-6.
    - Degenerate inputs skip winding and area (`:193-203`). Each case also runs after beautify (`:228-238`), and under x/y sign flips (`:242-258`) as a **tier 4** metamorphic check.
  - **Bug:** the "every possible offset and reverse" loop (`USE_COMBINATIONS_ALL`, `:259-291`) cycles and reverses `poly_copy` but passes the original `poly` to the checker (`:281`). The cyclic-shift and reversal metamorphic variants are never actually tested. The same polygon is tested 2n times.
  - Mesh-intersect and boolean: vertex and face counts, plus face/edge provenance (`BLI_mesh_intersect_test.cc:274-343`; `BLI_mesh_boolean_test.cc:152-243`). Tier 7 for tiny polyhedra, tier 3 in general. No manifold or watertightness assertion in the tests.
  - Convex hull: convexity and CCW per hull vertex (tier 5), and AABB-fit angle against analytic values (tier 7). "Area no larger than generated" (`:476`) is a one-sided property.
- **Corpora**: no external files. Everything is inline string specs (`fill_input_from_string`, `BLI_delaunay_2d_test.cc:39-105`) or procedural generators (text glyph B, random points/segments/polygons/grids/circles, spheres and grids for mesh-intersect). Hard parts: deliberately overlapping and inverted letters (text tests), triangles between two circles whose radii differ by 1e-4 (near-cocircular and degenerate, `:5023`), and tilted grids (collinear-rich).
- **Pathology handling**
  - Exact mode: with `T = mpq_class`, epsilon is 0 (`:96-99`). Orient and incircle are filtered: double with an error bound `supremum * index * DBL_EPSILON`, falling back to exact mpq (`BL/intern/delaunay_2d.cc:714-812`). In double mode, `filtered_orient2d<double>` calls `orient2d` on doubles (`:740-745`). The comment says Shewchuk's exact primitives are used when T is not exact (`:1609-1612`).
  - Double mode applies an **absolute epsilon** (1e-5 in tests) to snap an edge crossing onto an endpoint (`lambda * len_ab <= epsilon`, `delaunay_2d.cc:1838-1853`). That is a tolerance that can be abused at large or small parameter scales.
  - Duplicate vertices are merged by exact coordinate equality after sorting (`find_site_merges`, `:1336-1348`). Merges are recorded in `merge_to_index` and reported through `vert_orig`.
  - Overflow guards on id encoding use `BLI_assert` (`:2349`, `:2398`), which disappears in release.
  - No timeout, hang, or memory-limit tests. Mesh boolean checks piecewise winding number (`is_pwn`, `BL/intern/mesh_boolean.cc:1352-1395`) and zero-volume cells (`:535-555`) at runtime. Mesh intersect uses exact mpq planes and canonicalizes degenerate planes (`BL/intern/mesh_intersect.cc:100-128`).
  - Polyfill handles self-intersection by allowing overlapping triangles (documented, `BL/intern/polyfill_2d.cc:13-14`). Beautify refuses flips that would create zero-area triangles (`eps_zero_area = 1e-12f`, `BL/intern/polyfill_2d_beautify.cc:98-135`) and locks degenerate states (`:240-242`).
- **Fuzzing / mutation / property / metamorphic / differential / perf**
  - Fuzzing: done out of tree. Two "minimized test case found by fuzzing a fix for #160787" repros (`BLI_delaunay_2d_test.cc:3680-3723`).
  - Metamorphic: polyfill sign flips. Rotation and reversal are intended but broken, see above.
  - Differential: none executed. The double and mpq instantiations share expectations. The convex-hull comment mentions a brute-force reference, enabled only with a source define in convexhull_2d.cc, which was not fetched (`BLI_convexhull_2d_test.cc:521-526`).
  - Performance: the random, text, and mesh-intersect perf tests print timings and are disabled.
- **CPU/GPU or cross-platform consistency**: nothing in the fetched files. The comment "#ROTATION_EPS epsilon fails on MacOS, use a slightly larger epsilon" (`BLI_convexhull_2d_test.cc:396-399`) shows that cross-platform float differences are handled by loosening tolerance.
- **Known-bad / expected-failure handling**: `DO_TEXT_TESTS 0`, `DO_RANDOM_TESTS 0`, `DO_PERF_TESTS 0`, and `#if 0` on convex-hull Circle and Random disable whole families. `WITH_GMP` gates all exact tests. `POLYFILL2D_TEST_IS_DEGENERATE` disables winding and area checks for self-intersecting inputs. The macOS-specific tolerance bump is noted above.
- **Worth adopting for BRepGPU**
  - Pattern: instantiate the same CDT test over `double` and an exact rational type (`BLI_delaunay_2d_test.cc:94-104`). Strengthen it to compare the double output against the mpq output, a real tier 6 check, instead of both against hand-written counts.
  - `text_test` generator (`:4406-4630`): parameterized glyph outlines with arcs, holes, many copies, and inverted overlapping copies. It is a good scalable stress family for trim-loop CDT (many nested loops, arc-tessellated boundaries). Enable it with real oracles.
  - `RANDOM_TRI_BETWEEN_CIRCLES` with a 1e-4 radius ratio (`:4829`, `:5023`). Near-cocircular, needle-rich input.
  - `RANDOM_TILTED_GRID` with tilt 0.01: nearly axis-aligned collinear runs, matching parameter-lattice trims.
  - Non-zero-winding cases (`:1202-2716`) such as T-junction, shared subsegment, cancel-to-zero, island-in-hole. They test region classification of overlapping and duplicated trim loops, which matters when STEP faces carry redundant or overlapping loops.
  - The polyfill topology oracle (`BLI_polyfill_2d_test.cc:94-123`): an exact edge-multiplicity count where boundary edges appear once and interior edges twice. A cheap watertightness check for a single region. Fix the `poly` / `poly_copy` bug before copying the cyclic-shift and reverse metamorphic loop.
  - Polyfill bug inputs #67109 a/b/c (`:721-775`): the same polygon offset and rotated, where some variants fail. Direct evidence that rotation and offset metamorphic tests find real bugs.
  - Anti-patterns to avoid: `(void)out` fuzz repros, disabled stress families, and the copy-paste bug in the mpq coordinate check.

---

#### gCDT (yingtix/gCDT c5252b1eac5d)

- **Test inventory**: **no tests.** There is no test directory, CTest, or CI, and only one executable target (`gcdt/CMakeLists.txt`). `gcdt/README.md` says "The code has not been cleaned up yet; there are many meaningless/obsolete functions". `main` is a hard-coded driver: 9,000,000 points from `randomMap(..., seed 13159)`, `numCons = 0` (constraints disabled) (`gcdt/kernel.cu:5332-5354`). It ends with `return;` inside `int main` (`kernel.cu:5465`).
- **Oracles**
  - None at run time. The only correctness check is CPU-side code in `tryFastDelaunay` (`gcdt/fastDelaunay.cu:710-830`). It copies the GPU mesh to host and runs `testEdge`, a robust integer incircle on every interior edge (`fastDelaunay.cu:401-414`). It prints "error x/y/z/w" for non-Delaunay neighbours (`:735-812`). That is a tier 5 Delaunay check, print-only, never asserted, and **its call from main is commented out** (`kernel.cu:5446`).
  - `testEdgeInfo` checks twin-edge consistency and prints "wrong other" (`fastDelaunay.cu:536-566`), and is also unused from main. `testTopo` is commented out inside the flip loop (`kernel.cu:4824`).
- **Corpora / input generators** (`gcdt/kernel.cu:43-170`):
  - `randomMap`: uniform in [0.001, 0.999]^2 with 4 fixed seed points.
  - `twoLineMap`: points on two segments, all collinear.
  - `squareMap`: a regular sqrt(n) x sqrt(n) grid with no jitter, maximally cocircular. A jitter distribution is created but never used (`:96`, `:111-119`).
  - `gaussianMap`: clipped at 3 sigma.
  - `thinCircleMap`: an annulus of width 0.01, near-cocircular.
  - `readFile(path)`: text `n`, then `idx x y` lines, then `m`, then `idx a b` constraint lines. It removes exact duplicate points with a `std::map` and remaps constraints, drops self-loop and duplicate constraints, then normalizes to [0.005, 0.995] (`kernel.cu:5159-5292`). Its call is commented out (`:5338-5342`). No data files ship in the archive. The input datasets used for the gCDT paper are not referenced anywhere in the 24 files.
- **Pathology handling**
  - Exactness by quantization: input doubles are scaled to an integer grid of 2^30 (`logPointRes = 30`, `gcdt/include/math_base.cuh:19`) and clamped (`kernel.cu:171-188`). Orient and incircle are then evaluated in fixed-width big integers (`JIO::Integer`, `MyInteger<logPointRes*4+4>`, `math_base.cuh:267-327`; int128 from `include/intX_t.cuh`). The predicates are exact on the quantized input, but the quantization itself moves points by up to 2^-30 of the bbox and can merge distinct points. **Nothing detects or reports that merging.**
  - Termination: the GPU flip loops are `while (mark)` / `while (mark != 0)` with **no iteration cap** (`kernel.cu:4782`, `:4997`, `:5077`, `:5131`). The CPU check loop is capped at 50 iterations (`fastDelaunay.cu:723`).
  - Memory: fixed `maxTriangles = 4*numPoints` buffers (`kernel.cu:5360`; `gcdt/rawMesh.cu:870`) with no overflow check. The CUB temp buffer is hard-coded to 983,743,231 bytes (`kernel.cu:5386`). Kernel launch errors are not checked, because `getLastCudaError` is commented out in `utils::kernel` (`gcdt/utils.cu:34`).
  - The edge hash uses key `x ^ y` with first-writer-wins (`kernel.cu:221-229`). Colliding edges are dropped silently, which is a silent-corruption hazard.
- **Fuzzing / mutation / property / metamorphic / differential / perf**: none, apart from per-kernel `cudaEvent` timing printed to stdout (`gcdt/utils.cu:25-40`; `kernel.cu:4763-4836`).
- **CPU/GPU or cross-platform consistency**: none executed. `tryFastDelaunay` is the only host-side re-check of GPU output, and it is unused. CMake targets sm_70/75/80/86 but forces `-arch=sm_70` (`gcdt/CMakeLists.txt`). **P100 (sm_60) is not a listed target.**
- **Known-bad / expected-failure handling**: none. Commented-out alternative code paths stand in for configuration.
- **Worth adopting for BRepGPU**
  - The generator set, rewritten with oracles. `thinCircleMap`, unjittered `squareMap`, `twoLineMap`, and `gaussianMap` (`kernel.cu:60-170`) are the standard GPU-Delaunay stress distributions: cocircular, collinear, clustered.
  - The integer-quantization design (2^30 grid plus big-integer predicates) is a credible GPU-exact approach. If we adopt it, we must add a test that quantization never merges or reorders distinct trim vertices, or that a merge is reported as a refusal.
  - The host-side incircle re-check of the GPU mesh (`fastDelaunay.cu:401-414`, `:735-812`) is the right CPU/GPU differential hook. Run it every time and make it assert.
  - Anti-patterns: unbounded device flip loops, unchecked launches, fixed capacity buffers, hash first-writer-wins. Each maps to one of our explicit requirements (no hangs, no runaway memory, no silent bad meshes).

---

#### Blind spots these projects reveal (fault classes the bootstrap plan would miss)

1. **Oracle blindness to inverted or degenerate triangles.** Neighbor-symmetry topology checks pass on inverted and zero-area triangles (`direct-cdt-study/CDT-53eff288a4e7/CDT/extras/VerifyTopology.h:77-79`; `cdt.test.cpp:1956-1960`). The plan lists "orientation" and "sliver" checks. It should also state that these are independent of the manifold check, and must run on every intermediate CDT, not only on the final 3D mesh.
2. **The constrained-Delaunay property is skipped once constraints exist** (spade `cdt.rs:737-751`; poly2tri checks the unconstrained property; CDT only on conforming output). The plan needs an explicit tier 5 constrained empty-circle oracle in parameter space, with exact predicates.
3. **Area-balance oracles miss overlap plus gap** (earcut `deviation`, Blender polyfill area). The plan's "dense deviation vs exact geometry" has the same blind spot in 3D. Add pairwise overlap and coverage-multiplicity checks: each boundary edge once, each interior edge twice.
4. **FMA and fast-math contraction breaking robust predicates.** Only CDT's CI tests it (`ci_builds.yml:138-167`; `predicates.test.cpp:296-338`). This applies directly to nvcc, which contracts to FMA by default, and to the Metal compiler. Add a predicate self-test that runs on every target device and compiler flag set.
5. **Rounded constructions drifting off constraints.** Intersection and split vertices are rounded: spade allows 3e-2 (`cdt.rs:2637`); CDT stores no coordinates in golden files. The rounded point can land outside the triangles it splits (`CDT InvalidEdgeSplitVertex`). The plan should test that every inserted Steiner or split point lies on its parent trim segment within a stated bound, and refuse otherwise.
6. **Absolute epsilons in snapping** (Blender 1e-5 crossing snap `delaunay_2d.cc:1838-1853`; CDT `minDistToConstraintEdge`; poly2tri `EPSILON = 1e-12`). Parameter domains of arbitrary scale break absolute tolerances. Add a metamorphic test: scale the (u, v) domain by 1e-6 and by 1e6 and require the same topology. Blender's convex-hull scale sweep (`BLI_convexhull_2d_test.cc:358-409`) is the template.
7. **Cyclic shift and reversal of the input loop.** Polyfill bug #67109 only failed for some offsets or rotations, and Blender's own shift and reverse loop is silently broken (`BLI_polyfill_2d_test.cc:281`). The plan's mutation fuzzer should permute loop start vertex, loop direction, loop order, and edge order, and assert invariant output.
8. **Silent geometry drop on fallback paths** (earcut `splitEarcut` returns nothing, `earcut.js:249-272`; gCDT hash collisions). "Correct output or explicit refusal" needs an exact coverage oracle. The area of the output triangles must equal the area of the trimmed region to a stated bound, in addition to the dense 3D deviation check.
9. **Input-magnitude validity bounds.** spade's [2^-142, 2^201] plus NaN/inf refusal (`math.rs:69-108`) is derived from the predicates. The plan's "wild rational weights" family should include a test that out-of-range parameter values are refused, not processed. CDT has no NaN check at all.
10. **Precision-dependent outputs.** CDT keeps separate f32 and f64 golden files. The plan should decide whether CPU (f64) and GPU (f32 or mixed) are expected to produce identical topology or only equivalent topology, and write the oracle to match. A count-equality check alone will not catch the difference.
11. **Fuzzers that are compiled but never run, or that clamp inputs to small ranges** (spade CI `cargo fuzz build` only; spade fuzz targets clip to |20|). Our mutation fuzzer needs a scheduled run with a corpus and crash minimization, and must not clamp away the 1e6:1 scale family.
12. **Refinement termination as a resource contract.** Spade and CDT both bound refinement by an additional-vertex budget and report incomplete results (spade `refinement.rs:498-537`; CDT `refineTriangles` give-up counters). The plan's "termination within bound" should include a vertex and triangle budget per face, with a test that hitting the budget yields an explicit refusal or flag rather than a truncated mesh.

### Cluster 5: Robust meshing on wild inputs: TetWild, fTetWild, Thingi10K, libigl

Paths are relative to `quarantine/sources/`. I only read files. Nothing was built or run.

Papers: the TetWild (SIGGRAPH 2018) and fTetWild (TOG 2020) papers are **not** in `papers_proj/` or `BRepGPU/papers/`. I searched filenames, `.bib` and `.tsv` catalogs. The READMEs point to https://yixin-hu.github.io/tetwild.pdf and https://yixin-hu.github.io/ftetwild.pdf, and I did not read either. Anything below about how their Thingi10K evaluations were run comes only from the repositories. The Thingi10K paper **is** present (`BRepGPU/papers/pdfs/zhou-jacobson-2016-thingi10k.pdf`, arXiv 1605.04797v2), and I read it as text.

---

#### TetWild (Yixin-Hu/TetWild rev 49de8cd85f56de0c6dc7ac3c245a14c3aa68ee67)

- **Test inventory** -- There are none: no `tests/` directory, no test sources, no ctest registration. CI (`tetwild/.travis.yml`, `tetwild/.appveyor.yml`) only runs `conda build .ci/conda`. `tetwild/.ci/conda/build.sh` configures, then runs `ninja install`. `tetwild/.ci/conda/meta.yaml` has no `test:` section. Nothing executes the mesher in CI. `docs/` holds only two images (`docs/teaser.png`, `docs/Slide1.jpg`, the orientation figure).
- **Oracles** -- In CI, only tier 1 (it compiles). Robustness evidence lives off-repo: the README links to the 10k Thingi10K output tetmeshes and surfaces on Google Drive and to "figures in the paper: input/output & scripts" (`tetwild/README.md`, Dataset section). None of that was acquired. The in-code runtime checks below are the only oracle machinery in the tree.
- **Corpora** -- None in the tree. The input is Thingi10K (https://ten-thousand-models.appspot.com/). The pre-generated outputs are on Google Drive. Not read.
- **Pathology handling**
  - *Envelope (epsilon) check, a sampled Hausdorff bound:*
    - `CHECK_ENVELOP` is hard-defined true (`tetwild/src/tetwild/LocalOperations.cpp:24`).
    - A face is "out" if any sample point on it is farther than `eps_2` (squared) from the input surface. Samples are generated by `sampleTriangle` (`tetwild/src/tetwild/Common.cpp:143`) and queried against a geogram AABB with a nearest-facet hint (`LocalOperations.cpp:1046-1104`). A degenerate triangle is reported "inside" without testing (`LocalOperations.cpp:1049`).
    - The sampling is made conservative by shrinking epsilon. With d = `sampling_dist` = `eps_input/stage`, the effective eps is `eps_input - d/sqrt(3)*(stage+1-sub_stage)`. The `d/sqrt(3)` is the covering radius of the triangular sample lattice (`tetwild/src/tetwild/State.cpp:36-41`, comment "d_err = d/sqrt(3)").
    - An exact interval/subdivision envelope test survives only as commented-out code. Its loop capped at depth 6 and returned "outside" (`LocalOperations.cpp:977-1031`).
    - Preprocessing simplification uses a 0.8x shrunken envelope (`tetwild/src/tetwild/Preprocess.cpp:200-206`).
  - *Rational/exact insertion:*
    - Vertex positions are CGAL Epeck `Point_3 pos` (exact), with a double shadow `posf` (`tetwild/src/tetwild/CGALTypes.h:22-43`, `tetwild/src/tetwild/TetmeshElements.h:26,38-42`).
    - BSP subdivision and the conforming tetrahedralization run in exact arithmetic. Internal inconsistencies throw: `log_and_throw("ERROR: side == CGAL::ON_ORIENTED_BOUNDARY!!")` at `tetwild/src/tetwild/SimpleTetrahedralization.cpp:551`, and `TetWildError("MeshConformer::triangleIntersection3d")` at `tetwild/src/tetwild/MeshConformer.cpp:298,316`.
    - Rounding back to double is accepted per vertex only if every incident tet stays positively oriented under exact `CGAL::orientation`. Otherwise the vertex keeps its rational position (`tetwild/src/tetwild/MeshRefinement.cpp:57-95`).
    - Vertices that never round are only **logged** at debug level and written to the stats CSV (`tetwild/src/tetwild/tetwild.cpp:81-89`). The mesh is still output. This is a silent degradation channel: a flag in a CSV, not a refusal.
  - *Termination:*
    - The optimization loop is bounded by `max_num_passes` (default 80, `tetwild/include/tetwild/Args.h`). It exits early when max AMIPS energy drops below `filter_energy_thres` (default 10) (`MeshRefinement.cpp:259,285-286`).
    - A stall test (avg and max energy change < `delta_energy_thres` = 0.1) triggers a sizing-field update, and an update budget of 2 then exits the loop (`MeshRefinement.cpp:291-324`).
    - The inner `doOperationLoops` is also bounded by `max_pass` (`MeshRefinement.cpp:185-205`).
    - Nothing bounds the exact BSP stage. There is no wall-clock timeout and no memory cap. The README says the hardest model needed ">100GB memory", and a crash should be retried "on cluster with more resource" (`tetwild/README.md`, Important Tips). Docker termination is blamed on the Docker memory limit (README, Docker section).
  - *Degenerate/duplicate inputs:*
    - Vertices are welded with a fixed **absolute** tolerance of 1e-10, whatever the model scale (`Preprocess.cpp:104`). An empty mesh writes an empty `.msh` and throws "Empty mesh!" (`tetwild.cpp:186-194`).
    - Exactly degenerate triangles are dropped (`Preprocess.cpp:292`).
  - *Orientation:* inside/outside comes from the generalized winding number on tet centroids (> 0.5). If every tet is removed, the code flips all faces and retries, marked "TODO: test the correctness" (`tetwild/src/tetwild/InoutFiltering.cpp:45-75`).
  - *Interactive hang:* `pausee()` blocks on `std::cin` and calls `exit(0)` on '0' (`Common.cpp:42-48`). Any call left on a reachable path would hang a batch job.
- **Fuzzing / mutation / property-based / metamorphic / differential / performance-regression** -- None found. The `--targeted-num-v` 5% tolerance and the per-stage CSV records (`Args.h` `write_csv_file`) are instrumentation, not tests.
- **CPU/GPU or cross-platform consistency** -- None. An optional ISPC energy kernel (`TETWILD_WITH_ISPC`, `tetwild/CMakeLists.txt:12,148`) has no equivalence test. The README says it changes iteration counts.
- **Known-bad / expected-failure handling** -- None in the tree.
- **Worth adopting for BRepGPU**
  - (a) The conservative-sampling arithmetic in `State.cpp:36-41`. Shrink the deviation tolerance by the sampling lattice's covering radius so that a finite sample set proves a bound. Our "dense deviation vs exact geometry" oracle needs exactly this, or it is only a lower bound on the Hausdorff distance.
  - (b) The exact-then-round-with-orientation-recheck pattern (`MeshRefinement.cpp:57-95`). It carries over directly to parameter-space CDT vertices produced by trim intersections: round to double only if every incident triangle keeps its exact orientation.
  - (c) The anti-pattern to avoid: a "succeeded" output with unrounded or invalid elements reported only in a log (`tetwild.cpp:81-89`). Our contract demands refusal.
  - (d) The absolute 1e-10 weld tolerance (`Preprocess.cpp:104`) is a concrete scale bug to put in our 1e6:1 scale-ratio family. Test the same model at 1e-6x and 1e6x.

---

#### fTetWild (wildmeshing/fTetWild rev 8118f810478e0e65a7bf2d8cecdc5e203a876e97)

- **Test inventory** (Catch2 v3.5.3 via FetchContent, `ftetwild/tests/CMakeLists.txt:2-7`; only `main.cpp`, `tests.cpp`, `test_predicates.cpp`, `test_table.cpp` are compiled, `tests/CMakeLists.txt:12-20`)
  - Placeholder: `REQUIRE(1 == Approx(1))` (`ftetwild/tests/tests.cpp:9-12`).
  - Orientation predicate on the unit tet: two hand-picked signs (`ftetwild/tests/test_predicates.cpp:14-28`).
  - Triangle-triangle intersection near-miss:
    - The "rounded" decimal input must report no intersection (`test_predicates.cpp:31-52`).
    - The same triangles written in shortest-round-trip doubles (`-27.478179999999998`) accept **either** 0 or 666 (`test_predicates.cpp:55-78`). 666 is the early-out "all on one side" code (`ftetwild/src/external/triangle_triangle_intersection.cpp:247,256`).
  - Cut-table consistency: for 26 table indices, number of tet configurations equals number of diagonal configurations, with 1/3 diagonals for 2/8 configurations. The 5-diagonal case is commented out (`ftetwild/tests/test_table.cpp:30-59`). The table itself is generated offline by `ftetwild/python/build_cut_table.py` and `ftetwild/python/generate-table-code.py`. `ftetwild/python/plot-tet.py` and `ftetwild/python/save_all_confs.py` render each configuration to HTML for manual inspection.
  - Not compiled: `test_subdivision.cpp` (prints only, `ftetwild/tests/test_subdivision.cpp:8-29`) and `test_tree.cpp`. The latter compares fTetWild's point-triangle distance against geogram (`ftetwild/tests/test_tree.cpp:23-56`, which would be tier 6) and benchmarks 1e6 random queries against a hard-coded Windows path, `C:\Users\3Dscanner\Downloads\Octocat-v2.stl` (`test_tree.cpp:70`). `test_envelope.cpp` is listed but does not exist.
- **Oracles**
  - Predicate tests: tier 7 (spec-derived sign on hand-built configurations).
  - The 0-or-666 test is deliberately weakened to tolerate a platform-dependent code path. It still checks "no intersection", so tier 7, but it records that results differ by input spelling.
  - Table test: tier 5 (structural invariant of a generated table).
  - CI smoke run: `FloatTetwild_bin --input ../tests/bunny.off --level 0 --stop-energy 100` (`ftetwild/.github/workflows/continuous.yml`, "Run Example" steps), with no output check. Tier 1/2: "it exits".
  - No test checks a tetmesh, an envelope, or a winding-number filter.
- **Corpora** -- `ftetwild/tests/bunny.off` is the only mesh. The Thingi10K input link and the 10k output tetmeshes (Google Drive) are in `ftetwild/README.md`, Dataset section, and were not read. `ftetwild/replicability_instructions.htm` is the Graphics Replicability Stamp report:
  - Reviewer David Coeurjolly. Commit 4c66f57380342e6868f912890c974cc093d505f5. Score 5.
  - "Time spent for the test (build->first run, timeout at 100min): 10min."
  - "The authors also provide all scripts to replicate all figures of the paper."
  - It states **no** per-model timeout, memory cap, or success criterion for the 10k run. Those would be in the paper, which is absent. I did not guess them. The README's only resource statement is that the hardest model needed ">32GB memory" (`ftetwild/README.md`).
- **Pathology handling**
  - *Envelope:*
    - The default is sampled: sample spacing `dd = eps_input/1.5`, usable eps `eps_input - dd/sqrt(3)` (`ftetwild/src/Parameters.h:119-130`), checked by `sample_triangle_and_check_is_out` (`ftetwild/src/LocalOperations.cpp:714-790`) against the geogram tree (`ftetwild/src/AABBWrapper.h:118-170`).
    - `FLOAT_TETWILD_WITH_EXACT_ENVELOPE` (`ftetwild/CMakeLists.txt:49,139-142`) switches to the exact FastEnvelope (`ftetwild/cmake/FloatTetwildDependencies.cmake:179-185`, wangbolun300/fast-envelope @ 520ee04b, not acquired) via `is_out_sf_envelope_exact` (`AABBWrapper.h:179-192`).
    - CI pins `envelope: [OFF]`, so the exact envelope is never built in CI.
    - An `--envelope-log` option dumps up to 1e5 (triangle, is_out) rows to CSV (`LocalOperations.cpp:738-753`). That is a ready-made differential corpus, sampled vs exact, but nothing consumes it.
  - *Exactness:*
    - Positions are double. Orientation uses geogram `PCK::orient_3d` (`ftetwild/src/external/Predicates.cpp:15-17`).
    - Rationals are used only where floats blow up. AMIPS energy above 1e8 is recomputed in `triwild::Rational` (`LocalOperations.cpp:1240-1300`), and `orient_rational` exists (`ftetwild/src/TriangleInsertion.cpp:3524-3530`).
    - A degenerate tet in the energy evaluation returns +infinity (`LocalOperations.cpp:1269-1274`), so NaN is turned into "reject this operation".
    - Tolerances are hard-coded: `SCALAR_ZERO` 1e-8 (double) or 1e-6 (float) (`ftetwild/src/Types.hpp:19-26`).
  - *Termination:*
    - The main loop is bounded by `max_its` (default 80, `Parameters.h:68`). It breaks when max energy <= `stop_energy` and all input faces are inserted.
    - It also breaks when the minimum edge length has been hit and N=5 passes show no improvement (`ftetwild/src/MeshImprovement.cpp:157-216`).
    - The envelope grows by `eps_delta` when energy is stuck above 1e3 after `max_its/4` passes (`MeshImprovement.cpp:181-190`).
    - No wall clock, no memory limit, although `get_mem.h`/`getRSS.c` exist for stats (`ftetwild/src/Statistics.h:11`).
  - *Partial results are reported as success:*
    - Faces that fail to insert are retried each pass (`MeshImprovement.cpp:491-512`). `is_input_all_inserted` only gates the stopping rule (`TriangleInsertion.cpp:518-519`).
    - `main` ends with `return EXIT_SUCCESS` whether or not every face was inserted (`ftetwild/src/main.cpp:654`).
    - After the Delaunay stage, any inverted tet prints "EXIT_INV" and calls `exit(0)`, a **success** exit code with no output (`ftetwild/src/FloatTetDelaunay.cpp:244-250`).
    - `pausee()` and `myassert` are neutered: `pausee` returns immediately (`LocalOperations.cpp:1195-1197`), and `myassert` prints and continues (`TriangleInsertion.cpp:3254-3259`).
  - *Load failures* write an empty mesh and return `EXIT_FAILURE` (`main.cpp:408-416`). This is the only explicit refusal path.
- **Fuzzing / mutation / property-based / metamorphic / differential / performance-regression** -- None compiled. `test_tree.cpp` would have been differential (vs geogram) plus a benchmark, but it is disabled. Sanitizer options exist (`CMakeLists.txt:48-55`, `tests/CMakeLists.txt:26-28`), but CI does not enable them.
- **CPU/GPU or cross-platform consistency** -- CI matrix: ubuntu-22.04 / latest, macOS-13 / 14 / 15 / latest (x86 and ARM64), windows-2022, in Debug and Release (`continuous.yml`). The only cross-platform evidence is the bunny smoke run and the 0-or-666 tolerance.
- **Known-bad / expected-failure handling** -- Tests disabled by commenting them out (`tests/CMakeLists.txt:16-19`, `test_table.cpp:12-28,55-57`). There are no xfail lists.
- **Worth adopting for BRepGPU**
  - (a) The 0-vs-666 pair (`test_predicates.cpp:31-78`): the same geometry in two decimal spellings gives different code paths. Do the same for trim-curve control points written as STEP decimal text vs the binary double. It probes parse-rounding sensitivity in near-tangent trims.
  - (b) Envelope logging (`LocalOperations.cpp:738-753`) as a recipe. Record every accept/reject decision of the fast deviation check and replay it offline against an exact or dense oracle. This is a tier-6 differential test on real decisions, not on synthetic inputs.
  - (c) The `EXIT_INV` -> `exit(0)` and `return EXIT_SUCCESS` with uninserted faces (`FloatTetDelaunay.cpp:248-249`, `main.cpp:654`) are concrete examples of "silent bad mesh". Our harness should treat the exit code as untrusted and always re-verify the output.
  - (d) The offline-generated case table plus a structural invariant test (`build_cut_table.py`, `test_table.cpp`). If our GPU path uses lookup tables (for example, trim-cell classification), generate them by script and test their invariants.

---

#### Thingi10K (Thingi10K/Thingi10K rev cbac53559cfade2c4ffb2ef1ae8cae88462eaed4; the dataset itself was not acquired)

- **Test inventory**
  - `thingi10k/tests/test_thingi10k.py:5-15` downloads the npz variant and asserts that `genus>=1000` returns exactly 3 entries. For each, it checks that the file exists and that the loaded V/F row counts match the metadata `num_vertices`/`num_facets`.
  - Cache-clearing tests follow (`test_thingi10k.py:18-37`).
  - CI (`thingi10k/.github/workflows/deploy.yml`) only builds a wheel on tags, runs `twine check`, and uploads to PyPI. It never runs pytest.
- **Oracles** -- Tier 3: a stored count (3 models with genus >= 1000) and metadata-vs-file count consistency. No geometric oracle.
- **Corpora**
  - *Provenance:*
    - Every mesh of every "featured" thing on Thingiverse from 2009-09-16 to 2015-11-15. Featured status is chosen by Thingiverse staff for "design, beauty and manufacturability" (paper, section 2).
    - 10,000 models from 2,011 things by 1,083 designers. Formats: 9,956 .stl, 42 .obj, 1 .ply, 1 .off (paper, section 3).
  - *README statistics* (`thingi10k/README.md`): 50% non-solid, 45% self-intersecting, 31% coplanar self-intersections, 26% multi-component (the paper says 29%), 22% non-manifold, 16% degenerate faces, 14% non-PWN, 11% open, 10% non-oriented.
  - *Errata* (`thingi10k/README.md`, Errata): 49911 truncated ASCII STL, 74463 and 286163 empty, 81313 an OBJ containing NURBS curves/surfaces, 77942 corrupt binary STL. The package skips four of these via `CORRUPT_FILE_IDS = {49911, 74463, 286163, 77942}` (`thingi10k/src/thingi10k/_builder.py:46,471,493`). 81313 is *not* skipped.
  - *Downloads:*
    - Hugging Face `Thingi10K/Thingi10K`, pinned to tag `v1.5.0` (`_builder.py:42-45`).
    - Archives `Thingi10K_npz.tar.gz`, `Thingi10K.tar.gz` (raw), and `Thingi10K_tetwild_npz.tar.gz` (TetWild-remeshed surfaces) (`_builder.py:106-110`).
    - Metadata CSVs under `/metadata/`: `geometry_data.csv`, `contextual_data.csv`, `input_summary.csv`, `tag_data.csv`, `tetwild_geometry_data.csv` (`_builder.py:374-388`).
    - Mirrors: NYU Box, Google Drive (README).
- **Per-model flags and their definitions** (paper, section 3.1.2):
  - closed: every edge has >= 2 faces. In the package, `closed = num_boundary_edges == 0` (`_builder.py:593`).
  - oriented: zero signed incidence on every non-boundary edge.
  - vertex-manifold: the one-ring is a disc.
  - edge-manifold: exactly 2 faces per non-boundary edge.
  - no degeneracy: non-collinear vertices, by exact predicates.
  - no self-intersection: face pairs share only nothing, a vertex, or an edge, by exact predicates.
  - no coplanar intersection.
  - PWN: piecewise-constant winding number.
  - solid: PWN, self-intersection-free, and inducing a {0,1} winding field.
  - Also recorded: aspect ratio, intrinsic Delaunay, components, Euler characteristic, and area/valence/dihedral/aspect percentiles (`_builder.py:49-103`).
- **Filtering:**
  - `thingi10k.dataset(...)` (`thingi10k/src/thingi10k/_utils.py:185-298`) supports exact booleans (`closed`, `self_intersecting`, `vertex_manifold`, `edge_manifold`, `oriented`, `solid`, `pwn`; lines 69-84), regex on `license/category/subcategory/name/tags` (89-115), and inclusive ranges on `num_vertices/num_facets/num_components/num_boundary_edges/euler` (118-146).
  - `genus` is derived as (2-euler)/2 and restricted to single-component, closed, vertex-manifold, even-Euler models (149-182).
  - CLIP text search is optional (291-295).
  - **Gap:** `geometry_data.csv` carries `num_geometrical_degenerated_faces`, `num_combinatorial_degenerated_faces`, `num_duplicated_faces`, `num_self_intersections` and `num_coplanar_intersecting_faces` (`_builder.py:52-60`). The exposed dataset features omit all of them (`_builder.py:320-343`, `_generate_examples` 575-598). You cannot filter on "has degenerate faces" or "has duplicated faces" through the API. You would read the CSV directly.
- **What makes the models hard** (paper, section 3):
  - Triangle-soup STL, which needs welding.
  - Multi-component models whose components *overlap* to form the printed shape, so per-component processing is wrong.
  - Coplanar overlapping faces.
  - Extreme triangle aspect ratios. Paper Fig. 7 plots the max aspect ratio on a log axis running to 1e15.
  - Very high genus: the package's own test expects exactly 3 models with genus >= 1000 (`thingi10k/tests/test_thingi10k.py:8-9`).
  - Large and fragmented models. Fig. 3 is labelled "Thingi10K: 2.8M vertices". Fig. 5 labels component counts up to #C=8064, but which dataset each label belongs to is not recoverable from the text extraction.
  - Inverted orientation regions.
  - Open sheets.
  - The paper notes that assuming self-intersection-free input "would automatically exclude 45% of inputs".
- **Pathology handling** -- The package refuses empty files (`_builder.py:603-613`) and wraps loader errors as `ValueError` (`_utils.py:332-333`). Truncated or corrupt files outside `CORRUPT_FILE_IDS` would reach `lagrange.io.load_mesh`.
- **Fuzzing / mutation / property / metamorphic / differential / perf** -- None.
- **CPU/GPU or cross-platform consistency** -- None.
- **Known-bad / expected-failure handling** -- `CORRUPT_FILE_IDS` is a skip list, and the README errata section documents known-bad files.
- **Worth adopting for BRepGPU**
  - (a) The methodology, not the meshes. Thingi10K is a triangle corpus, so it does not exercise B-rep tessellation directly.
  - (b) The pattern to adopt: an ABC/STEP sweep with a per-model *flag table* computed once by an exact oracle (closed, oriented, manifold, self-intersecting, degenerate, PWN, solid). Our robustness sweep can then be stratified ("all models with seam-crossing trims and coarse tolerance") and failure rates reported per stratum.
  - (c) The errata/skip-list practice (`_builder.py:46`), with the reason recorded per ID.
  - (d) The two-variant design (raw, and the TetWild-cleaned surface). Keep the raw STEP alongside a healed version so tessellator failures can be attributed to the input or to the mesher.
  - (e) The vertex-count and component-count extremes are a reminder to size the GPU path's buffers by data, not by assumption.

---

#### libigl (rev 7100764c2a2833284a8bdfa9b948d2b3bf124624; data libigl-tests-data rev 19cedf96d70702d8b3a83eb27934780c542356fe)

Harness:
- Catch2 v2 (`libigl/tests/main.cpp`). One executable per module via `igl_add_test` with `catch_discover_tests` (`libigl/cmake/igl/igl_add_test.cmake`).
- Core globs `tests/include/igl/*.cpp` (`libigl/cmake/igl/modules/core.cmake:65-66`). The copyleft CGAL executable globs `copyleft/boolean/*.cpp` and `copyleft/cgal/*.cpp` (`libigl/cmake/igl/modules/copyleft/cgal.cmake:27-31`).
- Test data comes from `https://github.com/libigl/libigl-tests-data` at the pinned rev, via `LIBIGL_DATA_DIR` (`libigl/cmake/recipes/external/libigl_tests_data.cmake:10-11,20`).
- In Debug on Linux, `main.cpp` enables FPE traps (FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW). `test_common.h` provides `IGL_PUSH_FPE`/`IGL_POP_FPE` and hides `IGL_DEBUG_OFF` tests in Debug.
- CI (`libigl/.github/workflows/continuous.yml`):
  - Ubuntu-24.04 and macos-15, **Release only**, static and header-only builds, `LIBIGL_COPYLEFT_CGAL=ON`, glfw tests off. Windows-2022, Release.
  - `IGL_NUM_THREADS=4`.
  - No sanitizers, no fuzzers, no nightly corpora.
  - Because CI is Release only, the FPE traps never run in CI.
- `[slow]` is a plain tag, not hidden, so those tests run.

- **Test inventory** (fault class: representative citations)
  - *2D constrained triangulation (Triangle wrapper):*
    - Square, 4 points, flags `QcYY`: expects 4 vertices and 2 faces (`libigl/tests/include/igl/triangle/predicates.cpp:6-30`).
    - Area-bounded refine: every output area < 0.0625 (`triangle/refine.cpp:12-29`).
    - "Inner pinch" outline with a near-collinear notch vertex at y=1.00000001. The test triangulates the convex hull, removes the cavity by **winding number**, then refines. It checks 12 then 9 faces, and that the refined count is < 0.15x of the naive quality mesh (`triangle/refine.cpp:32-119`).
  - *Point insertion into a triangulated surface* (`triangle/remesh_at_points.cpp`). A property harness (lines 16-94) checks:
    - V is a prefix of VV.
    - Every sample point lands within 1e-14 of its barycentric position and is referenced.
    - Every output face is coplanar with, and identically oriented to, its parent.
    - Child areas sum to the parent area within 1e-12 relative.
    - Cases: face/vertex/boundary-edge points, a point on a shared edge splitting both faces, a **non-manifold edge with one flipped face**, where two points on the same edge must keep a consistent order in all three faces (202-253), all cube edge midpoints giving exactly 4-1 subdivision that stays closed manifold (255-300), and 1-4 dyadic-rational interior points per face on the knight (302-340).
  - *CGAL CDT:* square annulus with a hole seed, compared to exact ground-truth V and F *including face order* (`copyleft/cgal/triangulate.cpp:5-40`). CGAL Delaunay on 4 points is compared to ground truth via `unique_simplices` (`copyleft/cgal/delaunay_triangulation.cpp:6-26`).
  - *Delaunay with inconsistent tolerance predicates:*
    - Issue #521 (`libigl/tests/include/igl/delaunay_triangulation.cpp:8-102`) feeds 16 SketchUp-scale points, including (4.55e-13, 2.33e-12), into user-supplied `orient2d`/`incircle` that treat |val| < 0.0005 as zero.
    - It checks only F non-empty, 3 columns, and indices in [0,16) (98-101). This is a **termination regression**.
    - The implementation's Lawson loop `while(!all_delaunay)` has no iteration bound (`libigl/include/igl/delaunay_triangulation.cpp:47-56`). A regression would hang until the CI job timeout.
  - *Delaunay property:* `is_delaunay` on Delaunay vs flipped two-triangle configurations (`is_delaunay.cpp:4-32`). Intrinsic Delaunay is checked by `is_delaunay` all-true on a skewed grid (`intrinsic_delaunay_triangulation.cpp:34-54`) and by `is_intrinsic_delaunay` all-true on a 20x20 "peaks" surface (56-87).
  - *Ear clipping:* polygon and its reversal must both give n-2 triangles (`predicates/ear_clipping.cpp:4-24`).
  - *Exact predicates:*
    - orient2d, incircle, orient3d and insphere on collinear, coincident (a,a,a) and machine-epsilon-offset points (`predicates/predicates.cpp:16-60`).
    - Batched orient3d (`predicates/orient3d.cpp:11-63`).
    - Collinear overlap, endpoint touch, and a 1e-14 gap must report false (`predicates/segment_segment_intersect.cpp:5-43`).
    - `point_in_convex_hull` with a triangle hull, a collapsed segment hull, a single-point hull, and a "double-degenerate" hull of collinear decimals (`predicates/point_in_convex_hull.cpp:43-102`).
    - Cubic winding number of all-coincident and collinear control polygons (`predicates/cubic_winding_number.cpp:50-75`).
  - *Self-intersection detection/resolution:*
    - Cube: none found. Cube plus a slicing triangle: exactly 9 faces involved, all pairs including the added face (`predicates/find_self_intersections.cpp:14-59`).
    - A 9-petal "rose" fan with an analytic pair list (62-94).
    - Shared edge is not an intersection, then a moved vertex creates a coplanar overlap with `CP(0)` true (96-118).
    - Two real-world near-coplanar 17-digit triangle pairs that must report *no* intersection (120-162).
    - The knight upsampled twice must stay intersection-free (164-190).
    - Extraction of intersection segments vs exact expected points (192-227).
    - CGAL `remesh_self_intersections`: resolve exactly, then re-detect and require `IF.rows()==0`, over `all_meshes()` including non-manifold `truck.obj` (`copyleft/cgal/remesh_self_intersections.cpp:26-64`). `CubeWithFold` has no assertion (10-24).
  - *Winding number:*
    - Bunny call with no assertion (`winding_number.cpp:4-14`).
    - Fast winding number vs exact winding number at the vertex centroid, soup and point-cloud forms, within 1e-2, on bunny/elephant/hemisphere (`fast_winding_number.cpp:55-107`). A single-point cloud is checked against cached values within 1e-15 (17-53).
    - Embree antipodal winding vs `igl::winding_number` on a fixed-seed random **non-manifold soup**: 10 vertices, 30 triangles, 100 queries, within 1e-5 (`embree/winding_number_antipodal.cpp:80-126`). Closed-mesh integrality is also checked (13).
  - *Decimation:*
    - Hemisphere halved: birth-index normals within 0.02 (`decimate.cpp:11-39`).
    - Closed genus-0 meshes decimated to 0 faces must yield exactly a tet (41-72).
    - qslim of a 5-gon cylinder down to its 10 ring vertices must have two-sided sampled Hausdorff ~0 within 2e-10 (`qslim.cpp:9-44`).
    - `collapse_edge_would_intersect_mesh`: transversal obstacle reported, distant obstacle not, vertex-touching obstacle ignored (`collapse_edge_would_intersect_mesh.cpp:50-117`).
    - `progressive_hulls`: volume never shrinks, and two runs are bitwise identical (`progressive_hulls.cpp:25-62`).
  - *Topology:*
    - Boundary loops: cube has 0. Bunny has 5 loops, sum 223, max 80, min 22, "taken from meshlab" (`boundary_loop.cpp:7-53`).
    - `is_edge_manifold`: true on the manifold list, false on `truck.obj` (`is_edge_manifold.cpp:4-24`).
    - `split_nonmanifold`: edge fan, vertex-boundary, disk flap/tent, vertex kiss, non-orientable, and "flap" with **two face orderings** (`split_nonmanifold.cpp:185-243`). "Crosses" are two orthogonal grids welded at 2^p resolution for p=1..6 (245-276). All check edge- and vertex-manifold output and the `I(SF)==F` provenance (13-32).
    - `orient_halfedges` invariants on the manifold list (`orient_halfedges.cpp:12-80`). Note: this is edge orientation for Crouzeix-Raviart, not face-orientation repair. No test exists for `bfs_orient` or `orient_outward`.
  - *Booleans / outer hull:*
    - Union of `two-boxes-bad-self-union.ply` with an empty mesh, minus-test, cube intersected with itself. All require no exterior edges, vertex and edge manifold, and genus by Euler formula (`copyleft/boolean/mesh_boolean.cpp:60-108`).
    - `outer_facet` on duplicated opposite faces, fully degenerate `degenerated.obj`, inverted normals, and `sliver_tet.ply`: a valid index plus the expected `flipped` (`copyleft/cgal/outer_facet.cpp:5-97`).
    - `order_facets_around_edges` with duplicated faces, plus "NormalSensitivity": epsilon-different stored normals give different, both-expected orders (`copyleft/cgal/order_facets_around_edges.cpp:119-205`).
    - Points within 1 ULP inside/outside a cube face and corner (`copyleft/cgal/points_inside_component.cpp:26-83`).
    - `outer_hull_legacy` with no assertion, commented "Is this just checking that it doesn't crash?" (`copyleft/cgal/outer_hull_legacy.cpp:13-14`).
  - *Tet meshing (TetGen):* the Schoenhardt prism (untetrahedralizable without Steiner points) only checks `V.rows() <= TV.rows()` (`copyleft/tetgen/tetrahedralize.cpp:81-103`). Quad face (105-123).
  - *Cage / offset validity:* `lazy_cage` checks enclosure via fast-winding signed distance and no transversal intersection. It includes a regression where a coarse grid used to let the cage pierce the input (`lazy_cage.cpp:12-82`).
  - *I/O:* a nonexistent PLY returns false with empty V/F (`readPLY.cpp:65-77`). `increment_ulp` on lowest/0/max/inf/NaN (`increment_ulp.cpp:5-15`).
  - *Threading:* parallel_for equals serial bitwise (`parallel_for.cpp:110-128`). A bounded thread count under nesting guards issue #2412, fast_winding_number spawning unbounded threads (`parallel_for.cpp:699-760`).

- **Oracles**
  - Tier 7 (spec/analytic): predicate signs (`predicates/predicates.cpp`); rose pair list (`find_self_intersections.cpp:77-92`); extracted intersection points (`find_self_intersections.cpp:220-225`); decimate to tet (`decimate.cpp:61-68`); qslim cylinder exact (`qslim.cpp:41-43`); 4-1 subdivision counts (`remesh_at_points.cpp:292-299`); `increment_ulp`.
  - Tier 6 (model/differential): embree vs reference winding (`winding_number_antipodal.cpp:80-126`); fast vs exact winding (`fast_winding_number.cpp:76,100`); parallel vs serial.
  - Tier 5 (property): `remesh_at_points` area-tiling and orientation harness; `remesh_self_intersections` "resolve then detect none"; boolean manifold, closed and genus; `split_nonmanifold`; `is_delaunay` all-true; `lazy_cage` enclosure; refine area bound.
  - Tier 4 (metamorphic):
    - Upsampling preserves "no self-intersection" (`find_self_intersections.cpp:177-188`).
    - Polygon reversal (`ear_clipping.cpp:20-23`).
    - Face-order permutation (`split_nonmanifold.cpp:215-243`).
    - Repeated-run determinism (`progressive_hulls.cpp:48-62`).
    - **Broken:** the intended one in `copyleft/cgal/hausdorff.cpp:28-53` (an upsampled face's upper bound must not exceed its parent's). A `break;` at line 52 exits the loop after `j=0`, so the `REQUIRE` at line 49, guarded by `j>0`, never runs. The test checks nothing.
  - Tier 3 (stored answers): bunny boundary counts from MeshLab (`boundary_loop.cpp:34-52`); cached fast-winding values (`fast_winding_number.cpp:46-52`); refine-inner-pinch 12/9 faces (`refine.cpp:103,105`); CGAL triangulate exact face list *and order* (`copyleft/cgal/triangulate.cpp:28-39`, brittle to any internal change); peel-layers 486/708 (`peel_outer_hull_layers.cpp:19-20`); cube-triangle "9 triangles" (`find_self_intersections.cpp:55`).
  - Tier 1/2 (no or implicit oracle): `winding_number: bunny`; `remesh_self_intersections: CubeWithFold`; `outer_hull_legacy`; `PeelOuterHullLayers: CubeWithFold` (`peel_outer_hull_layers.cpp:54-80`); `delaunay_triangulation_issue_521` (termination plus index range only); `tetrahedralize: schoenhardt` (one weak inequality); `hausdorff` (broken as above).
- **Corpora** -- `libigl-tests-data/`: 55 files, none generated.
  - *Plain clean meshes:* `cube.obj`/`cube.off` (8v/12f), `tet.obj`, `decimated-knight.obj` (502/1000) with .mesh/.tgf/.dmat companions, `TinyTorus.obj` (24/48, genus 1), `hemisphere.obj` (145/256, open), `cheburashka.off`, `elephant.off` (78,792/157,160).
  - *Open meshes with holes:* `bunny.off` (34,834/69,451, 5 holes); `bunny.ply` (35,947 vertices for the same 69,451 faces, so it carries unreferenced vertices).
  - *Hard ones:*
    - `degenerated.obj`: 7 vertices, 24 faces. Each of 12 axis triangles fanned around the origin appears with **both orientations**, giving zero volume, duplicated opposite faces, and non-manifold edges.
    - `sliver_tet.ply`: 5,009 vertices, 4 faces. The four referenced vertices span ~0.06 in x but ~0.0014 in z (near-coplanar sliver), and 5,005 are unreferenced.
    - `cube_with_fold.ply`: 26/48, binary, from PyMesh, a cube with an inward fold, so self-touching.
    - `two-boxes-bad-self-union.ply`: 486/708, overlapping boxes from a bad union.
    - `truck.obj`: 2,956/4,770, non-manifold.
    - `duplicated_faces_{V,F,N1,N2}.dmat`: a duplicated-face configuration with two epsilon-different normal sets.
    - `extrusion.obj`: 6/8, an open prism from SketchUp-style coordinates (-9.439857999999999).
    - `boolean_minus_test_{cube,green}.obj`: SketchUp exports in inches.
  - Larger parametrization/FE assets (`3holes`, `camelhead`, `annulus`, `face.obj`, `arm`, `hand.mesh`, `car.msh`, the TMS `.msh`) feed non-meshing tests.
  - Several names referenced by tests (`quad_cube.ply`, `test_binary_sphere*.msh`, `edge-fan.ply`, and others) are written by the tests themselves. `mesh_error.ply` is intentionally absent.
- **Pathology handling** (runtime, beyond the tests)
  - The Triangle wrapper validates inputs only with `assert` (`libigl/include/igl/triangle/triangulate.cpp:56-62`), so the checks vanish in Release.
  - The unbounded Lawson loop above (`delaunay_triangulation.cpp:47`).
  - `remesh_intersections` guards degeneracy with `assert(!P.is_degenerate())` (`libigl/include/igl/copyleft/cgal/remesh_intersections.cpp:102`), again Release-invisible.
  - Exact constructions use Epeck throughout copyleft/cgal, with a `slow_and_more_precise_rounding` option for rounding back (`libigl/include/igl/copyleft/cgal/RemeshSelfIntersectionsParam.h:20-32`).
- **Fuzzing / mutation / property-based / metamorphic / differential / performance-regression**
  - Fuzzing: none. The only randomized geometry is the fixed-seed soup (`winding_number_antipodal.cpp:82-104`).
  - Property, metamorphic and differential: as listed above.
  - Performance: `CATCH_CONFIG_ENABLE_BENCHMARKING` is defined (`igl_add_test.cmake`). Only `slice_sorted: benchmark` (`slice_sorted.cpp:55`, Debug-hidden) and `[timing]` parallel_for cases exist (`parallel_for.cpp:495,568`). There are no thresholds and no regression tracking.
- **CPU/GPU or cross-platform consistency** -- Linux, macOS and Windows in Release, header-only and static (`continuous.yml`). No tolerance-per-platform files. parallel_for serial-equivalence is the only determinism check.
- **Known-bad / expected-failure handling** -- No `[!shouldfail]`/`[!mayfail]` tags. `IGL_DEBUG_OFF` hides one slow test in Debug. Weak tests are left in place and flagged by comments (`outer_hull_legacy.cpp:13`).
- **Worth adopting for BRepGPU**
  - (a) The `check_remesh_at_points` harness (`triangle/remesh_at_points.cpp:16-94`) transfers almost verbatim to parameter-space CDT of a trimmed face. Per input cell, check that children tile the parent's area to 1e-12 relative, have one orientation sign and no zero-area triangles, that inserted points are referenced, and that original vertices are a prefix. This is a tier-5 oracle needing no golden file.
  - (b) The consistent-ordering-across-a-flipped-non-manifold-edge case (`remesh_at_points.cpp:235-252`). This is our seam/shared-edge problem: points inserted on a B-rep edge must appear in the same order in both adjacent faces' parameter-space meshes, even when the faces traverse the edge in opposite directions. It goes straight into the watertightness suite.
  - (c) `delaunay_triangulation_issue_521` (`delaunay_triangulation.cpp:66-102`): inject deliberately inconsistent epsilon predicates and assert termination. Adopt it with an explicit flip/iteration budget and a harness timeout. This is the "tolerance coarser than features must not hang" case.
  - (d) The near-coplanar 17-digit triangle pairs (`find_self_intersections.cpp:120-162`) and the ULP-offset inside/outside points (`points_inside_component.cpp:26-83`). These are cheap seeds for our self-intersection oracle's own validation. Test the oracle before trusting it.
  - (e) Upsample-preserves-property metamorphic tests (`find_self_intersections.cpp:164-190`). For us: refine the tolerance, and the output must still be self-intersection-free and watertight.
  - (f) The differential winding-number test on a fixed-seed non-manifold soup (`winding_number_antipodal.cpp:80-126`). Use it as the pattern for testing a GPU winding or inside-outside kernel against a CPU reference.
  - (g) Data files `degenerated.obj`, `sliver_tet.ply` and `duplicated_faces_*.dmat` as inputs to our *post-tessellation* checks: manifold, orientation and sliver detectors must flag them.
  - (h) The pitfall: the broken Hausdorff test (`copyleft/cgal/hausdorff.cpp:52`). Every BRepGPU oracle test should include a mutation check that it fails when the predicate is negated.

---

#### Blind spots this project reveals (missing from the BRepGPU bootstrap plan)

1. **Exit code is not truth.** fTetWild returns success after inverted tets (`FloatTetDelaunay.cpp:249`) and with uninserted faces (`main.cpp:654`). TetWild outputs unrounded vertices with only a debug log (`tetwild.cpp:81-89`). Our harness must re-verify every output independently, not trust "success". The plan has the oracles but does not say they override the mesher's own status.
2. **Inconsistent-predicate termination.** A tolerance-thresholded orient/incircle can make Lawson flipping cycle (`libigl delaunay_triangulation.cpp:47`, issue #521). The plan's "tolerance coarser than features" is about geometry. Add "tolerance-based predicates inside the CDT", with a flip budget and a test that injects epsilon predicates.
3. **Oracle self-validation.** libigl's Hausdorff test silently checks nothing (`hausdorff.cpp:52`), and several tests assert nothing. Add mutation testing of the oracles themselves: negated predicates, perturbed outputs.
4. **Sampled-deviation soundness.** Dense sampling only gives a lower bound unless the tolerance is shrunk by the sample covering radius (TetWild `State.cpp:36-41`, fTetWild `Parameters.h:128`). "Dense deviation vs exact geometry" needs that correction, or an exact or interval check, or it will pass meshes that violate tolerance between samples.
5. **Decimal-spelling sensitivity.** The same geometry as "27.47818" vs "27.478179999999998" takes different code paths (fTetWild `test_predicates.cpp:31-78`). Add STEP text-vs-binary round-trip variants to the mutation fuzzer.
6. **Absolute tolerances hidden in preprocessing.** TetWild welds at 1e-10 absolute (`Preprocess.cpp:104`). fTetWild uses `SCALAR_ZERO` 1e-8 (`Types.hpp:24`). The 1e6:1 scale family should also scale the *whole model* by 1e+-6 and assert identical topology. That is a metamorphic test the plan does not list.
7. **Consistent ordering of points on shared edges across faces with opposite traversal** (`remesh_at_points.cpp:235-252`). This is distinct from "watertight": a mesh can be watertight per face pair yet T-junctioned when three or more faces meet, or when edge orientation flips.
8. **Stratified failure reporting.** Thingi10K's per-model flag table lets failure rates be reported per pathology class. The ABC sweep in the plan has no such stratification, so a regression concentrated in, say, periodic-seam models would be diluted.
9. **Memory as a first-class failure.** Both wild meshers simply tell users to get more RAM (>100GB, >32GB). Our "no runaway memory" needs a per-model memory cap in the harness that counts as failure.
10. **Debug-only safety checks.** libigl's input validation is `assert` (`triangle/triangulate.cpp:56-62`, `remesh_intersections.cpp:102`), and its FPE traps are Debug-only while CI is Release-only. Run the robustness sweep in a checked build too, with FPE traps on, so NaN/inf generation is caught.

### Cluster 6: Metric-driven remeshers: Mmg, Mmg CI corpora, JIGSAW

Citation roots: mmg and its corpora are cited relative to `quarantine/sources/`. JIGSAW is not under `sources/`, so its citations are relative to `quarantine/` (`whole-mesher-study/jigsaw/...`). Everything here comes from reading files. Nothing was built or run.

---

#### mmg (MmgTools/mmg v5.8.0, commit 4d8232c8aebfed877935d75d4d4a67e850962422)

**How a test passes (applies to everything below).** Nearly every test is a CLI run registered with `ADD_TEST` (376 active registrations in `mmg/cmake/testing/*.cmake`; the 45 in `mmg_tests.cmake` run twice, once each for mmgs and mmg3d). There is no output comparison step. Exit status follows `MMG5_SUCCESS=0 / MMG5_LOWFAILURE=1 / MMG5_STRONGFAILURE=2` (`mmg/src/common/libmmgtypes.h:49-65`). LOWFAILURE means "failed but a conforming mesh was saved" (`mmg/src/mmgs/mmgs.c:260-262`), and the CLI returns `ier` as its exit code (`mmg/src/mmgs/mmgs.c:560`, `MMGS_RETURN_AND_FREE(mesh,met,ls,ier)`). So a plain exit-code test asserts "no internal failure path was taken", which is a bit more than "no crash". When `PASS_REGULAR_EXPRESSION` is set, ctest ignores the exit code entirely. A test can then crash after printing the expected string and still pass.

**Test inventory**
- *File I/O and format round trips* (Gmsh ascii/binary, VTK vtk/vtu/vtp, Tetgen, Medit; iso, aniso and level-set fields): `mmg/cmake/testing/mmg_tests.cmake:67-140`, `mmg/cmake/testing/mmg3d_tests.cmake:295-378`, `mmg/cmake/testing/mmg2d_tests.cmake:209-366`.
- *Abnormal termination / refusal paths*: zero-extent mesh ("unable to scale mesh", `mmg/cmake/testing/mmg_tests.cmake:187-192`); output path made unwritable by turning it into a directory (`mmg/cmake/testing/mmg_tests.cmake:29-44,194-208`); solution that fails to load (`mmg/cmake/testing/mmg3d_tests.cmake:386-393`); contradictory options `-hsiz` with `-hmax`/`-hmin` must print "Mismatched options" (`mmg/cmake/testing/mmg_tests.cmake:313-326`); missing or wrong parameter file (`mmg/cmake/testing/mmg3d_tests.cmake:654-686`); `-optim` with `-ls` must be refused (`mmg/cmake/testing/mmgs_tests.cmake:457-475`).
- *Non-manifold input*: `NM_Cube`, `NM_Complex`, `cone-nm` (`mmg/cmake/testing/mmg3d_tests.cmake:34-38`), `Cow_NM` at hausd 10 and 20, and `Car_NM` (`mmg/cmake/testing/mmgs_tests.cmake:244-247,340-348`). Level-set multimaterial plus open boundary plus non-manifold check: `mmg/cmake/testing/mmg3d_tests.cmake:689-692`, `mmg/cmake/testing/mmg2d_tests.cmake:563-567`.
- *Non-orientable / mis-oriented surfaces*: Moebius strip run with `-d -nr` (`mmg/cmake/testing/mmgs_tests.cmake:234-237`); level set on a mesh with inconsistently oriented triangles (`mmg/cmake/testing/mmgs_tests.cmake:507-511`).
- *Open boundaries*: peninsula and island with `-opnbdy` (`mmg/cmake/testing/mmg3d_tests.cmake:503-540`); open-surface `optim+aniso` (`mmg/cmake/testing/mmgs_tests.cmake:410-419`).
- *Ridge/corner detection (`-ar`, `-nr`)*: house mesh with "lot of reference edges, ridges, corners and singularities" (`mmg/cmake/testing/mmg_tests.cmake:410-425`); Santa at `-ar 90` (`mmg/cmake/testing/mmg3d_tests.cmake:110-111,209-210`, LONG_TESTS only); `-ar 0.02`, which makes almost every edge a ridge (`mmg/cmake/testing/mmg3d_tests.cmake:854-860`); ridge preservation under level-set splitting (`mmg/cmake/testing/mmg3d_tests.cmake:731-737`). The default ridge threshold is cos 45 deg (`mmg/src/common/mmgcommon_private.h:90`). The dihedral test is `mmg/src/mmgs/analys_s.c:445-469`.
- *Hausdorff (`-hausd`) sweeps*: 0.0001 to 20 (`mmg/cmake/testing/mmg3d_tests.cmake:181-213`; `mmg/cmake/testing/mmgs_tests.cmake:244-247`). Local per-reference hausdorff on two spheres (refs 36/37/38 at 0.005/1/0.1, per `mmg-ci-tests/mmg/mmg/HausdLoc_2Spheres/README`): `mmg/cmake/testing/mmg_tests.cmake:228-255`. The default hausd is 0.01 (`mmg/src/common/mmgcommon_private.h:121`).
- *Required / frozen entities*: required vertex, edge, triangle and ridge on a multidomain interface (`mmg/cmake/testing/mmg_tests.cmake:328-342`; `mmg/cmake/testing/mmg2d_tests.cmake:111-133`); frozen parallel-interface triangles (`mmg/cmake/testing/mmg3d_tests.cmake:854-863` plus `mmg/cmake/testing/code/compare-para-tria.c`); required vertex under `xreg` (`mmg/cmake/testing/code/req-vert-3d.c`).
- *Level-set discretization*: about 60 tests. They cover nonzero isovalue, `-rmc` bubble removal including a "max pile bug" regression, snapping, `-lssurf` and multimaterial (`mmg/cmake/testing/mmg3d_tests.cmake:638-851`; `mmg/cmake/testing/mmgs_tests.cmake:254-569`; `mmg/cmake/testing/mmg2d_tests.cmake:531-839`).
- *Snapping that could create non-manifold vertices*: four hand-made 9-vertex, 8-triangle meshes (`mmg/cmake/testing/mmgs_tests.cmake:536-569`, `mmg/cmake/testing/mmg2d_tests.cmake:806-839`).
- *Coordinate regularization, including a "negative area" check*: `mmg/cmake/testing/mmg2d_tests.cmake:739-760`; `mmg/cmake/testing/mmg_tests.cmake:378-391`.
- *Precision*: single-precision versus double-precision Medit input (`MeshVersionFormatted 1/2`, `mmg/cmake/testing/mmg_tests.cmake:264-276`).
- *Duplicate elements*: duplicate triangle (`mmg/cmake/testing/mmg3d_tests.cmake:542-545`); orphan and unused points (`mmg/cmake/testing/mmg_tests.cmake:300-310`, `mmg/cmake/testing/mmg2d_tests.cmake:116-118,454-463`).
- *Mesh generation from boundary only (mmg2d)*: square, NACA, non-convex "ACDC", Garonne river (`mmg/cmake/testing/mmg2d_tests.cmake:471-523`).
- *Metric linear-algebra unit tests*: `mmg/cmake/testing/code/test_met3d.c`, `mmg/cmake/testing/code/test_met2d.c` (`mmg/cmake/testing/libmmg3d_tests.cmake:249`).

**Oracles**
- CLI exit-code tests (the large majority): **tier 2**. The exit code catches LOWFAILURE and STRONGFAILURE paths and, in Debug or RelWithAssert builds (the long-tests workflow uses RelWithAssert, `mmg/.github/workflows/long-tests.yml:26`), internal `assert`s. Nothing checks hausdorff achieved, quality, ridge count or topology. Hausdorff sweeps, `-ar` cases and non-manifold cases are tier 2 despite their names. The worst example is `mmgs_Car_NM`. Its README says "weird results: strange ridges + faces that disappear" (`mmg-ci-tests/mmgs/mmgs/Car_NM/README`) and it is filed under "Detected Bugs" (`mmg/cmake/testing/mmgs_tests.cmake:337-343`), yet it passes on exit code 0.
- `-d` (ddebug) runs: **tier 5, in-process**. They call `MMG5_chkmsh` (`mmg/src/mmgs/libmmgs.c:283`, `mmg/src/mmg2d/libmmg2d.c:206`), which checks adjacency symmetry, missing edges, unused vertices, ball consistency and unit normals (`mmg/src/mmgs/chkmsh_s.c:72-283`). Only about 8 tests pass `-d`, for example `mmg/cmake/testing/mmg2d_tests.cmake:31,147,157,506,512`, `mmg/cmake/testing/mmgs_tests.cmake:236,312`.
- `compare-para-tria`: **tier 5**. Every MG_PARBDY triangle in the input must reappear with the same vertex refs and coordinates within 1e-5, and the counts must match (`mmg/cmake/testing/code/compare-para-tria.c:110-246`).
- `ridge-preservation-in-ls-mode`: **tier 5**. Every input ridge that still exists in the output must still be tagged MG_GEO (`mmg/cmake/testing/code/ridge-preservation-in-ls-mode.c:107-152`). It is weak by design: split ridges are skipped (`:125-137`).
- `req-vert-3d/s`: **tier 5**. The coordinates of required vertex 1 must not move by more than 1e-5 after remeshing (`mmg/cmake/testing/code/req-vert-3d.c`).
- `test_met2d/3d`: **tier 7**. Eigenpairs, simultaneous reduction and metric intersection are checked against analytic answers, with tolerances of 10 to 100 times `MMG5_EPSOK=1e-15` (`mmg/src/common/mettools.c:456-519`). Weak point: the hardest cases are defined but the loop bounds skip them. Symmetric case 4 (nearly repeated eigenvalue 1.00495, off-diagonals around 1e-23) is excluded by `i < 3` (`mmg/cmake/testing/code/test_met3d.c:66-86`). Non-symmetric case 3 is excluded by `i < 2` (`:121`). Simultaneous-reduction case 2 is excluded by `i < 1` (`:164`).
- `isoref` pairs: **tier 4 (metamorphic), weak**. Relabeling the isosurface reference from 10 to 5 should give the same output, but the check only diffs `wc -wl` counts (`mmg/cmake/testing/mmg3d_tests.cmake:847-851`, `mmg/cmake/testing/mmgs_tests.cmake:302-306`, `mmg/cmake/testing/mmg2d_tests.cmake:660-664`).
- `ADD_RUN_AGAIN_TESTS`: **tier 2 with a metamorphic flavor**. The remesher's own output is fed back as input with `-hgrad -1`, and only exit code 0 is required (`mmg/cmake/modules/macros.cmake:299-333`). It is enabled only when LONG_TESTS is set (`mmg/cmake/modules/mmg3d.cmake:216-222`).
- `LSSnapval`: **tier 7 for intent, checked by log regex**. The meshes and level-set values are hand-authored. The manifold cases must not print "unsnap at least 1 point" (FAIL_REGULAR_EXPRESSION) and the non-manifold cases must print it (`mmg/cmake/testing/mmgs_tests.cmake:536-569`). The runtime check behind it is `mmg/src/common/mmg2.c:640-735`.
- Refusal tests (PASS_REGULAR_EXPRESSION): **tier 7 for the refusal contract**, weak because they match log text and ignore exit code. `-val` WILL_FAIL tests (`mmg/cmake/testing/mmg3d_tests.cmake:421-427`, `mmg/cmake/testing/mmgs_tests.cmake:58-63`): **tier 1**. `-val` prints defaults and returns 0 from the parser (`mmg/src/mmgs/libmmgs_tools.c:133-136`), and the expected nonzero exit follows from that.
- The tests named `LeakCheck_*` do **not** check leaks in ctest. They only regex-match the error message. Valgrind runs only in the SonarQube Jenkins job, on three tiny examples, with ctest failures swallowed (`ctest ... || /usr/bin/true`) (`mmg/scripts/JENKINSFILE.sonarqube:37-41`).

**Corpora**
- Downloaded at configure time from `https://static.bordeaux.inria.fr/mmg/mmg{,2d,3d,s}.tgz`, using MD5 checks of `.version` files (`mmg/cmake/modules/LoadCiTests.cmake:40-104,155-158`).
- The mmg3d corpus was not acquired and was not read. Every `${MMG3D_CI_TESTS}` path (`mmg/cmake/testing/mmg3d_tests.cmake`, `mmg/cmake/testing/libmmg3d_tests.cmake:273-307`) points at it: NM_Cube, NM_Complex, cone-nm, Santa, Linkrods, M6, OptLs_*, test_para_tria, test_req_vert, LSMultiMat 3d-opn.
- An offline mode (`ONLY_VERY_SHORT_TESTS`) runs one cube from `libexamples/` (`mmg/cmake/modules/mmg3d.cmake:227-236`).
- See the mmg CI corpora section below for the inventory.

**Pathology handling**
- *Timeouts*: only CI-level `ctest --timeout 7200` (`mmg/.github/workflows/main-job.yml:343,350`). No per-test TIMEOUT property exists.
- *Iteration caps*: termination is built into the code rather than tested. Every adapt loop has a hard cap, for example `maxit=10` with `while(++it<maxit && nc+ns>0)` (`mmg/src/mmgs/mmgs1.c:1279-1330`), `it<40` (`:889`), and `maxit=5` for analysis (`mmg/src/mmgs/analys_s.c:761-800`). No test checks the cap.
- *Memory budget*: mmg owns its memory accounting. `memCur` is checked against `memMax`, which defaults to 50% of RAM (`mmg/src/common/mmgcommon_private.h:76-78,302-307,406-432`). Running past the budget prints "unable to allocate ... increase ... -m" and takes the failure path. The only `-m` tests (`-m 100` on small meshes: `mmg/cmake/testing/mmg3d_tests.cmake:406-409`, `mmg/cmake/testing/mmgs_tests.cmake:53-56`, `mmg/cmake/testing/mmg2d_tests.cmake:46-48`) check that a cap is accepted and the run succeeds. **No test runs out of the budget and checks for a graceful refusal.**
- *Degenerate extent*: a bounding-box extent below 1e-30 is refused (`mmg/src/common/scalem.c:69-73`). This is tested.
- *NaN/inf*: no input-coordinate check exists. `isfinite` appears only in asserts on metric eigenvalues (for example `mmg/src/mmg3d/mmg3d1.c:2165`, `mmg/src/mmgs/libmmgs_tools.c:994-1006`). No test feeds NaN.
- *Inconsistent orientation*: a BFS flips triangles to repair orientation. Where repair is impossible (Moebius), mmg tags the conflicting edge MG_REF+MG_NOM and continues instead of refusing (`mmg/src/mmgs/analys_s.c:138-150`).
- *Level-set snapping*: a snap is rejected if it would make an all-zero triangle or a non-manifold vertex ball (`mmg/src/common/mmg2.c:652-655,723-733`).
- *Predicates*: plain floating point with epsilons (`MMG5_EPSOK=1e-15`, `MMG5_EPSD=1e-30`, `mmg/src/common/mmgcommon_private.h:94-98`). No exact predicates.

**Fuzzing / mutation / property-based / metamorphic / differential / performance-regression**
- Fuzzing, mutation, sanitizers: none found. A grep for `fsanitize`, `MEMORYCHECK` and `fuzz` across the tree found nothing.
- Metamorphic: only `isoref` and `RUN_AGAIN` (above).
- Configuration matrix: `MMG_PATTERN` on/off (pattern splitting versus Delaunay insertion), `USE_POINTMAP`, SCOTCH renumbering, int32/int64 (`mmg/.github/workflows/main-job.yml:35-147`). Each configuration is checked only against itself. Outputs are never compared across configurations, so this is not differential testing.
- `-DMMG_COMPARABLE_OUTPUT` removes timestamps and git info from logs so they can be diffed (`mmg/.github/workflows/main-job.yml:194-198`; `mmg/src/common/libtools.c:46-58`). In this snapshot no step diffs anything, and `MMG_ERROR_RULE` is set but never consumed.
- A stored log `mmg-ci-tests/mmg2d/mmg2d/Circle/mmg2d_SimpleCircle.ref` and many stored `.o.meshb` outputs exist in the corpora but are not referenced by any test.
- Performance regression: none. Disabling cases that run too long is handled by hand (see Known-bad).
- Static analysis: clang `scan-build` in the SonarQube job (`mmg/scripts/JENKINSFILE.sonarqube:30`). Coverage goes to Codecov (`mmg/.github/workflows/main-job.yml:366-390`).

**CPU/GPU or cross-platform consistency**
- Ubuntu 20.04, macOS 12/14 and Windows 2022, each run with its own pass criteria (`mmg/.github/workflows/main-job.yml:36-147`). The Debian Docker Jenkins job runs `ctest` in Debug (`mmg/Jenkinsfile:17-31`).
- No cross-platform output comparison. No GPU.

**Known-bad / expected-failure handling**
- `WILL_FAIL` is used only for the `-val` cases (`mmg/cmake/testing/mmg3d_tests.cmake:427,709`; `mmg/cmake/testing/mmgs_tests.cmake:63,265,369`; `mmg/cmake/testing/mmg2d_tests.cmake:56,584`).
- Tests commented out, with reasons:
  - `mmg3d_NM_cone_ani_fast`: "at second run ... a tet with 4 ridge vertices" (`mmg/cmake/testing/mmg3d_tests.cmake:39`).
  - `SphereIso_0.020-0.015`: "not enough mem on windows 4G" (`mmg/cmake/testing/mmg3d_tests.cmake:83`).
  - `CubeSkin0.0125_Inside0.25/0.5`: "too long" (`mmg/cmake/testing/mmg3d_tests.cmake:99-100`).
  - `MultiDom_Ellipse_ReqEntitiesAni`: over 1 h on Windows (`mmg/cmake/testing/mmg_tests.cmake:344-351`).
  - `BUG_OptLsSingularities` and `TestDoSol_1 -m 100` (`mmg/cmake/testing/mmg3d_tests.cmake:919-927`).
  - The `_default` variants (`mmg/cmake/testing/mmgs_tests.cmake:259-262`).
- Tests that silently accept known-bad output: `Car_NM` (above).
- Unit cases silently skipped: `test_met3d` loop bounds (above).

**Worth adopting for BRepGPU**
1. *Frozen-interface invariant test* (`mmg/cmake/testing/code/compare-para-tria.c`). Mesh two faces that share an edge independently. Then assert that the shared-edge vertices match by reference and coordinate, and that their counts match. This is the cross-face watertightness oracle we need: every trim edge shared by two faces must discretize identically on both sides. Use bitwise equality, not mmg's 1e-5.
2. *Feature-preservation predicate* (`mmg/cmake/testing/code/ridge-preservation-in-ls-mode.c`). Every input feature edge must survive as a feature or as a chain of sub-edges. Close mmg's gap by checking sub-edge chains instead of skipping split edges.
3. *Minimal hand-authored snap-to-non-manifold cases* (`mmg-ci-tests/mmgs/mmgs/LSSnapval/8elts1.mesh` plus `8elts1-nm.sol`). Snapping trim-curve vertices to grid or boundary points can pinch a region in exactly this way. Write 9-vertex fixtures with a known expected outcome (snap or refuse) and assert the outcome, not a log line.
4. *Re-run on own output* (`ADD_RUN_AGAIN_TESTS`). Re-tessellating our own output mesh must succeed. Strengthen it to idempotence: the second pass must be identical, or must satisfy a stated bound.
5. *Graded failure codes* (`MMG5_LOWFAILURE` = "conformal mesh saved but goal unmet"). This maps onto our "correct output or explicit refusal". Adopt a three-way result and test each exit path explicitly, which mmg does not do for memory exhaustion.
6. *Owned memory accounting with a configurable budget* (`MMG5_TAB_RECALLOC`, `mmg/src/common/mmgcommon_private.h:406-432`). Then add the test mmg lacks: a fixture with a tiny budget that must refuse cleanly.
7. *Analytic linear-algebra tests with near-degenerate spectra* (`mmg/cmake/testing/code/test_met3d.c:66-82`). Run all of the cases, including the ones mmg skips. They apply to our metric and anisotropy math on stretched parametrizations.
8. *Orientation repair or cut-on-conflict* (`mmg/src/mmgs/analys_s.c:138-150`) plus a Moebius fixture. These cover inconsistently oriented B-rep shells.

---

#### mmg CI corpora (static.bordeaux.inria.fr/mmg: mmg 0.0.20, mmg2d 0.0.38, mmgs 0.0.25; mmg3d 0.0.46 not acquired)

**Test inventory.** 75 case directories and 283 files, 212 MB: `mmg-ci-tests/mmg/mmg` has 25 dirs and 88 files, `mmgs/mmgs` has 22 and 88, `mmg2d/mmg2d` has 28 and 107. Directory names state the fault class:
- *Non-manifold*: `mmgs/mmgs/Car_NM` (7344 vertices, 14700 triangles, 1585 edges); `mmgs/mmgs/Cow_NM`.
- *Non-orientable*: `mmgs/mmgs/Moebius-strip` (24 vertices, 32 triangles).
- *Wrong orientation*: `mmgs/mmgs/LSTriaOri/fault.mesh` (4225 vertices, 8192 triangles).
- *Open boundary*: `mmgs/mmgs/OpnbdyOptimAni`, `mmg2d/mmg2d/Opnbdy`, `mmg/mmg/OptLsSurf_peninsula`.
- *Sharp ridges, corners, singularities*: `mmg/mmg/SurfEdges_house/housebad.meshb` (18 MB); `mmg2d/mmg2d/SquareGeneration` (4 corners, 13 ridges).
- *Anisotropic metrics*: `TorusholesAni` (planar shock), `TorusholesAni_chocCyl` (cylindrical shock), `CubeVolAni`/`CubeVolAni2` (metric not aligned with z, per README), `SphereVolAni`, `mmg2d/mmg2d/SquareAniso`.
- *Required entities on interfaces*: `MultiDom_Ellipse_ReqEntities` (382693 vertices, 2.23 M tets, 76 MB; one required triangle, edge, corner and ridge listed in its README); `MultiDom_Cube_ReqEntities`; `mmg2d/mmg2d/Disk_ReqEntities`, whose README says required triangles are "ignored for now".
- *Tiny features*: `Disk_ReqEntities/disk-tiny.mesh`; `TorusholesAni_chocCyl/torusholesTiny`.
- *Abnormal end*: `LeakCheck_AbnormalEnd{2,7,8}`, with READMEs naming the failing stage (scale, saveMesh, saveSol).
- *Precision*: `MeshVersionFormatted1` (single precision) and `MeshVersionFormatted2`.
- *Snapping to non-manifold*: `LSSnapval`, with 9-vertex inputs and hand-set values.
- *Negative area*: `mmg2d/mmg2d/CoorRegularizationNegativeArea` (347 vertices, 612 triangles).
- *Random perturbation*: `mmg/mmg/CoorRegularizationRandomCube/cubeRandom.mesh` (8 corners and 102 ridges tagged).
- *Boundary-only generation*: NACA (132 edges), non-convex ACDC (62 edges), Garonne river.
- *Hybrid tria/quad and prism*: `Hybrid`.

**Oracles.** None in the corpora themselves. Stored outputs (`*.o.meshb`, `*.d.meshb`, `Circle/mmg2d_SimpleCircle.ref`) exist but no registered test reads them. They are at best candidate tier-3 goldens, and they are stale: the `.ref` log carries `/Users/froehly/...` paths.

**Corpora.** Provenance is the Inria sync server (`mmg/cmake/modules/LoadCiTests.cmake:30-38`). Adding new cases requires Inria affiliation.
- Symlinks were stripped in extraction. One referenced path is missing: `${MMG2D_CI_TESTS}/OptLsSurf_box/box_multiMat.mesh` (`mmg/cmake/testing/mmg2d_tests.cmake:558`), probably a stripped symlink.
- Four other "missing" paths are intentional. The `LocParamsCrea/*.mmg2d|.mmgs` files are created by the tests, and `multi-mat-false.*` exists to test the missing-file path.
- `mmg2d/mmg2d/API_tests/2d.c` and `2d.F90` are code in an untrusted corpus. They were not run.
- The mmgs tests also read the mmg2d corpus (`2squares`, `mmg/cmake/testing/mmgs_tests.cmake:311-326`).

**Pathology handling.** The corpus carries the pathology classes listed above. Termination, memory and NaN behavior are covered under mmg.

**Fuzzing / mutation / property / metamorphic / differential / perf.** None in the corpus.

**CPU/GPU or cross-platform consistency.** None in the corpus.

**Known-bad / expected-failure handling.** The READMEs record known-bad behavior in prose only: Car_NM "faces that disappear", and Disk_ReqEntities required triangles "ignored for now".

**Worth adopting for BRepGPU**
- Copy the naming practice: one directory per fault class, plus a README giving the exact command and what the case attacks. This is cheap provenance for our authored adversarial families.
- Recreate `LSSnapval`-style minimal fixtures (9 vertices) for snap and pinch decisions in trim-loop processing.
- `Moebius-strip` and `LSTriaOri` are ready-made orientation-conflict inputs, usable for our shell-orientation pass after conversion. Attach real oracles to them: orientation consistency and an explicit cut report.
- `CoorRegularizationRandomCube` (random vertex jitter with tagged corners and ridges) is a seed for a coordinate-perturbation mutation fuzzer.

---

#### JIGSAW (Darren Engwirda; archive `whole-mesher-study/archives/jigsaw-d7859d7e1b71e4ee1a33663a617ada9ffb4777b8.tar.gz`; version 1.1.0 per `whole-mesher-study/jigsaw/version.txt:17`)

**Test inventory**
- 26 library "unit tests" in `whole-mesher-study/jigsaw/uni/`, in three families: E^2 (`test2d_*`), S^2 (`test2s_*`) and E^3 (`test3d_*`). Each family covers: a plain domain (a); h(x) on a grid (b) and on a mesh (c); multi-part with ID tags (d); initial conditions (e); restricted DT via TRIPOD (f); unconstrained DT via TRIPOD (g); and MARCHE gradient limiting (h, i). See `whole-mesher-study/jigsaw/uni/README.md`. A representative input is a hard-coded 4-vertex unit square at `hmax=0.25` (`whole-mesher-study/jigsaw/uni/test2d_a.c:47-85`).
- 6 CLI runs in CI (`whole-mesher-study/jigsaw/.github/workflows/cmake.yml`, "Tests for cmd-jigsaw"):
  - `example.jig`, `geo/bunny.jig`: a 10680-vertex, 21348-triangle surface remeshed by restricted Delaunay.
  - `geo/airfoil.jig`, `geo/lakes.jig`: 2D with `GEOM_FEAT` and `MESH_TOP1`.
  - `geo/parts.jig`: a multi-part 3D surface whose coordinates are all below 5e-8 in absolute value. This is effectively an absolute-scale stress test.
  - `geo/earth.jig`: coastlines on an ellipsoid, given in lon/lat, with the first vertex at lon = pi on the seam (`whole-mesher-study/jigsaw/geo/earth.msh:2-6`).

**Oracles.** Every unit test returns only the jigsaw/tripod/marche return code (`whole-mesher-study/jigsaw/uni/test2d_a.c:80-100`), and `test_all` stops at the first nonzero code (`whole-mesher-study/jigsaw/uni/test_all.c:39-105`). That is **tier 2**. The CLI runs are **tier 1/2**: nothing reads `out/`, which ships empty apart from `.keep`. The images in `img/` are documentation, not goldens. There is no check of Delaunay-ness, radius-edge bounds, `MESH_EPS2` surface error, manifoldness or part tags, even though the library advertises all of them.

**Corpora.** Five geometry files ship in `whole-mesher-study/jigsaw/geo/`: airfoil (476 edges), lakes (303 edges), bunny, parts (1392 vertices, 2784 triangles) and earth (8314 vertices and edges). There is no external corpus. The MATLAB and Python front-ends, which carry more examples, are separate repositories and are not in this archive.

**Pathology handling**
- *Termination*: the refinement loop is `if(++_pass>_opts.iter()) break;` (`whole-mesher-study/jigsaw/src/libcpp/rdel_mesh/rdel_mesh_3.hpp:1087`). `MESH_ITER` defaults to "INF" (`whole-mesher-study/jigsaw/inc/jigsaw_jig_t.h:204-210`), implemented as `numeric_limits<indx_t>::max()` (`whole-mesher-study/jigsaw/src/liblib/init_jig_t.hpp:99-100`). Hitting the cap simply breaks out of the loop. No error code is returned, so a truncated mesh looks the same as a converged one.
- *Convergence caveat*: the release notes say sharp 1-dim features (acute dihedral angles between surface patches) "are NOT CURRENTLY PROTECTED ... Valid meshes are currently only produced if the geometry contains surface features that meet at angles of greater than 60 deg. Convergence may be compromised otherwise" (`whole-mesher-study/jigsaw/version.txt:240-246`). Sharp 0-dim features are protected by protecting balls (`:232-238`). No test exercises the sub-60-degree case.
- *Predicates*: orientation and in-ball use filtered floating point with exact expansion fallback, after Shewchuk, Burnikel-Funke-Seel and Devillers-Pion (`whole-mesher-study/jigsaw/src/libcpp/predicate/predicate_k.hpp:1-25`).
- *Restricted-surface intersection*: the dual-to-geometry clipping uses an epsilon model. The tolerance is `_RTOL = eps^0.85 * mean circumradius` (`whole-mesher-study/jigsaw/src/libcpp/rdel_mesh/rdel_base_3.hpp:310-312,507`). Near-degenerate hits (`!_safe`) are re-checked with the exact circumball predicate and dropped if inconsistent (`:519-540`). The release notes call this approach "still employing an epsilon floating-point model" (`whole-mesher-study/jigsaw/version.txt:150-155`). Other precision fixes are recorded but not regression-tested: translate-to-origin (`:54`), an aabb-tree splitter precision bug (`:67`), and `%.17g` lossless I/O (`:63-65`).
- *Input validation*: only index-range checks, for example "GEOM. EDGE2 indexing is incorrect" (`whole-mesher-study/jigsaw/src/geo_load.hpp:829-840`). There is no NaN/inf coordinate check; `isfinite` appears only inside interval arithmetic (`whole-mesher-study/jigsaw/src/libcpp/expansion/ia_float.hpp:355`). There is no memory budget.
- *Surface-error control*: `MESH_EPS1/EPS2` default to 5/12 of h(x) (`whole-mesher-study/jigsaw/inc/jigsaw_jig_t.h:366-385`). `MESH_TOP1/TOP2` refine until nodes are locally 1- or 2-manifold (`:238-258`). `MESH_RAD2` is the radius-edge ratio, default 1.05 (`:262-268`). These are guarantees by construction, and none is tested.

**Fuzzing / mutation / property-based / metamorphic / differential / performance-regression.** None found. The `uni/` tree and the workflow contain no assertions, comparisons or timing checks.

**CPU/GPU or cross-platform consistency.** The CI builds Debug on Windows MSVC, Ubuntu GCC and macOS clang (`whole-mesher-study/jigsaw/.github/workflows/cmake.yml:16-34`). Outputs are not compared across platforms. No GPU.

**Known-bad / expected-failure handling.** `test2s_d` is commented out of `test_all` (`whole-mesher-study/jigsaw/uni/test_all.c:22,71`) and its source file is absent. There are no other skip lists.

**Worth adopting for BRepGPU**
1. *Build the oracles for JIGSAW's advertised guarantees, which JIGSAW never tests*: radius-edge ratio at most RAD2 everywhere outside protected balls, surface error at most EPS2*h(x), and local manifoldness when TOP2 is set. Each is a tier-5 predicate we can run on our own tessellations, with our tolerance and chord-error contract in place of h(x).
2. *Iteration cap that must report.* JIGSAW's silent `break` is the anti-pattern. Our refinement loop needs a bound and an explicit "budget exhausted" status, plus a test that forces the bound.
3. *The acute-dihedral (under 60 degrees) convergence caveat.* Our authored family of near-tangent trims and acute feature angles should include sub-60-degree face-to-face junctions and assert termination within budget.
4. *`parts.msh`-style absolute-scale fixtures* (all coordinates around 1e-8) and translate-to-origin as a preprocessing step. Add metamorphic tests: translating, scaling or rotating the input must give the same output combinatorics.
5. *Epsilon-flag-then-exact-recheck* (`whole-mesher-study/jigsaw/src/libcpp/rdel_mesh/rdel_base_3.hpp:519-540`) is a pattern worth studying for trim-curve and surface intersection. Note the license: commercial distribution only "BY DIRECT ARRANGEMENT WITH THE AUTHOR" (`whole-mesher-study/jigsaw/LICENSE.md`). Study it, do not vendor it.
6. *`earth.msh`*: lon/lat input with a vertex on the lon = pi seam is a ready periodic-seam parameter-space fixture.

---

#### Blind spots this project reveals

- **Cross-face interface identity.** mmg's `compare-para-tria` shows the need for a dedicated oracle: each shared B-rep edge must be discretized bit-identically on both adjacent faces. "Watertight" checks after stitching can mask the mismatch.
- **Feature survival through refinement.** Every input feature edge must remain a feature, or a chain of feature sub-edges, after splitting. The plan checks geometry, not feature-tag preservation.
- **Snap-induced non-manifold pinches.** Snapping a trim or loop vertex onto a nearby vertex or boundary can make a bow-tie. This needs tiny hand-authored fixtures with expected decisions (LSSnapval).
- **Non-orientable or inconsistently oriented shells** (Moebius, mis-oriented input): the plan has orientation checks on output but no adversarial orientation input or expected cut/refuse behavior.
- **Re-tessellation of own output / idempotence** (`RUN_AGAIN`): not in the plan.
- **Budget exhaustion as a tested path.** Both projects cap memory or iterations, but neither forces the cap in a test. The plan's "termination within bound" needs fixtures that deliberately hit the bound, and the result must be an explicit refusal, not JIGSAW's silent truncation.
- **Degraded-success status** (mmg LOWFAILURE: valid mesh, goal unmet). The plan's binary "correct or refuse" should decide whether a third status exists, and test it.
- **Acute dihedral junctions under 60 degrees** between surface patches, the known non-convergence regime for restricted-Delaunay surface meshers.
- **Absolute coordinate scale** (around 1e-8) separate from the 1e6:1 ratio family, plus translate, rotate and scale metamorphic invariance.
- **Near-repeated eigenvalue metric math** for anisotropic sizing on stretched parametrizations. The cases exist in mmg but are skipped.
- **Single-precision input files** (`MeshVersionFormatted 1`): STEP is text, but downstream float32 consumers and round-trip loss should be covered.
- **Tests that pass while documented-bad** (mmg `Car_NM`). Every fixture in our corpus whose README records known-bad output needs an xfail with a real oracle, not an exit-code pass.

### Cluster 7: GPU meshers, backend consistency, fuzzing: OpenSubdiv, gDel3D, Manifold, fidget, OpenSCAD, Warp

#### OpenSubdiv (v3_7_0, 9dab8a47bfbb1388ec8388fe61f5f916e6123f38)

- **Test inventory.** Subdivision limit-surface evaluation and refinement over a 123-shape corpus (`opensubdiv/regression/shapes/*.h`, plus `all.h`): creases (`catmark_cube_creases0..2`, `catmark_inf_crease0/1`, semisharp/infsharp cubes), corners, darts, extraordinary vertices at valence 8/64/360 (`catmark_pole8/64/360`, `loop_pole*`), non-manifold edges/vertices (`catmark_nonman_edges`, `_edge100`, `_verts`, `_bareverts`, `_quadpole360`), valence-2 folds (`catmark_val2_foldover`, `_back2back`, `_nonman`), holes (`catmark_hole_test1..4`), face-varying boundary rules (`catmark_fvar_bound0..4`), hierarchical edits, left/right-handed orientation, non-quad input. Registered ctests: `regression/far_regression/CMakeLists.txt:28`, `regression/bfr_evaluate/CMakeLists.txt:31-36` (three configurations), `regression/osd_regression/CMakeLists.txt:36` (only when GL tests are on), `examples/glImaging/CMakeLists.txt:50`.
- **Oracles.**
  - `far_regression` compares Far refinement with the legacy Hbr implementation vertex by vertex at absolute 1e-6 (`regression/far_regression/far_regression.cpp:19-29,126-170`). Tier 6: the model is an older implementation of the same rules, so both can share a misreading of the rules.
  - `bfr_evaluate` compares the Bfr surface evaluator with Far PatchTable evaluation for position and first/second derivatives (`regression/bfr_evaluate/main.cpp:593-700`). The tolerance is relative to the bounding box, 5e-5 of the largest extent (`main.cpp:106,519-545`). Derivative tolerances are 5x and 25x that (`main.cpp:664-665`). Tier 6, two independent evaluators. It also has a separate round-trip property for Ptex/subface parameterization conversion (`main.cpp:551-590`), tier 4.
  - `osd_regression` compares only the CPU and CPU-with-GL-buffer backends with Hbr at 1e-6 (`regression/osd_regression/main.cpp:36-49,120-205`). **CUDA, OpenCL, GLSL, Metal, TBB and OpenMP evaluators never take part in a regression comparison.**
  - `glImaging` writes a PNG per kernel (CPU/OPENMP/TBB/CUDA/CL/XFB/GLSL) (`examples/glImaging/glImaging.cpp:186-273,682`), but nothing compares the images. Tier 1.
  - `hbr_regression` checks against 185 stored OBJ baselines (`regression/hbr_regression/baseline/`). Tier 3.
- **Known-bad handling.** `bfr_evaluate` exits successfully when the number of failing shapes *equals* `-pass N` (`main.cpp:244-245,936`). The position test runs with `-pass 2` (`CMakeLists.txt:31-32`). Two anonymous shapes may fail, and the run still passes if a different pair fails. Boundary vertices under the "none" interpolation rule are skipped as undefined (`osd_regression/main.cpp:155-158`). `far_perf` is registered to execute the `far_regression` binary (`regression/far_perf/CMakeLists.txt:30`), so CTest never runs the perf harness.
- **CI.** Azure runs `ctest -R "bfr|far"` on Ubuntu 22.04, macOS 14 and Windows 2022 (`azure-pipelines.yml:6-62`). That is cross-OS CPU consistency, checked against a reference within tolerance and not bitwise. No GPU job exists.
- **Pathology.** Extreme valence and non-manifold topology are tested as *inputs that must evaluate*. There are no NaN, timeout or memory tests.
- **Worth adopting.** (1) The shape corpus as a source of topology torture. Poles of valence 360, non-manifold edges shared by 100 faces and valence-2 folds map directly onto degenerate patch corners and collapsed edges in B-rep faces. (2) Tolerances relative to the bounding box, with derivative tolerances scaled from the position tolerance. (3) The lesson that a project with six GPU backends tests none of them against the CPU in CI. GPU parity has to be designed in.

#### gDel3D (ashwin/gDel3D a429be8d702a1f0f0d9366e14f7f492b1c071c9b)

- **Test inventory.** There is no test suite. `Demo.cpp` generates 100,000 points (seed 123456789) and runs the GPU triangulation (`GDelFlipping/src/Demo.cpp:50-52,68`). It then calls the CPU `DelaunayChecker`: Euler characteristic, orientation, adjacency symmetry and local Delaunay (`Demo.cpp:90-94`).
- **Oracles.** The local Delaunay check runs a CPU adaptive-exact insphere test across every interior facet (`DelaunayChecker.cpp:282-360`), tier 5. Euler, orientation and adjacency are tier 5 structural properties (`:171-277`). **Failures are printed (`***Fail***`), and `main` still returns 0** (`Demo.cpp:94-99`), so nothing raises an error.
- **Input distributions** (`InputCreator.cpp:110-200`): uniform, Gaussian, ball, sphere, grid (coordinates snapped to a 512³ lattice, which gives massively cospherical and coplanar input, `:156-170`), and thin sphere (radius 0.45 + U(0, 0.001), `:174-190`). Duplicate points are filtered out *in the generator* (`:192-200`). Duplicate handling is therefore never exercised.
- **Robustness technique.** The GPU predicates are a CUDA port of Shewchuk's code that forces correctly rounded multiplies, `__dmul_rn` / `__fmul_rn`, so nvcc cannot contract them into FMAs (`gDel3D/GPU/KerShewchuk.h:47-54`). Exact predicates with simulation of simplicity run on the GPU (`GpuDelaunay.cu:740-755,872,1166-1191`). Facets the GPU cannot fix are gathered (`:1330-1347`) and repaired on the CPU by star splaying (`GpuDelaunay.cu:95-97`).
- **CPU/GPU.** The CPU checker is the oracle for the GPU output. No CPU-vs-GPU output comparison exists.
- **Worth adopting.** (1) The `__dmul_rn` technique, required for any Shewchuk-style expansion arithmetic in CUDA. Metal needs an equivalent guarantee that it will not contract. (2) The grid and thin-sphere distributions as degenerate-input generators for parameter-space point sets. (3) The GPU-fast/exact-fallback/CPU-repair structure, with a CPU property checker as the offline oracle.

#### Manifold (elalish/manifold 761661c15bacb6c8459d682c715de84591d51774)

- **Test inventory.** Polygon triangulation: 100 named cases in `test/polygons/polygon_corpus.txt` (2016 lines; names include `CoincidentHole*`, `Colinear*`, `NearlyColinearY`, `Sliver..Sliver10`, `Degenerate..Degenerate6`, `DegenerateRing`, `TouchingHole`, `Slit`, `KissingZigzag`, `BarelyValid`, `Precision..Precision4`, `PointPoly`, `Woodgrain*`, `CondensedMatter64`). Large stress files: `sponge.txt` (2381 lines), `zebra.txt` (63,021) and `zebra3.txt` (65,466). Booleans on OBJ fixtures in `test/models/`: `self_intersectA/B`, `degenerate_split_quad_*`, `degenerate_swap_cycle_*`, `dedupe_stale_duplicate`, `openscad-nonmanifold-crash`, and Thingiverse-style pairs (`Generic_Twin_7081.1.t0_*`, `Havocglass8_*`, `Cray_*`) (`test/boolean_complex_test.cpp:1466-1560`, `test/boolean_test.cpp:294-309`).
- **Oracles.**
  - Each polygon case checks the stored triangle count, tier 3 (`test/polygon_test.cpp:91-101`). The same case also runs **two metamorphic relations**: rotation by 180° must give the same count, and a translated duplicate must give exactly twice the count (`:37-63,97-101`), tier 4.
  - In debug builds, `intermediateChecks` runs `CheckTopology`, which verifies halfedge pairing and manifoldness. It also runs `CheckGeometry`, which requires every triangle to be CCW within 2ε (`src/polygon.cpp:71-129,976-981`). Tier 5. Neither checks area conservation or that each input edge appears in the output.
  - Boolean tests check Status, genus, volume and surface area within 1e-5, plus `MatchesTriNormals` (`test/boolean_test.cpp:604-626`). Tier 5-7.
- **Fuzzing.** Google FuzzTest drives the triangulator through `test/polygon_fuzz.cpp`.
  - The input domain is up to 10,000 polygons of up to 10,000 finite float points each, with ε ∈ [-1, 0.1] (`:69-76`). A second fuzz target rounds the coordinates to integers to force exact degeneracy (`:82-99`).
  - The fuzzer is seeded from the corpus (`:101-136`).
  - **Termination is part of the oracle.** Each run is wrapped in `std::async` with a timeout of max(n, 10000) ms. A timeout leaves `faulted` true and fails the test (`:40-49`).
  - `geometryErr` (the non-CCW check) is treated as acceptable on arbitrary input (`:34-37`). Topology errors and other exceptions fail the test.
  - `test/manifold_fuzz.cpp:38-97` fuzzes up to 100 union/difference operations on transformed cubes with a 10 s timeout per operation, and requires `Status()==NoError`. Translation magnitudes are deliberately limited to [0.1, 10]: "larger numbers may cause precision issues, prefer to test them later" (`:37`).
  - `scripts/minimizer.sh` loops FuzzTest's reproducer minimization.
- **Cross-platform consistency.** This is the strongest found in this lane. `scripts/determinism_cases.sh` names six Boolean cases, including `AlmostCoplanar` and `MultiCoplanar`. CI exports their OBJ output on Linux, macOS and Windows with TBB off (`.github/workflows/manifold.yml:79-92,170-180,525-535`). `scripts/compare_artifact.sh` normalizes line endings, then requires **identical SHA-256 across the three OSes** and prints a diff on mismatch. The CI matrix covers TBB ON/OFF, GCC 11/13/14, a libc++-hardened build (`manifold.yml:220`), ASan (`sanitizer.yml`, `scripts/sanitizer_cases.sh`) and TSan with a TBB built for TSan (`tsan.yml`).
- **Performance regression.** `scripts/compare_pr_perf_guard.py:140-233` compares a PR to its base over repeated runs. It flags a time regression when the delta of minimum times exceeds both a percentage and an absolute-milliseconds threshold, and it flags peak-RSS regressions the same way. A weekly benchmark workflow also runs.
- **Worth adopting.** (1) The whole polygon corpus as a seed set for our parameter-space CDT. Our oracle should be stronger: area conservation, constraint-edge presence and orientation, not only triangle counts. (2) Timeout-as-failure fuzz harness with a size-scaled bound. (3) Rotate-180 and translated-duplicate metamorphic relations. (4) Integer-rounded fuzz domain. (5) Bitwise cross-OS determinism over a small hard set. (6) A dual-threshold time+RSS performance guard.

#### Fidget (mkeeter/fidget eb4e9487cac6570110ff0a4d8ae81ad9d8e35110)

- **Test inventory and oracles.**
  - One canonical-operation suite is instantiated for every evaluator backend, VM and JIT (`fidget-jit/src/lib.rs:1413-1416`, `fidget-core/src/vm/mod.rs:1400`). It is not a comparison of backends with each other. Each op is checked against Rust's f32 implementation over a "spicy" argument set that includes NaN (`fidget-core/src/eval/test/mod.rs:46-62`), tier 6.
  - Interval evaluators are checked for **enclosure soundness**: 32 interior samples per interval must fall within the returned bound, or the bound must be NaN where the function is NaN or infinite (`eval/test/interval.rs:1087-1170`). Tier 5. A register-allocator stress function exercises the JIT (`mod.rs:14-40`).
  - Meshing (`fidget-mesh/src/octree.rs:1093-1700`) has exhaustive tests over all 256 cube-corner masks for manifoldness and edge matching, single- and multi-threaded (`:1340-1400`). It also tests vertex-dedup checks (`:1561-1590`), QEF near-planar degeneracy (`:1639`) and cancellation (`test_octree_cancel`, `:1688`).
- **GPU.** `fidget-wgpu` tests compile the shaders and check internal consistency of a rendered two-sphere scene: depth versus alpha, and the expected color set (`fidget-wgpu/src/voxel/mod.rs:2620-2700`). There is no comparison with the CPU renderer or with analytic geometry. `require_gpu!` skips GPU tests when `NO_GPU` is set (`fidget-wgpu/src/lib.rs:722-728`). CI sets `NO_GPU` on Linux and Windows (`.github/workflows/ci.yml:159-164`), so GPU tests run only on macOS (Metal). Tests run with `--retries 2` (`ci.yml:166`), which hides flakiness.
- **Worth adopting.** (1) One property suite run against every backend; for us that means CPU scalar, CPU SIMD, CUDA and Metal. (2) Interval-enclosure soundness by interior sampling, a direct template for testing that our cheap chord-deviation bounds never understate. (3) Exhaustive enumeration of a finite case space, the 256 corner masks. The analogue for us is the trim-loop/cell crossing configurations.

#### OpenSCAD (openscad/openscad 21780e0987e72f6ea9ceaa5a8e2d41f58fe66b96)

- **Test inventory.** There are 524 `.scad` inputs under `tests/data/scad` (2D, 3D, bugs, issues, misc, experimental) and 121 regression output groups under `tests/regression/`. Most outputs are golden PNGs, with some golden STL/OFF/3MF/CSG/echo output (`tests/CMakeLists.txt`, 1558 lines).
- **Oracles.**
  - Golden images are compared with a block-wise image comparison. Per-pixel differences below `PIXEL_TOLERANCE = 8` are zeroed, and the check then looks for 3x3 blocks whose differences all share a sign (`tests/image_compare.py:8-80`). Tier 3.
  - **Backend differential:** Manifold and CGAL renders must match the *same* expected image. An explicit, commented exception list covers the cases where they legitimately differ (`tests/CMakeLists.txt:605-640`, `RENDER_DIFFERENT_EXPECTATIONS`, `PREVIEW_DIFFERENT_EXPECTATIONS`), tier 6.
  - `validatestl.py:100-128` rejects empty meshes, NaN or infinite vertices and normals, and any unbalanced directed edge, meaning an open or inconsistently oriented edge. Tier 5. It does not detect self-intersection.
  - Expected-error tests (`add_failing_test ... --retval=1`, `tests/CMakeLists.txt:1000,1083-1087`) require an explicit failing exit status for an empty union and for parser errors. That is the "explicit refusal" oracle.
  - Manifoldness corner cases are listed: cubes touching at an edge or vertex under Minkowski, and `rotate_extrude` touching an edge (`:718-723`).
  - `--enable=discretization-by-error` has golden CSG dumps and PNGs (`:1269-1281`, `regression/dump-discretization-by-error/`), which is chord-error discretization, but only as a regression check.
- **Known-bad handling.** `disable_tests_safe` lists tests disabled for named issues (`:1313-1340`). These include z-fighting nondeterminism across machines and platform-specific disables (`:1482-1510`). A "Heavy" config tags slow tests (`:735-740`).
- **Worth adopting.** (1) An exception list for backend differences, with a comment on each entry explaining why. (2) The directed-edge balance check as a cheap first watertightness filter. (3) Golden tests that require an explicit error exit status.

#### NVIDIA Warp (28159649a2bb8b91e1badf692007c38a9f62d6b5), marching cubes only

- The same test function is registered for every device from `get_test_devices()`, CPU and each CUDA device (`warp/tests/geometry/test_marching_cubes.py:413,588-600`). There is no direct CPU-vs-CUDA diff.
- Oracles:
  - Structural validity: finite vertices, in-bounds and distinct face indices (`:73-88`), tier 5.
  - Sphere vertices must lie within one cell of the analytic radius (`:119-127`), a coarse tier 7.
  - Differentiable surface area must be within 1e-2 of 4πr², and its gradient within 1e-2 of 8πr (`:236-277`), tier 7.
  - Empty output on an empty field (`:196-212`).
  - **Degenerate grids with fewer than two nodes on an axis must raise `ValueError`** (`:215-230`), an explicit refusal test.
- **Worth adopting.** Per-device instantiation of analytic-spec tests. This is the cheapest correct CPU/GPU consistency design: every device must meet the spec independently.

#### Blind spots this cluster reveals
- GPU parity is not tested in mature projects (OpenSubdiv, fidget) unless a project builds it in. A CPU-vs-GPU comparison plan has to say which oracle each backend meets independently, not just that "they match".
- FMA contraction and compiler reassociation silently break exact or adaptive predicates on GPU (gDel3D's `__dmul_rn`). The plan has no test that predicates stay exact under each device compiler's flags.
- Timeouts as an oracle are missing from our fuzzing. Manifold fails any triangulation taking longer than max(n, 10 s).
- Cross-OS and cross-thread-count bitwise determinism (Manifold). Our plan times and checks output but does not require reproducibility across thread counts, OSes or devices.
- Metamorphic relations: rotation, translation, duplication and their effect on triangle count and area.
- Known-failure counting that passes when *a different* case fails (OpenSubdiv `-pass 2`). Our expected-failure lists must be by name.

## Synthesis

### 1. Union catalog of fault classes

Each row names the fault class, the strongest test of it seen in this lane (project, citation, BLACKBIRD oracle tier), and whether the bootstrap plan covers it.

The bootstrap plan has these inputs: ABC sample/sweep, NIST/CAx-IF STEP, OCCT regressions, authored adversarial families, a STEP/B-rep mutation fuzzer, and the watch. Its oracles are dense deviation; watertight, manifold, orientation, self-intersection and sliver checks; termination within bound; and correct output or explicit refusal.

Citation note: in the synthesis tables, CGAL paths drop the leading `cgal/` when the package name is shown, for example `Number_types/...` means `cgal/Number_types/...`. Every other path follows the report's citation roots.

Plan status:
- **covered**: named in the plan.
- **partial**: implied, but the plan names no input family or no oracle that would actually detect it.
- **blind**: absent.

#### A. Input validity and numeric range

| # | Fault class | Strongest test seen (tier) | Plan |
|---|---|---|---|
| A1 | NaN/Inf coordinates or parameters | spade refuses NaN/inf via `validate_coordinate` (`spade/src/delaunay_core/math.rs:69-108`, tests `:460-500`) (7); fidget propagates NaN through every evaluator (`fidget-core/src/eval/test/mod.rs:61`, `interval.rs:141-153`) (5-7); OpenSCAD `validatestl.py:111-118` checks output (5). Untested in CGAL, geogram, Triangle, CDT, mmg, JIGSAW and libigl. | partial: "refused at ingress" is a design rule with no test family |
| A2 | Coordinate exponent range: overflow, underflow, subnormal differences | spade bounds [2^-142, 2^201] plus underflow flush (`math.rs:20-31,156-169`); CGAL `Interval_nt` overflow/underflow fixed points (`Number_types/test/Number_types/Interval_nt.cpp:137-229`) (7); filter-regime switches at 1e-146 and 1e153 (`Filtered_kernel/.../Orientation_2.h:75-89`); robust `MIN_POSITIVE` probes (`robust-predicates-rs/src/lib.rs:2467-2536`) (7); CDT y=1e38 and a 1-ulp bbox (`cdt.test.cpp:911-1013`) | partial: the 1e6:1 family stays far inside this range |
| A3 | Absolute tolerances that break under global scale | Blender convex hull swept over 1e-15..1e15 (`BL/tests/BLI_convexhull_2d_test.cc:358-409`) (4). Hazards found but not tested: TetWild 1e-10 absolute weld (`tetwild/src/tetwild/Preprocess.cpp:104`), Blender 1e-5 snap (`BL/intern/delaunay_2d.cc:1838-1853`), poly2tri 1e-12 (`poly2tri/poly2tri/common/utils.h:51`). JIGSAW `parts.msh` with all coordinates below 5e-8 | partial: a ratio is tested, whole-model scale and translation are not |
| A4 | Malformed, truncated or corrupt files must be refused | geogram's 19 corrupted OBJ/OFF/MESHB/PLY/STL files must exit nonzero (`geogram/tests/FileInput.robot:12-81`) (7); OpenSCAD `add_failing_test --retval=1` (`openscad/tests/CMakeLists.txt:1083-1087`) (7); Thingi10K errata and skip list (`thingi10k/src/thingi10k/_builder.py:46`) | partial: the mutation fuzzer is planned, but no refusal oracle per malformation is named |
| A5 | The same number written two ways takes different code paths | fTetWild: `27.47818` vs `27.478179999999998` accepts 0 or 666 (`ftetwild/tests/test_predicates.cpp:31-78`) (7, deliberately weakened) | blind |
| A6 | Silent input repair: duplicates and zero-length segments dropped, coplanar facets merged | Triangle warns, and with `-Q` stays silent (`triangle/triangle.c:9997-10010,12521-12541`); TetGen `-T` merges (`tetgen/tetgen.cxx:12140`); earcut exact `===` filter (`earcut/src/earcut.js:103-124`). No project tests that repairs are reported | blind |

#### B. Predicates and arithmetic

| # | Fault class | Strongest test seen (tier) | Plan |
|---|---|---|---|
| B1 | Near-degenerate predicate signs | CGAL `Static_filters.cpp:110-337`: collinear/cocircular up to rounding, perturbed 2^-45, static vs dynamic filter must agree (6); CDT `predicates.test.cpp:1051-1249`: ULP grids, full magnitude range, adaptive vs exact (6); geogram dyadic SOS ray parity vs analytic membership (`geogram/src/tests/test_orient_3d_SOS/main.cpp:333-384`) (7) | blind for predicates; the plan tests meshes only |
| B2 | Exact-zero degenerate results | robust exactly cospherical `== 0.0` (`robust-predicates-rs/src/lib.rs:2600`) (7); CDT small-grid exact integer oracles (`predicates.test.cpp:728-891`) (6). robust's 4,000 fixtures contain no zero (weak) | blind |
| B3 | FMA contraction, fast-math, x87, optimizer reassociation | CDT CI fast-math job (`direct-cdt-study/CDT-53eff288a4e7/.github/workflows/ci_builds.yml:138-167`) plus error-free-transform test (`predicates.test.cpp:296-338`) (7); geogram runtime contraction probe (`geogram/src/lib/geogram/basic/numeric.h:419-452`) and `-ffp-contract=off` (`geogram/cmake/platforms/Linux-gcc.cmake:40-43`); gDel3D `__dmul_rn` (`gdel3d/GDelFlipping/src/gDel3D/GPU/KerShewchuk.h:47-54`); TetGen `-O0` in its makefile only (`tetgen/makefile:24-26`) | blind |
| B4 | Rounding-mode assumptions | CGAL `Interval_nt_nearest.cpp` reruns the suite under `CGAL_ALWAYS_ROUND_TO_NEAREST` with `nextafter` (`Number_types/include/CGAL/FPU.h:347-358`) (7); PMP corefinement recompiled the same way | blind |
| B5 | Exactness and drift of constructed intersection, split and circumcenter points | CDT `InvalidEdgeSplitVertex` refusal leaves the mesh unmodified (`cdt.test.cpp:1493-1548,1953-2003`) (7); CGAL construct-in-double, verify-exactly, recompute (`Triangulation_3/include/CGAL/Robust_weighted_circumcenter_filtered_traits_3.h:56-81`); TetWild rounds a vertex only if no incident tet flips (`tetwild/src/tetwild/MeshRefinement.cpp:57-95`). Weak or hazardous: CGAL silent 4-ulp snap (`Triangulation_2/include/CGAL/Constrained_triangulation_2.h:1169-1216,1806-1815`); spade tolerates 3e-2 (`spade/src/cdt.rs:2634-2637`); CDT goldens store no coordinates | blind |
| B6 | Exact-valid output becomes invalid at output precision | CGAL `data-snap_rounding/intersection_when_rounded_*.off`, `collapse_*` (`cgal/PMP_Boolean_operations/test/PMP_Boolean_operations/data-snap_rounding/`) (5) | blind; STL is float32 |
| B7 | Quantization merges distinct points | gCDT 2^30 grid (`gcdt/include/math_base.cuh:19`, `gcdt/kernel.cu:171-188`). Untested anywhere | blind if we quantize |

#### C. 2D constraint and loop degeneracy (parameter-space CDT inputs)

| # | Fault class | Strongest test seen (tier) | Plan |
|---|---|---|---|
| C1 | Duplicate points | spade bulk-load vertex count equals dedup count (`spade/src/delaunay_core/bulk_load_fuzz_tests.rs:5-18`) (5+7); CGAL 28-point set with exact duplicates (`Triangulation_2/.../_test_cls_delaunay_triangulation_2.h:77-88`). gDel3D filters duplicates *in the generator*, so they are never tested; CDT's `DuplicateVertexError` is never tested | partial (zero-length edges only) |
| C2 | Overlapping, collinear, duplicated, hanging or zero-length constraints; T-junctions | CGAL 18-case truth table under each intersection policy (`_test_cls_constrained_triangulation_2.h:353-374`) (7); CDT corpus `issue-42-*`, `Hanging*`, `overlapping constraints*`, `square with crack` against 562 canonical goldens (3); Blender shared-subsegment and T-junction cases (`BL/tests/BLI_delaunay_2d_test.cc:1202-2716`) (7/3) | partial |
| C3 | Intersecting-constraint policy (refuse or split) as a specified contract | CGAL tag throws `Intersection_of_constraints_exception` (7); CDT `NotAllowed`/`TryResolve`; spade `add_constraint` panics vs `try_add_constraint` | blind: STEP trim loops that cross need a stated policy |
| C4 | Self-tangent loops, touching holes, hole outside the outer ring, bowties, slits, spikes | earcut fixtures (`earcut/test/fixtures/`, `expected.json`) (3+5); CGAL Polygon_repair 36 WKT inputs (`Polygon_repair/test/Polygon_repair/data/in/`) (3+5); Manifold polygon corpus (`manifold/test/polygons/polygon_corpus.txt`) (3+4) | partial ("near-tangent trims") |
| C5 | Inside/outside classification: even-odd vs nonzero, redundant or overlapping loops | CGAL `issue9325.cpp:94-123,191-200`, independent winding count at witness points (6); geogram SOS ray parity (7); Blender non-zero-winding family (7/3); libigl fast vs exact winding (6) | blind |
| C6 | Cocircular lattices, collinear runs, thin shells, clusters (Delaunay ties) | gDel3D grid and thin-sphere generators (`gdel3d/GDelFlipping/src/InputCreator.cpp:110-200`); gCDT `squareMap`/`thinCircleMap`/`twoLineMap` (`gcdt/kernel.cu:43-170`); spade epsilon grid (`spade/src/delaunay_core/bulk_load.rs:1192-1250`); Shewchuk tilted lattice (paper §4.5); CGAL `layers.xy`. Their oracles are print-only or tier 5 | partial |
| C7 | Collinear-only, zero-area and lower-dimensional input | earcut collinear-only gives 0 triangles (`earcut/test/test.js:155-160`) (7); CGAL dimension degeneracy (`_test_cls_constrained_triangulation_2.h:142-165`) (7); AFSR `lowdim.cpp:48-51` | partial |
| C8 | Loop start vertex, direction, loop order, mirror and offset (metamorphic) | Manifold Turn180 and translated Duplicate (`manifold/test/polygon_test.cpp:37-63,91-101`) (4); Blender polyfill sign flips (4). Blender's shift and reverse loop is broken (`BL/tests/BLI_polyfill_2d_test.cc:281`), even though polyfill bug #67109 a/b/c failed only for some offsets (`:721-775`) | blind |
| C9 | Sliver and ULP-scale geometry in loops | poly2tri `deadly_quad`, concave-by-1e-15 boundary (`poly2tri/unittest/main.cpp:50-113`); CDT #211/#230, 5e-17 and 4e-17 offsets (`cdt.test.cpp:1534-1598`); Manifold `Sliver..Sliver10`, `Precision*` | covered in spirit (tiny features), but these concrete inputs are absent |

#### D. Output validity

| # | Fault class | Strongest test seen (tier) | Plan |
|---|---|---|---|
| D1 | Inverted or degenerate triangles that topology checks pass | CDT admits `verifyTopology` misses them and adds `verifyWinding` (`CDTr/extras/VerifyTopology.h:77-95`); Triangle exact `checkmesh` (`triangle/triangle.c:6694-6781`, print-only); spade `sanity_check` (`spade/src/delaunay_core/triangulation_ext.rs:891-912`) (5) | partial: the 3D orientation check runs on the final mesh only, not on each uv CDT |
| D2 | Constrained-Delaunay property | Triangle `checkdelaunay` with exact predicates (`triangle/triangle.c:6793-6886`, print-only); TetGen `check_delaunay` (print-only). Skipped by spade (`spade/src/cdt.rs:735-752`), commented out in geogram (`geogram/src/lib/geogram/delaunay/CDT_2d.cpp:1122`), unconstrained in poly2tri (`poly2tri/poly2tri/common/shapes.cc:394-409`) | blind |
| D3 | Every input constraint present as a chain of output edges | TetGen "missing segments" count (`tetgen/tetgen.cxx:32242-32246`) (5). No 2D project checks it | blind |
| D4 | Coverage: no overlap, no gap (area balance is insufficient) | libigl `remesh_at_points` children tile the parent area to 1e-12, one orientation (`libigl/tests/include/igl/triangle/remesh_at_points.cpp:16-94`) (5); Blender polyfill edge multiplicity, each boundary edge once and each interior edge twice (`BL/tests/BLI_polyfill_2d_test.cc:94-123`) (5). earcut area-only `deviation` misses overlap plus gap (`earcut/src/earcut.js:816-847`) | blind in uv; partial in 3D (deviation) |
| D5 | Closed, manifold, oriented, no self-intersection, bounds a volume | CGAL Alpha_wrap_3 `is_valid_wrap` stack (`Alpha_wrap_3/include/CGAL/Alpha_wrap_3/internal/validation.h:132-217`) (5); OpenSCAD directed-edge balance (`openscad/tests/validatestl.py:120-127`) (5); Thingi10K exact flag table (paper §3.1.2) | covered |
| D6 | Deviation bound soundness (sampled vs proven) | TetWild/fTetWild covering-radius shrink (`tetwild/src/tetwild/State.cpp:36-41`, `ftetwild/src/Parameters.h:119-130`); CGAL bounded-error Hausdorff, analytic plus vs sampler plus translation (`Polygon_mesh_processing/test/.../test_hausdorff_bounded_error_distance.cpp:397-895`) (7/6/4); Warp analytic sphere and area (`warp/tests/geometry/test_marching_cubes.py:119-127,236-277`) (7) | partial: "dense" sampling is unspecified, so it may only be a lower bound |
| D7 | One-sided containment / no crossing of the reference surface | Alpha_wrap_3 `is_outer_wrap_of_triangle_mesh` (`validation.h:229-282`) (5); libigl `lazy_cage` (`libigl/tests/include/igl/lazy_cage.cpp:12-82`) | blind: deviation magnitude is not side or crossing |
| D8 | Shared-edge identity and consistent point order across faces | mmg frozen-interface triangles within 1e-5 (`mmg/cmake/testing/code/compare-para-tria.c:110-246`) (5); libigl ordering on a flipped non-manifold edge (`remesh_at_points.cpp:202-253`) (5); CGAL exact-equality stitching (`PMP_Mesh_repair/include/.../stitch_borders.h:73-91`) | partial: "shared-edge guarantees stay" has no named oracle |
| D9 | Feature, crease and required-entity preservation | mmg ridge preservation and required-vertex tests (`mmg/cmake/testing/code/ridge-preservation-in-ls-mode.c:107-152`, `req-vert-3d.c`) (5); CGAL protecting-ball adjacency (`Mesh_3/test/Mesh_3/test_meshing_polylines_only.cpp:145-189`) (5); OpenSubdiv crease and corner shapes (6) | blind (feature tags and seams through refinement) |
| D10 | Advertised quality bounds actually met (min angle, size) | CDT refinement `findUnrefinedTriangles().empty()` (`cdt.test.cpp:1625-1687`) (5). Not checked by Mesh_2, Mesh_3, Triangle, TetGen or JIGSAW | partial (sliver check) |
| D11 | Idempotence: re-running on own output | CGAL re-refine stabilises within 10 passes (`Mesh_3/test/Mesh_3/test_meshing_utilities.h:131-144`) (4); min-size no-op (`test_min_size_criteria.cpp:111-129`) (4); mmg run-again (exit code only) | blind |

#### E. Termination and resources

| # | Fault class | Strongest test seen (tier) | Plan |
|---|---|---|---|
| E1 | Infinite flip and refinement loops | CDT two "would loop forever" regressions (`cdt.test.cpp:1550-1623`); spade `repro_refine_hang_*` asserting `refinement_complete` (`spade/src/delaunay_core/refinement.rs:1256-1300`); earcut refine on near-cocircular input under a 5 s timeout with a proven tie margin (`earcut/test/test.js:197-212`, `earcut/src/earcut.js:995-1006`); libigl issue-521 injects inconsistent epsilon predicates into an unbounded Lawson loop (`libigl/tests/include/igl/delaunay_triangulation.cpp:8-102`, `include/igl/delaunay_triangulation.cpp:47`) (2) | covered ("no hangs"), but concrete hang reproducers are absent |
| E2 | Timeout as an oracle with a size-scaled bound | Manifold fuzz fails on max(n, 10000) ms (`manifold/test/polygon_fuzz.cpp:40-49`) (2); CGAL per-test 1200 s; mmg CI 7200 s | covered |
| E3 | Cap reached with invalid output still emitted, or exit status lying | geogram emergency exit (`CDT_2d.cpp:700-712`); JIGSAW `MESH_ITER` break (`whole-mesher-study/jigsaw/src/libcpp/rdel_mesh/rdel_mesh_3.hpp:1087`); fTetWild `exit(0)` and `EXIT_SUCCESS` (`ftetwild/src/FloatTetDelaunay.cpp:244-250`, `ftetwild/src/main.cpp:654`); TetWild unrounded vertices logged only (`tetwild/src/tetwild/tetwild.cpp:81-89`); earcut drop (`earcut.js:249-272`) | partial: "budgets as reported backstop" exists, but no test forces a cap |
| E4 | Explicit, structured incomplete or refusal status | spade `refinement_complete`; CDT give-up counters (`cdt.test.cpp:1745-1752`); CGAL Mesh_3 `Mesh_error_code.h:27-31`; TetGen codes 1/2/3/4/5/10 (`tetgen/tetgen.h:2487-2525`); mmg SUCCESS/LOWFAILURE/STRONGFAILURE (`mmg/src/common/libmmgtypes.h:49-65`); Alpha_wrap_3 robustness exit codes 1-9 (`Alpha_wrap_3/benchmark/Alpha_wrap_3/Robustness/robustness_benchmark.cpp:18-110`) | partial: the three-way result (valid, valid but goal unmet, refused) is undecided |
| E5 | Memory budget exhaustion | mmg owns memory accounting, default 50% of RAM (`mmg/src/common/mmgcommon_private.h:76-78,406-432`), but no test exhausts it. gCDT fixed buffers with no overflow check (`gcdt/kernel.cu:5360,5386`). TetWild "needs >100GB" | partial ("no runaway memory" has no forced test) |
| E6 | Recursion depth and stack exhaustion | CGAL "infinite loop ... segfault once the stack overflows" (`Triangulation_2/test/Triangulation_2/test_cdt_degenerate_case.cpp:27-36`) (2); poly2tri unbounded `EdgeEvent` recursion (`poly2tri/poly2tri/sweep/sweep.cc:126-170`) | blind |
| E7 | Small input angles and acute feature junctions (termination) | CDT concentric shells at a 10-degree corner and 15-30 degree corners (`cdt.test.cpp:1793-1846,2202-2228`) (5); spade power-of-two shells (`refinement.rs:809`); CGAL acute polyline corners made terminal (`Mesh_3/include/CGAL/Mesh_3/polylines_to_protect.h:336-340`); JIGSAW: below 60 degrees "convergence may be compromised" (`whole-mesher-study/jigsaw/version.txt:240-246`); Triangle 28.6 degrees (`triangle/triangle.c:1645-1655`) | partial (near-tangent trims; no face-junction angle family) |
| E8 | Cooperative abort and cancel leave state intact | CDT `isAbortCalculation` (`cdt.test.cpp:1105-1164`); fidget `test_octree_cancel` (`fidget-mesh/src/octree.rs:1688`); CGAL `forced_stop` (`Mesh_3/include/CGAL/Mesh_3/Mesher_3.h:319-335`) | blind |
| E9 | Library-mode recoverability after refusal | TetGen `throw` under `TETLIBRARY` (`tetgen/tetgen.h:2489-2491`) vs Triangle `exit()` even as a library (`triangle/triangle.c:1411-1418`); geogram `geo_assert` throws in release | blind |

#### F. Determinism and backends

| # | Fault class | Strongest test seen (tier) | Plan |
|---|---|---|---|
| F1 | Run-to-run and allocation-address determinism | CGAL 20 runs byte-identical (`Mesh_2/test/Mesh_2/reproductibility.cpp:80-87`) and `Random_allocator` (`Mesh_2/test/Mesh_2/test_mesh_terrain.cpp:1-9`) (4); Mesh_3 determinism (`test_meshing_determinism.cpp:76-155`) | blind |
| F2 | Serial vs parallel, and thread count | libigl `parallel_for` equals serial bitwise (`libigl/tests/include/igl/parallel_for.cpp:110-128`); fidget single vs multi-thread mesh manifoldness (`fidget-mesh/src/octree.rs:1387-1400`); CGAL tolerates 4x Hausdorff slack in parallel (`test_meshing_utilities.h:403-410`) | blind |
| F3 | Cross-OS bitwise output | Manifold SHA-256 of six Boolean outputs across Linux, macOS and Windows (`manifold/scripts/compare_artifact.sh`, `manifold/scripts/determinism_cases.sh`) (3, cross-platform); CDT goldens on 4 OS/arch (3) | blind |
| F4 | Backend conformance: CPU vs GPU, exact vs float | fidget one canonical suite per backend (`fidget-jit/src/lib.rs:1413-1416`) (6/5); Warp per-device spec tests (`warp/tests/geometry/test_marching_cubes.py:413,588-600`) (7/5); OpenSCAD shared goldens with a commented exception list (`openscad/tests/CMakeLists.txt:605-640`) (6); Blender double and mpq against the same expectations (`BL/tests/BLI_delaunay_2d_test.cc:94-104`). Anti-pattern: OpenSubdiv compares only CPU and CPUGL with Hbr (`opensubdiv/regression/osd_regression/main.cpp:36-49`) | blind: "timed against NVIDIA" is not parity |
| F5 | Precision-dependent topology (f32 vs f64) | CDT keeps separate f32/f64 goldens (`cdt.test.cpp:516-527`) | blind |

#### G. Oracle integrity and test hygiene

| # | Fault class | Seen in | Plan |
|---|---|---|---|
| G1 | Checkers that report but do not fail | Triangle `-C` (`triangle/triangle.c:6771-6779,6873-6884`); TetGen `-C` and `test_double` (`tetgen/tetgen.cxx:36481-36488`, `tetgen/predicates.cxx:586`); gDel3D (`gdel3d/GDelFlipping/src/Demo.cpp:90-99`); gCDT (call commented out, `gcdt/kernel.cu:5446`); geogram valgrind (`geogram/tests/lib/VorpatestLibrary.py:78-85`); mmg regex tests ignore exit codes | blind |
| G2 | Dead, tautological or vacuous assertions | libigl Hausdorff `break` (`libigl/tests/include/igl/copyleft/cgal/hausdorff.cpp:52`); Blender mpq checks x twice (`BL/tests/BLI_delaunay_2d_test.cc:416-421`); polyfill unshifted copy (`BL/tests/BLI_polyfill_2d_test.cc:281`); CGAL `test_coref_epic_points_identity` without a `.cmd`; Alpha_wrap_3 dead asserts (`test_alpha_wrap_3_mesh.cpp:46,61-66`); robust tautology (`robust-predicates-rs/src/lib.rs:2389-2390`); CGAL `Kernel_checker` with K1==K2; CDT duplicated test (`cdt.test.cpp:874-899`) | blind; mutation testing addresses it |
| G3 | Anonymous known-failure counts and loose whitelists | OpenSubdiv `-pass 2` (`opensubdiv/regression/bfr_evaluate/main.cpp:936`, `CMakeLists.txt:31-32`); earcut per-fixture error allowance up to 13% (`earcut/test/expected.json:63-82`); mmg `Car_NM` passes while its README says faces disappear | partial ("refusals and failures retained") |
| G4 | Fuzzers built but never run, or with clamped domains | spade CI `cargo fuzz build` only, inputs clipped to abs 20 (`spade/.github/workflows/spade_actions.yaml:67-84`, `spade/fuzz/fuzz_targets/bulk_load_fuzz.rs:12`); robust fuzz targets check only `is_finite`; Manifold cube CSG limited to `GoodNumbers` [0.1, 10] (`manifold/test/manifold_fuzz.cpp:37-38`) | partial |
| G5 | Failure minimization into permanent regressions | CGAL `bisect_failures` (`STL_Extension/include/CGAL/bisect_failures.h`), which produced the triwild-min files; Manifold `scripts/minimizer.sh`; spade paste-ready `Debug` formatter (`spade/fuzz/fuzz_targets/fuzz_shared.rs:16-20`); CGAL nightly time-seeded RNG with printed seed (`Random_numbers/include/CGAL/Random.h:233-241`) | blind |
| G6 | Performance and memory regression gate | Manifold dual-threshold time+RSS PR guard (`manifold/scripts/compare_pr_perf_guard.py:140-233`). Every other project prints timings only | partial (timed vs NVIDIA; no gate) |
| G7 | Debug-only safety checks | libigl FPE traps only in Debug while CI runs Release (`libigl/tests/main.cpp`, `continuous.yml`); asserts that vanish in Release (`libigl/include/igl/triangle/triangulate.cpp:56-62`); CGAL requires a non-NDEBUG testsuite build (`Installation/include/CGAL/config.h:52-54`) | blind |

### 2. Cases, corpora and techniques worth adopting

Each item names the source and the BRepGPU use. These are inputs and oracle designs to reimplement; no code is to be copied. Licenses are in Appendix A notes.

**Parameter-space CDT inputs (convert to our trim-loop format):**
1. Manifold `test/polygons/polygon_corpus.txt` (100 named cases), plus `sponge.txt`, `zebra.txt`, `zebra3.txt` for scale. Keep its two metamorphic relations, and replace triangle counts with D2/D3/D4 oracles.
2. artem-ogre/CDT `direct-cdt-study/CDT-53eff288a4e7/CDT/tests/inputs/` (44 files) plus `expected/` (562 canonical topology goldens) and `inputs/predicates/` (4 × 1,000 Nanevski fixtures). Its canonical topology-string format (`cdt.test.cpp:258-399`) is a good regression form; add coordinates.
3. earcut `test/fixtures/` (59) and `bench/tiles-fixture.bin` (119,680 integer-grid polygons, decoder `bench/tiles-fixture.js:4-17`). The tile corpus suits a speed+robustness sweep of collinear-rich loops.
4. CGAL near-degenerate CDT regressions: `Triangulation_2/test/Triangulation_2/issue_4405.cpp`, `issue_5055.cpp`, `issue_5080.cpp`, `issue_7328.cpp`, `test_cdt_degenerate_case.cpp`. Also `Mesh_2/test/Mesh_2/issue_781_very_tiny_edge.txt`, `triwild-10070-min*.obj`, and `Polygon_repair/test/Polygon_repair/data/in/*.wkt` (36 malformed rings).
5. Blender non-zero-winding and overlap cases (`BL/tests/BLI_delaunay_2d_test.cc:1202-3262`) and the text-outline generator (`:4406-4630`). Run it with real oracles; upstream it is compiled out.
6. Generators to reimplement:
   - spade epsilon grid (`spade/src/delaunay_core/bulk_load.rs:1192-1250`), generalized to any magnitude;
   - gDel3D grid and thin-sphere (`gdel3d/GDelFlipping/src/InputCreator.cpp:156-190`);
   - gCDT thin circle, unjittered square and two lines (`gcdt/kernel.cu:43-170`);
   - Blender triangles between circles at a 1e-4 radius ratio and tilted grids (`BL/tests/BLI_delaunay_2d_test.cc:4751-5091`);
   - Shewchuk's tilted lattice (paper §4.5), which hung non-robust code.
7. Triangle's troubleshooting prose as a family (`triangle/triangle.c:2266-2305,3016-3165`): three segments through one point; a vertex on a segment interior; long collinear rows; a narrow band far from the origin (6247133..6247134).

**Predicate and arithmetic conformance (run per device and per compiler flag set):**
8. A fast-math/FMA build job that asserts error-free transforms (CDT `predicates.test.cpp:296-338`, `ci_builds.yml:138-167`). Add geogram's contraction probe as a device kernel (`geogram/src/lib/geogram/basic/numeric.h:426-450`) and a startup check of `epsilon`/`splitter` and `Two_Product` exactness (Shewchuk `predicates.c:643-683`).
9. Static-filter-vs-exact differential on constructed degeneracies perturbed by 2^-45 (`Filtered_kernel/test/Filtered_kernel/Static_filters.cpp:110-337`), across magnitudes past 1e-146 and 1e153.
10. A ulp-grid sign map with an analytic answer, `sign(y-x)` (`robust-predicates-rs/examples/predicate-map/main.rs`), asserted identical on CPU, CUDA and Metal.
11. A round-to-nearest interval variant (`Number_types/include/CGAL/FPU.h:347-358`, `Interval_nt.cpp:87-229`) for any GPU interval filter.
12. Bit-exact reference values on near-cocircular inputs (robust `issue48`, `robust-predicates-rs/src/lib.rs:2603-2650`) for CPU/GPU predicate value equality, not just sign agreement.

**Output oracles:**
13. The libigl `remesh_at_points` harness (`libigl/tests/include/igl/triangle/remesh_at_points.cpp:16-94,202-253`), per uv cell: area tiling, single orientation, consistent order on shared or flipped edges.
14. Blender polyfill edge-multiplicity check (`BL/tests/BLI_polyfill_2d_test.cc:94-123`), with its shift/reverse loop fixed.
15. TetGen missing-segment count (`tetgen/tetgen.cxx:32242-32246`) and stale-marker checks (`:31771-31782`) for GPU scratch flags.
16. The CGAL Alpha_wrap_3 `validation.h` stack, plus a non-manifold-edge check, plus `is_outer_wrap` crossing/containment.
17. CGAL bounded-error Hausdorff (`Polygon_mesh_processing/include/CGAL/Polygon_mesh_processing/distance.h:1560-1612`) or the TetWild covering-radius correction (`tetwild/src/tetwild/State.cpp:36-41`) to make the deviation oracle sound.
18. mmg `compare-para-tria.c` pattern, made bitwise, for shared B-rep edges.
19. CGAL `issue9325.cpp` independent winding classifier at witness points, for trimmed-region in/out labeling; geogram dyadic SOS ray parity (`geogram/src/tests/test_orient_3d_SOS/main.cpp:333-384`) as its tier-7 variant.
20. Validator inputs with exact expected verdicts: CGAL `Data/data/meshes/degeneracies/` (16 files), Alpha_wrap_3 `test/Alpha_wrap_3/data/` (17), libigl `degenerated.obj`, `sliver_tet.ply`, `duplicated_faces_*.dmat`, `two-boxes-bad-self-union.ply`, `truck.obj`.

**Termination, status and harness:**
21. Hang reproducers: CDT `cdt.test.cpp:1550-1623`, spade `refinement.rs:1256-1300`, earcut `test.js:151-153,197-212`, libigl issue-521, and the CGAL seed 1473902576 on `trihole.off`. Re-run them with an explicit work budget and a harness timeout.
22. A status taxonomy and harness: TetGen refusal codes, mmg three-way status, Alpha_wrap_3 robustness exit codes 1-9 with its `/usr/bin/time -v`, signal and timeout runner (`compute_robustness_benchmark_data.py:24-73`).
23. Manifold's timeout-as-failure fuzz wrapper (`manifold/test/polygon_fuzz.cpp:40-49`), integer-rounded fuzz domain (`:82-99`) and corpus seeding (`:101-136`), plus `scripts/minimizer.sh`. Also CGAL's time-seeded, seed-printing RNG and `bisect_failures`.
24. Manifold cross-OS determinism hashing (`manifold/scripts/compare_artifact.sh`, `manifold/scripts/determinism_cases.sh`) and the dual-threshold time+RSS guard (`manifold/scripts/compare_pr_perf_guard.py`). CGAL `Random_allocator` for address-independence.
25. fidget's one-suite-per-backend instantiation and interval-enclosure sampling (`fidget-core/src/eval/test/interval.rs:1087-1170`). This is the template for testing that cheap chord-deviation bounds never understate.
26. A Thingi10K-style per-model flag table (`thingi10k/src/thingi10k/_builder.py:49-103,593`; paper §3.1.2), computed once per ABC/STEP model by exact oracles, so sweep failures can be stratified by pathology class.
27. OpenSubdiv regression shapes (`opensubdiv/regression/shapes/*.h`, 123) for topology torture of collapsed corners and extreme valence: poles at valence 360, edges shared by 100 faces, valence-2 folds. Use them if subdivision-like or degenerate-corner patches enter the input set.

### 3. Gap list: fault classes the bootstrap plan is blind to, ranked by risk

Risk means likelihood × consequence for a fast CPU+GPU parameter-space tessellator whose output is machined, weighted by how unlikely the bootstrap corpora and fuzzer are to expose the class by accident.

1. **Device/compiler arithmetic breaking exact or adaptive predicates** (B3, B4, F5). nvcc contracts FMA by default; Metal's defaults are unverified. The result is a silently wrong sign, then a flipped or overlapping triangle on near-tangent trims. No geometry corpus reliably reveals it, and the failure differs per device. Needed: per-device, per-flag predicate conformance (§2 items 8-12) before any GPU CDT result is believed.
2. **Unchecked uv-CDT output properties** (D1-D4, B5). The plan has these as property words, not oracles: constraint presence, the constrained-Delaunay property, orientation independent of topology, coverage and overlap, and Steiner points on their parent constraints. A missing or flipped uv triangle can sit inside the chord tolerance in 3D, or appear only as a tiny self-intersection. Five of six mature 2D libraries skip at least two of these. Needed: exact per-face uv oracles on every CDT the mesher emits, CPU and GPU.
3. **Oracles that do not fail, or cannot fail** (G1, G2). Seven projects in this lane ship one. Our certification excision moves all correctness weight onto offline oracles, so an insensitive oracle passes everything. Needed: every oracle changes exit status, known-bad inputs carry exact verdicts (§2 item 20), and fleet mutation testing of the suite measures sensitivity. Doctrine says no tests for tests, so this is the doctrine's mutation testing, not a second certification layer.
4. **Precision loss at output: f64-valid meshes invalid as float32 STL** (B6). Slicers and CAM consume float32. Near-tangent trims and tiny features beside huge faces are exactly where rounding creates self-intersections and collapses. Needed: run the validity oracles on the emitted format as well as the internal mesh, using CGAL's snap-rounding fixtures as seeds.
5. **Caps reached and output still emitted, or exit status lying** (E3, E4). "Termination within bound" passes a truncated mesh. Five studied projects do this. Needed: tests that force each work, vertex, iteration and memory budget to fire, full property checks on the returned object, and a decided status set (valid / valid-goal-unmet / refused) with the face-level report the journal already defaults to.
6. **Shared-edge identity and point order across faces** (D8). Watertight checks after welding hide uneven or misordered discretization on shared B-rep edges, especially with opposite traversal, seams and non-manifold sheets. GPU per-face parallelism makes this likely. Needed: a bitwise shared-edge oracle before any weld.
7. **Deviation-oracle soundness and one-sided crossing** (D6, D7). Unspecified "dense" sampling is a lower bound. Needed: a covering-radius correction or bounded-error two-sided distance, plus a crossing/containment check against the exact surface.
8. **Global scale, translation and rotation invariance; exponent range; NaN/Inf from upstream evaluation** (A1-A3). The 1e6:1 family tests ratios, not absolute magnitude or offset, and absolute epsilons hide everywhere (TetWild 1e-10, Blender 1e-5, poly2tri 1e-12). Needed: metamorphic whole-model scaling by 1e±6, far-from-origin translation, and rotation, with identical topology required. Add parameter values at the predicate exponent bounds, and NaN/Inf injected *after* ingress (rational-weight evaluation, pole parametrizations) with an explicit-refusal oracle.
9. **Trim-loop permutations and classification** (C3, C5, C8). Loop start vertex, direction, order, duplicated and redundant loops, crossing loops, even-odd vs nonzero. Blender's history shows offset and rotation find real bugs. Needed: a fuzzer mutation set on loops, a stated crossing policy (CGAL truth-table style), and an independent winding classifier at witness points.
10. **Determinism: run-to-run, thread count, CPU vs GPU, cross-OS** (F1-F4). GPU atomics and hash ordering make nondeterminism likely, and the incremental-remeshing goal needs reproducibility. Needed: an explicit parity policy (identical topology, or a stated equivalence) and per-backend instantiation of the same spec suite, with Manifold-style hashing for a small hard set.
11. **Feature and seam survival through refinement; idempotence** (D9, D11). Needed: input feature edges remain feature chains, and re-tessellation is stable.
12. **Memory exhaustion and resource refusal** (E5, E9, E6). No studied project forces a memory budget. GPU fixed buffers (gCDT) overflow silently. Needed: a per-model RSS and device-memory cap in the sweep counted as failure, forced-exhaustion fixtures, and recovery of the caller and device state after refusal.
13. **Silent input repair** (A6). A valid mesh of a different input. Needed: every repair (dedup, zero-length removal, merging) is reported with location.
14. **Acute face-to-face junctions below 60 degrees and acute trim corners** (E7). These are the known non-convergence regimes of restricted-Delaunay and Ruppert refinement. Needed: authored family with a termination budget and protected-corner oracle.
15. **Fuzzer hygiene** (G4, G5). Scheduled runs, no clamped domains (keep the 1e6 family), timeout-as-failure, seed logging, automatic minimization to permanent regressions, and paste-ready repro output.
16. **Stratified sweep reporting** (Synthesis §2 item 26). Without a per-model flag table, a regression concentrated in one class (periodic seams, coarse tolerance) is diluted in ABC-wide rates.
17. **STEP decimal-spelling variants** (A5). Low cost: text-vs-binary round-trip spellings of the same control points in the mutation fuzzer.
18. **Performance and memory regression gate** (G6). Timing against NVIDIA is a comparison, not a gate. Manifold's dual time+RSS threshold over repeats is a sound minimum.

## Appendix A: catalog rows for acquired references (merge into papers/reference-catalog.tsv)

These rows use the columns of `papers/reference-catalog.tsv`, tab-separated. Paths are relative to the BRepGPU root. In this workspace `quarantine/archives` and `quarantine/sources` resolve into `../quarantine_proj/`, so the files physically live in the shared collection. This lane did not edit the shared `quarantine_proj/MANIFEST.md` or `QUARANTINE.md`.

GitHub ZIPs came from `https://codeload.github.com/<repository>/zip/<revision>` and were extracted with `python3 -I scripts/ingest_zip.py <archive> <destination> --root <archive_root> --skip-symlinks`. The other sources were fetched and extracted as described in the notes after the rows. Everything was acquired 2026-10-17.

```tsv
name	repository	revision	archive	sha256	destination	archive_root	retained_files	omissions
cgal	https://github.com/CGAL/cgal	28811b671a12b5caa9e3688569dadbc6b3728fe6	quarantine/archives/cgal-28811b671a12.zip	0373a4fa0951f54ad5478d10e297759eb6d1fed6bafa954afc5cd8b75ce9eae4	quarantine/sources/cgal	cgal-28811b671a12b5caa9e3688569dadbc6b3728fe6	17460	
ftetwild	https://github.com/wildmeshing/fTetWild	8118f810478e0e65a7bf2d8cecdc5e203a876e97	quarantine/archives/ftetwild-8118f810478e.zip	0ea25b1576e08969c0fc803caca84e0599ceb94650e2db4797c6770e660cc82f	quarantine/sources/ftetwild	fTetWild-8118f810478e0e65a7bf2d8cecdc5e203a876e97	91	
tetwild	https://github.com/Yixin-Hu/TetWild	49de8cd85f56de0c6dc7ac3c245a14c3aa68ee67	quarantine/archives/tetwild-49de8cd85f56.zip	5aa5426c5d598e60ed7253b9bfe8265817b1a1f44f7f41c14db120edb38b14d5	quarantine/sources/tetwild	TetWild-49de8cd85f56de0c6dc7ac3c245a14c3aa68ee67	79	
mmg	https://github.com/MmgTools/mmg	4d8232c8aebfed877935d75d4d4a67e850962422	quarantine/archives/mmg-4d8232c8aebf.zip	1e4d255e9376cbd13146a14fad8d09688066b64528332bb59f1c9e767ec9dcd0	quarantine/sources/mmg	mmg-4d8232c8aebfed877935d75d4d4a67e850962422	384	1 symlink: libexamples/mmg3d/IsosurfDiscretization_lsOnly/met.mesh -> plane.mesh
libigl	https://github.com/libigl/libigl	7100764c2a2833284a8bdfa9b948d2b3bf124624	quarantine/archives/libigl-7100764c2a28.zip	6ad02c6e23d52e29139874dc59002007a1c706a90f92e6d6d8de72d677b6c75f	quarantine/sources/libigl	libigl-7100764c2a2833284a8bdfa9b948d2b3bf124624	1696	
libigl-tests-data	https://github.com/libigl/libigl-tests-data	19cedf96d70702d8b3a83eb27934780c542356fe	quarantine/archives/libigl-tests-data-19cedf96d707.zip	88ba0e208884619a42b0956cb54ce76e022ae7c4f9a9d8a6fa4d947bda8b065a	quarantine/sources/libigl-tests-data	libigl-tests-data-19cedf96d70702d8b3a83eb27934780c542356fe	55	
opensubdiv	https://github.com/PixarAnimationStudios/OpenSubdiv	9dab8a47bfbb1388ec8388fe61f5f916e6123f38	quarantine/archives/opensubdiv-9dab8a47bfbb.zip	360e53c1949157c16957b4b719573a31dcee3a054d4e6729f03fa39620be4e21	quarantine/sources/opensubdiv	OpenSubdiv-9dab8a47bfbb1388ec8388fe61f5f916e6123f38	1145	
earcut	https://github.com/mapbox/earcut	9e70986eb94054c621c926888dd05bc26bffb902	quarantine/archives/earcut-9e70986eb940.zip	eb172033e9be47a210a7e977bef22e912e7f63d168a9d1b02bcd0fe3c11e9ac2	quarantine/sources/earcut	earcut-9e70986eb94054c621c926888dd05bc26bffb902	79	
spade	https://github.com/Stoeoef/spade	c8befc96bbbc1898a89cb19f9f3104a848936374	quarantine/archives/spade-c8befc96bbbc.zip	87f1965128423a3f33063c7ceb3e24602828416dcd0c8656d522d1ab42eac6b4	quarantine/sources/spade	spade-c8befc96bbbc1898a89cb19f9f3104a848936374	119	
thingi10k	https://github.com/Thingi10K/Thingi10K	cbac53559cfade2c4ffb2ef1ae8cae88462eaed4	quarantine/archives/thingi10k-cbac53559cfa.zip	1e8ab8b57cdf178c801ebcc91f94448f173464a98878c2bda028aaad5a50f45a	quarantine/sources/thingi10k	Thingi10K-cbac53559cfade2c4ffb2ef1ae8cae88462eaed4	13	dataset not acquired (Hugging Face Thingi10K/Thingi10K tag v1.5.0)
gdel3d	https://github.com/ashwin/gDel3D	a429be8d702a1f0f0d9366e14f7f492b1c071c9b	quarantine/archives/gdel3d-a429be8d702a.zip	9733d902d39b3277916553644115ea53fed918b9ffb849a06ede2d56caab8907	quarantine/sources/gdel3d	gDel3D-a429be8d702a1f0f0d9366e14f7f492b1c071c9b	39	
robust-predicates-rs	https://github.com/georust/robust	c2183a06cc4e68f24836f981acb08e28d4d5f3d3	quarantine/archives/robust-predicates-rs-c2183a06cc4e.zip	082d2877ddd2df51b71f89afe11f34341597b878f1ea596ba188765f52629323	quarantine/sources/robust-predicates-rs	robust-c2183a06cc4e68f24836f981acb08e28d4d5f3d3	18	
poly2tri	https://github.com/jhasse/poly2tri	1008179917fd38c1824b8fe810f020fef0e5f6b5	quarantine/archives/poly2tri-1008179917fd.zip	d7a09b6abccb3bd56468e56967d0886bab7d7deea7b90db0386e9347c181344c	quarantine/sources/poly2tri	poly2tri-1008179917fd38c1824b8fe810f020fef0e5f6b5	54	
mmg-ci-tests-mmg	https://static.bordeaux.inria.fr/mmg/mmg.tgz	0.0.20	quarantine/archives/mmg-ci-tests-0.0.20.tgz	d4dddb3a6a5a0bb6dcb5a4448aa0353b0fe0af7d775503b9785d32386ca83ca0	quarantine/sources/mmg-ci-tests/mmg	mmg	88	
mmg-ci-tests-mmg2d	https://static.bordeaux.inria.fr/mmg/mmg2d.tgz	0.0.38	quarantine/archives/mmg2d-ci-tests-0.0.38.tgz	9800964f642996bff3b54eb80d92d5062cd1f2fac30cd38590e58338f5ad6736	quarantine/sources/mmg-ci-tests/mmg2d	mmg2d	107	1 symlink: mmg2d/OptLsSurf_box/box_multiMat.mesh -> box.mesh
mmg-ci-tests-mmgs	https://static.bordeaux.inria.fr/mmg/mmgs.tgz	0.0.25	quarantine/archives/mmgs-ci-tests-0.0.25.tgz	ca8a719bbf5cfa79ae6969643b8258b6d9fce308e5587117ce6b8c0c84984dab	quarantine/sources/mmg-ci-tests/mmgs	mmgs	88	
triangle	https://www.netlib.org/voronoi/triangle.zip	1.6 (netlib, fetched 2026-10-17)	quarantine/archives/triangle-1.6-netlib-2026-10-17.zip	1766327add038495fa3499e9b7cc642179229750f7201b94f8e1b7bee76f8480	quarantine/sources/triangle	(none; flat zip)	7	
shewchuk-predicates	https://www.cs.cmu.edu/afs/cs/project/quake/public/code/predicates.c	May 18 1996 header (fetched 2026-10-17)	quarantine/archives/shewchuk-predicates-c-2026-10-17.c	f8662c3f407d1c1c5dcd4dd49ea8b8ddd801a71827d65734206ddc3741792029	quarantine/sources/shewchuk-predicates	(single file)	1	
tetgen	https://wias-berlin.de/software/tetgen/1.5/src/tetgen1.6.0.tar.gz	1.6.0	quarantine/archives/tetgen1.6.0.tar.gz	87b5e61ebd3a471fc4f2cdd7124c2b11dd6639f4feb1f941a5d2f5110d05ce39	quarantine/sources/tetgen	tetgen1.6.0	7	
blender-selected	https://github.com/blender/blender	3f4216e1bad1e76e9c2d4a00b4a9d09954593f47	quarantine/archives/blender-selected-3f4216e1bad1/ (10 raw files + SHA256SUMS)	see SHA256SUMS	quarantine/sources/blender-selected	source/blender/blenlib	10	only 10 files fetched; full repository not acquired
```

**Notes on the rows.**
- **Mmg CI corpora.** Each `.tgz` was fetched together with its `.version` file, the scheme Mmg's own `cmake/modules/LoadCiTests.cmake:40-104` uses, and extracted with `tar -xzf` into a new destination. The one symbolic link was deleted from the extraction; the archive still holds it. The mmg3d corpus (`https://static.bordeaux.inria.fr/mmg/mmg3d.tgz`, version 0.0.46, 332,793,613 bytes) was **not** acquired.
- **Triangle.** The zip has no top-level directory, so `ingest_zip.py --root` cannot import it. It was extracted with `unzip -q` into a new `quarantine/sources/triangle`. The 7 files contain no detritus.
- **TetGen.** Extracted with `tar -xzf --strip-components 1`. The tarball contains **no LICENSE file**; its README refers to "GENERAL LICENSE TERMS AND CONDITIONS" kept elsewhere.
- **Blender.** 10 files were fetched from `https://raw.githubusercontent.com/blender/blender/3f4216e1bad1e76e9c2d4a00b4a9d09954593f47/source/blender/blenlib/{tests,intern}/...`, and their digests are listed in `quarantine/archives/blender-selected-3f4216e1bad1/SHA256SUMS`:
  - `BLI_delaunay_2d_test.cc` `05098dde...`
  - `BLI_polyfill_2d_test.cc` `10bbd9b3...`
  - `BLI_mesh_intersect_test.cc` `a065ae7f...`
  - `BLI_mesh_boolean_test.cc` `a7a3bbba...`
  - `BLI_convexhull_2d_test.cc` `db41e8d3...`
  - `delaunay_2d.cc` `403bf225...`
  - `polyfill_2d.cc` `54627f28...`
  - `polyfill_2d_beautify.cc` `ea328c82...`
  - `mesh_intersect.cc` `1bc49aa8...`
  - `mesh_boolean.cc` `8bfac3b2...`
- **Detritus.** No `.git`, `__MACOSX`, `.DS_Store` or `._*` entries were found in any acquired ZIP. The only symlink in a ZIP is the mmg one listed above.
- **Licenses** (from the archives; see each tree):
  - CGAL: GPL/LGPL split per package (`cgal/LICENSE.md`).
  - Mmg: LGPL-3.0-or-later (`mmg/LICENSE`).
  - libigl, TetWild: MPL-2.0 and GPL (`LICENSE.MPL2`, `LICENSE.GPL`). fTetWild: MPL-2.0.
  - OpenSubdiv: Tomorrow Open Source Technology License 1.0, a modified Apache-2.0 (`opensubdiv/LICENSE.txt`).
  - earcut: ISC. spade, georust/robust: MIT/Apache-2.0. poly2tri: BSD-3. Thingi10K tooling: Apache-2.0. gDel3D: NUS BSD-style (README).
  - Triangle: "Private, research, and institutional use is free ... commercial system ... ONLY BY DIRECT ARRANGEMENT WITH THE AUTHOR" (`triangle/README:38-44`).
  - JIGSAW: commercial use only by arrangement (`whole-mesher-study/jigsaw/LICENSE.md`).

  This is study material. Nothing is selected as a dependency.

## Appendix B: referenced but not acquired or not read

| Item | Where referenced | Why it matters | Source |
|---|---|---|---|
| Thingi10K meshes (10,000 files) and metadata CSVs | `thingi10k/src/thingi10k/_builder.py:42-45,106-110,374-388` | Corpus and flag-table methodology. A triangle corpus, not B-rep | Hugging Face `Thingi10K/Thingi10K` tag v1.5.0 |
| TetWild (SIGGRAPH 2018) and fTetWild (TOG 2020) papers | `tetwild/README.md`, `ftetwild/README.md` | Thingi10K run criteria (timeouts, memory, failure definition) were not established | https://yixin-hu.github.io/tetwild.pdf, https://yixin-hu.github.io/ftetwild.pdf (add to SHOPPING_LIST if not fetchable) |
| geogram test data | `geogram/.github/workflows/continuous.yml:37-41` | CDT2d, Intersect and CSG inputs (`constraints_100K`, `triangle_duplicated_constraints`) | `BrunoLevy/geogram.data` |
| mmg3d CI corpus 0.0.46 | `mmg/cmake/modules/LoadCiTests.cmake` | NM_Cube, Santa, M6 and similar | `static.bordeaux.inria.fr/mmg/mmg3d.tgz` (333 MB) |
| fast-envelope (exact envelope) | `ftetwild/cmake/FloatTetwildDependencies.cmake:179-185` | Exact vs sampled envelope differential | `wangbolun300/fast-envelope` @ 520ee04b |
| CGAL testsuite Docker images and nightly results | `cgal/Scripts/developer_scripts/list_test_runner_machines:6-11` | Actual platform/compiler matrix | Docker Hub `cgal/testsuite-docker`; cgaltest.geometryfactory.com |
| artem-ogre/CDT external fuzz harness (Som1Lse) | `direct-cdt-study/CDT-53eff288a4e7/docs/README.md:364` | The fuzzer that found the minimized looping case | not public in tree |
| Blender full test registration, `BLI_math_mpq.hh`, buildbot CI | Blender selected-file gaps | Which Blender tests actually run | blender/blender at the pinned commit |
| Nanevski et al. 2001 predicate test set (original location) | `robust-predicates-rs/src/lib.rs:2237-2239` | Fixture provenance | cs.cmu.edu pscico archive |
| gCDT paper datasets | none in the 24 files | Which inputs the GPU CDT was validated on | not referenced |
