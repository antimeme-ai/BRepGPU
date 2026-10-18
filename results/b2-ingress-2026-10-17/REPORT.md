# B2-ingress: units, vertex loops, colours, diagnostics, label

Unit B2-ingress of maneuver B2
([brief](../../docs/fast-mesher-campaign/briefs/B2-lane-defects.md)). It fixes
the ingress defects that M4 lanes G1, G5, G6, G9, G11 and G15 found, under beads
`brepgpu-12s.37.1`, `.37.2`, `.37.4` to `.37.10` and `.37.18`.

**How it ran.**
- Built and tested on the fleet only, against the pinned OCCT 8.0.1 and CUDA
  12.8.1.
- The corpus is lane M4-G6's 500 files, read-only.
- The OM10 source was read in place for two review checks; counts only appear
  here.
- The dispatcher consolidated the lane's returns into this report.

**Review.** An independent adversarial reviewer read the change three times:

| Pass | Outcome |
|---|---|
| 1 | Not ready: 14 findings and nits |
| 2 | No blocker; 4 should-fixes and a reasoning gap |
| 3 | 2 must-fixes, 2 should-fixes and nits |

Every finding is fixed, except one moved to B2-reader (`.37.27`).

## Format: BGSRC001 version 2

| Table | Change |
|---|---|
| `LOOPS___` | Rows end with a vertex id (vertex loops), then a `u32` status (0 admitted, 1 refused). |
| `FACES___` | Rows end with a `u32` status (0 admitted, 1 refused). |
| `SOLIDS__` | Rows end with a `u32` status (0 complete, 1 incomplete). |
| `DIAGS___` | Rows end with typed (name, kind, value) fields. |
| `OCCSTYLE` | New 14th table: per-occurrence styles. |

Both readers changed together, and both refuse any other version by name. The
adapter is now v7 (label token `face-status`), and the reference exporter v4.

**Design constraint for M8.** A face is refused only through a refused loop.
Both readers enforce three equivalences:
- a face is refused ⇔ one of its loops is refused;
- a definition is incomplete ⇔ one of its faces is refused;
- every refused loop has a `source_face_refused` record.

A face-level refusal that is not a loop's would need a format revision.

## 1. Units and uncertainty (`.37.8`, `.37.9`, `.37.10`, `.37.4`)

**Study.**
- The unit gate read OCCT's root-only `FileUnits` names. As a result:
  - six millimetre files named `'MILLIMETRE'` were refused;
  - a child part in another unit passed.
- Vertices were scaled by the root factor alone.
- The XDE transfer overwrote `SetSystemLengthUnit`, so millimetre output relied on
  a global default.
- The uncertainty was a file-wide maximum that ignored units and contexts. The
  reference exporter wrote it unconverted.

**Fix.** `native/step_units.hpp`, shared by both exporters, follows G6's rules
UR-1, UR-2, UR-4, UR-6 and UR-7.
- **Units.** Each context's units are resolved from the unit entities:
  - SI prefixes;
  - conversion-based chains, to depth 8, with cycles refused by name;
  - plain SI-metre targets.
- **Unit names.** A name that disagrees with its factor gives a
  `source_unit_name` finding.
- **Gate.** Admission stays mm-only. A refusal now names the resolved factor,
  never a name.
- **Vertices** use their own context's factor.
- **Transfer.** The document unit is set to mm before transfer and checked after.
- **Uncertainty** is read per context, each measure in its own unit.
  - Named measures govern: closure, distance_accuracy_value (also spelled with
    spaces), point coincidence tolerance.
  - "Largest of several unnamed measures" is our own rule, and is labelled as
    such.
  - Each solid gets a `source_uncertainty` record (`governing_mm`,
    `diagonal_mm`, `usable`).
  - Per UR-7, a value below 1e-15 of the diagonal, including 0, makes no usable
    claim.
  - The header maximum takes only usable values, and both readers say no
    consumer may read it as a part's value.

**Tests** (`UnitAndUncertaintyTests`).

| Case | Before | After |
|---|---|---|
| CBU `'MILLIMETRE'` | refused by name | equals its SI twin |
| Child part in inches | accepted | refused at 25.4 |
| 25.4 mm unit named `'millimetre'` | accepted | refused |
| Global unit M / INCH | 0.01 / 0.3937 | 10 |
| Presentation / metre / assembly-root contexts | 1.0 / 1e-5 / 0.5 | 1e-7 / 0.01 / 2e-3 |
| `'INCH'` that resolves to 1 mm | silent | finding |
| Spaced uncertainty name | governed by 0.5 | 0.01 |
| Declared 0 / 1e-300 | header 0 or 1e-300 | `usable` = 0; header excludes it |
| Version-1 file | — | refused |

`.37.9` stays latent behind the mm-only gate. Lifting that gate is M8 work
(`brepgpu-12s.25.6`).

## 2. Vertex loops and structural refusal (`.37.7`)

**Study.**
- The NIST drill-point cones bound a face by a circle plus a vertex loop at the
  apex. AP242 WR6 and WR7 make this legal; the reader refused the whole file.
