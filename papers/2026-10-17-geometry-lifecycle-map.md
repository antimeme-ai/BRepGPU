# Geometry lifecycle map: from authoring intent to a mesh that lives on

Research lane M1 of the fast-mesher campaign, 2026-10-17, branch
`research/fast-mesher-study`. Brief:
[docs/fast-mesher-campaign/briefs/M1-lifecycle-map.md](../docs/fast-mesher-campaign/briefs/M1-lifecycle-map.md).
Nothing was built, run, meshed or benchmarked. The lane read the doctrine, the
operator-decision journal, the campaign plan, the working-draft design
([docs/FAST_MESHER_DESIGN.md](../docs/FAST_MESHER_DESIGN.md)), the seven
2026-10-17 fast-mesher papers, the 2026-10-16 whole-mesher studies, the
2026-10-15 STEP shape-processing and meshing studies, and the code. Seven
literature sub-lanes (L1-L7) and one code-mapping sub-lane (C1) read in
parallel; this report is the synthesis. Sources, read depth and hashes are in
[2026-10-17-geometry-lifecycle-map-sources.tsv](2026-10-17-geometry-lifecycle-map-sources.tsv).
PDFs are in ignored `papers/pdfs/`.

How to read it:

- §0 is the one-page summary. §1 is the stage table with coverage marks. §2
  defines every stage and maps it. §3 lists contradictions. §4 is the ranked gap
  list as stub briefs for M4, with operator decisions separated from research.
- Tags: **[src]** a source establishes it (citation in the stage's sub-lane
  record, or inline); **[code]** read in this repository at file:line;
  **[own]** this lane's inference or derivation; **[inf]** inferred from a
  source but not stated by it.
- Coverage marks, per stage, against three things we have (design draft,
  papers, code):
  - **covered (evidence)**: a source-grounded design decision exists and the
    papers carry the evidence;
  - **covered (assumption)**: the design decides it, but by an unsourced call or
    an assumption the literature does not settle;
  - **partial**: some of the stage is decided or built; named parts are not;
  - **absent**: neither the design nor the papers address it.
  Code status is stated separately for each stage, because the old mesher
  leaves the build when the new one lands (operator decision).
- Design section numbers (§5.2 etc.) refer to `docs/FAST_MESHER_DESIGN.md` as of
  this lane. Paper names drop the `papers/2026-10-1x-` prefix where unambiguous.

## 0. Decision-relevant findings

1. **The ingress bridge already rewrites geometry, whatever the ShapeProcess
   flags say.** OCCT's STEP translation layer itself (StepToTopoDS, StepToGeom)
   drops file pcurves on planes, recomputes missing pcurves, projects vertex
   parameters, ignores `surface_curve.master_representation`, merges coincident
   vertices, turns a mirrored `cartesian_transformation_operator_3d` into a
   rotation, accepts scaled locations, sets shell `Closed` without checking and,
   by default, merges alternative-shape and tessellated representations into the
   product shape [src L2: `StepToTopoDS_TranslateEdgeLoop.cxx:110-112, 399,
   603-616, 844-871`; `StepToGeom.cxx:2187-2210`; `StepToTopoDS_Builder.cxx:150`;
   `DESTEP_Parameters.hxx:167-168`]. The 2026-10-15 lifecycle fix only switched
   off ShapeProcess operators. Consequences: the "source authority" our
   readers claim is partly OCCT's, and the steel-manned NVIDIA arm called "our
   unprocessed read" is not unprocessed. Separately, our adapter walks only
   `raw->Outer()`, so a `BREP_WITH_VOIDS` (an OCCT subclass of
   `MANIFOLD_SOLID_BREP`) loses its void shells silently
   [code `native/step_source/main.cpp:313,332`;
   `quarantine/new-iteration/occt/src/DataExchange/TKDESTEP/StepShape/StepShape_BrepWithVoids.hxx:32`].
2. **The design freezes shared edge chains; every mature mesher re-opens them.**
   Gmsh, OCCT BRepMesh and Parasolid all let trouble in a face (unrecoverable
   boundary, self-intersecting UV wire, local refinement near inner loops)
   re-split the shared edge and re-mesh every incident face, in bounded rounds
   [src L4: Gmsh `meshGFace.cpp:195-215, 2158-2171`; OCCT
   `BRepMesh_ModelHealer.cxx:57, 150-205`; Parasolid FD §107.2.2.1, §107.2.16].
   Design §5.2-5.3 settles each chain before any face consumes it and refuses a
   face that cannot be triangulated; it has no face-to-edge path, though the
   healing paper it cites proposes one (§7.4 item 5).
3. **The export boundary undoes FP64 validity and the shading work unless it is
   designed as a stage.** lib3mf and PrusaSlicer store positions as float32;
   glTF positions and normals are float32; USD points are float32
   [src L5]. Rounding exact coordinates to floats created new self-intersections
   in about 12% of self-intersecting Thingi10K models [src Lazard–Valque 2025 §1].
   STL and 3MF carry no vertex normals at all, so the design's exact per-corner
   normals reach a consumer only through glTF, USD, OBJ, PLY or AP242
   tessellated [src L5]. 3MF requires overlapping components to be united, so
   touching gears exported as components print as one solid [src 3MF Core 1.4.0
   §4.2]. Design §5.5 checks "validity on written bytes"; the consumer-side
   reality is a float32 re-import plus exact-position weld.
4. **"Mesh each definition once, place it many times" is only safe for rigid and
   mirrored placements.** Under a scale s the world error is s·ε_def. Parasolid
   reuses facets across rigid and reflection transforms only and forbids scale in
   instance transforms; OCCT computes deflection in the first occurrence's frame
   and silently reuses it for every other occurrence [src L7: Parasolid FD
   §14.2.9, §107.2.16; OCCT `BRepMesh_Deflection.cxx:30-62`,
   `BRepMesh_ModelPreProcessor.cxx:54-70`]. The mesh key must carry ε in
   definition units, and ownership must be per definition, never per occurrence
   (OCCT's 2012 race, Mantis 0023260) [src L7].
5. **Tolerance semantics: there is no published chord-to-GD&T rule, and two of
   the draft defaults sit in the wrong place.** Metrology systems measure against
   exact surfaces and use triangles for display [src L1: PC-DMIS, Calypso].
   Vertices-on-surface chords bias features of size one way (a hole of radius R
   with sag δ has mating diameter 2(R−δ)) [own L1]. The relative default 1e-3 of
   the diagonal gives 0.1 mm on a 100 mm part, larger than whole tolerance bands
   in NIST's fully-toleranced test cases [src L1: NIST FTC-07 band 0.076 mm],
   and 0.005 mm equals some files' own coincidence uncertainty (FTC-08). The
   ingress keeps one uncertainty, the maximum over contexts
   [code `native/step_source/main.cpp:277`]; FTC-10 carries 0.01 mm on the B-rep
   context and 1e-6 mm on its presentation context [src L1].

Two further findings shape the gap list: the design has no state for "the check
could not be evaluated" (every mature checker separates it from "invalid"
[src L3]), and the code refuses per file at every layer while the design
promises per-entity refusal with the rest of the part meshed [code
`native/step_source/main.cpp:431-432`; `native/mesher/source.cpp:642-643`].

## 1. The lifecycle at a glance

Stages S1-S18 are the brief's list (S5 includes the M2 sub-stage S5b). Stages
marked **+** (S19-S25) are added because the literature has them as distinct
stages; mature-system evidence for each is in its §2 entry (L4 lists eighteen
such stages across Gmsh, Netgen, OCCT, SMESH, Cubit and Parasolid; the ones not
given their own row here, such as boundary layers, periodic constraints and
canonical renumbering, are CAE-only or already inside S7-S9). "Code" is the
current repository; the old mesher is retired by operator decision, so code that
lives only in it is counted as evidence of an implemented mechanism, not as
current coverage.

| # | Stage | Mark | Design | Code today | Main gap |
|---|---|---|---|---|---|
| S1 | Authoring intent and its loss in exchange | absent | — | colours, materials, product names read | identity/feature passthrough; nominal-model flag |
| S2 | Tolerance and metrology semantics (the request) | covered (assumption) | §4.1, §9.6 | positional_mm query; one uncertainty | PMI-aware request; per-context uncertainty; one-sided bias |
| S3 | File exchange and reading | partial | §3 ingress bridge | STEP via OCCT XDE, mm only, per-file refusal | OCCT translation-layer edits; entity coverage; units |
| S4 | Validation and data-quality analysis | partial | §5.1 refusals | decoder checks, BRepCheck as warning | pre-heal measurement; PDQ profile; indeterminate status |
| S5 | Healing, repair, tolerant modelling | covered (assumption) | §5.2 R1-R6, V1-V4; §7 | none (old path certified and refused) | thresholds are calls; feedback loop; M2 |
| S5b | Reconciliation from well-conditioned neighbours | in flight (M2) | §7 | — | M2 lane, not researched here |
| S6 | Simplification, small features, virtual topology | partial | §5.2 item 5 | none | ε proof of boundary-only faces; validity checks |
| S7 | Topology construction and shared boundaries | covered (evidence) | §4.3, §5.2 | DefinitionBoundaryForest, node keys | face-to-edge feedback; classification map |
| S8 | Edge discretization | covered (evidence) | §5.2.1, §6.1 | adaptive curves (certified) | proximity/max-length caps |
| S9 | Face meshing | covered (evidence) | §5.3, §6 | adaptive faces, packets, direct CDT | band quality; termination bounds |
| S10 | Assembly and instancing | partial | §5.1, §5.5 | mesh once per definition; scaled/mirrored refused | scale and mirror semantics; per-occurrence attributes |
| S11 | Mesh post-processing and quality improvement | partial | §5.3 band rules, §4.7 cap rule | none in current path | legality rules; CAE profile |
| S12 | Normals, attributes, LOD | partial | §4.7 | analytic normals in display; hard edges everywhere | normals cannot reach STL/3MF; LOD absent |
| S13 | Export formats and validity rules | partial | §5.5 export | no STL/3MF/OBJ/glTF writer | float32 narrowing; weld; units; 3MF union rule |
| S14 | Downstream consumers | partial | §2 goal 3 | — | consumer conformance targets |
| S15 | Error reporting and diagnostics | partial | §4.5-4.6, §5.5 records | typed errors, whole-run abort | record format; indeterminate; roll-up |
| S16 | Incremental and associative remeshing | partial | §7 | generations within one run | identity across runs; equivalence oracle |
| S17 | Caching, persistence and provenance | partial | §5.1 cache, §5.5 source maps | in-memory caches; SHA-256 and META strings | on-disk cache keys; determinism under non-bit-identity |
| S18 | Performance engineering at scale, CPU/GPU | partial | §5.5, §6.2, §9.5 | serial definitions; FIFO pool; no NUMA, no SIMD | heterogeneous placement; measurement protocol |
| S19+ | Sizing field and work estimation | partial | §5.5 work estimate, §6.1 | — | max-length, proximity, grading |
| S20+ | Boundary check, retry and face-to-edge feedback | absent | — | old path: boundary rounds within run | the loop and its termination |
| S21+ | Physical embedding and thin features | covered (assumption) | §5.6 | old path: hot-path contact check | soundness of ε-inflated planning; export rounding |
| S22+ | Imprint, merge and contact interfaces | absent | — | — | scope; matched discretization without merge |
| S23+ | Mesh-to-geometry classification map | partial | §5.5 source maps | FacetKey, ConstructionSite.source | collapse history; per-format face IDs |
| S24+ | Provisional state and progressive refinement | partial | §7 | — | representation of provisional results |
| S25+ | Offline verification oracles | covered (evidence) | §10 | Python original-source readers | reader-equivalent export oracle; indeterminate handling |

## 2. Stages

Each stage gives purpose; inputs and outputs; the decisions made there and who
or what makes them; known failure modes; mature practice; then the mapping to
the design, the papers and the code, the coverage mark, and contradictions.

### S1. Authoring intent and its loss in exchange

- **Purpose.** Carry what the designer meant (features, parameters, constraints,
  analytic choice, symmetry, which faces form one hole, whether the model is
  nominal) far enough that downstream processing does not misread the shape.
- **Inputs → outputs.** Native CAD with history → a neutral "dumb solid" (STEP
  advanced B-rep), optionally with PMI, persistent IDs and validation
  properties.
