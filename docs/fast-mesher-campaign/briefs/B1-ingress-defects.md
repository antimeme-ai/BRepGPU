# B1 brief: fix the four ingress and admission defects M1 found by reading

**Purpose.** M1 found four defects by reading code
(`papers/2026-10-17-geometry-lifecycle-map.md` §3.4). The operator fixes every
bug inside the phase that finds it, so these close before M4 does. Beads:
`brepgpu-12s.1` (this maneuver) and its children `.1.1` to `.1.4`.

**Serves the goal by.** The STEP reader in `native/step_source` is the ingress
bridge that the fast mesher keeps (design §3), and it also prepares the input
for the steel-manned NVIDIA arm. A silent geometry change at ingress makes
every later speed and fidelity number false for both arms.

**The defects.**
1. **Voids dropped.** `native/step_source/main.cpp:313,332` walks only
   `raw->Outer()`. `StepShape_BrepWithVoids` subclasses
   `StepShape_ManifoldSolidBrep`, so a hollow solid loses its void shells with
   no record.
2. **Invented parameter map.** Non-SameParameter coedges get a 9-sample
   straight-line map from 3D-curve parameter t to pcurve parameter s
   (`main.cpp:402`). It is not a measured correspondence.
3. **Plane-angle unit unchecked.** `main.cpp:235-236` reads the context's
   plane-angle unit and never checks or applies it.
4. **Two admission rules for placements.** The display
   (`native/om10/validation.hpp:177`) accepts any positive-determinant affine
   instance. The source readers (`native/mesher/source.cpp:651-663`,
   `python/brepgpu/imported_source.py:296-305`) refuse scaled and mirrored
   placements. A scaled instance can be displayed from a mesh made for a
   different placement.

**Read first.**
1. `BLACKBIRD.md` (tenet 4: we never patch, we plan), `AGENTS.md`.
2. `journal/2026-10-17-fast-mesher-direction.md`, including gate G1. These
   operator decisions bind this work:
   - D-units: honour declared units.
   - D-placements: rigid and mirrored placements will share one mesh, and
     uniform scale will get a mesh per scale. That widening is M8 work (bead
     `brepgpu-12s.25.4`), not B1.
   - D-partial-output: the fast mesher will refuse per entity. Today's
     per-file refusal stays as it is in B1.
   - D-master: follow the declared master representation.
3. Lifecycle map §0 finding 1, §2 (the reading stage and the S3/S5 rows),
   §3.3 and §3.4.
4. `papers/2026-10-15-step-shape-processing-lifecycle.md`.
5. The code around each site, every reader of the tables it writes (`COEDGES_`,
   `LOOPS___` and the solid and shell tables), and the existing tests for
   `native/step_source`, `native/mesher/source.cpp`, `native/om10` and
   `python/brepgpu/imported_source.py`.
6. The OCCT 8.0.1 source in `quarantine/`:
   - `StepShape_BrepWithVoids`;
   - `StepToTopoDS_Builder` (how voids are translated);
   - `STEPConstruct_UnitContext` and `StepData_Factors`, to find which angle
     quantities OCCT converts by itself;
   - `BRepLib::SameParameter` and `ShapeFix_Edge::FixSameParameter`, as
     references for measured reparameterization.

**Work, per defect, in order.**
1. **Study.** Read the site, its consumers and the reference code. State the
   actual failure: which input triggers it, what comes out, and who consumes
   it.
2. **Plan.** Write the fix plan into the report (below) before any code. Name:
   - the mechanism and its grounding (a source file and function, or a paper
     section);
   - the oracle;
   - the authored fixture.

   Expected shape of each fix, unless the study shows otherwise:
   - **Voids.** Carry every void shell, with its orientation, through to the
     tables, the readers and the volume. If a downstream consumer cannot take
     more than one shell, it refuses explicitly with a record. Never drop a
     shell silently.
   - **Parameter map.** Replace the invented map with a measured
     correspondence that has a stated residual bound in mm, grounded in a
     reference method. If no sound map can be built within the bound, refuse
     the coedge explicitly with its residual.
   - **Plane angle.** Establish which angle-bearing quantities OCCT converts and
     which raw values our code reads itself. Apply the declared unit wherever
     we read raw angles. Refuse any angle unit we cannot interpret.
   - **Placements.** Write one admission predicate with an explicit tolerance:
     rigid only, as today's readers enforce. The display, the C++ reader and
     the Python reader all call it or mirror it exactly, and a test pins that
     they agree.
3. **Red test.** Add the fixture and a test that fails on the current code.
   Fixtures are small authored STEP files committed under the repository's
   test-data convention; never use corpus or OM10 bytes. Examples:
   - a block with an internal void;
   - a degree-context file with a cone and trimmed circles;
   - a non-SameParameter coedge;
   - a two-occurrence assembly with one scaled placement.
4. **Fix.** Make the change at the site. Keep the surrounding code's idiom.
5. **Green.** Run the new tests and the existing affected suites.
6. **Review.** Review the diff adversarially against the plan and the
   reference before reporting.

**Where code runs.** Edit in the laptop checkout. Build and test only on the
fleet host:
- Use the fleet SSH alias the dispatcher gives you, with `-o BatchMode=yes`.
- Create a lane directory under the fleet BRepGPU root (`lanes/B1/`), outside
  every existing directory there, and copy tracked files into it with
  `git ls-files` and `rsync --files-from`.
- Use the pinned OCCT 8.0.1 and CUDA toolchains the repository's build already
  points at. Do not install or build new toolchains.

**Outputs.**
- Code and test changes in the laptop checkout. Do not commit.
- `results/b1-ingress-defects-2026-10-17/REPORT.md`, containing:
  - per defect, the study, plan, red evidence, fix, green evidence and review
    notes;
  - the commands that ran;
  - suites and counts.

  Use fleet-relative paths only.

**Done when.**
- All four defects have a passing test that failed before the fix.
- The affected existing suites pass.
- The report is complete and has passed a privacy scrub.

Any defect whose correct fix needs an operator decision stops and comes back,
with the evidence.

**Must not.**
- Run anything on the laptop except editing and `git` reads.
- Widen admission beyond today's rigid-only rule; that is M8.
- Touch corpus data, OM10 bytes or the M4 lanes' outputs.
- Commit or push.
- Put hostnames, addresses or absolute workstation paths in tracked files.
- Weaken an existing check to make a test pass.

**Return.** Per defect, a few lines: the cause, the fix, the test, and the
before and after behaviour. Then any further defects found, each with a file and
line, for new beads.
