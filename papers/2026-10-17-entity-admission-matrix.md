# Entity admission matrix: what "arbitrary B-rep input" means for the mesher

Research lane M4-G5 of the fast-mesher campaign, 2026-10-17. Briefs:
[M4-common](../docs/fast-mesher-campaign/briefs/M4-common.md) and
[M4-G5](../docs/fast-mesher-campaign/briefs/M4-G5.md); bead `brepgpu-12s.6`;
stub: lifecycle map §4.1 G5 ([geometry-lifecycle-map](2026-10-17-geometry-lifecycle-map.md)).
Sources and read depth: [2026-10-17-entity-admission-matrix-sources.tsv](2026-10-17-entity-admission-matrix-sources.tsv).

What ran: a text-level Part 21 census and an OCCT 8.0.1 translation census over
839 unique STEP files on the fleet host, in the lane directory
`lanes/M4-G5/` under the BRepGPU root. No mesher, Boolean or NVIDIA code ran.
Nothing geometric was built or run on the laptop; laptop work was reading,
PDF text extraction and editing these two files. One
reading sub-lane studied the reference kernels (OCCT, truck, NVIDIA headers,
vcad); its claims that this paper relies on were spot-checked against the code
(§3.3).

Tags: **[src]** a source establishes it; **[code]** read at file:line at the
stated revision; **[meas]** measured by this lane's census; **[own]** this lane's
derivation or inference; **[call]** a design call, open to override.

## 0. The answer

Frequencies are files containing the kind and instances of it, deduplicated by
SHA-256, split into two strata that must not be pooled:

- **exports**: 506 files from NIST MBE PMI (49), the CAx-IF public library (53),
  five open-hardware packages (376), step-conformance (27) and the OM10 watch (1).
  These are 240,497 faces and 642,569 edges written by production CAD exporters.
- **OCCT dataset**: 333 files from OCCT's 7.9.0 test dataset. These are 22,606
  faces and 53,045 edges, mostly bug reproductions, so they over-represent
  pathology by design.

ABC and Fusion 360 Gallery are not acquired (operator decisions pending; corpus
manifest §1-2). ABC's own Figure 3 puts about 15% of its patches in an
unexplained "Other" bar [src Koch 2019 §3.3, Fig. 3], larger than its B-spline
share, so ABC is the largest unknown left in this matrix (§2.4).

### 0.1 The matrix, most frequent kinds first

"Today" is the current ingress at HEAD 627163b (`native/step_source/main.cpp`)
plus the old decoder (`native/mesher/source.cpp`, `source_cache.cpp`), which
the operator has retired. Today's ingress refuses per file. Every refusal
proposed below is per entity, with the rest of the part meshed and marked
incomplete (D-partial-output).

| # | Kind | Exports files / instances | OCCT dataset files / instances | Legal in AP242 ed.4 | Today | Decision | Representation; error budget |
|---|---|---|---|---|---|---|---|
| 1 | Plane face | 490 / 114,075 | 258 / 8,209 | yes | admitted | **admit** | native; exact |
| 2 | Manifold solid B-rep, closed outer shell | 493 / 3,190 solids | 223 / 710 | yes | admitted (outer shell only) | **admit** | shells meshed as given; closure computed from edge uses, never trusted (§5.3) |
| 3 | Line edge | 465 / 369,391 | 231 / 21,138 | yes | admitted | **admit** | native; exact |
| 4 | Circle edge | 481 / 186,619 | 144 / 15,226 | yes | admitted | **admit** | native; exact |
| 5 | Cylinder face | 464 / 77,043 | 140 / 5,085 | yes | admitted | **admit** | native; exact |
| 6 | B-spline edge (polynomial or rational) | 155 / 77,878 | 178 / 16,481 | yes | admitted | **admit** | native; exact |
| 7 | Cone face | 163 / 19,634 | 82 / 532 | yes | admitted | **admit** | native; exact |
| 8 | B-spline face, degree ≤ 3 | 111 / 22,909 | 100 / 3,945 | yes | admitted | **admit** | native; exact |
| 9 | Torus face, ring (R > r) | 79 / 4,710 | 62 / 944 | yes | admitted | **admit** | native; exact |
| 10 | Sphere face | 63 / 1,604 | 76 / 833 | yes | admitted | **admit** | native; exact |
| 11 | Geometric set / curve set beside the shape | 43 / 1,006 | 70 / 247 | yes | ignored | **report, not meshed** (D-shapes) | record only |
| 12 | Open shell in a surface model (sheet) | 15 / 37 | 72 / 755 | yes (MSSR WR3) | refused (no solid) | **admit; mesh and report open** (§6) | faces as usual; free-edge chains are single-face edges (design R5) |
| 13 | Ellipse edge | 55 / 8,681 | 28 / 188 | yes | admitted | **admit** | native; exact |
| 14 | B-spline edge of degree > 3 (bare 3D curve) | 20 / 570 | 38 / 3,350 | yes | admitted | **admit** | native; exact |
| 15 | B-spline face of degree > 3 | 8 / 157 (deg 4-6) | 54 / 1,802 (deg 4-12) | yes | refused by the old decoder (degree ≤ 3) | **admit up to degree 25** (§4.6) | native; exact |
| 16 | Surface of linear extrusion face | 1 / 9 | 39 / 683 | yes (WR5: line, conic, polyline, B-spline profile) | refused | **admit** | native swept evaluator; exact, parameterization preserved (§4.2) |
| 17 | Constructive (supplemental) geometry representation | 24 / 26 | 7 / 9 | yes | ignored | **report, not meshed** (D-shapes) | record only |
| 18 | Face bounded by a `vertex_loop` (pole) | 14 / 141 | 8 / 23 | yes (WR6, WR7) | **whole file refused** | **admit** | pole vertex with a fan (design §4.3); exact |
| 19 | `brep_with_voids` | 5 / 16 solids, 86 voids | 17 / 17 solids, 31 voids | yes (ABSR WR4: void orientation false) | **voids silently dropped** (B1 is fixing) | **admit** | every shell meshed, orientation from the void's flag |
| 20 | Surface model is the only exact geometry | 1 file | 76 files | yes | refused (no solid) | **admit** (as row 12) | as row 12 |
| 21 | Closed shell inside a surface model | 0 | 21 / 22 | yes (MSSR WR3) | refused | **admit; report "closed sheet", no solid asserted** | as row 12 |
| 22 | Wireframe or geometric set only | 0 | 20 files | yes | refused | **nothing to mesh; record** | record only |
| 23 | No shape geometry (external-reference assemblies) | 11 files | 10 files | yes | refused | **record; resolve references** (G9) | record only |
| 24 | Tessellated items beside an exact shape | 15 files | 2 files | yes | ignored | **report, not meshed** (D-shapes) | record only |
| 25 | Surface of revolution face | 1 / 2 | 16 / 516 | yes (WR5) | refused | **admit** | native swept evaluator; exact, parameterization preserved (§4.1) |
| 26 | Non-advanced `face_surface` in a surface model | 0 | 13 / 312 | yes (MSSR WR4-WR13) | refused | **admit** | same kinds as advanced faces |
| 27 | Torus with R ≤ r written as `toroidal_surface` (spindle or horn) | 4 / 354 | 7 / 52 | yes (no rule forbids it) | refused by the old decoder | **admit** | native torus with interior pole lines (§4.4) |
| 28 | Declared `closed_shell` with free edges | 0 | 6 shells | the type allows it | Closed flag forced by OCCT | **admit; report open** | as row 12 |
| 29 | Tessellated-only part | 1 file (NIST FTC-08 `-tg`) | 4 files | yes | refused | **report, not meshed**, by the literal reading of D-shapes; **operator question 1** | if admitted: pass-through, deviation unknown |
| 30 | Hyperbola edge | 0 | 2 / 8 | yes (conic) | refused | **admit** | native; exact (parameterization convention open, §4.5) |
| 31 | Solid whose outer is an `open_shell` | 0 | 2 | no (`outer : closed_shell`) | admitted by OCCT | **admit; report invalid and open** | as row 12 |
| 32 | CSG (`boolean_result`, CSG primitives) | 0 | 2 / 4 | yes (CSG shape representation) | refused | **refuse: not a B-rep** | refusal record (§5) |
| 33 | Parabola edge | 0 | 1 / 4 | yes (conic) | refused | **admit** | native or exact degree-2 B-spline; exact |
| 34 | `degenerate_toroidal_surface` | 0 | 1 / 4 | yes (WR1: R < r) | admitted as a plain torus; `select_outer` lost | **admit** | as row 27, with `select_outer` checked against the face's lobe (§4.4) |
| 35 | Offset surface | 0 | 0 | not in advanced faces; yes in surface models (MSSR, via `msf_surface_check`) | refused | **admit** [call] | elementary base: exact elementary kind; other base: native evaluation with a computed bound (§4.3) |
| 36 | Faceted B-rep (`poly_loop` on planes) | 0 | 0 | yes (FBSR) | refused | **admit** | planar polygons; exact |
| 37 | Polyline edge | 0 | 0 | yes (WR2; ≥ 3 points, WR9) | admitted (OCCT makes a degree-1 B-spline) | **admit** | degree-1 B-spline, knots 0..n−1; exact incl. parameterization |
| 38 | `surface_curve_swept_surface`, `fixed_reference_swept_surface` | 0 | 0 | yes (in `swept_surface`; WR5 constrains the profile) | refused (OCCT has no reader) | **refuse** until the Part 42 semantics are read (§5) | refusal record |
| 39 | Composite-curve edge | 0 | 0 | no (WR2; MSSR WR8) | OCCT fails the wire | **refuse: illegal** | refusal record |
| 40 | `offset_curve_3d` edge | 0 | 0 | MSSR only (WR8) | OCCT reads it without unit scaling | **refuse** until the Part 42 semantics are read | refusal record |