- **Decisions and deciders.**
  - The designer chooses surface types, features, the dimensioning standard and
    whether the model is nominal. The CAx-IF PMI practice sets
    `product_definition_shape.name = 'nominal model'` only when the design is
    meant to be manufactured; simplified, maximized or minimized models leave it
    empty [src L1: CAx-IF PMI RP v4.1 §4].
  - The CAD system and translator decide face splitting, spline conversion and
    PMI linkage. Construction history (ISO 10303-55/-108/-111, AP203 ed.2) exists
    in the standards but "no commercial CAD software has implemented AP203
    version 2" [src L1: Kim et al. 2008 §1; Sadler 2019 §2].
  - The receiver decides whether to merge split faces; the CAx-IF prefers the
    sender merge a hole's two half-cylinders, "otherwise postprocessors might
    combine neighboring advanced_face entities" [src L1: CAx-IF PMI RP §5.1].
- **Failure modes.** Analytic surfaces exported as splines, which metrology
  software cannot use until it "simplifies" them back [src L1: Zeiss 2002]; a
  hole split into two faces with position and size tolerances attached to
  different entities [src L1: Lipman–Kwon 2021 §6.1.5]; repair that inverts
  intent [src L1: González-Lluch 2017]; at least $1B a year of rework in US
  automotive alone, partly from "different scale and closure tolerances"
  [src L1: Brunnermeier–Martin 1999].
- **Mature practice.** Keep one tessellated entity per exact entity and a
  geometric link back to the canonical surface "to ensure exact measurement and
  positioning" [src L1: CAx-IF 3D Tessellated RP v1.1 §5.4]; pass face and edge
  UUIDs through to metrology (QIF) [src L1: CAx-IF/DMSC cross-domain RP 2026 §5].
- **Requirements for us.** Read and keep surface kind per face, seam/periodic
  structure, `shape_aspect` and `id_attribute` links, persistent UUIDs and the
  nominal-model flag; never "beautify" (snapping a near-analytic spline is a
  geometry change); report near-analytic faces as facts [own L1].
- **Mapping.** Design: nothing. Papers: none before this lane. Code: product
  names, colours and material/density are read
  [code `native/step_source/main.cpp:279-302`]; no `shape_aspect`, UUID or
  nominal flag.
- **Mark: absent.**

### S2. Tolerance and metrology semantics: what the caller's ε means

- **Purpose.** Turn a caller's accuracy request into a stated, checkable meaning,
  and keep it apart from the other tolerances that travel with the data.
- **Inputs → outputs.** Caller request (ε, units, optional angle/edge caps); the
  file's units and uncertainty contexts; optionally PMI → one request per run
  with its meaning recorded, plus warnings.
- **Five quantities that get conflated** [src L1 unless marked]:
  1. design tolerance: a zone or size band (ASME Y14.5 / ISO 1101; profile zones
     differ: ASME offset surfaces, ISO swept sphere) [src Morse 2018 Table 3];
  2. model accuracy / coincidence tolerance: STEP
     `uncertainty_measure_with_unit`, "the largest gap anticipated between
     elements that should be deemed coincident" [src PDES 1998 §2.7.1];
  3. measurement uncertainty and the decision rule (GUM, JCGM 106, ISO 14253-1;
     simple acceptance can be wrong up to 50% near a limit) [src JCGM 106 §8];
  4. specification/correlation uncertainty [src Morse 2018 §2];
  5. the mesh chord deviation. No source equates it with any of the others
     [own L1].
- **Decisions and deciders.** Designer (tolerances, standard, envelope vs
  independency default [src CAx-IF PMI RP §4]); translator (whether PMI
  survives: in NIST CTC-05, 76% of STEP annotations survived as representation
  and 39% as presentation [src Cheney 2015]); inspector (decision rule);
  metrology software (exact geometry by default; PC-DMIS
  `PointmapExactComputation = 1`, tessellation for display [src Hexagon docs]);
  the operator, for ε (journal: "ε is a request").
- **Failure modes.** Semantic value differing from displayed value; lost
  symbols; mixed units inside NIST test parts; two uncertainty contexts in one
  file four orders apart (FTC-10/11); tessellated vertices 0.009 in off exact
  geometry in neutral files [src L1: Cheney 2015 slides 28, 54-56, 62; NIST FTC
  files].
- **No chord-to-tolerance rule exists** in the literature read. JCGM 106 defines
  Cm = T/(4u) and calls 10:1 and 4:1 rules "ambiguously or incompletely defined"
  [src]. A 10:1 analogue would give δ ≤ T/20 for features of size; this is an
  analogy, not a published rule [own L1]. Vendor STL guidance is per-printer
  (1/20 layer thickness; 0.01 mm) [src L1: Hubs, Strongparts web pages].
- **One-sided bias [own L1].** Vertices on the surface make inscribed chords: a
  hole of radius R gets mating diameter 2(R−δ); two-point sizes vary by 2δ around
  it; a profile zone t loses δ of one half-width.
- **Mapping.** Design §4.1 (ε a request in world units), §9.6 (0.005/0.025 mm for
  unit-trustworthy inputs; 1e-3/1e-4 of the diagonal; relative only for
  arbitrary-scale inputs), §4.2 and journal (source discrepancy reported per
  edge, never charged against ε). Papers: tolerant-geometry-healing §3.1 (PDQ
  values), fast-tessellation §5 (consumer resolutions). Code: ε is the
  `--positional-mm` query [code C1]; one uncertainty, the maximum over all
  instances, stored unchecked [code `native/step_source/main.cpp:277,427`];
  decoded and checked ≥ 0, used by nothing [code `native/mesher/source.cpp:726`].
- **Mark: covered (assumption).** The defaults are operator decisions; their
  relation to tolerance bands is unsourced.
- **Contradictions.** (a) The relative default 1e-3 of the diagonal exceeds whole
  tolerance bands of real toleranced parts (0.1 mm on 100 mm versus FTC-07's
  0.076 mm band) and the CAx-IF says relative checks break below 20 mm
  [src L1, L3: CAx-IF GVP §4.13.1]. (b) "Report source error alongside" is
  right, but downstream comparison sees the sum; report the combined bound too
  [own L1]. (c) Uncertainty must be read per representation context, not as one
  maximum [src L1, L2].

### S3. File exchange and reading

- **Purpose.** Turn exchange bytes into typed, unit-explicit definitions,
  occurrences, styles and per-entity diagnostics, without silently changing them.
- **Inputs → outputs.** Part 21 (and `.stpZ`), IGES, Parasolid XT, ACIS SAT/SAB,
  3DM, JT, plus the partners' exchange agreement (VDA 4950) → the source store the
  mesher sees.
- **Decisions and deciders.** Sender: geometry kinds, units, plane-angle unit,
  uncertainty, whether pcurves are written, `master_representation` (Part 42:
  "all characteristics … are derived from the master representation"
  [src L2: ISO 10303-42 via IFC4.3]). Recommended practices: degrees on write,
  angle parameters in the context's plane-angle unit, `mapped_item` with
  `axis2_placement_3d` only, "Receiving system chooses whether to use the 3D
  curves or recalculate them" [src L2: PDES 1998 H.1.1, H.1.9, H.1.15]; "Scaling
  and mirroring shall not be applied in the context of assemblies"
  [src L2: PDM Schema Usage Guide 4.3 §4.7.2]; mirrored parts are separate
  products [src L2: CAx-IF Alternative Shapes v1.0 §4]. Receiver: units, master
  representation, repair, refusal granularity, which alternative shape.
- **Failure modes (silent unless marked)** [src L2]:
  - units defaulted with a warning when a context is missing or unrecognised;
    default plane angle is degrees; double conversion of inch files; IGES with
    an empty unit flag is legally inches; XT has no unit field;
  - OCCT translation-layer edits listed in finding 1;
  - mirror read as rotation; scale accepted unchecked;
  - Part 46 style priority ignored; OCCT 8.0.1 binds a dangling STYLED_ITEM to
    whatever entity sits at that rank (OCCT #1578);
  - open shells accepted as solid outers; alternative shapes merged;
  - missing external references reported as success (OCCT 0029873); Part 21
    edition-3 REFERENCE/ANCHOR sections not lexed;
  - truck drops a failed edge and misnumbers later ones while returning Ok;
  - no parallel parsing in OCCT; Foxtrot parses about 60 MB/s in parallel.
- **Requirements (from L2, condensed).** Key every record by (file SHA-256,
  Part 21 `#id`) plus UUIDs; units and every uncertainty per context, refusing
  missing units rather than defaulting; parameter-space unit maps per surface
  kind (angle on periodic surfaces; cone v = length/cos(semi-angle)); carry and
  honour `master_representation`, recording "pcurve absent / recomputed /
  residual" per coedge; per-entity refusal with closure (a refused face marks its
  shell incomplete, ids stay stable, dangling references refuse dependants);
  read placements from raw entities with determinant and scale; product
  structure with SHUO/CDORSI and main-versus-alternative SDR; Part 46 styles;
  validation properties carried for later comparison; shell closure checked, not
  trusted; translation warnings harvested as data.
- **Entity coverage** (L2 table, condensed; H handled, R refused, I ignored):
  `manifold_solid_brep` H; `brep_with_voids` voids lost (finding 1);
  `shell_based_surface_model`, open shells, `faceted_brep`, wireframe R;
  plane/cylinder/cone/sphere/torus/B-spline H; `degenerate_toroidal_surface`
  indistinguishable from a torus (OCCT drops `select_outer`); surface of
  revolution and linear extrusion R (both convert exactly to rational B-splines
  in AP242, since the swept curve is restricted to line, conic, polyline or
  B-spline [src AP242 `advanced_face` WR5]); `offset_surface` R (not legal in an
  advanced face; arrives only from surface models or procedural formats);
  polyline edges R (legal); hyperbola/parabola R; inch and SI-prefixed units R;
  plane-angle unit read but never checked; AP242 tessellated entities I;
  validation properties I; PMI I; IGES, XT, SAT, 3DM, JT: no reader.
- **Mapping.** Design §3 (keep the OCCT 8.0.1 XDE bridge, extended: pcurve
  reconstruction off, per-entity refusal, units to mm, extra surface kinds or
  B-spline conversion with reserved error; owned reader later), §5.1. Papers:
  step-shape-processing-lifecycle (ShapeProcess flags only), om10-step-source-study,
  om10-tolerant-incidence-reading, whole-mesher-native-runtime (strict native
  reader proposed). Code [C1]: `native/step_source/main.cpp` (STEP only; mm only
  at :246; mixed units refused at :247; surfaces :198-210; curves :172-177;
  per-file throw everywhere, :431-432; non-SameParameter coedges get a 9-sample
  straight-line t→s map, :402, not a measured correspondence); decoder narrows
  to surface degree ≤ 3 (`native/mesher/source.cpp:479`); `native/source_uv/`
  reconstructs pcurves with OCCT ShapeConstruct/ShapeFix
  (`source_uv.cpp:235-240`).
- **Mark: partial.**
- **Contradictions.** (a) "Pcurve reconstruction off" cannot be achieved through
  OCCT's translation layer (finding 1); our own `source_uv` also reconstructs
  them. (b) "Units converted to mm" is necessary but not sufficient (angle units,
  per-kind parameter maps, per-context uncertainty, exact factors). (c) Reserving
  conversion error for revolution/extrusion is unnecessary (exact), while
  offset surfaces need a computed bound. (d) "Owned reader later" is in practice
  "partly now": raw-entity reading of pcurves, transforms and SDR selection is
  needed on top of OCCT, and the design must say which layer wins when they
  disagree [src L2; own].

### S4. Validation and data-quality analysis

- **Purpose.** Decide whether the input is fit for the requested use before
  effort is spent and before healing changes the evidence; and validate the
  translation itself against what the sender declared.
- **Inputs → outputs.** The B-rep as received; sender declarations (closure
  uncertainty, geometric/assembly validation properties, cloud-of-points,
  optional PDQ Quality Stamp); a threshold profile → per-entity findings,
  per-criterion counts, a per-part verdict, the profile and tool version.
