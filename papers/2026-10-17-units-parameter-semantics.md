# Units and parameter-space semantics

Research lane M4-G6 of the fast-mesher campaign, 2026-10-17. Brief:
[docs/fast-mesher-campaign/briefs/M4-G6.md](../docs/fast-mesher-campaign/briefs/M4-G6.md)
with [M4-common.md](../docs/fast-mesher-campaign/briefs/M4-common.md). Stub:
lifecycle map §4.1 G6 (stages S3, S2; contradiction X13). Bead `brepgpu-12s.7`.
Sources and read depth:
[2026-10-17-units-parameter-semantics-sources.tsv](2026-10-17-units-parameter-semantics-sources.tsv).

What ran: one text-level census of unit and uncertainty contexts over every
STEP, IGES, SAT and XT file in the fleet corpus, plus one text-level extent
comparison of paired inch and millimetre exports. Both are owned Python probes
in the fleet lane directory `lanes/M4-G6/` (summarised in §6.1). No OCCT, mesher,
Boolean or NVIDIA code ran. Code citations are to commit `627163b`; B1 is editing
`native/step_source/main.cpp` concurrently, so its line numbers will move.
OCCT citations are to 8.0.1 (`b8f597c`) in `quarantine/new-iteration/occt/src/`.

Tags: **[src]** a source states it; **[code]** read in our code; **[occt]** read
in OCCT source (reference code, not a standard); **[meas]** this lane's census;
**[own]** this lane's derivation.

## 0. Findings that decide things

1. **The mm-only refusal guards the wrong thing.** OCCT already converts every
   length, angle and admitted pcurve to mm and radians per representation
   context before our code sees it [occt: `StepToGeom.cxx:1179-1183, 1316, 2380,
   2386, 2539`; `GeomConvert_Units.cxx:173-303`]. Our reader reads only three
   things raw: unit **names** for the gate, vertex `CARTESIAN_POINT`
   coordinates, and uncertainty values [code `native/step_source/main.cpp:235-249,
   277, 380, 427`]. All three are wrong for non-trivial files. Lifting the
   refusal means fixing those three reads, not writing a source-UV conversion
   map. The comment at `main.cpp:246` assumes such a map is needed; the OCCT
   source does not support that assumption.
2. **No raw plane-angle value is read anywhere in our STEP path.** Cone
   semi-angles, conic trims and pcurve parameters arrive through OCCT already in
   radians [occt as above]. B1's fix for the "unchecked plane-angle unit" must
   therefore refuse units it cannot interpret. It must not multiply OCCT-derived
   values by the declared factor: that is the PrePoMax double conversion
   (§4.1).
3. **Units and uncertainty belong to the representation context, not the file**
   [src: PDES AP203 RP §2.7.1, H.1.9; CAx-IF PMI RP §4.2]. The corpus has files
   with an unused millimetre context beside an inch B-rep context (FTC-07),
   a presentation context four orders of magnitude tighter than the B-rep
   (FTC-10), and assemblies with up to 1,314 B-rep contexts in one file [meas]. Today's
   file-wide maximum uncertainty overstates the B-rep's own value by 2× to
   1,000× in 7 of 488 B-rep files [meas].
4. **Degree files are common, and most spell the degree badly.** 70 of 488
   B-rep files (14%) declare degrees. 41 of them use the factor 0.0174532925,
   which makes 360° equal to 2π − 7.18e-9 rad. That gap is larger than OCCT's
   parametric confusion of 1e-9, so a full-circle trim stops being closed
   [meas; own arithmetic §4.4].
5. **Pcurves are nearly absent from real exports.** 396 pcurves in 2 of 488 B-rep
   files, both written by OCCT itself [meas]. The pcurve parameter map matters
   for the rare file that carries pcurves and for D-master; the angle unit
   matters chiefly for cone semi-angles (1,276 in degree contexts) and conic
   trims (672 in degree contexts) [meas].
6. **Inch input is a third of the corpus and its declared units are right.**
   136 of 488 B-rep files are in inches, 9 in centimetres and 6 in a
   conversion-based unit named `'MILLIMETRE'`. Today's gate refuses all 151,
   including the 6 that are millimetres [meas]. In 86 Rat Rig panels exported
   in both inches and millimetres, every inch file's coordinate extents ×25.4
   equal the millimetre file's to nine digits [meas]. That pairing is a ready
   metamorphic oracle for the lifted reader.

## 1. The return: unit and parameter map per kind

Symbols: **L** is mm per length unit of the item's context; **A** is radians
per plane-angle unit of the same context; **|V|** is a `vector`'s magnitude in
file units. "OCCT applies" says where OCCT 8.0.1 applies the map: the 3D carrier,
pcurves via `GeomConvert_Units::DegreeToRadian` (GCU), `rectangular_trimmed_surface`
bounds via `StepToGeom::MakeRectangularTrimmedSurface` (RTS), and trimmed-curve
parameters via `StepToGeom::MakeTrimmedCurve` (trim). "Corpus" counts entities
in B-rep contexts across 488 files [meas].

### 1.1 Surfaces

