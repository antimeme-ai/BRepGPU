# M7 plan tribunal: findings

Three reviewers read the M7 plan (`M7-plan.md` and the first-wave unit plans in
`plans/`) once each, under `briefs/M7-plan-tribunal.md`. This file collects
their findings for the planner's single fix pass. Line numbers refer to the plan
files as the planner left them (uncommitted at review time, committed with this
file).

**State (2026-10-19):** all three reviewers returned (36 plan findings, 28 bead
findings). The fix pass has not started: the operator called a break before it.

## Decision taken during the tribunal

- **T1.5, call 7 (CPU-only first headline): stands** [call; operator delegated].
  MS2 is labelled the declared CPU-only configuration, with the GPU-on
  configuration not yet measured. NVIDIA has no GPU path, so a CPU-only win is
  the stronger claim. The GPU-on configuration is its own declared
  configuration once F8 has run per lane. P24 and P25 gate MS3.

- **T3.3, builds during measurement windows** [call; operator delegated]: every
  timed run takes a per-lane exclusive lock; while it is held, builds and tests
  run only on CPUs outside that lane, or wait. The other lane's state is a
  recorded column.
- **T3.8, F7 FP32** [call; operator delegated]: F7 is a general arm, as design
  §21.4 states (Metal has no double), not device-only. A recentred FP32 CPU
  evaluation path is built with P24, so all four (F7, F8) combinations of the
  fraction exist.

## Dispositions

The planner fills one row per plan finding in a table at the end: `fixed`
(with the plan section), `rejected` (with the reason) or `bead`. Bead findings
are filed by the coordinator.

## Reviewer 1: strategy and goal fit

### Plan findings

**T1.1 The first arena cannot run as planned, so MS3 cannot be reached (M7-plan.md:1171-1190, 240-242, 892-894, 915-916, 1547-1552).** class: plan
- Wrong: I9 is done when "the first exploratory arena has run", the fixed 2^(8−2) fraction (operator, design §23.2). Four of eight flags have no second level when I9 runs: F2 needs P30, F5 needs P31 (both start after I9); F7 needs P24, F8 needs P25 (neither gates I9 or any milestone).
- Evidence: the fraction aliases F7 = F1·F2·F3·F4 and F8 = F1·F2·F5·F6; without both levels of every flag it is not resolution V. MS3 needs defaults adopted from the arena.
- Fix: P29–P31 depend only on the mechanism they replace, not on I9. P24, P25, P30, P31 land before I9 runs the first arena (runner can be built earlier). List all four in MS3's "After".

**T1.2 MS3 gates on oracles whose units are not on its path (M7-plan.md:1547-1552, 1433).** class: plan
- Wrong: MS3 requires oracles 1–10 and 14–17 green per report class; oracle 7 needs V7 and P20; oracle 8 needs V8, which consumes P21 output (L1378); oracle 9 needs P28. None in MS3's list or the §7.2 path.
- Fix: add P20, V7, P21, V8, P28 to MS3 and the §7.2 diagram (or narrow the gate to oracles applying to the claim's scope).

**T1.3 MS1's seeded corpus part will often contain faces no pre-MS1 unit meshes (M7-plan.md:1534-1540, 650-668, 670-696).** class: plan
- Wrong: MS1 draws from the analytic-only report class; ring torus is analytic (design §5.4 row 2) but tori are meshed only by the lattice (§13.1 L1838), which is P15, after MS1. P14 covers §13.1 rows 1–4.
- Evidence: ring-torus faces in 79 of 506 export files (entity-admission-matrix paper L57); fillets are tori. A seeded draw stalls MS1 or invites a re-draw.
- Fix: define MS1's draw class before drawing as the carriers P14 serves (plane, cylinder, cone, sphere, extrusion), recorded; or put P15 before MS1 (it is on MS2's path anyway).