- OCCT's rule (`StepToTopoDS_TranslateFace.cxx:547-662`) works on the surface it
  built.

**Fix.** `native/step_vertex_loops.hpp` is shared by both exporters. A loop is
carried from the raw entity and admitted only if all five checks pass, in order:

1. OCCT's result matches its rule on the built surface: natural bounds, dropped
   on a plane or torus, or one degenerated edge.
2. The solid has a usable uncertainty. Otherwise the loop is `not_evaluable`;
   there is no defaulted tolerance.
3. A sole bound lies on a closed surface (ISO 10303-42):
   - for a B-spline, this is proven on the control net;
   - for a revolution, either its generatrix is untrimmed periodic, or its ends
     meet or lie on the axis;
   - a trimmed surface is bounded, so it does not count.
4. The pole lies on the surface within the uncertainty.
5. The pole is a singular point, except under natural bounds.

Each refusal carries a reason code:

| Code | Reason |
|---|---|
| 1 | dropped_by_occt |
| 2 | translation_differs |
| 3 | not_evaluable |
| 4 | sole_bound_not_closed |
| 5 | pole_off_surface |
| 6 | pole_not_singular |

A refused loop sets the loop, face and solid statuses, and writes one record per
loop. A wire that no loop explains refuses its face, not the file.

**Consumers.**
- All 12 native face-loop walks and Python `mesh_imported` check the face status
  first.
- `qualify_physical_definition` raises `IncompleteSourceDefinition`.
- The reference exporter applies the same rule. It writes `records.tsv` (stage,
  severity, ids, message) for the vertex-loop faces it exports that the adapter
  refuses, and for its resolver notes.

**Tests** (`VertexLoopTests`, new fixture `tests/fixtures/apex-cone.step` and
variants).

| Case | Before | After |
|---|---|---|
| Apex cone | file refused ("non-edge UV source loop") | carried; both readers agree; meshers refuse face 30 by name |
| Plane vertex loop | file refused | face 17 and loop 43 refused, solid 15 incomplete, in both readers |
| Tampered statuses (4 cases) | — | refused by both readers |
| Uncertainty absent or 0 | admitted (`Confusion` fallback) | refused, reason 3 |
| Pole off the surface / not singular | admitted | refused, reason 5 / 6 |
| Trimmed plane / trimmed sphere | file refused ("unowned native wire") | face refused, reason 6 / 4 |
| Open B-spline, sole loop | admitted | refused, reason 4 |
| Closed B-spline (guard) | — | admitted |
| Trimmed-arc revolution | refused (OCCT 8.0.1 semantics, so not red) | reason 4 by the rule; whole-circle revolution admitted |
| Two refused loops on one face | one record | two records |
| `diagnose_imported_face.py` views | refused by the readers | valid sources |

OCCT's own volume for its seam-less cone face is 174.53, against the true 261.80.

## 3. Colours (`.37.5`, `.37.6`)

**Study.**
- Only OCCT's non-root style list was read, so these never appeared:
  - root-list styles on shape representations;
  - per-occurrence CDORSIs, whose style context OCCT drops in its typed field.
- The usage style column could never be set.
- A second style on one item silently overwrote the first.
- Consumers used innermost-wins precedence.

**Fix: reading** (`native/step_styles.hpp`, both exporters).
- Every styled item is read. Mapped items are resolved for both plain styles and
  CDORSIs.
- Colourless styles (null, invisibility) are recorded as unsupported.
- Over-ride chains are followed transitively and grouped, so the result does not
  depend on file order. A cycle stops the walk.
- Nested occurrence styles honour over-ride links. Unrelated disagreements are
  recorded as conflicts, under our "deeper wins" rule.
- Occurrence styles are written to `OCCSTYLE`, keyed by usage path.
- A mapped item styles one occurrence only when exactly one usage is placed by a
  plain relationship between its holder and the mapped representation, no twin
  exists, and the parent has one path. Otherwise the style collapses, with a
  record.
- Path counting is one Kahn pass that saturates at 2, with its work charged.

**Fix: precedence** (`occurrence_style()`, `adaptive_scene.cpp`).
- Only coloured styles take precedence.
- Order, highest first:
  1. an explicit occurrence style (deepest wins);
  2. the outermost coloured node style;
  3. the part style, with its faces keeping their own colours (G9's face-over-solid
     ruling, recorded per face).
- Colour variants are keyed on resolved per-face materials. Variant k > 0 marks
  the top byte of its display definition ID; more than 255 variants refuses.
- The legacy `scene_view.cpp` refuses a variant-needing occurrence by name.

**Tests.**
- `ColourTests` cover:
  - root-list styles;
  - occurrence rows, in either context order;
  - mapped-item CDORSIs;
  - out-of-order chains;
  - transitive and nested overrides;
  - the mapped-item collapse and plain-relationship cases, including the deep
    second path;
  - reference and owned styles being equal.
- Scene families (`styled`, `styled_mixed`, `styled_plain`, `styled_material`) are
  checked against an independent Python oracle that ranks candidates by cited
  rule. That oracle found a colourless face style beating the part style, now
  fixed.