| STEP kind | Part 42 parameterisation | u | v | OCCT parameterisation | Map STEP → OCCT | OCCT applies | Corpus | Our reader |
|---|---|---|---|---|---|---|---|---|
| `plane` | C + u·x + v·y | length | length | same | u·L, v·L | carrier, GCU (lines, B-splines; circles and ellipses scaled isotropically; **parabola and hyperbola pcurves not scaled**, `GeomConvert_Units.cxx:237-252`), RTS. Pcurves from `surface_curve` on planes are dropped and recomputed (`StepToTopoDS_TranslateEdgeLoop.cxx:605`) | 228 pcurves (OCCT-written) | admitted |
| `cylindrical_surface` | C + R(cos u·x + sin u·y) + v·z | angle | length | same | u·A, v·L | carrier, GCU (circle and ellipse pcurves become B-splines, then an affine pole map), RTS | — | admitted |
| `conical_surface` | C + (R + v·tan α)(cos u·x + sin u·y) + v·z; α in angle units; v is axial height [src PDES RP H.1.5] | angle | length (axial) | O + (R + v·sin α)(cos u·X + sin u·Y) + v·cos α·Z; v along the generatrix | u·A, **v·L / cos α**, α·A | carrier (α·A, then clamped to ≥ `Precision::Angular`, `StepToGeom.cxx:1316-1319`), GCU, RTS | 17,734 cones in radian contexts, 1,276 in degree contexts | admitted; reads OCCT radians |
| `spherical_surface` | C + R cos v(cos u·x + sin u·y) + R sin v·z; 0 ≤ u ≤ 360°, −90° ≤ v ≤ 90° [src IFC4.3 Part 42 text] | angle | angle | same, radians | u·A, v·A | carrier, GCU, RTS | — | admitted |
| `toroidal_surface` | C + (R + r cos v)(cos u·x + sin u·y) + r sin v·z [src IFC4.3] | angle | angle | same | u·A, v·A | carrier (radii taken absolute, `StepToGeom.cxx:2111-2115`), GCU, RTS | — | admitted |
| `degenerate_toroidal_surface` | as torus, plus `select_outer` | angle | angle | torus; `select_outer` dropped [src M1 L2] | as torus | as torus | 0 | refused |
| `b_spline_surface*`, `bezier_surface` | knots and weights dimensionless; poles are lengths | none | none | same | poles·L; u, v unchanged | carrier; GCU returns pcurve unchanged (correct) | 168 pcurves (OCCT-written) | admitted (degree ≤ 3) |
| `surface_of_revolution` | C + (λ(v) − C) cos u + ((λ(v) − C)·V)V(1 − cos u) + V × (λ(v) − C) sin u [src IFC4.3, Part 42 formula]; 0 < u < 360° | angle | parameter of the swept curve (curve-kind dependent, see §1.2) | u angle (rad), v = OCCT parameter of the swept curve | u·A; v by curve kind (line: v·L·\|V\|; circle or ellipse: v·A; B-spline: unchanged) | carrier; GCU and RTS scale **u only** (`GeomConvert_Units.cxx:201-204`). A circle or ellipse centred on the axis is re-placed and trimmed to [0, π] (`StepToGeom.cxx:2052-2068`), shifting v; file pcurves are not shifted to match | 0 | refused |
| `surface_of_linear_extrusion` | λ(u) + v·V, V with magnitude [src IFC4.3] | parameter of the swept curve | multiples of V | C(u) + v·D, D unit (`StepToGeom.cxx:2018-2026`) | u by curve kind; **v·L·\|V\|** | carrier only. GCU and RTS leave pcurves and bounds **unconverted** (`GeomConvert_Units.cxx:224-227`), wrong whenever L·\|V\| ≠ 1 or the swept curve is a conic in a degree file | 9 (1 file), all \|V\| = 1, no pcurves | refused |
| `offset_surface` | basis parameterisation | basis | basis | basis | as basis; distance·L | carrier; GCU leaves pcurves unconverted, wrong whenever the basis needs a map | 0 | refused |
| `rectangular_trimmed_surface` | bounds in basis units | basis | basis | basis | as basis | RTS for plane, cylinder, cone, sphere, torus; revolution u only; nothing for extrusion or offset | 0 | not read |
| `curve_bounded_surface` | boundary pcurves in basis parameter space | basis | basis | basis | as basis | via pcurve path | 0 | not read |

### 1.2 Curves (3D edge geometry and 2D pcurve geometry)

| STEP kind | Parameter | Unit | OCCT parameter | Map | OCCT applies | Notes |
|---|---|---|---|---|---|---|
| `line` (3D) | λ(u) = P + u·V [src IFC4.3: "the magnitude and units of Dir affect the parameterization"] | file length / \|V\| | arc length, mm | u·L·\|V\| | trim (`StepToGeom.cxx:2380`) | carrier direction normalised |
| `line` (2D, pcurve) | P + u·V in parameter space | parameter-space units / \|V\| | arc length in parameter space after GCU scaling | \|V\| then GCU | trim2d uses \|V\| (`:2533`); GCU rescales location and direction and renormalises, so an untrimmed oblique line on an anisotropic map changes its parameter speed | harmless once SameParameter is rebuilt |
| `circle`, `ellipse` | λ(u) = C + R(cos u·x + sin u·y); 0 ≤ u ≤ 360° [src IFC4.3] | angle | radians | u·A; an ellipse with R1 < R2 is re-axed and shifted by π/2 | trim (`:2386`), trim2d (`:2539`) | 672 parameter trims in degree contexts (max 360), 266 in radian contexts (max 4π) [meas] |
| `parabola` | λ(u) = C + a(u²·x + 2u·y) | dimensionless | U = 2a·u, a length | **U = 2·a·L·u** | **not applied**: trim factor 1 (`:2375-2405`), 2D "LATER !!!" (`:2550`), GCU leaves it (`GeomConvert_Units.cxx:237-244`) | 0 in corpus; our reader refuses |
| `hyperbola` | C + a cosh u·x + b sinh u·y | dimensionless | same | unchanged; a, b·L | carrier only; 2D pcurve not length-scaled (`GeomConvert_Units.cxx:245-252`) | 0 in corpus; refused |
| `polyline` | segment i on [i−1, i] [src IFC4.3] | dimensionless | degree-1 B-spline | unchanged | — | refused as edge geometry today |
| `b_spline_curve*` | knots | dimensionless | same | unchanged | — | admitted |
| `trimmed_curve` | inherits basis; trims in basis units or as Cartesian points; cyclic for closed bases ("370 degrees is equivalent to 10 degrees") [src IFC4.3 `IfcTrimmedCurve`] | basis | basis | as basis | trim | OCCT picks parameter over point unless master is Cartesian (`ExtractParameter`, `StepToGeom.cxx:2221-2317`) |
| `pcurve` | curve in the 2D parameter space of its basis surface [src IFC4.3 `IfcPcurve`] | the surface's u, v units | OCCT surface u, v | the surface row above | GCU, against the **face** surface, not `pcurve.basis_surface` (`StepToTopoDS_TranslateEdgeLoop.cxx:262-267, 576`) | its `definitional_representation` context is a 2D parametric context **with no units** (396 of 396) [meas] |
| `surface_curve`, `seam_curve` | 3D curve plus pcurves, independent parameters | — | — | per component | — | "Receiving system chooses whether to use the 3D curves or recalculate them" [src PDES RP H.1.1] |

