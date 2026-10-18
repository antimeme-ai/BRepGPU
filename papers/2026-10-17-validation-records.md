# Validation records and diagnostics: status lattice, native record schema, BRepGPU profile, human view

Research lane M4-G11 of the fast-mesher campaign, 2026-10-17, branch
`research/fast-mesher-study`. Brief:
[docs/fast-mesher-campaign/briefs/M4-G11.md](../docs/fast-mesher-campaign/briefs/M4-G11.md)
with [M4-common.md](../docs/fast-mesher-campaign/briefs/M4-common.md). Stub:
[lifecycle map §4.1 G11](2026-10-17-geometry-lifecycle-map.md) (stages S4, S15;
contradictions X7, X8). Bead `brepgpu-12s.12`.

Nothing was built, run, meshed or measured. The lane read the doctrine, the
operator-decision journal, the campaign plan, the lifecycle map, the working-draft
design, the healing, neighbour-reconciliation, test-plan and native-runtime
papers, the standards and vendor documents held in ignored `papers/pdfs/`, the
ISO 10303-59 data model as merged into the AP242 AIM (STEP Tools entity pages),
reference code in `quarantine/` at pinned revisions, and our own code. Sources and
read depth are in
[2026-10-17-validation-records-sources.tsv](2026-10-17-validation-records-sources.tsv).

Tags: **[src]** a source states it (citation inline); **[code]** read in this
repository or a pinned reference at file:line; **[own]** this lane's derivation
or proposal; **[inf]** inferred from a source but not stated by it. Reference
paths are relative to `quarantine/sources/` (SMESH, usd-brep, Gmsh, Netgen, mmg,
spade) or `quarantine/new-iteration/` (`occt/src`, `opennurbs`). Pinned revisions:
SMESH 83f8bf48, usd-brep c9c5979f, OCCT 8.0.1 b8f597c6, Gmsh b4e84a43,
openNURBS eb92af3b, Netgen 31e85f81, mmg 4d8232c8, spade c8befc96.

## 0. Decision-relevant findings

1. **Two axes, not one chain.** Every mature system that reports well keeps
   *what was checked* apart from *what the work produced*. ISO 10303-59 keeps a
   criterion, an inspection result, summary counts and instance reports with
   extreme locations as separate entities [src ISO 10303-59 entities in the AP242 AIM, STEP Tools pages]. Parasolid keeps
   check faults apart from operation results and says its check-failure tokens
   "should not be taken to mean that the body itself is invalid" [src Parasolid
   FD V35 §30.3 p.421]. The proposal below therefore has a per-check outcome
   (`pass` / `fail` / `not_evaluated` with a reason) and a per-entity status
   (`valid` / `valid_goal_unmet` / `indeterminate` / `refused`), with
   `incomplete` added only at part level and above, as D-partial-output requires.
2. **The lifecycle map overstates mature practice on "could not evaluate".**
   X7 says every mature checker separates it from "invalid". Read in code and
   documents, only Parasolid does so explicitly. OCCT has the token
   (`BRepCheck_CheckFail`), but `IsValid()` folds it into invalid. usd-brep
   reports "could not be validated" as an ordinary failed check. SMESH, Gmsh,
   Netgen and openNURBS have no such state. ISO 10303-59 has no token either;
   it carries the distinction only as counts, and the SASIG/JAMA Quality Stamp
   records `Number-Total-Entities` beside `Number-Checked-Entities` (§2). The
   design still needs the state. The literature shows how rarely it is done
   right, not that everyone does it.
3. **Partial output is the default nearly everywhere; only the exit status and the
   list differ.** SMESH, OCCT BRepMesh, Gmsh (`AbortOnError` 0) and Netgen all
   keep meshing past a failed face. OCCT then calls `setDone()` after ORing a
   `Failure` bit into one run integer, so the run claims success. Parasolid's
   facet-body conversion refuses the whole body by default
   (`PK_ERROR_on_fail_any_c`). Its partial mode writes a report listing every
   face and edge left without geometry [src FD §84.3.3 p.1574]. D-partial-output
   picks the partial default. The evidence says that default is safe only with
   (a) a mandatory per-face list, as Parasolid and Netgen keep, and (b) an exit
   status that cannot read as success, as usd-brep's geometry validator does.
4. **The mesh-versus-sender volume check must not use the CAx-IF percentage bands.**
   [own] If the mesh lies within two-sided distance ε of the target surface, the
   volume error is at most about 2·ε·A to first order. A Ø1 mm pin at ε = 0.005 mm
   has A/V ≈ 4 mm⁻¹, so its honest volume deviation can reach about 4%. The
   CAx-IF production "OK" band is 0.5% [src GVP v4.6 §4.13.3 p.27]. The sender's
   validation properties are therefore compared with the exact properties of the
   geometry as read (an ingress check). The mesh is compared with exact
   properties through the ε·A bound, never through a percentage.
5. **Thresholds are agreements, and the profile must record which measure was
   used, not just the value.** JAMA calls its values non-binding [src JAMA V5.1
   §2.2 p.13-14]. JAMA's own company survey asks suppliers to mark "same measure"
   (◎) apart from "same kind, different measure" (○) [src JAMA 2025 supplier
   sheet p.2]. G-SH-LG has two ISO 10303-59 measures: from the 3D edge projected
   onto the faces, or from the stored pcurves [src JAMA V5.1 §3.2.19 p.30]. For
   us the difference is material, because OCCT discards plane pcurves on read
   (lifecycle finding 1). A profile row is therefore (criterion, measure, value,
   polarity, unit, provenance), not (criterion, value).
