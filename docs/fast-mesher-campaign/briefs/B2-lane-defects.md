# B2 brief: fix the defects the M4 lanes found

**Purpose.** The M4 research lanes and B1 reported defects in our code while
reading and measuring it. The operator's policy is to fix every bug inside the
phase that found it, so B2 closes before M4 does. Bead: `brepgpu-12s.37`. Its
children are the defects.

**Serves the goal by.** Most of the defects sit in the ingress path the fast
mesher keeps, or in the benchmark harness that produces the NVIDIA steel-man
numbers. A wrong reading of units, colours or face bounds corrupts both arms of
every comparison. A harness that does not run NVIDIA at its strongest corrupts
the comparison itself.

**Units of work.** Each unit is one conceptual unit, run by one lane. Units that
edit the same file run one after another.

| Unit | Beads | Files | Runs |
|---|---|---|---|
| B2-ingress | `.37.4`, `.37.7`, `.37.8`, `.37.9`, `.37.10`, `.37.5`, `.37.6`, `.37.1`, `.37.2` | `native/step_source`, `native/reference_source`, the colour consumers in `native/mesher` | after lane M4-G1 returns, since G1 may add translation-layer findings in the same code |
| B2-display | `.37.3` | `native/om10/cuda.cu` and its tests | now |
| B2-harness | `.37.17` | `examples/render_om10_nvidia.py` and the owned benchmark drivers | now |
| B2-oracles | `.37.12`, `.37.13` | `python/brepgpu/imported_oracles.py`, `imported_assembly.py`, `native/om10/validation.hpp`, `native/om10_reference/raw.hpp` | now |

Dispositions that need no lane:
- **`.37.11`:** the cylinder-seam ctest pair fails on unmodified code. It tests
  the retired old mesher. The dispatcher confirms that nothing in the kept path
  depends on it, then closes it as retired and records its fixture for M8.
- **`.37.14`:** meshing a void-bearing source end to end needs a mesher that
  meshes the block at all. It moves to M8.

**Read first, every unit.**
1. `BLACKBIRD.md` (tenet 4: we never patch, we plan), then `AGENTS.md`.
2. `journal/2026-10-17-fast-mesher-direction.md`, including gate G1 and the
   operator decisions (D-*).
3. Each bead's description (`bd show <id>`), and the paper that found the defect:
   - G6 `units-parameter-semantics` for `.37.8`, `.37.9`, `.37.10`, `.37.4`;
   - G15 `pmi-tolerance-requests` for `.37.4`;
   - G5 `entity-admission-matrix` for `.37.7`;
   - G9 `instancing-semantics` for `.37.5`, `.37.6`;
   - G11 `validation-records` for `.37.1`, `.37.2`;
   - G7 `determinism-cache-keys` for `.37.3`;
   - G8 `heterogeneous-placement-measurement` for `.37.17`;
   - `results/b1-ingress-defects-2026-10-17/REPORT.md` for `.37.12`, `.37.13`.
4. B1's report and its tests (`tests/test_step_ingress.py`), which B2-ingress
   extends.

**Work, per defect, in order.**
1. **Study** the site and its consumers, and state the actual failure.
2. **Plan** in the report before any code: the mechanism and its grounding, the
   oracle, and the fixture.
3. **Red test.** Fixtures are small authored files; never use corpus or OM10
   bytes.
4. **Fix,** in the surrounding code's idiom.
5. **Green:** the new tests and the affected suites.
6. **Self-review** against the plan.

The dispatcher then runs an independent adversarial review. Its findings are
fixed before anything lands.