Rows are ordered by files containing the kind across all 839 files. Faces
inside supplemental surface models are counted in their rows too; row 12's
exports are NIST supplemental geometry, not sheet parts (§2.3).

### 0.2 Findings that change the design

1. **Today's ingress refuses legal, common input.** By static derivation over
   the census (§2.5), today's ingress refuses 34 of 506 export files (6.7%) and
   162 of 333 OCCT-dataset files (49%) on entity kind alone. The largest export
   cause is `vertex_loop` face bounds: 14 files, among them the NIST CTC-01/02/04/05,
   FTC-06 and STC-06/10 fidelity fixtures. The reader throws at
   `native/step_source/main.cpp:316` on any bound that is not an `edge_loop`,
   although AP242 advanced-face WR6 and WR7 make vertex loops legal [code; src].
2. **Revolution and extrusion are exact in shape but not in parameterization when
   converted.** Lifecycle map contradiction X12 says both convert exactly to
   rational B-splines. That is true of the point set, but OCCT's conversion
   writes the rotation as rational quadratic arcs [code `GeomConvert_1.cxx:544-643`],
   and no rational map is linear in angle (§4.1). The file's pcurves are in angle
   units, so a converted carrier forces a non-linear remap of every pcurve. A
   native swept evaluator keeps the file's parameterization and costs one small
   evaluator. **The matrix admits both natively.**
3. **Offset surfaces never occur in the 839 files** and are illegal in AP242
   advanced faces [src `advanced_face_geometry_select`]. The design's
   "representation reserve" for them therefore guards an empty class in this
   corpus. An offset of a plane, cylinder, cone, sphere or torus is the same
   kind of surface, exactly and with an affine parameter map (§4.3), so only a
   non-elementary base needs new machinery.
4. **SolidWorks writes spindle tori as plain `toroidal_surface` with R < r.**
   354 export faces in 4 files (Ender-3, CR-10) do this
   [meas], and the old decoder refuses them [code `source_cache.cpp:276-281`].
   The parameterization then has singular lines inside the UV domain, not on its
   boundary. The mesher needs interior pole lines for this kind (§4.4).
5. **B-spline degree above 3 is real in production exports.** Voron, Rat Rig and
   CAx-IF files carry surfaces of degree 4-6 and edges of degree 5-11. The OCCT
   dataset reaches degree 12 for surfaces and 14 for curves [meas]. JAMA's
   recommended limit is degree 5 [src JAMA PDQ V5.1 G-SU-HD, G-CU-HD], OCCT's
   hard limit is 25 and SMLib's is 17 [code]. The design's density rules already
   hold for any degree; only the evaluator and the batch layout change.
6. **Export edges almost never carry pcurves.** Only 205 of 642,569 export edges
   (0.03%) are `surface_curve` or `seam_curve` with associated pcurves; the rest
   are bare 3D curves [meas]. Under D-master, "recompute with a recorded
   residual" is the normal path for exports, not the exception. This is a
   cross-lane finding for G1 (§8).

## 1. Method

### 1.1 Corpus

| Set | Location on the fleet host (BRepGPU root) | Files (unique) |
|---|---|---|
| NIST MBE PMI | `corpus/sources/nist/` | 71 (49) |
| CAx-IF public library | `corpus/sources/cax-if/` | 53 (53) |
| Open-hardware TDPs (Ender-3, CR-10, Voron 2.4r2, Voron Trident, Rat Rig V-Core 3) | `corpus/sources/tdp/` | 376 (376) |
| OCCT dataset 7.9.0 | copied from the laptop's ignored `quarantine/sources/occt-dataset-7.9.0` to `lanes/M4-G5/held/` | 336 (333) |
| step-conformance 96f30bbe0954 | same, `lanes/M4-G5/held/` | 27 (27) |
| OM10 MAN VA01.3 | the existing private watch file | 1 |

The OCCT dataset and step-conformance were not on the fleet host. The 363 STEP
files were copied byte for byte, by the list from `find`, with `rsync
--files-from`. Duplicates were removed by SHA-256 of the file bytes before
counting (NIST ships the same files in several archives). ABC (corpus manifest
§1) and Fusion 360 Gallery (§2) are not acquired.

### 1.2 Text census (`lanes/M4-G5/probe/census.py`, standard library only)

- **Lexer.** Splits the DATA sections into records on `;` outside strings and
  comments. DATA is matched case-insensitively; the OCCT dataset has a
  `Data;` file. Each record is keyed by `#id` and its top-level entity names,
  so complex instances such as rational B-splines are typed correctly.
  Parameters are parsed lazily into nested lists.
- **Counts.** Every entity type; each face by its resolved surface kind; each
  B-spline face's maximum degree; each edge by its 3D curve kind, resolving
  `surface_curve`, `seam_curve`, `trimmed_curve` and replicas to their base and
  recording how many pcurves the wrapper carries; pcurve 2D kinds; swept-surface
  profile kinds; offset bases; `select_outer`; R ≤ r on tori.