6. **Today's diagnostics put magnitudes in prose.** The `DIAGS___` table holds a
   severity, a stage string, ids and a free-text message, and every number
   lives inside that message
   (`native/step_source/main.cpp:324, 368, 381`; `native/mesher/values.hpp:164-168`).
   A record model with typed values replaces this table; it is not a field added
   to it.

## 1. What binds this lane

- **D-partial-output:** mesh the rest and mark the part incomplete, by default.
- **D-profile:** a named BRepGPU profile seeded from JAMA and CAx-IF. Users may
  tighten it. Loosening is recorded.
- **D-PMI:** warn and comply, never refuse.
- **D-determinism:** identical output across thread counts on one backend.
- **D-measure:** account per definition and per occurrence.
- **D-provisional:** provisional meshes are display only.
- **Internal data rule:** owned native values and binary formats internally;
  JSON only at an external or requested presentation boundary (BLACKBIRD,
  "Internal data formats").
- **Fidelity contract** (draft design §4): ε against the repaired target;
  shared-edge topology; orientation; accounting; reaching a cap never yields
  `valid`; invalid inputs that people use are meshed faithfully and reported.
- **M2, adopted at G1:** the per-item ladder (rungs 0–5), structural-first branch
  choice, refusal per item with every dependent face refused and recorded, and
  surviving alternatives reported (neighbour-reconciliation §5.4–5.6).

## 2. How mature systems and standards model findings

| System | Per-check outcome | "Could not evaluate" | Magnitude / location | Merge or cap | Roll-up | Human output |
|---|---|---|---|---|---|---|
| ISO 10303-59 in AP242 [src AIM] | `data_quality_inspection_result`. `…_with_judgement` adds `judgement : BOOLEAN` | No token. Only `number_of_inspected_instances` against defects | `measured_value_select` (boolean, measure, angle+length, angle+ratio). `extreme_instance` holds `locations_of_extreme_value` (point on edge curve, point on face surface, subedge, subface, knot locators…) | `summary_report_type` from `conclusion_only` to `full_statistics`. `detailed_report_type`: `measured_element` or `inferior_quality_element`. `preferred_ordering`: `extremity_order` or `detected_order` | Criterion report: up to two statistics, plus a representative measured value | None; this is a data model |
| Quality Stamp (SASIG/JAMA) [src JAMA V5.1 §7.5 p.92-93] | Per criterion: `Number-Checked-Entities`, `Number-Violating-Entities` | `Number-Total-Entities` against `Number-Checked-Entities` | Up to three parameters with units. No instances | None | One stamp per part; optional `Quality-Value` score | XML file beside the part, or an attribute inside it |
| Parasolid [src FD V35 ch.30, ch.11, §84.3.3] | Fault token, entities, position | `PK_TOPOL_state_check_fail_c` and `PK_FACE_state_check_fail_c`: checker failure, "not … invalid". Grouped checks skip later groups once one fails | Position, but no magnitude in the token | `max_faults`. "Only the most serious fault may be returned for each geometrical entity". Not all faults guaranteed | Report records: `status`, `old_items`, `new_items`. The report is overwritten by the next writing call and is not rolled back | Through the application |
| OCCT 8.0.1 [code] | 37-value `BRepCheck_Status` (`BRepCheck_Status.hxx:20-59`) | `BRepCheck_CheckFail` set on exception (`BRepCheck_Result.cxx:46-61`), but `IsValid` is false on any non-NoError (`BRepCheck_Analyzer.cxx:464-466`). The default edge check is sampled, "possible not correct result" (`BRepCheck_Analyzer.hxx:87-91`) | None | `Message_Alert` merges same-type alerts by default (`Message_Alert.cxx:30-42`, `Message_CompositeAlerts.cxx:46-64`) | BRepMesh ORs face and wire masks into one status, then calls `setDone()` (`BRepMesh_IncrementalMesh.cxx:101-118`) | Text |
| SMESH [code] | `COMPERR_OK`, `COMPERR_WARNING` ("computed anyway"), and the KO codes (`SMESH_ComputeError.hxx:42-66`) | No; exceptions are KO | Comment text; `myBadElements` for bad input mesh | `Worst()` keeps one error (`SMESH_TryCatch.cxx:79-98`) | Per sub-mesh `COMPUTE_OK`/`FAILED_TO_COMPUTE`. Compute keeps going; the run returns a boolean (`SMESH_Gen.cxx:200-257`) | GUI table: algorithm, shape, error, shape id, bad mesh (`SMESHGUI_ComputeDlg.cxx:116`) |
| usd-brep [code] | 191 requirements `BA.nnn` (`brep_validator.py:69`) | No: "could not be validated" is a failed check (`:2498-2507`). The geometry tool keeps hard errors out of the invalid count (`brep_geometry_validator_main.cpp:505-516`) | Prim only; sub-prim detail lives in message text | First violation only at three sites | Exit status = count of invalid breps, capped at 250 (`:680-684`) | One line per file, plus `FAIL[j]` lines |
| Gmsh [code] | `PENDING`/`DONE`/`FAILED` per entity (`GEntity.h:139`) | No | Free text | Last error and first error kept as strings | `nUnmeshed` count. `AbortOnError` default 0 (`DefaultOptions.h:408-410`) | Log lines |
| Netgen [code] | `facemeshstatus` 0 / 1 / −1 (`occgeom.cpp:312-325`) | No | None | — | `ErrorInSurfaceMeshing()`. `GetUnmeshedFaceInfo` lists failed faces (`occgeom.cpp:2328-2358`) | Warning line |
| mmg, spade [code] | `MMG5_SUCCESS`, `LOWFAILURE` ("failed but we can save a conform mesh"), `STRONGFAILURE` (`libmmgtypes.h:43-66`). spade `refinement_complete` (`refinement.rs:34-39`) | No | — | — | Return code | — |
| ACIS HEAL [src R10 Healing] | Autoheal result: fully healed, partially healed, stitched only, failed | No | Per-entity attributes. Old geometry retained after fix. Text change log per entity | — | Per body | Scheme highlighting of bad entities |
| CAx-IF GVP [src v4.6 §4.13] | Testing: green/yellow/red. Production: binary OK/Not OK | — | Deviation from the sender's value | — | Per property | — |
| NIST SFA [src AMS 200-12 §3.3, §6.5 p.61] | Error (red) / warning (yellow) | — | Entity worksheet cell | — | Status tab | Each error cites the recommended-practice section it violates; detection site and real error site can differ |