**T1.4 No unit builds the §3.2 target field (M7-plan.md:537-565, 650-668, 670-696).** class: plan
- Wrong: target S_f + φ_f·n_f with ℓ_f, M_φ, ψ_f, nearest-boundary-point extension and reflex-bisector trigger; P10 claims only resolved edge and g_f; P14/P15 claim the ledger but not §3.2.
- Evidence: r_disp needs M_φ and d_t (§3.4); A0 needs ψ_f (§3.6); band constraint lines depend on d > ε_loc/4 at reflex corners (§3.2).
- Fix: P10 produces `TargetField` (g_f per loop, ℓ_f from P9's lfs, φ evaluator, M_φ and ψ_f bounds); P13, P14, P15, P16 consume it; add P9 to P10's consumes.

**T1.5 Call 7 inverts the design's measurement stance (M7-plan.md:86-89, 1541-1546, 232-233).** class: operator → decided by coordinator on delegation
- Wrong: first headline CPU-only and no milestone needs the GPU; design §20.1 names lane mode with GPU "the shipped configuration that must win"; F8 default GPU on; §22.4 item 8 sets owned lane (socket + GPU) against NVIDIA, CPU-only "reported beside".
- Fix proposed: operator rules; if call 7 stands, label MS2 CPU-only with GPU-on not yet measured, and P24/P25 gate MS3.
- **Decision (coordinator, operator delegated):** call 7 stands. MS2 is labelled the CPU-only declared configuration, the GPU-on configuration not yet measured; NVIDIA has no GPU path, so a CPU-only win is the stronger claim. The GPU-on configuration is its own declared configuration once F8 has run per lane. P24 and P25 gate MS3.

**T1.6 MS2 would publish a Fabricate-profile headline with no written output (M7-plan.md:1541-1546, 789-806).** class: plan
- Wrong: P20 (writers) and V7 come after MS2, so nothing checks the files a consumer receives.
- Evidence: §3.3 every point "as written (after float32 rounding)" within ε_f; oracle 1 on each written format at ε_f (§24.1 row 1); Fabricate consumers are STL and 3MF (§6.2).
- Fix: MS2 requires P20 (STL and 3MF at least), oracle 1 on files read back, and V7. Export stays outside timed scope (§22.3).

**T1.7 MS1 claims "checked by the oracles" without the embedding oracle (M7-plan.md:1534-1540, 217, 1317-1331).** class: plan
- Wrong: oracle 5 not among MS1's oracles; P11 builds clearance before MS1.
- Evidence: V5 needs only V1 and an output reader; §9 L1480 plans to score NVIDIA's inside-out fillet class with it; yet V5 starts after P18.
- Fix: V5 starts after V1, run first on NVIDIA's recorded outputs; MS1 adds oracle 5.

**T1.8 Ingress gate checked on the watch alone (M7-plan.md:1108, 1547-1552).** class: plan
- Wrong: I5 done when "the §22.3 ingress gate holds on the watch on both lanes"; MS3 never asks for the gate on the corpus.
- Evidence: §5.6 and §22.3 set the gate per input and lane.
- Fix: MS3 requires the gate on every scored input of the stratum, failures listed by input.

**T1.9 Before MS2, NVIDIA is measured only on the watch (M7-plan.md:1476, 227, 1130-1148).** class: plan
- Wrong: the only early NVIDIA run is the watch (wave 2); I7 (`.25.20` inventories) waits for V3, V6, B3; post-MS1 stage-budget revision and every pre-MS2 choice compare only against the watch.
- Evidence: §22.6 budgets "checked per report class, never on the watch alone".
- Fix: split I7: I7a inventories and per-key chords, labelled pre-oracle screens on the corpus strata, right after B3 (wave 2–3); I7b achieved fidelity through V3 and V6.

**T1.10 I5 too large for one conceptual unit, on MS2's critical path (M7-plan.md:1087-1108).** class: plan
- Wrong: one agent and one review would cover geometry/topology/units/placements/styles decode, the C++ store writer, streaming per definition into L1, and the ingress gate, without OCCT — the whole Part 21 schema walk (§5.2, §5.6).
- Fix: split into I5a (geometry and topology decode plus store writer, oracle 8 byte equality), I5b (product structure, placements, units, styles), I5c (streaming into L1 and the gate).

**T1.11 Parts of the design have no owning unit; version 3 incomplete (M7-plan.md:51-63, 192-201, 1068-1085; plans/P1-source-view.md:76, 148-156).** class: plan
- Wrong: §5.4 rows 22–27 (tessellated pass-through `.61`, not_a_brep, semantics_unavailable, illegal_edge_geometry, external_reference_unresolved, no_surface_geometry) belong to no unit; P1's Reason enum lacks those codes and the §5.4 offset codes; version 3 has no section for row 22's triangles, so call 3's "format changes once" cannot hold; §18.1 C3 (no knife-edge fold) belongs to no construction unit.
- Fix: rows 22–27 to I4; their codes appended in P1 Task 1 now (I1 and V1 consume that enum early); decide row 22 now (version 3 section or pass-through outside the store); C3 joins the P13/P14 triangle checks.

**T1.12 Two of MS1's oracles have never been seen to fail (M7-plan.md:1538-1539, 111-119, 1274-1277).** class: plan
- Wrong: oracles 10 and 17 gate MS1 but no real defective output reaches their fail branch before MS1; NVIDIA outputs carry no chains or producer records for oracle 10; F5's other level arrives with P31 after I9; nothing reaches oracle 17's branch.
- Fix: MS1's record states per oracle which real output exercised its fail branch, marks oracles 10 and 17 "fail branch not yet exercised" (honest under call 12; no hand-made mutants).

### Bead findings
- T1.b1 M7-plan.md:32, 589, 667, 761, 1169 "(§11)"/"(§9)" mean the plan's own sections but L12 says § numbers are the design's; write "plan §11", "plan §9".
- T1.b2 Design §22.3 warm runs need a second query in one process; P18's `brepgpu-fm mesh` is one-shot and no unit owns a session mode. Assign to P18 or I8.
- T1.b3 plans/I1-step-parse.md:99 splits the budget by OCCT's phase proportions, which describe OCCT, not the owned reader; keep the total as the gate and report owned per-phase costs against it.
- T1.b4 plans/V1-oracle-geometry.md:422 census width targets come only from ε_ref = 0.005 mm; add the §23.4 relative levels (1e-4 of the diagonal).
- T1.b5 M7-plan.md:1047-1050 state that G-4's raw-everywhere write applies only under `--new-path`, so the old mesher's stores stay unchanged until P23.
- T1.b6 M7-plan.md:1192-1206 have each unit write its families into `tests/families/` from wave 1 on, leaving I10 the presence and branch check.
- T1.b7 M7-plan.md:820-821 P21 records "rounding residual per entity" but version 3 has no field; name the DIAGS___ typed row it uses.
- T1.b8 plans/P1-source-view.md:349 `build_l1` and `content_digests` take a whole Store, but I5 streams per definition (design §5.6); add a P1 test that one definition's sub-store gives the same L1Definition bytes as the whole store.

## Reviewer 2: sequencing, interfaces and executability

### Plan findings

**T2.1 V2, V3, V4 must land before the code they judge, but their done criteria need that code's output and formats that do not exist yet (M7-plan.md:1269, 1274-1277, 1290, 1295-1296, 1308, 1314-1315, 1411, 548-549).** class: plan
- Wrong: V2 lands before P10 starts, reads "P10's chain dump", done when it decides every sample of P10's families and MS1's outputs. V4 lands before P12, reads "P12's chain dump", must decide MS1's inputs. V3 lands before P14, reads BGFMOUT1 (defined by P18 after P14), must decide MS1's inputs. Cycles; two throwaway dump formats.
- Fix: define the chain and face sections of the output container once, early (P2, or a small unit before V2); P10, P12, P18 write it. Each V unit's done criterion = authored families + NVIDIA's recorded outputs. "Decides every face or sample of MS1" moves into MS1's criteria.

**T2.2 P11 and P16 done only when oracles pass that start after them; P11 deadlocks (M7-plan.md:585-586, 591-592, 712-713, 717; rows 217-218).** class: plan
- Wrong: P11 "done when oracle 5 finds no forbidden pair"; V5 starts after P18, which needs P14, P12, P11. P16 "done when oracle 6 passes" but V6 starts after P16. Contradicts §7.1/§8 "oracles stay one step ahead".
- Fix: V5 starts after V1 (authored embedding families + NVIDIA inside-out fillet outputs), before P11. V6 starts after V1 (G13's seven surfaces), before P16.

**T2.3 The required execution skill breaks the three-agent cap and the one-review rule (M7-plan.md:78-85; line 3 of each unit plan).** class: plan
- Wrong: unit plans require superpowers:subagent-driven-development, which dispatches a fresh implementer per task, spec and quality review after every task, up to five fix rounds, and a final whole-branch review. Contradicts "one unit per agent", "one independent review per unit" (call 6, §4), and the operator's three-agent cap; three units plus reviewers exceed three; B3 still running.
- Fix: inline execution (one agent per unit doing every task, superpowers:executing-plans) plus one review at the end. Reviewers and any running lane, B3 included, count toward the three slots.

**T2.4 Several units start before units they consume have landed (§6.1 rows 197, 199, 211, 214, 223, 227, 228; also 372, 383-385, 408, 413, 637, 1079, 1141, 1162, 1378, 1460).** class: plan
- P4 consumes RecordSink (P2) and its model tier uses V1's eval.hpp, but starts after P1 only (wave 2 runs P4 beside P2).
- P5's estimate(..., const KeyRequest&) needs P3, but P5 starts after P4 only (wave 3 runs P5 beside P3).
- P13 consumes SpanBounds (P5) and ClearanceSlice (P11) but lists only P3, V4.
- I4 edits fast_mesher/carriers/ and consumes P4's T1 checks but lacks P4.
- I7 consumes P19's key set but lacks P19. I8 consumes I7 but lacks I7.
- V8 consumes P8 and P21 outputs but has only I5.
- P22's watch render needs P15 and P16, not on its chain.
- Fix: add these edges and re-derive wave snapshots (P2 before P4; P3 before P5, or move estimate to P7).

**T2.5 Critical path has a false edge that serialises two units; milestones understate prerequisites (M7-plan.md:1425-1434, 1451, 1535, 1547-1549).** class: plan
- Wrong: diagram P8 → P10, but P10 depends on P7, P9, V2, not P8; slot A then runs P8 and P10 serially on the critical path. I3 → P8 edge not drawn, so "ingress runs with slack" is false (I3 on P8's critical path). MS1 transitively needs I2, I3 (through P8) but omits them. MS3 needs oracles 7, 8, 9 → V7 (after P20), V8 (after I5, P8, P21), P28; none in MS3's list or diagram.
- Fix: path P7 → {P8, P10, P11} → P12; P8 in another slot, after I3; list I2, I3 under MS1; add P20, P21, V7, V8, P28 as MS3 prerequisites. (Overlaps T1.2.)

**T2.6 Wave-1 hand-offs and "disjoint" shared edits collide (M7-plan.md:138-145, 1443-1447; P1 plan L68; V1 plan L47, 66; I1 plan L64, 70, 113).** class: plan
- Driver script: scripts/check_native_face_domain.py hard-codes units in three places (choices :149, module map :114, lint() if/else :53-66, which lints native/face_domain for any unknown unit); P1 (twice), I1 and V1 all edit the same lines; "one --unit row each" is false.
- Adapter: in native/step_source/main.cpp, I1's timers wrap :409 and :1263-1274, which P1 Tasks 6 and 2 rewrite; both add options to the single option loop :398-403; P1 Task 2 edits I3's region (:941) and P4's (:304-322); every later anchor shifts after the first merge.
- Hand-offs: V1's CMake adds source_store at Task 1 though it consumes nothing until Task 6, so V1 fails to configure until P1 Task 1 lands; V1 Tasks 6 and 7 need P1 Task 2 (Python v3 writer, adapter --store-version), not only Task 3; "rebase onto P1's branch" pulls P1's unreviewed commits into I1 and V1.
- Fix: P1 Task 1 turns the driver into a unit table once, I1 and V1 each add one row. Land P1 Tasks 1–3 as an early reviewed merge, I1 and V1 cut from it. I1's timers land after P1 Tasks 2 and 6. Anchor shared edits by function name, not line.

**T2.7 Interface names collide; several shared types have no owner (M7-plan.md:319, 344, 346, 348, 317, 441, 661, 685-686, 1624-1629).** class: plan
- bgfm::Profile is P2's return type (profile_brepgpu_pdq_1()) and P3's sizing-profile enum. FaceOutput mesh_face(const FaceInput&) produced by both P14 and P15 (ODR violation).
- Unowned: Vec2, Vec3, Box3, Interval, FlagSet, EntityKey, FaceInput, OccurrenceFormatStatus, CurveRecord (only in P4's evaluate signature), EdgeSlot/FaceSlot index types. §13's "each produced by exactly one unit" is false.
- Fix: P1 owns a bgfm core-types header; rename P2's type (RecordProfile); P14 owns one dispatching mesh_face with P15 registering its generators.

**T2.8 Refusal-code vocabulary incomplete; points at a code that does not exist (M7-plan.md:282, 503-504; P1 plan L76).** class: plan
- P8's orient returns "refusal reason 7" (from design §5.3); no code defines reason 7; in P1's append-only bgsrc::Reason value 7 is coincident_source_entities. Homeless codes: offset_singular, not_a_brep, semantics_unavailable, illegal_edge_geometry, no_surface_geometry (§5.4); nonfinite_evaluation (§4.1); lattice_floor, projection_failed (§4.7); move_bound_exceeded (§8.1). P1's FaceRefusal carries only bgsrc::Reason. Enum frozen in wave 1 and consumed by I1.
- Fix: add a named orientation-undecided code; define the whole code list in P1, or state the split between ingress Reason and producer StopName/records and which one face status carries. (Overlaps T1.11.)

**T2.9 P1's Python version 3 cannot be written through the validator it sits behind (P1 plan L146-158, 187; M7-plan.md:51-58).** class: plan
- write_source calls validate_source first (imported_source.py:780-781), which rejects carrier kinds 7, 8 (:472), curve kinds 6, 7 (:493), an absent pcurve (looks up source.curve(0); rejects map kind 0, :421). Task 2's round-trip, Task 3's fixture and V1's fixtures fail however implemented. v2 COEDGES already carries `reconstructed` (written :735); v3's pcurve_presence duplicates it with no linking rule.
- Fix: Task 2 widens validation per version and defines reconstructed ≡ (presence == 2), or drops reconstructed in v3.

**T2.10 P1's L1 determinism test fails by construction (P1 plan L347, 354, 368; M7-plan.md:92-98).** class: plan
- L1Definition.faces/edges/vertices/loops/coedges are store slots (row order), not declared identity columns; the shuffled twin changes them, so renumber_gives_identical_l1 fails. Duplicate-vertex ties fall back to "store order", contradicting call 8 and design §4.2 mechanism 10.
- Fix: declare the slot vectors identity columns, or keep only definition-local indices; one identifier-free tie-break.

**T2.11 I1 Task 1's command does not run against the adapter; its phases don't cover the whole run (I1 plan L70, 93-94).** class: plan
- The adapter takes INPUT and OUTPUT as argv[1..2], then options in pairs (main.cpp:398-403); `--phase-times <input> out/...` puts the flag first, and a flag with no value throws "source option missing value". The eight phases miss :582-626 (product/solid binding, styles), :974-1068 (translated-wire comparison), :1195-1220 (orientation records, FACES rows), :1253-1262 (carrier rows through surface()); the sum-of-phases test fails and budget proportions omit this work.
- Fix: valued option after the positionals, or change the parser; assign every region to a phase or add "other".

**T2.12 P1 reader's behaviour on inconsistent rows undecided; spec pointer wrong (P1 plan L15, 220, 251).** class: plan
- Points to native/mesher/source.cpp:405-470 as "the structural checks"; those are helpers (index_rows, controls, knot checks, make_csr); real ownership/status checks are validate() at :482-854. Only non-finite values and dangling references are defined; unspecified: face missing from its definition, loop/coedge owner mismatch, admitted face holding a refused loop — file fault or malformed_record?
- Fix: cite :482-854 and give each check class its outcome.

### Bead findings
- T2.b1 `.25.35` missing from the bead map; map it to P18/MS1, and I7 and P22 avoid these fixtures as old-path controls.
- T2.b2 P1 Task 3 asserts vertex 302's tolerance == .0003 but the fixture writes .0001*3 = 0.00030000000000000003.
- T2.b3 P1 Task 3 "adds" coedge 503, which already exists in native_mesher_source_fixture.py (500–503, curve 703, map kind 3).
- T2.b4 validate_source caps B-spline surface degree at 3 and refuses reflected placements, so P5's degree 4–25 families and P19's mirror families can't be written through the call-10 writer.
- T2.b5 I1 Task 4: substituted, appended, with_unit are in tests/test_step_ingress.py:38/553/557, moved/turned at test_step_reader_raw.py:312/457; test classes sit among the builders after :811; test_step_ingress.py missing from the file list.
- T2.b6 I1 Task 3: sort of records with duplicate ids must be stable (or keyed by id then offset) to keep the table D0.
- T2.b7 I1 Task 5: _OriginalValues accepts .5 and D exponents and has no binary-token case; state which differences are the reference's laxity rather than owned defects.
- T2.b8 I1 Task 4 cites "tagged-table layout of master plan §5"; that §5 is the code-layout table.
- T2.b9 I1 Task 1: NVIDIA ingress requires a new output directory per run; n = 3 reuses one path.
- T2.b10 P1 Task 7 and V1 Task 7: no step produces the version-3 stores of the watch and the admitted corpus the censuses read.
- T2.b11 P1 Task 1, I1 Task 2, V1 Task 1: red step expects "header not found" but CMakeLists and driver row are created only in Step 3.
- T2.b12 V1 SkippedFace.reason is a string_view while the store hands it a bgsrc::Reason.
- T2.b13 I6's identity(const EntityTable&) can't see the §5.7 header strings, which live in ParseResult.header.
- T2.b14 I7 cites benchmark_om10_nvidia.py lines "before B3's changes land"; B3 is editing those now; re-anchor in I7's unit plan.

## Reviewer 3: fleet, measurement and the arena

### Plan findings

**T3.1 The first arena (I9) cannot run the 2^(8−2) fraction it promises (M7-plan.md:229, 1187-1190, 1485, 232-233, 241-242).** class: plan (= T1.1)
- Wrong: F2 (P30) and F5 (P31) start only after I9; F7 (P24) and F8 (P25) are not I9 dependencies.
- Evidence: design §23.2 L2840-2846 generates F7 = F1·F2·F3·F4, F8 = F1·F2·F5·F6; all eight flags must switch in every one of the 64 runs; a fraction over six flags is a different design with a different resolution.
- Fix: I9 delivers the runner plus a declared sub-design over flags whose both levels exist, labelled "admitted set at T", stating its own resolution; the full fraction runs only after P24, P25, P30, P31; §9 rows and I9 done criterion say so.

**T3.2 MS3 does not carry the decided GPU gating (M7-plan.md:1547-1552, 1433, 86-89).** class: plan
- Wrong: the decision (CPU-only first headline; GPU units gate MS3) is not in the plan text; MS3 lists neither P24 nor P25 nor the per-lane F8 arena; requires oracles 1–10 and 14–17, omitting oracle 12 (FI-1, FI-2 across backends) and oracle 11 under GPU fixed placement.
- Evidence: V9 done criterion "12 once P24 lands" (L1403); nothing requires it before MS3.
- Fix: add P24, P25, the per-lane F8 run, oracle 12, and oracle 11 under GPU placement on each GPU to MS3 and the §7.2 path; call 7 states the GPU units gate MS3.

**T3.3 Nothing keeps the fleet's measurement lanes free while three units build and test on the same host (M7-plan.md:78-85, 1464-1478).** class: plan (operator part decided above)
- Wrong: wave 1 runs I1's timed runs on both nodes, P1 and V1 censuses on node 1, and three agents' builds and tests at once; no lane reservation; other lane's state not recorded.
- Evidence: scripts/measurement_lane.py:46-52, 311-317 quiet rule host-wide (aggregate /proc/stat minus our tree, 150% limit): a concurrent single-threaded run on the other socket passes as quiet; a unit's build marks the run loaded. scripts/check_native_face_domain.py:120 builds --parallel 4, unpinned. Design §22.4 items 2–4 and §2 require a per-lane gate with other-lane state as a recorded blocking factor. The §9 "run early" tasks (P11 Task 1 L1477, I8 Task 1 L1478) have no one to run them; wave 2 is {P4, P2, I2} under the cap.
- Fix: fleet lane discipline [call]: per-lane exclusive lock taken by every timed run, blocking builds/tests or confining them to CPUs outside a held lane; other-lane-state column; coordinator named as executor of the early runs, and how they count against the cap stated.

**T3.4 The ingress gate MS2 relies on is measured before its protocol exists (M7-plan.md:1105-1108, 1453, 1542, 1473 vs 1478; plans/I1-step-parse.md:91-99).** class: plan
- Wrong: I1 sets the budget from n = 3 medians under the old host-wide gate before the idle noise floor is measured; I5 declared done ("gate holds on both lanes") before I8's protocol exists; "cold" runs check no filesystem type.
- Evidence: design §22.4 item 1 (≥ 10 fresh processes for headline numbers), item 4 (per-lane gate with measured threshold); §22.3 refuses STEP or .brep on tmpfs for cold runs; measurement_lane.py has no tmpfs check, no filesystem type, no per-lane gate.
- Fix: I1's budget labelled provisional; the gate verdict for MS2 comes from MS2's own per-lane alternating series (owned ingress vs NVIDIA minimal ingress, ≥ 10 each, under I8's protocol); idle noise floor measured before I1 Task 1's timings, or I1's timings re-run after it.

**T3.5 MS2 never requires the oracles to decide the owned watch output (M7-plan.md:1541-1546; cf. MS1 1538-1539).** class: plan
- Wrong: V3's done criterion covers MS1's inputs and NVIDIA's watch output only (L1295-1296); V5, V6 cover only MS1 outputs; no unit requires a verdict on the owned watch mesh under the declared configuration in either profile.
- Evidence: design §22.5 only publishable row is owned "valid, achieved"; §22.2 owned side measured by the same oracles; §24 not_evaluated never green.
- Fix: MS2 requires, on the owned watch output in both profiles under the declared configuration, oracles 1–6, 10, 16, 17 to pass, and one output digest across every process of the series.

**T3.6 The existing quiet settle makes planned run counts and the arena time box infeasible (plans/I1-step-parse.md:91-95, 325-327; M7-plan.md:1171-1190).** class: plan
- Wrong: every measurement_lane.py run idles ≥ 120 s before timing.
- Evidence: measurement_lane.py:883-885 sets 3 checks 60 s apart and up to 3 tries; settle() at :521-528 sleeps between checks. I1 Tasks 1 and 5 ≈ 11,000 runs (≈ 233 inputs × 2 lanes × n = 3 over 3 commands in Task 1 and 5 thread counts in Task 5) ≈ 370 h of idle settle. Lanes cannot overlap without marking each other loaded (T3.3). Design §23.6 boxes the first arena in hours; at ≤ 30 runs/h/lane, 128 runs per input ≈ 4 h.
- Fix: I8 builds a continuous per-lane per-CPU monitor; §22.4's "minute before" becomes the tail of the series' own preceding run, counted as ours, not idle sleep; the arena uses it; I1 measures the watch and a seeded per-stratum sample at n = 3, the rest at n = 1.

**T3.7 Nothing scheduled produces the store set and admitted list the wave-1 fleet runs read (plans/P1-source-view.md:448, 470-472; plans/V1-oracle-geometry.md:420-422; plans/I1-step-parse.md:325; M7-plan.md:59-63).** class: plan
- Wrong: no task extracts the watch and corpus at --store-version 3 or writes the admitted list; I2, I3, P4 later change the adapter's v3 output, so stores go stale.
- Evidence: P1 Task 2 tests v3 on void-block.step only; no plan names a producer for these stores.
- Fix: coordinator fleet task after P1 Task 2: extract every file at v3; record admitted list, adapter binary digest, each store's digest; re-extract after each unit that changes the adapter lands; every census names the digest of the store set it read.

**T3.8 F7's FP32 level has no CPU implementation, but the fraction runs FP32 with the GPU off (M7-plan.md:871-894, 373-381).** class: plan (operator part decided above)
- Wrong: no unit builds CPU FP32 evaluation.
- Evidence: in §23.2 F7 and F8 are separate generators (F7·F8 = F3F4F5F6), so all four (F7, F8) combinations appear 16 times each; §21.4 states FP32 as a general arm.
- Fix: P4 or P24 builds a recentred FP32 CPU evaluation path (decided: with P24).

**T3.9 The GPU measurements come before the GPU context row exists (M7-plan.md:232-233, 889-891, 911-913, 1155-1158).** class: plan
- Wrong: GPU state required by §22.4 items 5 and 7, and the reporting tool refusing mixed link widths, arrive with I8; P24, P25 do not depend on I8.
- Evidence: §22.4 items 5, 7 (SM clock, throttle reasons, PCIe gen/width sampled during the run; refusing tool); measurement_lane.py:448-460 captures only index, utilisation, memory used and SM clock, only before and after.
- Fix: P24's and P25's fleet runs depend on I8's context row and reporting tool, or the GPU half of the context row moves into P24's first task.

**T3.10 The "after MS1" measurements run on inputs MS1 cannot mesh (M7-plan.md:1484, 1413, 1432).** class: plan
- Wrong: MS1 has no torus, revolution, B-spline faces (P15), no collapse (P17), no placements (P19), so the watch and most of the corpus cannot mesh at MS1; same for move_bound_exceeded and S2-reversal counts (§27 #27).
- Evidence: MS1 after P1–P14 and P18 (L1535); §22.6 budget checked per report class, never on the watch alone.
- Fix: split: after MS1, analytic-only report class; after P15, P16, P17, P19, every report class and the watch, before MS2's configuration is declared — the revision MS2 uses.

**T3.11 Several design §27 open questions have no run, though plan §13 says they all do (M7-plan.md:1620-1622; 483, 559, 1483).** class: plan
- Missing: #30 (G15 H5 gaps vs u_ctx); #31 (G15 H4 c_max; production-exports stratum used at MS3); #33 (G9 H4 face-over-solid count); #37 (G14 angle-term census, which MS2's Render arm relies on); #38 (G14 slicer and fTetWild measurements behind "no grading"); #39 (G5 H1, H2); #40 (offset census); #44 face budget 16× (refusals score as owned losses), cap repair 2, fuzz timeout 8×, Δκ floor 8×, m_α 10×.
- Evidence: design L3204-3214, 3218; the plan has no G15, G14, G9 H4 or G5 H entries.
- Fix: #37 → P5 or P14; #38 → P20 with V7; #39 → P4; #30 → P10 after I2; #31 → P28; #33 → P19; #40 → I4; #44 distributions → P11, P12, P14/P15, P16 and I11, before MS2.

**T3.12 The I7, I8 and I9 unit blocks leave out protocol items the design requires (M7-plan.md:1130-1190).** class: plan
- I8/I7 missing: cold rule (empty cache directory verified per row, page drop both sides, tmpfs refusal, filesystem type recorded, §22.3); warm rule; separate meshing-only stage-ratio run (§22.3); environment padding and seeded interleaving (§22.4 item 2); timer overhead below 5% (§22.4 item 6); NVIDIA ideal-overlap bound max(ingress, makespan) until streamed and dynamic-pull arms exist (§22.2); exclusion tables .56 and .61 (§22.5).
- I9 missing: hours time box and primary response named before the first run (§23.6); every default and claim with and without the watch (§23.4); requests per input fixed before any result (0.005, 0.025 mm, 1e-3, 1e-4 of the diagonal; PMI-bearing inputs absolute only; §23.4); never two arena runs on one lane at once (§22.4 item 3).
- Fix: list each item in its block.

### Bead findings
- T3.b1 plans/I1-step-parse.md:92 puts --phase-times before the input and output arguments; the adapter reads argv[1] input, argv[2] output, options from argv[3] (native/step_source/main.cpp:401, 408, 1274). (= T2.11)
- T3.b2 plans/P1-source-view.md:471 and plans/V1-oracle-geometry.md:421 run censuses without --ingress-file, so page-cache state is not_managed and per-MB read times mix warm and cold; add --ingress-file or label warm.
- T3.b3 scripts/measurement_lane.py:226-230 sets no CUDA_VISIBLE_DEVICES for a run given only --node, so a CPU-only run sees both GPUs; for the CPU-only declared configuration hide devices or record none was touched.
- T3.b4 M7-plan.md:1476 the wave-2 NVIDIA watch run names no arm set; harness keeps one representative per arm per invocation (benchmark_om10_nvidia.py:671-673); state the §22.2 screen (every arm at n = 1, then best k = 3 at n ≥ 10, per lane) and the driver repeating invocations.
- T3.b5 the stage-budget revision (M7-plan.md:1413) should state the declared configuration's worker count: MS2's CPU-only configuration has 12 workers (§20.1 item 2) while §22.6's target is figured for 11.
- T3.b6 V9 block (M7-plan.md:1401-1403) should add O-bytes under fixed GPU placement on each GPU separately (§24.2), beside the GPU0-vs-GPU1 comparison.