- **Solids and shells.** Solids by subtype, with outer-shell type, void count
  and void orientation. For every `closed_shell` and `open_shell`, edge uses are
  counted through faces, oriented faces, bounds and oriented edges. A shell is
  `closed2` when every edge has exactly two uses, `free` when some edge has one,
  and `nonmanifold` when some edge has more than two. A two-use edge traversed
  the same way twice is counted as an orientation inconsistency.
- **Representations.** Which representation types hold which geometric items,
  and what links them: `shape_definition_representation`, any
  `*representation_relationship` (simple or complex), or a `representation_map`.
- **File class.** Each file is classed by the geometry it contains: exact
  solid, faceted, surface model, tessellated, wireframe or geometric set.
- **Errors.** Four files first failed: one lower-case `Data;` and three complex
  `definitional_representation` instances. The parser was fixed, the four were
  re-run, and the final table has zero errors.

### 1.3 OCCT census (`lanes/M4-G5/probe/occt_census.cpp`)

- **Reader.** OCCT 8.0.1 is the repository's installed build. Its
  `StepToGeom.cxx` and `StepToTopoDS_Builder.cxx` are byte-identical by SHA-256
  to the laptop's `quarantine/new-iteration/occt` at b8f597c. The probe uses
  `STEPControl_Reader` with ShapeProcess flags cleared after `ReadFile`, as
  `native/step_source` does, then `TransferRoots`.
- **Counts.** Unique faces by `GeomAdaptor` type and Geom class; unique edges by
  curve type and B-spline degree; shells by the `Closed()` flag against
  free-edge counts; faces with no surface; and every translation check message.
- **Isolation.** Each file ran in its own process under a 1800 s timeout. All
  864 processes exited 0.
- **Face-map probe** (`face_map.cpp`). For one file it lists the STEP face
  entities that `TransferBRep::ShapeResult` leaves unmapped.

### 1.4 Run context

Run context is recorded although no number here is a performance claim. All
reads were cold first reads. Both censuses used CPU only on a dual-socket host
(SMT on, AutoNUMA on, governor `schedutil`), with no pinning. Load average was
6.1 at the start of the text census and 17.0 at the start of the OCCT census,
with other campaign lanes running.

Summed per-file times were 158 s for the text census and 132 s for the OCCT
census, which includes the 241 MB Voron assembly. The data are under
`lanes/M4-G5/out/`: per-file long rows (`census-final.tsv`, `occt.tsv`), the
aggregates, the per-file text/OCCT comparison (`compare.tsv`), the static
admission table (`bridge-admission.tsv`) and the matrix counts (`matrix.tsv`).

### 1.5 Limits

- Text counts include faces in every representation, including supplemental
  and alternative ones. §2.3 separates them where the links allow it.
- OCCT counts unique `TShape`s and translates geometric sets into faces, so
  per-file face counts differ from the text counts in both directions
  (`compare.tsv`; 27 files differ).
- Degree counts for edges cover bare B-spline edges only. A B-spline wrapped in
  a `surface_curve` (9,426 edges, almost all in the OCCT dataset) is counted by
  kind but not by degree.
- The admission of today's ingress is derived statically from code and census,
  not run (§2.5).

## 2. Frequencies (question 1)

### 2.1 Geometry kinds

The matrix in §0.1 gives every kind. Ratios that matter:

- **Exports.** Planes (47.4% of faces) and cylinders (32.0%) dominate. Next
  come B-splines (9.6%), cones (8.2%), tori (2.1%) and spheres (0.7%).
  Revolution, extrusion, degenerate tori, hyperbolae, parabolae, offsets,
  faceted solids and polylines together are 11 faces in 2 files (0.005%) [meas].
- **OCCT dataset.** Revolutions (516 faces) and extrusions (683) are 5.3% of its
  faces, and B-splines of degree ≥ 6 are 1,331 faces in 37 files [meas]. This
  stratum is a bug collection: it tells us what arrives in practice at a widely
  used reader, not how often.
- **The watch.** OM10 uses only planes, cylinders, cones, spheres, ring tori and
  B-splines of degree ≤ 3, with 7 degree-5 B-spline edges [meas]. Every kind in
  rows 12-40 is absent from it, so the watch cannot exercise this matrix. That
  is consistent with the no-overfitting rule.

### 2.2 B-spline degree

| Max degree of face | Exports faces (files) | OCCT dataset faces (files) |
|---|---|---|
| 1 | 124 (5) | 414 (35) |
| 2 | 4,008 (18) | 347 (31) |
| 3 | 18,777 (99) | 3,184 (66) |
| 4 | 2 (2) | 54 (6) |
| 5 | 147 (6) | 417 (15) |
| 6 | 8 (2) | 161 (10) |
| 7-9 | 0 | 935 (48 file-degree pairs) |
| 10-12 | 0 | 235 (27 file-degree pairs) |

Bare B-spline edges of degree > 3 appear in 20 export files (570 edges, mostly
degree 5) and 38 OCCT-dataset files (degrees up to 14). Rational B-splines are
2,505 export faces in 59 files.

### 2.3 Shape-level kinds and the "main representation"

- **Voids.** 86 void shells in 16 solids across 5 export files: Voron 2.4r2 and
  Trident full assemblies, the CR-10 assembly and two Ender-3 files. A further
  31 voids sit in 17 OCCT-dataset files. Every void has orientation `.F.`, as
  ABSR WR4 requires [meas; src].
- **Sheets and open shells.**
  - **Exports.** All 15 export files that carry a `shell_based_surface_model`
    are NIST models whose 37 open shells hold supplemental geometry beside the
    solid. The two faces checked in CTC-04 are named "Passing Plane.1" and
    "Angle Plane.2". The one export file whose only exact geometry is a
    surface model is step-conformance's `bowl_01.step`.
  - **OCCT dataset.** 76 files have a surface model as their only exact
    geometry: 755 open shells and 22 closed shells inside surface models.
  - **Links.** Surface models are linked to products by a
    `shape_representation_relationship` in 49 files and directly by a
    `shape_definition_representation` in 29. CAx-IF-style
    `constructive_geometry_representation` holds supplemental items in 24 export
    files [meas].
  - **Consequence.** Link type alone cannot separate main from supplemental
    geometry. NIST CTC-04 links its supplemental planes exactly as it links its
    solid [meas]. Choosing the main representation needs the raw-entity reading
    that G1 is scoping.
- **Tessellation.**
  - 22 files carry AP242 tessellated items, 16 of them exports. In 15 exports
    the tessellation sits beside an exact shape: mostly
    `complex_triangulated_surface_set` (1,248 instances in 11 NIST files), in
    `tessellated_shape_representation` linked by relationships.
  - 5 files are tessellated-only: NIST `nist_ftc_08_asme1_ap242-e1-tg.stp` and
    four in the OCCT dataset. Three files carry a `tessellated_solid` [meas].
- **Shell validity.**
  - 6 OCCT-dataset `closed_shell`s have free edges; 2 OCCT-dataset solids use an
    `open_shell` as their outer shell, which `manifold_solid_brep.outer :
    closed_shell` forbids; 11 shells (5 closed, 6 open, 8 files) traverse an edge the same way twice.
  - Every export closed shell has every edge used exactly twice [meas].
  - The OCCT census agrees: 7 shells are flagged `Closed()` with free edges, and
    7 that close are flagged open [meas].
- **No geometry.** The 11 export files with no shape geometry are the CAx-IF
  `s1-*` multi-file assemblies, whose geometry is held by external references
  [meas].