**Separating what the sources establish from what they suggest:**
- *Established:* ISO 10303-59 splits criteria into `erroneous_data` (the data
  violates a definition) and `inapt_data` (legal but harmful downstream). Each
  splits by topology, geometry, topology–geometry relationship and solid.
- *Established:* a Part 59 numerical criterion's threshold is typed by side.
  `gap_between_faces_related_to_an_edge` and
  `g1_discontinuity_between_adjacent_faces` require a
  `shape_data_quality_lower_value_limit`. `short_length_edge` and
  `small_area_face` require an upper value limit. Each criterion also declares
  its inspected element types, extreme-location types and accuracy types
  [src AIM entity pages].
- *Established:* the Quality Stamp exists so the receiver does not re-check. It
  records the tool, its release, the check profile, the guideline version and
  the OS. Checkers "using different algorithms" are not equivalent even after
  benchmarking, so the two sides must agree how to treat differences
  [src JAMA V5.1 §7.3–7.4 p.91].
- *Inferred:* the threshold names the near edge of the defect range. "Lower
  limit" means values at or above it are defects, as for gaps. "Upper limit"
  means values at or below it are defects, as for tiny edges. The pages read do
  not say this in prose; it is the only reading consistent with the criteria the
  polarity is attached to [inf].

## 3. Q1: the status lattice and the roll-up

### 3.1 Check outcome (one per check per subject)

`outcome ∈ {pass, fail, not_evaluated}`; `not_evaluated` carries one reason:

| Reason | Meaning | Source analogue |
|---|---|---|
| `check_failed` | The measurement itself broke: projection did not converge, an evaluator returned a non-finite value, or the routine threw | Parasolid `…_check_fail_c`, OCCT `BRepCheck_CheckFail` |
| `prerequisite_failed` | Skipped because an earlier check this one depends on failed | Parasolid grouped checks |
| `budget` | The check's own work bound fired before a verdict | — (own) |
| `unsupported` | No owned method exists for this subject kind, for example an offset surface with no computed bound (lifecycle X12) | usd-brep's unresolved-tolerance case, which it misfiles as a failure |
| `no_threshold` | The profile has no value for this criterion: JAMA's 合意値, "agreed value" | JAMA §8.1 |

**Rule A, gating checks are conservative [own].** A check whose verdict selects
a construction branch, such as an R1–R6 classification or a vertex rule, never
leaves a decision open. `not_evaluated` there takes the refusal branch, exactly
as design §5.2 R6 already does for failed or non-monotone projections. Only
non-gating checks can leave a subject `indeterminate`.

### 3.2 Entity status (face, then definition, occurrence, run)

Each entity's status is derived from the outcomes of the fidelity-contract
clauses that apply to it. *Hard* clauses: deviation (§4.2), shared-edge topology
(§4.3), orientation (§4.4), embedding (§4.8), and production (triangles exist).
*Soft* goals: quality targets (JAMA ch.6 A-TR-MA minimum angle, A-TR-AS aspect
ratio; JAMA defines the measures but sets no thresholds [src JAMA V5.1 §2.1
p.13]), the shading cap angle (§4.7), and any requested size goal.

| Face status | Rule [own] |
|---|---|
| `refused` | No triangles: a hard clause failed, or Rule A took the refusal branch. Carries a reason code and the finding that caused it |
| `indeterminate` | Triangles exist; no hard clause failed; at least one hard clause is `not_evaluated` |
| `valid_goal_unmet` | Every hard clause passed; at least one soft goal failed or was not evaluated, including a soft cap that fired |
| `valid` | Every clause passed |

Total order for reduction: `valid < valid_goal_unmet < indeterminate < refused`.
SMESH's `Worst()` is the same idea with three levels.

Mapping onto the systems read:
- mmg `SUCCESS` and `LOWFAILURE` sit at `valid` and `valid_goal_unmet`. Its
  conform mesh is saved and the goal is unmet.
- `STRONGFAILURE` is `refused`.
- SMESH `COMPERR_WARNING` is `valid_goal_unmet`.
- Parasolid's check-failure tokens are `indeterminate`.
- No system read has a state matching `indeterminate` at the level of the meshed
  entity. That level is this design's addition [own].

**Where `indeterminate` actually arises in an uncertified mesher [own].** With
certification excised, construction carries the hard clauses. `indeterminate`
is then rare by intent:
- (a) the density rule has no stated bound for a carrier kind, and the face is
  meshed by a fallback rule, so the deviation clause is `unsupported`;
- (b) the pre-mesh ε-inflated thin-feature query (design §5.6, lane G4) hits its
  budget, so the embedding clause is `budget`;