**Where the angle unit of a pcurve comes from.** The 2D parametric context
carries no unit assignment [meas]. AP203 RP §2.7.1: "units on parametric
representations are taken from the global_unit_assigned_context entity. They
are not always degrees as might be extrapolated from reading the text of
10303-42." H.1.9: "Consistently use the plane_angle_unit (degree or radian)
referenced by global_unit_assigned_context for all entity model angles and
parameterisations" [src]. The governing context is the one of the
representation that holds the face (its surface), which is what OCCT uses
(the current `StepData_Factors` at `MakePCurve`, `StepToTopoDS_TranslateEdge.cxx:571-579`)
[occt].

**Pcurves on periodic surfaces.** u on cylinders, cones, spheres and tori, and
v on spheres and tori, is cyclic with period 360 angle units (A·360 rad in
OCCT). Parameter values outside one period are legal [src IFC4.3
`IfcTrimmedCurve`]; the corpus has radian conic trims beyond 2π (up to 4π) in 6 NIST AP242 files
[meas]. A reader must reduce cyclic parameters with the declared period after
applying A, never assume [0, 2π). Our reader already relies on OCCT's
`ShapeFix_Wire::FixShifted` for this [code `native/source_uv/source_uv.cpp:240`].

### 1.3 Later readers (IGES, Parasolid XT, ACIS SAT)

| Format | Length unit | Angle unit | Uncertainty analogue | Parameter-space quirks | Source |
|---|---|---|---|---|---|
| IGES 5.3 | Global field 14 unit flag: 1 inch, 2 mm, 3 named in field 15, 4 ft, 5 mile, 6 m, 7 km, 8 mil, 9 micron, 10 cm, 11 microinch. Flag 0 with no name becomes inch; flag 0 or 3 is restored from the name; flag 3 with an unknown name keeps factor 1 with a warning. Field 13 (model-space scale) is read and checked > 0 but not applied | radians in entity 120 start and end angles; degrees in the MSBO analytic surfaces 192 and 194 (u), 196 and 198 | field 19, minimum resolution, in model units | 120: (u, v) swapped and mirrored against OCCT, u shifted by the start angle; 118 and 122 normalised to [0, 1]; 192/194 u in degrees; 196 and 198 in degrees with a translation and mirror; pcurves (142) need this transform | OCCT 8.0.1 only (`IGESData_GlobalSection.cxx:297-322, 385-410`; `IGESData_BasicEditor.cxx:438-520`; `IGESData_IGESModel.cxx:446`; `IGESToBRep_TopoSurface.cxx:1790-2075`). **Secondary**: the IGES 5.3 text (§2.2.4.3.14) was removed for its notice and is on SHOPPING_LIST.md |
| Parasolid XT | No unit field. Size box 1000 units, resolution 1e-8 units; "highly recommended to set the default unit to one meter" | radians ("angular resolution … in radians") | fixed 1e-8 units; tolerant edges carry their own tolerance | cone R(u, v) = P − v·A + (X cos u + Y sin u)(r + v tan a): v axial, axis reversed against the PK interface; swept surface v along a unit D; spun surface has (u, v) = (curve, angle), swapped against STEP revolution | XT Format Reference V35 §4.2.1, §5.2.2.3, §5.2.2.10-11; Functional Description V35 §16.2, A.3.13 ("we recommend applications interpret 0 as indicating a mapping 1 Parasolid unit = 1 metre", scale attribute integer in [−6, 3]) |
| ACIS SAT | Header line 3, first field: millimetres per model unit | radians | resabs, resnor in the header | — | SAT Save File Format 7.0, header; required since ACIS 6.3 |

Corpus check of these formats [meas]: the three SolidWorks IGES files declare
flag 2 `MM`, scale 1.0, resolution 1e-8, maximum coordinate 499,990. The two
SolidWorks XT files carry no unit, size box `1e3` and resolution `1e-8` in the
transmit header. One SAT (NIST D2MI, NX) declares 25.4 mm per unit with resabs
3.937e-8 (that is 1e-6 mm in inches). The SolidWorks SAT declares 1 mm per unit.

## 2. What OCCT converts and what reaches our code raw (question 2)

### 2.1 OCCT 8.0.1 conversion pipeline