### 2.4 What ABC would change

- **What is known.** Koch et al. list ABC's surfaces as plane, cone, cylinder,
  sphere, torus, revolution, extrusion and NURBS. Its curves are lines, circles,
  ellipses, parabolae, hyperbolae and NURBS [src Koch 2019 §3.1]. Figure 3 reads
  roughly: plane 50%, cylinder 19%, "Other" 15%, B-spline 10%, torus, cone,
  sphere and revolution a few percent each, and no extrusion bar
  [src Koch 2019 Fig. 3, read from the chart].
- **Inference.** "Other" most likely aggregates OCCT's extrusion, offset,
  Bézier and other-surface types, because ABC is read through OCCT
  [src Koch 2019 §3.2; inf]. If so, ABC's Onshape/Parasolid exports may carry
  extrusions and perhaps offsets far more often than this corpus does.
- **The measurement that settles it.** Run this census over the fixed ABC
  sample once the operator answers the download-term question.

### 2.5 What today's ingress admits (static derivation)

This is a derivation, not a run.

- **Method.** Each file's census was evaluated against the refusal sites of
  today's ingress at HEAD 627163b and the old decoder
  (`lanes/M4-G5/probe/bridge_admission.py`):
  - `main.cpp:177` throws on curves other than line, circle, ellipse, Bézier
    and B-spline;
  - `main.cpp:210` throws on surfaces other than plane, cylinder, cone, sphere,
    torus and B-spline;
  - `main.cpp:313` and `:332` refuse non-advanced faces and walk only
    `Outer()`;
  - `main.cpp:316` refuses any bound that is not an `edge_loop`;
  - `main.cpp:291` requires each leaf product to bind exactly one solid;
  - `source.cpp:479` refuses surface degree above 3;
  - `source_cache.cpp:276-281` refuses tori with |R| ≤ |r|.
- **Results.**

  | Cause | Exports | OCCT dataset |
  |---|---|---|
  | `vertex_loop` bound | 14 | 7 |
  | no solid: external references / no geometry | 11 | 10 |
  | surface degree > 3 (old decoder) | 6 | 29 |
  | no solid: surface model only | 1 | 76 |
  | no solid: tessellated only | 1 | 4 |
  | surface of extrusion | 1 | 10 |
  | surface of revolution | 0 | 13 |
  | no solid: wireframe or set only | 0 | 20 |
  | hyperbola / parabola edge | 0 | 3 |
  | non-advanced face | 0 | 3 |
  | **files refused on kind** | **34 of 506** | **162 of 333** |
  | spindle torus (refused later by the old mesher) | 4 | 7 |
  | voids silently dropped | 5 | 17 |

- **Not derivable from text counts.** Refusals from multi-solid leaf products
  and ambiguous roots need the product graph and are not counted.

## 3. What the standard allows and what reference kernels do (question 2)

### 3.1 AP242 ed.4 longform (ISO/TS 10303-442 ed.7 `mim_lf.exp`) [src]

- **`advanced_face`** (lines 7971-8077).
  - Its geometry is `advanced_face_geometry_select` = `b_spline_surface`,
    `elementary_surface` or `swept_surface` (line 744).
  - Edge-loop edges must be `edge_curve`s (WR1). Edge geometry must be exactly
    one of line, conic, polyline, `surface_curve` or B-spline curve (WR2). A
    polyline edge needs ≥ 3 points (WR9).
  - Vertices are `vertex_point`s on `cartesian_point`s (WR3). Bounds are
    `edge_loop` or `vertex_loop` (WR7), and a vertex loop's vertex is a point
    vertex (WR6). Surface curves associate pcurves only (WR8).
  - Swept faces need a line, conic, polyline or B-spline profile (WR5).
- **Excluded from advanced faces.** Offset surfaces, `rectangular_trimmed_surface`,
  `curve_bounded_surface`, composite-curve edges and offset-curve edges [src].
- **What `swept_surface` includes.** In this edition it also takes in
  `surface_curve_swept_surface` and `fixed_reference_swept_surface`
  (lines 34682-34687). They are therefore legal in advanced faces, subject to
  WR5 [src].
- **`advanced_brep_shape_representation`** (lines 7924-7969). Items are
  `manifold_solid_brep`, `mapped_item` or `axis2_placement_3d`. Every face of
  every shell is an advanced face (WR2). The outer shell is not an
  `oriented_closed_shell` (WR3). Every void has orientation false (WR4).
  `brep_with_voids.voids` is a set of `oriented_closed_shell` (line 10257).
- **`manifold_surface_shape_representation`** (lines 22299-22585).
  - Shells are open, closed or oriented closed (WR3). Faces are `face_surface`
    (WR4) on a surface accepted by `msf_surface_check` (WR5).
  - `msf_surface_check` (line 47386) accepts elementary surfaces, swept
    surfaces with valid profiles, **offset surfaces of valid bases** (self-intersect
    false or unknown), surface replicas and non-self-intersecting B-splines.
  - Edge geometry may also be `offset_curve_3d`, `curve_replica` or `pcurve`
    (WR8).
- **`faceted_brep_shape_representation`** (lines 17235-17300). Faces are
  `face_surface` on a plane with exactly one outer bound (WR2, WR3); voids are
  orientation false (WR5).
- **Geometry entities.**
  - `toroidal_surface` has no radius rule (line 36492).
    `degenerate_toroidal_surface` requires `major_radius < minor_radius` and
    adds `select_outer : BOOLEAN` (line 13716).
  - `b_spline_surface` and `b_spline_curve` carry no degree limit
    (lines 9245, 9181).
  - `parabola` requires `focal_dist ≠ 0`; `hyperbola` has `semi_axis` and
    `semi_imag_axis` (lines 26030, 20039).
  - `offset_surface` is `basis_surface`, `distance` and `self_intersect`
    (line 25122).
- **AP203 and AP214.** These files (95% of the corpus is AP214 or AP203 by
  `FILE_SCHEMA` [meas]) use the same integrated resources for advanced B-reps.
  This lane read only the AP242 longform, so identical WHERE rules in the older
  longforms are **[inf]**.

### 3.2 Part 42 semantics this lane could read

ISO 10303-42 itself is paywalled. Its definitions arrive here only through
republications.

- **`toroidal_surface`.** σ(u,v) = C + (R + r cos v)(cos u·x + sin u·y) + r sin v·z.
  u and v are angles in the plane-angle unit. Part 42 states that the surface
  self-intersects unless R > r [src IFC4.3 IfcToroidalSurface, "Definition
  according to ISO/CD 10303-42:1992"].
- **`surface_of_revolution`.** u is the rotation angle over 0-360 degrees in the
  plane-angle unit; v is the profile curve's parameter [src IFC4.3
  IfcSurfaceOfRevolution].
- **`surface_of_linear_extrusion`.** v runs over the whole real line along the
  extrusion vector; u is the profile's parameter. IFC carries the extrusion
  depth separately, so whether v is scaled by the vector's magnitude is
  **[inf]** from the formula image [src IFC4.3 IfcSurfaceOfLinearExtrusion].
- **`polyline`.** Segment i is parameterized on i−1 ≤ u ≤ i, so the range is
  0 ≤ u ≤ n−1 [src IFC4.3 IfcPolyline].
- **Not available from any open source read.** The parameterizations of
  `hyperbola`, `parabola`, `offset_curve_3d`, `surface_curve_swept_surface` and
  `fixed_reference_swept_surface`, and the meaning of `select_outer`. The
  STEP Tools module pages defer each of them to "the corresponding MIM entity
  from ISO 10303-42" [src SMRL basic_geometry §4.2.12, §4.2.14]. They go on the
  shopping list (§11).