- (c) a non-gating input check fails internally.

Case (c) changes the face status only if that check backs a hard clause.
Offline oracle verdicts are a separate mechanism. They use the same record type
in their own phase (§4.2), never as a receipt on the runtime record (BLACKBIRD
tenet 5).

### 3.3 Roll-up

**Definition (part).** The roll-up is a commutative monoid over face records:
counts per status, plus the maximum status among produced (non-refused) faces.
Definition-level clauses fold in by the same maximum: shell closure and Euler
where the shell is closed, face-to-face embedding, and definition refusal at
ingress. The part status is then

```
refused         if no face produced, or a definition-level refusal
incomplete      else if n_refused > 0           (D-partial-output)
max_produced    otherwise ∈ {valid, valid_goal_unmet, indeterminate}
```

An `incomplete` part also reports its worst produced status. The display reads,
for example, "incomplete (3 of 412 faces refused); rest valid_goal_unmet".
Part-level order:
`valid < valid_goal_unmet < indeterminate < incomplete < refused`.

**Occurrence.** An occurrence takes the maximum of its definition's status and
its own clauses: placement admitted or refused under D-placements, and the
scaled ε (lane G9). An occurrence of a refused definition is refused.

**Run.** Status counts per definition and per occurrence (D-measure), plus the
same mapping over occurrences: `refused` if nothing was produced, `incomplete`
if any occurrence is incomplete or refused, else the maximum. Because the
reduction is a commutative monoid over a canonically keyed multiset, it is
identical for any thread count or completion order. That is what D-determinism
needs [own].

**Exit status [own]**, mapped from run status. It never reads as success when
any face is refused or indeterminate (lifecycle S15 requirement). Tool failure
stays distinct from input refusal, as usd-brep keeps hard errors apart:

| Exit | Run status |
|---|---|
| 0 | every occurrence `valid` or `valid_goal_unmet` |
| 2 | output written; some occurrence `indeterminate`; none incomplete |
| 3 | output written; `incomplete` |
| 4 | nothing produced: input refused |
| 1 | tool failure: internal error, I/O, device, resource exhaustion of the run itself |

**Provisional state.** Pending items exist only in live state (design §7;
D-provisional). No final record may carry `pending`. A run that ends with a
pending item has an internal error, not a status [own].

**What the evidence leaves to the operator:** whether triangles of an
`indeterminate` face reach a manufacturing export (§9 Q-OP-1).

## 4. Q2: the record schema, in native terms

Everything below is owned C++ values in struct-of-arrays tables. These are
persisted in the same tagged-table binary container the source snapshot already
uses (`BGSRC001`: eight-byte tag, row count, byte size per table;
`native/step_source/main.cpp:425-426`). Byte layout is left to the builder
(design §12). Enums are `enum class : uint16_t` with append-only numeric
values, because they are persisted. Strings appear only for external names (STEP
names, file names, profile names), held in one string pool and referenced by
`Range<char>`, as `Diagnostic` does today. **No record holds a number inside
text.** Human text is generated from records at presentation time (§6).

### 4.1 Shared value types

```cpp
enum class EntityKind : uint8_t { step_instance, product, definition, occurrence,
  solid, shell, face, loop, coedge, edge, vertex, carrier, curve, facet, chain };
struct EntityRef { EntityKind kind; uint64_t id; };          // id is the source id (G16)
struct OwnerRef  { DefinitionId definition; uint64_t occurrence_path; }; // path id from G9/G16
enum class ValueKind : uint8_t { length_mm, angle_rad, area_mm2, ratio, count, boolean };
struct Measured  { ValueKind kind; double value; };          // boolean: 0/1
enum class ParamKind : uint8_t { none, t, uv };
struct Location  { std::array<double,3> xyz; ParamKind param; std::array<double,2> p;
                   EntityRef on; };                          // Part 59 point_on_edge_curve / point_on_face_surface
enum class Method : uint16_t { closed_form, sampled_uniform, sampled_adaptive,
                   newton_projection, exact_predicate, bound_from_carrier };
struct Accuracy  { uint8_t kind; double value; };            // none | bound | estimate (Part 59 shape_measurement_accuracy)
```

### 4.2 Tables

