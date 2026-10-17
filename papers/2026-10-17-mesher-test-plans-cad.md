# Test plans of public CAD/B-rep tessellators: what they check, what they miss

2026-10-17. Research lane for the owned fast tessellator
(`journal/2026-10-17-fast-mesher-direction.md`). Reading only: nothing in this study
was built, run, or meshed. Agents parsed fixture bytes with `python3 -I`, grep and
awk. A sibling lane covers general mesh-generation and robustness libraries
(`papers/2026-10-17-mesher-test-plans-meshgen.md`).

Projects: OCCT BRepMesh (DRAW suites, GTests, public fixture dataset), Gmsh,
Netgen, SALOME SMESH, FreeCAD, CadQuery, build123d, truck, Fornjot, SolveSpace,
NVIDIA usd-brep, openNURBS and SISL. The bug trackers of OCCT (Mantis archive and
GitHub), FreeCAD, Gmsh, CadQuery, build123d, truck and Fornjot were also read. The
tracker lane surfaced one further project, step-conformance. It is young, but it is
the only suite here that scores meshes against exact geometry.

Paths in a project section are relative to that project's quarantine root, which
the section names. Each line citation comes from a reader who opened that file.
Claims the readers inferred rather than read are marked "inferred". Seven reader
lanes did the reading in parallel. This lane spot-checked the claims that carry
weight in the synthesis:
- OCCT `BRepLib.cxx:2873` boundary-link exclusion;
- `MeshTest_PluginCommands.cxx:525` early exit;
- `tests/parse.rules:1` SKIPPED rule;
- the ModelHealer `aIterNb = 5` cap;
- `bug25586_1` permanent TODO;
- presence or absence of tracker-named fixtures in the dataset.

## Decision-relevant findings in one page

1. **Almost no mature public mesher tests achieved deviation against exact
   geometry.** In the whole OCCT suite, one GTest
   (`BRepMesh_IncrementalMesh_Test.cxx:123-153`) does this, and it samples only
   triangle centroids. OCCT's reported "deflection" is self-measured. It skips
   boundary links and triangles at poles (`BRepLib.cxx:2793-2889`). Its regression
   goldens freeze violations: `bug29962` asserts 5.80 at a requested 0.01.
   Gmsh, Netgen, SMESH, FreeCAD, CadQuery, build123d, truck, Fornjot, SolveSpace and
   usd-brep measure nothing pointwise. The closest checks are truck's mesh-to-OCCT-mesh
   comparison at 10x slack, SMESH's centroid-only `FT_Deflection2D` (which it only
   prints), and loose whole-body volume/area checks. OCCT closed Mantis 0033691 as
   won't-fix: "BRepMesh does not guarantee the specified tolerance". **Our planned
   dense-deviation oracle has no mature precedent, and it must deliberately cover the
   places others skip:**
   - boundary links;
   - pole- and apex-adjacent triangles;
   - mid-edge points;
   - the surface-to-mesh direction, which is what finds missing surface.
2. **Silent per-face failure is the dominant field failure, and every project has
   its own way of hiding it.** Each of these looks like success:
   - Gmsh: an unmeshed face is a Warning, and the process still exits 0.
   - Netgen: missing faces print only `"WARNING! NOT ALL FACES HAVE BEEN MESHED"`.
   - FreeCAD: an unmeshable face becomes an empty domain.
   - CadQuery: the 3MF writer drops shapes that failed to tessellate.
   - truck: `to_polygon` skips `None` faces.
   - SolveSpace: a dropped face is reported only in the debug log.
   - usd-brep: partial output comes back with a success status.
   - OCCT: 0 triangles "without any error" (Mantis 0031865), and GH #1337 returns a
     sparse mesh with OK status.
   
   **Our output contract needs per-face accounting.** Every input face must yield
   triangles or an explicit refusal record. step-conformance's per-face triangle
   counts are a working example. Watertightness alone misses this on sheets and open
   shells.
3. **No suite terminates by construction, and NaN defeats the caps.**
   - Projects bound work with iteration caps: OCCT ModelHealer 5 rounds and
     surface control 11 passes; Netgen a 20k-sample cap, give-up at quality class
     200 and at 2x face area; Gmsh `MaxRetries` 10; SolveSpace `maxSegments`;
     SISL recursion depth 200.
   - Hang tests are external CPU limits that pass by finishing.
   - **NaN breaks loop guards.** Gmsh HXT `x/attempt` with `attempt==0` never
     breaks. truck curve subdivision with NaN fans out to depth 100 (inferred).
     OCCT's five `value < bound` parameter checks all pass NaN (PR #1598).
   - **A cap can silently override the tolerance.** SolveSpace stops subdividing
     at `maxSegments` even while the deviation still exceeds the chord tolerance
     (`ratpoly.cpp:258-275`).
   
   We need injected NaN at evaluator outputs, not only at ingress, and a rule that
   hitting a cap yields refusal, never output.
4. **Real B-reps are inexact, and our plan has no family for it.**
   - Stored tolerances far above nominal: Gmsh's A319 at 6.77, component8 at
     0.074, tank at 0.12.
   - Trims fitted 1e-4 to 1e-3 off their surfaces: step-conformance
     `freeform_panel`, truck #136.
   - Fusion STEP exports with no pcurves at all.
   - "SameParameter" flags that lie (Mantis 0023837).
   
   OCCT's reaction is to floor face deflection at twice the face tolerance
   (`BRepMesh_Deflection.cxx:128-154`) without saying so. ABC filtered healing-needing
   models out (journal). This is the **top-ranked gap** below. It needs a spec
   decision about which carrier "exact geometry" means when edge and surface disagree.
5. **Weld, orientation and export faults pass as mesher bugs.** Examples:
   - FreeCAD's tolerance comparator breaks strict weak ordering, and STL/3MF open
     edges result (#12744).
   - build123d's rounding-grid weld and float32 narrowing.
   - CadQuery's tests lock in unwelded 24-vertex boxes.
   - openNURBS judges closedness on position-welded vertices, so "a flattened box"
     can read as closed.
   
   Our shared-edge identity guarantee is the right design. The oracle must check
   identity, not welded positions.
6. **The checkers themselves have blind spots, and we should not copy them.**
   - OCCT `tricheck` stops scanning at the first face without edge polygons.
   - FreeCAD self-intersection skips triangle pairs that share a vertex, so it
     misses folds.
   - FreeCAD's NaN check is `isnan` only, so inf passes.
   - SolveSpace misses in-plane overlaps.
   - openNURBS: the validity verdict depends on whether a log pointer is passed.
   - usd-brep: release builds compile `SM_ASSERT` to `if(a) {}`.
   - OCCT turns missing fixtures into SKIPPED; usd-brep turns a missing LFS fixture
     into success.
7. **Oracle strength across projects is low.** Each project tallies as:

   | Project | Tier 1-3 share of tests |
   |---|---|
   | OCCT bugs/mesh | 82% (tier 6-7: 4 of 230) |
   | Gmsh | ~99% (exit code only) |
   | Netgen | OCC never reaches the golden suite (`has_occ = False`) |
   | SMESH | ~80% |
   | NVIDIA tessellation | ~35 of 45 checks |
   | CadQuery | 100% |
   
   The strong patterns worth adopting:
   - OCCT's metamorphic scale-invariance (`bug32982_2`);
   - SolveSpace's naked-edge + self-intersection + analytic-volume triple;
   - Fornjot's validator negative controls;
   - usd-brep's 191-rule ingress validator as a refusal catalog;
   - step-conformance's two-way exact distance with per-face accounting.
8. **Acquired corpora** (catalog rows in Appendix A):
   - The OCCT public fixture dataset. 121 of 210 file-using bugs/mesh cases have
     their fixtures, but most hang and memory fixtures are absent and must be
     re-authored.
   - Gmsh's hard industrial STEP/BREP benchmarks, which CTest never runs.
   - truck's test-data submodule, with four ABC STEP files and OCCT-written
     primitives.
   - step-conformance's CC0 corpus with exact truth.
   - Netgen, SMESH, FreeCAD, Fornjot sources.
   
   BRL-CAD's B-rep CDT (`libbrep/cdt`) is a mature owned tessellator that we could
   not acquire: GitHub `BRL-CAD/brlcad` returned 404 on 2026-10-17 (Appendix C).

# Per-project findings

## OCCT BRepMesh

OCCT 8.0.1, commit `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`,
`quarantine/new-iteration/occt` (the older `quarantine/sources/occt` snapshot at
`3d097a0328e7` was not used). Public fixtures: `opencascade-dataset-7.9.0.tar.xz`
(the archive OCCT CI downloads, `.github/actions/run-tests/action.yml:55-56`),
acquired by this lane and extracted to `quarantine/sources/occt-dataset-7.9.0`
(3,388 files, ~360 MB). Read in full: `tests/mesh/{grids.list,begin,end}`, all six
grid `begin` files, all 21 `tests/mesh/data/advanced/*`, the scripted
`data/standard/*` cases; **every one of the 230 cases in `tests/bugs/mesh/`** (the
`bug25044_1..58` and `bug25588_1..32` series verified identical apart from the
fixture name) plus `tests/bugs/{begin,end,parse.rules}`; all 15 `tests/perf/mesh/*`
and `tests/perf/{begin,end,parse.rules}`; `tests/parse.rules`; `tests/de_mesh`
control files and samples; `resources/DrawResources/TestCommands.tcl`
(`locate_data_file` 1181-1227, `_run_test` 1301-1431, `_check_log` 1434-1621,
`testdiff`, `_check_time`, `checktrend`), `CheckCommands.tcl` (`checktrinfo`
961-1088); `src/Draw/TKTopTest/MeshTest/{MeshTest.cxx, MeshTest_PluginCommands.cxx,
MeshTest_CheckTopology.cxx}`; `src/Draw/TKDraw/Draw/Draw_BasicCommands.cxx`
(`cpulimit` 529-640); `IMeshTools_Parameters.hxx`, `BRepMesh_Deflection.cxx`,
`BRepMesh_ModelHealer.cxx`, `BRepMesh_DelaunayDeflectionControlMeshAlgo.hxx`,
`BRepMesh_MeshAlgoFactory.cxx`, `BRepMesh_ModelPostProcessor.cxx`,
`BRepMesh_IncrementalMesh.{hxx,cxx}`, `BRepMesh_CurveTessellator.cxx`,
`src/ModelingAlgorithms/TKTopAlgo/BRepLib/BRepLib.cxx:2793-2889`; all six files in
`src/ModelingAlgorithms/TKMesh/GTests/`; CI testgrid scripts. Per-case fixture
resolution and tier assignments are in the lane's scratch tables (not repo files).

### Test inventory

- **`tests/mesh`**: 6 grids (`standard_shading`, `standard_incmesh`,
  `advanced_shading`, `advanced_incmesh`, and the two `_parallel` variants) over
  shared case lists `data/standard` (212) and `data/advanced` (21): 699 instances.
  Serial and parallel grids share expectations. `tests/mesh/end` loads the shape,
  `checkshape`, `tclean`, then `vdisplay` or `incmesh res ${Deflection} -parallel
  ${parallel}` (`:34-40`, `Deflection 0.1`), then `trinfo`, `triarea`, `tricheck`.
  177 standard files are `shading_NNN.brep`, 31 are `shading_wrongshape_NNN.brep`
  (deliberately invalid).
- **`tests/bugs/mesh`**: 230 cases, one primary class each:

| Fault class | n | Representative cases |
|---|---|---|
| Hang / infinite loop / too slow | 28 | `bug24938:6` (`cpulimit 100`, "BRepMesh_IncrementalMesh hangs"), `bug25281` (infinite-loop STEP, parallel), `bug31144:6` (`cpulimit 10`, OuterWire loop), `bug32767` (seam splitting), `bug33315`, `bug26692_1/2`, `bug27685`, `bug28118`, `bug31378`, `bug32982_1/2`, `bug29715`, 12 x `bug25378_*` |
| Memory (OOM, runaway, stack) | 4 | `bug23513` ("doesn't stop until out-of-memory"), `bug23631` ("infinite memory consumption"), `bug25364` (`meminfo` budgets), `bug25806` (stack overflow) |
| Exception / crash | 9 | `bug23105`, `bug26372`, `bug28194`, `bug30785`, `bug31125` (empty compound), `bug31865` (OutOfRange in `BRepMesh_PairOfIndex::Append`), `bug32692_1/2/3` |
| Cracks / free links / missing face mesh | 29 | `bug26291`, `bug27959` (Delaunay gaps on internal edges), `bug30149` (free triangle), `bug29149`, `bug24593_1/2`, `bug25045`, `bug28719`, `bug31853` |
| Wrong deflection / under- or over-refinement | 20 | `bug33050`, `bug29962` (spikes missed), `bug25287` ("way out of tolerance"), `bug27693` (stripes), `bug22778` (NURBS vs analytic), `bug32471` |
| Poles / cone apex | 2 | `bug25042`, `bug28379` |
| Seams / periodic / closed surfaces | 6 | `bug25586_1/2/3`, `bug23512_1/2`, `bug31476` |
| Tiny / thin / sub-precision features | 10 | `bug25080`, `bug25817` (crescent), `bug25827`, `bug26664`, `bug30234` (zero-length edge), `bug31199_1..4`, `bug33060` |
| Tolerance / units / scale | 40 | 32 x `bug25588_*`, `bug25044_59/60`, `bug25837_1/2` (IGES in metres), `bug27158`, `bug31251`, `bug32053` |
| Self-intersecting / open wires | 5 | `bug25061`, `bug23202`, `bug25628`, `bug24775` (false intersection), `bug31926` |
| Parallel / platform nondeterminism | 1 (+1) | `bug29751` ("different meshes for windows and linux"), `bug32241` |
| API / state / parameter semantics | 15 | `bug25469_1..3`, `bug25479` (meshing one shape corrupts another), `bug25503_1/2`, `bug25612`, `bug26532`, `bug28247`, `bug31461`, `bug28089_1/2` |
| Generic regression collections | 60 | `bug25044_1..58`, `bug30167`, `bug31258` |
| Vacuous | 1 | `bug23580` (variables set, nothing checked) |

  Only 19 bugs/mesh cases build geometry inline; 210 use `locate_data_file`.
- **`tests/perf/mesh`**: 15 timing cases (`dchrono ... counter`), e.g. `bug23650` (one
  B-spline, "80 seconds for 132 triangles"), `bug24968_1/2`, `bug26965` (hang plus
  infinite memory), `bug30511` (1,119,744 triangles).
- **`tests/de_mesh`**: 117 cases, mostly mesh-format I/O; checks are post-round-trip
  `checktrinfo` counts.
- **GTests**: 24 in `src/ModelingAlgorithms/TKMesh/GTests` (Delaun 6,
  DiscretAlgoFactory 9, BaseMeshAlgo 3, IncrementalMesh 4, CircleTool 1, GeomTool 1);
  `TKXMesh/GTests` lists no files.

### Oracles

- **Implicit for every DRAW case** (`tests/parse.rules`, `bugs/mesh/parse.rules`):
  the log fails on `\bException\b`, `\bError\b`, `\bFailure\b`, `\bFailed\b`, "Process
  killed by CPU limit", and (bugs/perf mesh) "Not connected mesh inside face".
  `incmesh` prints `Meshing statuses: ... Failure` (`MeshTest.cxx:271-318`), so every
  `incmesh` implicitly asserts no Failure status. Tier 2. (`bug33060:2` spells its
  title "f a i l u r e" to dodge the rule.)
- **`tricheck`** (`MeshTest_CheckTopology.cxx:53-246`): free links, free nodes,
  async shared-edge polygons, cross-face node distance > `BRep_Tool::Tolerance(edge)`
  (`:114`), "Not connected mesh inside face", optional zero-area. Tier 5 but
  topological only: no orientation, no self-intersection, no geometry. Blind spot:
  the connectivity loop ends at `if (aBoundaryEdgeMap.Size() == 0) break;`
  (`MeshTest_PluginCommands.cxx:525`, verified by this lane), so one face without
  edge polygons stops the scan for all later faces (reading of the code).
- **`checktrinfo`** (`CheckCommands.tcl:961-1088`): bare flags assert nonzero (tier
  2); numbers are goldens via `checkreal` `abs(v-e) > tol_abs + tol_rel*|e|` (tier 3);
  `-max_defl` bounds the **self-reported** deflection (used only by `bug22778`,
  `bug30167`).
- **Area**: `triarea` vs `BRepGProp` area, asserted only when `max_rel_tol_diff > 0`
  (defaults 0, `mesh/begin:6`, `bugs/begin:12`) and then against a stored error
  (`checkreal "area difference" $rel_err $rel_tol $max_rel_tol_diff 0`) — a golden for
  the error, not a bound. Enabled in 16/212 standard and 6/21 advanced cases.
  `data/standard/V7` locks a 77.709% area error to ±0.001; J4 (6.5%) and S8 (47%)
  print it and never assert.
- **Strong lines**: tier 6 `bug25837_2:27` `checkreal "Relative area" $r_area 1.0 0.01 0.0`;
  tier 7 `bug32471:10` `checktrinfo result -tri 2 -nod 4 -defl 0 -tol_rel_defl 1e-7`,
  `bug30234:15` `-tri 0 -nod 0`; tier 4 `bug25612:23` (`-surf_def_off` must give
  fewer triangles), `bug26532:116` (edge polygon identical with and without `-min`),
  `bug32982_2` (`tscale ... 0.001` with deflection scaled must reproduce `-nod 4036 -tri
  7450`: scale invariance), `bug25469_1:22` (meshing shape 2 leaves shape 1
  unchanged), `bug22778:29` (NURBS-converted face <= 2x analytic triangles, TODO).
- **The only test of achieved deviation against exact geometry**:
  `BRepMesh_IncrementalMesh_Test.cxx:123-153` — UV nodes in the trimmed range and on
  `aCylinder->Value(u,v)` within `Precision::Confusion()`, and per triangle
  `EXPECT_LE(anActualPoint.Distance(anExpectedPoint), THE_DEFLECTION)` at the
  **centroid only**. `BRepMesh_CircleTool_Test.cxx:38-84`: 100k seeded random
  triangles, passes if `aFailRate <= 0.01`.

Tally of strongest oracle per bugs/mesh case: tier 1 ~25, tier 2 ~129, tier 3 34,
topological tier 5 29, tier 4 9, tier 6-7 4. **Tiers 1-3: ~188/230 (82%)**; tiers
4-7: 13 (6%), or 42 (18%) counting topological tricheck. 194/230 also save
screenshots compared only advisorily. Mesh grids assert tricheck clean, >= 1
triangle, no Failure, except where 37 standard and 12 advanced cases carry `bug_*`
exemptions; counts are printed as XML properties but never asserted.

### Corpora

`locate_data_file` searches the script's `data/`, then `CSF_TestDataPath`
subdirectories breadth-first (`TestCommands.tcl:1181-1227`). **A missing file
raises a Tcl error that `tests/parse.rules:1` turns into SKIPPED, not FAILED**
(`SKIPPED /Tcl Exception: .*[fF]ile .* could not be found/ data file is missing`,
verified). Against the public dataset:

| Area | cases with all fixtures present | unique files present |
|---|---|---|
| bugs/mesh | 121 of 210 file-using | 120 of 192 |
| perf/mesh | 5 of 15 | 5 of 11 |
| mesh/data | 83 of 231 | 82 of 228 |
| de_mesh | 8 of 95 | 3 of 83 |

Present mesh-relevant files total 210 files, 29.7 MB; all 58 `bug25044_face*.brep`
and 32 `1.stp_face*.brep` are present; 72/179 `shading_NNN` and 7/31
`shading_wrongshape_NNN` are present. **Most hang/memory fixtures are absent**
(verified by this lane): `bug23513_infface.brep`, `OCC396_f2903.brep` (bug23631),
`bug25281_tess_infloop_extract.step`, `bug28118_18547.stp`, `bug25364_WT_Grundplatte.brep`,
`bug25378_Blower_{bad,good}.brep`, `OCC25806_shape_1040739_1.brep`,
`bug26692_face_bad_shading.brep`, `bug27685_MyShoeLast.igs`, `bug33315.brep`,
`bug32982.brep`, `bug24968_Shape_1.brep`, and thin/tiny/deflection fixtures
`bug31199_*`, `bug33060.brep`, `bug33050.brep`, `bug29149.brep`, `bug25287_bad_tess_face_plus.step`.
In public CI those cases are silently SKIPPED.

### Pathology handling

- `cpulimit N` (`Draw_BasicCommands.cxx:595-640`): `setrlimit(RLIMIT_CPU)` plus a
  5 s elapsed-time watcher thread, "Process killed by CPU limit", `exit(2)`;
  defaults 1000 s (bugs, mesh) and 500 s (perf); per-case 100 (bug24938), 10
  (bug31144). **Hang tests pass by finishing**; most add only a count golden.
- Memory: `_run_test` logs `MEMORY DELTA` (`meminfo h`, `:1413-1416`); `testdiff`
  flags > 16 KiB + 5%, advisory only. Only `bug25364` asserts memory (per-counter MB
  budgets, `:66-101`). `MemLeakTest` (`MeshTest.cxx:482-498`, 10,000 mesh+clean
  iterations) is called by no test.
- Mesher-side bounds the tests rely on: ModelHealer amplification at most 5 rounds
  (`BRepMesh_ModelHealer.cxx:179`, verified: `const double aIterNb = 5`), dividing
  edge deflection by 3 each round (`:57`), then `SelfIntersectingWire|Failure`
  (`:161-162`); surface deflection control at most 11 passes
  (`BRepMesh_DelaunayDeflectionControlMeshAlgo.hxx:85`); per-face exceptions caught
  with `OCC_CATCH_SIGNALS` into `IMeshData_Failure` (`BRepMesh_FaceDiscret.cxx:109-129`).
- Malformed input: `bug32692_1/2/3` REQUIRE specific statuses ("SelfIntersectingWire
  Failure", "Reused", "OpenWire Failure Outdated"); `bug30785` REQUIRES "Failure"
  (explicit refusal); `bug31125` empty compound; `bug30234` zero-length edge.
- **NaN/inf: none.** Test deflections range 1e-6..1000. `initParameters` rejects
  `Deflection < Precision::Confusion()` (`BRepMesh_IncrementalMesh.hxx:81-104`,
  verified); a NaN deflection passes every `<` check (inferred here; the tracker lane
  found OCCT PR #1598 proposing exactly this fix).

### Fuzzing / mutation / property / performance

No fuzzing, mutation or sanitizer configuration. Property-based: only the seeded
CircleTool GTest. Performance: `dchrono` counters compared by `testdiff`
(`_check_time`, > 0.5 s + 5%), advisory; in-test time assertions only in 9
`bug25378_*` cases (`checktime`, ±5% of a hard-coded time) and `bug25503_1:19`.
macOS CI runs only `testgrid caf basic`; mesh DRAW tests run on Linux and Windows;
`retest-failures` re-runs FAILED cases (can hide flakes).

### Tolerance semantics

`IMeshTools_Parameters.hxx` defaults: Deflection 0.001, Angle 0.5 rad,
DeflectionInterior/AngleInterior default to Deflection / 2 x Angle, MinSize 0.1 x
min(Deflection, DeflectionInterior), InParallel false, Relative false,
InternalVerticesMode true, ControlSurfaceDeflection true,
EnableControlSurfaceDeflectionAllSurfaces false, CleanModel true (forced false in
`IncrementalMesh::Perform`), AdjustMinSize false, ForceFaceDeflection false,
AllowQualityDecrease false.
- Edge deflection (`BRepMesh_Deflection.cxx:66-98`): absolute, or relative
  `clamp(maxShapeSize/(2*edgeBoxSize), 0.5, 2) * edgeBoxSize * rel`, then raised to
  the vertex-to-curve-end distance (`:94`). Face deflection (`:128-154`):
  `max(DeflectionInterior, max(2*MaxFaceTolerance, mean wire deflection))` unless
  ForceFaceDeflection — **a face with inflated tolerance is meshed coarser by
  design** (bug31251). Mesh reuse if `current < 1.1*required` (`:158-167`).
- Surface control samples triangle centroids and non-frontier link midpoints
  (frontier links skipped, `:301`), stops on MinSize, angle or 11 passes; analytic
  surfaces skip it unless `-surf_def_all` (`BRepMesh_MeshAlgoFactory.cxx:55-100`).
- `trinfo` "Maximal deflection" is `Poly_Triangulation::Deflection()` set by
  `BRepLib::UpdateDeflection` (`BRepLib.cxx:2793-2889`): centroid and interior-link
  midpoint samples only — "Do not estimate boundary links due to high distortions at
  the edge" (verified at `:2873`) — and triangles touching pole-edge nodes skipped
  (`:2812`). **Self-reported and sampled; never edge-vs-curve, never at poles.**
- `-defl` goldens freeze violations: `bug33050:12` 0.0027 at requested 0.001;
  `bug29962:30` 5.80 at 0.01; `bug29149:10` 1.46 at 0.1; `bug31251` 0.246 at 0.004.
  `checkdeflection` does not exist in this tree; `veriftriangles`
  (`MeshTest.cxx:1070-1217`, 7 UV samples per triangle) is stronger and used by no test.

### Known-bad handling

`TODO <platforms>:<regex>` marks lines as known (BAD); `?` makes it unstable;
an unmatched TODO yields IMPROVEMENT; `REQUIRED` lines must appear and are exempt
from bad-word rules (`TestCommands.tcl:1501-1597`). `tests/mesh/end:44-108` turns
`bug_*` variables into TODOs whose free-link/free-node patterns ignore the count.
`bug25586_1/2/3:1` TODO the `checkprops -s` result, so they are permanently BAD and
check nothing (verified: `puts "TODO OCC25586 ALL: Error : The area of result shape
is"`). `bug25307:1` REQUIRES "Not connected mesh inside face", locking a defect in place.

### Adoptable OCCT fixtures

Dataset paths relative to `quarantine/sources/occt-dataset-7.9.0`.

| Case | Fixture (present?) | Fault class | Upstream tier |
|---|---|---|---|
| bugs/mesh/bug24938 | brep/bug24938_27773.brep (yes) | classifier hang (relative 1.5) | 3 + cpulimit |
| bugs/mesh/bug31144 | brep/bug31144.brep (yes) | infinite loop via OuterWire | 1 |
| bugs/mesh/bug32767 | brep/bug32767.brep (yes) | seam-edge splitting hang | 3 |
| perf/mesh/bug26965 | brep/bug26965_mesh_hang.brep (yes) | hang + runaway memory | 3 + counter |
| perf/mesh/bug23650 | brep/bug23650_slowmesh.brep (yes) | one B-spline, 80 s for 132 triangles | 2 |
| perf/mesh/bug23795 | brep/bug23795_s.brep (yes) | performance regression | 3 |
| perf/mesh/bug25264 | brep/bug25264_wire.brep (yes) + inline `revol` | near-degenerate revolution axis (~2e-9) | 1-2 |
| perf/mesh/bug30511 | brep/bug30511_hugeassembly0.brep (yes) | shared solid instanced ~93k times, 1.12M triangles | 3 |
| bugs/mesh/bug29715 | step/bug29715_slow.stp (yes) | slow (acceleration-grid sizing) | 2 |
| bugs/mesh/bug31865 | brep/bug31865.brep (yes) | large deflection -> 0 triangles, OutOfRange | 5-topo |
| bugs/mesh/bug32692_2 | brep/bug32692_broken_mesh.brep (yes) | broken prior mesh reuse | 3 (REQUIRED) |
| bugs/mesh/bug32692_3 | inline `cylinder c 0 0 0 10; mkface f c` | infinite face, no wires | 3 (REQUIRED) |
| bugs/mesh/bug31125 | inline empty `compound` | empty input crash | 2 |
| bugs/mesh/bug30234 | inline `circle cc 0 0 0 0` edge | zero-length edge | 7 |
| bugs/mesh/bug25042 | inline `pcone 200 0 500` | cone apex, missing edges | 3 + 5-topo |
| bugs/mesh/bug28379 | inline `pcone s 0 0.1 1`, defl 1e-3 | tiny cone apex | 1 |
| bugs/mesh/bug29962 | inline 8x8 B-spline with two 999 spikes | interior features missed by sampling | 3 (golden 5.80 at 0.01) |
| bugs/mesh/bug24127 | inline `bsplinesurf` | deflection not respected (midpoint-only split) | 3 |
| bugs/mesh/bug30008_2 | inline cylinder cut by rational spline, nurbsconvert | interior angular deflection on B-spline | 3 |
| bugs/mesh/bug29751 | inline sphere minus cylinder, rotated 45 deg | cross-platform nondeterminism | 3 + 5-topo |
| bugs/mesh/bug32241 | inline thrusections, axis 2000 away | knot-seeded sampling, per-OS goldens | 3 |
| bugs/mesh/bug31926 | inline sewn tetrahedron cut from a sphere | wrong outer-wire selection | 5-topo |
| bugs/mesh/bug25157 | inline half cone minus cylinder | missing face at revolution <= 180 deg | 2 |
| bugs/mesh/bug32471 | brep/bug32471.brep (yes) | planar deflection must be 0 | 7 |
| bugs/mesh/bug25837_2 | iges/bug25837_S.igs (yes) | IGES in metres | 6 |
| bugs/mesh/bug25586_1..3 | brep/bug25586_{circle_extrusion, cylindrical_with_circle_hole, quasi_closed}.brep (yes) | closed/periodic surfaces with holes, narrow notch | 1 (permanent TODO) |
| bugs/mesh/bug26372 | brep/bug26372_RP.brep (yes) | surface of revolution failure | 3 |
| bugs/mesh/bug27239 | brep/bug27239_f1.brep (yes) | duplicate polygon nodes | 5-topo |
| bugs/mesh/bug27453 | step/bug27453_165-174_piattello.stp face 13 (yes) | wrongly meshed surface | 5-topo |
| bugs/mesh/bug27693 | step/bug27693_cornetta-step.stp face 4 (yes) | over-refinement in U/V stripes | 3 |
| bugs/mesh/bug27845 | brep/bug27845_shape.brep (yes, 901 KB) | spline cavity at coarse 10/180 deg | 3 + 5-topo |
| bugs/mesh/bug28719 | brep/bug28719_display_issue.brep (yes, 9.6 MB) | faces left untriangulated | 5-topo |
| bugs/mesh/bug30149 | step/bug30149_comb_bug.step (yes) | free triangle after Delaunay cleanup | 5-topo + 3 |
| bugs/mesh/bug31449_2 | brep/bug31449_face_loopback_M.brep (yes) | slow + free nodes at 1e-4 | 5-topo + 3 |
| bugs/mesh/bug25594 | brep/bug25594_face1.brep (yes) | valid face not shaded | 5-topo + 3 |
| bugs/mesh/bug25045 | step/bug25045_ick_0433.stp (yes) | faces fail to mesh | 5-topo |
| bugs/mesh/bug32053 | brep/bug32053_prob_face.brep (yes) | invalid mesh after scaling | 3 |
| bugs/mesh/bug26532 | brep/bug26533_aal2.brep (yes) | MinSize distorts edge polygon | 4 |
| bugs/mesh/bug30442 | others/bug30442_points.txt (yes) + inline pipe | pipe sweep triangulation | 3 |
| bugs/mesh/bug25044_1..58 | brep/bug25044_face1..58.brep (all yes, 2.6 MB) | heterogeneous single faces | 2 |
| bugs/mesh/bug25588_1..32 | brep/1.stp_faceNNN.brep (all yes, 1.9 MB) | FindUV 2-D tolerance inconsistency | 2 |
| mesh/data/advanced/A6 | brep/OCC21961-shd002A8-f204.brep (yes) | self-intersecting wire -> no triangulation | 2 (TODO) |
| mesh/data/standard/V7 | brep/shading_wrongshape_018.brep (yes) | invalid shape, 78% area error locked | 3 |
| mesh/data/standard/V6 | brep/shading_wrongshape_017.brep (yes) | invalid shape -> Failure | 2 (TODO) |
| mesh/data/standard/S8 | brep/shading_170.brep (yes) | 47% area error never asserted | 1 |
| bugs/mesh/bug23513 | brep/bug23513_infface.brep (**absent**) | out-of-memory face | re-create |
| bugs/mesh/bug31199_2 | brep/bug31199_failure1.brep (**absent**) | thin face | re-create |

Appendix B lists every present bugs/perf fixture outside the two big series.

### Fault classes learned

Edge-amplification non-termination (needs a hard cap); surface-refinement runaway on
distorted B-splines; OOM on infinite/untrimmed faces; recursion stack overflow;
healing infinite loop reached through the mesher; seam splitting hang;
near-degenerate revolution axis slowdown; interior features missed by
under-sampling; deflection target not reached while the stored deflection exceeds
the request; U/V stripe over-refinement; tolerance-inflated deflection; unit/scale
sensitivity; 2-D same-point tolerance mismatch; cracks between adjacent faces; free
triangles/islands; faces silently left without triangulation; free/duplicate
nodes; cone apex/pole degeneracy; closed periodic surfaces with holes; thin/crescent
faces; sub-precision links and zero-length edges; glued vertices on small edges;
self-intersecting internal wires blocking a face; open wires checkshape calls valid;
false self-intersection detection; exceptions at large deflection; empty/invalid
topology crashes; cross-shape state corruption through shared edges; mesh
reuse/staleness; platform nondeterminism; MinSize side effects. Harness blind spots:
missing fixtures SKIP; area check off by default and golden-not-bound; tricheck early
exit; self-reported deflection excludes boundary links and poles.

---

## Bug trackers: field-failure classes

Web reading only (no attachments downloaded).

**Reach.** `tracker.dev.opencascade.org` no longer resolves (DNS); the OCCT Mantis
tracker survives as a static archive at `https://occt3d.com/dev/tickets/<7-digit id>/`
with a search index `https://occt3d.com/dev/ticket-search.json` (6.6 MB, 7,498
tickets; ids run to ~33,900, so many are missing). Ticket pages state "Binary files
are not included in the public website build"; attachments survive only where they
became OCCT dataset fixtures. **159 tickets in category OCCT:Mesh** were triaged by
title and excerpt, 144 further mesh-related titles scanned, 86 pages fetched, ~60
read in detail. OCCT GitHub: ~70 titles, 15 issues + 7 PRs read. FreeCAD: ~250
titles, 23 issues + 2 PRs read. Gmsh: the GitLab web UI 404s (issues moved to
`/-/work_items/`), but the anonymous REST API returns titles/descriptions
(`gitlab.onelab.info/api/v4/projects/3/issues?search=`); notes return 401, so root
causes on GitLab were unreachable; 466 issues by title search, 17 descriptions read,
5 fix commits read via the GitHub mirror. CadQuery 8, build123d 9, truck 12, Fornjot
12 issues read. Also `dan335/step-conformance` (acquired; see its own section) and
PrusaSlicer #12122.

**OCCT:Mesh tally (159; classes overlap):** non-defect 42; of 117 defects —
unspecified wrong/bad mesh 37, seam/periodic/cone/sphere/revolution 20, missing
face 17, hang/too slow 16, deflection/tolerance 15, crash 11, gap/hole/free nodes
10, self-intersecting wire/tiny edge 6, degenerate 6, memory 5, parallel 3 (+2).
Many closures read "no change required", "won't fix", "unable to reproduce" or
"fixed in frame of #0026106" (the 7.4.0 mesher rewrite), often without a dedicated test.

### Failure classes with evidence

1. **Hangs / non-termination.** Mantis 0024938 (recursive
   `checkWiresIntersection` changes its own loop constraints; test `bug24938`
   `cpulimit 100` + counts); 0028118 (periodic B-spline edge range `-1.31e-11 ..
   0.2145` tessellates into 90K points, stack overflow in UBTree; test only `tri > 0`);
   0032767 (scaled B-spline seam split wrongly, regression since 7.4.0); 0026965
   (sweeps hang allocating 7-30 MB/s; reviewer asked for "some default restriction
   against insertion of infinite number of points"; no dedicated test); 0033082 (open:
   stuck when angular deflection < 0.2); OCCT GH #1337 (box fused with a 2*pi torus
   ring whose trims cross the seam: 7.9.3 48 ms / 12,586 triangles; 8.0.0p1
   **12,506 ms / 446 triangles with OK status**; GTest asserts only
   `EXPECT_GT(aTorusTriangles, 1000)`); Gmsh #623 (1D mesh with 1022 intersections
   loops forever; conic-fillet variant fine), #3392 (`recover_edge` infinite loop),
   #1107 (vertices 1e-6 apart hang algorithm 9); **Gmsh fix commit 229c75006**:
   `conflictRatio = x/stat.attempt` with `attempt==0` gives NaN, `== 0.0` is never
   true, the loop never breaks.
2. **Runaway memory.** Mantis 0023631 (one face consumes memory until bad_alloc;
   test meshes at 0.01 rather than the reported 1.195); 0025310 (per-face temporaries
   kept for all faces); 0031449 (unit-M round trip hangs and eats memory); Gmsh #415
   (leak + infinite loop in `OCC_Internals::makeFaceSTL`); **truck #89**
   (`put_together_same_attrs` builds a 120 GB adjacency matrix; NaN normals
   suspected); step-conformance `plate_900_holes`: OCCT 14.8 s / 1000 MB, 48x the
   100-hole time for 9x the size.
3. **Cracks / gaps / free nodes.** Mantis 0023167/0023409 (a hole that "neither
   incmesh nor tricheck nor trinfo ... notice"; fixed by changing tricheck — a
   checker false negative); 0030149 (free triangle after Delaun cleanup); 0023837
   (open: "BRepMesh shouldn't trust the SameParameter flag" — cross-face mismatch);
   **FreeCAD #12744** (vertex `std::set` comparator with a 2.2e-15 tolerance breaks
   strict weak ordering, duplicate points survive, STL/3MF open edges; fixed with a
   test in `tests/src/Mod/Part/App/BRepMesh.cpp`); build123d #232 (3MF: 9,720 open
   edges, 50 shells); CadQuery #390 (unwelded per-face output); FreeCAD #25514
   (non-manifold at fillets only when deviation is reduced); Fornjot #60/#430
   (Delaunay + point-in-polygon filter removes boundary segments).