- `tests/step_styles_fixture.cpp` runs the resolver without a transfer, because
  OCCT crashes on one of the structures (`.37.27`).

## 4. Diagnostics and label (`.37.1`, `.37.2`, `.37.18`)

- **Typed fields.** `DIAGS___` rows carry typed fields, in G11 §4.1's kind order
  plus code and parameter. No magnitude remains in message text.
- **BRepCheck.** One summary per solid (subjects, checked, failed, not evaluated)
  and one finding per subject. Outcome 1 is fail. Outcome 2 is not evaluated: a
  `CheckFail`, a missing result, or a raised exception, which is now counted
  rather than swallowed. The certification wording is gone.
- **Label.** `shape-processing=translation-only`, naming G1's translation-layer
  edits, in both exporters.

## 5. NVIDIA harness consequence

An incomplete source definition reached `qualify_physical_definition`, and the
harness wrote it as NVIDIA's `FAIL`.

The harness now checks for incomplete definitions before NVIDIA's ingress. If it
finds one:
- every `physical.tsv` row is written `NOT_EVALUATED`;
- the run exits 4 (`ingress_refused`, ours);
- NVIDIA receives no input.

**Test:** an authored plane-vertex-loop fixture through the adapter, the reference
exporter, the minimal ingress and the driver. Before the fix it exited 3; after,
it exits 4 with `physical.tsv` row (15, NOT_EVALUATED).

## Suites (fleet, binaries built from this tree)

| Suite | Result |
|---|---|
| `test_step_ingress` | 72/72 |
| `test_source_uv`, `imported_source`, `imported_assembly`, `om10_scene`, `imported_geometry` | 8, 5, 5, 2, 6 OK |
| `test_imported_meshing`, `imported_domain_sweep` | 35, 8 OK |
| `test_reference_source`, `test_placement_oracles` | 4, 18 OK |
| `test_om10_nvidia` | 11/11 |
| `test_measurement_lane` | 38/38 |
| `test_nvidia_prepared_pipeline` | not run: needs Python `pxr`, absent on the fleet |
| Native mesher ctest | 130/132 |
| om10 ctest | 5/5 |

The two mesher failures are the retired old mesher's cylinder-seam pair
(`.37.11`).

## OM10 (read-only; counts only)

- The adapter exits 0: 107 definitions, 9,162 faces, 196 usages.
- **Styles:**
  - 9 `OCCSTYLE` rows;
  - 0 unplaced, 1 unsupported (the null style on its mapped item), 4 precedence
    records, 0 conflicts;
  - all 9 CDORSIs target a component shape representation.
- **Checks:** 107 BRepCheck summaries, 2,680 findings.
- **Vertex loops:** none; 0 refused faces, 0 incomplete definitions.
- **Legacy scene path:** one OM10 occurrence has an occurrence style that beats
  its definition's face styles, so the legacy `scene_view.cpp` path refuses it by
  name.
  - Dispatcher decision: keep refusing on that retired path rather than render a
    wrong colour.
  - The kept adaptive display path needs no extra geometry copy for OM10.

## Corpus, 500 files

**Totals.**

| | Accepted | Refused |
|---|---|---|
| Before this unit | 230 | 270 |
| After | 232 | 268 |

- No file moved from accepted to refused.
- The two files newly accepted are `nist_ctc_03_asme1_ap242-e2.stp` and
  `nist_stc_08_asme1_ap242-e3.stp`. Each has a single millimetre context named
  `'MILLIMETRE'`.
- Per-face refusals across the 232 accepted files: 0 for every reason.
- The C++ reader reads all 232.
- The 9 NIST vertex-loop files still refuse earlier, at "unsupported nonplanar
  multiple-wire conversion". Their reference records admit every drill-point apex.

**Refusals by reason.**

| Reason | Files |
|---|---|
| non-mm unit factor | 155 |
| C/S residual | 45 |
| nonplanar multiple-wire | 39 |
| leaf-product binding | 14 |
| closed/periodic spline chart | 12 |
| conic chart grade | 2 |
| unowned solid | 1 |

## Further defects, filed

| Bead | Defect |
|---|---|
| `.37.27` | OCCT 8.0.1 recurses without bound (SIGSEGV) on a plain SRR between a mapped item's holder and its mapped representation. Needs a pre-transfer named refusal. |
| `.37.28` | Closed B-spline carriers made periodic by OCCT are refused by both readers. |
| `.37.29` | A carrier's UV hint comes from its first face only. |
| `.37.22` | The reference exporter still exports only the outer shell. Its records walk the void shells. |
| `.37.23` | The reference format has no occurrence styles. |
| `.37.26` | Extra mapped items are dropped silently. |
| `brepgpu-12s.25.10` (M8) | No carrier exists for surfaces of revolution or linear extrusion. |

Recorded but not filed:
- **OCCT upstream:** `StepVisual_StyleContextSelect` has no relationship case.
- **AP242 WR2** excludes `rectangular_trimmed_surface` from advanced faces. OCCT
  and the adapter accept it, now handled by OCCT's rule.