- **Factors.** `STEPConstruct_UnitContext::ComputeFactors` resolves each unit to
  a factor: an SI unit gives its prefix; a conversion-based unit gives the
  measure value times the prefix of an **SI** target. A conversion-based target
  (FOOT defined in INCH) returns status 3, "default taken"
  (`STEPConstruct_UnitContext.cxx:322-384, 370`). Length becomes
  metres×1000/cascade unit (`:428`). Defaults before reading the context:
  length 1, **plane angle π/180** (`:280-281`). A GUAC with no plane-angle unit
  therefore yields degrees without any warning. The loop over a context's units
  returns only the last unit's status (`:296-300`), so an earlier failure is
  overwritten.
- **Per representation.** `STEPControl_ActorRead::PrepareUnits` sets the
  factors for each representation's context. It applies the angle mode
  (`read.step.angleunit`: file, rad or deg; default file, `DESTEP_Parameters.hxx:162`).
  With no GUAC the incoming factors are kept, so the context inherits from its
  caller; this is the 0029803 fix (§4.2). With no context at all, factors reset
  to 1, 1, 1, which is radians (`ResetUnits`) (`STEPControl_ActorRead.cxx:2285-2393`).
- **Transforms.** Each `axis2_placement_3d` of an item-defined transformation is
  read in its own representation's context. A
  `cartesian_transformation_operator` is read in Rep2's context, which OCCT itself
  marks "by Rep2 - ?" (`STEPControl_ActorRead.cxx:2408-2545`).
- **Values converted.** Points, radii, offsets, vector magnitudes (×L); cone
  semi-angle (×A); trims for lines (×L·|V|) and conics (×A); pcurves through GCU;
  RTS bounds; validation properties (`STEPConstruct_ValidationProps.cxx:716-730`);
  tessellated nodes (`StepToTopoDS_TranslateFace.cxx:412`); PMI values in XCAF.
- **Not converted** (OCCT gaps, §1): pcurves and RTS bounds on extrusion,
  offset and revolution-v; parabola trims and parabola or hyperbola pcurves;
  2D-line parameter speed after GCU.
- **Precision.** The context's uncertainty (minimum over its length measures,
  `STEPConstruct_UnitContext.cxx:498-538`) times L becomes the
  translation precision; without one, `read.precision.val` (1e-4 in cascade
  units) is used with a warning (`STEPControl_ActorRead.cxx:2370-2385`).
- **The XDE path overwrites the target unit.** `STEPCAFControl_Reader::Transfer`
  calls `prepareUnits`, which sets the model's length unit from the XCAF
  document or, for a new document, from the global `xstep.cascade.unit`
  (`STEPCAFControl_Reader.cxx:506-522, 536`; `XSAlgo_ShapeProcessor.cxx:777`;
  `UnitsMethods.cxx:20`, default 1 mm).
- **`FileUnits`** reports unit **names** from the first shape representation of
  each **root** product only. It falls back to all contexts only when it finds
  none (`STEPControl_Reader.cxx:612-749`). SI names are synthesised
  (`getSiName`); conversion-based names are the file's own strings.

### 2.2 Our reads, site by site [code at `627163b`]

| Site | What it reads | Raw or via OCCT | Correct today? |
|---|---|---|---|
| `native/step_source/main.cpp:235-249` | unit names from `FileUnits`; refuses anything but `millimetre`/`millimeter`/`mm`; detects cm and m and then refuses them; refuses differing root units | names, roots only | No. Root-only, so a child context in another unit passes. Case-sensitive names refuse the six NIST files whose unit is `CONVERSION_BASED_UNIT('MILLIMETRE', 1 mm)` [meas]. A file could name a 25.4 mm unit `'millimetre'` and pass |
| `main.cpp:250` | `SetSystemLengthUnit(1.0)` | — | Ineffective: overwritten by `prepareUnits` in the XDE `Transfer` (§2.1). Today's mm output rests on the global default |
| `main.cpp:277`, `:427` | uncertainty = max `ValueComponent` over every `uncertainty_measure_with_unit` in the file, × the root factor | raw value; unit and context ignored | No. Overstates the B-rep's own uncertainty in 7 files, by 2× to 1,000× [meas, §3] |
| `main.cpp:380` | vertex coordinates from `CARTESIAN_POINT` × the root factor, compared with the OCCT vertex | raw | Correct only when the vertex's context has the root's unit |
| `main.cpp` carriers, curves, pcurves, ranges, transforms | from OCCT `Geom` objects and `TopoDS` | via OCCT (mm, radians, OCCT parameterisations) | Yes, for the admitted kinds |
| `main.cpp:235-236` angle names | read, never checked (B1 defect 3) | — | No angle is read raw, so nothing needs the factor applied; unknown units must be refused |
| `native/reference_source/main.cpp:113-117, 144, 207` | same name gate; raw max uncertainty written with units 1 | raw | Same defects; the uncertainty is written unconverted |
| `native/source_uv/source_uv.hpp:25` | fixed grades: projection 1e-7 mm, C/S residual 1e-3 mm, query 5e-3 mm | constants | Unit-safe after conversion. They do not scale with the source's own uncertainty (see H4) |
| `native/mesher/source.cpp:724-727`; `python/brepgpu/imported_source.py:171-172, 273-275` | decode `units_mm` and the uncertainty, check them, use neither | — | — |

So in our path the parameter-space map is OCCT's, and it is correct for every
kind the reader admits (plane, cylinder, cone, sphere, torus, B-spline;
line, circle, ellipse, B-spline). Lifting mm-only needs three things:

1. an entity-graph unit resolver per context;
2. per-context factors at the raw vertex read;
3. per-context uncertainty.

None of the parameter maps changes. The owned reader that later replaces
OCCT needs the whole table in §1, including the four places OCCT gets wrong.

## 3. Uncertainty per context (question 3)