4. **Orientation.** Thin evidence: OCCT GH #1352 / Mantis 30515 (tricheck false
   positive on an INTERNAL seam face), truck #55 (counter-clockwise wires break STL),
   Fornjot #1935 (validate outward faces).
5. **Self-intersecting discretized wires.** Mantis 0024084 (open since ~2013,
   parent of 23105/25586/31421/32462), 0025589 (interference checker misses some),
   0031421 (valid face, wires 0.40 apart, export fails), 0025061 (TODO test); Gmsh
   #1242, #432, #3260.
6. **Wrong deflection.** Mantis 0024127 (`splitSegment` "takes only one(!) sample
   point in the middle"; deviation > 2.5 at 1.0); 0025287 (requested 1e-4, observed
   3x: deflection floored at 2x shape tolerance; test pins the out-of-tolerance
   `-defl 0.00031502...`); **0033691 (won't fix): "BRepMesh does not guarantee the
   specified tolerance ... provides results UP TO the specified deflection ... can
   skip even the very obvious cases if those cases lie outside the considered test
   points."**; 0029962 (B-spline bump ignored at any deflection); 0032241 (NURBS
   range splitter seeds per knot interval only); FreeCAD #25311 (triangle 0.75 mm
   inside the part at 0.1 mm deviation); truck #136 (cone tip 30 mm off; fitted trims
   1e-4..1e-3 off the surface defeat a 1e-6 inversion tolerance; file unit ignored).
7. **Missing faces with OK status.** **OCCT GH #572 / PR #1605**: cone seam pcurves
   are infinite `Geom2d_Line`s; `SeamEdgeAmplifier::splitEdge` evaluates at
   `FirstParameter()` = -2e100 (X ~ -3e84) and always swaps the pcurves, so the wire
   self-crosses and the face is dropped (also PrusaSlicer #12122's missing chamfer);
   Mantis 0025157 (face missing at revolution <= 180 deg, fine at 181), 0027595 (38
   faces from an unhandled `gp_VectorWithNullMagnitude`), 0031865 (large deflection
   -> **0 triangles without any error**); CadQuery #705 (crash on `None` triangulation).
8. **Degenerate triangles.** OCCT GH #1558 (helical pipe: 18 + 14 zero-area triangles
   at the seam from distinct indices for coincident points, `statusFlags=0` —
   silent); Mantis 0027798/0025042 (developer: "it will always provide at least one
   degenerative triangle near the apex of the cone and e.g. pole of the sphere ...
   please skip them manually"); Fornjot #145 ("not a triangle" panic).
9. **Crashes / exception safety / NaN.** Mantis 0031125 (empty compound null
   dereference), 0026321 (catch block uses a member the try block nulled), 0030345 /
   0030347 / 0030234 (tiny deflection, 0-parameter pcurve, zero-length edge); OCCT GH
   #507 (ASan container-overflow, 7.7+); **PR #1572** (swallowed edge failure leaves
   an empty pcurve, then `GetPoint(-1)` with range checks compiled out — out-of-bounds
   write in release); **PR #1598** (every parameter check is `value < bound`, so NaN
   passes all five); PR #1450 (iterator invalidation in the Delaun kill cascade,
   verified only by byte-identical output); GH #688 / PR #695 (Delaunay recursion
   overflows TBB's 1 MB stacks; with OCCT's own pool the overflow is caught and glTF
   silently loses geometry); FreeCAD #27752 (throw reaches `std::terminate`); truck
   #128/#123 (`ref_direction` parallel to axis -> NaN placement, panics much later as
   "tolerance must be no less than 1e-6"; reproducers are NIST PMI files).
10. **Nondeterminism.** Mantis 0022600 (TBB disabled for data races), 0023260 (mutex
    map keyed by `TopTools_ShapeMapHasher`, which accounts for location, so located
    copies of one edge were unprotected), 0029751 (Windows 1223 vs Linux 1221
    triangles); Gmsh #3278 (1-in-1000 freeze on specific Xeons); **build123d #1492**
    (merged vertex chosen by memory address -> BREP and mesh differ between
    processes); step-conformance (truck `sled_base` 0.069 mm off on x86, passes on M2);
    FreeCAD #7393 (GUI 2,505 vs CLI 13,360 vertices from hidden context parameters).
11. **Poles / seams / periodic.** OCCT GH #1337, #1558, #572, #430; Mantis 0032767,
    0028118, 0022818/0023282, 0033476 (torus fillet edges 42 vs 43 points); Gmsh
    "Impossible to mesh periodic surface" (#1941 mesh-size dependent, #2237, #3147,
    #1242), #2683; step-conformance: closed B-spline faces with no seam edge lost by
    one reader, cone-to-point failed by every reader but OCCT.
12. **Tiny edges / near-coincident vertices.** Gmsh #1107; Mantis 0030234, 0027226,
    0026532 (`-min 0.06` makes the last segments huge; the test was criticised as
    having "no meaning due to absence of any logical check"); Fornjot #494, #1689;
    build123d #881.
13. **Tolerance coarser than features.** Mantis 0021415 (won't fix: vertex tolerance
    ~40 on a radius-20 cylinder, "mesh is adjusted to vertices"), 0025287, 0023918,
    0031449, 0025837, 0031865, 0023400; truck ignores file units.
14. **Huge triangle counts.** FreeCAD #17183 (1e7 km sphere; size-derived deflection
    clamped to an upper limit, `ViewProviderExt.cpp` ~L978; fixed by removing the
    clamp), #32865 (ball screws reach "billions of triangles"; proposed 2,000,000 cap),
    #12896 (document-stored deviation ~0.01 makes a file unopenable), #17175 (45 s on
    7.6.3 vs 1 s on 7.3); Mantis 0027693 (strip propagation), 0030511 (93k shared
    instances, 30 s), 0028247 (`-min` changes count 4,357 -> 14,412).

### Cross-cutting