| Table | One row per | Fields | Source of the field |
|---|---|---|---|
| `RUN_____` | run | input SHA-256 (exists today); tool build id; backend set; thread count; ε requests (value, unit, absolute or relative); units honoured (D-units); profile id + profile digest; non-default flags (η, β, G, arena flags); start/end time; OS type/version; host class, never a hostname. Part information read from the STEP header: name, originating system, time stamp | Quality Stamp Part-Information and Check-Environment [src JAMA §7.5]; doctrine on private detail |
| `CRITERIA` | profile row in force | criterion id; PDQ code (`G-SH-LG`…, or owned `B-…`); Part 59 entity name id; measure variant (§5.1); assessment kind (numerical, logical); polarity (lower or upper limit, or range); unit; seed value; active value; extra parameters (α geodesic factor, width tolerance); provenance (document id, clause pool ref); origin (seed, user tightened, user loosened); action (report, warn, gate) | Part 59 `shape_data_quality_criterion`, `…_assessment_by_numerical_test.threshold`, `self_intersecting_shell.geodesic_separation_factor` [src AIM]; D-profile |
| `MEASURE_` | inspected element, dense | per edge: class R1–R6; max d_f; max \|x\|; min θ; owner face; sample count; method; per-end vertex move. Per face: area, triangle count, max deviation bound used | Design §5.5 per-edge records; Part 59 `detailed_report_type.measured_element` |
| `FINDINGS` | (phase, criterion, subject[, second subject]) that failed or was not evaluated | finding key; phase; criterion; subject `EntityRef`; second subject (pairs); owner; outcome + reason; measured value; threshold copy; method; accuracy; extreme locations (one or two `Location`); Part 59 class (`erroneous` or `inapt`); action range | Part 59 `shape_data_quality_inspection_instance_report_item` + `instance_report_item_with_extreme_instances`; `inferior_quality_element` |
| `ACTIONS_` | thing done to make the mesh | action key; kind; the finding(s) it answers; affected entities; value before (P1) and after (P2) of the same measure; maximum displacement; carrier chosen (3D curve, S₁∩S₂, owner surface); M2 rung (0–5); number of surviving alternatives; branch-rule step that decided; the alternatives' keys | Parasolid report `status`/`old_items`/`new_items` [src FD ch.58 p.984-985; ch.11 p.139-143]; ACIS old geometry kept after fix [src R10]; M2 §5.4–5.6 |
| `STATUS__` | face, definition, occurrence, run | entity; status; reason code; per-clause outcome (2 bits per clause); triangle count; finding range; counts per status (non-face rows) | §3 |
| `SUMMARY_` | (phase, criterion) | inspected count; violating count; not-evaluated count; representative (worst) value + location; truncated flag | Part 59 `data_quality_inspection_criterion_report` with `statistical_value_type`, `representative_measured_value`; Quality Stamp Criteria |

**Phases** (`uint8_t`): `P0 raw_entities` (direct STEP entity reads, if lane G1
recommends them); `P1 as_translated` (pre-heal, measured on what the OCCT
bridge hands over); `P2 resolved` (after R1–R6 and V1–V4, measured on resolved
chains); `P3 mesh` (emitted triangles); `P4 export` (written bytes, lane G3);
`P5 oracle` (offline verdicts). The same criteria run at P1 and P2. "Before and
after healing" is the pair (P1, P2) of one key with the phase changed. VDA 4955
§3.1.6.1 is why P2 exists: healing can itself create tiny elements, and only a
re-measurement finds them [src VDA 4955 v4.1]. P1 is not "the input before
anything touched it", and its name says so: the bridge has already edited the
geometry (lifecycle finding 1, contradiction S4(a)).

**Action kinds** (`uint16_t`): edge R1…R6; vertex V1…V4; small-feature collapse
(G10); missing-pcurve recompute with residual (D-master); ε tightening near a
thin feature (§5.6); orientation flip; face-to-edge re-split (G2); algorithm
fallback; refusal. The lifecycle map notes that face- and body-level silent
actions also need records (S15) [inf L3]; orientation flips and fallbacks are
those.

**Identity and order [own].** A finding key is (phase, criterion, subject,
second subject). An action key is (kind, primary entity). Records are sorted by
key at the end of a run. The id is the index after sorting, so ids are identical
across thread counts (D-determinism). The run record carries no
completion-order data except times. Subject identity is the source id that lane
G16 settles. Until then, `EntityRef` holds today's `SourceId` values plus the
STEP instance number where the bridge retains it.

**No merging, no cap on records [own].** OCCT's default merge drops later alerts
of one type. Parasolid's `max_faults` and most-serious-only rule make counts
lower bounds. Records are one per subject and criterion, so their number is
bounded by entity count times criteria. The dense measurements cover edges,
around 25 k rows for the watch [inf from 9162 faces]. If a budget ever caps
findings, `SUMMARY_.truncated` is set and the counts are labelled lower bounds.

**The JSON boundary.** `brepgpu report` formats these tables at the boundary,
as text (default), TSV, JSON on request, or Quality Stamp XML (an external
protocol). Nothing internal parses its own output.

### 4.3 Owned criterion codes

PDQ codes take the form `X-YY-ZZ` with X ∈ {G geometric, O non-geometric,
D drawing, A CAE mesh} [src SASIG V2.1 slides p.17; JAMA V5.1 TOC].
Owned criteria take the unused prefix **B** so they never collide:

| Code | Criterion | Kind |
|---|---|---|
| B-ME-DV | mesh deviation from target (P3, P5) | numerical, lower limit = ε |
| B-ME-WT | shared-edge identity / watertightness | logical |
| B-ME-OR | 3D orientation against the surface normal | logical |
| B-ME-SI | mesh self-intersection outside reported source pairs (P5) | logical |
| B-ME-VP | \|V_mesh − V_exact\| ≤ 2εA (§5.3) | numerical |
| B-RQ-UN | ε below the source's stated uncertainty (D-PMI) | numerical |
| B-RQ-PM | ε loose against the tightest PMI tolerance (D-PMI, lane G15) | numerical |
| B-VP-SV | sender validation property against the exact property as read | numerical |
| B-ED-CL | edge needed R3–R6 (P1) | logical, carries class |
| B-FA-RF / B-FA-IN | face refused / face indeterminate | logical, carries reason |

Mesh-quality goals reuse the JAMA ch.6 A-codes (A-TR-MA, A-TR-AS). JAMA defines
the measures and deliberately sets no thresholds [src JAMA V5.1 §2.1 p.13].

## 5. Q3: the BRepGPU profile `brepgpu-pdq-1`

### 5.1 What the profile is and is not

- It holds the data-quality criteria for the report (`CRITERIA`). It does not
  hold the mesher's ε-derived healing bands η, β and G. Those are parameters of
  the request and arena flags. They are recorded in `RUN_____` with every
  non-default value, under the same rule: a user may move them, and the move is
  written down [own].