**What the sources say.**

- The global uncertainty belongs to a `representation_context`. It is "the
  largest gap anticipated between elements that should be deemed coincident".
  Pre-processors "should point to one uncertainty_measure_with_unit which should
  identify a length_measure", named "closure" [src PDES RP §2.7.1].
- The CAx-IF calls it "the maximum value for when 2 points are considered
  equal", with the name `'distance accuracy value'` [src PMI RP §4.2].
- Post-processors "shall use the uncertainty_measure_with_unit value for error
  checking of the file where an error is a gap in the shape which is larger than
  the length_measure value" [src PDES RP §2.7.1].

**What the corpus shows** [meas, 488 B-rep files, 2,620 B-rep contexts]:

- Every B-rep context carries exactly one `LENGTH_MEASURE` uncertainty. In every
  context it is in the context's own length unit; no non-length uncertainty
  occurs.
- Names: `DISTANCE_ACCURACY_VALUE` (2,102), `distance_accuracy_value` (492),
  `closure` (18), `point coincidence tolerance` (5), empty (3).
- Values, as mm per file: 0.01 (294 files), 1e-5 (105), 0.005 (24), 2e-5 (16),
  0.0508 (7, which is 0.002 in), and Pro/E relative values from 0.064 to
  0.597 mm. CAx-IF exports also declare 1e-13 to 5e-12 mm, below double
  resolution at part scale.
- **Contexts that are not B-rep carry their own values.** In the AP203 FTC-10,
  the B-rep context declares 0.01 mm and the
  `mechanical_design_geometric_presentation_representation` context declares
  1e-6 mm. In FTC-07 and FTC-11 the 1e-6 mm context is unused. In FTC-07 the
  B-rep context is in inches (0.0003937 in = 0.01 mm) and the unused one in mm.
  In the AP242 FTC-10 one 0.005 mm context is shared by the B-rep, 19 tessellated
  draughting models and the constructive geometry.
- **Assemblies vary per part.** `as1_pe_203` has five B-rep contexts from 0.064
  to 0.597 mm, plus non-B-rep contexts up to 0.672 mm. `s1-ug-203` has B-rep
  contexts at 2e-5 and 0.02 mm. In 7 files today's file-wide maximum is 2×,
  2×, 5.4×, 5.6×, 10.5×, 45× and 1,000× the governing value of at least one
  B-rep context.
- The same NIST part declares very different values across exports, for example
  CTC-02 at 0.119, 0.0508 and 0.01 mm [src G15 paper §2.3]. The value describes
  the exporter, not the part.

**Which context governs which entity.**

- A representation item takes the units and uncertainty of the
  `context_of_items` of the representation that contains it [src PDES RP
  §2.7.1; IFC4.3 republication of Part 43 is not read, so this rests on the RP].
  B-rep faces, edges and vertices are governed by the B-rep context.
  Presentation, draughting, tessellated-PMI and constructive items are governed
  by theirs.
- Pcurve parameters are governed by the B-rep context's units (§1.2). They have
  no uncertainty of their own.
- A placement between representations reads each placement in its own context
  [occt `ComputeTransformation`].
- Across parts of an assembly no source states a rule. **[own]** A coincidence
  or contact decision between items of two contexts uses the larger of the two
  governing values, since each sender only claims its own gaps.
- OCCT uses the minimum of the length uncertainties within one context
  (`STEPConstruct_UnitContext.cxx:514, 532`); the sources define the value as the
  largest anticipated gap. **Resolution [own]:** with one value per context (all
  2,620 in the corpus) the two agree. If a context carries several length
  values, prefer the one named `closure`, `distance_accuracy_value` or
  `point coincidence tolerance` (case-insensitive). With several such values,
  record them all and govern by the largest, because that is the only one that
  bounds every gap the sender anticipated.

**The uncertainty record** (owned native record, one per context):

- context entity id, role (B-rep, presentation, tessellated, parametric,
  unused) and the ids of the representations that use it;
- length, angle and solid-angle units, each with:
  - its unit entity id;
  - the name as written;
  - the declared factor;
  - the factor used (§5 UR-3);
  - a snap flag;
- every uncertainty measure, each with:
  - its value as written;
  - its own unit entity, resolved separately;
  - its name and description;
  - the value in mm;
- the governing uncertainty in mm, and the rule that chose it.

Every B-rep record key carries the context id. The source header's single
`source_uncertainty_mm` becomes the governing value of the definition's B-rep
context.

## 4. Known pitfalls (question 4)

### 4.1 Double conversion (PrePoMax)

PrePoMax issue t/183, 6 May 2022: a SolidWorks STEP in inches came in labelled
inches but scaled by 25.4, so 2.0 in showed as 50.8 in. The maintainer:
OpenCascade "converts all units to mm automatically", the unit was then applied
again, and "Instead a scale function was added to PrePoMax to solve this
problem" [src forum thread, read in full]. The failure is two conversion sites
for one value. Our code has the same shape: `main.cpp:380` converts raw values
while everything else arrives from OCCT already converted. **Rule [own]:** each
value records whether it came raw from the file or from OCCT, and gets exactly
one conversion. The Rat Rig pair oracle (H1) catches a second conversion as a
25.4 ratio.

### 4.2 OCCT 0029803