- **Silent bad output is the dominant field symptom**: status flags 0 with a wrong
  mesh (GH #1558, #1337; Mantis 0031865, 0023167); the checker itself misses or
  misreports faults (0023409, #1352).
- **Rewrites cause regression clusters caught by users, not tests** (#0026106 ->
  32253, 32241, 32767, 30345, 30347, 31865, 31125; 7.7.0 -> #572; PR #745 -> #1337).
- **"Change your parameters" closures** (0033691, 0023918, 0021415, #682, #430,
  FreeCAD #12896/#17755) show OCCT's contract is weaker than "deflection <= tolerance".
- **Weld, export and display-mesh-as-export faults masquerade as mesher bugs**
  (FreeCAD #12744, #29811, #25514; build123d #232; CadQuery #390). An owned
  tessellator should own vertex identity and manifold output as part of its contract.

### Reproducers worth acquiring (not acquired; licences unstated)

GitHub attachments: OCCT #507 `file1.step.zip`, #688 `3066.zip`, #572
`Sfacciata.zip`, #1444 `occ.zip`, #682 `Smiley.step.txt`; inline generators in #1558
(helical pipe), #1337 (box + 2*pi ring), #430 (small sphere); FreeCAD #12744
`OpenEdges.FCStd`, #25311 `mesh_export_issue.zip`, #20456, #25514 `GE-G.2.step.zip`,
#17175 `freezesocct76.zip`, #33251, #29811; truck #41, #123 `Ear.stp`, #128 (NIST
`nist_ctc_04_asme1_rd.stp`, `nist_ctc_04_asme1_ap203.stp`, `nist_stc_07_asme1_ap242-e3.stp`
— already in our NIST plan). Gmsh uploads (anonymous download unverified): #623
`Hook-circular.step` (+ control `Hook-conic.step`), #415, #1107, #3167
`gmshissue.brep`, #541 `thing.step`, #2237, #3147 `debug.step`, #3278, #3260. Mantis
attachments 0033082, 0026965, 0033476, 0033681, 0033314, 0033906, 0025589, 0023631,
0026074 are not public; several tracker-named fixtures are also **absent from the
public dataset** (verified: `bug25281_tess_infloop_extract.step`,
`bug28118_18547.stp`, `OCC396_f2903.brep`, `bug31301.stp`,
`bug25287_bad_tess_face_plus.step`, `bug23105_f372.brep`, `bug25551_m-a-i-B6.brep`,
`bug25061_a.brep`). These belong on the SHOPPING_LIST if pursued.

---

## Gmsh

Revision `b4e84a43d02f97bc50ef37ed21e5e97096d072be` (GitHub mirror live-clones/gmsh;
the same commit as gitlab.onelab.info/gmsh/gmsh HEAD on 2026-10-17 by `git
ls-remote`; GitLab's archive endpoint redirected to a sign-in page).
CHANGELOG heads "5.0.0 (Work-in-progress)". `quarantine/sources/gmsh`.
Read: `CMakeLists.txt:2294-2382`, `.gitlab-ci.yml` (all 504 lines),
`utils/docker/Dockerfile.ci`; `src/common/GmshMessage.cpp` (`Msg::Error`,
`Msg::Exit`), `GmshGlobal.cpp`, `DefaultOptions.h`; `src/parser/Gmsh.y:360-417`;
`src/mesh/Generator.cpp:490-880`, most of `meshGFace.cpp`, `meshGFaceBDS.cpp:900-990`,
`BDS.cpp:69-80`, `meshGEdge.cpp:417-470,780-905`, `BackgroundMeshTools.cpp:1-300`;
`src/geo/OCCEdge.cpp:383-422`, `OCCFace.cpp`, `GModelIO_OCC.cpp:4970-4985,6036-6342,6420-6480`,
`GFace.cpp:2005-2040`, `GFace.h:355-366`; every `.geo/.opt` in
`benchmarks/{brep,step,iges,statreport}`, `statreport/testsuite{,3D}.sh`, key
`benchmarks/occ` and `occ_large` files, `speccpu/README.txt`, `misc/check_mesh_quality.geo`,
`plugins/run.py` + `ref.json`. STEP headers/entity histograms and BREP tolerance
statistics were extracted read-only (grep/awk and a `python3 -I` parser).

### Test inventory

- CTest (`CMakeLists.txt:2319-2382`) globs 409 files: 21 tutorials, 78 examples,
  6 `1d`, 81 `2d`, 95 `3d`, 67 `extrude`, 52 `occ`, 4 `io_msh`, 5 `io_vtk`. Each runs
  `add_test(${TEST} ./gmsh ${TEST} -3 -nopopup -o ./tmp.msh)` (`:2337`). Excluded:
  `io_vtk/vtu_roundtrip.geo` "fails on the CI machines, not understood yet"
  (`:2326`), `occ/mmg3d_multidomain_bgmesh.geo` (`:2328`). Python tutorials and
  the 93 `examples/api/*.py` are disabled: `if(0 AND ENABLE_BUILD_DYNAMIC)`
  (`:2364`). `filter_tests` silently skips OCC files when OCC is absent (`:2294-2317`).
- **What a pass means**: process exit 0. `GmshMainBatch` ends `Msg::Exit(0)`
  (`GmshGlobal.cpp:523`) -> `exit((forceLevel || level) ? level :
  _atLeastOneErrorInRun)` (`GmshMessage.cpp:405`), set by any `Msg::Error`
  (`:502`). Warnings never fail. `./tmp.msh` is never inspected.
- 121 of 409 use `SetFactory("OpenCASCADE")`; only 19 import an external CAD file
  (t20, 7 in `examples/boolean`, 11 in `benchmarks/occ`: box_structured,
  cube-sphere-cube-compound, CylinderInnerLoopCrossingSeam,
  duplicate_degenerated_edges, large_radius_cylinder,
  PeriodicFaceWithLargeTolerance, rod_distance_field, rod_multibloc, TrimmedSurface,
  unable_to_recover_edge, unable_to_recover_edge-2).
- In-script checks (benchmarks/occ): inner loop crossing a periodic seam
  (`CylinderInnerLoopCrossingSeam.geo:10`); UV tolerance on a periodic face
  (`PeriodicFaceWithLargeTolerance.geo:11`); unrecoverable boundary edges
  (`unable_to_recover_edge.geo:12`); large-radius cylinder at coarse size
  (`large_radius_cylinder.geo:12`); duplicate degenerated pole edges
  (`duplicate_degenerated_edges.geo:15`); quality floor on a trimmed surface
  (`TrimmedSurface.geo:24`); Boolean result numbering
  (`boolean_existing_results.geo:17-78`).
- **Not in CTest**: `benchmarks/{brep,step,iges,occ_large,statreport,misc,2d_large,
  3d_large,bl,speccpu,plugins}` — every hard industrial model (A319, Laufrad, screw,
  component8, bad_step_intersects, problemSphere, tough_second_order, U_Joint_2).
- CI: `linux_ci` runs `ctest -j 8 --output-on-failure` with no `--timeout`
  (`.gitlab-ci.yml:21-36`), then `valgrind --leak-check=full ... --error-exitcode=0
  ./gmsh ../tutorials/t5.geo -3` (`:30`) — **leaks can never fail the job**. Windows
  jobs `--timeout 600`. No sanitizer, fuzz or coverage job.

### Oracles

| Check | Where | Tier |
|---|---|---|
| exit 0 (no `Msg::Error`, no crash) | all CTest | 2 |
| `If(nbtriangles == 0) Error(...)` | 5 occ + 2 occ_large (unregistered) | 2-3 |
| entity counts/tags after Booleans | `occ/boolean_existing_results.geo:17-78` | 7 (small hand-derived) |
| min ICN quality >= 0.4 / 0.1 via `Plugin(AnalyseMeshQuality)` | `occ/TrimmedSurface.geo:24`, `occ/OptimizationRotatedSurface.geo:34` | 5 (element shape, not fidelity) |
| read-back counts = written | `io_msh/read_back.geo:16` | 4 |
| counts within 20% of a formula | `misc/check_mesh_quality.geo:28-31` (unregistered) | 6-7 loose |
| counts within 30% of stored, AvgQuality >= 0.75 | `speccpu/*.geo` (built-in kernel, unregistered) | 3 banded |
| statreport columns | `statreport/testsuite.sh` | 1 (appended, never compared) |

Strong lines: `If (minquality < 0.4) Error("Quality is far too low...")`
(`occ/TrimmedSurface.geo:24-25`); `If(#s() != 1) Error("BooleanIntersection
returned %g surfaces instead of 1 ...")` (`boolean_existing_results.geo:17-18`).
Tally: 391 of 409 registered tests have no assertion (tier 2 by exit code); 18 have
checks, ~8 concern CAD surface meshing, 2 above tier 3. **No test compares mesh
nodes against exact CAD geometry; no watertightness, area or volume check on OCC
models.**

Runtime internal checks (warnings/status, not test oracles): `isMeshValid`
mixed orientation vs CAD normal (`meshGFace.cpp:2306-2317`); `BDS_Face_Validity`
(`BDS.cpp:69-80`, `Mesh.CheckSurfaceNormalValidity` default 1);
`repairDuplicateTriangles` (`meshGFace.cpp:2262`); closed-loop check (`:959`).

### Corpora

- `benchmarks/step` (33 files, 6.9 MB): `bad_step_intersects.step.gz` (AP203, "Cube
  avec Tore inclu dedans", 3 MANIFOLD_SOLID_BREPs, 24 faces, 12 B-spline surfaces,
  tol 1e-5); `screw.step.gz` (EUCLID 1998, tori/cones, 3 SEAM_CURVEs, 44 PCURVEs);
  `component8.step.gz`/`component4.step.gz` (declared tolerance **0.0742**);
  `1385_Laufrad_1000.stp` (impeller, 122 faces); `tough_second_order.step.gz`
  (5.4 MB, 48,425 entities, 929 faces); `periodic_u_cst.step` (Fusion 360, periodic
  B-spline); `linkrods`, `io1-pe-203` (old Pro/E schema CCD_CLA_GVP_AST),
  `presse_asm`, `SampleCrankshaft4A`, `U_Joint_2`, `Block` (CATIA V5),
  `mesh_T106_V3` (ANSA), Onshape `part`/`wing_fuselage`, `submarine`, `wrenchnut`.
- `benchmarks/brep` (10 files, 1.1 MB): `A319.brep.gz` (83 rational B-spline faces,
  **max vertex/edge tolerance 6.77** at coordinates ~3e4); `problemSphere.brep` (80
  faces, 77 small spheres, 77 degenerated edges); `tank.brep` (tolerance 0.119, all
  healing on); `capot.brep` ("curves 44 and 47 are very small", tolerance 0.08);
  `test_11551.brep`.
- `benchmarks/occ`: `PeriodicFaceWithLargeTolerance.brep` (3D tol ~1e-7; the
  large tolerance is UV/pcurve endpoint mismatch, cf. `meshGFace.cpp:1715` "OCC
  pcurves can require a large UV tolerance"); `unable_to_recover_edge_2d-2.brep`
  (vertex tolerance 4.38); `CylinderInnerLoopCrossingSeam.brep`;
  `TrimmedSurface.brep`; `full_sphere.brep` (4 degenerated edges).
- `benchmarks/statreport` (28 files, 17 MB): `15_cylinder_head.brep` (8.4 MB, 1054
  faces, tolerance up to 0.033), `Top.brep` (324 faces, 71 tori), `Cone_1`,
  `Cylinder_1`, `Torus_1` (single face), `sphere_Rhino3D_default.igs`, CATIA
  `Block.stp`. `benchmarks/iges/{cube1000.igs, model1.igs}` need sewing at 0.1/0.01.
- Dangling references: `testsuite.sh:27-28` names files not in `statreport/`;
  `occ_large/Inductor3D.geo` merges absent `Part_2.step`.

### Pathology handling

- `Mesh.MaxRetries` 10 (`DefaultOptions.h:1430`) bounds outer re-mesh of PENDING
  faces (`Generator.cpp:800`, drops to 1 thread after iteration 2) and inner
  recursion when the 1D mesh self-intersects in UV (`meshGFace.cpp:1517,2118`);
  `remeshUnrecoveredEdges` splits intersecting segments (`:86+`).
- Periodic faces with invalid elements: <= 3 "refine all bounding edges" passes
  (`meshGFace.cpp:2161`), comment: refining "does not always help, e.g. near the
  apex of a cone ... each one doubles the number of nodes".
- `Mesh.AlgorithmSwitchOnFailure` 1: a mixed-orientation Delaunay mesh on a
  non-plane OCC face is discarded and redone with MeshAdapt (`meshGFace.cpp:2466-2477`).
- Seams/poles: both seam images and both curve orientations tried
  (`meshGFace.cpp:1621-1626`); loop closure with escalating UV tolerance `fact[6] =
  {1e-12,1e-9,1e-6,1e-3,1e-2,1e-1}` x parametric diagonal (`:1936-1950`);
  degenerated edges get 1 segment (`meshGEdge.cpp:809-812`).
- Reporting: `Msg::Error` (test fails) for "Impossible to mesh periodic surface"
  (`meshGFace.cpp:2438`), non-closed 1D loop (`:960,1955`), duplicate triangles
  (`:2263`). **Warning only (test passes)** for "Unable to recover the edge ..."
  (`:350`, face set FAILED), "%d element(s) remain invalid in surface %d"
  (`meshGFaceBDS.cpp:966-967`), "No elements in surface" (`Generator.cpp:510`). **A
  non-periodic face that fails to mesh exits 0.** The FAILED status set by
  `refineMeshBDS` is overwritten by `gf->meshStatistics.status = GFace::DONE`
  (`meshGFace.cpp:1567`, inferred from code order).
- Exceptions in parallel loops rethrown as `std::runtime_error`
  (`Generator.cpp:593-605,778-793`), uncaught in batch (inferred: terminate).
- Termination/memory: curve-length integration recursion depth 25
  (`meshGEdge.cpp:433`, up to ~2^25 points per curve, inferred); closed degenerate
  line given zero length "to avoid infinite loop in integration" (`:787-793`). **No
  time or memory limit in the 2D mesher.**
- NaN/inf: no `isnan/isinf/isfinite` in the meshing/OCC files listed;
  `BGM_MeshSize` rejects only `lc <= 0` (`BackgroundMeshTools.cpp:262-266`); a NaN
  size passes (inferred).
- Healing on import (`_healShape`, `GModelIO_OCC.cpp:6036-6341`, `Geometry.Tolerance`
  1e-8): `OCCFixDegenerated`, `OCCFixSmallEdges`, `OCCFixSmallFaces`,
  `OCCSewFaces`, `OCCMakeSolids`, all default 0; reports counts/area before and
  after, asserts nothing. `BRepTools::Clean` discards file triangulations (`:4975`).

### Fuzzing / mutation / property / performance

None found. Seeded per-face perturbation for parallel reproducibility
(`meshGFace.cpp:55-81`), untested. `speccpu/` = SPEC CPUv8 candidate workloads
(built-in kernel). `statreport/testsuite.sh` appends #faces, #fail, #triangles,
quality and 2D wall time to `statreport.dat` and never compares; columns `#e`, `tau`,
`#Egood` come from fields nothing assigns (`GFace.h:363-365`, inferred garbage).

### Tolerance semantics

Size is an edge-length target, not a deflection. `BGM_MeshSize` = min of point,
curvature, field, entity sizes, clamped to `[MeshSizeMin, MeshSizeMax]` then
multiplied by factors (`BackgroundMeshTools.cpp:211-276`) — the factor can undercut
`MeshSizeMin`; a large `MeshSizeMin` silently defeats curvature sizing. Curvature
size `lc = 2*pi / (kappa * Mesh.MeshSizeFromCurvature)` (`:72`) — elements per
2*pi, not sagitta (implied chord error ~r*pi^2/(2N^2) is our derivation; Gmsh never
computes or checks it). Curve nodes by integrating 1/lc, floored by
`MinimumCirclePoints` 7 (`OCCEdge.cpp:390-407`). OCC BRepMesh "STL"
(`Mesh.StlLinearDeflection` 0.001 relative, 0.3 rad) only for bounds/octree/drawing.
**No test verifies achieved deviation.**

### Known-bad handling

No xfail. CMake exclusion with reasons; silent dependency skip; Python tests off
wholesale; hard models outside the glob. Comments: `occ/OptimizationRotatedSurface.geo`
"WITH this rotation below seconds order mesh optimization fails";
`occ_large/spheroid_order4.geo` "ok in 4.7 ; broken in 4.8".

### Adoptable cases

See the adoption table in the synthesis; primary items:
`benchmarks/occ/{CylinderInnerLoopCrossingSeam.brep, PeriodicFaceWithLargeTolerance.brep,
unable_to_recover_edge_2d.brep, unable_to_recover_edge_2d-2.brep, cylindrical_surface.brep,
full_sphere.brep, TrimmedSurface.brep}`, `benchmarks/occ_large/SlenderSurface.brep`,
`benchmarks/brep/{problemSphere.brep, A319.brep.gz, capot.brep, tank.brep}`,
`benchmarks/step/{bad_step_intersects.step.gz, component8.step.gz, component4.step.gz,
screw.step.gz, periodic_u_cst.step, 1385_Laufrad_1000.stp, tough_second_order.step.gz,
sewfaces.step, io1-pe-203.step.gz}`, `benchmarks/iges/{cube1000.igs, model1.igs}`,
`benchmarks/statreport/{15_cylinder_head.brep, Top.brep, Cone_1.brep, Torus_1.brep,
sphere_Rhino3D_default.igs}`.

### Fault classes learned

Silent per-face failure with exit 0; unrecoverable boundary edges; self-intersecting
1D mesh in UV; seam image/orientation ambiguity; inner loop crossing the seam;
pcurve endpoint gap needing escalating UV tolerance; degenerated (possibly
duplicated) pole edges; cone-apex refinement divergence; mixed-orientation
triangles vs CAD normal; duplicate triangles; B-rep tolerances far above nominal;
tiny edges/sliver faces; unsewn shells; intersecting solids in one file; size
clamps defeating curvature sizing; unbounded refinement with no time/memory limit;
NaN size propagation; thread nondeterminism; exception escape from the parallel
mesher.

What transfers: Gmsh is an FE mesher targeting element shape and edge length; it
never bounds chordal deviation, so its quality oracles do not transfer. Its
pathological inputs and its seam/pole/UV-tolerance machinery do, as does the lesson
that "exit 0 plus non-zero element count" hides per-face failures.

---

## Netgen (NGSolve/netgen)

Revision `31e85f811fd64b163a4b4d9a1dc587115a7f3133`, `quarantine/sources/netgen`.
Read: `tests/pytest/{test_occ.py, test_occ_identifications.py, test_tutorials.py,
meshes.py, compare_results.py, conftest.py, CMakeLists.txt, results.json}` (all 37
keys parsed), `test_boundarylayer.py`, `test_savemesh.py`, `test_meshclass.py`,
parts of `test_edgedescriptor_meshing.py`; `tests/catch/*` (TEST_CASE lines;
`libm.cpp`, `simd_math.cpp` in part); `.gitlab-ci.yml`, `tests/{CMakeLists.txt,
dockerfile, build_*.sh}`; `libsrc/occ/{occgenmesh.cpp, occ_face.cpp 1-330,
occ_edge.cpp, occ_utils.cpp, occgeom.cpp 304-327/488-560/1215-1320/2320-2360,
occgeom.hpp, occmeshsurf.cpp 500-540, python_occ.cpp 401-441}`;
`libsrc/meshing/{basegeom.cpp, meshing2.cpp give-up paths, meshfunc.cpp 520-545}`;
`python/meshing.py`; `ng/ngtesting.tcl`.

### Test inventory

- pytest: 119 test functions in 21 files. `test_tutorials.py::test_geoFiles`
  expands to 211 parametrized cases (37 inputs x per-file subsets of 6
  MeshingParameters presets, `test_tutorials.py:96-116`).
- **OCC is switched off in the golden regression**: `test_tutorials.py:13-14`
  `has_occ = False # skip occ tests (current development on interface)`.
  `tutorials/frame.step` and `tutorials/screw.step` are never meshed by the
  regression suite; `results.json` has no entry for either. All 37 golden keys
  are CSG `.geo`, `.in2d` or STL.
- OCC tests that do run build shapes in code: `test_occ.py` (5 tests: analytic
  area/volume of a plate with two holes, unit square, box, cylinder, box+cylinder
  fusion `:21-43`; internal glued face `:45-54`; bent-edge splitting on a thin
  cylinder hole `:57-77`); `test_occ_identifications.py` (4 tests, CLOSESURFACES
  identification `:6-76`); OCC boxes in `test_boundarylayer.py`,
  `test_meshclass.py::test_copy_mesh`, ~20 `test_edgedescriptor_*` tests.
- Fault classes covered: sequential vs TaskManager determinism
  (`test_tutorials.py:163-170`); count/hash/quality regression (`:69-79`);
  analytic measure (`test_occ.py:6-19`); curvature safety (`test_occ.py:57-77`);
  periodic identification; boundary-layer topology; save/load; libm cross-platform
  bit reproducibility (`tests/catch/libm.cpp:83-109`).
- CI (`.gitlab-ci.yml`): `linux_mpi` builds `-DUSE_OCC=OFF`; only windows/mac jobs
  build with OCC and run ctest (pytest wrapped with `TIMEOUT 1800`,
  `tests/pytest/CMakeLists.txt:4`). `generate_results` is a manual re-baseline job.
- `ng/ngtesting.tcl` is a GUI timing driver over `.in2d/.geo/.stl`; checks nothing.

### Oracles

| Check | Tier | Where |
|---|---|---|
| element counts > 0 | 2 | `test_occ.py:11-14`, `:63` |
| golden `ne1d/ne2d/ne3d`, quality histogram, badness, min/max angle (rel 1e-10), SHA-256 of element/point arrays | 3 | `test_tutorials.py:69-79` |
| sequential vs parallel `.vol.gz` byte-identical (`filecmp.cmp`) | 4 | `test_tutorials.py:170` |
| `Integrate(1.0, mesh) == approx(volume)` on a curved order-5 mesh | 7, **skipped without ngsolve** | `test_occ.py:16-19` |
| max angular span of edges on the cylinder `assert maxturn <= 1/cs` | 5 | `test_occ.py:77` |
| prisms = 2 x layer-surface elements; no "elements are not matching" on stdout | 5 | `test_boundarylayer.py:46-49` |
| wire points subset of volume vertices | 5 | `test_edgedescriptor_meshing.py:494-498` |
| save->load->save text diff empty | 4 | `test_savemesh.py:51` |

Tally: of ~330 executed cases ~300 are tier 2-3; ~12 are tier 4-5; tier 7 only
behind `importorskip("ngsolve")` (CI installs `netgen-occt`, not ngsolve —
inferred from pip lines, so those checks likely SKIP). **No OCC/STEP test reaches
even tier 3.** `compare_results.py` is a developer diff tool (flags >±20% change in
badness/triangle count, `:80-91`), not a gate.

### Corpora

- `tutorials/` 43 entries, 1.1 MB. `tutorials/frame.step` (495,592 B, Pro/E 2002,
  375 ADVANCED_FACE: 204 cylinders, 163 planes, 8 cones; only 3 coarsest presets
  run, `test_tutorials.py:104-105`). `tutorials/screw.step` (88,552 B, Euclid 1998;
  10 faces incl. 3 toroidal, 63 B-spline curves; **coarse presets known not to
  work**, `test_tutorials.py:108-109`).
- `tests/pytest/geofiles/plane.stl` only (excluded as too slow). No BREP/IGES.

### Pathology handling

- Failed face (`occgeom.cpp:304-327`): mesh in PARAMETERSPACE; on failure retry
  once in PLANESPACE; then `facemeshstatus[nr] = -1` and
  `PrintError("Problem in Surface mesh generation")`. `SingularMatrixException`
  and `UVBoundsException` become `MESHING2_GIVEUP` (`occgenmesh.cpp:440-476`).
- **Silent holes**: `MeshSurface` counts failed faces, prints `"WARNING! NOT ALL
  FACES HAVE BEEN MESHED"` and returns void (`basegeom.cpp:1081-1086`). 3D meshing
  later throws `"Stop meshing since surface mesh not consistent"`
  (`meshfunc.cpp:532-538`), but for surface-only output `GenerateMesh` returns 0
  and the Python wrapper only throws on non-zero (`python_occ.cpp:425-430`). A
  surface mesh with missing faces can therefore return as success (inferred from
  control flow; untested).
- Termination guards: advancing front gives up at `qualclass > mp.giveuptol2d`
  (200) (`meshing2.cpp:511-517`) or meshed area > 2 x face area
  (`occgenmesh.cpp:432`, `meshing2.cpp:1414-1419`); non-empty final front = GIVEUP
  (`meshing2.cpp:1612-1613`); edge division capped at 20,000 samples ("Could not
  divide Edge", `basegeom.cpp:522-542`); `RestrictHTriangle` depth < 10
  (`occgenmesh.cpp:182`); projection Newton <= 50 iterations with NaN check and
  `det < 1e-15` -> false (`occ_face.cpp:276-327`); `SplitBentEdges` <= 8 rounds,
  rejects midpoints that intersect other triangles (`basegeom.cpp:1384-1450`).
- Seams/poles: `OCCFace::AlignGeomInfo` copies the neighbour parameter at singular
  points (|du| < 1e-8|dv|) and shifts periodic parameters by one period
  (`occ_face.cpp:35-64`); boundary sampling nudged inward 1e-10 to avoid the
  cone-tip normal (`:175-178`); an edge used three times in a face throws `"have
  edge more than twice in face"` (`:132-133`).
- Missing OCC triangulation for curvature sampling: retry ladder of BRepMesh
  deflections {0.1,0.05,0.02,0.2,0.01,0.005} x bbox diagonal, then skip with a
  warning; throws for infinite shapes (`occgenmesh.cpp:643-675`). Close-edge size
  floor 1e-3 x diagonal (`:826-832`).
- None of these paths is tested. No NaN, memory or hang tests; only the 1800 s
  CTest timeout.

### Fuzzing / mutation / property / performance

None found (searched `tests/`, CI, CMake, `cmake/` for fuzz/sanitize/asan/valgrind).
17 inputs marked `slow`, skipped unless `--runslow`/`RUN_SLOW_TESTS`; likely never
run in CI (inferred). No performance gate; `print("needed", ...)` timing is not
asserted.

### Tolerance semantics

No chordal deflection parameter. Size is curvature-driven:
`ComputeH(kappa) = max(minh, maxh*kappa*cs < 1 ? maxh : 1/(kappa*cs))`
(`occgenmesh.cpp:61-79`); edge curvature sampled at 20 points per edge
(`:596-625`); face curvature sampled at the vertices/centroid of an OCC
triangulation (relative deflection 0.01, angle 0.5, `occ_utils.hpp:117-118`),
skipped where curvature < 1e-3. `SplitBentEdges` splits interior edges whose
normal turn >= `BentEdgeAngle = min(max(1/cs, pi/3), pi/2)` (`occgeom.hpp:297-301`).
Only `test_occ.py:57-77` measures an achieved geometric quantity (angular span on
an exact cylinder). **No test measures distance from mesh to exact surface.**

### Known-bad handling

`has_occ = False` disables all STEP regression; per-file preset exclusions in
`getMeshingparameters` (`test_tutorials.py:98-115`: `cylinder.geo` "gives
inconsistent reults", `cylsphere.geo` coarse "other mesh on MacOS", `screw.step`
coarse "don't work", `part1.stl` very_coarse "does not work"); `pytest.mark.slow`;
`_test_with_inner_corner` disabled by underscore, "not working yet"
(`test_boundarylayer.py:131-132`); Catch `[!mayfail]` on libm fingerprints.

### Adoptable cases

- `tutorials/screw.step` — helical B-spline trims on torus/cone; known coarse-size
  failure: bait for give-up when tolerance is coarse relative to thread pitch.
- `tutorials/frame.step` — 375-face Pro/E part, many small fillets/holes.
- `test_occ.py:57-63` — box(-1..1) minus cylinder r=0.02 vs maxh 0.5: tiny hole
  beside a large face; curvature/deflection property check.
- `test_occ.py:24-26` — 7x4 plate with two reversed-circle holes, analytic area
  28-2pi.
- `test_occ.py:47-51` — box with an internal glued face (non-manifold).
- `test_edgedescriptor_meshing.py:455-466` — free wire embedded in a box.

### Fault classes learned

Parameter-space give-up needing a plane-space fallback; silent missing faces
returned as success; singular Jacobian / UV-out-of-bounds during projection at
poles/apexes; pole parameter ambiguity; periodic seam wrap by one period; edge
used >2x in a face; large vertex tolerance pushing projections outside the UV box;
missing auxiliary triangulation; infinite shapes; close-edge size collapse; edge
division non-convergence; bent edges bridging curvature (post-split with
self-intersection guard); libm last-bit nondeterminism changing meshes; thread
nondeterminism; coarse-size failure on helical geometry; platform-dependent
topology.

---

## SALOME SMESH

Revision `83f8bf48211b922fb47623be7a74746a87f97d74` (GitHub mirror
SalomePlatform/smesh; git.salome-platform.org failed TLS), `quarantine/sources/smesh`.
Read: `test/{0README, tests.set, CTestTestfileInstall.cmake, CMakeLists.txt,
test_helper.py}` plus a census of assert/raise/Compute/Nb*() over all 109 `.py`;
fully or in part `netgen_runner_2D.py`, `SMESH_mechanic.py`,
`split_overconstrained_box.py`, `SMESH_beta_law.py`, `test_polyhedron_per_solid.py`,
`test_import_1D2D_with_tetras_and_pyras.py`, `body_fitting_viscous_layer_cylinder.py`,
`blocFissure_01_without_session.py`, `SMESH_flight_skin.py`, `SMESH_Nut.py`,
`SMESH_test5.py`; `src/StdMeshers.test/{tests.set, HexahedronTest.cxx,
HexahedronCanonicalShapesTest.cxx}`; `doc/examples/{tests.set,
quality_controls_defl.py, defining_hypotheses_adaptive1d.py}`;
`src/SMESHUtils/SMESH_ComputeError.hxx`, `SMESH_TryCatch.*`,
`src/SMESH/SMESH_subMesh.cxx` (1505-1720, 1993-2065), `SMESH_Gen.cxx`,
`src/SMESH_SWIG/smeshBuilder.py` (1944-2063), StdMeshers error sites,
`src/Controls/SMESH_Controls.cxx` (Deflection2D 2149-2210), `src/SMDS/SMDS_Mesh.cxx:72-119`.

### Test inventory

- `test/tests.set`: GOOD 41, BAD 40, SESSION_FREE 6, WITH_MG 9+1, WITH_SHAPER 9,
  UNSTABLE 1, CPP 1 (`SMESH_RegularGridTest`), UNIT 2 (Hexahedron tests).
  "BAD" is **plug-in gating, not xfail** (`tests.set:26-28`); GOOD+BAD register
  with a 600 s `TIMEOUT` (`CTestTestfileInstall.cmake.in:26`).
- `doc/examples/tests.set`: 51 GOOD + 113 BAD (+2 HOMARD) + 1 UNSTABLE; only 14 of
  171 example scripts contain `assert`.
- Scenarios: block/hexa decomposition (`ex01`-`ex24`), algorithm switching,
  distribution laws, polyhedra per solid, 1D/2D import, body-fitted Cartesian +
  viscous layers, dual mesh, mesh I/O round trips, quality functors, crack
  insertion (`blocFissure_*`, 32 case scripts). None target B-rep pathology.
- `HexahedronTest.cxx:108-191`: 2 BREP files, 10 trials per thread count 1..N,
  asserts `nbAdded == 1024` / `== 35150`; written "to detect and solve segfault"
  (`:106`) — a threading race regression.
- **No test exercises a failing compute or the error API**: grep for
  `GetComputeErrors|GetAlgoState|GetFailedShape|COMPERR|ComputeError` over `test/`
  and `doc/examples/` returns nothing; no test asserts Compute returns False.

### Oracles

| Check | Tier | Where |
|---|---|---|
| `isDone = mesh.Compute(); if not isDone: raise` then print counts | 2 | ~50 of 109 `test/*.py`, 68 doc examples |
| `CheckCompute()` -> `IsComputedOK()` | 2 | 22 uses |
| exact element counts | 3 | `SMESH_beta_law.py:76-77` (`NbQuadrangles() == 551`), `netgen_runner_2D.py:174-176`, Hexahedron tests |
| counts vs reference, ±5% on nodes/tris/tets | 3 | `src/Tools/blocFissure/gmu/getStatsMaillageFissure.py:53-77` |
| mesh volume vs OCC exact solid volume | ~7 | `test_polyhedron_per_solid.py:116` (`< 1e-12`, planar); `body_fitting_viscous_layer_cylinder.py:79,87` (`< 1e-2` vs pi r^2 h) |
| dual surface group area vs geometry, `< 0.015` relative | ~7 | `SMESH_create_dual_mesh_tpipe.py:156` |
| zero over-constrained volumes; aspect ratio < 200; min pyramid volume > 1e-10 | 5 | `split_overconstrained_box.py:75`, `test_import_1D2D...:177-183` |
| algorithm switch leaves face count unchanged | 4 | `SMESH_algo_switch_*.py:66` |

Tally: ~85 of 107 registered scripts tier 1-3 (~60 "Compute OK + print", ~25
counts); ~12 compare volume/area to exact geometry at 1e-12 (planar, exact by
construction) to 1.5-5% (curved); ~8 tier 4-5. **Nothing measures surface
deviation**: `doc/examples/quality_controls_defl.py` computes `FT_Deflection2D` and
only prints it.

### Corpora

In-repo B-rep inputs total 10 KB: `test/data/HexahedronTest/NRTM1.brep` (4,327 B),
`NRTMJ4.brep` (6,116 B). Everything else is built in-script from GEOM/SHAPER
primitives and Booleans. External `DATA_DIR/Shapes/Brep/{flight_solid,slots}.brep`
and `DATA_DIR/MedFiles` are referenced (`SMESH_flight_skin.py:45-51`,
`SMESH_Nut.py:102-105`) but not reachable from here.

### Pathology handling

- Error model (`SMESH_ComputeError.hxx:42-65`): `COMPERR_BAD_INPUT_MESH` -2,
  `STD_EXCEPTION` -3, `OCC_EXCEPTION` -4, `EXCEPTION` -6, `MEMORY_PB` -7,
  `ALGO_FAILED` -8, `BAD_SHAPE` -9, `WARNING` -10 ("sub-mesh is computed anyway"),
  `CANCELED` -11, `NO_MESH_ON_SHAPE` -12, `BAD_PARMETERS` -13. `Worst()` keeps the
  most severe (`SMESH_TryCatch.cxx:79-98`).
- Per-sub-shape compute under `OCC_CATCH_SIGNALS` with a 2 MB `MemoryReserve`;
  `bad_alloc`/`Standard_OutOfMemory` -> `MEMORY_PB`; logs `"<algo> failed on
  sub-shape #<id> with error <name>"` (`SMESH_subMesh.cxx:1584-1631, 2029-2060`).
  Degenerate edges with no mesh count as OK (`:2034-2036`).
- Python `Compute()` catches everything, prints errors and returns a bool; partial
  meshes stay in place (`smeshBuilder.py:1944-2056`).
- Algorithm refusals: Quadrangle `"Face must have 4 sides but not N"` (BAD_SHAPE),
  `"Too few boundary nodes"`; explicit guard `"Bug: infinite loop in
  StdMeshers_Quadrangle_2D::CheckNbEdges()"` after >8 loops
  (`StdMeshers_Quadrangle_2D.cxx:1729-1733`); Regular_1D `"Too small common ratio
  causes too many segments"` at eltSize < 1e-100 (`StdMeshers_Regular_1D.cxx:1206`);
  deflection mode returns false silently when `GCPnts_UniformDeflection` fails
  (`:1326-1328`).
- **Memory guard is dead code**: `SMDS_Mesh::CheckMemory` begins `return -1;`
  (`SMDS_Mesh.cxx:72-74`).
- Cancellation checked only between sub-meshes; the only hang guard is the 600 s
  CTest timeout. No in-tree B-rep surface triangulator at this revision (MEFISTO
  absent; NETGEN_2D/MG_CADSurf/GMSH are external plug-ins).

### Fuzzing / mutation / property / performance

None found. `HexahedronTest.cxx` thread sweep is the nearest stress test.

### Tolerance semantics

No global chordal tolerance. `Deflection1D` -> `GCPnts_UniformDeflection`;
`Adaptive1D` deflection estimate samples only 4 interior points per segment
(`StdMeshers_Adaptive1D.cxx:898-912`). `FT_Deflection2D` measures only element
**centroid** distance to the surface (`SMESH_Controls.cxx:2171-2208`) — a
non-maximum metric that underestimates chordal error.

### Known-bad handling

`UNSTABLE` CTest label; BAD = plug-in gating; blocFissure writes a `.new`
reference on mismatch for a human to paste in (`getStatsMaillageFissure.py:83-91`);
`test_vlapi_growthlayer.py:71,89` counts coupled to Netgen defaults ("if Fails!
change the default value").

### Adoptable cases

- `test/data/HexahedronTest/NRTM1.brep`, `NRTMJ4.brep` — small real BREPs; thread
  count determinism.
- `HexahedronCanonicalShapesTest.cxx:148-232` — box, cylinder, sphere r30, cone to
  apex, truncated cone: canonical poles/apexes/seams with analytic area/volume.
- `doc/examples/quality_controls_defl.py:15-17` — sphere r100 fused with a 200^3
  box: curved/planar Boolean seams.
- `doc/examples/defining_hypotheses_adaptive1d.py:15-23` — box cut by a rotated
  box, thin cylinder r2, 1 mm slab: slivers and small features.

### Fault classes learned

Wrong side count / multiple wires for an algorithm (explicit refusal); too few
boundary nodes; pre-computed sub-mesh conflicts; degenerate pole edges that must
not count as failure; kernel exceptions mapped to codes; out-of-memory with a
reserve; runaway segment counts from geometric progressions; infinite loop in side
detection; thread-dependent results/segfaults; partial success hidden behind a
bool API; centroid-only deviation metrics; regression counts coupled to an
external mesher's defaults.

---

## FreeCAD

Revision `cc8a5eb0dc813ab2b89479c72a9ee2159d31a475`, `quarantine/sources/freecad`
(submodules — OndselSolver, GSL, AddonManager, coin, pivy — are absent from the
GitHub ZIP and irrelevant here).
Read in full: `tests/src/Mod/MeshPart/App/MeshPart.cpp`,
`tests/src/Mod/Part/App/BRepMesh.cpp`, `tests/src/Mod/Mesh/App/{Mesh,Exporter,
Importer,MeshFeature}.cpp`, `tests/src/Mod/Mesh/App/Core/{MeshIO,KDTree}.cpp`,
`src/Mod/Mesh/App/MeshTestsApp.py`, `src/Mod/Mesh/MeshTestsGui.py`. In part:
`src/Mod/Part/TestPartPreview.py`, `tests/src/Mod/Part/App/TopoShapeExpansion.cpp:1620-1660`,
`src/Mod/OpenSCAD/OpenSCADTest/app/test_importCSG.py:320-345`,
`src/Mod/CAM/CAMTests/TestPathOpUtil.py:610-640`. Source: `src/Mod/Part/App/BRepMesh.cpp`,
`TopoShape.cpp` (210-232, 1005-1075, 3595-3700), `Tools.cpp` (250-330, 685-740,
880-907), `TopoShapePyImp.cpp`, `src/Mod/Part/Gui/ViewProviderExt.cpp`,
`src/Mod/MeshPart/App/Mesher.cpp` (120-420), `AppMeshPartPy.cpp`,
`src/Mod/Mesh/App/Core/{Evaluation,Degeneration}.cpp`, `Elements.cpp:639-701`;
CI `.github/workflows/sub_buildUbuntu.yml:160-240`, `cMake/FreeCAD_Helpers/SetupSanitizers.cmake`.

### Test inventory

- SMESH/Mefisto meshing of a 10^3 box: `tests/src/Mod/MeshPart/App/MeshPart.cpp:34`,
  `:80` (call SMESH directly, bypass `MeshPart::Mesher`).
- Per-face domain merge, 4 gtests: `tests/src/Mod/Part/App/BRepMesh.cpp:133/144/155/166`
  (no domains; empty domains; two quads sharing an exact edge; two quads whose
  shared corners differ by 1e-10, `:83-88`). Inputs are hand-built triangles.
- Mesh export placement (`tests/src/Mod/Mesh/App/Exporter.cpp:78,101,127`) uses mesh
  primitives, so the B-rep tessellation branch (`src/Mod/Mesh/App/Exporter.cpp:119`)
  is never reached.
- Mesh kernel: import readers; grid on an almost-planar mesh z=1e-18
  (`tests/src/Mod/Mesh/App/Mesh.cpp:78,98`); 8 topology-after-split tests
  (`MeshTestsApp.py:99-197`); corrupted facet repair (`:54-89`); 11 tri-tri /
  edge-edge intersection predicate tests including near-coincident float pairs
  (`:273-436`); one self-intersection test (`:438-479`).
- **Zero tests** of `MeshPart.meshFromShape`, `Mesher::createStandard` (the OCC
  path), the Netgen path, `TopoShape::exportStl`, `TopoShape::getFaces` or
  `getFacesFromSubElement` on a real shape (grepped `tests/` and every
  unittest-bearing `src/Mod/**` Python file). No Part test of tessellation.

### Oracles

- Tier 1 (5): `testCollapseFacets*` (no assertion, `MeshTestsApp.py:40-51`);
  `testSphereMesh`/`testLoadMesh` start threads, never join, never assert (`:504-517`).
- Tier 2: `EXPECT_EQ(success, true)` (`MeshPart.cpp:66,107`).
- Tier 3: `EXPECT_EQ(mesh->NbNodes(), 1244); EXPECT_EQ(mesh->NbTriangles(), 2484);`
  (`MeshPart.cpp:68-69`); 3MF counts.
- Tier 5: the split tests assert `hasNonManifolds()`, `hasInvalidNeighbourhood()`,
  `hasPointsOutOfRange()`, `hasFacetsOutOfRange()`, `hasCorruptedFacets()` false and
  `isSolid()` true (`MeshTestsApp.py:104-109`) — on a 12-triangle box, never on
  tessellator output.
- Tier 4: placement metamorphic ray tests (`:213-224`, `:249-265`).
- Tier 7: intersection results, exporter bounding boxes, BRepMesh merge counts
  (`BRepMesh.cpp:173-174`).
Tally: of ~70 mesh-relevant tests ~55-60 are tier 1-3; tier 4-7 checks target the
mesh kernel or merge helper, **never B-rep tessellation quality**.

**Mesh validity evaluators** (exist in code, mostly unused by tests; worth porting
with blind spots fixed):
- `MeshEvalOrientation::Evaluate` (`src/Mod/Mesh/App/Core/Evaluation.cpp:139-160`):
  shared directed edge traversed the same way = wrong orientation.
- `MeshEvalSolid` (`:294-305`): no border edge (closedness only).
- `MeshEvalTopology` (`:340+`): edge shared by >2 facets.
- `MeshEvalPointManifolds` (`:476-500`): heuristic `adjacentPoints > sharedFacets+1`;
  misses two closed fans touching at one vertex (inferred).
- `MeshEvalSelfIntersection` (`:603-677`): grid + float boxes; **skips pairs that
  share any vertex** (`:641-659`), so folds between adjacent triangles are
  invisible; stops at the first hit.
- `MeshEvalDegeneratedFacets` / `IsDegenerated(eps)` (`Degeneration.cpp:473-483`,
  `Elements.cpp:639-680`); `MeshEvalDeformedFacets` (angle bounds);
  `MeshEvalFoldsOnSurface` (`:928-958`); `MeshEvalFoldOversOnSurface` (`:1007-1035`,
  neighbour normals >120 degrees apart).
- `MeshEvalNaNPoints` (`:228-237`) checks `isnan` only: **±inf passes**.
Users: only the GUI dialog `DlgEvaluateMeshImp.cpp`, `Writer3MF.cpp:157`, `MeshIO.cpp:991`.

### Corpora

`data/tests/mesh.3mf` (12,279 B), `mesh.obj`, four NASTRAN `.bdf`;
`tests/src/Mod/Part/App/brepfiles/{cylinder1.brep (11,141 B), helix1.brep (42,600 B),
unorientableSolid.brep (23,355 B)}` used by Boolean tests
(`TopoShapeExpansion.cpp:1631-1649`: BRepCheck rejects the unorientable solid, yet
the fuse must succeed with volume 2 x 87432.002). No tessellation test uses them.

### Pathology handling

CI "C++ tests" `timeout-minutes: 1`, CLI Python tests 10 minutes
(`sub_buildUbuntu.yml:168,208`); sanitizers default OFF
(`SetupSanitizers.cmake:37-42`). No NaN/inf/huge-coordinate/zero-deflection tests.
Untested source guards: deflection below `gp::Resolution()` clamped to
`Precision::Confusion()` "Since OCCT 7.6 a value of equal 0 is not allowed"
(`ViewProviderExt.cpp:1086-1090`); upper clamp for huge objects commented out
(`:1092-1095`); infinite faces bounded by `selectRange` and re-meshed at fixed
0.005/0.1 rad (`Tools.cpp:685-698`); `getDomains` emits an **empty domain for an
unmeshable face "so face/domain counts match"** with no error (`TopoShape.cpp:3612-3619`).

### Fuzzing / property / performance

None found (searched for fuzz|hypothesis|benchmark|perf). `tests/visual/baselines/
coin-nodes/*.png` are render snapshots of Coin nodes, not tessellation checks.

### Tolerance semantics

- Viewer deflection `((dx)+(dy)+(dz))/300 * Deviation` (`Tools::getDeflection`,
  `Tools.cpp:895-902`; Deviation 0.01-100, default 0.2), passed as absolute with
  `AllowQualityDecrease = True` (`ViewProviderExt.cpp:1100-1106`).
- `defaultAngularDeflection(lin) = min(0.1, 5*lin + 0.005)` — comment: "a
  work-around for unreliable linear tolerance enforcement in OCC, especially on
  nurbs surfaces" (`TopoShape.cpp:218-232`); used by `exportStl`, `getFaces`.
- `Part.Shape.tessellate(tol, reset=False)` reuses an existing triangulation unless
  `reset` (`TopoShapePyImp.cpp:1724-1745`) — tessellation hysteresis.
- `MeshPart.meshFromShape`: `LinearDeflection`+`AngularDeflection`+`Relative` go
  to `BRepMesh_IncrementalMesh` after `BRepTools::Clean` (`Mesher.cpp:227-239`); the
  keyword `Deflection` alone routes to **Mefisto/SMESH `StdMeshers_Deflection1D`**
  (`AppMeshPartPy.cpp:543-551`) — same word, different semantics. No validation of
  zero/negative values.
- Output narrowed to float (`Mesher.cpp:157-160`).
- Merge (`src/Mod/Part/App/BRepMesh.cpp:123-173,292`): `vertexLess` with
  `fabs(...) >= tol` is not a strict weak ordering (std::sort precondition,
  inferred); facets degenerate after the merge are not removed.
- No test compares achieved deviation with exact geometry.

### Known-bad handling

No skip/xfail/DISABLED_ in these tests; SMESH/OCCT version `#if` gating;
`MergeExporter::write` swallows `Base::Exception` to `std::cerr` from a destructor.

### Adoptable cases

- `tests/src/Mod/Part/App/brepfiles/unorientableSolid.brep` — BRepCheck-invalid
  solid real documents depend on: mesh it or refuse explicitly.
- `tests/src/Mod/Part/App/brepfiles/helix1.brep`, `cylinder1.brep` — helical sweep
  tangent to a cylinder; long thin curved faces.
- `tests/src/Mod/Part/App/BRepMesh.cpp:81-130` — shared corners differing by 1e-10:
  cross-face weld tolerance.
- `src/Mod/Mesh/App/MeshTestsApp.py:286-298`, `:340-351`, `:439-475`, `:54-89` —
  near-coincident float triangle pairs, coplanar overlap STL, repeated-vertex
  facet: inputs for our offline self-intersection/validity oracle.
- `tests/src/Mod/Mesh/App/Mesh.cpp:72-110` — zero-extent bounding boxes.

### Fault classes learned

Same-name tolerance with different semantics; bbox-relative absolute deflection
(one far point degrades every face); linear tolerance not enforced on NURBS
(angular cap workaround); stale triangulation reuse; bbox depends on triangulation
presence; zero/sub-resolution deflection; infinite faces; **unmeshable face becomes
a silent empty domain**; double-to-float narrowing; non-transitive weld comparator
and post-weld degenerates; orientation flip on reversed faces
(`Tools.cpp:319-321`, untested); BRepCheck-invalid but usable shapes;
self-intersection checker blind to adjacent folds; NaN checked but not inf;
exporter errors swallowed.

---

## CadQuery

Revision `5f5bc62b255f62e3512105740025361b6c31eddd`, `quarantine/sources/cadquery`.
Read: `tests/test_cadquery.py:5740-5870`, `tests/test_exporters.py` (51-53, 520-660,
750-870, 1030-1070), `tests/test_assembly.py` (86-110, 217-240, 1405-1580),
`tests/test_shapes.py:240-256`, `tests/conftest.py`, `pytest.ini`,
`cadquery/occ_impl/shapes.py` (490-535, 1600-1760), `exporters/__init__.py:36-135`,
`exporters/threemf.py:20-130`, `cadquery/assembly.py:548-625`, `changes.md`.

### Test inventory and oracles

~35 test items touch tessellation, all tier 1-3:
- `test_cadquery.py:5844`: 1x1 plane -> 4 vertices, 2 triangles; 1e-9 x 1e-9 plane
  "should not crash, but return no verts": `assert len(verts) == 0` (`:5853-5855`) —
  **silent empty output asserted as correct**.
- `test_exporters.py:850`: box -> 24 vertices / 12 triangles, which locks in
  unwelded per-face output (a watertight indexed mesh would fail it).
- STL ASCII/binary header and substring checks (`:762-821`); assembly STL file
  exists (`test_assembly.py:1440-1480`); glTF stdout/stderr empty for a shape
  instanced twice (`:1565-1576`, guarding "RWGltf_CafWriter skipped node ... without
  triangulation data").
No tier 4-7 tessellation check. No fuzz/property/benchmark/timeout (grepped).

### Tolerance semantics (source)

- `Shape.mesh` (`shapes.py:1606-1612`): `BRepMesh_IncrementalMesh(self.wrapped,
  tolerance, True, angularTolerance)` — **isRelative=True** for tessellate, 3MF, AMF,
  TJS, VRML and assembly meshing; the reuse check
  `BRepTools.Triangulation_s(self.wrapped, tolerance)` compares a stored absolute
  deflection against a relative request (mismatch inferred).
- `exportStl` default `relative=True` (`:493-523`), while `exporters.export`
  documents `tolerance` as "the deflection tolerance, in model units"
  (`exporters/__init__.py:53`).
- `toVtkPolyData`: tolerance becomes `SetDeviationCoefficient`; `if tolerance:`
  treats 0 as unset (`:1704-1726`).
- `ThreeMFWriter` silently drops shapes that did not tessellate (`threemf.py:44-46`).
- Reversed faces get (1,3,2) winding (`shapes.py:1641-1658`); `changes.md:407`
  "Tessellation winding fix #420", `:330` "indexing bug in tessellation #1163" —
  no orientation or index-range assertion in tests.

### Adoptable cases

`Face.makePlane(1e-9, 1e-9)` (sub-tolerance face: we refuse explicitly);
`boxes2_assy` (`test_assembly.py:217-225`, one shape instanced twice);
`nested_assy_sphere` (`:86-98`, nested locations + spheres).

### Fault classes learned

Relative tolerance documented as absolute; deviation coefficient vs chordal
deflection; silent drop of untessellatable shapes; empty mesh accepted for tiny
faces; instanced shapes missing triangulation; unwelded output locked in by tests;
winding/indexing regressions on reversed faces with no regression oracle;
falsy-zero tolerance.

---

## build123d

Revision `f5cca68a7500ec452759a463010aced0cbbd278f`, `quarantine/sources/build123d`.
Read: `tests/test_mesher.py`, `tests/test_brep_from_stl.py:1-212`,
`tests/test_benchmarks.py`, `tests/test_exporters3d.py:395-700`,
`tests/test_direct_api/{test_import_export.py, test_vtk_poly_data.py, test_shape.py,
test_bound_box.py, test_empty_shapes.py, test_face.py}` (cited ranges),
`src/build123d/mesher.py:100-520`, `topology/shape_core.py` (1926-1945, 2434-2560),
`exporters3d.py`, `vtk_tools.py`, `geometry.py:104`, `.github/workflows/{benchmark,test}.yml`.

### Test inventory and oracles

~60 items touch meshing; ~45 tier 1-3, ~15 tier 4-6.
- `Mesher` (3MF/STL via lib3mf): counts (box 8 vertices/12 triangles after weld,
  `test_mesher.py:69-79`), unit/metadata/colour round trips, invalid extensions.
- `TestDuplicateVertices` (`:264-273`): no assertion ("dual vertices raise
  exception here").
- `TestHollowImport` (`:276-283`): `Cylinder(5,10) - Box(5,5,5)` -> STL -> reload,
  `assertAlmostEqual(test_shape.volume, stl[0].volume, 0)` (~±0.5 on ~660) — tier 6.
- `test_face.py:578-587`: torus(10,1) -> STL -> import,
  `assertAlmostEqual(imported_torus.area, torus.area, 0)` — tier 6, loose.
- `TestImportDegenerateTriangles` (`:286-297`): `tests/cyl_w_rect_hole.stl` must
  import as a valid manifold Solid with no zero-area faces.
- `TestInvalidMesh` (`:329-337`): mocked `IsValid()` False -> `RuntimeError("3mf
  mesh is invalid")`.
- `tessellate_with_uvs` (13 tests, `test_exporters3d.py:501-646`): mocked missing
  triangulation, index range `assertLess(max(i0,i1,i2), n)` (`:615-617`), UV ranges.
- Metamorphic: sphere bbox (6,6,6) unchanged by `s.mesh(1e-3)`
  (`test_bound_box.py:263-267`).
- Empty shape export refused with "nothing to export" (`test_empty_shapes.py:505-521`).
- `test_brep_from_stl.py:153-210`: 4 mesh -> primitive-recovery tests (area within
  0.1, bbox within 1e-2).
- Benchmarks: `pytest-benchmark --benchmark-autosave` then `compare --csv`
  (`.github/workflows/benchmark.yml:28-31`); artefacts uploaded, **no failure
  threshold**; normal job `--benchmark-disable`.

### Corpora

`tests/cyl_w_rect_hole.stl`: 39,084 B binary STL, 80 zero-byte header, 780
triangles of which **46 are zero-area collinear** (counted by our byte parse); no
provenance.

### Pathology handling

Degenerate imported triangles; empty shapes; mocked failures; sub-tolerance shape
skipped with `warnings.warn("Degenerate shape ... - skipped")`
(`mesher.py:425-430`, untested); non-manifold 3MF result only **warns**
(`mesher.py:458-459`). No timeout/memory/NaN/huge-coordinate tests.

### Tolerance semantics (source)

Every meshing path hardcodes `isRelative=True` (`mesher.py:278-284`, default
`linear_deflection = 0.001`; `shape_core.py:1939-1942`; `export_stl`
`exporters3d.py:527-530`, whose docstring describes an absolute distance and which
calls `Perform()` twice). Vertex weld rounds to `-log10(TOLERANCE)` = 6 decimals
(`mesher.py:315-352`, `TOLERANCE = 1e-6`): a rounding-grid weld that splits points
1e-9 apart across a cell boundary and merges points ~1e-6 apart; collapsed
triangles dropped; positions written as float32. `_mesh_shape` and
`Shape.tessellate` dereference the triangulation without a None check
(`mesher.py:291-296`; `shape_core.py:2452-2463`). No deviation test.

### Adoptable cases

- `tests/cyl_w_rect_hole.stl` — 46 zero-area slivers: weld/validator input.
- `Cylinder(5,10) - Box(5,5,5)` — inner pocket; volume oracle (tighten).
- `Solid.make_torus(10, 1)` — thin torus; analytic area 4 pi^2 R r, volume
  2 pi^2 R r^2 (tier 7).
- `fillet(Box(1,1,1).edges(), 0.1)` (`test_brep_from_stl.py:185`) — 6 planes, 12
  cylinders, 8 sphere corners.
- `split(Cylinder(1,2)+Sphere(1), Plane.XY.offset(1).rotated((0,30,0)), Keep.BOTTOM)`
  (`:155-159`) — oblique cut through cylinder/sphere tangency.
- `test_mesher.py:266-270` BuildPart 45-degree rotated rectangle extrusion —
  historical duplicate-vertex case.
- `Solid.sweep(... Transition.ROUND)` (`test_shape.py:563-570`, issue #855) —
  self-folding sweep; explicit refusal.

### Fault classes learned

Relative tolerance documented as absolute; rounding-grid weld; silent post-weld
triangle drop; float32 narrowing; missing-triangulation dereference; non-manifold
output with only a warning; degenerate input triangles; self-folding sweeps; bbox
must not change when triangulated; redundant double meshing; benchmarks without
thresholds.

**Across FreeCAD/CadQuery/build123d:** none checks pointwise deviation against
exact geometry, watertightness, orientation or self-intersection of tessellator
output. Worth carrying: FreeCAD's evaluator definitions with their blind spots
fixed, and build123d's mesh-vs-B-rep volume/area pattern made strict.

---

## truck (ricosjp/truck)

Revision `88ed005249e5e3a6b07f62425399435905cd3ab6`, `quarantine/sources/truck`.
Its test data is the git submodule `resources` -> `ricosjp/resources4truck`, empty
in the GitHub ZIP. This lane acquired the submodule at the commit truck pins
(`9bf9de1426c5fc2f2b8d63f501dca0d3c53ebd91`, read from the GitHub contents API for
truck at the pinned revision) into `quarantine/sources/resources4truck` (36 files;
see Appendix A). Read: `.gitmodules`, `Makefile.toml`, `.gitlab-ci.yml`, `CHANGELOG.md`;
`truck-meshalgo/src/tessellation/{mod.rs,triangulation.rs}`,
`src/analyzers/{topology.rs,in_out_judge.rs,collision.rs,point_cloud/mod.rs}`,
`src/filters/optimizing.rs` (excerpts), `tests/{tessellation,analyzers,filters}/*`;
`truck-polymesh/src/{errors.rs,polygon_mesh.rs}`; `truck-geotrait/src/lib.rs`,
`src/algo/{curve.rs,surface.rs}`, `tests/{curve.rs,surface.rs}`;
`truck-geometry/src/specifieds/{sphere,circle,torus}.rs`, NURBS excerpts and tests;
`truck-stepio/tests/input/tessellate_shape.rs`, `tests/io/ioi.rs`;
`truck-shapeops/tests/fillet.rs`, `src/transversal/integrate/tests.rs`;
`truck-base/src/tolerance.rs`.

### Test inventory

truck-meshalgo: 36 integration tests (tessellation 8, analyzers 12, filters 9,
vtk 7), 7 in-source, 2 asserting doctests; stepio 2, shapeops ~3 checking + ~3
assertion-free tessellations. Fault classes:
- Closed solid -> closed mesh after welding: `truck-meshalgo/tests/tessellation/triangulation.rs:21-34`
  (`solid_is_closed`: bottle, punched-cube, torus-punched-cube, sphere, torus),
  `:37-50` (CompressedSolid).
- Seam/pole on a cylinder split into two revolved half-faces: `:81-135` (`special_cylinder*`).
- **Edge curves not on the surface**: `:138-167` (`robust_closed`) bows each cube
  edge into a quadratic B-spline, asserts plain `triangulation` refuses every face
  (`face.surface().is_none()`), then `robust_triangulation` + weld gives `Closed`.
- Agreement with an OCCT mesh: `:53-72` (`compare_occt_mesh`) at tol 0.005.
- STEP -> mesh closure on external data: `truck-stepio/tests/input/tessellate_shape.rs:19-35`
  over 5 OCCT-written STEP files and ABC `abc-0000/0006/0008/0035`; STEP write ->
  reread -> closure: `truck-stepio/tests/io/ioi.rs:17-48`.
- Fillet surfaces closure: `truck-shapeops/tests/fillet.rs:405-410`.
- Mesh volume/centroid under random rigid motion (property test, planar prisms):
  `truck-meshalgo/tests/analyzers/volume.rs:15-46`.
- Tolerance-driven subdivision primitives: `truck-geotrait/tests/curve.rs:62-82`,
  `tests/surface.rs:156-205`, `truck-geometry/tests/{bspcurve.rs:171-195,
  nurbscurve.rs:100-125, circle.rs:90-97}`.

### Oracles

- Tier 1: `large_number_meshing` (`triangulation.rs:75-79`), fillet
  `create_fillet*`, `integrate/tests.rs::punched_cube`.
- **Tier 5, the main tessellation oracle** (closure after weld):
  `poly.put_together_same_attrs(TOLERANCE * 2.0).remove_degenerate_faces().remove_unused_attrs();
  assert_eq!(poly.shell_condition(), ShellCondition::Closed, ...)`. `Closed`
  (`src/analyzers/topology.rs:35-63`) = every undirected edge used exactly twice in
  opposite directions. It holds only after welding within 2e-6 (meshalgo), 5e-5
  (stepio) or 1e-4 (fillet); the doctest at `src/tessellation/mod.rs:159-165`
  asserts raw output is **not** closed. Watertight after weld, not by construction.
- Tier 6: `assert!(res.is_clung_to_by(ans.positions(), 0.05)); assert!(ans.is_clung_to_by(res.positions(), 0.05));`
  — faces within 0.05 of the other mesh's vertex cloud (`point_cloud/mod.rs:36-52`),
  10x slack over the meshing tolerance, mesh-to-mesh, not against exact surfaces.
- Tier 4+7: `prop_assert_near!(msolid.volume(), volume)` under random rigid motion
  — planar faces only, so tessellation tolerance is never exercised.
- Tier 5 sampled deviation on subdivision primitives only: `truck-geotrait/tests/curve.rs:79`
  `prop_assert!(res.distance(ans) < 0.1)` at tol 0.05; `tests/surface.rs:193,204`
  counts failures and `assert!(count > 8, ...)` over 10 random surfaces (one in
  ten may fail).
Tally (~17 tests that tessellate B-reps): ~5 tier 1-2, 9 tier 5 closure, 2 tier 6,
1 tier 4/7 planar. **None checks deviation from the exact surface or
self-intersection.**

### Corpora

`resources4truck` (now acquired): `step/abc-0000.step` (53,226 B),
`abc-0006` (39,989), `abc-0008` (40,445), `abc-0035` (192,899) from the ABC dataset
(MIT per its Readme); `occt-{cone,cube,cylinder,sphere,torus,assy}.step`;
`shape/*.json` (serde B-reps generated by truck's own examples, e.g. `large-torus`
500x100); `obj/by_occt.obj` (OCCT reference mesh); unrelated mesh models.

### Pathology handling

- `nonpositive_tolerance!` asserts `tol >= TOLERANCE` (1e-6) — refusal by panic
  (`truck-geotrait/src/lib.rs:17-29`; `src/tessellation/mod.rs:259,275,319,329`).
  Sphere panics on `tol >= radius` (`truck-geometry/src/specifieds/sphere.rs:135`). Untested.
- Per-face refusal: boundary points that fail `search_parameter` (100 Newton
  trials, `triangulation.rs:24-50`) make the face `None` (`:203-226`);
  `collect_option()` makes it explicit, but **`MeshedShape::to_polygon` silently
  skips `None` faces** (`src/tessellation/mod.rs:67`).
- Silent drops: spade insertion errors -> `None` (`triangulation.rs:506`);
  constraints failing `can_add_constraint` skipped (`:528-539`); point-in-boundary
  returns false on ray-through-vertex ambiguity (`:466-491`); `area.so_small2()`
  triangles dropped (`:653`); loop closure at absolute 1e-3 in (u,v) (`:360`).
- Termination: curve subdivision depth budget 100 (`algo/curve.rs:76-130`, stop
  `if dist2 < tol * tol || trials == 0`); **with NaN evaluation `dist2 < tol^2` is
  always false, so recursion fans out to depth 100 on both halves — effectively a
  hang** (inferred). Surface subdivision 100 passes (`algo/surface.rs:230,243`); NaN
  makes `> tol*tol` false and is accepted silently. No NaN/timeout tests.
- `remove_degenerate_faces` removes only index-repeating faces (`filters/optimizing.rs:303-305`);
  index range checks only under `debug_assertions` (`truck-polymesh/src/polygon_mesh.rs:35-42`).

### Fuzzing / property / performance

proptest in 10 crates; tessellation-related: `volume.rs:15`, `curve.rs:62`. No fuzz,
mutation or criterion benches; `#[ignore]` timing tests print and never compare
(`triangulation.rs:688-712`).

### Tolerance semantics

`TOLERANCE = 1e-6` absolute. `triangulation(tol)` is a chordal target checked at
**one hash-jittered sample** per segment/cell: curves split while the distance at
p in [0.4,0.6] exceeds tol (`algo/curve.rs:93-130`); surfaces split cells while one
jittered interior point deviates from the bilinear interpolant (`algo/surface.rs:219-306`).
Analytic overrides use exact sagitta: sphere `delta = 2*acos(1 - tol/r)`
(`sphere.rs:139`), torus per-direction (`torus.rs:191-201`). No angular or size
control. Achieved deviation never checked on face meshes.

### Known-bad handling

`#[ignore]` on benches only; no skip/xfail lists. The `RobustMeshableShape` doc
claims it "does not close the mesh" while `robust_closed` asserts it does — stale doc.

### Adoptable cases

`resources4truck/step/abc-{0000,0006,0008,0035}.step` (real ABC parts, closure);
`resources4truck/step/occt-{cone,cylinder,sphere,torus}.step` (analytic seams, cone
apex, sphere poles, written by another kernel); `resources4truck/shape/torus-punched-cube.json`
+ `obj/by_occt.obj` (paired reference mesh); `resources4truck/shape/large-torus.json`
(count stress). From test code (re-express in our format): `special_cylinder_model`
(`triangulation.rs:81-115`, seam split into two half-faces); `robust_closed`
(`:138-167`, curve-not-on-surface: refuse or project); fillet `complex_surface`
(`truck-shapeops/tests/fillet.rs:255-411`).

### Fault classes learned

Not watertight by construction (weld-dependent closure); curve not on surface;
silent face drop on flattening; periodic seam/domain wrap; pole/zero derivative;
under-sampled deviation check (one jittered sample); NaN -> exponential recursion
or silent acceptance; CDT constraint loss; ambiguous point-in-polygon dropping
triangles; sliver area filter opening holes; absolute epsilons (1e-6 world, 1e-3
UV); tolerance domain panics; mesh-to-mesh oracle slack hiding deviation.

---

## Fornjot (hannobraun/fornjot)

Revision `7e856686b8105d21ce4673c10622f6ef206b89af`, `quarantine/sources/fornjot`.
**State**: README line 1 "This project has been shut down. Its goals were never
reached."; mainline `crates/` not developed in over a year in favour of
`experiments/` (2024-10-30 ... 2025-12-03); last release v0.49.0 (2024-03-21);
`crates/fj-core` is a deprecation shim, code lives in `crates/fj/src/core/`.
Reference material on validation design, not a mature mesher. Read: README,
CHANGELOG head, experiments READMEs, `justfile`, `.github/workflows/ci.yml`;
`crates/fj/src/core/approx/{tolerance.rs,circle.rs}`,
`algorithms/triangulate/{mod.rs,delaunay.rs,polygon.rs}`, `algorithms/approx/{face.rs,curve.rs}`,
`validate/*.rs`, `validation/{config.rs,error.rs,checks/*.rs}`,
`new/validation/*`, `math/{scalar.rs,triangle.rs}`, `instance.rs`,
`interop/tri_mesh.rs`, `tools/export-validator/src/{main.rs,ffi.rs}`, `models/*`.

### Test inventory and oracles

64 `#[test]` (6 `#[ignore]`) plus the CI export validator over 8 models.
- Tier 7: circle discretization counts `test_increment(1., 0.5, 3.); test_increment(1., 0.1, 7.);
  test_increment(1., 0.01, 23.);` (`approx/circle.rs:95-97`) = ceil(pi/acos(1-tol/r));
  exact-triangle membership in triangulation tests (`triangulate/mod.rs:133-136`),
  which the code itself calls of limited use because triangulations are not unique
  (`:203-208`).
- **Validator negative controls**: build a valid tetrahedron, assert the check
  passes, mutate (remove a face, swap in a duplicate curve, reverse a cycle), assert
  it fails (`validation/checks/half_edge_has_no_sibling.rs:82-104`,
  `coincident_half_edges_are_not_siblings.rs:288-372`).
- Tier 5 in CI: `tools/export-validator/src/main.rs` exports each model to 3MF and
  calls lib3mf `lib3mf_object_isvalid` (= `isManifoldAndOriented`: every edge used
  once in each direction). Vertices merged by **exact** `Point` equality before
  export (`interop/tri_mesh.rs:143-163`) — exact-coordinate watertightness, stricter
  than truck's welded closure.
- Tier 2 production asserts: `delaunay.rs:58-61` triangle validity; `approx/face.rs:37-56`
  "Invalid approximation: Distinct points are too close".
None checks deviation from exact geometry, self-intersection or termination.

### Validation check catalog (property-oracle design reference)

`AdjacentHalfEdgesNotConnected` (`validation/checks/half_edge_connection.rs:38-160`);
`FaceHasNoBoundary` (`face_boundary.rs:15-31`); `InteriorCycleHasInvalidWinding`
(`face_winding.rs:21-75`); `HalfEdgeHasNoSibling` (`half_edge_has_no_sibling.rs:21-77`,
exact key matching); `CoincidentHalfEdgesAreNotSiblings` (`coincident_half_edges_are_not_siblings.rs:93-266`,
3 samples per edge pair, O(N^2)); `MultipleReferencesToObject` (all 6 tests
`#[ignore]`, `multiple_references.rs:211-438`); `DistinctVerticesCoincide` /
`IdenticalVerticesNotCoincident` (`validate/solid.rs:50-177`); sketch winding
(`validate/sketch.rs:47-110`); new-architecture `CoincidentNonSiblingHalfEdges`.
Curve/Cycle/HalfEdge/Region/Surface/Vertex validators are empty. Thresholds
(`validation/config.rs:40-68`): tolerance 0.001, `identical_max_distance = 10 x tol`,
`distinct_min_distance = 2 x identical_max_distance`.

### Corpora, pathology, tolerance

8 code-defined models (`models/`, 96 KB), lines/circles/sweeps only; `models/all`
applies non-axis-aligned transforms "useful for testing". `Tolerance::from_scalar`
rejects <= 0 but `From` panics (`approx/tolerance.rs:27-53`); NaN/inf `Scalar`
panics (`math/scalar.rs:52-58`); tol > 2r yields acos domain NaN -> panic
(inferred); `face.rs:37` uses a fixed absolute 0.02 regardless of the tolerance
passed (small models panic, inferred); default tolerance = smallest non-zero bbox
extent / 1000 (`instance.rs:86-102`). Tolerance definition: "the maximum allowed
deviation of the approximation from the actual shape" (`approx/tolerance.rs:5-6`);
circle sagitta exact; faces triangulated from boundary points only. No fuzz/property/benchmark.

### Adoptable cases

`sharp_concave_shape` polygon (1,0),(2,8),(2,9),(1,1),(0,9) (`triangulate/mod.rs:236-240`,
unconstrained Delaunay leaks outside); square with non-convex quad hole (`:145-153`);
ray-through-vertex and seam point-in-polygon cases (`polygon.rs:231-273`);
`models/holes`, `models/split`, `models/all`; validator mutations as negative
fixtures (delete a face; duplicate curve; reversed interior cycle).

### Fault classes learned

Unconstrained-Delaunay leakage; coincident non-sibling edges (crack generator);
open shell; disconnected cycle; wrong hole winding; topology aliasing; vertex
identity vs position; scale-blind absolute epsilon; tolerance outside formula
domain; non-unique triangulation making exact-triangle oracles brittle;
exact-coordinate welding.

---

## SolveSpace

Revision `ef966335e7fbc617ba8e759149a567158b37aa7b`, `quarantine/sources/solvespace`.
Read: `test/{harness.h,harness.cpp,CMakeLists.txt}`, all 9 `test/group/*/test.cpp`,
`test/analysis/contour_area`, `test/core/{prune,solver}` heads,
`test/constraint/pt_face_distance`; `bench/harness.cpp`; `src/mesh.cpp`
(1-140, 640-1221); `src/srf/{triangulate.cpp, ratpoly.cpp, surface.cpp, boolean.cpp,
surfinter.cpp}` (cited ranges); `src/{solvespace.cpp, generate.cpp, groupmesh.cpp,
textscreens.cpp, export.cpp, polygon.h, polygon.cpp, dsc.h, defs.h}` excerpts;
`.github/scripts/build-ubuntu.sh`; `CMakeLists.txt` sanitizers.

### Test inventory

One harness, 270 `TEST_CASE`s in 61 files; 238 `.slvs` fixtures (3.1 MB), 86 golden
PNGs. Mesh-relevant: six `test/group/boolean_*` regressions for NURBS Boolean
failures — `boolean_coplanar_union` (#1452, phantom split points, missing side
face), `boolean_knife_edge` (#1091, four coincident edges), `boolean_tangent_crossing`,
`boolean_tangent_fillet`, `boolean_tangent_spline` (#1291, tangent/grazing
contact), `boolean_tangent_edge` (#1619, result "failed or succeeded depending on
uninitialized memory"); `test/group/translate_asy` (interference positive
control); ~13 golden renders. **No direct tests of `triangulate.cpp`, STL/OBJ
export, chord tolerance or `ratpoly.cpp`** (grep of `test/` for
Export/stl/Triangulat/chordTol is empty).

### Oracles

Strong set (`test/group/boolean_knife_edge/test.cpp:12-35`):
```cpp
SKdNode::From(m)->MakeCertainEdgesInto(&el,
    EdgeKind::NAKED_OR_SELF_INTER, /*coplanarIsInter=*/true, &inters, &leaks);
CHECK_FALSE(inters); CHECK_FALSE(leaks); CHECK_TRUE(noEdges);
CHECK_EQ_EPS(m->CalculateVolume() / 1.8303900357690596, 1.0);
```
`CHECK_EQ_EPS` is `fabs(value - reference) < LENGTH_EPS` (1e-6; `harness.cpp:196-207`,
`src/defs.h:7`). Analytic volumes (tier 7) in 3 tests (13250.0, 296000.0, 1.83039...);
**mesh-copied volumes (tier 3 dressed as 7)** in 3 (`205477.058604742`,
`0.9730161212`, `204638.6271882122`) whose comments admit inscribed facets — a
chord-tolerance change would break them. Golden renders: pixel-exact
`refPixmap->Equals(*frame)` at 600x600 (`harness.cpp:250-319`). `CheckSave`
**deletes Surface/SCtrl/TrimBy/Curve/CCtrl/CurvePt lines** before comparing
(`:135-139`), so the B-rep shell is never compared. CI builds with ASan+UBSan
(`.github/scripts/build-ubuntu.sh:8`); test order shuffled by seed (`harness.cpp:363-366`).
Tally for ~20 mesh tests: 7 tier 5 (3 of the volume checks tier 7, 3 tier 3), ~13
tier 3 pixel goldens.

### Bad-mesh detection and reporting

`SKdNode::MakeCertainEdgesInto` (`src/mesh.cpp:954-1046`, `FindEdgeOn` `:837-932`):
naked edge = an edge without exactly one anti-parallel mate (within `LENGTH_EPS`;
`:975-990`), multiplicity-balanced so matched non-manifold stacks pass, and
T-junctions are not geometrically detected (inferred). Self-intersection = edge
strictly crossing a triangle plane inside the triangle (`:886-925`); source admits
"should handle in-plane intersections separately but don't yet" (`:891-893`).
UI "Show Naked Edges" (`solvespace.cpp:1065-1097`) reports watertight/valid or
"%d problematic edges, bad."; "Show Interfering Parts" (`:901-918`). Mesh export
calls `ShowNakedEdges(/*reportOnlyWhenNotOkay=*/true)` and **writes the file
anyway** (`export.cpp:829`). Boolean failure sets `booleanFailed` and suggests
"force NURBS surfaces to triangle mesh" (`boolean.cpp:733-741`, `textscreens.cpp:575-580`).
**Silent, debug-log-only failures**: whole face dropped on trim-polygon assembly
failure (`surface.cpp:453`); "couldn't find an ear! fail" returns a partial
triangulation (`triangulate.cpp:494`); "couldn't merge our hole" (`:85`);
`AddTriangle` drops near-collinear triangles `|ab x bc| < 1e-10` (`mesh.cpp:20-23`).
`RemoveDegenerateTriangles` before `SnapToMesh` explicitly to stop polynomial
triangle-count growth (`groupmesh.cpp:432-435`).

### Pathology, performance

Structural recursion bounds only: curve PWL stops at `(tb-ta) < 1/maxSegments`
(`ratpoly.cpp:266-267`), grid at depth > 3 for twist (`triangulate.cpp:571-575`),
surface-intersection marching capped at `max(300, maxSegments*3)` (`surfinter.cpp:475`).
No hang/timeout/NaN/malformed-file tests. `bench/harness.cpp` times load+regenerate,
no threshold, not in CI. No fuzz/property/mutation.

### Tolerance semantics

Display chord tolerance is relative: `chordTol` percent (default 0.1) of the
bounding-box max size (`generate.cpp:220-226`); export tolerance absolute mm
(default 0.1, `solvespace.cpp:54,500-502`); `maxSegments` 20 display / 64 export.
**The segment cap overrides the tolerance**: curve stop condition
`((tb - ta) < step || d < chordTol) && (tb-ta) <= max_dt` (`ratpoly.cpp:258-275`)
stops subdivision once a span is short in parameter even if deviation `d` exceeds
the tolerance — a silent tolerance violation. Surfaces: planes/bilinear ear-clipped
from trims; others an adaptive (u,v) grid sampled at {0,1/3,2/3,1} with a 2.5 degree
twist test, then trim-band ears chosen by `ChordToleranceForEdge` at 3 samples
(`triangulate.cpp:503-516,536-711`) and **never refined**: trim-band triangles
carry no tolerance guarantee. Achieved deviation never tested.

### Adoptable cases

`test/group/boolean_{coplanar_union, knife_edge, tangent_edge, tangent_fillet,
tangent_spline, tangent_crossing}/normal.slvs` (Boolean-output fixtures with
zero-naked-edge/no-self-intersection predicates and analytic volumes where noted;
to be re-expressed as B-rep fixtures), `test/group/translate_asy/normal.slvs`
(interference positive control), `test/group/translate_nd/normal.slvs` (285 KB scale).

### Fault classes learned

Missing face on trim assembly failure (silent); partial ear-clip triangulation
(silent); phantom split vertices -> T-junctions; tangent-contact misclassification;
grazing intersection non-convergence; knife-edge multiplicity; uninitialized-state
nondeterminism; tolerance silently capped by segment limits; unrefined trim-band
ears; display vs export tolerance mismatch; in-plane self-intersection blind spot;
collinear triangle drops opening cracks; degenerate-triangle count blow-up;
report-not-refuse export; pixel-golden brittleness.

---

## NVIDIA usd-brep

Revision `c9c5979f04cea8c38213004d4f04df0dcfb40d05` (`VERSION` 1.0.0),
`quarantine/sources/usd-brep`; binary package `quarantine/nvidia-usd-brep-1.0.0-linux`
inventoried and diffed only. Extends, does not repeat,
`papers/2026-10-16-native-meshing-reference-test-plan.md` and
`papers/2026-10-17-nvidia-smtess-study.md`. Read: `tests.sh`, `repo.toml:70-105`,
`tests/prog_test/{prog_test.sh, src/prog_test.cpp, src/smTessTest.cpp (82-196, 2139-2960,
3367-3942)}` plus grep counts over all 13 prog_test files; `tests/SM_API_test/src/{SM_API_test.cpp,
TestSmBrep.cpp (580-640, 1040-1460), TestSmGeneral.cpp:862-940, TestSmErrorCodes.cpp:201-293}`;
`TestSmApiUsd.cpp:155-656`; `usd_regression_test.cpp` (1326-1424, 2595-3168); HYDRA tests;
`tools/brep_validator/brep_validator.py` (requirements 69-1805, checks 2009-8110),
`tools/brep_validator/README.md`, `tools/brep_validator_test/*.py`,
`tools/brep_geometry_validator/src/brep_geometry_validator_main.cpp`; OCCT<->USD
converter tests; kernel `SmBrep.cpp:37029-37140` (CompareBreps),
`SmEdgeuse.cpp:5929-6345`, `SmFace.cpp:19680-21052`, `SmMessages.h`, `SmConfig.h`,
`SmApiTessellationParams.h`; `KNOWN_ISSUES.md`; `TestFiles`, `TestFilesSmb`.

### Test inventory

- No committed CI config; `repo.toml:89-104` says `tests.sh` is "the same script CI
  invokes" (aggregate `timeout = 3600`). `tests.sh` runs 13 suites in order, aborting
  on the first nonzero exit (usd_test_app, usd_brep_utils_test, prog_test,
  SM_API_test, SM_API_USD_test, HYDRA, geometry validator, heal_file, SmPyLib,
  SmPyDevLib, brep_validator_test, occt_to_usd, usd_to_occt). On macOS a missing
  binary is a skip. `tests/python/roundtrip_{smlib,usd}_test.py` are never invoked.
- prog_test: 17 suites (16 default); a suite passes iff it returns `SM_SUCCESS`
  (`prog_test.cpp:156-161`); logs never compared. `my_test_tess`
  (`smTessTest.cpp:2365-2950`) runs tests 1-16 (tri-strips, STL read/merge of
  `Tess/BoxFillet.stl`, `Solids/fillet_ts.smb` stitch+tessellate,
  `Solids/nurb_fillet.smb` at several chord heights, ray hits on `Tess/Extrusion.pbp`,
  sewing until no lamina edges, decimation and poly-Boolean regressions).
  `smSurfaceTest.cpp:915-923` `my_test_surface_tessellation` is a stub returning success.
- SM_API_test: 115 `TestSm...()` calls; signal handlers turn SIGSEGV/SIGABRT/SIGFPE
  into exit codes (`SM_API_test.cpp:1720-1735`). `TestSmTessellate`
  (`TestSmBrep.cpp:1348`: Failures, MaxEdgeCap, Params, Boundaries);
  `TestSmPolyBrepMassPropertiesWinding` (`TestSmGeneral.cpp:862`).
  `TestSmInvalidInputCodes` (`TestSmErrorCodes.cpp:201`) covers ~27 entry points but
  **not `SmApiTessellate`**.
- SM_API_USD_test: 20 functions, incl. `TestTessellatedMeshData:155`,
  `TestSubsetIndicesSkipLooplessFaces:230`, `TestEmptyMeshExtraction:354`,
  `TestTessellateBrepArray:517`, `RejectsMixedValidInvalidBreps:814`,
  `SingularWorldTransform:977`.
- Python: ~516 `def test_` methods (400 in `test_omni_solid.py`); ~25 touch
  tessellation (`test_omni_solid.py:606-800, 5623-5762, 5991`).
- Fault classes: UV trims shifted out of domain by (1000,1000) (`TestSmBrep.cpp:1127-1203`);
  missing edge geometry (`:1220-1238`); NaN/zero/null inputs to boundary sampling
  (`:1240-1262`); failing members alongside surviving ones (`TestSmApiUsd.cpp:586-627`);
  winding reversal; concurrency determinism (`TestSmApiUsd.cpp:560-582`); healer
  crash on a Revit wall (`usd_regression_test.cpp:2595-2655`); 107 schema negatives;
  periodic span boundaries (`test_validation_boundary.py:152-212`); seam/pcurve
  representations through OCCT (`test_occt_to_usd.py:679-858`).

### Oracles

- **Tier 1 by construction in release CI**: prog_test tests 5-8 ignore the status
  of `my_test_one_tess`, which writes an STL, deletes it and returns `SM_SUCCESS`
  unconditionally (`smTessTest.cpp:2615-2640`, `:~190`). `SM_ASSERT(a)` expands to
  `if(a) {}` in release (`SmMessages.h:714`); `SE(a)` is `((void)(a))` (`:627`);
  `SM_ASSERT_VALID` compiles away without `SM_DEBUG_CODE`, set only under `_DEBUG`
  (`SmAssertArray.h:133`, `SmConfig.h:126-131`). `tests.sh` runs release builds, so
  **the prog_test tessellation suite only checks that it returns success**. Across
  prog_test: ~2,500 `SER` calls vs ~185 explicit `return SM_ERR`.
- Tier 2: most SM_API tests; `my_test_healer_no_crash`: "that crash IS the
  regression signal… it returns here regardless of whether the healed solid is
  valid" (`usd_regression_test.cpp:2641-2643`).
- Tier 3: `report.laminaEdges != 4 || report.spineEdges != 0 || report.failures.GetSize() != 1`,
  `polygons.GetSize() != 10` (`TestSmBrep.cpp:1155-1170`); `CompareBreps` round trips
  (counts, types, max vertex gap; not surfaces, trims or edge curves).
- Tier 4: relative vs absolute max-edge equivalence at `kTol = 1.0e-9`
  (`TestSmBrep.cpp:1301-1309`); refined output larger than coarse (`:1342-1344`);
  reversed winding keeps volume 48 (`TestSmGeneral.cpp:903`); 4 threads x 8
  iterations bitwise identical (`TestSmApiUsd.cpp:570-573`).
- Tier 5: `CheckTessellatedBrep` (`TestSmApiUsd.cpp:485-515`: indices in range, points
  finite, `std::abs(rNormal.GetLength() - 1.0f) >= 1e-4f` fails); every undirected edge
  used exactly twice — box and cylinder only (`test_omni_solid.py:750-788`).
- Tier 7: cylinder side normals equal `(x/r, y/r, 0)` to 6 places
  (`test_omni_solid.py:774-777`); planar box volume 48.
Tally: of ~45 tessellation checks ~35 tier 1-3, ~10 tier 4-7. **No test compares
triangle-to-surface deviation with chord height, checks self-intersection, compares
a curved mesh's volume/area with analytic values, or checks facet orientation
against the face normal.** Inside-out triangles near fillets are a documented known
issue (`KNOWN_ISSUES.md:45-49`) with no test.

### Corpora

`TestFiles` + `TestFilesSmb`: 580 files, 27,934,459 B. `pt_TestFiles` (398 files,
~25.9 MB, SMLib-native: Fillet 98, Drop 50+20, Solids 38 incl. `airfoil.brep` 771 KB,
Shell 25, PointSet 22, STL 18, Defeature 12 incl. `Platte.smb` 1.23 MB, Tess 8, plus
SSI/CCI pairs such as `ssi_rhino_bug1`, `ssi_coincident_spheres`, `ssi_tor_overlap`);
**135 of 398 are referenced by basename nowhere** (approximate). `brep_validator/`
130 files: 107 `Test_BA_<code>_<defect>.usda` negatives (107 of 191 codes), mutations
of `CubeBrepArray.usda` (generator docstring: "out of sync… some … edited
manually"). `usd_TestFiles` 16 (non-manifold `.smb`, `RevitWallHealerCrash.usda`).
`occt_breps` 29 self-authored files (seams with/without pcurves, two-turn cylinder,
partial sphere, inconsistent/sparse pcurves, plate with hole, periodic B-spline,
conics). The binary package ships only the SM_API test apps; its validator fixtures
are byte-identical to the source.

### Pathology handling

NaN rejected in boundary sampling, mass properties, helix inputs, validator BA.660;
**no NaN/Inf/negative-tolerance test for `SmApiTessellate`**. `KNOWN_ISSUES.md:61-65`:
malformed topology "can read out of bounds and crash the process"; mitigation is
validate-first; untested. No in-process termination test; subprocess timeouts only
(300 s OCCT conversion, 120 s HYDRA, 3600 s suite); the 100-pass retry cap has no
test. No memory tests or sanitizers; "no memory is freed in most of the tests"
(`prog_test.cpp:360-364`). Periodic span `2*pi - 2e-6` accepted, `2*pi + 2e-6` rejected
(`test_validation_boundary.py:167-192`).

### Fuzzing / property / performance

No fuzz/hypothesis/sanitizer (grepped tests, tools, premake, repo config). Fixed
hand mutants only. prog_test prints `clock()` per suite with no threshold.

### Tolerance semantics

`SmApiTessellationParams.h:14-36`: `kChordHeightTol = 0.0` ("0 = ignore", absolute);
curve and surface angles 25 degrees; `kMaxEdgeLength` a target, "not a hard cap",
negative = 0.025 x |v| x bbox diagonal. **The default mesh is angle-only.** Tests
verify chord height and angle only through count monotonicity. Validator:
`NUMERICAL_TOLERANCE = 1e-11`, `UV_CLOSURE_TOL = 1e-6` absolute in parameter space.

### Known-bad handling

No xfail; defects only in `KNOWN_ISSUES.md`. **A missing LFS healer fixture prints
"SKIP" and returns `SM_SUCCESS`** (`usd_regression_test.cpp:2604-2610`), so a
pointer-only checkout passes silently. Retired validator rules with reasons
(`brep_validator/README.md:40-55`); `test_nurbs_endpoint_agreement_is_left_to_kernel`
pins a scope limit.

### brep_validator: candidate ingress refusal catalog

Layer 1 (`tools/brep_validator/brep_validator.py`): 191 requirements, 67 check
methods. Families: per-array sizes/types/index ranges (BA.000-327);
`intersectTol3d` not positive (BA.010); face with no loop (BA.140);
degenerate/reversed ranges (BA.155-275); control vertices outside extent
(BA.310-465); NURBS order/weights/knots (BA.330-471: weights <= 0 rejected, knots
decreasing); analytic carriers (BA.480-555: radius <= 0, non-unit/non-orthogonal
frames, cone semi-angle outside (0, pi/2)); periodic spans > 2*pi, sphere V outside
[-pi/2, pi/2] (BA.560-571); not exactly 2 faceuses per face (BA.580); radial chain
not closed (BA.581); orphan edge (BA.582); line/circle/ellipse endpoints not at
vertices within `intersectTol3d` (BA.600-602); circle endpoint off radius (BA.610);
any NaN/Inf (BA.660); radial chain mixing edges (BA.670); UV trims far outside
face range (BA.750); UV loop not head-to-tail (BA.763); zero-length UV trim
(BA.764). Layer 2, kernel `AssertValid` levels 0-2 (`tools/brep_geometry_validator`):
UV domain outside natural domain; degenerate UV domain; surface degenerate to a
point/curve within faceTol/4; crossed/missing/near-miss seam; coincident or
partially coincident vertices/edges; inner loop outside outer or nested in a hole;
loops sharing a vertex; trim jump across a periodic boundary; trim shorter than
tol3d; dog-leg near a pole; radial/mate orientation not opposite; trim interval not
equal to/contained in edge interval; radial point distance above edge tolerance.
The tests for layer 2 cover plumbing only.

### Adoptable cases

`TestFiles/occt_breps/{cylinder_u_seam, cylinder_u_seam_no_pcurves, cone_u_seam,
sphere_u_seam, torus_u_seam, torus_v_seam, cylinder_two_turn,
sphere_partial_full_uv_extent, plane_inconsistent_pcurves, box_sparse_pcurves,
plate_with_hole, sphere_hole, periodic_bspline, bezier_surface, hyperbola,
parabola}.brep`; `TestFiles/brep_validator/coverage_limits/{shifted_partial_period_face,
shifted_full_period_face, cylinder_without_repeated_edge, cylinder_with_vertex_loop}.usda`;
`Test_BA_{581,582,580,763,764,175,155,160}_*` negatives; `DefectiveCube.usda`,
`DefectiveExtents.usda`; `TestFiles/usd_TestFiles/RevitWallHealerCrash.usda`;
non-manifold `.smb` set (`boxes_share_one_edge`, `boxes_share_one_vertex`,
`box_with_detached_sheet`, `box_with_two_vertex_shells`, `nonManifoldCubes.usda`,
`cubeWithVoid.usda`); `pt_TestFiles/Fillet/{vase1, vase3, blade_hub_1..3, cliff_1..4,
torii_fillet, blade_fillet_fails}.smb` (needs SMLib-format reader);
`pt_TestFiles/Defeature/Platte.smb`; `pt_TestFiles/Solids/airfoil.brep`; NVIDIA's own
tessellation inputs `Solids/nurb_fillet.smb`, `fillet_ts.smb`,
`Tess/nurby_box_cyl_sphere.smb`, `tess_20_2_80_0.smb` to re-run under real oracles.
Recipes: (2,1,1)-scaled cylinder with trims at 1000..1010 (`TestSmApiUsd.cpp:586-610`);
box face with trims shifted by (1000,1000) expecting refusal naming face 3
(`TestSmBrep.cpp:1137-1147`); periodic span 2*pi ± 2e-6.

### Fault classes learned

Trims outside the surface domain; missing edge geometry (partial output, not a
crash); partial-success masking; lamina/spine edges counted, not refused; winding
reversal; inside-out triangles near fillets; concurrency nondeterminism; parameter
plumbing loss visible only on curved faces (planar faces ignore quality controls);
relative vs absolute scaling errors; periodic span violations and shifted ranges;
pole latitude bounds; seam crossing/missing/near-miss; dog-leg trims near a pole;
degenerate patches; loop nesting/disjointness; coincident edges/vertices; open UV
loop; zero-length UV trims; trim/edge interval mismatch; radial chain corruption;
orphan-edge diagnostic cascade; endpoint-vertex disagreement; non-finite data;
malformed analytic frames; NURBS structural invalidity; foreign-exporter UV
conventions; out-of-bounds reads on malformed topology; non-manifold multi-body
inputs; **release-compiled-away assertions making a green run meaningless**;
missing-fixture skips reporting success.

---

## openNURBS

Revision `eb92af3ba1806b0a34a99aba0d3bda83e3d46083` (per
`quarantine/new-iteration/MANIFEST.md`), `quarantine/new-iteration/opennurbs`. Read:
`opennurbs_mesh.h:544-1383` (`ON_MeshParameters`); `opennurbs_mesh.cpp`
(`ON_MeshFace::IsValid` 160-240, `IsCorrupt` 933-1035, `ON_Mesh::IsValid` 1045-1270,
`IsManifold/IsClosed/IsOriented/IsSolid` 3469-3950, setters 6803-7195, density and
tolerance maps 7222-7425, `ON_MeshTopology::IsValid` 10733-10800);
`opennurbs_statics.cpp:1070-1270`; `opennurbs_brep.h:1800-1880`; `opennurbs_brep.cpp`
(Mesh/SetMesh 1120-1180, `IsValidTrim/Loop/Face/Edge/Vertex` 2774-4465, `IsCorrupt`
4465-4640, `ON_Brep::IsValid` 4645-5760); `opennurbs_brep_isvalid.cpp`;
`opennurbs_brep_io.cpp`; `opennurbs_planesurface.cpp:769-830`; SubD mesh excerpts;
NURBS/knot `IsValid`; `opennurbs_extensions.cpp:4192-4226`; `CMakeLists.txt:375-460`;
`example_test/*`, `example_brep/example_brep.cpp:505-600`; the `example_files/` tree.

### Is there a mesher? No.

`ON_Brep::CreateMesh(const ON_MeshParameters&, ON_SimpleArray<ON_Mesh*>&)` is declared
(`opennurbs_brep.h:1815`) and defined nowhere (grepped all `.cpp` for
`ON_Brep::CreateMesh` and `ON_Surface::CreateMesh`). `CMakeLists.txt:381`
`if(EXISTS …/opennurbs_plus.h)` lists private sources absent from the public tree
(`opennurbs_plus_brep.cpp`, `opennurbs_plus_trimesh.cpp`, `opennurbs_plus_validate.cpp`,
`opennurbs_plus_mesh_*`, `opennurbs_plus_testheader.h`): the mesher, a richer
validator and McNeel's real test harness live there (inferred). What exists: cached
per-face render/analysis meshes read/written in brep chunks
(`opennurbs_brep_io.cpp:735-770,1011-1037`); SubD fragment containers (the archive
never saves the SubD mesh: "TODO change to c = 1 when ON_SubDMesh::Write()/Read()
actually work", `opennurbs_subd_archive.cpp:1504-1506`); a one-quad
`ON_PlaneSurface::CreateMesh`. **No tessellator test suite**: `example_test/README.md`
says part of it moved to a GoogleTest in `src4/opennurbs/tests/fileio`, not public.

### Tests and oracles

1. `example_test/example_test.cpp`: 3dm read-write-read with `ONX_Model::ContentHash()`
   comparison (tier 4) plus manifest/iterator invariants (tier 5, bookkeeping); hash
   comparison skipped for 3dm versions 1-69 (`opennurbs_extensions.cpp:4192-4197`),
   so only 15 of 153 shipped files get it. Harness defects: `example_test.cpp:1123`
   repeats `FailureCount()` where `WarningCount` was meant; `TestResult(type)`
   ignores its argument (`opennurbs_extensions.cpp:4208,4223`).
2. `ON_MeshParameters_AreValid()` static-init self-check: presets classify back to
   their own type; density round-trips for 9 values (tier 7); failure only `ON_ERROR`.
3. `example_brep`: twisted cube must pass `IsValid()`.
No mesh output is tested at any tier.

Reusable mesh predicates: `ON_Mesh::IsValid` (normals unit if squared length within
[0.985,1.015], `opennurbs_mesh.cpp:1147`; float-range bbox as NaN/inf proxy;
degeneracy only exact coincidence `!(V[vi[0]] != V[vi[1]])` — collinear zero-area
triangles pass). `IsClosed/IsManifold/IsOriented` key edges by **position-welded**
vertex-location ids (`:3705-3900`); comment at `:3835`: "It is possible that
degenerate meshes, like a flattened box, will be flagged as closed."
`ON_Mesh::IsPointInside` always returns false (`:3500-3510`).

### Corpora

`example_files/`: 168 files (~113 MB); 153 `.3dm` (116,318,495 B) across V1-V8.
V4 McNeel showcase products (`v4_Wheel_PG.3dm` 10,982,355 B, DVDCase_Solid, Gear,
MechPartA/B, PerfumeBottle_Solid, Fluted_Cup, SolidHandle); V5 `v5_clip` (9.05 MB),
`v5_disk_brake` (9.68 MB), teacup, teapot, ring, extrusions; V6/V7 SubD logos; files
written by `example_write` (`my_brep`, `my_trimmed_surface`). A byte heuristic
(ON_Mesh class-UUID counts) suggests cached Rhino render meshes in several V4/V5
models (`v5_disk_brake` 888 mesh ids / 66 brep ids; `v5_teapot` none); inconclusive
for compressed/legacy chunks.

### Pathology and validity

NaN/Inf rejected via `ON_IsValid` on knots and `ON_CV_COORDINATE_IS_VALID`
(`-1e307 < x < 1e307`). `ON_NurbsSurface::IsValid` **does not check weights**;
curves allow negative weights of consistent sign. Validator bugs: seam-trim matching
puts the `return` inside `if (text_log)` so a null log falls through to
`m_E[trim.m_ei]` (`opennurbs_brep.cpp:5515-5522`); `:5548,5554` dereference
`text_log` without a null check — **validity verdict depends on whether logging is on**.
Meshing-parameter setters silently keep the old value on NaN/out-of-range input
(`opennurbs_mesh.cpp:6825-6851`). No termination/memory/fuzz/CI anywhere.

### Tolerance semantics

`ON_MeshParameters` (`opennurbs_mesh.h:1162-1286`): `Tolerance` = "approximate
maximum distance from center of edge to surface" — a midpoint criterion, not a
maximum; `RelativeTolerance` mapped by `ToleranceFromObjectSize`
(`e = rel<0.5 ? 1+rel(6-4rel) : 2+2rel; tol = size*10^-e`, `opennurbs_mesh.cpp:7383-7425`;
Fast 5e-4 x size, Quality 2.5e-4 x size); `MinimumEdgeLength` default 1e-4 absolute
("The 1.0e-4 is a guess", `:7368-7380`) — edges shorter are never split even when
other criteria fail; `MaximumEdgeLength`; grid stage (`GridAspectRatio`,
`GridMinCount/MaxCount`, `GridAngle`, `GridAmplification`); refine stage
(`RefineAngle`); `JaggedSeams` (independent per-face meshing: gaps and T-joints);
`SimplePlanes`; `ClosedObjectPostProcess` (closes gaps from "loops in trimming
curves"). Density slider: `SetRefine(density < 0.65)` (`:7238-7240`) — **raising
density above 0.65 switches refinement off**. B-rep validity: trim endpoint vs
edge endpoint within `10*(edge_tol + t0_tol + t1_tol)` floored at 0.01 absolute
(`opennurbs_brep.cpp:5262-5290`), endpoints only; trim inside surface domain with 1%
normalized fuzz, loosened because tighter "is generating more tech support than it
is worth" (`:4296-4340`).

### Ingress refusal catalog (ON_Brep::IsValid and parts)

Structural `IsCorrupt` (index ranges, back-pointers); brep level (empty; faces
without edges/loops/surfaces/trims; deleted components still referenced; invalid or
wrong-dimension curves/surfaces; nested polycurves; `m_iso` disagreeing with
`IsIsoparametric`; `m_bRev3d` inconsistent; trim endpoint farther than `dtol` from
edge endpoint; adjacent trims not sharing vertices; 2D trim outside domain; split
seam; three seam trims for one edge; unmatched seam; mismatched seam iso pair);
trim (domain not increasing; singular trim with unequal vertices; boundary trim
whose edge has >= 2 trims; mated trim alone; negative tolerance); loop (no/duplicate
trims; slit loop rules; ends do not meet in 2D); face (no loops; first loop not
outer; null surface; transposed proxy); edge (vertex not referencing it; closedness
disagreeing with vertices; negative tolerance); vertex (back-reference mismatch).
**Not checked**: vertex tolerance vs actual edge-end distance; interior
edge-vs-trim deviation; loop orientation; loop self-intersection; inner-loop
containment; shell closure; surface weights.

### Adoptable cases

`example_files/V4/v4_Wheel_PG.3dm`, `V5/v5_disk_brake.3dm`, `V5/v5_clip.3dm` (product
scale, probable cached Rhino meshes as a tier-6 reference of unknown deviation);
`V5/v5_teacup.3dm`, `V5/v5_ring.3dm`, `V5/v5_teapot.3dm`; `V4/v4_{DVDCase_Solid,
PerfumeBottle_Solid, SolidHandle, Gear, MechPartA, MechPartB, Fluted_Cup}.3dm`;
`V1/v1_T-Joint1.3dm`, `v1_T-Joint2.3dm` (T-junctions, inferred from name),
`V1/v1_{five_solids, UntrimmedSurfaces, MatchSrf, Patch}.3dm`; `V5/v5_extrusions.3dm`
(`ON_Extrusion` ingress); `V7/v7_my_brep.3dm`, `v7_my_trimmed_surface.3dm`;
`V7/v7_rhino_logo_subd.3dm` (refuse or out of scope); twisted cube recipe
(`example_brep/example_brep.cpp:505-596`).

### Fault classes learned

Declared-but-unimplemented mesher API; silent parameter rejection; non-monotone
density knob; absolute-unit hard floors; midpoint-only chord criterion; jagged
seams; closed-object gap repair masking trim defects; float-collapsed faces
(RH-10177/54563/30283 comments); exact-only degeneracy test; position-welded
closedness; endpoint-only trim/edge agreement; 1% trim leakage tolerated; split
seams; seam-pair corruption; unchecked rational weights; logging-dependent validator;
cached-mesh staleness; version-gated round-trip checks.

---

## SISL

Revision `e0a82b1a851e932657f3e1af25033fbbcd33e907` (per
`quarantine/new-iteration/NUMERICAL_MANIFEST.md`), `quarantine/new-iteration/sisl`.
Read: `CMakeLists.txt`, `README`, `ChangeLog`, `doc/manual/manual.toc`,
`SISL_4.7_manual.pdf` pp. 2-4, `doc/manual/chap_introduction.tex:210-232`,
`chap_intro_abridged.tex:70-90`, function docs for s1237/s1238/s1242/s1613/s1506/
s6shadepol/s1310/s1450; `src/s1613.c`, `src/s1613bez.c`, `src/s1450.c:150-240`,
`src/s1451.c:70-135`, `src/sh1762.c` excerpts, `include/sislP.h:380-460`; all `app/*`
and `examples/*.cpp`; `viewer/src/sisl_aux.cpp:55-320`. Web: GitHub issues #67, #7.

**No surface tessellator and no test suite.** The 4.7 manual has no tessellation
chapter. Nearest relatives: `s1613` (curve -> polyline with an a-priori bound),
`s1237/s1238` (isocurve wireframes), `s1506` (grid evaluation), viewer display mesh
= refined control net with no error bound (`viewer/src/sisl_aux.cpp:81-205`).
`CMakeLists.txt:42` is literally `# Apps, examples, tests, ...?`; no `add_test`, no
CI, zero `assert(` in 615 `src` files. `examples/example01-15.cpp` throw on
`jstat < 0` (tier 2); `app/*` print only (tier 1); `app/s1320prob.C:128` "Denne
linja krasjer" ("this line crashes").

Pathology: `s1613bez` caps recursion at `klevel > 200` -> status 2 -> `jstat = -160`
"Curve too complicated. No linearization is performed" (`s1613.c:302-308`) — a good
explicit refusal, but `klevel` is function-static (not thread-safe, inferred).
`sh1762` uses static `xc`/`xmax` to steer subdivision order (`sh1762.c:66-67,5481,5497,6075`);
upstream **issue #67** (open): concurrent `s1859` calls race and ThreadSanitizer
reports "different intersection curves, curve counts, and downstream tessellation".
`s1613` never checks `aepsge`: `tant = (tb-ta)*sqrt(smaxd/(8*aepsge))` (`s1613.c:283`)
gives inf at 0 and NaN below 0, then a `(int)tant` cast (UB, inferred). Issue #7:
`rcoef` holds weight-premultiplied coefficients; users passing unmultiplied weights
get non-planar circles, "by design".

Tolerance: `aepsge` "tells when two points are regarded as equal"; `aepsco`
"included from historical reasons and no longer used". `s1613` bound per knot
interval |error| <= (b-a)^2 max|f''|/8 **per coordinate** (Euclidean error up to
sqrt(dim) x aepsge, inferred); `s1613` never reads `ikind`/`rcoef`, so the bound is
not valid for rational curves (inferred). Fixed absolute constants
`AEPSGE` 1e-6, `REL_PAR_RES` 1e-12 (`include/sislP.h:384-388`).

Adoptable recipes: `app/s1320prob.C` (rational quadratic arc vs line, crash
reproducer); `app/s1606prob.C` (sub-millimetre curves at epsge 1e-6); issue #7 circle
(9-point degree-2, corner weight 0.707) for weight-convention ingress; issue #67 for
concurrency determinism; `s1613` with `aepsge <= 0` and a curve driving recursion
past 200 levels.

Fault classes: no surface tessellator (display uses an unbounded control net);
per-coordinate bounds taken as Euclidean; rational curves linearized with
polynomial bounds; weight premultiplication ambiguity; closedness judged on the
control polygon without weights; static recursion state -> thread nondeterminism;
recursion cap turned into explicit refusal (good practice); unvalidated tolerance
arguments; absolute hard-coded epsilons; ignored legacy parameters; tangential SSI
instability tied to tolerance size; reproducers without expected values.

---

## step-conformance (dan335/step-conformance)

Revision `96f30bbe09549ef37dbd0a956d70bd8a183fd5d2`, `quarantine/sources/step-conformance`
(82 files). Found by the tracker lane, acquired and read by this lane: `README.md`,
`corpus/` listing and `corpus/LICENSE` (CC0 1.0), `corpus/generated/cone_point.truth.json`.
**Not mature**: the GitHub repository was created 2026-10-12, has one star and a
single-day push history. It is included because its test design is the only one in
this study that scores tessellated output against exact geometry, and its corpus is
CC0. Treat its reported results as one author's measurements, not established fact.

### Design (from README)

- Readers are external programs `PROGRAM OUT TOLERANCE FILE` that write
  `OUT/<stem>.bin` (vertices f64, triangles u32, **per-B-rep-face triangle counts —
  "Every face appears, even one with no triangles; that is how a face given up on is
  caught"**) or `OUT/<stem>.err` with a reason. This is the refusal contract we want.
- Each part is run at 0.0125, 0.05, 0.1 and 0.5 mm. Checks: (1) **file facts** read
  from the STEP text by an independent tool (step-check): at least as many faces as
  the file defines, none empty; no mesh outside the file's own geometry bound; mesh
  reaching every B-rep vertex; unit and assembly placement; (2) **two-way distance**:
  mesh-to-true-surface at every vertex and triangle centre, and true-surface-to-mesh
  (finds missing surface). True surface is exact for generated parts (surfaces of
  revolution and prisms, where nearest points are computed exactly) and for the
  freeform panel (to 0.001 mm); for shop parts it is OCCT's mesh at 0.001 mm (the
  README states the limit: a reader right where OCCT is wrong scores as wrong).
- Pass = checks pass and both distances <= **4x tolerance** ("OpenCascade's own mesh
  reaches 3.4 times its tolerance on `bowl_01` at 0.0125 mm"). > 60 s = fail.
  Distances are sampled at points (stated limitation).
- Timing only for passing runs (median of up to 5 runs / 10 s), split into
  reading/meshing, peak RSS via a relay process.

Oracle tiers: file facts are tier 5-7 (spec-derived from the file); exact two-way
distance on generated parts is tier 7 (sampled); shop parts tier 6 against an OCCT
model. Weakness: the 4x slack and point sampling (vertex + centroid) can miss
mid-edge excursions.

### Corpus

`corpus/generated/` (19 parts with `.truth.json`): `cylinder` (full-wrap face, no
seam edge, "how Fusion writes every cylinder"), `cylinder_seam` (control),
`cylinder_nurbs` (rational B-spline closed round, no seam), `cone_point` (apex with no
vertex), `cone_point_vertex` (control), `dome` (pole inside the face), `dome_nurbs`
(row of control points at the pole), `washer`, `torus_ring`, `torus_ring_nurbs`,
`cylinder_inch`, `sheet_disc` (3.6 m x 19 mm), `tiny_pin` (0.5 mm), `far_cylinder`
(100 m from origin), `cylinder_placed`, `cylinder_twice` (instanced, rotated),
`plate_100_holes`, `plate_900_holes` (910 faces, two faces holding 900 holes each;
1.4 MB), `grooved_shaft` (753 faces). `corpus/freeform_panel.step` +
`freeform_panel.truth.json`: cubic B-spline walls closed round with no seam edge,
**trimmed by fitted ring curves a few ten-thousandths of a mm off the walls**, as
real exports are. Shop parts: `hinge_jig`, `lego_4x2_01`, `door_handle_guide`,
`rail_guide`, `lego_3_pins`, `sled_base` (6 B-spline faces), `bowl_01` (revolution
with seam curves) — Fusion exports carry **no pcurves**, so a reader must find each
edge on its surface itself.

Reported findings (author's 2026-10-12 run, OCCT 7.8.1): every reader but OCCT fails
`cone_point`; OCCT puts surface 0.6 mm outside the freeform panel at 0.1 mm and
finer; OCCT 14.8 s / 1000 MB on `plate_900_holes` (48x the 100-hole time).

---

# Cross-project synthesis

## How each project defines tolerance

| Project | Linear tolerance means | Measured where | Achieved deviation tested? |
|---|---|---|---|
| OCCT | chordal deflection, absolute or relative per edge box; face deflection floored at 2x face tolerance | centroid + interior link midpoints; boundary links and pole triangles excluded (`BRepLib.cxx:2793-2889`) | one GTest, centroids only |
| Gmsh | element edge length target; curvature size `2*pi/(kappa*N)` | not measured | no |
| Netgen | curvature-driven h `1/(kappa*cs)`; bent-edge angle | not measured (one angular span test) | no |
| SMESH | per-hypothesis 1D deflection; `FT_Deflection2D` at centroid | centroid only, printed | no |
| FreeCAD | OCCT absolute from bbox `(dx+dy+dz)/300*Deviation`; angular cap `min(0.1, 5*lin+0.005)` | delegated to OCCT | no |
| CadQuery / build123d | OCCT **relative** (documented as absolute) | delegated | volume/area ±0.5 (build123d) |
| truck | chordal at **one jittered sample** per segment/cell; exact sagitta for sphere/torus/circle | one sample | mesh-to-OCCT-mesh at 10x |
| Fornjot | "maximum allowed deviation"; exact circle sagitta | exact for its curves | counts only |
| SolveSpace | midpoint chord, **capped by maxSegments**; display % of bbox vs export mm | midpoint (curves), 4 grid samples (surfaces); trim-band ears unrefined | no |
| usd-brep | chord height default 0 = **ignored**; angle 25 degrees | not measured | no |
| openNURBS | "approximate maximum distance from center of edge to surface" | edge midpoint | n/a (no public mesher) |
| SISL `s1613` | a-priori bound (b-a)^2 max abs(f'')/8 **per coordinate** | analytic bound, polynomial only | no |
| step-conformance | "no triangle strays farther than TOLERANCE from the true surface" | vertices + triangle centres, both directions, pass at 4x | yes (sampled) |

Consequence for our spec: deflection must be defined as the supremum over each
whole triangle (and each boundary polyline segment) of the distance to the true
surface (and curve), in world units. Each face's tolerance must be stated together
with how it treats the B-rep's own stored tolerance. Every project above
approximates this at sample points. Our offline oracle should measure it densely,
including boundary-adjacent and singular-point-adjacent triangles.

## (1) Union catalog of fault classes these projects learned to test

Columns: the class; which projects test it or recorded it in their trackers; the
strongest upstream oracle tier seen; what we need.

| # | Fault class | Seen in | Best upstream tier | What our oracle/corpus needs |
|---|---|---|---|---|
| **Termination and resources** |||||
| T1 | Refinement loop non-termination (edge amplification, surface control, 1D-intersection retry) | OCCT (28 hang cases), Gmsh #623/#3392, Netgen guards, SMESH Quadrangle loop guard | 1 (finishes under cpulimit) | geometry-derived loop bounds; cap reached -> refusal; wall/CPU bound per case |
| T2 | NaN defeating loop guards or parameter checks | Gmsh 229c75006, OCCT PR #1598, truck curve recursion (inferred) | none tested | inject NaN/inf at evaluator outputs, not only ingress |
| T3 | Runaway memory / superlinear output growth | OCCT bug23513/23631/26965, truck #89 (120 GB), step-conformance plate_900 (48x) | 3 (meminfo budget in 1 case) | peak-RSS budget per case; growth-rate scaling family |
| T4 | Recursion stack overflow | OCCT bug25806, GH #688 (TBB 1 MB stacks), SISL depth cap | 1 | iterative algorithms or bounded depth; small-stack runs |
| T5 | Edge discretization explosion from wrong curve range | Mantis 0028118 (90K points) | 2 | per-edge point budget derived from curve length/curvature |
| T6 | Triangle-count blow-up (tiny tolerance relative to size; bbox-derived deflection; clamps) | FreeCAD #17183/#32865, Mantis 0027693, 0030511 | 3 | output-size budget with explicit refusal; strip-propagation family |
| **Silent incompleteness** |||||
| S1 | Face left without triangulation, status OK | all projects (see finding 2) | 5-topo (OCCT tricheck) | per-face accounting: triangles or refusal record for every input face |
| S2 | Partial triangulation inside a face (ear-clip gives up, constraint dropped) | SolveSpace, truck, Fornjot #60/#430 | 5 (SolveSpace naked edges) | domain coverage + surface-to-mesh distance |
| S3 | Shapes dropped at export / instanced shapes without triangulation | CadQuery 3MF/glTF, FreeCAD exporter swallow | 2 | per-occurrence accounting in assemblies |
| **Topology of output** |||||
| W1 | Cracks between faces (shared-edge polygons differ) | OCCT tricheck cross-face, Mantis 0023837, Fornjot coincident non-siblings | 5-topo | shared-edge identity, not welded positions |
| W2 | Free triangles/islands, free nodes, duplicate nodes | OCCT bug30149, bug27239, bug31449 | 5-topo | per-face connectivity and node use |
| W3 | Weld failures (non-transitive comparator, rounding grid, float32) | FreeCAD #12744, build123d mesher | 2-3 | identity-based closure; float export displacement bound |
| W4 | Non-manifold edges/vertices, bow-ties | FreeCAD MeshEvalTopology/PointManifolds, openNURBS | 5 | edge use exactly 2 + single-cycle vertex links |
| W5 | Orientation / inverted triangles / inside-out near fillets | FreeCAD MeshEvalOrientation, usd-brep KNOWN_ISSUES, CadQuery #420 | 4 (usd-brep reversed-winding volume) | orientation vs face normal per triangle |
| W6 | Self-intersection, including adjacent folds and coplanar overlap | SolveSpace (blind in-plane), FreeCAD (skips adjacent) | 5 | offline exact/robust check incl. adjacent and coplanar pairs |
| W7 | Degenerate / zero-area / sliver triangles, apex triangles | OCCT bug25042, GH #1558, Fornjot #145, build123d STL | 5 (tricheck -small) | area and min-altitude floors; pole/apex fan policy |
| **Geometric fidelity** |||||
| G1 | Under-sampled deviation (midpoint, one sample, centroid only) | OCCT 24127/29962/33691, truck, openNURBS, SolveSpace | 5 (one GTest) | dense sampling incl. edges and interiors |
| G2 | Tolerance silently overridden (B-rep tolerance floor, segment cap, MinSize) | OCCT bug25287/31251, SolveSpace maxSegments, openNURBS MinimumEdgeLength | 3 (goldens freeze violations) | any override reported, never silent |
| G3 | Knot/seed sampling misses interior curvature | OCCT 0032241, bug29962 | 3 | spikes/bumps family between knots |
| G4 | Relative vs absolute tolerance confusion | CadQuery, build123d, FreeCAD Deflection keyword | none | one documented unit; tests at several scales |
| G5 | Planar faces ignore quality controls / analytic surfaces skip control | usd-brep (parameter plumbing visible only on curved faces), OCCT `-surf_def_all` | 3 | exercise every surface type at each parameter |
| **Parametric pathologies** |||||
| P1 | Poles and apexes (incl. apex without vertex, pole inside face) | OCCT, Netgen, Gmsh, truck, step-conformance cone_point/dome | 3 | analytic truth at singular points |
| P2 | Seams: missing seam edge (closed face bounded by two circles), duplicated seam nodes, seam pcurve misclassification | step-conformance cylinder/dome_nurbs, GH #1558, #572 | 5 (step-conformance) | no-seam-edge family; seam identity checks |
| P3 | Trim loops crossing the periodic seam; multi-turn faces; shifted periodic ranges | Gmsh CylinderInnerLoopCrossingSeam, OCCT GH #1337, usd-brep cylinder_two_turn/shifted_period | 3 | seam-crossing trim family |
| P4 | Self-intersecting discretized wires in UV (coarse angle, thin rings, narrow notches) | OCCT (Mantis 0024084 open since 2013), Gmsh #1242 | 2 | thin-ring and notch families; must refine, not drop |
| P5 | Unconstrained-Delaunay leakage / constraint loss in concave domains | Fornjot sharp_concave, truck constraint skip | 7 (Fornjot exact triangles, brittle) | complete-domain coverage oracle (already designed) |
| P6 | UV tolerance for pcurve endpoint gaps | Gmsh escalating `fact[6]`, Netgen AlignGeomInfo | 2 | gap family with known 3D/UV disagreement |
| **Input pathologies** |||||
| I1 | B-rep tolerances far above nominal | Gmsh A319 (6.77), component8, tank; OCCT bug25287 | 3 | see gap 1 |
| I2 | Edges/trims not on their surface; missing pcurves | truck robust_closed, step-conformance freeform_panel and Fusion parts, usd-brep plane_inconsistent_pcurves | 6 (step-conformance) | see gap 1 |
| I3 | Zero-length edges, tiny edges, near-coincident vertices | OCCT bug30234/33060, Gmsh #1107, Fornjot #494 | 7 (OCCT bug30234) | in plan |
| I4 | Thin/crescent/narrow faces, slivers beside huge faces | OCCT bug31199/25817, Gmsh capot | 3 | in plan (tiny features) |
| I5 | Infinite/untrimmed faces, empty compounds, faces without loops | OCCT bug32692_3/bug31125, FreeCAD Tools.cpp, usd-brep loopless | 7 (REQUIRED status) | explicit refusal fixtures |
| I6 | Invalid-but-used shapes (unorientable, self-folding sweeps, intersecting solids) | FreeCAD unorientableSolid, build123d #855, Gmsh bad_step_intersects | 2 | policy decision (gap 10) |
| I7 | Structural B-rep invalidity (radial chains, seam pairing, loop nesting, NURBS knots/weights) | usd-brep 191 rules, openNURBS IsValid | 5 (negative fixtures) | ingress refusal catalog |
| I8 | Rational weight conventions, zero/negative weights | SISL issue #7, openNURBS curve weights | none | weight-convention fixtures |
| I9 | Units and placement (inch, metres, mirrored/scaled instances, far from origin) | OCCT bug25837, step-conformance cylinder_inch/far_cylinder/cylinder_twice, truck units | 6 | in plan partly (scale ratio) |
| **Determinism and state** |||||
| D1 | Thread-count / platform nondeterminism | OCCT bug29751/32241, Netgen filecmp, usd-brep 4x8 bitwise, Gmsh #3278 | 4 | validity invariant across threads/backends |
| D2 | Data races on shared sub-shapes keyed by located identity | Mantis 0023260 | 2 | instanced assemblies under parallel meshing |
| D3 | Memory-address-dependent choices | build123d #1492 | 3 | run-to-run identity of topology decisions |
| D4 | Cross-shape state corruption; stale mesh reuse; tessellation hysteresis | OCCT bug25469/25503, FreeCAD `reset`, CadQuery/build123d reuse | 4 | fresh vs re-mesh equivalence |
| D5 | Static/global state in recursion | SISL issue #67, Gmsh global options | 2 | concurrent independent jobs |
| **Harness failure modes** |||||
| H1 | Missing fixture counted as skip or success | OCCT SKIPPED rule, usd-brep LFS SKIP | — | missing fixture = failure |
| H2 | Assertions compiled out in release | usd-brep `SM_ASSERT` | — | oracles independent of build flags |
| H3 | Goldens that freeze defects | OCCT V7 (78% area error), bug29962, SolveSpace mesh-copied volumes, OCCT bug25307 REQUIRED defect | — | goldens never substitute for a bound |
| H4 | Permanent TODO / IMPROVEMENT churn hiding live failures | OCCT bug25586_*, tests/mesh/end TODO generator | — | refusals and failures retained, never exempted by pattern |

## (2) Cases and shapes worth adopting

Paths are relative to `quarantine/sources/` unless they begin with `new-iteration/`
(meaning `quarantine/new-iteration/`). Fixtures in third-party formats (`.brep`,
`.stp`, `.igs`, `.3dm`, `.smb`, `.slvs`) need our STEP reader or an explicit
converter, and any converter step must be visible.

### First cohort: exact truth available (tier 7 possible)

| Path | Fault class |
|---|---|
| `step-conformance/corpus/generated/{cylinder, cylinder_seam, cylinder_nurbs}.step` + `.truth.json` | full-wrap face with no seam edge vs control; rational exact cylinder |
| `step-conformance/corpus/generated/{cone_point, cone_point_vertex}.step` | apex without and with a vertex (every non-OCCT reader failed) |
| `step-conformance/corpus/generated/{dome, dome_nurbs, torus_ring, torus_ring_nurbs, washer}.step` | pole inside face, row of control points at pole, doubly curved, inward cylinder normal |
| `step-conformance/corpus/generated/{cylinder_inch, sheet_disc, tiny_pin, far_cylinder, cylinder_placed, cylinder_twice}.step` | units, 3.6 m x 19 mm aspect, 0.5 mm part, 100 m offset, placement, instancing |
| `step-conformance/corpus/generated/{plate_100_holes, plate_900_holes, grooved_shaft}.step` | scaling (hole count, 753 full-wrap faces) |
| `step-conformance/corpus/freeform_panel.step` + `.truth.json` | trims fitted 1e-4..1e-3 off closed seamless B-spline walls |
| `gmsh/benchmarks/statreport/{Cone_1.brep, Torus_1.brep, Cylinder_1.brep}` | single-face analytic primitives (analytic area) |
| SMESH `HexahedronCanonicalShapesTest.cxx:148-232` recipes | box, cylinder, sphere, cone-to-apex, truncated cone |
| OCCT inline `bug30234` (zero-radius circle), `bug32692_3` (infinite cylinder face), `bug31125` (empty compound), `bug32471` (planar deflection = 0) | spec-derived refusal or exact answers |

### Second cohort: hard real inputs (deviation against exact carriers, plus topology and termination)

| Path | Fault class |
|---|---|
| `occt-dataset-7.9.0/brep/bug24938_27773.brep`, `brep/bug31144.brep`, `brep/bug32767.brep`, `brep/bug26965_mesh_hang.brep`, `brep/bug23650_slowmesh.brep`, `step/bug29715_slow.stp` | historical hangs and slow cases |
| `occt-dataset-7.9.0/brep/bug31865.brep` | large deflection -> 0 triangles |
| `occt-dataset-7.9.0/brep/bug25586_{circle_extrusion, cylindrical_with_circle_hole, quasi_closed}.brep` | closed/periodic faces with holes, narrow notch |
| `occt-dataset-7.9.0/step/bug27693_cornetta-step.stp` | U/V strip over-refinement |
| `occt-dataset-7.9.0/step/bug30149_comb_bug.step`, `brep/bug28719_display_issue.brep`, `step/bug25045_ick_0433.stp`, `brep/bug27845_shape.brep` | free triangles, untriangulated faces |
| `occt-dataset-7.9.0/brep/bug30511_hugeassembly0.brep` | ~93k shared instances, 1.12M triangles |
| `occt-dataset-7.9.0/iges/bug25837_S.igs` | IGES in metres |
| `occt-dataset-7.9.0/brep/bug25044_face{1..58}.brep`, `brep/1.stp_face{NNN}.brep` (32) | 90 heterogeneous single faces: cheap breadth |
| `occt-dataset-7.9.0/brep/shading_wrongshape_*.brep` (7 present) | deliberately invalid shapes: refusal or explicit partial |
| `gmsh/benchmarks/occ/{CylinderInnerLoopCrossingSeam, PeriodicFaceWithLargeTolerance, unable_to_recover_edge_2d, unable_to_recover_edge_2d-2, full_sphere, TrimmedSurface}.brep` | seam-crossing inner loop, UV tolerance, unrecoverable boundary, 4.38 vertex tolerance, duplicate pole edges |
| `gmsh/benchmarks/brep/{A319.brep.gz, tank.brep, capot.brep, problemSphere.brep}` | tolerances 6.77 / 0.12 / 0.08; 77 small spheres (77 poles) |
| `gmsh/benchmarks/step/{component8.step.gz, component4.step.gz, bad_step_intersects.step.gz, screw.step.gz, periodic_u_cst.step, tough_second_order.step.gz, io1-pe-203.step.gz}` | 0.074 declared tolerance; intersecting solids; tori/cones/seams; periodic B-spline; 929 faces; old Pro/E schema |
| `gmsh/benchmarks/iges/{cube1000.igs, model1.igs}`, `gmsh/benchmarks/step/sewfaces.step` | unsewn faces (refuse or report open, never invent closure) |
| `gmsh/benchmarks/statreport/{15_cylinder_head.brep, Top.brep}` | 1054 faces; 71 tori |
| `netgen/tutorials/screw.step`, `netgen/tutorials/frame.step` | helical trims (coarse-size failure); 375-face Pro/E part |
| `resources4truck/step/abc-{0000,0006,0008,0035}.step`, `resources4truck/step/occt-{cone,cylinder,sphere,torus}.step` | ABC parts; another kernel's analytic seams/poles |
| `usd-brep/TestFiles/occt_breps/*.brep` (29: seams with/without pcurves, two-turn cylinder, inconsistent/sparse pcurves, plate with hole, periodic B-spline, conics) | pcurve and seam representation |
| `usd-brep/TestFiles/brep_validator/Test_BA_*.usda` (107) and `coverage_limits/*.usda` | ingress refusal negatives |
| `usd-brep/TestFiles/usd_TestFiles/*.smb` non-manifold set, `RevitWallHealerCrash.usda` | non-manifold, sheet, void bodies; foreign exporter |
| `freecad/tests/src/Mod/Part/App/brepfiles/{unorientableSolid, helix1, cylinder1}.brep` | invalid-but-used solid; helical sweep |
| `new-iteration/opennurbs/example_files/V4/v4_Wheel_PG.3dm`, `V5/v5_disk_brake.3dm`, `V5/v5_clip.3dm`, `V1/v1_T-Joint{1,2}.3dm` | product-scale trimmed NURBS (needs a 3dm reader; optional) |
| `solvespace/test/group/boolean_*/normal.slvs` (6) | Boolean outputs with tangent/knife-edge configurations (re-express as B-rep) |
| `build123d/tests/cyl_w_rect_hole.stl` | 46 zero-area slivers: weld/validator negative input |

### Recipes to author (fixtures absent or inline in upstream tests)

- **OCCT `bug29962`**: an 8x8 B-spline with two z=999 poles. Tests interior spikes between seeds.
- **OCCT `bug32982_2`**: scale invariance under `tscale 0.001` with scaled deflection, as a metamorphic oracle.
- **OCCT GH #1337**: a box fused with a 2*pi torus ring whose trims cross the seam.
- **OCCT GH #1558**: a helical pipe that produces coincident seam nodes.
- **OCCT GH #572**: cone seam pcurves as infinite lines.
- **Recreations of the absent hang/OOM fixtures:**
  - `bug23513`: an infinite face that runs out of memory.
  - `bug31199`: thin faces.
  - `bug25281`: an infinite-loop STEP.
- **truck `robust_closed`**: cube edges bowed off the face surfaces.
- **truck `special_cylinder`**: a cylinder split into two seam halves.
- **Fornjot negative controls**: a missing face, a duplicated curve, and a reversed interior cycle.
- **SISL issue #7**: a premultiplied vs non-premultiplied weight circle.
- **usd-brep periodic span**: `2*pi ± 2e-6`.
- **Netgen `test_occ.py:57-63`**: a box with a 0.02-radius through hole.
- **CadQuery 1e-9 face**: must be refused explicitly, not returned as an empty mesh.

## (3) Gaps: fault classes our bootstrap plan would be blind to, ranked by risk

Each gap is checked against the bootstrap as stated: ABC sample and sweep,
NIST/CAx-IF, OCCT regression shapes, the authored families (poles, seams, tiny
features, near-tangent trims, wild weights, periodic surfaces, zero-length edges,
tolerance coarser than features, 1e6:1 scale), the STEP/parameter mutation fuzzer and
the watch. Its oracles are dense deviation, watertight/manifold/orientation/
self-intersection/sliver checks, termination, and correct output or explicit refusal.

1. **Inexact B-reps: stored tolerances above the requested deflection, edges and
   trims off their surfaces, missing or lying pcurves.**
   - Risk: highest. This is how real CAD exports look:
     - Gmsh A319 6.77, component8 0.074;
     - step-conformance freeform_panel and Fusion parts with no pcurves;
     - Mantis 0023837;
     - truck #136, where the result was 12-30 mm off.
   - Why the bootstrap misses it:
     - ABC filtered out healing-needing models.
     - "Tolerance coarser than features" concerns the requested tolerance, not the
       B-rep's own.
     - The dense-deviation oracle is ill-defined when edge and surface disagree.
   - Needs:
     - a spec decision on which carrier "exact geometry" means (surface, edge curve,
       or both within stated tolerance);
     - how shared edges stay crack-free when the two faces' surfaces disagree by
       more than the deflection;
     - authored families with controlled edge-to-surface offsets 1e-6..1e-2 and
       pcurve-free variants.
2. **Silent face loss on non-closed outputs, and missing surface.**
   - Watertightness catches a dropped face only on closed solids.
   - Mesh-to-surface deviation does not catch missing surface at all.
   - Needs:
     - per-face accounting (every input face has triangles or a refusal record);
     - the surface-to-mesh distance direction;
     - sheet bodies, open shells and assemblies of sheets in the corpus.
3. **Derived non-finite values inside the pipeline.**
   - Ingress refusal will not catch NaN produced from finite inputs:
     - degenerate frames (truck #128: `ref_direction` parallel to axis);
     - 0/0 in progress ratios (Gmsh 229c75006);
     - Newton steps at poles;
     - acos domain errors (Fornjot tol > 2r).
   - Needs: fault injection of NaN/inf at evaluator, projection and size-function
     outputs, with the termination oracle and the refusal oracle both armed. The
     STEP mutation fuzzer reaches these only by luck.
4. **Concurrency, backend and run-to-run validity; shared instances under
   parallelism.**
   - The plan has no oracle that the same input stays valid across thread counts,
     CPU/GPU backends and repeated runs:
     - functionally identical is the bar, per the operator;
     - OCCT 0023260, bug29751; Gmsh #3278; build123d #1492; SISL #67.
   - Nothing in the plan covers instanced assemblies where one shape occurs under
     several placements and is meshed concurrently (OCCT bug30511, CadQuery
     boxes2, step-conformance `cylinder_twice`).
   - Needs a metamorphic oracle: validity and deviation bounds hold for 1..N
     threads and every backend, and topology decisions are run-invariant.
5. **Oracle blind spots copied from upstream.**
   - Each listed oracle has a known upstream failure mode:
     - self-intersection that skips adjacent and coplanar pairs;
     - closedness by position-weld;
     - `isnan` without `isinf`;
     - an early-exit topology scan;
     - deviation sampled at centroids and interior midpoints only;
     - missing fixtures counted as skips.
   - Needs: each oracle specified against the specific degenerate configurations
     it must flag (adjacent fold, coplanar overlap, T-junction, position-coincident
     but identity-distinct vertices, inf coordinate, single missing face,
     boundary-segment excursion, pole-adjacent excursion). Fornjot-style negative
     controls are that oracle's own direct check, not a second-order system.
   - A missing fixture is a failure.
6. **Scale and complexity cliffs in time, memory and output size.**
   - The ABC sample is small parts. The plan has no growth-rate check. Upstream
     evidence:
     - plate_900_holes is 48x slower for 9x the size, at 1000 MB in OCCT;
     - Mantis 0027693 strip propagation;
     - FreeCAD's "billions of triangles";
     - OCCT 93k instances.
   - Needs:
     - scaling families (N holes, N grooves, N instances) with a reported growth
       exponent;
     - a peak-RSS bound;
     - an output-size budget that refuses explicitly when a requested tolerance
       implies an absurd triangle count.
7. **Legal-but-unusual periodic topology beyond generic "seams and poles".**
   - The specific variants that broke mature readers:
     - a closed face bounded by two circles with no seam edge;
     - an apex with no vertex;
     - a pole inside a face's interior;
     - trim loops crossing the seam (Gmsh `CylinderInnerLoopCrossingSeam`, OCCT
       GH #1337);
     - multi-turn faces;
     - shifted periodic ranges (usd-brep `coverage_limits`);
     - seam pcurves given as infinite lines (GH #572);
     - INTERNAL-oriented seam faces (GH #1352).
   - Needs explicit families rather than reliance on ABC frequency.
8. **Units, placement and absolute coordinate magnitude; export precision.**
   - The 1e6:1 scale ratio covers relative size but not:
     - parts 100 m from the origin;
     - inch/metre files (OCCT bug25837, `cylinder_inch`);
     - mirrored or scaled placements;
     - float32 narrowing at export (FreeCAD, build123d).
   - Needs: placement/units families, and an export-displacement bound measured on
     the actual written bytes (already in our earlier meshing-oracle study).
9. **Re-meshing state: stale reuse, hysteresis, incremental remeshing.**
   - Upstream bugs:
     - OCCT bug25469/25503;
     - FreeCAD `tessellate(reset)`;
     - CadQuery/build123d reuse checks that compare relative against absolute.
   - The operator's roadmap includes incremental remeshing of changed parts, and
     the plan has no fresh-versus-incremental equivalence oracle.
10. **Policy for invalid-but-used inputs.**
    - The cases:
      - unorientable solids that downstream documents depend on (FreeCAD);
      - self-folding sweeps (build123d #855);
      - intersecting solids in one file (Gmsh `bad_step_intersects`);
      - non-manifold bodies (usd-brep set).
    - Our self-intersection oracle would flag a faithful tessellation of a
      self-intersecting input, so the spec must say "faithful plus reported" or
      "refuse". This joins the open garbage-face question in the journal.
11. **Rational-weight conventions and zero/negative weights.**
    - "Wild rational weights" covers magnitudes. It does not cover:
      - premultiplied vs projected control points (SISL #7);
      - negative weights of consistent sign (accepted by openNURBS curves);
      - zero weights at full-multiplicity knots;
      - surfaces whose weights no validator checks (openNURBS).
    - Needs explicit ingress refusal or support fixtures.

The following are already covered by the plan and need no new family; this table
records which upstream evidence supports each:

| Already in the plan | Upstream evidence |
|---|---|
| zero-length edges | OCCT bug30234 |
| tiny features beside large faces | Netgen thin hole, OCCT bug31199 |
| tolerance coarser than features | Netgen screw coarse failure |
| near-tangent trims | SolveSpace tangent Booleans |
| poles/seams at a generic level | covered above |

# Appendix A: acquisitions (rows for papers/reference-catalog.tsv)

Not merged into `papers/reference-catalog.tsv` by this lane. Columns match that file.
Acquired 2026-10-17. `quarantine/archives` and `quarantine/sources` are this
workspace's links into the shared collection. Every ZIP was extracted with
`scripts/ingest_zip.py --skip-symlinks` into a new destination. The OCCT dataset is
a `.tar.xz`, so it was extracted with `tar -xJf ... --strip-components 1` into a new
directory instead. Its listing showed no absolute or `..` paths and no
`.git`/`.DS_Store`/`__MACOSX`/`._*` entries. The extraction holds 3,388 regular
files and no links or special files.

```tsv
name	repository	revision	archive	sha256	destination	archive_root	retained_files	omissions
gmsh	https://gitlab.onelab.info/gmsh/gmsh (ZIP from mirror https://github.com/live-clones/gmsh)	b4e84a43d02f97bc50ef37ed21e5e97096d072be	quarantine/archives/gmsh-b4e84a43d02f.zip	c4c0803823caca87cd8ffc3d86d4780cdf3df3ebd62461392663cb011df6ea56	quarantine/sources/gmsh	gmsh-b4e84a43d02f97bc50ef37ed21e5e97096d072be	4006	gmsh-b4e84a43d02f97bc50ef37ed21e5e97096d072be/doc/news/index.html (symlink)
netgen	https://github.com/NGSolve/netgen	31e85f811fd64b163a4b4d9a1dc587115a7f3133	quarantine/archives/netgen-31e85f811fd6.zip	30e6799098dc1310431eec10422a71e86cd9521d4f4b3253831b1962f11994fb	quarantine/sources/netgen	netgen-31e85f811fd64b163a4b4d9a1dc587115a7f3133	697	submodule external_dependencies/pybind11 not in GitHub ZIP
fornjot	https://github.com/hannobraun/fornjot	7e856686b8105d21ce4673c10622f6ef206b89af	quarantine/archives/fornjot-7e856686b810.zip	9b214b40a24db7859dc47e801e82d1fceaf28bb459c826ef6fd904a0e4156174	quarantine/sources/fornjot	fornjot-7e856686b8105d21ce4673c10622f6ef206b89af	557	fornjot-7e856686b8105d21ce4673c10622f6ef206b89af/tools/export-validator/lib3mf/libs/lib3mf.dylib (symlink) | fornjot-7e856686b8105d21ce4673c10622f6ef206b89af/tools/export-validator/lib3mf/libs/lib3mf.so (symlink)
smesh	https://github.com/SalomePlatform/smesh (mirror; git.salome-platform.org TLS failure)	83f8bf48211b922fb47623be7a74746a87f97d74	quarantine/archives/smesh-83f8bf48211b.zip	e12f9de305d5799efc3156edc9c8c8a84b2d93e6321aebc5e0e8f2bb416ebb69	quarantine/sources/smesh	smesh-83f8bf48211b922fb47623be7a74746a87f97d74	2838	
freecad	https://github.com/FreeCAD/FreeCAD	cc8a5eb0dc813ab2b89479c72a9ee2159d31a475	quarantine/archives/freecad-cc8a5eb0dc81.zip	07ec6b4c9f79d05c66279c53ad7bae77b17dc154ea03cfca0210c3abf7b89455	quarantine/sources/freecad	FreeCAD-cc8a5eb0dc813ab2b89479c72a9ee2159d31a475	13294	submodules OndselSolver, GSL, AddonManager, coin, pivy not in GitHub ZIP
resources4truck	https://github.com/ricosjp/resources4truck	9bf9de1426c5fc2f2b8d63f501dca0d3c53ebd91	quarantine/archives/resources4truck-9bf9de1426c5.zip	c2c604a89ca93f602c100cb3ea71a2bdaad53534c21f74bcf84b0eeabbf8087f	quarantine/sources/resources4truck	resources4truck-9bf9de1426c5fc2f2b8d63f501dca0d3c53ebd91	36	
step-conformance	https://github.com/dan335/step-conformance	96f30bbe09549ef37dbd0a956d70bd8a183fd5d2	quarantine/archives/step-conformance-96f30bbe0954.zip	5fe0c3a335a19f51b5cd62fee03fa607041e2010f06938c934ae908e6968b1f7	quarantine/sources/step-conformance	step-conformance-96f30bbe09549ef37dbd0a956d70bd8a183fd5d2	82	
occt-dataset-7.9.0	https://github.com/Open-Cascade-SAS/OCCT/releases/download/V7_9_0_beta1/opencascade-dataset-7.9.0.tar.xz	release asset V7_9_0_beta1 (fetched by OCCT 8.0.1 CI, .github/actions/run-tests/action.yml:55-56)	quarantine/archives/opencascade-dataset-7.9.0.tar.xz	a92ed91c3271c299287c1c404bb9454d463251094bfc848acc44aee60e6a026c	quarantine/sources/occt-dataset-7.9.0	opencascade-dataset-7.9.0	3388	tar extraction, --strip-components 1; no detritus found
```

Notes:
- `resources4truck` is the revision that truck `88ed005249e5` pins as its
  `resources` submodule, per the GitHub contents API.
- The revision recorded for gmsh equals gitlab.onelab.info HEAD by `git ls-remote`.
  The GitLab archive endpoint redirected to sign-in, and its API returned 403.
- Archive sizes in bytes:

  | Archive | Bytes |
  |---|---|
  | gmsh | 36,648,729 |
  | netgen | 3,708,500 |
  | fornjot | 7,349,453 |
  | smesh | 30,712,538 |
  | freecad | 91,274,029 |
  | resources4truck | 16,142,437 |
  | step-conformance | 888,465 |
  | dataset | 98,739,184 |

- Licences:

  | Reference | Licence |
  |---|---|
  | Gmsh | GPL-2.0-or-later with exceptions (`LICENSE.txt`) |
  | Netgen | LGPL-2.1 |
  | SMESH | LGPL-2.1 |
  | FreeCAD | LGPL-2.1 |
  | Fornjot | 0BSD (`LICENSE.md`) |
  | step-conformance code | MIT/Apache-2.0 |
  | step-conformance corpus | CC0 1.0 (`corpus/LICENSE`) |
  | resources4truck ABC STEP files | MIT, per its `Readme.md` |
  | resources4truck bundled Sketchfab OBJ models | CC-BY |

  The OCCT dataset fixtures carry no per-file licence statement; treat them as
  OCCT test material.
- These are reference and corpus material only. No dependency is selected and no
  code is copied.

Restoration, from the repository root (destinations must be new; compare each
printed SHA-256 with the row above before extracting):

```sh
mkdir -p quarantine/archives quarantine/sources
curl -fL https://codeload.github.com/live-clones/gmsh/zip/b4e84a43d02f97bc50ef37ed21e5e97096d072be -o quarantine/archives/gmsh-b4e84a43d02f.zip
curl -fL https://codeload.github.com/NGSolve/netgen/zip/31e85f811fd64b163a4b4d9a1dc587115a7f3133 -o quarantine/archives/netgen-31e85f811fd6.zip
curl -fL https://codeload.github.com/hannobraun/fornjot/zip/7e856686b8105d21ce4673c10622f6ef206b89af -o quarantine/archives/fornjot-7e856686b810.zip
curl -fL https://codeload.github.com/SalomePlatform/smesh/zip/83f8bf48211b922fb47623be7a74746a87f97d74 -o quarantine/archives/smesh-83f8bf48211b.zip
curl -fL https://codeload.github.com/FreeCAD/FreeCAD/zip/cc8a5eb0dc813ab2b89479c72a9ee2159d31a475 -o quarantine/archives/freecad-cc8a5eb0dc81.zip
curl -fL https://codeload.github.com/ricosjp/resources4truck/zip/9bf9de1426c5fc2f2b8d63f501dca0d3c53ebd91 -o quarantine/archives/resources4truck-9bf9de1426c5.zip
curl -fL https://codeload.github.com/dan335/step-conformance/zip/96f30bbe09549ef37dbd0a956d70bd8a183fd5d2 -o quarantine/archives/step-conformance-96f30bbe0954.zip
curl -fL https://github.com/Open-Cascade-SAS/OCCT/releases/download/V7_9_0_beta1/opencascade-dataset-7.9.0.tar.xz -o quarantine/archives/opencascade-dataset-7.9.0.tar.xz
shasum -a 256 quarantine/archives/{gmsh-b4e84a43d02f,netgen-31e85f811fd6,fornjot-7e856686b810,smesh-83f8bf48211b,freecad-cc8a5eb0dc81,resources4truck-9bf9de1426c5,step-conformance-96f30bbe0954}.zip quarantine/archives/opencascade-dataset-7.9.0.tar.xz
python3 -I scripts/ingest_zip.py quarantine/archives/gmsh-b4e84a43d02f.zip quarantine/sources/gmsh --root gmsh-b4e84a43d02f97bc50ef37ed21e5e97096d072be --skip-symlinks
python3 -I scripts/ingest_zip.py quarantine/archives/netgen-31e85f811fd6.zip quarantine/sources/netgen --root netgen-31e85f811fd64b163a4b4d9a1dc587115a7f3133 --skip-symlinks
python3 -I scripts/ingest_zip.py quarantine/archives/fornjot-7e856686b810.zip quarantine/sources/fornjot --root fornjot-7e856686b8105d21ce4673c10622f6ef206b89af --skip-symlinks
python3 -I scripts/ingest_zip.py quarantine/archives/smesh-83f8bf48211b.zip quarantine/sources/smesh --root smesh-83f8bf48211b922fb47623be7a74746a87f97d74 --skip-symlinks
python3 -I scripts/ingest_zip.py quarantine/archives/freecad-cc8a5eb0dc81.zip quarantine/sources/freecad --root FreeCAD-cc8a5eb0dc813ab2b89479c72a9ee2159d31a475 --skip-symlinks
python3 -I scripts/ingest_zip.py quarantine/archives/resources4truck-9bf9de1426c5.zip quarantine/sources/resources4truck --root resources4truck-9bf9de1426c5fc2f2b8d63f501dca0d3c53ebd91 --skip-symlinks
python3 -I scripts/ingest_zip.py quarantine/archives/step-conformance-96f30bbe0954.zip quarantine/sources/step-conformance --root step-conformance-96f30bbe09549ef37dbd0a956d70bd8a183fd5d2 --skip-symlinks
mkdir quarantine/sources/occt-dataset-7.9.0
tar -xJf quarantine/archives/opencascade-dataset-7.9.0.tar.xz -C quarantine/sources/occt-dataset-7.9.0 --strip-components 1
```

# Appendix B: OCCT bugs/perf mesh fixtures present in the public dataset

The table excludes the `bug25044_face*` and `1.stp_face*` series, which are all
present. Paths are relative to `quarantine/sources/occt-dataset-7.9.0`. Of the 230
bugs/mesh and 15 perf/mesh cases, only those below plus the two series can run
against public data.

| Case | Fixture |
|---|---|
| bugs/mesh/bug23184_1 | brep/bug23184_tessface2.brep |
| bugs/mesh/bug23184_2 | brep/bug23184_tessface.brep |
| bugs/mesh/bug24938 | brep/bug24938_27773.brep |
| bugs/mesh/bug25045 | step/bug25045_ick_0433.stp |
| bugs/mesh/bug25586_1 | brep/bug25586_circle_extrusion.brep |
| bugs/mesh/bug25586_2 | brep/bug25586_cylindrical_with_circle_hole.brep |
| bugs/mesh/bug25586_3 | brep/bug25586_quasi_closed.brep |
| bugs/mesh/bug25594 | brep/bug25594_face1.brep |
| bugs/mesh/bug25837_1 | iges/bug25837_S.igs |
| bugs/mesh/bug25837_2 | iges/bug25837_S.igs |
| bugs/mesh/bug26372 | brep/bug26372_RP.brep |
| bugs/mesh/bug26532 | brep/bug26533_aal2.brep |
| bugs/mesh/bug26928 | brep/bug27356_comp.brep |
| bugs/mesh/bug27239 | brep/bug27239_f1.brep |
| bugs/mesh/bug27453 | step/bug27453_165-174_piattello.stp |
| bugs/mesh/bug27693 | step/bug27693_cornetta-step.stp |
| bugs/mesh/bug27845 | brep/bug27845_shape.brep |
| bugs/mesh/bug28500 | brep/bug28500_shape_mesh_artifact.brep |
| bugs/mesh/bug28719 | brep/bug28719_display_issue.brep |
| bugs/mesh/bug29641 | brep/bug29641.brep |
| bugs/mesh/bug29715 | step/bug29715_slow.stp |
| bugs/mesh/bug30149 | step/bug30149_comb_bug.step |
| bugs/mesh/bug30442 | others/bug30442_points.txt |
| bugs/mesh/bug31144 | brep/bug31144.brep |
| bugs/mesh/bug31258 | brep/bug31258.brep |
| bugs/mesh/bug31449_2 | brep/bug31449_face_loopback_M.brep |
| bugs/mesh/bug31865 | brep/bug31865.brep |
| bugs/mesh/bug32053 | brep/bug32053_prob_face.brep |
| bugs/mesh/bug32471 | brep/bug32471.brep |
| bugs/mesh/bug32692_2 | brep/bug32692_broken_mesh.brep |
| bugs/mesh/bug32767 | brep/bug32767.brep |
| perf/mesh/bug23650 | brep/bug23650_slowmesh.brep |
| perf/mesh/bug23795 | brep/bug23795_s.brep |
| perf/mesh/bug25264 | brep/bug25264_wire.brep |
| perf/mesh/bug26965 | brep/bug26965_mesh_hang.brep |
| perf/mesh/bug30511 | brep/bug30511_hugeassembly0.brep |

# Appendix C: wanted but not acquired

- **BRL-CAD** (`libbrep/cdt`, an owned B-rep CDT tessellator; docs at
  brlcad.org/docs/api, `ON_Brep_CDT_*`):
  - `github.com/BRL-CAD/brlcad` returned 404 on the API, git and web on 2026-10-17.
  - `git.code.sf.net/p/brlcad/code` was not found.
  - The only reachable copy is the third-party mirror `gitverse.ru/geoceti/brlcad`
    (HEAD `faa869a7481564a4f7c19553a8aaaff197d5366f`). It was not used because its
    provenance is unverified.
  - The operator should decide whether to acquire it from an official source.
- **Epic CADKernel** (Unreal Engine Datasmith, an owned B-rep tessellator): source
  access requires an Epic account. It is not public.
- **SALOME `DATA_DIR` sample shapes** (`Shapes/Brep/flight_solid.brep`, `slots.brep`)
  referenced by SMESH tests: no public location was confirmed.
- **Fixtures absent from the public OCCT dataset:** the hang/OOM/thin-face set named
  in the OCCT section, and the eight tracker-named fixtures. Mantis attachments are
  not in the public archive build.
- **Tracker attachments:** the OCCT/FreeCAD/truck/Gmsh items listed in the tracker
  section were not downloaded, because their licences are unstated.
- **Rhino's mesher and McNeel's test harness:** these are in the private
  `opennurbs_plus` layer.
