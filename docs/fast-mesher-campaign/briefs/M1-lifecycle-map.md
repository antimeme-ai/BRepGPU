# M1 brief: whole-lifecycle map

**Purpose.** Produce the complete picture the campaign has lacked: the whole
lifecycle of B-rep geometry as it becomes a mesh and lives on, mapped onto what
we have, with every gap called out. Research so far arrived one question at a
time and left blind spots that surface only when they bite. The gap list from
this lane decides what M4 studies, so its quality decides what we learn next.

**Serves the goal by.** The design (M5) is rewritten from this map, and the
tribunal (M6) judges the design against it. A stage missing here is a stage the
mesher will get wrong.

**Read first, in this order.**
1. `BLACKBIRD.md` (workspace doctrine; take it literally).
2. `AGENTS.md` (repository rules).
3. `journal/2026-10-17-fast-mesher-direction.md`: every operator decision; later
   entries override earlier ones.
4. `docs/fast-mesher-campaign/PLAN.md`, then `docs/FAST_MESHER_DESIGN.md` (a
   working draft, patched per finding; treat it as a statement of intent and
   decisions, not as a hardened design).
5. Existing research: the seven 2026-10-17 fast-mesher papers
   (owned-mesher-audit, nvidia-smtess-study, fast-tessellation-literature,
   mesher-test-plans-cad, mesher-test-plans-meshgen, parallel-growth-clusters,
   tolerant-geometry-healing). Also the 2026-10-16 whole-mesher studies
   (algorithms, geometry-contract-study, native-runtime, runtime-resolution),
   `papers/2026-10-15-step-shape-processing-lifecycle.md`, the 2026-10-15
   meshing studies, and anything else in `papers/` or the shared literature
   collection that bears on a stage. Build on them; do not redo them.

**Work.**
1. **Define the lifecycle from literature and mature systems, not from our
   design.** For each stage give its purpose, inputs and outputs, the decisions
   made there, who or what makes them, and its known failure modes. At least:
   - authoring intent, and how it is lost in exchange;
   - tolerance and metrology semantics: how GD&T, PMI and stated tolerances
     are read and misread downstream. The operator calls metrology
     interpretation among the densest sources of trouble;
   - file exchange and reading: STEP AP203/214/242, IGES, native formats,
     units, assemblies, PMI;
   - validation and data-quality analysis (SASIG PDQ, VDA 4955,
     ISO 10303-59, CAx-IF);
   - healing, repair and tolerant modelling;
   - simplification: defeaturing, virtual topology, slivers and small features
     (CUBIT/Sandia, Sheffer, the CAD-to-analysis model-preparation literature);
   - topology construction and shared boundaries;
   - edge discretization and face meshing;
   - assembly and instancing;
   - mesh post-processing and quality improvement;
   - normals, attributes and LOD;
   - export formats and their validity rules;
   - downstream consumers: slicers, CAM, CAE, rendering, physics and
     collision;
   - error reporting and diagnostics to people;
   - incremental and associative remeshing after edits;
   - caching, persistence and provenance;
   - performance engineering at scale, including heterogeneous CPU/GPU.

   Use surveys of CAD-to-mesh pipelines (Owen and successors, CAD/CAE
   integration and model preparation) and the documentation of Gmsh, Netgen,
   CUBIT, Parasolid, ACIS and OCCT. Add any stage the literature has that this
   list lacks.
2. **Map every stage to what we have**: the design draft, the papers above, and
   the code (`native/step_source`, `native/reference_source`, `native/mesher`,
   `native/direct_cdt`, the scene/display path, and the Python consumers under
   `python/`). Mark each stage one of: covered with evidence; covered by
   assumption; partially covered; absent. Flag every place where the design
   contradicts the literature.
3. **Gap list, ranked by risk to the operator's goals.** Write each gap as a
   stub brief for one M4 lane:
   - purpose;
   - the decision it blocks;
   - the kind of knowledge that fills it: literature, reference code,
     measurement, or an operator decision;
   - concrete leads (papers, systems, code paths);
   - what done means.

   Each stub must be one conceptual unit. Separate gaps that need an operator
   decision from gaps that need research.
4. M2 is studying one known gap in parallel: deriving the desired final
   position of inexact or malformed geometry from easier, well-conditioned
   neighbours. List it in the map; do not research it.

**Outputs.**
- `papers/2026-10-17-geometry-lifecycle-map.md`, containing the map, the stage
  mapping, the contradictions, and the ranked gap stubs.
- `papers/2026-10-17-geometry-lifecycle-map-sources.tsv`, following the
  existing `*-sources.tsv` conventions, with read depth per entry (full /
  method+results / abstract).
- Openly available PDFs into `papers/pdfs/` (gitignored). The laptop has about
  23 GiB free: fetch only what you will read, and stop and report if a source
  is large.
- Unobtainable items go in a section titled "For SHOPPING_LIST.md".

**Done when.** Every stage above, plus any the literature adds, has a
definition, a mapping and a coverage mark. Every gap has a stub brief. The
report has passed a privacy scrub.

**Must not.**
- Edit any file other than the outputs above.
- Commit.
- Build or run code or geometry.
- Put hostnames, absolute workstation paths or private operational detail in
  the outputs (the repository is public).
- Guess where the code or literature can be read.

**Return.** The ranked gap list, one line per gap with its kind
(research / operator decision), and the 3–5 most decision-relevant findings.