"STEP Import: Units (inch) not applied to entity possessing
GEOMETRIC_REPRESENTATION_CONTEXT" [src: OCCT test `tests/bugs/step/bug29803`;
the Mantis tracker did not resolve from the laptop]. Constructive geometry
(`constructive_geometry_representation`) in an inch file had a plain
`geometric_representation_context` with no unit assignment and was read with
default units. OCCT's fix keeps the caller's factors when a context has no GUAC
(`STEPControl_ActorRead.cxx:1437-1505, 2344-2359`). A context without units
therefore inherits from whichever representation the reader was in. For us
**[own]**: a unit-less context that holds geometry we mesh is unit-less input
(D-units, refuse unless the caller states units). A unit-less context holding
only constructive or presentation items is recorded and not meshed. The corpus
has 34 contexts with neither dimension nor units and 396 unit-less parametric
contexts. No B-rep sits in a unit-less context [meas].

### 4.3 IGES and XT defaults

- IGES: an empty unit flag with no name is read as inch by OCCT; this matches
  the lifecycle map's statement that it is legally inches [src M1 L2;
  occt `IGESData_GlobalSection.cxx:387-393`; the normative text is not held].
- XT has no unit field. Parasolid "highly recommend[s]" one metre per unit and
  recommends applications interpret a missing scale as metres [src XT ref
  §4.2.1; FD A.3.13].
- Whether these count as "declared" under D-units is an operator question (§8).

### 4.4 Further pitfalls found by this lane

1. **Truncated degree.** `PLANE_ANGLE_MEASURE(0.0174532925)` (41 files) is
   1.14e-9 relative short of π/180. 360° then becomes 2π − 7.18e-9 rad, larger
   than `Precision::PConfusion()` (1e-9). The other spelling, 0.01745329252
   (29 files), is off by 3.2e-12 relative. On a 1 m radius the first gap is
   7.2e-6 mm [own arithmetic, fleet Python].
2. **Factor rounding.** The AP203 RP Appendix E inch is 2.54 × CENTI metre.
   Computed in OCCT's order it gives 25.400000000000002 mm, not 25.4 [own
   arithmetic]. Geometrically negligible, but two readers that order the
   product differently disagree at the last bit (G7).
3. **Name-based unit detection.**
   - Unit names are free text: `'INCH'` 336×, `'inch'` 92×, `'DEGREE'` 148×,
     `'degree'` 1×, `'MILLIMETRE'` 6× as written in the corpus [meas].
   - The OM10 watch defines an unused centimetre unit next to the millimetre one
     its contexts use [src `2026-10-15-om10-step-source-study.md`].
   - Only the entity graph is authoritative.
4. **Root-only `FileUnits`** (§2.1). The corpus has no file whose B-rep units
   differ from its root's [meas], so this is latent here. An authored mixed-unit
   assembly is needed as a fixture.
5. **Silent defaults.** A GUAC without a plane-angle unit is read as degrees
   with no warning. A nested conversion-based unit (FOOT via INCH) falls back
   to factor 1 with a warning (§2.1). The corpus has neither [meas]; the 4 NIST
   AP242 files without a solid-angle unit are harmless.
6. **The cone semi-angle clamp** to `Precision::Angular` changes a zero or
   negative angle silently (`StepToGeom.cxx:1319`).
7. **Assembly-level uncertainty** taken as a file maximum (§3).

## 5. Refusal rules and unit rules (the return)

These are proposals for M5. Each states its hypothesis and falsifier in §6.2.

- **UR-1 Scope.** Units and uncertainty are resolved per representation context
  and carried on every record that context governs. No file-level unit exists.
  Items take the context of their representation. Pcurve parameters take the
  angle and length units of the context of the face's representation. Each
  placement is read in its own context.
- **UR-2 Resolution from the entity graph.**
  - An `si_unit` gives its prefix power of ten.
  - A `conversion_based_unit` gives its measure value times its target's factor,
    resolved recursively with a depth bound of 8. A cycle, a missing target, a
    target of another kind, or a non-finite or non-positive value is refused.
  - Names never set a factor. A name that disagrees with its factor, such as
    `'INCH'` that is not 25.4 mm within 1e-9 relative, is recorded as a
    finding.
- **UR-3 Exact constants (proposal; operator confirmation asked, §8).**
  - A resolved length factor within 1e-9 relative of 25.4, 304.8, 0.0254,
    2.54e-5, 1, 10, 1,000 or 1e6 mm uses that constant.
  - A resolved angle factor within 2e-9 relative of π/180 uses the double
    nearest π/180.
  - Both factors as declared are recorded, with the snap flag.
- **UR-4 Refuse** (per context, so per entity under D-partial-output, and never
  per file):
  1. a context that holds meshed geometry and has no length unit, unless the
     caller states one (D-units);
  2. any length or angle unit UR-2 cannot resolve;
  3. two units of the same kind in one GUAC (OCCT status 14);
  4. a context that holds an angle-bearing value we read (cone semi-angle, conic
     parameter trim, pcurve or RTS bound on an angular parameter, revolution) but
     has no plane-angle unit. The caller may state the unit. A context with no
     angle-bearing value is accepted with a record.
- **UR-5 Accept** inch, foot, mil, micro-inch and every SI prefix, and mixed units
  across contexts in one file. Each context is converted to mm once. A missing
  solid-angle unit is recorded, never refused (no geometry uses it).
- **UR-6 One conversion per value.** Raw reads (vertices, uncertainty, and
  every raw read the owned reader later adds) use their own context's factor.
  OCCT-derived values are never multiplied again. Each read site states its
  provenance.
- **UR-7 Uncertainty.**
  - Read per context, each measure in its own unit.
  - Govern the context by the rule in §3.
  - Record absence; never default to a value for coincidence decisions.
  - The data-quality profile (D-profile) supplies thresholds, and D-PMI's
    "ε tighter than the source's own uncertainty" warning uses the governing
    B-rep value.
  - A declared value below 1e-15 of the definition's bounding-box diagonal is
    recorded as making no usable claim [own].