### 3.3 Reference kernels

From the reading sub-lane, with the starred items spot-checked by this lane
against the code.

**OCCT 8.0.1 (b8f597c).**

- **Revolution and extrusion** become `Geom_SurfaceOfRevolution` and
  `Geom_SurfaceOfLinearExtrusion` with no conversion (`StepToGeom.cxx:2082-2098`).
  - The extrusion vector's magnitude is discarded: `gp_Dir D(V->Vec())`
    (`StepToGeom.cxx:2020`)*. That changes v whenever |V| ≠ 1, which the census
    finds in 14 surfaces in 4 files [meas].
  - A circle or ellipse meridian centred on the axis is re-positioned and
    trimmed to [0, π] (`StepToGeom.cxx:2049-2066`).
- **Offset surfaces** become `Geom_OffsetSurface` (`StepToGeom.cxx:1939-1965`).
  - `Geom_OffsetSurface::Surface()` gives a closed-form equivalent for plane,
    cylinder, cone, sphere and torus bases (`Geom_OffsetSurface.cxx:867-993`).
  - `GeomConvert` approximates any other base with a hard-coded 1e-4 tolerance
    and never checks `IsDone`/`MaxError` (`GeomConvert_1.cxx:771-787, 934-962`).
- **`degenerate_toroidal_surface`** becomes a plain `Geom_ToroidalSurface`.
  `select_outer` is read nowhere outside the StepGeom and RW layers (grep)*.
  The OCCT tracker has an open report (0033618, filed against 7.7.0) of wrong
  orientation for degenerate tori from revolved profiles, with four duplicates
  [src search snippets; tracker unreachable].
- **Hyperbola and parabola** become `Geom_Hyperbola`/`Geom_Parabola`.
  - Trims on them get no parameter factor (`StepToGeom.cxx:2377-2431`).
  - The 2D trimmed case is marked `// LATER !!!` (`StepToGeom.cxx:2547-2551`).
- **Polylines** become degree-1 B-splines with knots 0..n−1
  (`StepToGeom.cxx:1751-1830`). That matches the IFC republication of Part 42.
- **Degree.** `BSplCLib::MaxDegree()` is 25. A surface of degree above 25
  throws, the throw is caught, and the face fails (`Geom_BSplineSurface.cxx:65-69`;
  `StepToGeom.cxx:1918-1987`).
- **B-spline faces are forced periodic** (`StepToTopoDS_TranslateFace.cxx:556-565`).
  The census sees "Surface forced to be periodic" in 86 of 839 files
  (2,721 faces) [meas].
- **Voids.** `brep_with_voids` keeps its voids.
  - `faceted_brep_and_brep_with_voids` loses them.
    `StepShape_FacetedBrepAndBrepWithVoids` derives directly from
    `ManifoldSolidBrep` (`.hxx:34`)*, so the `ManifoldSolidBrep` branch at
    `STEPControl_ActorRead.cxx:1832` takes it before its own branch at `:1846`*.
  - The census found no faceted B-reps, so this never fires here.
- **Shells.**
  - `oriented_face` in a shell is dropped with only a warning, because
    `StepToTopoDS_TranslateShell.cxx:87` down-casts to `FaceSurface`*.
  - The shell `Closed` flag is computed and then overwritten with true for
    every solid shell and every closed shell in a surface model
    (`StepToTopoDS_Builder.cxx:150, 217, 235, 303, 350, 366, 442`). The census
    sees 7 shells flagged closed that have free edges [meas].
  - Surface models are read under the default `ReadShapeRepr = All`
    (`DESTEP_Parameters.hxx:167`)*.
- **Tessellation.** `read.step.tessellated` defaults to **On**
  (`DESTEP_Parameters.hxx:168`)*.
  - Tessellated faces with no link to a B-rep face become faces with a
    `Poly_Triangulation` and no surface. The census sees 557 such faces in
    5 files [meas].
  - Linked tessellations attach to the B-rep face, and BRepMesh then probably
    reuses them instead of remeshing [inf from `BRepMesh_ModelPreProcessor.cxx:88-90`].
- **Swept surfaces other than revolution and extrusion** have no reader class at
  all; their faces fail [code grep; inf].
- **Composite-curve edges** fail and take their whole wire with them
  (`StepToTopoDS_TranslateEdge.cxx:420-423`; `StepToTopoDS_TranslateEdgeLoop.cxx:832-835`).
- **`offset_curve_3d`.** Its distance is not unit-scaled
  (`StepToGeom.cxx:1384`).
- **Silent face loss.** In NIST CTC-04 two supplemental plane faces (#14622,
  #14665, each alone in an open shell) map to no shape. The transfer emits no
  check message for them [meas, `face_map.cpp`].

**truck (88ed005, `truck-stepio`).**

- Voids, surface models, oriented faces, revolution and extrusion are read
  natively. The extrusion vector's magnitude is kept.
- Faceted B-reps, offsets, degenerate tori, CSG and tessellation are unmapped,
  and their faces are dropped with only an `eprintln`.
- Parabola edges are built as hyperbolae (`src/in/mod.rs:2499-2519`)*, a
  reference bug.

**NVIDIA usd-brep 1.0.0 (headers only).**

- SMLib's maximum degree is 17 (`SMLib/NL_Nurbsdef.h:28`).
- The OCCT-B-rep importer lowers revolution and extrusion to NURBS and
  "resolves" offset surfaces and curves "to their basis"
  (`OCCT_BREP_IMPORT/OcctGeometryConvert.h:24-55`). If the code does what the
  header says, offsets lose their distance on the NVIDIA arm **[inf]**.

**vcad (eba7a2e).** Fits offsets of B-spline bases by using sampled
`base + d·n` points directly as bicubic control points, with no error bound
(`vcad-kernel-step/src/entities/surfaces.rs:1026-1060`).

**Parasolid and ACIS.** The healing paper §1.1-1.2 covers tolerance semantics
only. Their support for offset and swept surfaces, degree limits and sheets is
not in any source this repository holds (§11).

## 4. Representation of each admitted kind, and its cost (question 3)

The design's density rules (§6.1) need, per carrier, bounds on the second
directional derivatives in parameter space (Waldron form). Each derivation below
supplies them. "Exact" means the carrier is evaluated as the file defines it,
so no representation reserve is taken out of ε.

### 4.1 Surface of revolution: native, not converted

- **Evaluator** [own from §3.2]. S(u,v) = C + Rot_z(u)·(λ(v) − C), where u is
  the angle in radians after the G6 unit map and λ is the profile.
- **Second derivatives.** With ρ(v) the profile's distance from the axis:
  ‖S_uu‖ = ρ(v), ‖S_uv‖ = ‖λ'(v)_⊥‖ (the component of the profile derivative
  perpendicular to the axis), and ‖S_vv‖ = ‖λ''(v)‖. Over a span this gives
  D_uu = ρ_max, D_uv = max ‖λ'‖ and D_vv = max ‖λ''‖. The last two come from the
  profile curve's own rules: control-net differences for a B-spline,
  closed forms for a conic.