- **Decisions and deciders.** The standard defines what each check measures;
  recommended values are non-binding; companies override them (Isuzu/Honda 0.5°
  tangency against JAMA's 0.01°); CAE quality thresholds are deliberately left to
  the solver; translation acceptance is "an agreed tolerance" [src L3: JAMA V5.1
  §2.1-2.2; JAMA 2025; CAx-IF GVP §4.13]. The ISO 10303-59 model separates
  criterion, measurement accuracy, inspection result with judgement, summary
  counts and instance reports with extreme locations [src L3: AP242 AIM pages].
- **Failure modes.** Checkers disagree by algorithm and platform [src JAMA §7.4];
  Parasolid returns at most the most serious fault per entity and skips later
  groups [src FD §30.2]; OCCT's default edge-on-surface check is sampled
  ("possible not correct result") [src `BRepCheck_Analyzer.hxx:87-91`];
  NVIDIA's validator heals before validating by default [src usd-brep
  `brep_geometry_validator_main.cpp:17`]; tolerance-legal gaps trigger healing
  that creates tiny elements [src VDA 4955 §3.1.6.1]; hidden thresholds (ACIS
  heal); relative centroid checks meaningless under 20 mm [src CAx-IF §4.13.1].
- **Check classes and owners.** L3's catalog assigns 20 classes to ingress
  refusal, healing classification, meshing, export or offline oracle. Thresholds
  the literature gives: gaps ≤ 0.01 mm (JAMA), 0.02 mm assumed accuracy
  (CAx-IF), tangent ≤ 0.01° and crease ≥ 0.2° with the band between a defect
  (JAMA G-SH-NT), COPS sharp at 1.0°, tiny edge ≥ 0.01 mm, tiny face ≥ 1e-4 mm²,
  validation-property bands (volume/area green < 1%, industry 0.5%; centroid
  0.02 mm + 0.1% above 20 mm), tessellated validation properties green < 0.01%
  [src L3].
- **Requirements.** Measure the input before healing and again after, against
  the same profile; ordered checks (syntax → topology → consistency → geometry →
  PDQ → validation properties) each passed/failed/skipped/indeterminate; the
  profile as data with provenance; read and compare sender declarations; state
  each measurement's method and accuracy [own L3].
- **Mapping.** Design §5.1 (refuse non-finite and malformed values per entity;
  stored tolerances as metadata), §5.2 measures every edge sample (which is a
  validation measurement), §9.4 (corpus classes "needed healing"). Papers:
  tolerant-geometry-healing §3 (PDQ values), test-plans-cad (usd-brep's 191-rule
  ingress validator as a refusal catalog), test-plans-meshgen A1-A6. Code [C1]:
  decoder checks non-finite, positive weights, knot rules, inventories, loop
  closure by vertex id, monotone maps (`native/mesher/source.cpp:86-690`);
  BRepCheck failure is only a severity-1 diagnostic
  (`native/step_source/main.cpp:328-330`); Python mirrors the checks
  (`imported_source.py:263-403`); no self-intersection or solid-validity check.
- **Mark: partial.**
- **Contradictions.** (a) The design's measurement happens on OCCT-translated
  geometry (finding 1), so "the input before healing" is not what is measured.
  (b) No indeterminate state (S15). (c) Thresholds hard-coded anywhere conflict
  with "thresholds are agreements" [src L3]; the design marks η, β, G as flags,
  which is consistent, but records need threshold provenance.

### S5. Healing, repair and tolerant modelling

- **Purpose.** Decide, per edge and vertex, where the shared boundary lies when
  the 3D curve, pcurves and surfaces disagree, without changing the source.
- **Inputs → outputs.** Decoded B-rep → per-edge class, resolved positions,
  per-occurrence UVs, measured discrepancy, owner face, vertex moves, refusals.
- **Decisions and deciders.** Representation authority differs by system:
  Parasolid and ACIS make the pcurve-on-surface primary for tolerant edges; OCCT
  and openNURBS make the 3D curve primary. Healers re-intersect transverse edges
  and give tangent edges to an owner face (analytic over freeform, lower
  curvature fixed) [src tolerant-geometry-healing §1, §5.1]. Meshers on imperfect
  CAD (Steinbrenner, Henshaw, OCCT BRepMesh) choose one representative curve per
  edge and push the mismatch onto the other face [src same, finding 4].