- **UR-8 Cyclic parameters.** Angular parameters on periodic surfaces and closed
  conics are reduced with the period 360·A after conversion. Values outside one
  period are legal, never refused.
- **UR-9 Kinds whose maps OCCT gets wrong** (extrusion, offset and revolution
  pcurves and RTS; parabola trims; parabola and hyperbola pcurves).
  - While OCCT is the bridge: either admit these kinds only with recomputed
    pcurves and a recorded residual (D-master), or refuse them.
  - The owned reader applies the §1 maps itself.
- **UR-10 Other formats** (for the later readers).
  - IGES: resolve fields 14 and 15 with the table in §1.3. Treat field 19 as the
    uncertainty analogue. Refuse flag 3 with an unknown name and a non-unit
    field 13 until IGES 5.3 is read.
  - XT and SAT: radians. SAT length from its header. XT length per the operator
    answer (§8).

## 6. Corpus census (question 5) and measurements

### 6.1 Probes

- **`unit_census.py`** (owned, standard-library Python, fleet lane directory).
  - Strips Part 21 comments and splits instances with a string-aware regex.
  - Parses only the instances it needs into a small value tree.
  - Resolves units per context from the entity graph as in UR-2.
  - Records every uncertainty with its own unit.
  - Maps representations to contexts. Attributes geometry to contexts by
    breadth-first search from each representation's items, stopping at other
    representations and at `mapped_item`. Shape representations go first, so
    presentation items do not claim B-rep geometry.
  - Determines roots as product definitions never related by a NAUO.
  - Counts per context: cones and semi-angles, conic parameter trims, pcurves
    by basis-surface kind, revolution, extrusion and its vector magnitude,
    offset, degenerate torus, RTS, parabola and hyperbola, and Cartesian
    operators.
  - Reads the IGES global section, the SAT header and the XT header text. Its
    IGES parse failed on CRLF files, so those three headers were read directly.
- **`pair_extent.py`** compares per-axis extents of all 3D `CARTESIAN_POINT`s
  between same-named Rat Rig files under `Imperial Units (Inches)` and
  `Metric Units (mm)`.

Outputs are in the fleet lane's `out/` (`census-files.tsv`, `census-contexts.tsv`,
`census-other.tsv`, `ratrig-pairs.tsv`).

Run context:

- lane: CPU, unpinned, `nice 10`;
- one text pass of 2 minutes over 500 STEP files;
- load average 7.7 at start and 29.8 after the run, with other M4 lanes active;
- governor schedutil, about 3.0 GHz mean;
- SMT on, AutoNUMA on;
- cold file cache.

No timing claim rests on these runs.

### 6.2 Results [meas]

| Quantity | Value |
|---|---|
| STEP files / with B-rep | 500 / 488 (12 are external-reference shells or the tessellated FTC-08) |
| B-rep contexts | 2,620 (1 to 1,314 per file); none unit-less; none without a plane-angle unit |
| Length unit per B-rep file | mm 343 (6 via `'MILLIMETRE'`), inch 136, cm 9 |
| Angle unit per B-rep file | radian 418; degree 70 (41 at 0.0174532925, 29 at 0.01745329252) |
| Unit pairs per file | (mm, rad) 313; (in, rad) 105; (in, deg) 31; (mm, deg) 30; (cm, deg) 9 |
| Inch by exporter | Autodesk Translation Framework 88, CATIA V5 8, NX 8.5 4, Pro/E 2, others |
| Mixed B-rep units in one file; B-rep unit differing from root's; transforms across a unit change | 0; 0; 0 |
| Files with contexts of different units | 4 (FTC-07 copies: inch B-rep, unused mm context) |
| Today's gate | accepts 337 files (220,963 faces), refuses 151 (13,051 faces): inch 136, cm 9, `'MILLIMETRE'` 6 |
| Cones | 17,734 in radian contexts (α 1.4e-4 to 1.309 rad); 1,276 in degree contexts (α 1 to 59) |
| Conic parameter trims | radian 266 (max 4π; 23 beyond 2π, in 6 files); degree 672 (max 360) |
| Pcurves | 396 in 2 files, both OCCT 6.1 exports: 228 on planes, 168 on B-splines; parametric contexts unit-less |
| Revolution / extrusion / offset / RTS / parabola-hyperbola / degenerate torus / Cartesian operator in B-rep | 0 / 9 (1 file, \|V\| = 1) / 0 / 0 / 0 / 0 / 0 |
| `brep_with_voids` | 5 files: CR-10, two Ender-3 files, both Voron full assemblies |
| Rat Rig inch/mm pairs | 86 pairs, every extent ratio (inch × 25.4 / mm) = 1.000000000 |
| Validation-property records | `property_definition` mentioning validation in 66 files, 21 of them inch: a GVP unit oracle |
| IGES / XT / SAT | flag 2 MM ×3 / no unit, size box 1e3 ×2 / 25.4 and 1 mm per unit |

### 6.3 Hypotheses and falsifiers for M5 and the arena

- **H1 (lift).** With UR-1 to UR-6, every refused corpus file reads with
  `translated_vertex_displacement` ≤ its governing uncertainty, and each Rat Rig
  inch file's mesh bounding box ×25.4 equals its mm twin's within 1e-9 relative.
  **Falsifier:** any displacement above uncertainty, or any pair ratio ≠ 1. The
  GVP volume and centroid in the CAx-IF and NIST inch files are a second, independent oracle
  (falsifier: volume ratio outside 1 ± 1%, the CAx-IF green band).