- **Why not convert.**
  - OCCT's conversion uses rational quadratic arcs of at most 150° in u
    [code `GeomConvert_1.cxx:544-643`].
  - A rational parameterization cannot be linear in angle: if cos(at+b) equalled
    a rational function of t on an interval, it would equal it everywhere,
    giving a rational function with infinitely many zeros [own].
  - So a converted carrier no longer agrees with pcurves written in angle
    units. Every pcurve then needs a non-linear remap (tan-half-angle per arc),
    which is not polynomial, so the remapped pcurve is itself an approximation
    [own].
  - Conversion also raises the degree, adds weights and loosens the rational
    second-difference bound (Zheng–Sederberg uses the minimum weight; design
    §6.1).
- **Cost.**
  - One evaluator that composes the existing curve evaluators with a rotation:
    two trig values per u sample, shared across a SoA batch.
  - One more surface kind in the batch layout, and the angle-unit map that G6
    already has to define.
  - Frequency is 2 export faces and 516 OCCT-dataset faces [meas], so the cost
    is code, not run time.
- **Hypothesis H1.** At equal ε, native evaluation emits no more triangles and
  takes no more time than the converted carrier, and needs no pcurve remap.
  **Falsifier:** arena runs on the 17 revolution files plus an authored family
  (B-spline, conic and line profiles; profiles touching the axis) show the
  converted carrier faster at equal oracle deviation, or show native deviation
  above ε.

### 4.2 Surface of linear extrusion: native

- **Evaluator** [own]. S(u,v) = λ(u) + v·V, with V read raw, including its
  magnitude (OCCT drops it, §3.3).
- **Derivatives.** S_uv = 0, S_vv = 0 and ‖S_uu‖ = ‖λ''(u)‖. The surface is
  ruled, so the density rule spaces samples in u only and v needs only its trim
  boundary.
- **Conversion.** For a B-spline profile, conversion to a degree-1-in-v
  B-spline is exact and keeps the parameterization [code
  `GeomConvert_1.cxx:646-691`]. For a conic profile it inherits §4.1's u
  problem.
- **Cost.** Smaller than revolution. Frequency is 9 export faces and
  683 OCCT-dataset faces [meas].
- **Hypothesis H2.** Native ruled sizing emits no interior samples in v away
  from trims. **Falsifier:** an authored extrusion family whose oracle deviation
  exceeds ε, or whose v-interior sample count is non-zero.

### 4.3 Offset surface