**Unit-specific scope.**
- **B2-ingress.**
  - **Units and uncertainty.** Resolve them per transferred representation
    context from unit entities, not names, following G6's UR-1, UR-2, UR-4, UR-6
    and UR-7. Vertices use their own context's factor. Uncertainty is read per
    context in its own unit, and the B-rep context's value governs.
    - Keep today's mm-only admission. Lifting it is M8 work.
    - After this unit, a file is refused as non-mm because its resolved length
      factor is not 1, never because of a name.
    - Fix `SetSystemLengthUnit` so that the output is mm by construction, not by
      a global default.
  - **`vertex_loop` bounds.** Admit them as AP242 allows (WR6, WR7): carry the
    pole vertex through the tables and readers. A consumer that cannot take one
    refuses that face explicitly, not the whole file.
  - **Colours.** Read per-occurrence styles, including root-list overrides that
    target shape representations. Apply CAx-IF precedence: the outermost style
    wins, and an explicit override wins in its context. Two styles on one item is
    a recorded conflict, never a silent overwrite.
  - **Diagnostics.** Store magnitudes as numeric fields, not inside text.
    Separate "could not check" from "invalid" in the BRepCheck record, name the
    faulted entity, and remove certification wording. Use G11's record model
    where it is cheap. Do not build the whole model; that is M8.
  - **Corpus check.** Re-run the read-only corpus ingress check that B1 used
    (500 files), before and after. Report every change in the accepted and
    refused sets, by reason.
- **B2-display.** Make CUDA slot assignment deterministic, for example by
  count, scan and fill in canonical order, as G7 requires for D1 layers. Show
  that the output was or was not run-to-run visible before the fix, and that it
  is byte-identical across runs and across both GPUs after.