- The two scales differ, and the records show both. At ε = 0.005 mm,
  β = 0.0025 mm. An edge with a 0.008 mm face gap passes JAMA's G-SH-LG
  (≤ 0.01 mm) but is R3/R4 work for the mesher. P1 shows a PDQ pass with an
  `B-ED-CL` R3 finding, and both are true.
- A row names its measure variant. G-SH-LG has two Part 59 measures,
  `gap_between_faces_related_to_an_edge` and
  `gap_between_pcurves_related_to_an_edge`. JAMA added the second in V5.0 and
  notes that native CAD usually keeps pcurves [src JAMA V5.1 §3.2.19 p.30]. The
  profile runs both where pcurves exist. When OCCT discarded the file's plane
  pcurves, the pcurve variant is `not_evaluated / unsupported`. It is never
  silently replaced by recomputed ones.

### 5.2 Seed values

Polarity uses the Part 59 limit side (§2). "Action" is what the profile does
when the check fails. Under D-PMI and "invalid but used inputs meshed
faithfully" (design §4.9), data-quality failures *report*; they do not refuse.
Refusal comes only from the mesher's own gating checks (R6 and the ingress
admission refusals) and from export format rules.

| Code | Part 59 criterion | Seed | Polarity | Provenance | Action |
|---|---|---|---|---|---|
| G-SH-LG | gap_between_faces_related_to_an_edge; …pcurves… | 0.01 mm | lower | JAMA V5.1 §3.2.19 p.30; §8.1 p.100 | report |
| G-FA-EG | gap_between_edge_and_base_surface | 0.01 mm | lower | JAMA §3.2.15; §8.1 | report |
| G-FA-VG | gap_between_vertex_and_base_surface | 0.01 mm | lower | JAMA §8.1 p.99 | report |
| G-LO-LG | gap_between_adjacent_edges_in_loop | 0.01 mm | lower | JAMA §3.2.13; §8.1 | report |
| — | gap_between_vertex_and_edge | 0.01 mm | lower | **own**, by analogy; no JAMA row | report |
| G-ED-TI | short_length_edge | 0.01 mm | upper | JAMA §3.2.12; §8.1 | report |
| G-FA-TI | small_area_face | 1e-4 mm² | upper | JAMA §8.1 p.99 | report |
| G-FA-NA | entirely_narrow_face | width 0.01 mm | logical + width | JAMA §3.2.17. Its rationale says manufacturing should use a larger L | report |
| G-SU-DC | nearly_degenerate_surface_boundary | 0.01 mm | upper | JAMA §8.1. JAMA excludes fully collapsed boundaries on purpose | report |
| G-SH-IS, G-LO-IS, G-FA-IS | self_intersecting_shell, …loop, intersecting_loops_in_face | interference 0.01 mm; α unset | logical | JAMA §3.2.29 etc.: d < 0.01 mm, arc > 0.01·α, α "agreed value" | report; `no_threshold` until α is set (§8) |
| G-SH-NT | g1_discontinuity_between_adjacent_faces | 0.01° smooth; crease ≥ 0.2° | lower | JAMA §3.2.28 p.34. Isuzu applies 0.5° (JAMA 2025 sheet p.2), Nissan 3°. CAx-IF COPS sharp 1.0° (GVP v4.6 §6.2) | report. The mesher's own continuity rule (0.01°/1°, design §5.2.6) is separate |
| G-SH-NM | non_manifold_at_edge | 0 | logical | JAMA §3.2.30 | report; export gate for 3MF (design §4.9) |
| G-SH-FR | free_edge | — | logical | JAMA §8.1 | report; part marked open |
| G-SH-OU | over_used_vertex | 3 | upper | JAMA §8.1 | report |
| G-LO-IT, G-FA-IT, G-SH-IT | open_edge_loop; inconsistent_face_and_surface_normals; …closed_shell_normals | — | logical | JAMA §8.1 | report (loop open = ingress refusal of that face, design §5.1) |
| G-CU-HD, G-SU-HD | excessively_high_degree_* | 5 | upper | JAMA §8.1 | report only; the mesher accepts any degree |
| O-CM-AP | system accuracy setting | 0.01 mm / 0.01° | agreement | JAMA §4.2.1.3 | report the file's uncertainty against it |
| B-VP-SV | volume, area | 0.5 % | lower | CAx-IF GVP v4.6 §4.13.3 p.27 (production) | warn; see §9 Q-OP-2 |
| B-VP-SV | centroid | 0.02 mm + 0.1 % of diagonal when the diagonal > 20 mm, else 0.02 mm absolute | lower | GVP §4.13.1 (the 20 mm rule), §4.13.3 | warn |
| B-ME-VP | mesh volume against exact | 2·ε·A | lower | **own** (§5.3) | goal |
| B-ME-DV | mesh deviation | ε | lower | design §4.2 | hard clause |
| B-RQ-UN, B-RQ-PM | request against uncertainty or PMI | — | — | D-PMI; ratio from lane G15 | warn and comply |
| A-TR-MA, A-TR-AS | min angle, aspect ratio | none seeded | — | JAMA ch.6: measures without thresholds | goal, off unless requested |

Values JAMA leaves as 合意値 (agreed value) are seeded as `no_threshold`, not
invented: G-SH-SA, G-SO-TI, G-CU-CR, G-SU-CR, G-SU-FO, G-SU-RN and α. Their
measurements are still written to `MEASURE_` where the subject is measured
anyway. SASIG is not a second source of numbers: JAMA records that the SASIG
guideline gives no threshold values (JAMA V5.1 §2.10 p.16), while the SASIG
V2.1 slides list "(Recommended values)" as a bracketed attachment (p.17). That
attachment (SASIG PDQ V3.0 Attachment C) is already on SHOPPING_LIST.md and is
what would settle it.

