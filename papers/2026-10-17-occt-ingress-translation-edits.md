# What the OCCT 8.0.1 ingress bridge changes before we see the geometry

Research lane M4-G1 of the fast-mesher campaign, 2026-10-17. Brief:
[M4-G1](../docs/fast-mesher-campaign/briefs/M4-G1.md) with
[M4-common](../docs/fast-mesher-campaign/briefs/M4-common.md). Stub: lifecycle map
§4.1 G1 ([map](2026-10-17-geometry-lifecycle-map.md)). Bead `brepgpu-12s.2`.
Sources and read depth: [sources TSV](2026-10-17-occt-ingress-translation-edits-sources.tsv).

What ran: one read-only OCCT 8.0.1 probe (`g1-probe`, summarised in Appendix A),
built on the fleet against the repository's pinned OCCT install, over 915 STEP
files (the M3 corpus plus the held STEP sets). No meshing, no NVIDIA run, nothing
on the laptop except reading and editing this paper. Probe sources, raw per-file
outputs and summaries live in the fleet lane directory `lanes/M4-G1/` under the
BRepGPU root (`src/`, `out/`).

Tags: **[src]** read in a source at the cited place; **[meas]** measured by the
probe on the fleet; **[own]** this lane's derivation; **[code]** this repository.
OCCT paths are relative to `src/` of OCCT `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`
(8.0.1); `TKDESTEP/` means `DataExchange/TKDESTEP/`. The fleet's OCCT source tree
is byte-identical to the laptop quarantine copy for the four central files
(SHA-256 compared).

## 0. Answers in brief

1. **The bridge edits geometry with shape processing off, but on this corpus
   almost all of those edits have nothing to act on.** 98.7% of non-planar
   coedges (638,046 of 646,424) carry no pcurve in the file: every NIST, TDP,
   step-conformance and OM10 file writes 3D curves only [meas]. Where files do
   carry pcurves (67 files, mostly the OCCT regression set, Gmsh, CAx-IF), OCCT
   overrules the file on every planar coedge (8,309 dropped, all declared
   `master_representation = pcurve_s1/s2`) and on 1,817 of 8,377 non-planar ones
   (1,603 reparameterised, 81 replaced by a reprojection, 133 dropped) [meas].
   99.6% of non-planar coedges that have a file pcurve declare a pcurve master, so
   each of these overrules a declared master.
2. **Corrections to the M1 list.** OCCT does **not** recompute missing pcurves on
   the no-processing path: the 98.7% stay missing (BRepCheck calls 3,735 of 4,007
   translated solids invalid, 3,675 because of them) [meas, src
   `TKDESTEP/StepToTopoDS/StepToTopoDS_TranslateEdgeLoop.cxx:569-683`]. It only
   (a) evaluates a planar pcurve on demand (`BRep_Tool::CurveOnPlane`), never
   stored, and (b) reprojects an existing file pcurve that fails its check. No
   distinct vertex records were merged anywhere in the corpus (0 of 459,061)
   [meas]. Vertex coordinates are never moved (max change 0 mm) [meas].
3. **Placements: three OCCT defects, zero corpus frequency.** OCCT's reading of
   `cartesian_transformation_operator_3d` (used for assembly SRRWT and
   `mapped_item` placements) is wrong against ISO 10303-42 in three measured ways:
   uniform scale `S` becomes `1/S` with the origin divided by `S`; a mirrored
   operator becomes a rotation; an omitted `axis3` is taken as `axis1 × axis2`
   instead of the standard's default (0,0,1) [meas on authored operators; src
   `TKDESTEP/StepToGeom/StepToGeom.cxx:2153-2213`]. No file in the 915 contains a
   `cartesian_transformation_operator` at all (all 5,645 assembly placements are
   axis-pair `item_defined_transformation`s; all 495 `mapped_item`s use axis
   targets) [meas]. This settles B1's study item: the standard maps `P → A + S·T·P`
   (scale 2 enlarges); OCCT computes `(A + T·P)/S`.
4. **Authority rule** (§4): the raw Part 21 entity is authoritative for everything
   the file declares, including the declared master of each surface curve; OCCT's
   translated shape is only an evaluator and is never consumed where it disagrees
   with raw beyond the file's uncertainty; every disagreement is recorded per
   entity. Where the file declares nothing, our own construction fills the value
   with a recorded residual; OCCT reconstructions are not an authority.
5. **Steel-man** (§5): NVIDIA reads only `.brep`. Its strongest legitimate input
   is OCCT's default STEP read (FixShape), not our no-processing export: on this
   corpus the default arm leaves 0 non-planar faces without pcurves (against
   144,160) and 31 BRepCheck-invalid solids (against 3,735) [meas]. The current
   NVIDIA arm, "our unprocessed read", is therefore neither unprocessed (the
   translation edits above apply) nor NVIDIA's strongest arm: it disadvantages
   NVIDIA on curved faces without file pcurves, the exact path of the two watch
   refusals already diagnosed. Default processing has its own cost: edge
   tolerances grow to 0.108 mm on NIST parts and to 480 mm in one OCCT regression
   file, and 18 solids gain free edges [meas].