- **Elementary bases: exact closed forms** [own; agrees with OCCT's
  `Geom_OffsetSurface::Surface()` [code]].
  - Plane: a translated plane.
  - Cylinder: radius R + d. Sphere: radius R + d. Torus: minor radius r + d.
    In each, u and v are unchanged.
  - Cone with semi-angle α:
    - With N = cos α·ρ̂ − sin α·z, S + dN = C − d sin α·z + (R + d/cos α
      + v' tan α)·ρ̂ + v'·z, where v' = v − d sin α.
    - This is a cone with reference radius R + d/cos α, apex shifted along the
      axis, and an affine v map.
  - The sign follows the surface normal's sense. A negative radius after the
    offset means the offset passes through the axis, and that face is refused
    as degenerate.
- **Other bases: native evaluation Ŝ = S + dN** [src PMC §11.3.1 eq. 11.17].
  - **Singularity.** Ŝ is singular where a principal curvature equals −1/d
    [src PMC §11.3.2 eq. 11.28, in its sign convention].
  - **Density bound** [own derivation].
    - D²Ŝ = D²S + d·D²N.
    - For N = n/‖n‖ with n = S_u × S_v, ‖D²_w N‖ ≤ ‖D²_w n‖/‖n‖ + 3‖D_w n‖²/‖n‖².
      Differentiate N' = P n'/‖n‖ with P = I − NNᵀ and bound
      ‖(N'Nᵀ + NN'ᵀ)n'‖ ≤ 2‖n'‖²/‖n‖.
    - Over a Bézier span, ‖D_w n‖ and ‖D²_w n‖ are bounded from control-net
      differences of S up to third order, and ‖n‖ from below.
    - So M̂ ≤ D_ww + |d|(max‖D²n‖/min‖n‖ + 3 max‖Dn‖²/min‖n‖²) is a
      non-understating bound.
    - Where min ‖n‖ cannot be bounded away from zero, the span holds a pole
      (handled as in the design) or a cusp (refused).
  - **Why not approximate.** No source read gives a tessellation bound for
    offsets. PMC §11.3.7 states that offsets of rational surfaces are not
    representable in the same class and surveys approximations (Farouki;
    Patrikalakis–Prakash) whose errors are estimated by sampling [src]. OCCT
    approximates with an unchecked fixed tolerance, and vcad with none (§3.3).
    A sampled reserve can understate between samples; the native bound does
    not.
- **Cost.**
  - Elementary bases: a constructor-time rewrite, zero run time.
  - Other bases: evaluating N costs one cross product and one normalization
    per sample. The bound needs per-span third-order differences, computed once
    per carrier and cached.
  - Frequency is zero in 839 files [meas].
- **Decision [call].** Admit both. The elementary path is nearly free and is
  what OCCT itself does. The general path is gated by the census of ABC's
  "Other" bar (§2.4): build it when any acquired corpus shows offsets of
  non-elementary bases. Until then, such a face is refused with the record in
  §5. That is honest because the class is empty in every corpus held and the
  refusal names it.
- **Hypothesis H3.** M̂ never understates. **Falsifier:** an authored family
  (offsets of bicubic patches with d from 0.01 to 0.9 of the minimum curvature
  radius, both signs) where the offline oracle finds deviation above ε.
  **Cost falsifier:** triangle count more than 2× an oracle-tuned sizing.

### 4.4 Spindle, horn and degenerate tori

- **Same surface, unbounded chart behaviour** [src IFC4.3 Part 42 text]. With
  R ≤ r the torus formula still holds. The surface passes through the axis where
  R + r cos v = 0, at v* = ±arccos(−R/r).
- **Interior singular lines** [own]. There S_u = 0, so v = v* are lines of
  singular parameterization inside [0, 2π). The design's poles sit on chart
  boundaries; these do not.
- **Representation.**
  - The native torus kind gains a flag R ≤ r and the two pole lines.
  - Edges incident to the axis points become poles: one vertex and a fan.
  - The face's UV domain must not straddle v* except at those points.
  - The existing torus density bounds (‖S_uu‖ ≤ R + r, ‖S_uφ‖ ≤ r, ‖S_φφ‖ = r;
    design §6.1) still hold and remain exact.
- **`select_outer`.** On a degenerate torus it should name the lobe the face
  lies on. The Part 42 definition is unread (§3.2), so the reader records the
  flag and checks that the face's UV domain lies on one side of v*. A mismatch
  with either reading is reported, never repaired [call].
- **Cost.** Interior pole lines in the chart logic, and the authored family
  below. 354 export faces (SolidWorks: knurled nut, thermistor nozzle, both
  assemblies) and 56 OCCT-dataset faces [meas].
- **Hypothesis H4.** Treating v* as pole lines meshes all 410 faces within ε
  with no fold across the axis. **Falsifier:** the offline oracle finds a
  triangle whose normal flips against the surface normal near the axis, or a
  triangle crossing the axis.

### 4.5 Hyperbola, parabola and polyline edges

- **Exact evaluators.** The conics are evaluated natively. Edge density from
  closed-form curvature is maximal at the vertex: a/b² for the hyperbola and
  1/(2f) for the parabola [own, standard conic geometry]. A polyline is a
  degree-1 B-spline with knots 0..n−1, exact including its parameterization
  [src IFC4.3; code OCCT].
- **What is open.** Part 42's parameterizations of hyperbola and parabola
  (§3.2) matter only where pcurves or trims are expressed in the curve's
  parameter. An `edge_curve` is bounded by its vertices, so 3D sampling is
  convention-free. For a non-SameParameter use, the measured correspondence
  that B1 is building applies.
- **Cost.** Two small evaluators. Frequency is 12 edges in 3 OCCT-dataset files
  [meas].

### 4.6 B-spline degree above 3

- **Density.** The design's span bounds D_uu = p(p−1)·max‖Δ²_u P‖, and
  correspondingly for uv and vv, hold for any degree. So does the
  rational Zheng–Sederberg form. Nothing in the error budget changes.
- **Evaluator cost.** De Boor or Bernstein evaluation costs O(p²) per point.
  The design already sorts SoA batches by degree class (§6.2); this needs one
  generic-degree class beside the specialised p, q ≤ 3 classes.
- **Cap.** 25, inherited from the bridge: OCCT cannot read anything higher
  [code]. JAMA's degree-5 recommendation is a data-quality warning under
  D-profile, never a refusal [src JAMA V5.1 G-SU-HD, G-CU-HD].
- **Hypothesis H5.** A generic-degree batch class costs less than 5% of
  whole-part meshing time on the Voron assemblies (degrees ≤ 6) and less than
  25% on the OCCT-dataset files with degree ≥ 8. **Falsifier:** measured stage
  times.

### 4.7 Voids, vertex loops, sheets, faceted solids, non-advanced faces

- **Voids.**
  - Every shell is meshed. A void's triangles take the orientation implied by
    its `oriented_closed_shell` flag.
  - The volume oracle computes outer minus voids.
  - Cost is B1's table change plus one more shell per solid in scheduling.
- **Vertex loops** (poles of cones and spheres).
  - The loop's vertex becomes the pole vertex with a fan, as the design already
    does for poles (§4.3 item 3).
  - The cost is the reader change at `main.cpp:316`. 141 export faces
    [meas].
- **Sheets and open shells.** See §6.
- **Faceted B-reps.**
  - Each `poly_loop` on a plane is a planar polygon, triangulated in the plane's
    chart with no approximation.
  - Shared edges are keyed by point-entity pairs; coincident point entities with
    distinct ids are resolved by G1's vertex rule.
  - Frequency is zero, and the cost is small.
- **`face_surface` in surface models.** These use the same geometry kinds as
  advanced faces, without the advanced-face WHERE rules, so geometry checks run
  per kind. 312 OCCT-dataset faces [meas].

## 5. Refusal records for kinds not admitted (question 4)

Every refusal carries:

- the file's SHA-256 and the Part 21 `#id`;
- the entity type;
- a reason code;
- the AP legality (legal or illegal, with the WHERE rule);
- the affected faces, edges and shells by id;
- what still meshed;
- the part status, `incomplete` (D-partial-output).

Refused reason codes:

| Kind | Reason code | Legal? | Corpus frequency | Honest under "arbitrary input"? |
|---|---|---|---|---|
| `surface_curve_swept_surface`, `fixed_reference_swept_surface` | `semantics_unavailable` | legal | 0 | Yes, for now. The frame definition is in Part 42, which no open source reproduces; OCCT and truck cannot read these either. Implementing from a guess would be the dishonest act. Revisit when Part 42 is acquired or a corpus shows one. |
| Offset of a non-elementary base | `offset_general_not_built` | legal in surface models only | 0 | Yes while the class stays empty; §4.3 states the trigger to build it. |
| Offset whose base has a principal curvature at or past −1/d inside the face, or an elementary offset through the axis | `offset_singular` | legal (`self_intersect` may be unknown) | 0 | Yes. The surface is singular there. |
| `offset_curve_3d` edge | `semantics_unavailable` | surface models only | 0 | Yes, as for the swept kinds. OCCT also mis-scales it. |
| Composite-curve edge | `illegal_edge_geometry` | illegal (WR2; MSSR WR8) | 0 | Yes. Illegal, and OCCT fails it too. |
| CSG (`boolean_result`, `csg_solid`) | `not_a_brep` | legal in CSG representations | 2 OCCT-dataset files | Yes. A B-rep mesher cannot mesh a CSG tree without a B-rep evaluator, which would be a separate unit. |
| Tessellated-only part | `no_exact_representation` | legal | 1 export + 4 | **Operator question 1.** Under the literal D-shapes it is reported. Refusing it is honest only if the record says a tessellation exists and was not used. |
| Wireframe or geometric-set-only part | `no_surface_geometry` | legal | 20 OCCT-dataset files | Yes. There is nothing to mesh. It is a record, not a failure. |
| External-reference assembly with no local geometry | `external_reference_unresolved` | legal | 11 exports | Honest only as a temporary state. Resolving references is an assembly-structure gap (G9), not an entity kind. |
| Supplemental, constructive or alternative representations and geometric sets beside the main shape | `not_main_representation` | legal | 51 export files | Yes, by D-shapes, provided every one is reported. |
| B-spline of degree > 25 | `degree_above_reader_limit` | legal | 0 | Yes. The bridge cannot read it; refusal happens there. |

Not refusable under the operator's goals: every kind marked **admit** in §0.1.
Each is legal, present in production exports or common readers, and supportable
at bounded cost.

- **Refusing these is a choice, not a limit.** Today's refusal of vertex loops,
  revolution, extrusion, degree > 3, spindle tori, sheets and voids sits in
  that class.
- **What it costs.** On the export stratum it refuses 6.7% of files outright
  (§2.5). That includes the NIST fixtures, which the campaign uses for fidelity.

## 6. Open shells and sheets: mesh them (question 5)

**Decision: mesh, and report open.** For both display and printing pipelines.

- **Evidence that they are legitimate input.**
  - Sheets are legal AP242 content (MSSR WR3) [src].
  - OCCT and truck both read them [code].
  - D-features says features above ε are never dropped.
  - The design already meshes them faithfully and reports them open (design
    §4.9, a call that this evidence supports).
- **How they mesh.** A sheet's free edges have one incident face, so they fall
  under the single-face class R5 (design §5.2) with no new machinery. Shell
  closure is computed from edge uses, never taken from the declared type. The
  6 declared-closed shells with free edges and the 2 open shells that are
  actually closed (§2.3) show the declared type is not evidence.
- **Display.** Open meshes render. glTF needs double-sided material on open
  parts.
- **Printing.**
  - **3MF.** Its manifold, orientation and outward-normal rules bind only
    objects of type `model` or `solidsupport`. "Objects of type 'support',
    'surface' and 'other' are allowed to break these conditions"
    [src 3MF Core 1.4.0 §4.1]. An open part therefore exports as a `surface`
    object, never as a `model` object.
  - **STL.** It has no object type. Whether an open part may be written to STL
    at all is G3's and D-formats' policy. This lane only notes that slicers do
    not fill holes; PrusaSlicer closes slice-loop gaps up to 2 mm and nothing
    more [src lifecycle map S14, L5].
- **Report contents.** The part's status is `open`, with each free-edge chain by
  source edge id. A closed shell inside a surface model is reported as a closed
  sheet, with no solid or volume asserted. A solid whose outer is an
  `open_shell` (illegal) is meshed faithfully and reported invalid.
- **Supplemental sheets are not parts.** The NIST reference planes are not
  meshed as part of the product (D-shapes). Telling main geometry from
  supplemental geometry needs the representation selection G1 is scoping
  (§2.3).
- **Hypothesis H6.** Every open shell in the 77 surface-model-only files meshes
  with a free-edge report whose chains match the census's free-edge count per
  shell. **Falsifier:** any mismatch between reported free edges and the
  edge-use count, or any open part written as a 3MF `model` object.

## 7. Hypotheses and falsifiers, collected

| ID | Claim | Falsifier |
|---|---|---|
| H1 | Native revolution is no slower and needs no pcurve remap | Converted carrier faster at equal oracle deviation; native deviation > ε |
| H2 | Native extrusion needs no v-interior samples | Oracle deviation > ε or v-interior samples on the authored family |
| H3 | The offset bound M̂ never understates and costs ≤ 2× oracle-tuned sizing | Deviation > ε, or triangle count > 2× |
| H4 | Interior pole lines mesh all 410 R ≤ r torus faces cleanly | Flipped or axis-crossing triangles |
| H5 | The generic-degree class costs < 5% (Voron) and < 25% (degree ≥ 8 files) of meshing | Measured stage times |
| H6 | Sheet free-edge reports match edge-use counts; no open part becomes a 3MF `model` | Any mismatch |
| H7 | Admitting rows 12-34 raises export-file admission from 93.3% to the share limited only by external references and tessellated-only parts (≥ 97.6%) | Re-run of the static admission, then an actual ingress run, on the same 506 files |

## 8. Cross-lane findings (not this gap; for the dispatcher)

- **G1 / D-master.**
  - Export edges carry pcurves in 0.03% of cases, so recomputing with a recorded
    residual is the main path.
  - OCCT forces B-spline faces periodic in 86 files.
  - OCCT reads tessellation by default and may let BRepMesh reuse it, which
    matters for the steel-manned NVIDIA arm that goes through OCCT `.brep`
    **[inf]**.
  - OCCT drops the extrusion magnitude.
  - In NIST CTC-04 two supplemental faces map to no shape and produce no
    message.
  - OCCT's shell `Closed` flag is forced to true.
- **G6.** The u parameter of a revolution, both torus angles and the angles in
  hyperbola and parabola parameters are in the plane-angle unit. OCCT does not
  unit-scale `offset_curve_3d` distances.
- **G9.** The 11 export files with no shape geometry are CAx-IF `s1-*`
  external-reference assemblies.
- **NVIDIA arm.** If the importer header is accurate, offsets reach the SDK as
  their basis surface (§3.3). Before an offset face is ever compared, the
  steel-man has to be checked.

## 9. Defects found

In our code at HEAD 627163b, for the dispatcher to file:

1. **`native/step_source/main.cpp:316`** (and `:313` for the UV pass) throws
   "non-edge UV source loop" on any `vertex_loop` bound. That refuses whole files
   that AP242 advanced-face WR6 and WR7 make legal. 14 export files are
   affected, including the NIST CTC-01/02/04/05, FTC-06 and STC-06/10 fixtures
   and the D2MI models [meas; code]. The refusal is explicit, not silent, but it
   is a gap in a reader that claims to read advanced B-reps.
2. **`native/step_source/main.cpp:202`** maps every Geom torus to carrier kind 5
   with no record of R ≤ r or of a source `degenerate_toroidal_surface`. The old
   decoder refuses such tori (`native/mesher/source_cache.cpp:276-281`), so
   nothing is silently wrong today. A new consumer of the CARRIERS table would
   inherit an unflagged singular chart. Recorded so the B1 / M8 table format
   carries the flag.
3. The void defect (`main.cpp:313, 332`) is already B1's.

No other code defect was found. Reference-implementation defects (OCCT, truck,
NVIDIA header) are listed in §3.3. They are findings about references, not
beads for this repository, except where they bias the NVIDIA arm (§8).

## 10. Operator questions

1. **Tessellated-only parts under D-shapes.**
   - D-shapes reads "main shape representation only; report the others", and
     the option text grouped "tessellated-only" with the others. A part whose
     only representation is AP242 tessellated (NIST FTC-08 `-tg`, 3
     `tessellated_solid` files) can either be reported and refused, or passed
     through as its own triangles, labelled "source tessellated, deviation
     unknown", with no ε claim.
   - Which? The lane did not settle it.
2. **ABC download term (already open).** This matrix is the clearest case so far
   for it: ABC's 15% "Other" patches decide whether general offsets and
   extrusions are rare or common (§2.4).

## 11. For SHOPPING_LIST.md

- ISO 10303-42 (current edition) normative text. It is already listed; this lane
  needs specifically:
  - the parameterizations of `hyperbola`, `parabola`, `offset_curve_3d`,
    `surface_curve_swept_surface` and `fixed_reference_swept_surface`;
  - the meaning of `degenerate_toroidal_surface.select_outer`;
  - whether `surface_of_linear_extrusion`'s v is scaled by the vector magnitude.
- AP203 ed.2 and AP214 longform EXPRESS, to confirm that the advanced-B-rep
  WHERE rules equal AP242's (§3.1). Openly published copies were not looked for
  in this pass.
- OCCT tracker issues 0033618-0033622 (degenerate torus import), 0029233 and
  0031672. The tracker host did not resolve; only search snippets were read.
- Piegl and Tiller, *The NURBS Book*, chapters 7-8 (conic and swept-surface
  conversion; paywalled). The non-angular parameterization is derived here
  instead (§4.1).
- Farouki 1986 (offset approximation by bicubic Hermite) and
  Patrikalakis–Prakash 1988 (plate offsets), both cited through PMC §11.3.7;
  Maekawa 1999, "An overview of offset curves and surfaces", CAD 31; Piegl and
  Tiller 1999, "Computing offsets of NURBS curves and surfaces", CAD 31.
  All paywalled; none read.
- Parasolid and ACIS documentation on offset and swept surface support, degree
  limits and sheet bodies (licensee or login only). This extends the existing
  Parasolid and ACIS rows.

## Appendix A. Commands (fleet-relative, from the BRepGPU root)

```sh
# lane setup and held sets (from the laptop: find ... > list; rsync --files-from=list ./ <fleet>:BRepGPU/lanes/M4-G5/held/)
python3 -I lanes/M4-G5/probe/census.py lanes/M4-G5/probe/files.tsv lanes/M4-G5/out/census.tsv
c++ -std=c++17 -O2 -I<occt-install>/include/opencascade lanes/M4-G5/probe/occt_census.cpp -o lanes/M4-G5/probe/occt_census -L<occt-install>/lib -lTKDESTEP -lTKDE -lTKXSBase -lTKShHealing -lTKTopAlgo -lTKGeomAlgo -lTKBRep -lTKGeomBase -lTKG3d -lTKG2d -lTKMath -lTKernel
lanes/M4-G5/probe/run_occt.sh          # one process per file, timeout 1800 s, exit status recorded
python3 -I lanes/M4-G5/probe/join.py out/census-final.tsv out/occt.tsv out/occt-dedup.tsv out/compare.tsv
python3 -I lanes/M4-G5/probe/aggregate.py out/census-final.tsv out/census-agg.tsv
python3 -I lanes/M4-G5/probe/bridge_admission.py out/census-final.tsv out/bridge-admission.tsv
python3 -I lanes/M4-G5/probe/matrix.py out/census-final.tsv > out/matrix.tsv
```

`<occt-install>` is the repository's existing OCCT 8.0.1 install on the fleet
host, the one `native/step_source` builds against.