### 5.3 The volume bound [own]

Suppose the mesh surface M and the target surface ∂S are within two-sided
Hausdorff distance ε. The design's deviation oracle is two-sided (§10). Then
the symmetric difference of the solids lies inside the ε-tube about ∂S. Its
volume is 2εA(1 + O(εκ)), where A is the surface area and κ a curvature bound.
So |V_M − V_S| ≤ 2εA to first order. Inscribed chordal meshes sit inside convex
regions, so the error has a sign there. The bound stays two-sided because
concave regions reverse the sign.

- Hypothesis H-VP: every part of the corpus satisfies
  |V_mesh − V_exact| ≤ 2εA_exact.
- Falsifier: any part beyond it, with V_exact and A_exact from an
  independent exact integration offline. A violation means the deviation clause
  failed, or the tube estimate fails on high-curvature features where εκ is not
  small. Either is worth knowing.

### 5.4 Tightening and loosening

- `origin` per row is `seed`, `tightened` or `loosened`. Direction follows
  polarity:
  - for a lower-limit (defect-if-above) criterion, a larger active value is
    looser;
  - for an upper-limit criterion, a smaller value is looser;
  - for a logical criterion, switching it off is loosening.
- A loosened row writes the seed and the active value. `RUN_____` lists every
  loosened criterion. The human view prints them on the first lines (§6). The
  profile digest changes, so equal profile names cannot hide different values.
  This is the Quality Stamp's `Check-Profile` with a digest added; the stamp's
  receiver check compares the profile used against the profile required
  [src JAMA §7.8 p.94-95].
- Parasolid offers the same three modes for tolerance limits in offsetting:
  `limit_no`, `limit_report` (allow and record) and `limit_yes` (fail)
  [src FD §58.10 p.993]. D-profile is the `limit_report` mode for loosening.

## 6. Q4: the human view

The view is generated from the tables; it holds no state of its own. Its design
follows what each mature output gets right and avoids what each gets wrong:

- **Header first (Quality Stamp):** input name and digest; tool and build;
  profile and digest, with loosened rows named; ε requests; run status and exit
  code, with its meaning in words.
- **Failures before successes (Netgen `GetUnmeshedFaceInfo`, Parasolid partial
  conversion list):** every refused face, then every indeterminate face. Each
  shows its source identity, owner chain and reason.