- **H2 (degree snap).** In the 41 files declaring 0.0174532925, OCCT-translated
  edges trimmed 0 to 360° have range 2π − 7.18e-9 without snapping and exactly
  2π with snapping. **Falsifier:** no such edge, or no change in OCCT
  closure classification. Measure by running `step_source` on those files with
  and without UR-3.
- **H3 (per-context uncertainty).** Governing by context changes the D-PMI
  warning outcome (ε below uncertainty) for at least one context in the 7 files
  of §3, and never for files with one context. **Falsifier:** no outcome changes.
- **H4 (fixed grades).** `source_uv`'s 1e-3 mm C/S residual grade refuses
  correctly converted inch files whose declared uncertainty is coarser
  (0.0508 mm). **Falsifier:** zero `C/S residual` refusals over the 136 inch files
  once admitted. If it holds, the grade must derive from the governing
  uncertainty.
- **Not answerable from this corpus.** How often pcurves on periodic or swept
  surfaces, degree-context pcurves, nested conversion units or mixed-unit
  assemblies occur in the wild. The same census over the ABC sample (once the
  operator settles the download term), and over Fusion 360 Gallery STEP (its
  segmentation STEP is ×10 against its native file, per the corpus manifest),
  settles it. Authored fixtures cover them for tests meanwhile.

## 7. Where the literature does not say

- The normative Part 41/42/43 texts are not held. Parameterisations here come
  from the IFC4.3 republication of Part 42 definitions, the AP203 RP and OCCT.
  The cone's tan-based axial v rests on PDES RP H.1.5 plus OCCT's v/cos α map,
  not on Part 42 itself.
- No source gives a rule for coincidence across parts with different
  uncertainties; §3's larger-of-two is our derivation.
- IGES 5.3 unit defaults, field 13 semantics and MSBO parameter conventions are
  from OCCT only (secondary).

## 8. Operator questions

1. **IGES empty unit flag.** Is the standard's inch default "declared" under
   D-units, so we accept it, or unit-less input refused unless the caller states
   units? The corpus IGES all declare `MM` explicitly.
2. **Parasolid XT.** XT has no unit field and only a "highly recommended" metre
   convention. Accept it as declared, or refuse unless the caller states units?
3. **Caller unit against a declared unit.** When the caller states a unit for a
   file that declares a different one (a known-bad exporter), should we refuse,
   keep the declared unit with a warning, or honour the caller with a record?
   D-units covers only unit-less input.
4. **UR-3 snapping.** Snapping changes the declared degree factor by 1.1e-9
   relative and the inch factor in its last bit. Does that count as honouring
   declared units, or should the declared factor be used as written and
   closure handled downstream?

## 9. Defects (for beads; the lane fixed nothing)

1. `native/step_source/main.cpp:235-249` and `native/reference_source/main.cpp:113-116`.
   The unit gate uses `FileUnits` names: roots only, first shape representation
   only, case-sensitive. It refuses 6 NIST AP242 files whose unit is
   `CONVERSION_BASED_UNIT('MILLIMETRE', 1 mm)` [meas]. A child context in
   another unit passes. A unit named `'millimetre'` with another factor would
   pass.
2. `native/step_source/main.cpp:380`. Vertex coordinates are read raw and
   multiplied by the single root factor, which is wrong for a vertex in a
   context with another unit. Latent: the gate admits only mm roots, and the
   corpus has no mixed file.
3. `native/step_source/main.cpp:277, 427` and `native/reference_source/main.cpp:144, 207`.
   Uncertainty is the file-wide maximum of raw values, ignoring each measure's
   unit and context. It overstates the B-rep value 2× to 1,000× in 7 corpus
   files. `reference_source` also writes it with no conversion.
4. `native/step_source/main.cpp:250` and `native/reference_source/main.cpp:117`.
   `SetSystemLengthUnit(1.0)` is overwritten in `STEPCAFControl_Reader::Transfer`
   by `prepareUnits` (OCCT `STEPCAFControl_Reader.cxx:506-522, 536`). The
   effective unit is the XCAF document's or the global `xstep.cascade.unit`.
   The output is mm today only by default. Fix direction: set the document
   length unit and assert it after transfer.
5. Caution for B1 defect 3, not a new defect: no plane-angle value is read raw
   in `step_source`. Applying the declared angle factor at any of our sites would
   convert twice. The fix is to refuse unresolvable or missing angle units
   (UR-4.4).
6. Evidence for B1 defect 1: `brep_with_voids` occurs in 5 corpus files,
   including both Voron full assemblies [meas].

OCCT 8.0.1 gaps, recorded for the owned reader (not ours to fix): unconverted
pcurves and RTS on extrusion, offset and revolution-v; parabola trims; parabola
and hyperbola pcurves; silent degree default; nested conversion units fall to
factor 1; the per-context status overwritten; minimum instead of governing
uncertainty; the cone angle clamp.

## For SHOPPING_LIST.md

- ISO 10303-41 (`measure_schema`: `si_unit`, `conversion_based_unit`,
  `uncertainty_measure_with_unit`) and ISO 10303-43 (`representation_context`,
  `global_unit_assigned_context`, `global_uncertainty_assigned_context`,
  item-defined transformations): normative texts, paywalled. They would replace
  the RP-based reading in §3 and UR-1.
- IGES 5.3 (already listed): §2.2.4.3 global section fields 13, 14, 15 and 19,
  and the MSBO surface parameterisations (entities 190-198), to replace the
  OCCT-only reading in §1.3.
- ISO 10303-42 normative text (already listed): parameterisations of
  `conical_surface`, `surface_of_linear_extrusion` and `parabola`.
- OCCT Mantis 0029803 page and its attachment `bug29803.stp`: the tracker did
  not resolve from the laptop. The test script is held.