6. **Ingress cost** (§6): over 844 MB, OCCT ReadFile takes 89.9 s (9.4 MB/s,
   71% of the bridge's 127 s read and transfer); a lexing-and-number-conversion
   lower bound takes 2.8 s (about 32× faster); the raw-entity walk an override layer
   needs takes 2.5 s (2.7% of the parse); default shape processing multiplies
   transfer time by 18× (37 s to 662 s). On the watch: bridge 3.1 s, default 13.9 s,
   against NVIDIA's recorded 6.1-7.6 s for preparation plus meshing.

**Recommendation for M5's ingress section** [own]: keep the OCCT bridge for now
with raw-entity overrides (pcurves and their masters, placements, units per
context, representation selection, voids, closure), because every consequential
edit is either exact or concerns a quantity the raw walk reads cheaply; and move
the owned reader forward as an ingress-speed unit, because OCCT's parse, not the
translation, is the ingress cost and the stage we must beat NVIDIA at.

## 1. Method

### 1.1 Source audit

Every function on the STEP read path from `XSControl_Reader::ReadFile` through
`STEPCAFControl_Reader::Transfer` was read for edits that do not depend on the
ShapeProcess bitset: `STEPControl_ActorRead` (all `TransferEntity` overloads,
`PrepareUnits`, `ComputeTransformation`, `ComputeSRRWT`, `TransferRelatedSRR`),
`StepToTopoDS_Builder`, `_TranslateShell`, `_TranslateFace`, `_TranslateEdgeLoop`,
`_TranslateEdge`, `_TranslateVertex`, `_GeometricTool`, `_MakeTransformed`,
`_TranslateCompositeCurve` (the curve-bounded-surface path), all of
`StepToGeom.cxx`, `XSAlgo_ShapeProcessor::CheckPCurve`/`ProcessShape`, and the
healing calls they reach: `ShapeFix_Edge::FixSameParameter`/`FixAddPCurve`,
`ShapeFix_EdgeProjAux`, `ShapeBuild_Edge::RemovePCurve`,
`ShapeCustom_Surface::ConvertToPeriodic`, `Geom_BSplineCurve::SetPeriodic`,
`ShapeFix_ShapeTolerance::LimitTolerance`, `BRep_Tool::CurveOnPlane`, and
`gp_Trsf::SetScaleFactor`/`Inverted`. Read parameters and defaults come from
`TKDESTEP/DESTEP/DESTEP_Parameters.hxx:153-181`.

The standard text used: the AP242 MIM long form EXPRESS as shipped by stepcode
(`data/ap242/242_mim_lf.exp`, stepcode `24783b44`), which carries Part 42's
`base_axis`, `first_proj_axis`, `second_proj_axis`, `surface_curve`,
`trimmed_curve`, `cartesian_transformation_operator` and
`constraints_param_b_spline`; and buildingSMART's IFC transcription of the Part 42
definition text for `cartesian_transformation_operator` ("P → A + S T P"; "If
|T| = −1, the transformation includes mirroring"). Part 42 itself is paywalled
(already on SHOPPING_LIST.md through the map).

### 1.2 The probe

`g1-probe` (Appendix A) reads each file exactly as `native/step_source` does
(`STEPCAFControl_Reader` with name/colour/material/props modes, `ReadFile`,
`SetSystemLengthUnit(1.0)`, an empty ShapeProcess bitset set **after** read and
asserted before and after `Transfer`) [code `native/step_source/main.cpp:233-262`].
Modes:

- `audit`: entity-type histogram; per-context unit factors and uncertainty;
  raw-attribute triggers for every StepToGeom branch; placements recomputed from
  raw entities with Part 42 `base_axis` and compared with the translated
  locations; per-product representation counts and the content of each leaf
  product's translated shape; for every coedge of every face of every
  `manifold_solid_brep` (outer shell and voids) the file pcurve as OCCT itself
  decodes it before any edit (`StepToTopoDS_GeometricTool::PCurve` and
  `StepToTopoDS_TranslateEdge::MakePCurve`), whether the translated edge stores a
  pcurve on that face, and, when both exist, whether the stored one is
  identical, the same curve reparameterised, or a different curve (9 samples,
  period images considered); residuals; edge and vertex tolerances against the
  file precision; shell closure flags against counted free edges; BRepCheck per
  solid; every translation warning and fail.
- `audit-default`: the same reader with OCCT's default processing (the bitset is
  not touched, so `InitializeMissingParameters` installs `FixShape`
  [src `TKDESTEP/STEPControl/STEPControl_Reader.cxx:864-869`]); per-solid pcurve
  completeness, maximum edge tolerance, free edges, BRepCheck.
- `timing`, `timing-default`: §6.
- `cto-check`: the six authored `cartesian_transformation_operator_3d` cases of
  §2.4, fed to `StepToGeom::MakeTransformation3d`, compared with `A + S·T·P`.

Residual definitions [own]:

- *file residual*: for 9 points `P` of the 3D edge curve, project `P` to the
  surface (`ShapeAnalysis_Surface::ValueOfUV`), project that UV onto the file
  pcurve and its period images, take `|P − S(uv′)|`. `S(uv′)` lies on the
  pcurve's image, so this is an upper bound on the distance from the 3D curve to
  the file pcurve's image. It can overstate where the surface projection picks a
  wrong branch.
- *translated residual*: `ShapeAnalysis_Edge::CheckSameParameter(edge, face, 23)`
  on the translated edge, the same measure BRepCheck uses. On planes it measures
  the 3D curve against OCCT's computed projection, that is, the curve's distance
  from the plane.
- Ratios are against the file *precision*: the uncertainty of the first unit
  context times its length factor, which is what OCCT uses
  [src `STEPControl_ActorRead.cxx:2370-2384`]. File precisions span 1e-14 to 5 mm
  (median 0.01 mm). 109 files have contexts with differing factors or
  uncertainties (mostly presentation contexts); the first context was used for
  them.

### 1.3 Corpus and runs

915 STEP files: CAx-IF 53, NIST 71, TDP 376 (Ender-3 103, CR-10 1, Rat Rig 261,
Voron 2.4 4, Voron Trident 7, including both full-assembly files of 185 and
241 MB), and held sets copied into the lane: OCCT dataset 7.9.0 336, Gmsh 35
(gzipped ones decompressed), step-conformance 27, resources4truck 10, FreeCAD 4,
Netgen 2, OM10 1. The usd-brep test STEP files are Git LFS pointers in our
snapshot and were not available. ABC and Fusion 360 remain blocked on operator
decisions (corpus manifest).

All 915 files completed both audit arms with exit 0. Audit runs used 8 to 12
parallel processes pinned to one socket each; they report counts, not times.
Host state during the audits: load average 9 to 34 (other lanes' meshing and
Python jobs at 100% CPU each), governor `schedutil`, SMT on, AutoNUMA on, mean
reported clock 2.3 GHz.

Two corrections made during the run, both to the probe: a null `face_bound` in
`bug33665.step` crashed the raw walk (guarded); and BRepCheck itself segfaults
inside `BRepClass3d_SClassifier::Perform` on one solid in each Voron full
assembly, so BRepCheck now runs under `OCC_CATCH_SIGNALS` and counts the crash as
`brepcheck_exception` (2 solids).

## 2. The edits

### 2.1 Table

Columns: the trigger, the effect, where it lives, the gate (if any read
parameter controls it, with the default), corpus frequency as instances (files),
and the disposition under D-master, D-shapes, D-units and D-placements:
**accept** (exact or representation-only; use OCCT's value), **raw** (read the
raw entity and use it; record OCCT's disagreement), **refuse** (refuse the
entity with a record; the rest of the part proceeds, D-partial-output).
"Measured 0" means the probe counted the trigger and found none; "not measured"
means the probe had no counter.

**Pcurves and edge parameters** (all in `TKDESTEP/StepToTopoDS/StepToTopoDS_TranslateEdgeLoop.cxx`
unless noted)

| # | Edit | Trigger | Effect | Where | Gate (default) | Frequency | Disposition |
|---|---|---|---|---|---|---|---|
| E1 | Plane pcurves discarded | face surface is `Geom_Plane` | every file pcurve on a plane is removed after the wire is built; queries get `BRep_Tool::CurveOnPlane` projections of the 3D curve, never stored | `CheckPCurves` :110-113; `ModelingData/TKBRep/BRep/BRep_Tool.cxx:406-478` | none | 8,309 coedges (67 files), all declared pcurve masters; file residual ≤ 4.1e-4 mm, at most 0.03× precision | raw (D-master); the measured loss is negligible here, but the master is the pcurve |
| E2 | Pcurves not read when `read.surfacecurve.mode = 3` | parameter | 3D only | :588-592 | `ReadSurfaceCurveMode` (0: read) | 0 (default) | n/a; keep default |
| E3 | Missing pcurves left missing | edge geometry is a plain 3D curve, or a surface curve with no pcurve on this surface | no pcurve; ShapeProcess would add one; the no-processing path does not | :569-572, :667-683 | ShapeProcess (off) | 638,046 non-planar coedges (674 files), 98.7% of non-planar | raw + own construction: recompute with a recorded residual (D-master) [code `native/source_uv` does this, §7] |
| E4 | Pcurve dropped when its two parameters are equal | `w1 == w2` | removed | :138-145 | none | in E6 total | raw |
| E5 | Pcurve range clamped to the curve's bounds; periodic reorder | non-periodic pcurve range beyond its curve; `w1 > w2` on U-periodic surface | `B.Range` rewritten | :147-172 | none | in E7 total | raw |
| E6 | Pcurve dropped by `CheckPCurve` | pcurve spans more than 6/8 of the surface span ("multi-periodic"), or its ends are more than precision from the 3D ends | `ShapeBuild_Edge::RemovePCurve` | `TKXSBase/XSAlgo/XSAlgo_ShapeProcessor.cxx:361-401` | none | 133 coedges (6 files) | raw; the mismatch becomes a starting-condition record |
| E7 | Pcurve reparameterised to the 3D range | always on non-planes (`CheckPCurve` → `ShapeFix_Edge::FixSameParameter` → `TempSameRange`) and `ShapeFix_EdgeProjAux` vertex projection when every edge of the wire has a pcurve | an affine map makes the pcurve's range equal the 3D range; SameParameter stays asserted, its error goes into the tolerance | `XSAlgo_ShapeProcessor.cxx:412-445`; `ModelingAlgorithms/TKShHealing/ShapeFix/ShapeFix_Edge.cxx:798-936`; EdgeLoop :843-869 | `read.stdsameparameter.mode` (0) | 1,603 coedges (29 files): same curve, different parameterisation. After translation every stored pcurve range equals its 3D range (0 exceptions) | raw; our parameter map is a measured correspondence (B1 defect 2) |
| E8 | Pcurve replaced by a reprojection | the file pcurve's SameParameter deviation exceeds `min(1, 2·precision)`, and `FixAddPCurve` + `FixSameParameter` give a smaller tolerance | the file pcurve is overwritten by OCCT's projection of the 3D curve | `XSAlgo_ShapeProcessor.cxx:447-503` | none | 81 coedges (10 files); deviation from the file pcurve up to 2.54 mm | raw (D-master: the declared pcurve master stays); record the 3D-curve disagreement |
| E9 | Edge and vertex tolerances raised to the measured deviation | E7/E8 deviation larger than the edge tolerance | `UpdateEdge` tolerance, `UpdateVertex` | `XSAlgo_ShapeProcessor.cxx:491-494`; `ShapeFix_Edge.cxx:914-929` | none | edge tolerance over precision: >100× on 2,884 edges (51 files), mostly the 1e-7 floor against declared uncertainties of 1e-13..1e-10; real growth to 3.77 mm in `bm2_ec_exhaust-A` | accept as metadata only; tolerances never set density (design §5.1) |
| E10 | Seam pcurve assignment chosen geometrically | seam or "like-seam" (two faces on one closed surface) | `SelectForwardSeam` picks which file pcurve is the forward one; like-seam pcurves forced present | :616-641, :689-775 | none | 636 seam coedges with file pcurves (23 files) | raw: follow the file's associated-geometry order and senses; check against OCCT's choice |
| E11 | Vertex parameters on 3D curves come from projection | always | `U1, U2` = projection of the vertex points onto the 3D curve; out-of-range values clamped; closed curves wrapped; a reversed range may reverse the curve | `StepToTopoDS_TranslateEdge.cxx:441-449`; `StepToTopoDS_GeometricTool.cxx:227-408` | none | every edge; "Update of 3D-Parameters has failed" 8 (≤ 2 files) | accept for parameters (a STEP edge has no stored parameters); raw for the curve itself (no reversal) |
| E12 | Vertex tolerance raised to the vertex-to-curve distance | vertex farther than precision from its edge curve | tolerance ← 1.000001 × distance; warning "Poor result from projection vertex / curve 3d" | `StepToTopoDS_TranslateEdge.cxx:451-479` | none | 999 edges (≥ 28 files); vertex tolerance > 100× precision on 2,075 vertices (69 files) | accept as metadata; the distance is a starting-condition record (healing classes R1-R6) |
| E13 | Line moved onto its vertices | line displaced from both vertices by the same amount, direction matching (an FPX Expert 2013 exporter quirk, OCCT #25415) | the 3D carrier is translated | `StepToTopoDS_TranslateEdge.cxx:452-472` | none | 20 edges (8 files) | raw: never accept a moved carrier; record the discrepancy |
| E14 | Closed curve with two distinct vertices: second vertex rebound to the first | `DifferentPointsOnClosedCurve` from `BRepLib_MakeEdge` | topology: two vertex records become one | `StepToTopoDS_TranslateEdge.cxx:489-504` | none | measured 0 (no distinct-record merges anywhere) | raw topology; let reconciliation decide |
| E15 | Null-arc edge replaced by a line, or skipped | non-closed curve and coincident vertices | a new straight carrier, or the edge disappears | `StepToTopoDS_TranslateEdge.cxx:513-541` | none | "This edge has null arc length" 2 (1 file); skipped 0 | refuse the edge with a record |
| E16 | Coincident vertices merged inside a loop | edge ends within `Precision::Confusion()`, or adjacent edges' ends within precision | `aTool.Bind` rebinds one vertex record to the other | :380-401, :410-491 | none | 0 distinct-record merges of 459,061 vertices; 8 "set confused" warnings (1 file) are edges whose start and end are already one record | raw topology; record |

**Curves and surfaces** (`TKDESTEP/StepToGeom/StepToGeom.cxx` unless noted)

| # | Edit | Trigger | Effect | Where | Gate | Frequency | Disposition |
|---|---|---|---|---|---|---|---|
| G1 | `surface_curve.master_representation` ignored | any surface curve | the 3D curve is always the edge geometry; pcurves are kept, edited or dropped per E1-E10. Grep finds no read of `SurfaceCurve::MasterRepresentation` on the translation path | absent from `TKDESTEP/StepToTopoDS/*`; only `trimmed_curve`'s master is read (:2340) | none | 16,654 coedges with a pcurve master and a pcurve on this face (70 files); 982 with curve_3d master and no pcurve here | raw (D-master); truck-stepio honours it (`truck-stepio/src/in/mod.rs:1700-1725, 2543-2570`) |
| G2 | B-spline repeated knot values merged | adjacent knots within `Epsilon` | multiplicities summed | :785-814 (curves), :1015-1083 (surfaces) | none | measured 0 | accept (exact) |
| G3 | B-spline end multiplicity > degree+1 clamped, end poles dropped | schema-invalid knots (`constraints_param_b_spline`) | poles with identically zero basis removed | :818-860 | none | measured 0 | accept (exact) [own] |
| G4 | B-spline read as periodic | knot sum = poles + 1 (schema-invalid in Part 42) | periodic curve | :879-893, :1087-1115 | none | measured 0 | refuse (schema-invalid; §7) |
| G5 | Closed B-spline curve forced periodic | `closed_curve = .T.`, degree > 1, ends within `Precision::Computational` | `SetPeriodic`: end multiplicity → degree, last pole dropped; geometry inside the domain unchanged [own] | :921-926; `ModelingData/TKG3d/Geom/Geom_BSplineCurve.cxx:777-815` | none | ≤ 1,810 curves (102 files) carry the trigger flags | accept (representation only) |
| G6 | Closed B-spline surface forced periodic | surface closed within 1e-7 | `ShapeCustom_Surface::ConvertToPeriodic`: knots rearranged, `SetUPeriodic`; warning "Surface forced to be periodic" | `StepToTopoDS_TranslateFace.cxx:556-566`; `ShapeCustom_Surface.cxx:475-600` | none | 2,733 surfaces (≥ 59 files) | accept; change bounded by the closure gap (≤ 1e-7) [own] |
| G7 | Bezier, uniform, quasi-uniform B-splines get generated knots | those entity types | knots 0..n with end multiplicity d+1 (quasi) or 1 (uniform) | :296-468, :522-700 | none | quasi-uniform curves 1,316 (30 files), surfaces 263 (24); no Bezier or uniform | accept, but the Part 42 knot values are in its paywalled text, not its EXPRESS: §7 |
| G8 | Polyline becomes a degree-1 B-spline | polyline | knots 0..n−1 | :1751-1832 | none | 17,808 polylines (23 files), mostly outside B-rep | accept |
| G9 | Line direction normalised; trims rescaled by magnitude | `vector.magnitude ≠ 1` | parameter scale changes; trimmed-curve trims multiplied by the magnitude, pcurve parameterisation re-mapped by E7 | :1649-1693, :2378-2381, :2533-2538 | none | 372,180 3D lines (574 files), 2,549 2D (30 files) | accept; our raw parameter map must apply the magnitude |
| G10 | Ellipse with `semi_axis_1 < semi_axis_2` swapped | legal per schema | axes swapped, X rotated, trims shifted by π/2 | :1552-1561, :2386-2392 | none | measured 0 | accept (exact reparameterisation) |
| G11 | Trimmed conic without `ref_direction` becomes the full curve | conic placement lacks ref_direction, trimmed by parameter | trims ignored, 0..2π | :2396-2421 | none | measured 0 | refuse |
| G12 | Trims clamped; equal trims widened or curve dropped | trims outside a bounded basis; equal trims | clamp; full period; null | :2439-2475 | none | not measured; 5,924 trimmed curves (114 files) carry trims | raw |
| G13 | Trimmed-curve master | master `cartesian`, `parameter`, `unspecified` | parameter preferred when unspecified; points projected otherwise | :2221-2320, :2336-2349 | none | cartesian 677, parameter 2,096, unspecified 3,151 | raw: follow the declared master; for unspecified, parameter if present (OCCT's rule), recorded |
| G14 | 2D trimmed curve on a B-spline returned untrimmed; point-trimmed 2D curves dropped | pcurve given as a trimmed curve | trims ignored or pcurve lost | :2503-2563 | none | not measured | raw |
| G15 | Cone semi-angle clamped | `semi_angle ≤ 0` | set to 1e-12 rad | :1308-1323 | none | measured 0 | refuse |
| G16 | Torus radii made positive, face sense flipped | `major_radius < 0` (schema-invalid) | `abs()`, wires reversed (OCCT #26376 SolidWorks/ProE convention) | :2104-2118; `StepToTopoDS_TranslateFace.cxx:479-491, 568-571` | none | 2 surfaces (1 file) | refuse (invalid), operator may choose accept-with-record |
| G17 | `degenerate_toroidal_surface.select_outer` dropped | that entity | read as a plain torus | (no handler: falls to `MakeToroidalSurface`) | none | 5 surfaces (2 files), all `select_outer = .T.` | raw (owned evaluation) or refuse |
| G18 | Revolved conic centred on the axis trimmed to a half | circle/ellipse centred on the axis, plane containing it | sweep curve repositioned, trimmed to [0, π] | :2037-2080 | none | measured 0 | accept (removes double cover) |
| G19 | Extrusion of a line along itself returns null | degenerate | face lost ("Surface has not been created") | :2007-2030 | none | not observed | refuse |
| G20 | Offset surface on a C0 basis converted | basis continuity C0 | `C0ShapeToC1Shape` approximates the basis | :1939-1966 | none | 0 offset surfaces in corpus | refuse (approximation without a bound) |
| G21 | Curve and surface replicas transformed through P1 | replica entities | uses `MakeTransformation3d` (see P1) | :1351-1370, :1967-1990 | none | 0 replicas | raw |
| G22 | Axis placement with parallel ref_direction | invalid | default X | :169-216 | none | not measured | refuse |

**Faces, shells, solids**

| # | Edit | Trigger | Effect | Where | Gate | Frequency | Disposition |
|---|---|---|---|---|---|---|---|
| T1 | Shell `Closed` flag set without a check | every `manifold_solid_brep` outer and every void | `Closed(true)` regardless of free edges | `StepToTopoDS_Builder.cxx:150, 217, 235` | none | all 4,121 shells flagged; 6 of them (6 files) have free edges | raw: closure is computed, never read from the flag |
| T2 | Faces that fail to translate are skipped | face translation fails | warning only; shell loses the face | `StepToTopoDS_TranslateShell.cxx:85-104` | none | 5 non-face-surface faces (1 file); 0 failed advanced faces | refuse face with record (our adapter refuses the file today) |
| T3 | A failed edge loop drops the wire | any edge of a loop fails | the face keeps its other wires; a hole can vanish | `StepToTopoDS_TranslateFace.cxx:727-743` | none | 0 observed | refuse face |
| T4 | Vertex loops become degenerate-edge wires; a sole vertex loop on a sphere, B-spline or revolution gives natural bounds; on planes and tori it is ignored | `vertex_loop` bounds | wire added, or ignored | `StepToTopoDS_TranslateFace.cxx:609-662` | none | 176 faces (27 files) gained a wire against their edge loops (cone apices in NIST, sphere poles) | accept (apex/pole representation); refuse the plane and torus cases |
| T5 | Voids kept | `brep_with_voids` | void shells added, reversed per orientation | `StepToTopoDS_Builder.cxx:179-249` | none | 34 solids (23 files), 118 void shells | accept OCCT; our adapter drops them (B1 defect 1) |
| T6 | Tolerance limiting | `read.maxprecision.mode = 1` | `LimitTolerance` | `StepToTopoDS_Builder.cxx:96-106` | mode 0 (off) | 0 | n/a |
| T7 | Vertex and edge tolerance floor | always | tolerances start at `Precision::Confusion()` (1e-7 mm), not at the file's uncertainty | `StepToTopoDS_TranslateVertex.cxx:108`; `StepToTopoDS_TranslateEdge.cxx:165` | none | every vertex and edge | accept as metadata; uncertainty is read per context from raw |
| T8 | Composite boundary curves reordered | curve-bounded surfaces | `ShapeFix_Wire` reorders segments | `StepToTopoDS_TranslateCompositeCurve.cxx:300-312` | none | 20 (≤ 8 files) | refuse: outside the admitted advanced-face set |

**Units, placements, products** (`TKDESTEP/STEPControl/STEPControl_ActorRead.cxx` unless noted)

| # | Edit | Trigger | Effect | Where | Gate | Frequency | Disposition |
|---|---|---|---|---|---|---|---|
| U1 | Units applied per representation context | every context | length factor on points, radii, offsets; plane-angle factor on cone angles, conic trims, rectangular-trim parameters (cone V by length/cos α), and pcurves on periodic surfaces (`GeomConvert_Units::DegreeToRadian`) | `PrepareUnits` :2285-2395; `StepToGeom.cxx:1308-1323, 1834-1890, 2384-2387`; `StepToTopoDS_TranslateEdge.cxx:576-579` | `read.step.angleunit` (file) | non-mm length units in 261 files (inch 210, cm 29, m 20, foot 2), plus 2 files with an unrecognised unit and 1 with none; degree contexts in 253 files | raw: honour declared units (D-units). Our adapter refuses all 264 today [code `native/step_source/main.cpp:239-249`] |
| U2 | Missing plane-angle unit taken as degrees | context without plane angle | factor π/180 | `TKDESTEP/STEPConstruct/STEPConstruct_UnitContext.cxx:280-281` | none | not separately measured | refuse unless the caller states units (D-units) |
| U3 | Entities with no reachable context read at factor 1 | entity not under a representation | mm and radians assumed silently (warning) | :1803, :2130 (items and face surfaces) | none | 342 entities (9 files, all AP242 PMI test files; the entities are `plane`s outside every shape representation the actor reached); 3 of the files are in inches, so those planes are read 25.4× too small | refuse (D-units) |
| U4 | Missing uncertainty → precision 1e-4 | context without uncertainty | precision drives E6, E8, E12, E16 thresholds | :2373-2383; `DESTEP_Parameters.hxx:156` | `read.precision.mode` (file) | 363 contexts (35 files) | record "uncertainty absent"; our thresholds come from ε (design) |
| P1 | `cartesian_transformation_operator_3d` read wrongly | any CTO in SRRWT, `mapped_item`, replicas | scale S becomes 1/S and the origin is divided by S; mirrors become rotations; omitted `axis3` defaults to `axis1×axis2` | `StepToGeom.cxx:2153-2213`; callers `STEPControl_ActorRead.cxx:2019, 2522`; `StepToTopoDS_MakeTransformed.cxx:81` | none | 0 CTO entities in 915 files [meas] | raw (D-placements); defect of OCCT, §8 |
| P2 | Axis-pair placements | `item_defined_transformation` | rigid transform from two `axis2_placement_3d`; always det +1 | `ComputeTransformation` :2408-2493 | none | 5,645 NAUO placements (134 files); 0 with OCCT scale ≠ 1 or det < 0 | accept |
| P3 | Swapped SRRWT placements corrected | each placement belongs to the other representation | swapped, warning | :2448-2463 | none | 43 (5 files); 54 more (4 files) belong to neither and are used as given | raw: apply the same rule explicitly, recorded |
| P4 | Root representation placement applied | a standalone `axis2_placement_3d` item in a root shape representation | the root shape is moved | `TransferEntity(ShapeRepresentation)` :1087-1110; root flag :405 | `read.step.root.transformation` (on) | not measured | raw; our adapter reads solids and usages directly, not the moved root |
| R1 | All representations merged into the product shape | a product with several SDRs, shape aspects, or reps related by plain `shape_representation_relationship` | one compound; tessellated and alternative representations included | `TransferEntity(ProductDefinition)` :617-857; `TransferRelatedSRR` :2679-2729 | `read.step.shape.repr` (All), `read.step.shape.relationship` (on), `read.step.shape.aspect` (on), `read.tessellated` (on) | 207 leaf products (64 files) mix a solid with other content; 138 products have ≥ 2 plain relationships; 13 have ≥ 2 SDRs; 6 have triangulation-only faces | raw (D-shapes): select the main representation; report the others |
| R2 | Product with both components and its own shape | assembly with directly assigned shape | merged | :838-840 | none | 208 (4 files) | raw; report |
| R3 | Missing external references reported as fails and skipped | `document_file` references | transfer continues | `TKDESTEP/STEPConstruct/STEPConstruct_ExternRefs.cxx:518` | none | 70 fails (17 files: CAx-IF s1 and other multi-file sets) | refuse the product with a record |

### 2.2 Pcurves in numbers

| Set | Files | Non-planar coedges | No pcurve in file | Plane pcurves dropped (E1) | Identical | Reparam. (E7) | Replaced (E8) | Dropped (E6) | Seam with file pcurve |
|---|---|---|---|---|---|---|---|---|---|
| All | 915 | 646,424 | 638,046 | 8,309 | 5,922 | 1,603 | 81 | 133 | 636 |
| OCCT dataset | 336 | 50,719 | 43,275 | 7,837 | 5,418 | 1,385 | 66 | 48 | 524 |
| Gmsh | 35 | 12,465 | 11,736 | 201 | 316 | 211 | 15 | 85 | 102 |
| CAx-IF | 53 | 2,920 | 2,752 | 228 | 168 | 0 | 0 | 0 | 0 |
| Netgen, resources4truck | 12 | 1,148 | 1,111 | 43 | 20 | 7 | 0 | 0 | 10 |
| NIST, TDPs, step-conformance, FreeCAD, OM10 | 479 | 579,172 | 579,172 | 0 | 0 | 0 | 0 | 0 | 0 |

Residuals against the file precision, coedges [meas]:

| Measure | ≤ 1 | ≤ 2 | ≤ 10 | ≤ 100 | > 100 |
|---|---|---|---|---|---|
| File pcurve, non-planar (upper bound) | 7,998 | 41 | 178 | 46 | 96 |
| Translated, non-planar (stored pcurves) | 7,993 | 53 | 161 | 21 | 16 |
| Translated, planar (3D curve off its plane) | 778,075 | 49 | 85 | 175 | 98 |
| File pcurve, planar | 8,309 | 0 | 0 | 0 | 0 |

So: dropping file pcurves on planes (E1) loses nothing measurable on this corpus
(worst 4.1e-4 mm, 0.03× precision). The non-planar repairs (E7, E8) do lower the
3D-versus-pcurve disagreement OCCT reports, at the price of replacing the
declared master. 407 planar coedges have a 3D curve more than one precision off
its plane; their only pcurve is OCCT's projection, so the disagreement is
invisible downstream unless it is measured. The largest file residuals (56 mm,
913 mm) are in known-bad OCCT regression files (`bm2_ec_exhaust-A`,
`trj4_k1_top-pe-214`) and may be inflated by the surface-projection upper bound.

### 2.3 Default processing (the NVIDIA arm's candidate input)

| Set | Solids | BRepCheck invalid, no processing | BRepCheck invalid, default | Max edge tolerance mm, no processing | Max edge tolerance mm, default |
|---|---|---|---|---|---|
| All | 4,007 | 3,735 | 31 | 3.77 | 479.7 |
| NIST | 70 | 70 | 0 | 1e-7 | 0.108 |
| OM10 | 107 | 107 | 0 | 1e-7 | 0.0098 |
| Ender-3 | 360 | 360 | 0 | 1e-7 | 0.025 |
| Rat Rig | 261 | 261 | 0 | 1e-7 | 0.060 |
| Voron 2.4 | 1,566 | 1,509 | 11 | 1e-7 | 0.030 |
| OCCT dataset | 712 | 572 | 15 | 3.77 | 479.7 |

Non-planar faces without a stored pcurve: 144,160 with no processing, 0 with
default processing. Solids with free edges: 6 and 18. Default processing also
changes the face count (270,760 against 270,726). BRepCheck needs a pcurve per
edge use, so "invalid" in the no-processing arm is chiefly "pcurves missing"
(3,675 of the 3,735 solids have such a face).

### 2.4 The CTO check (settles B1's item)

`cto-check` feeds six authored `cartesian_transformation_operator_3d` values to
`StepToGeom::MakeTransformation3d` and maps P = (1, 2, 3) [meas,
`lanes/M4-G1/out/cto-check.tsv`]:

| Case | Part 42 `A + S·T·P` | OCCT | Distance |
|---|---|---|---|
| translate (10, 20, 30) | (11, 22, 33) | (11, 22, 33) | 0 |
| rotate 90° about Z | (8, 1, 3) | (8, 1, 3) | 0 |
| scale 2, origin (10, 0, 0) | (12, 4, 6) | (5.5, 1, 1.5), scale factor 0.5 | 8.46 |
| mirror: axis2 negated (det −1) | (1, −2, 3) | (1, 2, 3), det +1 | 4 |
| mirror: axis3 negated (det −1) | (1, 2, −3) | (1, −2, −3), det +1 | 4 |
| axis3 omitted, axis2 = (0, 1, 1) | (1, 2, 3) | (1, −0.707, 3.536) | 2.76 |

Standard: `cartesian_transformation_operator` derives `scl := NVL(scale, 1.0)`
with `WR1: scl > 0.0`, and `cartesian_transformation_operator_3d` derives
`u := base_axis(3, axis1, axis2, axis3)`, where `base_axis` defaults `axis3` to
(0, 0, 1) and takes `u[2]` as the projection of `axis2`, keeping its sense
(AP242 MIM long form; the same functions in IFC4). The definition text: "P → A +
S T P", "If |T| = −1, the transformation includes mirroring" (IFC transcription
of ISO/CD 10303-42). OCCT builds `gp_Ax3(origin, axis3 or axis1×axis2, axis1)`,
which is always right-handed and ignores `axis2` when `axis3` is present; sets
`SetScaleFactor(S)` on the global-to-local transform, whose translation part is
not scaled; then inverts it [src `StepToGeom.cxx:2192-2210`; `gp_Trsf.cxx:196-204,
284-320`]. The inversion turns `x′ = S·R·x − R·A` into `x = Rᵀx′/S + A/S`. The
standard intends enlargement for S = 2. Assembly practice forbids scale and
mirror in assemblies anyway ("Scaling and mirroring shall not be applied in the
context of assemblies", PDM Schema Usage Guide 4.3 §4.7.2, via the map), which
explains why exporters rarely write them. Frequency on this corpus: zero CTO
entities, so zero firings; G9's corpus finding agrees.

## 3. What the map said, checked

| Map claim (finding 1, L2) | Result |
|---|---|
| drops file pcurves on planes | confirmed (E1); 8,309 coedges |
| recomputes missing pcurves | **not on this path**: missing stay missing (E3); OCCT reprojects only existing pcurves that fail `CheckPCurve` (E8), and evaluates planar ones on demand |
| projects vertex parameters | confirmed (E7 via `ShapeFix_EdgeProjAux`, E11) |
| ignores `master_representation` | confirmed for surface curves (G1); trimmed curves honour theirs (G13) |
| merges coincident vertices | code confirmed (E14, E16); **0 instances** on the corpus |
| mirrored CTO read as rotation | confirmed and extended: scale is also inverted and the axis3 default is wrong (P1); **0 instances** |
| accepts scaled locations | confirmed, with the wrong scale (P1) |
| sets shells `Closed` unchecked | confirmed (T1); 6 shells are open |
| merges alternative and tessellated representations | confirmed (R1); 207 leaf products |

## 4. Dispositions and the authority rule

D-master says follow the declared master, and with no pcurves recompute them with
a recorded residual. D-shapes says main representation only, report the others.
Reading the table under both, the bridge's edits fall into four groups [own]:

- **Exact or representation-only (accept):** G2, G3, G5, G6, G7, G8, G9, G10,
  G18, P2, T4 (apex and pole cases), T5, and parameters from projection (E11).
  Keeping OCCT here costs nothing.
- **Metadata OCCT computes that we do not consume (accept as record):** E9, E12,
  T7, U4. Tolerances never set density; the vertex-to-curve and pcurve-to-curve
  distances are starting-condition records for reconciliation.
- **Overrides that must read raw:** E1, E3-E8, E10, E13, E14, E16, G1, G12-G14,
  G17, G21, U1, P1, P3, P4, R1, R2, T1. They concern exactly the quantities the
  map already said the raw walk must read (pcurves and their masters, placements,
  units, representation selection, closure).
- **Refuse per entity with a record:** E15, G4 (no instance), G11, G15, G16,
  G19, G20, G22, T2, T3, T4 (plane and torus vertex loops), T8, U2, U3, R3.

**Authority rule** [own, grounded in D-master and Part 42's master semantics as
quoted in the map]:

1. The raw Part 21 entity is the authority for everything the file declares:
   topology and identity, vertex points, carriers and their parameterisation,
   units and uncertainty per context, placements, representation selection, and
   for every surface curve the declared `master_representation`.
2. OCCT's translated shape is an evaluator. A translated value is consumed only
   where it agrees with the raw value within the file's uncertainty for that
   context. Where they disagree, raw wins and the disagreement is written per
   entity: kind (E-, G-, U-, P-, R-, T- id above), magnitude, and both values.
3. When raw declares a master and its partner disagrees with it beyond the
   uncertainty, the master wins; the partner's discrepancy is a starting-condition
   metric for reconciliation and is never charged against ε (journal: error is
   measured against the repaired coherent target).
4. When the file declares nothing (no pcurve; unspecified trimming master), our
   own construction produces the value with a recorded residual. OCCT's
   reconstructions (E1's on-demand projection, E7, E8) are not an authority; they
   may serve as a cross-check.

Falsifiable form: for every override class, an authored fixture where OCCT's
value and the raw value differ by more than the uncertainty must produce the raw
value in our source store and a record naming the class; the arena checks the
meshed boundary against the raw master, not against OCCT's shape.

## 5. The steel-man

**What NVIDIA reads.** `occt_to_usd` takes `<input.brep> <output.usda|usdc>`
and calls `ConvertOcctFileToUsd`, which accepts binary or ASCII `.brep` only
[src usd-brep `c9c5979`, `tools/occt_to_usd/occt_to_usd.cpp:11-38`,
`source/OCCT_BREP_IMPORT/src/OcctFileConverter.cpp:56-148`]. The package has no
STEP reader; its README mentions STEP only as LFS test fixtures. A STEP user
reaches NVIDIA through OCCT. Our NVIDIA arm today is
`native/reference_source`, which writes each translated solid with the
no-processing reader [code `native/reference_source/main.cpp:110-128, 175-201`;
`scripts/benchmark_om10_nvidia.py:63-71`], followed for the watch by the documented
plane-range and periodic angle-split preparations
([NVIDIA reference preparation](../docs/NVIDIA_REFERENCE_PREPARATION.md)).

**What NVIDIA does with pcurves.** Planar faces use projected 3D topology and do
not consult stored pcurves ("Planar faces remain general because their
outer-loop selection and domain come from projected 3D topology") [src
`OcctStagingDriver.cpp:1375-1378, 2274-2281`]. Curved faces use stored pcurves
when present; without them the driver samples 3D curves, and repeated periodic
seams survive only with complete pcurves [src `OcctStagingDriver.cpp:1247-1300,
2346-2353`; our diagnosis
[2026-10-17-nvidia-periodic-input-usage-diagnosis](2026-10-17-nvidia-periodic-input-usage-diagnosis.md)].
So E1 does not touch NVIDIA, and E3 (missing pcurves) does.

**Strongest legitimate arm.** OCCT's default STEP read is what any OCCT user
gets, and the default arm leaves no curved face without pcurves on this corpus
(§2.3). It is legitimate: it is OCCT's own documented default, not our work. A
third arm, our own projected pcurves written into the `.brep`
(`native/step_source --query-brep-dir`, which also adds query-only constructions), gives NVIDIA our ingress work and must
be labelled as such if used.

**Does the bridge disadvantage or advantage NVIDIA?** [meas + own]

- Disadvantage: the no-processing arm hands NVIDIA 144,160 curved faces without
  pcurves and 3,735 BRepCheck-invalid solids; the two watch refusals already
  diagnosed come from this path.
- Neutral: E1 (NVIDIA ignores planar pcurves); E7, E8, E9 apply to both owned and
  NVIDIA input, since both read through the same translation.
- Possible advantage to NVIDIA, against fidelity: default processing raises
  tolerances (0.108 mm on NIST parts, above NIST FTC-07's whole 0.076 mm band;
  480 mm in one file). If NVIDIA's chord rule or healing respects stored
  tolerances, a looser input can make it faster. Our scoring already keeps
  measured deviation beside NVIDIA's time and never slows it down (design §9.6).

**Statement.** "Our unprocessed read" is mislabelled: rename it "OCCT
translation, no ShapeProcess". It is not NVIDIA's strongest arm. NVIDIA's arms
should be: default OCCT read; OCCT translation without ShapeProcess; and,
labelled, our projected pcurves; each crossed with heal 0/1. NVIDIA's time is its
fastest valid arm, with default-processing time counted in its import stage.
Measurement that settles which arm wins: run the official converter and
`SmApiTessellate` on all three arms over this corpus, pinned per socket (an
NVIDIA run, outside this lane's brief).

## 6. Ingress cost

**Scope of each time** [meas]. One fresh process per measurement, three
repetitions per file and mode, median reported. All 915 files completed both
modes.

- `lex`: read bytes already in memory; split the DATA section into records,
  count references and strings, convert every number with `std::from_chars`. It
  builds no entity. This is a **lower bound** on an owned reader's lexing and
  number conversion, not an owned reader.
- `readfile`: `STEPCAFControl_Reader::ReadFile` (OCCT's Part 21 parse into typed
  `StepData` entities).
- `rawwalk`: the walk an override layer needs over OCCT's parsed model (every
  solid's faces, loops, oriented edges, vertex points, surface-curve pcurve
  references and masters, assembly placements).
- `transfer`: `Transfer` into an XDE document with the empty bitset (the
  bridge), or with OCCT's default processing (NVIDIA's candidate arm).

Lane and context: one logical CPU (47, socket 1) with memory bound to node 1
(`numactl`), serial. Load average 3.4 to 41 during the run (other lanes' meshing
and analysis jobs; heaviest early in the run), governor `schedutil`, pinned-CPU
clock 1.2 to 3.5 GHz between processes, SMT on, AutoNUMA on. Warm page cache
(every file had been read by the audit passes); fresh process each time.
Cold-disk runs were not measured: dropping the page cache needs root. Spread
between repetitions is up to about 40% on single files (CR-10 ReadFile 4.5 to
6.3 s), so treat per-set sums to two significant figures.

| Set | Files | MB | lex s | ReadFile s | raw walk s | transfer, bridge s | read + transfer, bridge s | transfer, default s | read + transfer, default s | ReadFile ÷ lex | ReadFile MB/s |
|---|---|---|---|---|---|---|---|---|---|---|---|
| All | 915 | 844.2 | 2.77 | 89.9 | 2.46 | 37.1 | 127.0 | 662.3 | 754.7 | 32 | 9.4 |
| NIST | 71 | 71.5 | 0.31 | 8.18 | 0.16 | 2.85 | 11.0 | 23.8 | 32.3 | 26 | 8.7 |
| CAx-IF | 53 | 4.8 | 0.025 | 0.97 | 0.022 | 0.47 | 1.44 | 6.0 | 7.0 | 38 | 5.0 |
| OCCT dataset | 336 | 121.8 | 0.40 | 10.6 | 0.27 | 4.80 | 15.4 | 49.5 | 60.4 | 27 | 11.5 |
| Ender-3 | 103 | 99.0 | 0.36 | 10.9 | 0.28 | 3.52 | 14.4 | 51.0 | 62.3 | 30 | 9.1 |
| Rat Rig | 261 | 32.7 | 0.11 | 4.06 | 0.13 | 2.31 | 6.36 | 24.2 | 28.3 | 36 | 8.1 |
| Voron 2.4 | 4 | 241.6 | 0.73 | 25.3 | 0.79 | 11.8 | 37.1 | 143.0 | 169.5 | 35 | 9.6 |
| Voron Trident | 7 | 187.9 | 0.56 | 20.0 | 0.54 | 7.16 | 27.2 | 304.9 | 325.7 | 36 | 9.4 |
| OM10 watch | 1 | 18.8 | 0.055 | 2.12 | 0.064 | 0.94 | 3.06 | 11.8 | 13.9 | 38 | 8.9 |

Per file, ReadFile is 10 to 43 times the lexing lower bound. The remaining sets and the
per-file medians are in `lanes/M4-G1/out/timing-summary.tsv` and `timing-perfile.tsv`.

What the numbers say [meas + own]:

1. OCCT's Part 21 parse is the ingress cost: 71% of the bridge's read and
   transfer on the corpus, at about 9 MB/s, single-threaded (OCCT has no parallel
   STEP parsing; the map cites Foxtrot at about 60 MB/s in parallel). The lexing
   bound runs at about 300 MB/s on one core, so an owned reader has roughly a 30×
   head-room on this stage before typed-entity construction, and more with
   parallel record splitting.
2. Raw-entity overrides cost about 2.7% of ReadFile when built on OCCT's parsed
   model. Overriding raw values on top of the bridge is cheap; it does not need
   the owned reader.
3. OCCT's default processing multiplies transfer time by 18× (37 s to 662 s on
   the corpus; 0.94 s to 11.8 s on the watch). In the steel-manned comparison
   that time belongs to NVIDIA's import stage (design §9.6). For scale: the
   recorded NVIDIA watch preparation plus SDK batch is 6.1 to 7.6 s (journal), so
   OCCT's default read of the watch alone (13.9 s) costs about twice NVIDIA's
   recorded preparation plus meshing.
4. Our bridge's read and transfer of the watch is 3.1 s; the adapter's own pcurve
   projection (`source_uv`) comes on top and was not timed here. Beating NVIDIA's
   ingress requires the owned parse, not more work on the translation.

## 7. What the literature and code do not settle, and the measurements that would

- **Part 42 knot values for quasi-uniform and uniform B-splines** (G7). The
  EXPRESS declares the subtypes without knots; the values are in the text of ISO
  10303-42 (paywalled). Settle by reading the standard (shopping item) or by a
  CAx-IF round-trip record. 1,579 entities in the corpus depend on it.
- **Negative torus radius** (G16): OCCT follows a SolidWorks/ProE convention
  (OCCT #26376) that no standard states. Refusing is safe; whether to accept is
  the operator's call (2 instances).
- **Periodic-pattern B-splines** (G4): schema-invalid, no instance; refuse by
  default.
- **Our own reconstruction residuals.** D-master requires a per-coedge residual
  for every recomputed pcurve. `native/source_uv` records only maxima per solid
  (`SourceUVStats`) and the 17-point check [code
  `native/source_uv/source_uv.hpp:28-35`]; a per-coedge record is a gap for M5,
  not a defect (D-master postdates the code).
- **Shell-based surface models** (660 in 119 files) and their faces were not
  audited coedge by coedge; the probe walks manifold solids only.
- **Whether NVIDIA benefits from default processing**: the NVIDIA run above.

## 8. Defects

In OCCT (upstream, not ours; for the record and for an upstream report):

1. `StepToGeom::MakeTransformation3d` (`TKDESTEP/StepToGeom/StepToGeom.cxx:2153-2213`)
   inverts uniform scale and scales the origin, loses mirroring and uses
   `axis1×axis2` instead of (0, 0, 1) for an omitted `axis3` (§2.4).
2. `BRepCheck_Analyzer` segfaults in `BRepClass3d_SClassifier::Perform` from
   `BRepCheck_Solid::Minimum` on one solid of each Voron full assembly, translated
   without processing.

In our code (for beads):

3. `native/step_source/main.cpp:239-249` refuses every non-millimetre file: 264
   of 915 corpus files (28.9%: 210 inch, 29 cm, 20 m, 2 foot, 3 with an unrecognised or no unit), against D-units ("honour
   declared units"). B1 handles the plane-angle unit (defect 3) but not this.
4. `native/step_source/main.cpp:277` and `native/reference_source/main.cpp:144`
   keep one uncertainty, the maximum over all contexts; 109 corpus files have
   contexts with different uncertainties or factors (already noted in the map,
   still present).
5. `native/step_source/main.cpp:233-262` and `native/reference_source/main.cpp:110-128`:
   the label `shape-processing=none` (`main.cpp:423`, reference `:203`) is true of
   the ShapeProcess bitset but not of the translation, which still applies E1,
   E6-E10, E12, E13 and G5-G6; the label should name the translation edits that
   can fire, or the adapter should record them.
6. `native/step_source/main.cpp:400` takes every pcurve from OCCT
   (`BRep_Tool::CurveOnSurface`), so the COEDGES table carries OCCT's planar
   projections (E1), reparameterised (E7) and replaced (E8) pcurves instead of the
   file's declared masters (D-master). The fix is the raw-pcurve override of §4.

## 9. Operator questions

1. G16: refuse faces on a torus with negative major radius (schema-invalid), or
   accept OCCT's convention (reverse the face) with a record?
2. §5: may NVIDIA's arm include our own projected pcurves as a labelled third
   arm, or only OCCT's two reads?
3. U3: for AP242 PMI and supplemental geometry without a reachable unit context,
   refuse, or take the units of the main shape representation's context?

## 10. For SHOPPING_LIST.md

- ISO 10303-42 current edition, normative text for uniform and quasi-uniform knot
  values, `degenerate_toroidal_surface` evaluation and the CTO definition (the
  map already lists Part 42; this adds the specific clauses needed).
- The usd-brep Git LFS test fixtures (STEP, IGES, JT): our snapshot holds only
  pointers. Fetchable with Git LFS from the public repository at `c9c5979`; not
  fetched here.
- OCCT Mantis #25415 (FPX line shift) and #26376 (negative torus radius) records,
  cited only through code comments.

## Appendix A. The probe

Source `lanes/M4-G1/src/probe.cpp` (fleet), about 720 lines of C++17, linked
against the pinned OCCT 8.0.1 install with the system g++ 13.3 and the same
toolkit list as `native/step_source/CMakeLists.txt`. Runners
`run_audit.sh` (parallel, one process per file, `taskset`, 2 h timeout, 160 GB
address-space cap) and `run_timing.sh` (serial, `numactl --physcpubind=47
--membind=1`, three repetitions of each timing mode, load and clock recorded
before and after every process). Summaries: `aggregate.py`, `table.py`,
`timing_summary.py`. Output: `out/audit/`, `out/audit-default/`,
`out/timing.tsv`, `out/*-summary.tsv`, `out/table-*.tsv`, `out/cto-check.tsv`.

Commands, from the lane directory:

```sh
cmake -S src -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/bin/c++ \
  -DOpenCASCADE_DIR=<BRepGPU root>/worktree/quarantine/om10/occt-install/lib/cmake/opencascade
cmake --build build -j8
./build/g1-probe cto-check
src/run_audit.sh audit out/audit 8 0-46:2
src/run_audit.sh audit-default out/audit-default 8 0-46:2
src/run_timing.sh out/timing.tsv 47 1 3
python3 -I src/aggregate.py out/audit out/audit-summary.tsv
python3 -I src/timing_summary.py out/timing.tsv out/timing-perfile.tsv
```

What the audit mode does for one coedge, in order: decode the file pcurve with
OCCT's own pre-edit decoder; check whether the translated edge stores a pcurve
for the face's surface and location (`BRep_TEdge` curve list, so planar
on-demand projections do not count); if both exist, sample the stored pcurve,
project each point onto the file pcurve and its period images, and compare the
surface points (geometry), then evaluate the file pcurve at the stored
parameters (parameterisation); compute the file residual and OCCT's
`CheckSameParameter` deviation. Thresholds are `1e-9·(1 + 1e4·precision)` mm for
"same".

## Appendix B. Privacy

No hostnames, user names or absolute workstation paths appear here or in the
sources TSV; fleet paths are relative to the BRepGPU root. Corpus files are
cited by their public names. OM10 bytes stay private (copied only into the fleet
lane directory).