- **One finding per line (Parasolid's token, entities and position; SFA's cited
  clause; usd-brep's stable ids):**

  ```
  FAIL  G-SH-LG gap_between_faces_related_to_an_edge  P1
        edge 1834 (STEP #20511) faces 412|413  def 1713 'barrel-arbor' occ 1713/2
        0.0213 mm  > 0.010 mm  [JAMA V5.1 §3.2.19]  sampled 64, accuracy n/a
        at (12.418, -3.007, 4.551) t=0.42
        action R3 re-intersect: P2 residual 1.2e-6 mm, max move 0.0107 mm, rung 2, 0 alternatives
  NOTEVAL G-SH-LG gap_between_pcurves_related_to_an_edge  P1  face 412
        unsupported: plane pcurve discarded by translation layer
  ```

  The values above illustrate the format; they are not measured.
- **Summary per criterion (Part 59 summary report):** inspected, violating,
  not evaluated, and the worst value with its location. Instance lines expand
  under it (detailed report). Ordering is `extremity_order` by default, or key
  order (`preferred_ordering`).
- **Tree view by assembly (SMESH's table, extended):** occurrence path →
  definition → face, with status and counts at each level, so the roll-up of §3
  can be read top-down.
- **On the model:** the display colours refused, indeterminate and
  goal-unmet faces, and marks extreme locations. This follows ACIS's Scheme
  highlighting and CADIQ's 3D PDF review [src R10; CADIQ datasheet]. It is a
  display consumer of the same records, not a second report.
- **Messages from templates:** text is a function of (criterion, outcome,
  reason, action kind). Re-running the view on the same records reproduces it
  byte for byte. No free text is stored that could go stale or contradict the
  values.
- **Failure modes this view is built against** (lifecycle S15 list [src L3]):
  a status that lies (OCCT `setDone()`); lost localisation (usd-brep prim-only;
  `BRepCheck_Analyzer(...).IsValid()` reported per solid today,
  `native/step_source/main.cpp:328-330`); short-lived reports (Parasolid's
  report is overwritten by the next writing call); merged summaries (OCCT
  alerts); text-only logs (Gmsh, openNURBS, whose `IsValid` text is "not
  intended to be useful as a high level user interface tool",
  `opennurbs_object.h:520-536`); detection site differing from error site (SFA
  §6.5). The record's two-subject form answers the last: the subject and the
  related entity are both named.

## 7. Measurable hypotheses and falsifiers

| # | Hypothesis | Falsifier | Mechanism |
|---|---|---|---|
| H1 | Record emission (all tables, canonical sort) costs ≤ 1% of meshing wall time and ≤ 2% of output bytes on the arena corpus | Either limit exceeded on any input class | Arena timing rows with records written to a discard sink vs a file; byte counts |
| H2 | The sorted record set is byte-identical across 1, 12 and 24 threads on one backend | Any difference | Metamorphic run in the arena (D-determinism) |
| H3 | Exit status is never 0 when any face is refused or indeterminate | Forced-cap and fault-injection tests (design §10) returning 0 | Direct oracle on exit status |
| H4 | Every input face appears exactly once in `STATUS__`, and Σ statuses = face count, per definition and per occurrence | Missing or duplicate face | Accounting oracle |
| H5 | For R1–R3 edges, the P2 G-SH-LG value is ≤ η or β as the class requires | Any R1–R3 edge above its band at P2 | P2 re-measurement, the same code path as P1 |
| H6 | Our P1 G-SH-LG verdicts agree with an independent checker within stated accuracy on the NIST/CAx-IF fixtures | Disagreement beyond accuracy on more than a stated fraction | Offline comparison against OCCT ShapeAnalysis edge-to-surface distance at high sampling (P5). JAMA §7.4 predicts some disagreement; the size is unknown |
| H-VP | §5.3 | §5.3 | Offline exact integration |
| H7 | `indeterminate` is rare: < 0.1% of faces on ABC/NIST/OCCT corpora | Above it, which would mean case (a) or (b) in §3.2 is common enough to need its own work | Arena status counts |

## 8. What the literature does not say, and what would settle it

- **The geodesic separation factor α for self-intersection criteria.** JAMA
  leaves it as an agreed value. Part 59 makes it an attribute without a default.
  Settle by sweeping α over the corpus. Measure the self-intersection flag rate
  against hand-labelled true self-intersections on the authored families; pick
  the knee.
- **How far checkers disagree.** JAMA §7.4 states that they disagree but gives no
  size. Settle by H6.
- **Whether the CAx-IF 0.5% production volume band suits sub-20 mm parts at all.**
  GVP restricts only the *centroid* relative rule by the 20 mm argument; for
  volume it says nothing about small parts. The 0.02 mm model accuracy
  over A/V for a Ø1 mm pin suggests about 8% uncertainty from modelling accuracy
  alone [own]. Settle by computing sender-VP deviations on the NIST and
  open-hardware corpus (watch and printer parts) and recording their
  distribution against part size.
- **The cost of P1 + P2 double measurement** at arena scale is not in any
  source. H1 measures it.
- **Whether `indeterminate` faces should reach manufacturing exports.** No
  source decides it for a mesher; this is an operator question (§9).

## 9. Operator questions

These stop the questions they belong to. Each carries the default this paper
assumes until answered; none is settled here.

- **Q-OP-1.** Does a manufacturing export (STL, 3MF) include triangles of an
  `indeterminate` face? D-partial-output covers refused and indeterminate faces
  for the part's status, but not whether an unproven face's triangles are
  written. Default assumed: written to the internal schema and glTF with the
  face id; excluded from STL and 3MF, which cannot mark them; the part is
  `incomplete` for that export.
- **Q-OP-2.** What does a sender validation-property mismatch beyond the
  CAx-IF production threshold do? The D-profile options listed it, but the G1
  decision did not say. Default assumed: warn and comply (as D-PMI), finding
  recorded, the part's status unchanged.
- **Q-OP-3.** Exit codes 2/3/4 (§3.3) are new caller-visible behaviour.
  Confirm that a partial run exits non-zero by default.

## 10. Corrections to earlier repository papers

- Lifecycle map S15 says "usd-brep: 228 requirement IDs". The validator defines
  **191** requirements (`brep_validator.py:69`). 228 is the number of
  `_AddFailedCheck` call sites. The test-plans-cad paper's 191 is right.
- Lifecycle map X7 and finding "every mature checker separates 'could not
  evaluate' from 'invalid'": overstated; see §0 item 2 and §2.

## 11. Defects found (by reading; not fixed by this lane)

1. `native/step_source/main.cpp:324, 368, 381`: diagnostic magnitudes are
   written only inside free-text messages. Examples: `uv_metric_mm=…`,
   `source tolerance=…`, `displacement mm=…`. No typed value exists for a
   consumer or oracle to read. This is a design defect against the internal
   data rule's intent, not a crash.
2. `native/step_source/main.cpp:328-330`: `BRepCheck_Analyzer(solid).IsValid()`
   runs in its default sampled mode (`BRepCheck_Analyzer.hxx:87-91`). It folds
   `BRepCheck_CheckFail` into "invalid", and the result is recorded per solid
   with no faulted entity. The message also carries certified-regime language
   ("independent geometry qualification remains required"), which the
   certification excision removes.
3. `native/mesher/source.cpp:642-643`: any source diagnostic with severity ≥ 2
   refuses the whole source. This is already recorded as the per-file refusal
   contradiction (lifecycle §3.3); it is restated because the record model above
   replaces it.

## 12. For SHOPPING_LIST.md

Items already listed are not repeated: ISO 10303-59:2022, SASIG PDQ V3.0
Attachment C, the Parasolid PK reference for `PK_BODY_check_o_t` and the facet
fault tokens, and the CADIQ/CADfix manuals.

- **The "ISO 10303-59 使用ガイド" (Part 59 usage guide)** that JAMA V5.1 §3 cites
  for the detailed differences between its items and Part 59. Publisher: JAMA or
  JAPIA. Not found openly.
- **The SASIG Quality Stamp schema as a standalone `qualitystamp.xsd`** at its
  current version. JAMA V5.1 §7.9 prints a V2.1-era copy inline, which suffices
  for an exporter draft.
- **The JAMA/JAPIA importance table (Excel)** referenced in JAMA V5.1 §8.2. It
  is the filterable form of the importance matrix and is supplied separately
  from the PDF.
- **NX Check-Mate PDQ check documentation** (Siemens, licensee). This is the
  checker Isuzu applies with company thresholds (JAMA 2025 sheet). It would show
  how a production checker records measure variants and not-checked entities.