- **B2-harness.**
  - **NVIDIA workers:** follow G8's protocol §7. Assign definitions to NVIDIA
    workers largest-first by NVIDIA's own measured per-definition times. Pin
    single-threaded workers to physical cores of one socket, with `numactl`
    memory binding. Report the fastest valid arm of 12 or 24 workers.
  - **Owned runs:** pinned the same way.
  - **Context rows:** every timing emits its lane and the competing load (load
    average, other processes' CPU, delivered clock, SMT and AutoNUMA state).
  - **Scope:** do not re-run the published benchmarks. Mark every published
    figure that came from the old harness as such, in its report, with one added
    line.
  - **Smoke test:** a short run on the fleet that checks only that the pinning
    and context rows work. It is not a benchmark.
- **B2-oracles.**
  - Make the independent oracle check `CARTESIAN_TRANSFORMATION_OPERATOR_3D`
    placements.
  - For each stricter refusal-only placement rule, either fold it into
    `brepgpu::rigid_placement_status` or write down why it must stay stricter.
    Folding loosens the rule, so the justification must come from the geometry
    contract, not convenience.

**Where code runs.** Edit in the laptop checkout. Build and test only on the
fleet host:
- Use the alias the dispatcher gives you, with `-o BatchMode=yes`.
- Each unit has its own lane directory under the fleet BRepGPU root:
  `lanes/B2-<unit>/`.
- Copy tracked files with `git ls-files` and `rsync --files-from`, plus any new
  untracked files, listed by name.
- Use the pinned OCCT 8.0.1 and CUDA toolchains. Install nothing.

**Outputs.**
- Code and test changes in the laptop checkout. Do not commit.
- Return the report content as text. The dispatcher writes it to
  `results/b2-<unit>-2026-10-17/REPORT.md`. For each defect, the report gives
  the study, plan, red and green evidence and review notes, plus the commands
  and suite counts. Paths are fleet-relative.

**Done when.**
- Every bead in the unit has a test that failed before the fix and passes after.
- The affected suites pass.
- The report is complete and scrubbed.

A defect whose correct fix needs an operator decision stops and comes back with
the evidence.

**Must not.**
- Run anything on the laptop except editing and `git` reads.
- Widen admission (lifting mm-only, mirrors, scale): that is M8.
- Weaken any existing check.
- Touch OM10 bytes, modify corpus data, or touch other lanes' files.
- Commit, push or modify beads.
- Put hostnames, addresses or absolute workstation paths in tracked files.

**Return.** For each defect, a few lines: the cause, the fix, the test, and
before and after. Then the suite counts, and any further defects found, each
with file and line.

## Follow-on unit B2-reader (added 2026-10-18)

The first four units found further defects in `native/step_source` and
`native/reference_source`. That code is also where B2-ingress worked, so this unit
runs after B2-ingress lands. The same rules apply: study, plan, red, fix, green,
self-review, and then an independent review.

| Bead | Defect | Expected fix |
|---|---|---|
| `.37.20` | The reader composes item-defined placements as item_1⁻¹∘item_2 (OCCT `STEPControl_ActorRead.cxx:2488`). The standard and PDM Usage Guide r4.3 Example 14 require item_2∘item_1⁻¹. It also admits misread operator mirrors and an omitted `axis3` as rigid. | Read placements raw in the adapter, following the oracle's Part 42 transcription (`python/brepgpu/imported_oracles.py`). Refuse mirrors and scale while admission is rigid-only. |
| `.37.21` | The oracle's `representation_links` joins placing relations, so it merges parent and child. | Apply the skip that `_representation_products` uses. |
| `.37.19` | No exporter performs NVIDIA's strongest ingress, OCCT's default STEP read. | A `shape-processing=default` mode in `native/reference_source`, with its face map and admission built under FixShape. |
| `.37.22` | The reference exporter refuses `BREP_WITH_VOIDS` sources. | Carry voids as B1 did. |
| `.37.23` | The reference exporter has no occurrence styles, and its vertex-loop faces fail BRepCheck. | Write occurrence styles. Handle vertex loops. |
| `.37.25` | The reference XDE exporter refuses solids placed through `MAPPED_ITEM`. | Admit them. |
| `.37.26` | An extra `MAPPED_ITEM` in a parent representation is translated by OCCT as geometry that never reaches `USAGES__`, so the occurrence is lost silently. | Read mapped items raw; publish each as an occurrence, or refuse it with a record. |

The correctness constraint is G1's authority rule: the raw entity is
authoritative, and OCCT serves only as an evaluator.

The steel-man constraint is that each NVIDIA ingress arm must stay what it claims
to be. B2-harness's byte check applies only to a `translation-only` exporter with
no `raw-entity-corrections=`. Any raw correction this unit adds to the exporter
must be declared through that token.

Re-run the read-only 500-file corpus ingress check before and after, and report
every change in the accepted and refused sets, by reason.

### B2-reader split (2026-10-18)

B2-ingress landed and added beads `.37.27`–`.37.29`. B2-reader runs as two
parallel lanes, divided by file ownership:

- **B2-reader-adapter.**
  - Beads: `.37.20` (raw placements), `.37.26` (extra mapped items), `.37.27`
    (OCCT recursion: refuse by name before transfer), `.37.28` (periodic B-spline
    carriers), `.37.29` (carrier UV hint over every face).
  - Owns `native/step_source/`, `native/step_units.hpp`, `native/step_styles.hpp`,
    `native/step_vertex_loops.hpp` and the source readers in `native/mesher/` and
    `python/brepgpu/imported_source.py`.
- **B2-reader-export.**
  - Beads: `.37.19` (default-read exporter), `.37.22` (voids), `.37.23` (occurrence
    styles in the reference format), `.37.25` (mapped-item solids), `.37.21`
    (oracle `representation_links`).
  - Owns `native/reference_source/`, the reference-format readers in Python, and
    `python/brepgpu/imported_oracles.py`.
  - Reads the shared headers but does not edit them. A needed header change goes
    to the dispatcher, who passes it to B2-reader-adapter.

### B2-reader round 2 (2026-10-18)

Both B2-reader lanes returned with their beads fixed. Between them they found
defects that belong to B2 under the bug policy. They also raised four
coordination questions, which the dispatcher settled as follows.

**Dispatcher decisions.**
- **Placements in the reference exporter.**
  - Usages placed by `NEXT_ASSEMBLY_USAGE_OCCURRENCE` and mapped items both read
    raw placements through `native/step_placements.hpp`, the header the adapter
    landed. Under G1's authority rule the raw value is authoritative.
  - Where OCCT agrees within the adapter's tolerance, the exporter writes OCCT's
    value and stays `raw-entity-corrections=none`.
  - Where OCCT disagrees, the exporter writes the raw value, adds a record, and
    declares `raw-entity-corrections=P1` or `P2` (and so on), using the adapter's
    letters.
  - The NVIDIA arm then stays correct, and it says what was corrected. The
    translation-only byte check already applies only when the arm declares no
    corrections.
  - For mapped items, disagreement with OCCT is therefore no longer a refusal.
- **Mapped-item solids.**
  - The owned adapter keeps refusing them by name (`.37.26`). Admitting them is
    M8 work (`.25.11`).
  - When it is admitted, the occurrence uses the exporter's graph shape: the
    product is keyed by the mapped representation's label, the usage by the
    mapped item's label, and the names come from the holder product.
- **The conflicting test.**
  - With `.37.20`, the adapter refuses `IDENTITY_RELATION` by name.
  - The oracle test for `.37.21` therefore asserts two things on that file: the
    adapter's named refusal, and the oracle's own named placement refusal,
    reached without an adapter source. It must never report "correspondence
    differs".
- **Old version-1 reference sources** refuse by name and must be re-exported.
  That is correct behaviour, not a defect.

**Round-2 beads.**

| Lane | Bead | Work |
|---|---|---|
| adapter | `.37.30` | Closed B-spline edge curves. Already fixed in round 1; this records it. |
| adapter | `.37.31` | Carrier hints clipped to the natural domain while the projected pcurves leave it (`native/source_uv`). |
| adapter | `.37.32` | The depth-8 bound in `transfer_path` lets a deep holder escape the unit gate and the `.37.26` check. |
| export | `.37.33` | The exporter's usages take OCCT's located transform. Read them raw, as decided above. |
| export | `.37.34` | The oracle refuses non-identity representation maps, so mapped-item solids cannot be oracle-checked (`imported_oracles.py:753`). |
| export | `.37.35` | Orientation flags under the default read: for each face whose flag differs, decide whether the material side changed. Refuse the face if it did; otherwise record it as an exact flag-and-surface reversal. |

**Integration.** Both lanes edit one checkout, and the two lanes' hunks in
`tests/test_step_ingress.py` and `tests/test_placement_oracles.py` do not
overlap. After round 2, the export lane builds the union of both lanes' files
in a fresh lane directory, `lanes/B2-reader-merged/`. There it runs every suite,
the 500-file corpus through both the adapter and the exporter, and OM10. The
independent reviews then read that merged state.

### B2-reader round 2 decision: orientation (2026-10-18)

**What `.37.35`'s verdicts showed.**
- Of the 23 faces where OCCT's default read changes the orientation flag, none
  is a reversal of flag and surface together.
- On 22 of them (OM10's 6, and 16 in the corpus), the face's raw `same_sense`
  normal points into the material. This was tested by ray parity, which uses
  geometry only. On those faces FixShape repaired the file.
- On the remaining one, the raw use agrees with the shell. There FixShape turned
  a correct face.

**Decision, for the default-read arm.**
- A turned face whose raw normal points into the material is admitted with a
  record. That is the strongest default read, and it is right.
- A turned face whose raw use agrees with the shell refuses the definition as
  ingress, by name. NVIDIA's arm would carry an inward face there, and an
  invalid NVIDIA arm is never our win.
- An undecidable face still refuses.

**Owned side (`.37.36`).** The adapter publishes the raw use for those 22 faces,
so owned meshes would carry inward faces. The adapter lane first finds the
mechanism, then fixes it:
- either resolve the face from the shell's edge uses, with a record and a
  declared correction;
- or refuse the face by name.

The merged run waits for `.37.36`.