- **Failure modes.** Tolerance inflation absorbing error and then coarsening the
  mesh (OCCT's 2× floor; Gmsh A319 at 6.77); order-dependent results;
  equal-parameter comparisons; "healed" as a flag; cascading tolerance growth
  [src same §2.6; Segal 1990].
- **Mapping.** Design §5.2 R1-R6 and V1-V4, §7 staged resolver; thresholds η = ε/16,
  β = ε/2, G = min(4ε, ¼ lfs) are calls. Papers: tolerant-geometry-healing (the
  full study), om10-tolerant-incidence-reading. Code: none of R1-R6 exists; the
  old path measured curve-versus-surface residuals as enclosures and refused
  (`native/mesher/geometry_correspondence.cpp:215-228`); `source_uv` requires
  17-point-per-span correspondence ≤ `source_residual_mm`
  (`native/source_uv/source_uv.cpp:75-90`).
- **Mark: covered (assumption).** The research is strong; the thresholds and the
  owner rule order are unsourced for a mesher (the healing paper says so).
- **S5b, reconciliation from well-conditioned neighbours,** is the M2 lane
  (brief `M2-neighbour-reconciliation.md`): whether the desired final position
  comes mostly free from trusted neighbours, small versus big leaps, second-pass
  branch choice. Listed here; not researched by this lane.

### S6. Simplification: defeaturing, virtual topology, slivers, small features

- **Purpose.** Remove or hide detail that forces unwanted elements, without
  changing what the downstream use needs.
- **Inputs → outputs.** A valid B-rep → a modified B-rep, or an overlay
  (virtual) topology with history, or a mesh-level edit [src L4: Beall 2003;
  Shephard 1998; Sheffer 2000; Thakur 2011 §2.1].
- **Decisions and deciders.** CAE practice ties "small" to element size (Cubit:
  curves ≤ 0.25 h; Beall: 25-30% of target size); Sheffer ties connect operations
  to source tolerance; Owen 2019 ranks ML suggestions for a human; Buffa et al.
  show size alone is not a criterion even for analysis [src L4].
- **Failure modes.** Operations fail often (Cubit virtual collapse curve failed
  14,743 of 14,955 times on 94 GrabCAD parts); one-neighbour collapse is
  asymmetric; collapses need validity checks against unwanted dimensional
  reduction; eliminated features return under refinement; virtual merges bound
  deviation by element size, not tolerance [src L4: Owen 2019 Table 3;
  Sheffer 2000/2001; Shephard 1998].
- **For manufacturing output.** Parasolid says its small-feature option is for
  speed only, not for feature suppression [src L4: FD §107.2.15]; Stratasys:
  "cannot build the model any better … than the STL" [src L4]. Simplification
  below ε is a mesh-only operation inside the fidelity contract; above ε it
  changes the part [own L4].
- **Mapping.** Design §5.2 item 5 (edges inside their end vertices' β balls
  collapse; faces within β of their boundary mesh from boundary vertices alone;
  virtual, bottom-up, never in the source). Papers: tolerant-geometry-healing §5.6
  and §7.2. Code: none in the current path.
- **Mark: partial.**
- **Contradictions.** "Mesh from boundary vertices alone" does not by itself keep
  every triangle within ε on a curved or non-convex small face; it needs a proof
  or an authored-family oracle [inf L4]. Collapse validity (no unwanted
  dimensional reduction) and symmetric dispersal are not stated as checks. Collapse
  depends on ε, so collapse results must be keyed by ε in any cache [inf L4].

### S7. Topology construction and shared boundaries

- **Purpose.** Produce a conformal mesh in which faces meet along shared
  boundaries by identity.
- **Mature practice.** Bottom-up: vertices, then each edge once, then faces from
  those chains (Gmsh, SMESH, Netgen, OCCT); Parasolid's default
  `match_topol` "checks that the boundary vertices on a facet mesh always match
  … (possibly splitting facets in the neighboring facet mesh)"
  [src L4: Parasolid FD §107.2.2]. Every persistent-mesh system discretizes each
  physical edge once and shares sample identity [src fast-tessellation §3].
- **Decisions.** Chain order and ownership of tolerant edges; degenerate-vertex
  topology (Parasolid `degen`/`sing_topol`); seam double occurrence; pole fans.
- **Failure modes.** Geometry matching leaves gaps when a face locally tightens
  its chord near inner loops (Parasolid §107.2.2.1); unrecoverable edges inside a
  face (Gmsh); UV wire self-intersection after edge discretization (OCCT
  ModelHealer) [src L4].
- **Mapping.** Design §4.3 (one chain per physical edge, one 3D position per ID,
  one UV per (ID, face occurrence); seams two UV occurrences; poles one vertex
  with a fan), §5.2. Papers: owned-mesher-audit §6 (preserve list),
  fast-tessellation §3, parallel-growth-clusters §3. Code: `DefinitionBoundaryForest`
  (`native/mesher/boundary.hpp:147-182`), `PhysicalNodeKey` ("equality of XYZ
  never aliases two nodes", `native/mesher/mesh_values.hpp:21-35`), vertex
  aliases only from declared-degenerate edges (`boundary.cpp:212-290`), seam
  wedges, one seam wedge per face in the cap profile
  (`adaptive_cap_domain.cpp:217`); no non-manifold support [C1].
- **Mark: covered (evidence),** except the feedback path (S20).

### S8. Edge discretization

- **Purpose.** Sample each physical edge once, densely enough for the curve and
  for every incident face.
- **Mapping.** Design §5.2 item 1 and §6.1: sample count = max(curve rule, each
  incident face's admissible length); lines none; circles closed-form sagitta;
  B-spline ⅛Δt²p(p−1)max‖Δ²P‖; rational by recentred Zheng–Sederberg. Papers:
  fast-tessellation §1.4 (rules), nvidia-smtess §3 (the Hermite proxy NVIDIA uses
  understates; sub-samples never test the emitted chord). Code: adaptive curves
  (`native/mesher/adaptive_curves.cpp`) certify each chord with Rational
  acceptance (audit §1); retired with the old path.
- **Mature extras.** Parasolid and Gmsh also apply max edge length and proximity
  limits; Netgen skips edges under 1e-3 of the box and limits close-edge
  segmentation [src L4].
- **Mark: covered (evidence).** Caps beyond the chord rule are S19.

### S9. Face meshing

- **Purpose.** Triangulate each face's domain from its boundary chains so every
  emitted triangle is within ε of its carrier, positively oriented, with no
  n-gons and no silent drops.
- **Mapping.** Design §5.3 (planes boundary-only CDT; cylinders/cones θ-slab
  lines; spheres circumradius rule; tori and B-splines 2:1 dyadic quadtree from
  span bounds, interior quads, trim band by `direct_cdt`; no clip-anyway, no
  collinear ears, refuse untriangulable faces; 3D orientation check; geometry-
  derived termination bounds), §5.4 (in-face clusters), §6. Papers:
  fast-tessellation §1, §4; nvidia-smtess §3, §6; parallel-growth-clusters;
  whole-mesher-algorithms; direct-cdt studies. Code: `FaceTrimSession`,
  `build_packet_mesh`, `construct_region_cdt` on `native/direct_cdt` [C1].
- **Mark: covered (evidence).** Open items: the band has no published angle
  guarantee (parallel-growth-clusters finding 6); termination limits are named but
  not derived.

### S10. Assembly and instancing

- **Purpose.** Turn product structure into placed triangles without re-meshing
  repeated definitions, keeping each placement's tolerance, orientation,
  attributes and identity.
- **Decisions and deciders** [src L7]. Kernels and formats decide legal
  transforms: Parasolid instance transforms "must be a rigid motion or a
  reflection: no scale, shear or perspective"; OCCT `gp_Trsf` allows uniform
  scale and symmetries; STEP assembly placement is rigid by RP (scaled only via
  `cartesian_transformation_operator` with scl > 0); 3MF and USD allow full
  affine; glTF forbids shear. The mesher decides mesh identity (OCCT dedups by
  TShape and stores one triangulation per face in the definition frame). The
  writer decides emitted identity (a colour override forces a separate mesh in
  glTF and 3MF).
- **Failure modes.** Silent tolerance inheritance under scale (OCCT); data races
  when two occurrences lock separately (OCCT 0023260, fixed by keying the lock on
  TShape); wrong winding under mirroring unless emitters reverse triangle order
  when det < 0 (OCCT does `REVERSED xor det < 0`); converters flatten by default
  (Omniverse JT `instancingStyle 0`) [src L7].
- **Semantics table (L7, condensed).**

  | Placement | Tolerance | Orientation and normals | Allowed by |
  |---|---|---|---|
  | rigid | invariant; one mesh | n′ = An | all |
  | mirrored (det −1) | invariant; Parasolid reuses facets | reverse each triangle; transform normals by A | glTF, USD, 3MF (consumer compensates), Parasolid, OCCT; STEP only as a separate product |
  | uniform scale s | world error s·ε_def: mesh at ε/s, or at ε/max s | unchanged after renormalising | 3MF, USD, glTF; STEP via CTO; not Parasolid |
  | non-uniform | ≤ σ_max·ε_def: mesh at ε/σ_max; angle criteria not preserved | n′ ∝ A⁻ᵀn | glTF (TRS), USD, 3MF; not STEP AIM, OCCT gp_Trsf, Parasolid |
  | shear | as non-uniform | as non-uniform | 3MF, USD; not glTF, STEP, Parasolid |

- **Mapping.** Design §5.1 ("each repeated instance of a definition is meshed
  once and placed many times"), §5.5 (STL flattened, 3MF components). Papers:
  owned-mesher-audit §1 (assembly allowances), om10-step-source-study (166
  occurrences of 107 definitions). Code [C1]: one mesh per definition per query
  (`native/mesher/adaptive_model_main.cpp:249-270, 368-449`); placement applied
  only in display (`native/om10/math.hpp:164-178`); per-definition allowance is
  the minimum over its instances of (positional − storage error)/norm
  (`assembly.cpp:161-171`), so scale is accounted for in the arithmetic; but the
  readers refuse scaled and mirrored placements
  (`native/mesher/source.cpp:651-663`; `python/brepgpu/imported_source.py:296-305`)
  while the display accepts any positive-determinant affine instance
  (`native/om10/validation.hpp:177`).
- **Mark: partial.**
- **Contradictions.** Design "mesh once" holds only for rigid and mirrored
  placements; it needs a mesh key (definition, ε in definition units) and
  per-definition write ownership [src L7]. "Mesh once" is not "emit once": colour
  overrides duplicate emitted meshes in glTF and 3MF [src L7]. 3MF components
  that overlap must be united (S13).

### S11. Mesh post-processing and quality improvement

- **Purpose.** Change mesh shape after tessellation: flips, splits, smoothing,
  sliver or cap removal, decimation, repair.
- **Mature practice.** CGAL constrained edges can be split or collapsed "but not
  flipped, nor its endpoints moved by smoothing"; generic remeshers project onto
  the input mesh, not the exact surface (CGAL `do_project` default true);
  Manifold enlarges its collapse tolerance as error accumulates; QEM places
  vertices off the surface [src L5: CGAL `remesh.h:94-195`; Manifold `mesh.h`;
  Garland–Heckbert 1997]. Repair is ill-posed without knowing the downstream use
  [src L5: Campen–Attene–Kobbelt 2012].
- **Legality for a B-rep-bound mesh [src L5 + own].** Allowed inside face
  interiors with oracles rerun: flips of non-chain edges; splits with the new
  vertex evaluated on the exact surface (on a chain, at the exact curve
  parameter, identically for both faces); half-edge collapse to an existing
  vertex; tangential relaxation in (u,v) with exact re-evaluation. Forbidden
  in-core: QEM placement, smoothing projected onto the mesh, positional welding
  across faces, nearby merge, hole filling, overlap union, moving B-rep vertices,
  flips across chains. Separate downstream stages (association degraded):
  decimation, global remeshing (fTetWild, MMG), convex decomposition, quantized
  export.
- **Mapping.** Design §5.3 (band refinement inserts interior points only; no
  n-gons), §4.7 (caps above about 150° refined away). Papers:
  parallel-growth-clusters §3 (band CDT quality targets), test-plans-meshgen D10,
  D11. Code: no post-processing in the current path; the old `brepgpu-mesh` and
  the Python mesher had Lawson flips (`native/mesher/face_engine.cpp:1192-1215`;
  `python/brepgpu/imported_meshing.py:839`) [C1].
- **Mark: partial.** No CAE profile (needle limits) exists; the 150° cap rule is
  not tied to a bound (S12).

### S12. Normals, attributes and level of detail

- **Purpose.** Supply shading normals, crease structure, tangents and UVs,
  colours and materials, face and part IDs, and coarser versions.
- **Evidence.** Corner attributes and creases: "along a crease … each vertex has
  two distinct normals" [src L5: Hoppe 1996 §2]. Facet-normal error grows with
  circumradius: ∠ ≤ arcsin(√3R/ebs) [src Khoury–Shewchuk 2019]; large angles
  make a paraboloid "look like a washboard" [src Shewchuk 2002]. Industrial
  visual quality is judged by reflection lines; geometric tessellation makes
  tessellation discontinuities indistinguishable from real ones
  [src Guthe–Balázs–Klein 2006]. LOD carriers: AP242 alternative tessellations
  with accuracy parameters, 3MF Production fullres/lowres, Nanite cluster DAGs
  with locked group boundaries [src L5].
- **Requirements.** CAD-native LOD by re-tessellation at a coarser ε from the
  B-rep (association kept); QEM/progressive simplification only as a downstream
  stage; face IDs in every format that can carry them (3MF triangle sets, glTF
  `_FACEID`, USD GeomSubset) [own/inf L5].
- **Mapping.** Design §4.7 (exact per-corner normals per face occurrence; G1
  edges may share; creases never; caps above about 150° refined away), §5.2
  item 6 (continuity class from θ: ≤ 0.01° G1, ≥ 1° crease, between follows the
  source's regularity else crease). Papers: fast-tessellation §1.5,
  tolerant-geometry-healing §7.2. Code [C1]: display normals evaluated
  analytically from the carrier at each vertex UV
  (`native/om10/math.hpp:44-158`); faces never share display vertices, so every
  face boundary renders as a hard edge (`native/mesher/adaptive_scene.cpp:160-165`);
  material is RGB with fixed roughness 0.5 and STEP alpha dropped
  (`adaptive_scene.cpp:349-357`); no LOD.
- **Mark: partial.**
- **Contradictions.** Per-corner normals cannot reach STL or 3MF consumers
  (finding 3); the 150° threshold is a choice with no bound behind it (at 150°,
  R equals the longest edge, about 1.7× the equilateral case [own L5]); JAMA puts
  0.01-0.2° in a defect band, which the design's "follow encoded regularity"
  handles only if regularity is read (it is not today).

### S13. Export formats and their validity rules

- **Purpose.** Serialize the mesh to bytes a consumer accepts.
- **Rules that bind us** [src L5]:
  - STL: float32, no units, no instances, facet normal only, each edge shared by
    two triangles; readers ignore or recompute normals and rebuild topology by
    exact-position merge.
  - 3MF Core 1.4.0: each edge shared with exactly one other triangle by index;
    consistent outward orientation; distinct vertices; SHOULD NOT duplicate
    vertices; SHOULD NOT self-intersect (consumers MUST accept); `unit` default
    millimetre; overlapping items or components MUST be united; no normals;
    positions are decimal text but lib3mf and PrusaSlicer store float32.
  - glTF 2.0: positions, normals and tangents float32; one index stream for all
    attributes (a crease duplicates vertices; EXT_mesh_manifold restores the
    2-manifold); metres, +Y up; node matrices TRS-decomposable; the validator
    checks no manifoldness or orientation.
  - USD: `subdivisionScheme` defaults to Catmull-Clark and must be authored
    "none"; `metersPerUnit` defaults to 0.01; points float32; faceVarying normals
    over welded points; usdchecker does not check mesh topology.
  - AP242 tessellated: double precision; explicit link from tessellated faces to
    exact B-rep faces (a ready-made source map); accuracy parameters exist but are
    untested in MBx-IF.
  - OBJ: separate index streams (welded positions with per-corner normals); no
    units. PLY: float or double, no conventions. AMF/JT: normative texts paywalled.
- **Failure modes.** Rounding creates self-intersections (about 12% of
  self-intersecting Thingi10K models under naive rounding) [src Lazard–Valque];
  STL leaks; mm/m and Z/Y conversions for glTF; float32 spacing at 100 m from the
  origin is 7.63 µm, above a 5 µm budget, if world coordinates are baked [own L5].
- **Mapping.** Design §5.5 (internal schema, binary STL, 3MF; flattened STL; 3MF
  components; USD later; validity oracles on written bytes), §4.9 (3MF refuses
  parts whose edges have ≠ 2 uses). Papers: fast-tessellation §5;
  meshing-oracles-study (export displacement bound); test-plans-meshgen B6.
  Code [C1]: no STL, 3MF, OBJ or glTF writer anywhere; BGSRC001, BGACRV01,
  BGATRM01, BGMESH01, BGVIEW01, BGNMESH1 owned formats; atomic directory
  publication via `renameat2(RENAME_NOREPLACE)`
  (`native/mesher/output_directory.hpp:16-53`, Linux only).
- **Mark: partial.**
- **Contradictions.** "Validity on written bytes" must mean validity after a
  reader-equivalent re-import (parse, round to float32, merge by exact position),
  with self-intersection, orientation and manifoldness rechecked [src L5]. The
  design duplicates vertices per face occurrence; 3MF and STL writers must weld,
  glTF writers must emit merge maps [src L5].

### S14. Downstream consumers

- **Purpose.** Slicing, CAM, volume meshing, rendering, physics.
- **What they require, tolerate and silently repair** [src L5]:
  - PrusaSlicer: closes open slice loops with gaps up to 2 mm; morphological
    closing at 0.049 mm; on STL import exact facet match, then only if
    non-manifold a nearby merge; no hole filling; flips everything if volume is
    negative; float32 internally; `gcode_resolution` 0.0125 mm.
  - Cura: unions overlaps by default; stitches small holes; maximum resolution
    0.5 mm (minimum segment); maximum deviation 0.025 mm; drops loops under 1 mm
    circumference.
  - Mesh CAM (OpenCAMLib): an unordered triangle list, point-sampled drop
    cutter; B-rep CAM (Fusion): "machining tolerance is the sum of the tolerances
    used for toolpath generation and geometry triangulation".
  - TetGen merges coplanar facets and close vertices unless told not to;
    fTetWild's default envelope is 1/1000 of the diagonal (30 µm on a 30 mm
    part, 6× a 5 µm request); MMG defaults hausd 0.01 and ridge angle 45°;
    Netgen-from-STL rebuilds feature edges from angles.
  - Rendering: holes and cracks are the only absolute failures; reflection lines
    expose tessellation.
  - Physics: PhysX cooking welds and removes degenerate triangles; dynamic
    triangle meshes need an SDF; convex pieces ≤ 255 vertices; CoACD needs a
    2-manifold solid. Gear contact through faceted meshes and convex hulls is not
    credible [inf L5].
  - Requirement structure: visualization tolerates almost everything but holes;
    rapid prototyping needs closed, intersection-free, non-singular input;
    simulation also needs element shape [src Campen 2012].
- **Mapping.** Design §2 goal 3 (slicers and CAM accept without repair), §4.9.
  Papers: fast-tessellation §5. Code: in-repo consumers are the Python readers
  and examples (`imported_oracles.py`, `nvidia_reference.py`, `render_*.py`)
  [C1].
- **Mark: partial.** No consumer conformance targets are chosen.
- **Correction to an earlier paper.** Fast-tessellation §5's "Stratasys
  recommends a 0.0254 mm-class chord" misreads the source: the document
  transcribes per-CAD settings, and 0.001 in appears only as Solid Edge's
  "Conversion Tolerance" [src L5: Stratasys 2015 pp. 4-6].

### S15. Error reporting and diagnostics to people

- **Purpose.** Tell a person or calling program what is wrong, where in the
  source's own identifiers, how bad, what was done, and what remains true.
- **Mature practice** [src L3]. Parasolid: fault token, two entities and a
  position, capped by `max_faults`; separate check-failure tokens ("should not be
  taken to mean that the body itself is invalid"); facet conversion fails the
  whole body by default, partial conversion opt-in with a list of faces left
  without geometry. ACIS: findings and change logs as attributes on entities.
  OCCT: 36-value `BRepCheck_Status` with no magnitude; `Message_Alert` merges by
  default and drops later occurrences; BRepMesh ORs every face's status into one
  run integer and still sets done. SMESH: per-sub-shape error with a
  "computed anyway" warning state. Gmsh: no abort on error by default, last error
  only. usd-brep: 228 requirement IDs, prim-level location only. NIST SFA: errors
  cite the recommended-practice section they violate.
- **Failure modes.** Status that lies, lost localisation, short-lived reports,
  actions taken without report, merged summaries, text-only logs, sentinels in
  output data, defaults that hide information [src L3].
- **Requirements.** One record per finding (run id and input hash; source entity
  id and type; owner chain; class with PDQ code and ISO 10303-59 name; measured
  value, threshold, provenance, method and accuracy; extreme location; severity;
  action with before/after; checker status); per-criterion summaries with
  Quality-Stamp fields; run provenance; no merging by default; an exit status
  that cannot claim success when any face is refused or indeterminate; the human
  view generated from the same records [own L3]. Under doctrine, JSON is
  permitted only for the external presentation of these records; internal
  records are owned native values (BLACKBIRD "Internal data formats").
- **Mapping.** Design §4.5-4.6 (per-face accounting; valid / valid_goal_unmet /
  refused; caps never valid), §5.5 (per-edge and per-face records). Papers:
  test-plans-cad finding 2 (silent per-face failure dominant), test-plans-meshgen
  E3-E4. Code [C1]: `ErrorCode` exceptions
  (`native/mesher/values.hpp:11-25`); the model prints one error line and exits 1
  (`adaptive_model_main.cpp:561-567`); a worker exception cancels the whole
  component; `faces.tsv` lists only successful faces; the source snapshot's
  `DIAGS___` table holds five diagnostic kinds.
- **Mark: partial.**
- **Contradictions.** No `indeterminate` state; partial output as the default
  conflicts with Parasolid's default and with automotive receivers that reject
  whole models [src L3]; the design's action records are per edge, but face- and
  body-level silent actions (orientation flips, algorithm fallbacks) also need
  records [inf L3]; the stored-versus-measured tolerance report is one-sided
  (measured above stored is the harder fault) [src L3].

### S16. Incremental and associative remeshing after edits

- **Purpose.** After an edit, produce a mesh of the new B-rep that reuses the
  unchanged parts, keeps references (picks, contact pairs, boundary conditions)
  stable, and regenerates only what depends on the change.
- **Inputs → outputs.** Old B-rep, old mesh and identity records, new B-rep, a
  change description, parameters → new mesh, per-face status (reused, re-meshed,
  new, deleted), old-to-new correspondence.
- **Decisions and deciders** [src L6].
  - Which entities changed: the kernel or host, through its history (OCCT
    `BRepTools_History` Generated/Modified/Removed; TNaming evolutions; Parasolid
    bulletin board and partitioned rollback; or geometric comparison). Naming
    across topology change is the host's problem (Capoyleas–Chen–Hoffmann 1996,
    Kripac, Marcheix 2005, Bidarra 2005), and "neither technique is complete".
  - Invalidation radius: the tessellator. Parasolid's automatic incremental
    faceting refacets changed faces plus any face "adjacent to an edge that has
    been refined" (FD §107.2.16); Sheffer–Üngör 2001: changing a boundary entity
    removes the mesh on it "and all higher dimensional entities that rely on it";
    Sypkens Smit 2011 also frees a two-layer halo.
  - Reuse acceptance: OCCT accepts a stored face when its deflection is below
    1.1× the request (and above 0.9× only if quality decrease is allowed, off by
    default), and checks nothing else (`BRepMesh_ModelPreProcessor.cxx:46-98`).
- **Failure modes** [src L6]. Stale reuse: `BRep_Builder::UpdateFace` replaces a
  surface without clearing its triangulation and TKMesh never reads `Modified()`,
  so an in-place edit can be reused; FreeCAD cleans every shape before display
  meshing for this reason. Hash-key collisions (brepjs moved from `HashCode()`
  to identity keys). Tessellation hysteresis (OCCT's 1.1/0.9 band; Parasolid
  ignores changed facet controls under incremental faceting; Kumar–Manocha drift
  needing joint resynchronization). Seam breakage when a refined edge borders an
  unfaceted face. Algorithm-version drift (FreeCAD resets references on
  element-map version change). Incremental results differing from from-scratch
  results (Sypkens Smit §6.7).
- **Mapping.** Design §7 ("settling or editing an edge invalidates only its
  incident faces; the complication roadmap uses this path"), §10 (fresh versus
  incremental equivalence as a metamorphic oracle), §11. Papers:
  owned-mesher-audit §6 item 5 (generation and retention logic as the template),
  nvidia-smtess §6 item 9 (NVIDIA rebuilds all faces on any edge split),
  fast-tessellation §7 item 8 (OCCT reuse rule). Code [C1]: within one run only —
  `BoundaryEpoch`, per-face boundary generations, retained faces when the
  generation did not move (`native/mesher/adaptive_component.cpp:520-529`),
  `ContactEpochOwner` deltas; nothing is reused across runs.
- **Mark: partial.**
- **Contradictions.** (a) "Incident faces only" is necessary but not sufficient:
  invalidation must be driven by the host's change set, and refined edges
  re-facet their neighbours [src L6]. (b) Deflection-only reuse, OCCT's rule, is
  unsafe and the design must not adopt it [src L6]. (c) Equivalence of
  incremental and fresh results is unstated; without bit-identity it can only be
  tested per face given identical boundary samples [own L6].

### S17. Caching, persistence and provenance

- **Purpose.** Store tessellations so a regeneration with unchanged inputs is
  skipped or reproduced, and answer "where did this triangle come from".
- **Decisions and deciders** [src L6].
  - Key composition. OCCT persists nodes, UVs, normals, triangles and the
    measured deflection but not the requested parameters, while its reuse test
    prefers those parameters, so a reloaded mesh is judged differently from the
    same mesh in session; only the active triangulation of several per-purpose
    ones is written.
  - Identity versus content: "cache keys contain zero identity fragments;
    identity contains zero geometry" (brepjs ADR-0014).
  - Coexistence of B-rep and LODs: JT late-loads LODs and B-rep by GUID but ties
    triangles to faces only by implicit face-group order, which breaks if faces
    are reordered.
  - Provenance vocabulary: W3C PROV-DM (Generation, Derivation, Revision,
    Invalidation).
  - Kernel identities: Parasolid identifiers persist, tags do not; OCCT's shape
    hash is pointer plus location, not persistent.
  - Instancing records: USD prototypes per instancing key; Manifold
    `runOriginalID` + `runTransform`, per-triangle `faceID`, explicit
    merge-vertex vectors "avoiding unreliable floating-point comparisons".
- **Failure modes.** Stale or colliding keys; the "Frankenbuild": a cache keyed on
  inputs is correct only for deterministic tasks, so mixing results from
  non-deterministic producers yields outputs inconsistent with their dependencies
  [src L6: Mokhov–Mitchell–Peyton Jones 2018 §4.2.4, §6.4]; untracked
  dependencies (compiler, CPU, driver) in the key; determinism regressing unless
  tested (Gmsh CHANGELOG).
- **Mature practice.** Verifying traces, constructive traces and early cutoff
  [src Mokhov §2.3, §4.2]; kernel attribute events (split, merge, transfer,
  change, transform, delete) [src Parasolid FD §95.4]; internally deterministic
  parallel refinement is cheap [src L6: Blelloch et al. 2012].
- **Fields the output must carry** (L6, condensed; each sourced there): host
  persistent face id and body id; per-triangle face id; per-vertex
  classification (model vertex, edge or fin, face interior); per-vertex edge t
  or face (u,v); edge sample key (edge id + dyadic path); seam and wedge bindings
  or explicit merge pairs; occurrence id and transform kept apart from
  part-local geometry; generations and status (reused, re-meshed, new, deleted,
  provisional) with revision and invalidation links; a content key per face
  (face geometry digest, boundary-edge sample digests, requested parameters,
  mesher and algorithm version); backend id and determinism class in the key
  whenever bytes are not guaranteed identical; achieved error bound per face and
  edge; purpose tag (presentation, calculation, physics); old-to-new
  correspondence; source operation and history relation.
- **Mapping.** Design §5.1 (per-carrier cache keyed by source identity), §5.5
  (source maps, per-edge and per-face records, canonical facet order). Papers:
  owned-mesher-audit §4 (identity types to keep), whole-mesher-native-runtime
  (sectioned full-mesh binary). Code [C1]: in-memory `SourceCache` and
  `CurveSourceCache` built once per run; SHA-256 of the original STEP and
  `META____` revision strings; `BGNMESH1` embeds the source snapshot and checks
  its digest on read; `FacetKey`, `ConstructionSite.source`; no on-disk cache.
- **Mark: partial.**
- **Contradictions.** Dropping bit-identical output (operator decision) is
  compatible with caching, regression and incremental equivalence only if the
  backend is part of the cache key, or the edge-sample layer stays bit-identical
  across backends while face interiors may differ. CPU and GPU error estimates
  that differ near a threshold choose different sample sets, not just different
  coordinates, so adjacent faces from different backends would not share
  boundaries [src/inf L6]. Today's CPU1/CPU4/GPU byte-identity is a cheap
  regression oracle the draft gives up without replacement; "same backend, same
  input ⇒ same bytes at any thread count" is the least that early cutoff needs
  [own L6].

### S18. Performance engineering at scale, CPU and GPU

- **Purpose.** Minimise wall time and CPU time for thousands of faces whose costs
  differ by orders of magnitude, across two sockets and two GPUs, inside a memory
  budget, and measure it fairly.
- **Evidence** [src L7]. Greedy assignment on mixed processors fails ("CPUs grab
  tasks even if the GPU is about to become available"); HEFT with per-device
  cost models helps up to 2× and tolerates 5% estimate noise (StarPU); list
  scheduling has anomalies (Graham 1966); work stealing runs in T₁/P + O(T∞), and
  under background load the bound degrades with the processor average; OCCT
  hands faces out one at a time with only a surface-type ordering; oneTBB uses one
  arena per NUMA node; Linux automatic NUMA balancing migrates pages after first
  touch and NVIDIA advises disabling it for GPU applications; measurement bias
  from environment size and link order can flip conclusions (Mytkowicz 2009);
  Hoefler–Belli rules for reporting. No peer-reviewed faces/s figures for whole
  assemblies were found.
- **Arithmetic [own L7].** 847 s over 24 cores is 35 s at perfect scaling against
  NVIDIA's 6-8 s, so per-face cost must fall about 5× just to tie on wall time and
  about 37× to match NVIDIA's 22.9 CPU-s. T∞, the largest single face, bounds any
  schedule. NVIDIA's own efficiency is low (22.9 CPU-s over 12 processes is 1.9 s
  ideal against 6-8 s observed), so its wall time is beatable once CPU-seconds
  are competitive.
- **Mapping.** Design §5.5 (work estimate before work, largest-first over all
  definitions, one pool per socket, first-touch memory, GPU driven from its own
  socket), §6.2 (one SoA batch layout for SIMD lanes and warps), §9.5 (pinned
  lanes, load and frequency recorded). Papers: owned-mesher-audit, nvidia-smtess
  §4, fast-tessellation §6, parallel-growth-clusters §4. Code [C1]: serial loop
  over definitions; FIFO single-mutex pool with no stealing or priorities
  (`native/mesher/scheduler.cpp:60-70`); no NUMA or affinity code; no SIMD;
  `-ffp-contract=off` and `--fmad=false` still set
  (`tooling/native_cpu_rigor.cmake:18-23`; `native/mesher/CMakeLists.txt:165`);
  CUDA modules compute bounds and display, never CDT or topology.
- **Mark: partial.**
- **Contradictions.** Largest-first alone is the documented failure on
  heterogeneous hardware; placement needs per-device finish-time estimates
  [src L7]. Two strictly separate socket pools reintroduce static imbalance;
  first touch is undone by AutoNUMA [src L7]. Benchmark accounting must say
  whether NVIDIA's 22.9 CPU-s is per definition or per occurrence [src L7].

### S19+. Sizing field and work estimation

- **Purpose.** Decide target sizes before discretization: chord rule, maximum
  length, proximity between non-adjacent features, grading between a fine face
  and a coarse neighbour; and estimate work per item before work.
- **Mature practice.** Netgen `Analyse` (edge skip below 1e-3 of the box,
  curvature, close-edge proximity, user points); Gmsh mesh-size fields and
  `MeshSizeFromCurvature`/`ExtendFromBoundary`/min/max; Parasolid per-entity
  local tolerances; OCCT `MinSize` default 0.1 × deflection [src L4].
- **Mapping.** Design §5.5 (per-item work estimates from ∫√det M, band and edge
  counts; absurd output refused up front), §6.1 (density rules), §5.6
  (ε-inflated proximity as a density input). Papers: fast-tessellation §4 (optimal
  count), parallel-growth-clusters §2 (work estimates). Code: none in the current
  path.
- **Mark: partial.** Maximum edge length and grading are absent; CAE consumers
  want a cap (S14).

### S20+. Boundary check, retry and face-to-edge feedback

- **Purpose.** Detect when a face cannot be meshed on its settled chains (UV wire
  self-intersection at the chain's resolution, unrecoverable boundary, local
  refinement need) and feed that back into the shared edge, re-meshing every
  incident face, in bounded rounds.
- **Mature practice.** OCCT ModelHealer: pool offending edges across faces,
  re-sample at deflection/3, re-check every incident face, at most 5 rounds, then
  Failure; Gmsh: split the unrecoverable edge, un-mesh and re-queue adjacent faces,
  `MaxRetries`; Parasolid automatic incremental refinement: "any faces adjacent to
  an edge that has been refined will be automatically refaceted" [src L4;
  tolerant-geometry-healing §6.1].
- **Mapping.** Design: absent (§5.2 freezes chains; §5.3 refuses). Papers:
  tolerant-geometry-healing §7.4 item 5 proposes exactly this loop with a
  geometry-derived stop; whole-mesher-algorithms names the coupled edge/cell
  queue with schedule generations as one of two coordination policies. Code: the
  old path had in-run boundary rounds driven by failed certificates
  (`native/mesher/adaptive_component.cpp:509-531`; 3 rounds watch-wide).
- **Mark: absent** in the design.

### S21+. Physical embedding and thin features

- **Purpose.** Ensure no two emitted triangles intersect except along shared
  mesh entities, including after export rounding.
- **Mapping.** Design §5.6 (each face within ε of its surface; faces more than 2ε
  apart cannot intersect; an ε-inflated face-level query before meshing finds
  thin walls, gaps and knife edges, and tightens tolerance or refuses; full check
  offline) [operator: accepted as default]. Papers: whole-mesher-geometry-contract
  §5 ("physical embedding belongs to mesh construction"; complete cells within ε
  can still cross when clearance is below 2ε; incident triangles may overlap away
  from their shared vertex); fast-tessellation §8 question 2 (no cheap a-priori
  rule in the literature read; Li et al. 2026 unread). Code: the old path's
  165 s hot-path contact check found 0 forbidden pairs in 107 definitions
  (audit §0 item 4), moving to the offline oracle.
- **Mark: covered (assumption).** The 2ε argument covers non-adjacent faces; it
  does not cover two triangles of adjacent faces near a knife-edge dihedral
  except through the planning query, whose soundness is not shown; and float32
  export can re-introduce intersections (S13).

### S22+. Imprint, merge and contact interfaces

- **Purpose.** Make touching parts share interface topology (bonded CAE,
  multi-region meshes), or match discretization across a declared contact
  without welding.
- **Mature practice.** Cubit imprint then merge (merge tolerance between the
  largest gap and the smallest feature; default 5e-4); Gmsh `BooleanFragments`;
  Netgen `Glue`; SALOME `Partition`; mesh-level merge and sew in SMESH; Ansys
  shared topology, which forces each copy to become a unique part [src L4].
- **Failure modes.** Slivers or missing imprints at kernel tolerance; tolerant
  imprint cannot imprint intersecting surfaces [src L4].
- **Where it matters for us.** 3MF needs each part closed and manifold and unites
  overlaps, so it does not need shared interfaces; AMF multi-material regions
  share one vertex list; merging welds moving parts, which is wrong for gears and
  pivots [src/inf L4].
- **Mapping.** Design: absent (§2 non-goal: sewing unsewn sheets; §11 mentions
  contact surfaces need faithful density). Papers: none. Code: none.
- **Mark: absent.**

### S23+. Mesh-to-geometry classification map

- **Purpose.** Record, for every mesh entity, the lowest-dimension model entity
  it lies on, and keep that record valid through collapse and simplification
  [src L4: Shephard 1998 §3, §5; Parasolid `fin_fin` tables].
- **Mapping.** Design §5.5 (per-vertex and per-facet source maps: source edge
  node or surface root, generator, owner, revision, edge provenance). Papers:
  owned-mesher-audit §4 (identity types to keep), whole-mesher-native-runtime
  (full-mesh sectioned schema). Code: `FacetKey{generator, owner, revision,
  ordinal}`, `ConstructionSite.source`, scene `facet_sources`
  (`native/mesher/adaptive_trim_types.hpp:95-116`; `adaptive_scene.cpp:489`)
  [C1].
- **Mark: partial.** Collapse history (which source entities a collapsed vertex
  stands for) and per-format carriage (S13) are open.

### S24+. Provisional state and progressive refinement

- **Purpose.** Show a usable mesh at once, then refine it while the user watches
  without pops or cracks; let staged resolution keep provisional results as
  first-class state (operator decision).
- **Mature practice** [src L6]. Progressive meshes: base mesh plus vertex
  splits, geomorphs, "closest living ancestor" for selective refinement (Hoppe
  1996); view-dependent refinement requires the refined mesh to be "unique,
  regardless of the sequence of (legal) transformations" and geomorphs through a
  common vertex front (Hoppe 1997); Reyes bound/split/dice; anytime-algorithm
  properties (recognizable quality, monotonicity, interruptibility; Zilberstein
  1996); a lazy tessellation cache with time stamps and an in-progress state
  (Benthin 2015); Kumar–Manocha resynchronize all patches sharing a boundary
  together.
- **Requirements [own/inf L6].** Our dyadic edge-sample keys already make every
  provisional sample an ancestor of its refined samples, which gives a unique
  ancestor map and therefore geomorphs. A provisional record carries its
  achieved bound, provisional/final status and generation; replacement happens
  per edge-closed region, so all faces sharing a settled edge switch together.
- **Mapping.** Design §7 (provisional state first class: status, queue
  membership, dependent faces; sharpen-in-place later as a display consumer;
  speculative variant patches boundary positions keeping IDs). Code: none.
- **Mark: partial.** Whether provisional meshes may feed physics or contact is
  open.

### S25+. Offline verification oracles

- **Purpose.** Establish, off the product path, that outputs meet the contract,
  with oracles that change exit status.
- **Mapping.** Design §10 (two-sided sound deviation with covering-radius
  correction or bounded Hausdorff; exact per-face UV oracles; topology; offline
  self-intersection with the old contact BVH; metamorphic relations; forced caps;
  per-device predicate conformance; export on written bytes; negative controls;
  mutation testing later). Papers: test-plans-cad, test-plans-meshgen (the
  evidence), meshing-oracles-study, whole-mesher-geometry-contract §7. Code:
  Python original-source readers (`imported_oracles.py:265, 415, 634, 1284,
  1664`) [C1].
- **Mark: covered (evidence).** Open: the export oracle must be
  reader-equivalent (S13); oracles need an `indeterminate` outcome distinct from
  pass and fail (S15); the per-face deviation target depends on S5b.
</content>
</invoke>

## 3. Contradictions

### 3.1 Design against the literature

| # | Design (draft) | Literature | Stage |
|---|---|---|---|
| X1 | §3: the OCCT bridge with "pcurve reconstruction off" keeps source authority | OCCT's translation layer drops plane pcurves, recomputes missing ones, ignores `master_representation`, merges vertices, reads mirrors as rotations, merges alternative shapes, independent of ShapeProcess flags [src L2] | S3, S4, S5 |
| X2 | §5.2-5.3: an edge chain is final before any face consumes it; an untriangulable face is refused | Gmsh, OCCT and Parasolid re-split shared edges on face trouble and re-mesh all incident faces, in bounded rounds [src L4] | S20 |
| X3 | §5.1: each repeated definition is meshed once and placed many times | Safe only for rigid and mirrored placements; scale changes world error; the mesh key needs ε in definition units [src L7] | S10 |
| X4 | §5.5: validity oracles on written bytes | Consumers re-import as float32 and weld by exact position; rounding creates new self-intersections in about 12% of self-intersecting models [src L5] | S13, S21 |
| X5 | §4.7: exact per-corner normals; §5.5: alpha export is internal schema, STL, 3MF | STL and 3MF carry no vertex normals; only glTF, USD, OBJ, PLY, AP242 do [src L5] | S12, S13 |
| X6 | §5.5: 3MF components for instances | Overlapping components MUST be united, so touching parts fuse [src 3MF Core §4.2]; colour overrides cannot ride on components [src L7] | S13, S10 |
| X7 | §4.6: three statuses valid / valid_goal_unmet / refused | Every mature checker separates "could not evaluate" from "invalid" [src L3] | S4, S15, S25 |
| X8 | Journal default: mesh the rest, mark the part incomplete | Parasolid facet conversion fails the whole body by default, partial output is opt-in with a mandatory list; automotive receivers reject whole models [src L3] | S15 |
| X9 | §9.6: relative ε 1e-3/1e-4 of the diagonal | 1e-3 gives 0.1 mm on a 100 mm part, larger than whole tolerance bands in NIST test parts; CAx-IF relative checks break below 20 mm [src L1, L3] | S2 |
| X10 | Non-goal: bit-identical output; §7: incremental remesh via generations | Input-keyed caches are correct only for deterministic tasks; early cutoff needs an equality test [src L6] | S16, S17 |
| X11 | §5.5: largest-first scheduling, one pool per socket, first touch | Largest-first alone is the documented failure on mixed CPU/GPU; AutoNUMA migrates first-touched pages [src L7] | S18 |
| X12 | §3: revolution/extrusion converted to B-spline with conversion error reserved out of ε | In AP242 advanced faces both convert exactly; offset surfaces are not legal there and need a computed bound [src L2] | S3 |
| X13 | §3: units converted to mm at ingress | Also needed: plane-angle unit, per-surface-kind parameter maps (cone v/cos α), per-context uncertainty, exact factors [src L2] | S3 |
| X14 | §5.2 item 5: faces within β of their boundary mesh from boundary vertices alone | Virtual merges bound deviation by element size; boundary-only meshing of a curved or non-convex small face needs its own ε argument [src/inf L4] | S6 |
| X15 | §4.7: caps above about 150° refined away | Normal error bound depends on circumradius over local feature size; 150° is not tied to a bound [src L5: Khoury–Shewchuk] | S12 |
| X16 | §5.6: self-intersection by construction from the 2ε argument plus an ε-inflated query | The geometry contract study (§5) says embedding must be resolved in construction with robust contact classification; no cheap a-priori rule exists in the literature read [src whole-mesher-geometry-contract §5; fast-tessellation §8] | S21 |

### 3.2 Design against its own source papers

- The healing paper proposes a pooled, geometry-bounded face-to-edge loop
  (tolerant-geometry-healing §7.4 item 5); the design drops it (X2).
- The geometry contract study's "original spatial edges/vertices as physical
  skeleton authority" (§1) is superseded by R3 re-intersection, which the design
  marks as a call; the study's other claims (whole-cell bounds, onto
  correspondence, embedding in construction) were written for the certified
  regime and need restating for the uncertified one, not silent inheritance.
- Fast-tessellation §5's Stratasys 0.0254 mm "recommendation" is a misreading
  (S14).

### 3.3 Design against the code (things the design assumes exist)

- Per-entity refusal: every layer refuses per file today
  (`native/step_source/main.cpp:431-432`; `native/mesher/source.cpp:642-643`;
  `python/brepgpu/imported_source.py:401-402`).
- Uncertainty as metadata: one maximum over all contexts is stored
  (`native/step_source/main.cpp:277`).
- `-ffp-contract=off` and `--fmad=false` remain set in the build profile
  (`tooling/native_cpu_rigor.cmake:18-23`; `native/mesher/CMakeLists.txt:165`)
  although the operator released them.
- Repository docs still describe the certified regime (AGENTS.md;
  `docs/ALPHA_QUALIFICATION.md`); design §8 already schedules this.

### 3.4 Code defects found by reading (not run)

- `BREP_WITH_VOIDS` void shells dropped silently: only `raw->Outer()` is walked
  (`native/step_source/main.cpp:313,332`), and `StepShape_BrepWithVoids`
  subclasses `StepShape_ManifoldSolidBrep`
  (`quarantine/new-iteration/occt/src/DataExchange/TKDESTEP/StepShape/StepShape_BrepWithVoids.hxx:32`).
- Non-SameParameter coedges get an invented 9-sample straight-line t→s map, not
  a measured correspondence (`native/step_source/main.cpp:402`).
- The plane-angle unit is read but never checked (`native/step_source/main.cpp:235-236`).
- The display accepts scaled affine instances the mesher refuses
  (`native/om10/validation.hpp:177` against `native/mesher/source.cpp:651-663`).

These are recorded for the operator; this lane edits no code.

## 4. Gaps, ranked, as stub briefs for M4

Risk is weighed against the operator's goals in order: output people can send
to machines (correctness), then speed against a steel-manned NVIDIA, then
stunning output, then not boxing in the roadmap (explosion, physics, editing,
sharpen-in-place), all without overfitting to the watch. Gaps that need an
operator decision are listed separately in §4.2; several research gaps depend on
them and say so.

### 4.1 Research gaps

**G1. What the ingress bridge changes before we see the geometry.** *(S3, S4, S5; X1)*
- Purpose: establish exactly which translation-layer edits OCCT 8.0.1 makes on
  STEP input regardless of ShapeProcess flags, how often they fire on real
  inputs, and what must be read from raw entities instead.
- Decision it blocks: M5's ingress section (keep the bridge as is, keep it with
  raw-entity overrides for pcurves, transforms and representation selection, or
  bring the owned reader forward); which layer is authoritative when they
  disagree; whether the NVIDIA arm "our unprocessed read" is what it claims.
- Knowledge: reference code (OCCT StepToTopoDS, StepToGeom, STEPControl, XCAF)
  plus measurement on the fleet (raw entity versus translated shape, per entity,
  over the M3 corpus).
- Leads: L2 file:line list (`StepToTopoDS_TranslateEdgeLoop.cxx:110-112, 399,
  603-616, 844-871`; `StepToGeom.cxx:2187-2210, 2340`;
  `StepToTopoDS_Builder.cxx:150, 217, 235`; `DESTEP_Parameters.hxx:167-172`;
  `STEPControl_ActorRead.cxx:412-419, 2450-2462`); truck-stepio's honouring of
  `master_representation` (`src/in/mod.rs:2543-2570`); 2026-10-15
  step-shape-processing-lifecycle; `native/source_uv/source_uv.cpp:235-240`.
- Done: a table of every translation-layer edit with trigger, effect, frequency
  on the corpus and a disposition (accept, read raw, refuse), and a stated
  authority rule.

**G2. Face-to-edge feedback and its termination.** *(S20, S7, S9; X2)*
- Purpose: define how a face that cannot be meshed on its settled chain feeds
  back into the shared edge, which faces are invalidated, and what bounds the
  rounds by geometry rather than by a count.
- Decision it blocks: M5 §5.2-5.3 coordination policy (frozen chains plus
  refusal, or a coupled edge/face queue with schedule generations); interaction
  with staged resolution, caching and determinism.
- Knowledge: reference code (OCCT `BRepMesh_ModelHealer`, Gmsh
  `meshGFace.cpp`/`Generator.cpp` PENDING/retry, Parasolid incremental
  refinement semantics) plus literature on termination of coupled boundary
  refinement.
- Leads: L4 §1.2 and §1.4; tolerant-geometry-healing §6.1 and §7.4 item 5;
  whole-mesher-algorithms "two viable coordination policies"; old path boundary
  rounds (`native/mesher/adaptive_component.cpp:509-531`); Parasolid FD
  §107.2.2.1 (inner loops near the outer boundary).
- Done: a stated loop with its trigger set, invalidation set, geometry-derived
  stop rule and refusal record, and the authored families that force each branch.

**G3. Export precision and reader-equivalent validity.** *(S13, S14, S21; X4, X6)*
- Purpose: make the written file, as consumers actually read it, keep the
  guarantees the FP64 mesh has (manifold, oriented, no new self-intersections,
  deviation).
- Decision it blocks: M5 §5.5 export and §10 export oracle: float32 as declared
  quantization with a re-import oracle, or snap-rounding with intersection
  removal; welding policy per format; 3MF items versus components; origin
  handling for large assemblies.
- Knowledge: literature (snap rounding; Lazard–Valque 2025 method; CGAL
  snap-rounding fixtures) plus reference code (lib3mf, PrusaSlicer admesh,
  OCCT RWStl/RWGltf) plus measurement (re-import of our outputs).
- Leads: L5 §2 and §4; `valque-lazard-2025` (held); CGAL
  `PMP_Boolean_operations/test/.../data-snap_rounding`; test-plans-meshgen B6;
  meshing-oracles-study (export displacement).
- Done: per format, the re-import procedure the oracle emulates, the measured
  failure rate on the corpus under plain rounding, and the selected remedy with
  its cost.

**G4. Physical embedding near thin and knife-edge features.** *(S21; X16)*
- Purpose: decide whether the ε-inflated planning query is sound (including
  adjacent faces at small dihedral angles and near-tangent trims) and what the
  construction does when it is not.
- Decision it blocks: M5 §5.6 and whether an in-construction contact rule is
  needed for alpha.
- Knowledge: literature (Khoury–Shewchuk embedding conditions; Li et al. 2026
  JCDE, unread; restricted Delaunay embedding) plus own derivation plus authored
  families (thin walls, slivers, knife edges, near-tangent trims).
- Leads: whole-mesher-geometry-contract §5; fast-tessellation §8 question 2;
  owned-mesher-audit §0 item 4 (0 forbidden pairs in 107 definitions, which is
  one workload, not evidence of soundness); test-plans-cad W6.
- Done: a stated sufficient condition with its proof sketch or a counterexample
  family, and the construction rule for inputs that violate it.

**G5. Entity coverage for arbitrary inputs.** *(S3; X12)*
- Purpose: decide which STEP entities the mesher must accept for "arbitrary
  B-rep input" and how each is represented.
- Decision it blocks: the admission matrix in M5 (voids, sheets and open shells,
  faceted B-reps, surfaces of revolution and extrusion as exact rational
  B-splines, offset surfaces with a computed bound, polyline edges, degenerate
  tori, hyperbola and parabola, tessellated-only parts, surface degree above 3).
- Knowledge: literature and standards (AP242 longform WR rules, Part 42) plus
  measurement (entity frequencies over ABC, NIST, CAx-IF and OCCT corpora from M3).
- Leads: L2 §2 table; `native/step_source/main.cpp:172-210`;
  `native/mesher/source.cpp:479`; tolerant-geometry-healing §1.4.
- Done: an admission matrix with frequency per entity kind, the representation
  for each admitted kind, and the refusal record for the rest.

**G6. Units and parameter-space semantics.** *(S3, S2; X13)*
- Purpose: read every unit and uncertainty correctly, per representation
  context, including the plane-angle unit inside pcurve and analytic parameters.
- Decision it blocks: lifting the mm-only refusal; inch and SI-prefixed inputs
  (common in CAM and printing); IGES and XT unit defaults.
- Knowledge: reference code (OCCT `STEPConstruct_UnitContext`,
  `GeomConvert_Units`) plus standards (AP203 RP H.1.5, H.1.9; IGES 5.3
  §2.2.4.3.14) plus corpus measurement.
- Leads: L2 §1 requirements R2-R3; NIST FTC-10/11 (two contexts); PrePoMax
  double-conversion report; OCCT 0029803.
- Done: a unit and parameter-map table per surface kind, the per-context
  uncertainty record, and the refusal rules.

**G7. Determinism contract and cache keys without bit-identity.** *(S17, S16; X10)*
- Purpose: decide what must be bit-identical (edge-sample layer? per backend
  across thread counts?) so that caching, early cutoff, regression and
  incremental equivalence work after the operator dropped cross-backend
  bit-identity.
- Decision it blocks: M5's determinism statement, cache key composition and the
  metamorphic oracles in §10.
- Knowledge: literature (Mokhov et al. 2018; Blelloch et al. 2012; NVIDIA FP
  white paper) plus measurement (how often CPU and GPU sizing decisions differ
  near thresholds on the corpus).
- Leads: L6 §3; owned-mesher-audit §4 "Determinism"; fast-tessellation §6.3.
- Done: a determinism class per output layer, the cache key definition, and the
  oracle for each class. Depends on D-determinism (§4.2).

**G8. Heterogeneous placement and the measurement protocol.** *(S18; X11)*
- Purpose: choose how work is placed across two sockets and two GPUs, and how
  timings are taken so comparisons with NVIDIA are fair.
- Decision it blocks: M5 §5.5 scheduling and §9.5 measurement; whether the GPU
  is used for meshing work at all at 2.5 ms per face.
- Knowledge: literature (HEFT, StarPU performance models, work-stealing bounds
  under multiprogramming, Hoefler–Belli, Mytkowicz) plus measurement (per-device
  cost models on the corpus; AutoNUMA effect).
- Leads: L7 §1 Stage B and §3; owned-mesher-audit §5 measurement list;
  nvidia-smtess §4 (NVIDIA worker grouping); journal (load and frequency
  recording).
- Done: a placement rule with its cost model and fallback, a stated
  intra-face split threshold derived from T∞, and a measurement protocol
  covering accounting (per definition versus per occurrence), warm/cold, NUMA and
  repetitions.

**G9. Instancing key and per-occurrence semantics.** *(S10; X3, X6)*
- Purpose: define the mesh key, ownership, winding and normal handling, and
  per-occurrence attributes for every placement kind we accept.
- Decision it blocks: M5 §5.1 and §5.5; depends on D-placements.
- Knowledge: literature residue (CAx-IF AP242 assembly-structure RP, unread)
  plus reference code (OCCT RWMesh, XCAFPrs) plus measurement (how the NVIDIA
  baseline counts instances).
- Leads: L7 §2 table; OCCT `BRepMesh_Deflection.cxx:30-62`,
  `RWMesh_FaceIterator.hxx:96-127`; Mantis 0023260 fix f67d051;
  `native/mesher/assembly.cpp:161-171`.
- Done: a placement table with key, ownership and emit rules, and the accounting
  rule for comparisons.

**G10. Small-feature collapse soundness.** *(S6; X14)*
- Purpose: show (or refute) that the β-ball edge collapse and boundary-only face
  rule keep every emitted triangle within ε, preserve dimension, disperse
  symmetrically and record history.
- Decision it blocks: M5 §5.2 item 5 thresholds and checks.
- Knowledge: own derivation plus literature (Sheffer 2000 collapse operator;
  Shephard 1998 validity; Owen 2019 failure rates) plus authored families
  (slivers at η/2, η, 2β; curved and non-convex small faces).
- Leads: L4 §1.1, §3 items 2-3; tolerant-geometry-healing §5.6, §7.2.
- Done: a stated bound or counterexample per rule, and the validity checks.

**G11. Validation records and diagnostics.** *(S4, S15; X7, X8)*
- Purpose: define the native record model for findings (measure before and
  after healing; ISO 10303-59 layers; PDQ codes; threshold provenance; method
  and accuracy; extreme location; action), the status set including
  indeterminate, and roll-up from face to part to run.
- Decision it blocks: M5 §4.6 and §5.5 records; depends on D-partial-output and
  D-profile.
- Knowledge: literature (mostly read in L3) plus reference code (Parasolid fault
  model, SMESH compute error, usd-brep requirement IDs) plus doctrine (native
  internal values, JSON only at the presentation boundary).
- Leads: L3 §1, §2 catalog; JAMA V5.1 §3.1 Part 59 mapping, §7.5 Quality Stamp.
- Done: record schema (native), status lattice and roll-up rule, and the human
  view derived from the same records.

**G12. Invalidation and equivalence under edits.** *(S16, S24)*
- Purpose: define what change description the mesher consumes from the host,
  the invalidation set (changed faces, refined-edge neighbours, halo), reuse
  acceptance (geometry and boundary digests, not deflection), and the
  incremental-versus-fresh equivalence the oracle tests.
- Decision it blocks: the complication roadmap path in M5 §7 and §11.
- Knowledge: literature (Sypkens Smit 2011; Sheffer–Üngör 2001; Parasolid
  incremental faceting; Wu–Sarma 2004, unread) plus reference code (OCCT
  `BRepTools_History`, `BRepMesh_ModelPreProcessor`).
- Leads: L6 Stage A; `native/mesher/adaptive_component.cpp:520-529`.
- Done: an invalidation rule, a reuse rule, an equivalence statement and its
  oracle. Depends on G7.

**G13. Shading fidelity delivered to the consumer.** *(S12, S13; X5, X15)*
- Purpose: tie the cap rule and normal policy to a bound or a visual criterion
  (reflection lines), decide G1 classification when encoded regularity is
  absent, and decide how normals reach consumers whose formats cannot carry them.
- Decision it blocks: M5 §4.7 and the export set (with D-formats).
- Knowledge: literature (Khoury–Shewchuk 2019; Shewchuk 2002; Guthe et al. 2006;
  shadow-terminator papers, unread) plus measurement (reflection-line renders on
  authored families).
- Leads: L5 §1 Stage 2, §4 items 2, 9; fast-tessellation §1.5.
- Done: a cap or angle rule with its stated basis, a G1 rule, and the per-format
  normal policy.

**G14. Sizing beyond the chord rule.** *(S19)*
- Purpose: decide maximum edge length, grading between a fine face and its
  coarse neighbours, and proximity sizing, and whether any are needed when only
  ε is promised.
- Decision it blocks: M5 §6.1; CAE consumer support.
- Knowledge: reference code (Netgen `Analyse`, Gmsh size fields, Parasolid local
  tolerances) plus operator scope (D-consumers).
- Leads: L4 §1.4 item 1; parallel-growth-clusters §4 (grading band).
- Done: a sizing statement per consumer profile.

**G15. Tolerance requests against stated tolerances (PMI).** *(S2, S1; X9)*
- Purpose: decide whether and how the mesher reads semantic PMI, general
  tolerances and the nominal-model flag to warn about or bound ε, and how it
  reports the one-sided bias on features of size and the combined
  source-plus-chord bound.
- Decision it blocks: M5 §4.1 and §9.6; depends on D-PMI.
- Knowledge: literature (CAx-IF PMI RP v4.1; NIST FTC test cases; JCGM 106;
  ISO 14253-1, paywalled) plus own derivation (feature-of-size bias).
- Leads: L1 §1 Stage B, §2, §3.
- Done: a request-checking rule, the reported quantities, and the PMI subset read.

**G16. Source identity passthrough and the classification map.** *(S1, S23)*
- Purpose: carry STEP entity ids, persistent UUIDs, `shape_aspect` groupings
  (one hole across two faces), a geometric link per face, and per-vertex
  classification including collapse history into every output that can hold
  them.
- Decision it blocks: M5 §5.5 output schema and per-format face IDs.
- Knowledge: literature (CAx-IF PID v1.8, cross-domain RP, 3D tessellated RP
  §5.4; Shephard 1998 classification) plus format specs (3MF triangle sets, glTF
  custom attributes, USD GeomSubsets).
- Leads: L1 Stage A; L6 §2; L5 §1 Stage 2.
- Done: the identity fields and their carriage per format.

**G17. Contact interfaces for mechanisms.** *(S22)*
- Purpose: decide whether matched discretization across a declared contact pair
  (no merge) is needed for the physics roadmap, and how it coexists with
  instancing.
- Decision it blocks: whether M5 reserves this, and the contact-density
  constraint in §11; depends on D-imprint.
- Knowledge: literature (contact meshing in CAE; L4 imprint/merge evidence) plus
  operator scope.
- Leads: L4 §1.3; design §11 (gear flanks, pallets, pivots).
- Done: a scoped statement and the interfaces M5 must leave open.

**G18. Post-processing legality and LOD.** *(S11, S12)*
- Purpose: write the legality rules for in-core mesh operations and place LOD
  (re-tessellation at coarser ε) and decimation (downstream only).
- Decision it blocks: M5 statement of what operations exist in the mesher.
- Knowledge: literature (read in L5) plus own rules.
- Leads: L5 §D.
- Done: an allowed/forbidden/downstream table. Low risk; largely settled by L5.

**M2 (in flight, not researched here).** Reconciliation from well-conditioned
neighbours (S5b): brief `docs/fast-mesher-campaign/briefs/M2-neighbour-reconciliation.md`.

### 4.2 Operator decisions

Each blocks one or more research gaps or design sections. Options are the ones
the sources offer.

| ID | Decision | Options from the literature | Blocks |
|---|---|---|---|
| D-partial-output | When a face is refused or indeterminate: emit the rest marked incomplete, or refuse the part; and whether partial output is a default or an opt-in | Parasolid: whole body by default, partial opt-in with a list; journal default: mesh the rest | G11, S15 |
| D-placements | Accept scaled, non-uniform and sheared placements, or refuse them as Parasolid and the AP203/PDM practices do; share mirrored meshes and flip at emit, or mesh separately; tolerance per occurrence (ε/s) or per definition | L7 table | G9, S10 |
| D-units | Refuse unit-less input, or accept IGES's inch default and XT's metre convention; scope of unit conversion | L2 §4 item 4 | G6 |
| D-formats | Alpha export set (STL, 3MF; add glTF with EXT_mesh_manifold and face ids? OBJ, PLY, AP242 tessellated? USD timing); 3MF assembly policy (components that unite, or separate items that may move) | L5 §2, §5 | G3, G13 |
| D-consumers | Which consumers and settings define alpha acceptance (PrusaSlicer, Cura, a mesh-CAM path, TetGen or fTetWild, one renderer, PhysX); whether a CAE profile (needle limits, max length) is in scope | L5 §3 | G14, G18, S14 |
| D-PMI | Read semantic PMI to warn about or bound ε (with which ratio k), or not; what to do when ε is below the source's own uncertainty (warn, refuse, comply) | L1 §4 items 1, 3 | G15 |
| D-master | When `master_representation` is a pcurve and the 3D curve disagrees beyond uncertainty, follow the pcurve (Part 42) or refuse; files with no pcurves: refuse, or recompute with a recorded residual | L2 §4 item 1 | G1, S5 |
| D-shapes | Use only the main shape representation, or also tessellated-only and alternative shapes | L2 §4 item 5 | G5 |
| D-profile | Default data-quality threshold profile (JAMA V5.1, CAx-IF, a named BRepGPU profile); whether user profiles may loosen limits; what a sender validation-property mismatch does | L3 §4 items 1, 3 | G11 |
| D-determinism | Determinism contract: per backend bit-identical across thread counts; cross-backend identity of the edge-sample layer only; or tolerance-only; whether one model may mix backends | L6 §4 items 1-2 | G7, G12 |
| D-imprint | Imprint/merge scope: none; matched discretization across declared contacts without merge; shared-vertex multi-material output; or delegate to a modelling kernel | L4 §4 item 2 | G17 |
| D-features | May output ever drop features larger than ε (draft view), or is "never above ε" a hard contract; is CAE-style size-based defeaturing in scope; compounds | L4 §4 items 1, 4, 8 | G10, G18 |
| D-provisional | May provisional meshes feed physics or contact, or only display; maximum staleness before resynchronization | L6 §4 item 6 | S24, G12 |
| D-measure | Measurement protocol items beyond the governor question already open: AutoNUMA, SMT, warm/cold cache, accounting per definition or per occurrence | L7 §4 items 4-5 | G8 |
| D-docs | IGES 5.3 and JT v10 PDFs now in ignored `papers/pdfs/` carry "copying not allowed without permission" notices: keep for reading only, or remove | L2 §4 item 8 | — |

The two open questions carried by the campaign plan (CPU governor; pass-2 branch
choice, informed by M2) are unchanged.

## 5. For SHOPPING_LIST.md

Consolidated from L1-L7; the dispatcher merges these. Items already on
SHOPPING_LIST.md (ISO 10303-59:2022, SASIG PDQ V3.0 Attachment C, ISO/PAS 26183,
Parasolid PK pages, ACIS healing pages) are not repeated.

**Standards (paywalled).**
- ASME Y14.5-2018, Y14.41-2019; ISO 1101:2017, ISO 8015:2011, ISO 14405-1,
  ISO 2768-1, ISO 22081:2021, ISO 286-1; ISO 14253-1:2017 and ASME B89.7.3.1
  (decision rules) — tolerance semantics (S2).
- ISO 10303-42 current and ISO 10303-21:2016 normative text; ISO 10303-46 (style
  and visibility); ISO 10303-242 Ed.2/3 tessellated entities and accuracy
  parameters; ISO 10303-55/-108/-111 — reading (S3).
- ISO 14306:2017 (JT); ISO/ASTM 52915 (AMF) — export (S13).
- EN 9300-1xx (LOTAR validation properties and archiving) — validation (S4).

**Papers (paywalled or bot-walled).**
- Graham 1969 (LPT bound); Topcuoglu–Hariri–Wu 2002 (HEFT); Kasik et al. 2005
  "Ten CAD Challenges"; Yoon–Gobbetti–Kasik–Manocha 2008 monograph; Isenburg–
  Lindstrom 2005 "Streaming Meshes" (author URL 404); author copies of
  Blumofe–Leiserson 1999, Arora–Blumofe–Plaxton 1998, Mytkowicz 2009 (read from
  course-hosted copies, not stored) — scale (S18).
- Kripac 1995/1997; Marcheix–Pierra 2002; Farjana–Han 2018 (open access, but
  scripted download blocked; fetch in a browser); Wu–Sarma 2001/2004;
  François–Cuillière 2000; Cuillière et al. 2009/2011; Zhu et al. 2012;
  Raghothama–Shapiro 1997/1998 — incremental remeshing (S16).
- Thakur–Banerjee–Gupta 2009; Lee et al. 2005; White–Saigal–Owen 2005;
  Gopalakrishnan–Suresh defeaturing-error papers; Jun–Kim–Park 2002 (polyhedral
  machining, the closest CAM-from-STL source found); HAL copies blocked by bot
  protection (Foucault 2008, Hamri et al. 2005/2010, Danglade–Pernot–Véron 2014)
  — simplification (S6).
- Shakarji–Phillips 2015 full text; Lipman–Lubell 2015 (CAD 66); Srinivasan 2008;
  Heysiattalab–Morse (ASME/ISO differences) — metrology (S2).
- Bi et al. 2023 (parallel STEP parsing, J. CAD&CG) — reading performance (S3).
- Chiang–Li–Burley 2019 and Hanika 2021 (shadow terminator) — shading (S12).

**Too large to fetch under the lane's 30 MB cap.**
- Foucault 2007 PhD thesis, ÉTS, 80.3 MB (open):
  https://espace.etsmtl.ca/id/eprint/242/1/FOUCAULT_Gilles.pdf

**Vendor and member-only documentation.**
- CAx-IF CAESAR round results and statistics (member login); CAx-IF AP242
  assembly-structure recommended practice (scaled and mirrored placements).
- Spatial ACIS faceter, `api_check_entity`/insanity list, history/roll and SAT
  procedural-surface documentation (login).
- Parasolid PK reference for `PK_BODY_check_o_t` returns levels and facet fault
  tokens; Parasolid incremental-faceting example code (licensee).
- CADfix and CADIQ user manuals; PolyWorks, ZEISS INSPECT and Calypso CAD-import
  tessellation documentation; Ansys Mechanical associativity help; Mastercam and
  Fusion surface-triangulation tolerance definitions.
- Siemens JT 10.x current reference (account); CAD Exchanger SDK STEP
  documentation (moved).
- OCCT Mantis tickets 22238, 28025, 30087, 32127, 32141, 33543 (tracker
  unreachable); the Autodesk forum thread on Fusion STEP export quality (HTTP
  403), needed before the "Fusion exports have no pcurves" premise is cited as
  established; L2 found no primary source for it.

**Corrections to read.**
- Petersson–Chand 2001 (held) has a font-encoded body; it needs OCR before its
  claims can be re-checked.

## 6. Acquisition and scrub notes

- Sub-lanes added PDFs to ignored `papers/pdfs/` (listed with bytes and SHA-256
  in the sources TSV); no file over 30 MB was fetched. Where two sub-lanes
  fetched the same file under different names, the duplicate was removed and the
  TSV names the one kept.
- Nothing was built, run or meshed; no repository file other than this report
  and its sources TSV was edited; nothing was committed.
- Privacy scrub: the report and TSV were checked for hostnames, user names,
  absolute workstation paths and private operational detail; paths are
  repository-relative, and the shared reference shelf is named by its
  workspace-relative form (`quarantine_proj/...`) only where a sub-lane cited it.
