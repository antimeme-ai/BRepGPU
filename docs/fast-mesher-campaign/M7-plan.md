# M7 plan: the fast mesher in units

Status: M7 master plan of the fast-mesher campaign
([PLAN.md](PLAN.md) M7, bead `brepgpu-12s.23`), written from
[FAST_MESHER_DESIGN.md](../FAST_MESHER_DESIGN.md) as it stood after G2 (passed
2026-10-19). M8 executes it. The first wave's unit plans are in
[plans/](plans/); every later unit gets its unit plan just in time, from its
block below, in the same format.

## 0. Reading this plan

- **The design is the spec.** Section numbers (§n) are the design's. This plan
  decides only what the design leaves open: the units, their order, the
  interfaces between them, and the builder's choices of §29, each marked
  **[call]**.
- **A unit** is one conceptual unit (BLACKBIRD: delegation units are
  conceptual units). Its block names its purpose, design sections, beads,
  files, interfaces, the oracle tier of its red tests, the M8 measurements it
  needs first, its fleet runs and its done criteria.
- **No bite-sized steps here.** They belong in the unit plans.
- **Unit names** are `P` (producer: the mesher itself), `I` (ingress,
  measurement and the arena) and `V` (verification: the offline oracles of
  §24), plus the in-flight `B3`.

## 1. What M8 delivers

The owned mesher of the design, from STEP bytes to written meshes, faster than
a steel-manned NVIDIA USD/BRep on the real-input strata, with the oracles of
§24 checking every output, and the old mesher retired (§25). The path that
matters runs through three milestones (§11): the first owned end-to-end mesh
of a corpus part checked by the oracles, the first steel-manned headline on
the watch's own stratum, and the first corpus-stratum claim.

## 2. Calls this plan makes

Choices the design leaves to the builder (§29) or does not settle. Each is open
to override; each unit that carries one repeats it.

1. **[call] Code layout and namespaces** (§5 of this plan). The new mesher is a
   new standalone CMake project, `native/fast_mesher` (namespace `bgfm`). The
   source store moves to its own project, `native/source_store` (`bgsrc`,
   with `bgtt` for the tagged-table container and `bgsha` for SHA-256). The
   owned STEP reader is `native/step_reader` (`bgstep`). The offline oracles
   are `tests/oracles` (`bgor`), a project that links nothing from
   `native/fast_mesher`.
2. **[call] One new source-store reader; the old one stays with the old
   mesher.** `native/mesher/source.cpp` throws on the first fault and charges
   run-wide atomic metering (§25 removes both). `native/source_store` is a new
   C++20 reader that refuses per entity (§5.8), written from the old one as
   its specification. The old reader leaves with the old mesher (P23).
3. **[call] Source store version 3, defined once.** The design's new path
   needs fields BGSRC001 version 2 cannot hold (§12 of this plan, gap G-1).
   P1 defines version 3 whole: per-coedge pcurve presence and declared
   master, swept carriers, hyperbola and parabola curves, carrier flags, and a
   CONTEXTS table of per-context units and uncertainty. P1 changes the
   adapter's writer (`native/step_source/main.cpp:1273`), the new C++ reader
   and the Python reader and writer (`python/brepgpu/imported_source.py`)
   together. The adapter writes version 3 only under `--store-version 3`, with
   fields it does not yet know at their stated defaults; version 2 stays the
   default for the old mesher's reader until P23 retires both. Later units
   fill version 3's fields in the adapter without changing the format: I2 the
   contexts, I3 pcurve presence and masters, P4 the swept kinds, conics and
   torus flags.
4. **[call] Oracles are native C++.** Oracle 1 alone samples every triangle of
   every output against the trimmed target; the corpus is about 840 MB of STEP
   and the arena multiplies it by configurations and requests. Python stays
   the orchestration and fixture language. The oracle project shares only the
   source-store reader and the tagged-table I/O with the producer (both are
   input decoding, checked by oracle 8); no evaluator, projection, predicate
   or bound is shared.
5. **[call] Oracle arithmetic.** Oracles compute in binary64, carry a lower
   and an upper bound for every distance they report, and inflate each by a
   stated forward-error allowance (2⁻⁴⁰ of the magnitude bound of the data,
   which covers de Casteljau to degree 25 with margin). An oracle passes when
   the upper bound meets the threshold, fails when the lower bound breaks it,
   and otherwise refines; out of budget it returns `not_evaluated` with reason
   `budget` (§24 outcomes).
6. **[call] Execution.** Subagent-driven, one unit per agent, at most three
   agents at once (operator cap, journal 2026-10-19), one independent review
   per unit, findings fixed, then the unit lands (standing rule, journal
   2026-10-19). Each unit runs in its own git worktree on branch
   `fm/<unit-id>-<slug>` cut from the campaign branch, commits per task there,
   and the coordinator merges it after review. Builds and tests run on the
   fleet in a lane directory `lanes/<unit-id>/`, synced with
   `git ls-files | rsync --files-from` (the B2 and B3 convention).
7. **[call] The first headline is CPU-only.** §22.3 asks for one declared
   configuration. The GPU path (P24, P25) lands after the CPU pipeline; the
   first headline declares `gpu` (F8) off, and the GPU arm is its own declared
   configuration once its arena (F8, per lane) has run.
8. **[call] Coincident content.** P1 detects entities with identical content
   digests. Duplicate faces and duplicate edges inside one definition refuse
   with `coincident_source_entities` (§5.8). Two vertex records at one point
   are reported (a finding), not refused, since touching shells inside one
   definition are legal and meshed (§3.7, §3.11); tie-breaks between such
   entities fall back to an identifier-free second key (their output bytes).
9. **[call] L1 order.** L1 arrays are ordered by content digest, never by
   `#id` or store order; identity lives in parallel columns (§4.4). Renumbering
   a source then leaves every L1 byte outside the identity columns unchanged.
10. **[call] Native test pattern.** Native tests are C++ executables
    `tests/native_<area>_*.cpp` registered with CTest in their project, with
    fixtures authored by Python generators through
    `python/brepgpu/imported_source.py` (source stores) or
    `tests/step_families.py` (STEP text, extracted by I1 from
    `tests/test_step_reader_raw.py:811`). Fleet drivers are `unittest` modules
    `tests/test_<area>.py` that find binaries through `BREPGPU_*` environment
    variables (the `tests/test_step_ingress.py:45` pattern).
    `scripts/check_native_face_domain.py` gains one `--unit` per new project.
11. **[call] Commits.** Every commit ends with
    `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`, the line this
    repository's history uses.
12. **[call] What an oracle unit's red tests are.** The design says nothing
    tests an oracle separately and hand-made defective outputs wait for
    mutation testing (§24, §24.4); JSMNTL asks every build unit for red tests.
    An oracle unit's tests are spec checks of its own geometric primitives
    (evaluation, distance, membership) against closed-form values, the
    contract of that computation; no oracle unit mutates producer output to
    see itself fail. Its fail branches are reached by real defective outputs
    (NVIDIA's recorded invalid meshes, arena arms that drop a guarantee),
    exactly as §24 says.

## 3. Rules every unit follows

- **JSMNTL per unit** (BLACKBIRD): its written sub-plan (the unit plan, with
  the literature or reference implementation each mechanism follows, by
  revision and function or paper section), red tests at the oracle tier named
  in its block, code, green, one independent review, fixes, land.
- **Red tests first, at the stated tier.** A test that only checks "no crash"
  does not count as a unit's red test.
- **Fixtures are small and authored.** OM10 and corpus bytes are never copied
  into tests. Fleet runs read them in place from the fleet's `corpus/` and the
  watch's location, by environment variable.
- **No overfitting.** Nothing keyed to OM10 IDs, names, sizes or counts. A
  measurement on the watch is reported in the watch's own stratum.
- **Bugs found are fixed in the phase that finds them and filed as beads.**
- **Arena flags.** A unit that builds a mechanism with an arena flag (§23.2,
  §23.3) builds both levels of that flag, and its tests cover both. Flags are
  recorded in `RUN_____` (§7.3).
- **Shared edit points.** Two units that must edit one file in the same wave
  edit disjoint regions, and the coordinator merges in landing order:
  `scripts/check_native_face_domain.py` (one `--unit` row each);
  `native/step_source/main.cpp` (I1 the phase timers, P1 the writer at `:1273`,
  I2 the units code at `:426-551`, I3 the pcurve and representation code at
  `:666` and `:941`, P4 `surface()` and `curve3`/`curve2` at `:271-322`, P18
  nothing, P19 the mapped-item refusals at `:425`, `:477`);
  `python/brepgpu/imported_source.py` (P1 only).

## 4. Global constraints (copied into every unit plan)

- Toolchain:
  - C++20 on the pinned Clang 18.1.3 with libstdc++;
  - CUDA 12.8, `sm_60`, SASS only;
  - new code stays C++26-lane-clean, and device-visible headers stay C++20.
- **No new libraries.** Any need for one becomes a proposal for the operator.
  Pinned OCCT 8.0.1 is the existing bridge.
- **No JSON for internal data:** use the tagged-table binary container. TSV and
  JSON appear only at presentation, on request.
- **Fleet only:** every build, test, render and benchmark runs on the fleet; the
  laptop edits and reads git. Every GPU test runs on both GPUs. Transfer figures
  are never compared across GPU0 (x8) and GPU1 (x16).
- **Original bytes stay private:** OM10 and corpus bytes are never copied into
  tests. Fixtures are small and authored.
- **No overfitting:** nothing keyed to OM10 IDs, names, sizes or counts.
- **Bounded review:** each unit gets one independent review, its findings fixed,
  then it lands. Edge cases go to beads.
- **Bugs:** fix every bug in the phase it is found, and file a bead.
- Mutation testing waits until the verification stack settles.
- **Privacy:** no hostnames, addresses, absolute paths or personal names in
  tracked files. Commits end with the Co-Authored-By line used in this
  repository.

## 5. Code layout

| Path | Project | Namespace | Holds |
|---|---|---|---|
| `native/source_store/` | `brepgpu_source_store` | `bgsrc`, `bgtt`, `bgsha` | tagged-table container, streaming SHA-256, §5.8 reason codes, the BGSRC reader |
| `native/step_reader/` | `brepgpu_step_reader` | `bgstep` | owned Part 21 parse (I1), owned decode (I5), identity side tables (I6) |
| `native/fast_mesher/` | `brepgpu_fast_mesher` | `bgfm` | the mesher: `l1/`, `records/`, `request/`, `carriers/`, `bounds/`, `runtime/`, `measure/`, `charts/`, `lfs/`, `reconcile/`, `clearance/`, `closure/`, `band/`, `faces/`, `shading/`, `collapse/`, `publish/`, `place/`, `export/`, `cache/`, `gpu/`; the CLI `brepgpu-fm` |
| `tests/oracles/` | `brepgpu_oracles` | `bgor` | the offline oracles of §24 and their geometry kernel |
| `tests/` | — | — | `native_*.cpp` tests, `*_fixture.py` generators, `test_*.py` fleet drivers, `step_families.py` |
| `scripts/` | — | — | fleet measurement, NVIDIA harness, arena runner |

Every C++ target uses `brepgpu_native_cpu_rigor` from
`tooling/native_cpu_rigor.cmake`. CUDA targets pin the toolkit (code defect
D-1, §12) and build `60-real`.

## 6. Units

### 6.1 Overview

| Unit | Name | Design | Beads | Lane start after |
|---|---|---|---|---|
| P1 | source-view | §4.2 L1, §4.4, §3.5, §5.4 row 4, §16.2, §19.1 | — | — (wave 1) |
| V1 | oracle-geometry | §24.1 trim domain, §24 independence | — | — (wave 1; its Task 6 needs P1 Task 3) |
| I1 | step-parse | §5.6, §5.7 header | `.25.7` (with I5) | — (wave 1; its Task 3 needs P1 Task 1) |
| P2 | records-status | §7.1–§7.5, §4.7 stop names | — | P1 |
| P3 | request | §3.1, §6.1–§6.2, §16.2 request, §18.1 R_max, δ_exp | — | P2 |
| P4 | carriers | §4.6 Carrier, §5.4 rows 1–3, 9–10, 12–21, §5.8 geometry, §21.2 | `.25.10`, `.25.14` | P1 |
| V2 | oracle-reconciliation | §24.1 row 10 | — | V1 |
| P5 | span-bounds | §13.1 bounds, §6.2 angle terms, §10.3 curve rule, §21.1 | `.25.21` | P4 |
| P6 | task-runtime | §4.1, §4.2 mechanisms, §4.5, §4.7, §4.8, §20.1 items 1–2 and 8, §20.5 | — | P2 |
| I2 | units | §5.5 UR-1–UR-10, `.64`, `.65` | `.25.6`, `.25.15` | P1, I1 |
| I3 | raw-authority | §5.1, §5.2, §5.4 row 11, §16.1 raw placements | `.25.5` | P1, I2 |
| P7 | edge-measure | §8.1, §8.2 prior, §10.2 initial sampling, §7.4 runtime gap rows | `.25.30` | P3, P5, P6 |
| V3 | oracle-deviation | §24.1 row 1, §3.2 target, §3.3 | — | V2 |
| P9 | feature-size | §8.8, §19.3 regions, §9 candidates | `.25.29` | P5 |
| P8 | face-charts | §5.3 orientation S1 (T2o), §10.5, gate lifts | `.25.13`, `.25.12` | P7, I3 |
| P10 | reconcile | §8.2–§8.7, §3.2 resolved edge | — | P7, P9, V2 |
| P11 | clearance | §12 | — | P7, P9 |
| V4 | oracle-structure | §24.1 rows 2, 3, 4, 17 | `.25.32` | V1 |
| P12 | closure | §10, §11 | `.25.22`, `.25.23`, `.25.31` | P8, P10, P11, V4 |
| P13 | band-cdt | §13.2, §13.3, §21.3, §18.1 C1 | `.25.24` | P3, V4 |
| P14 | faces-analytic | §13.1 rows 1–4, §3.4, §3.6 | — | P12, P13, V3 |
| P18 | publish | §3.8, §4.4, §5.3 S2, §7.2, §17.1–§17.2 | `.37.14` | P14 |
| I4 | refusal-catalog | §5.8, §5.3 rlimits, §7.2 exit 4 | `.25.1` | P2, I3 |
| P15 | faces-lattice | §13.1 freeform rows, §13.4 | — | P13, P12 |
| P16 | shading | §3.10, §15.1, §15.2 | — | P14 |
| V5 | oracle-embedding | §24.1 row 5 | — | P18 |
| V6 | oracle-shading | §24.1 rows 6, 16 | — | P16 |
| P17 | collapse | §9 | — | P12, P9 |
| I5 | step-decode | §5.6, §5.1–§5.5 on the owned path | `.25.7` | I1, I2, I3, I4 |
| P19 | placements | §16 | `.25.4`, `.25.11` | P18 |
| I6 | identity | §5.7, §17.3 identity rows | `.25.33` | I1, P18 |
| V8 | oracle-ingress | §24.1 rows 8 | `.25.26` | I5 |
| P20 | export | §18, §17.3 | — | P19 |
| V7 | oracle-export | §24.1 row 7 | — | P20 |
| V9 | oracle-metamorphic | §24.1 rows 11, 12, 14, 15; §24.2 | — | P18, P6 |
| I7 | nvidia-harness | §22.2 | `.25.20` | V3, V6, B3 |
| I8 | headline-protocol | §22.3–§22.5 | `.25.9` | I5, P18, B3 |
| I9 | arena-runner | §23.2, §23.4, §23.6 | `.25.27` | I8 |
| I10 | family-catalogue | §23.5 families | — | I9 |
| I11 | fuzzer | §23.5 fuzzer, §4.8 timeout | — | I10 |
| P24 | gpu-kernels | §20.1 items 4–7, §21.1, §21.4 | `.25.2` | P15 |
| P25 | hetero-placement | §20.1 item 3, §20.2–§20.4, §20.6 | `.25.28` | P24 |
| P21 | native-source-record | §5.4 row 28 | — | P18 |
| P22 | consumers | §25 retired list | — | P19, P20, P21 |
| P23 | retire | §25 | `.25.3` | P22 |
| P26 | cache-incremental | §19, §24.1 row 13, §24.3 | — | P18, V9 |
| P27 | lod | §15.3 | `.25.25` | P26 |
| P28 | pmi | §6.3, §6.4, §24.1 row 9 | — | I1, I6 |
| P29 | postprocess-arm | §14 | — | P16, I9 |
| P30 | metric-cdt-arm | §23.2 F2 | — | P15, I9 |
| P31 | healing-classes-arm | §23.2 F5 | — | P10, I9 |
| B3 | harness-scoring (in flight) | §22.1, §22.5 | `.25.16`–`.25.19` | running |

The list covers every design section: §0–§2 (constraints, carried by every
unit), §3 (P1, P3, P10, P14, P16, P18), §4 (P1, P2, P6), §5 (I1–I6, P4, P8,
P21), §6 (P3, P5, P28), §7 (P2), §8 (P7, P9, P10), §9 (P17), §10–§11 (P7, P12),
§12 (P11), §13 (P13–P15, P30), §14 (P29), §15 (P16, P27), §16 (P19), §17 (P18,
I6, P20), §18 (P20), §19 (P26), §20–§21 (P6, P24, P25), §22 (B3, I7, I8), §23
(I9–I11, P29–P31 and each mechanism's flags), §24 (V1–V9, P26, P28), §25 (P21–
P23), §26 (reserved interfaces built in P3, P18, P20; physics deferred), §27
(each open question is settled by the unit or run named in its row), §28
(carried in the sections), §29 (calls above).

### 6.2 Producer units

#### P1 source-view

- **Purpose.** The new path's source input: store version 3 (call 3), one
  owned reader of it for every new consumer, refusing per entity; the
  mesher's L1 layer (decoded definitions, topology, use counts, shell
  closure); content digests for keys and tie-breaks; one streaming SHA-256
  and one tagged-table I/O.
- **Design.** §4.2 (L1, D0), §4.4, §4.6 Carrier fields, §3.5, §3.11, §5.4
  rows 4, 9, 10, 12, 13, 15, §5.5 UR-1 and UR-7 (what a context records), §5.8
  rows `nonfinite_value`, `malformed_record`, `coincident_source_entities`,
  §7.3 container, §16.2 content identity, §19.1 hashing in place.
- **Beads.** Contributes the reader layer of `.25.1` (closed by I4).
- **Files.** Create `native/source_store/` (`CMakeLists.txt`, `sha256.hpp`,
  `tagged_table.{hpp,cpp}`, `reasons.hpp`, `store.{hpp,cpp}`),
  `native/fast_mesher/` (`CMakeLists.txt`, `l1/digest.{hpp,cpp}`,
  `l1/definition.{hpp,cpp}`, `cli/main.cpp`), tests
  `tests/native_source_store_*.cpp`, `tests/native_fm_l1_*.cpp`,
  `tests/native_fm_l1_fixture.py`, `tests/test_fm_l1.py`. Modify
  `python/brepgpu/imported_source.py` (version 3 beside version 2),
  `native/step_source/main.cpp` (`--store-version 3` at the writer, `:1273`;
  hash in place at `:229-252`, `:409`), `native/reference_source/main.cpp:119,210`
  (hash in place), `scripts/check_native_face_domain.py`.
- **Consumes.** Nothing new.
- **Produces.** `bgsha::Sha256`, `bgsha::Digest`; `bgtt::RowReader`,
  `bgtt::RowWriter`, `bgtt::split_sections`, `bgtt::write_sections`;
  `bgsrc::Reason` (every §5.8 code); `bgsrc::ReadResult read_store(std::span<const std::byte>)`
  with `bgsrc::Store` and `std::vector<bgsrc::EntityRefusal>`;
  `bgfm::DigestSet content_digests(const bgsrc::Store&)`,
  `bgfm::L1Model build_l1(const bgsrc::Store&, const DigestSet&)` with `bgfm::L1Definition`,
  `bgfm::EdgeUseClass`, `bgfm::ShellClass`, `bgfm::ContentDigest`.
- **Oracle tier.** Spec (FIPS 180-4 vectors; literal store fields); property
  (use counts against a brute-force count; closure on authored shells);
  metamorphic (renumbering and row shuffles leave digests and L1 bytes
  unchanged; one changed bit changes exactly its Merkle ancestors).
- **M8 first.** None.
- **Fleet runs.** L1 census over the watch's store and the admitted corpus
  stores: definitions, edge-use classes, open shells, duplicate groups, read
  and L1 time per MB (§22.6 T0 line).
- **Done when.** Unit plan [plans/P1-source-view.md](plans/P1-source-view.md)
  green on the fleet; the watch's store renumbered gives identical digests and
  L1 bytes; the census is in the unit report.

#### P2 records-status

- **Purpose.** Everything a task emits about its outcome: check outcomes, the
  status lattice and its roll-up, exit codes, the native record tables, the
  runtime rows of the profile `brepgpu-pdq-1`, and `brepgpu-fm report`.
- **Design.** §3.9, §7.1–§7.5, §4.7 stop names.
- **Beads.** None.
- **Files.** `native/fast_mesher/records/` (`outcome.hpp`, `status.{hpp,cpp}`,
  `tables.{hpp,cpp}`, `profile.{hpp,cpp}`, `report.cpp`); tests
  `tests/native_fm_records_*.cpp`; CLI subcommand `report`.
- **Consumes.** `bgsrc::Reason`, `bgtt`, `bgfm::ContentDigest`, `bgfm::L1Definition`.
- **Produces.** `enum class CheckOutcome : uint16_t {pass, fail, not_evaluated}`,
  `enum class NotEvaluated : uint16_t {prerequisite_failed, budget, unsupported, no_threshold, nonfinite}`,
  `enum class FaceStatus : uint16_t {valid, valid_goal_unmet, indeterminate, refused}`,
  `enum class PartStatus : uint16_t {valid, valid_goal_unmet, indeterminate, incomplete, refused}`,
  `enum class StopName : uint16_t` (every §4.7 row), `struct ConstructionEvent`,
  `FaceStatus face_status(std::span<const ConstructionEvent>)`,
  `PartStatus rollup(std::span<const FaceStatus>, bool definition_refused)`,
  `int exit_code(std::span<const OccurrenceFormatStatus>)`, `class RecordSink`
  (`run`, `criterion`, `measure`, `finding`, `action`, `status`, `summary`;
  sorted and written at run end as the §7.3 tables), `Profile profile_brepgpu_pdq_1()`.
- **Oracle tier.** Spec: every row of the §7.1 event table and the §7.2 exit
  table as a literal case; metamorphic: the roll-up is identical under every
  permutation of its multiset.
- **M8 first.** None.
- **Fleet runs.** None beyond its suite; record cost enters MS1's stage budget.
- **Done when.** Every §7.2 exit code and every §7.1 event row has its literal
  test; `report` renders text and, on request, TSV; the §7.4 runtime rows are
  data, read by no construction step.

#### P3 request

- **Purpose.** The caller's request validated before anything runs, resolved
  once per session, and turned into the per-key request fields that the mesh
  key and every budget read: ε, ε_def, the ε_f map, ε_ref, the R_max class,
  δ_exp, the sizing profile and its terms.
- **Design.** §3.1, §3.2 (ε_ref), §6.1, §6.2 (profiles, no grading, no minimum
  size), §16.2 request fields, §18.1 (R_max binade, δ_exp per key and format,
  the δ_exp > ε_f/2 rule), §26.1 I1 per-face map.
- **Beads.** None.
- **Files.** `native/fast_mesher/request/` (`request.{hpp,cpp}`,
  `profiles.{hpp,cpp}`, `reserve.{hpp,cpp}`); tests `tests/native_fm_request_*.cpp`.
- **Consumes.** `bgsrc::Reason`, `RecordSink`; a root box supplied by the
  caller (from P5's span boxes at graph build; call: until P19 lands, R_max
  comes from the definition's own extent under the identity placement).
- **Produces.** `struct RequestInput`, `struct KeyRequest {double eps_def, eps_ref; FaceEpsMap eps_f; int32_t r_max_binade; double delta_exp; Profile profile; FlagSet flags;}`,
  `ValidatedRequest validate(const RequestInput&)` (refuses `invalid_request`),
  `double resolve_relative(const RequestInput&, const Box3& root_box)`,
  `double delta_round(double R)` (the symbol δ(R)),
  `double delta_exp(const KeyRequest&, FormatSet)`, `enum class Profile : uint16_t {fabricate, render, analyse, compare, simulate}`.
- **Oracle tier.** Spec: δ(R) at the table's values (3.3 nm below 64 mm, 26 nm
  below 512 mm); the +100 m case (δ ≈ 6.8 µm > ε = 5 µm recentres STL and
  refuses the other formats); relative ε against an empty or unbounded box
  refuses by name.
- **M8 first.** None.
- **Fleet runs.** None beyond its suite.
- **Done when.** Every `invalid_request` case of §5.8 and every reserve rule of
  §18.1 has its literal test.

#### P4 carriers

- **Purpose.** T1: every admitted carrier and curve kind as an owned record
  with an evaluator (point and jets to second order), including the native
  swept evaluators and the torus variants, with T1's geometry refusals.
- **Design.** §4.6 Carrier record, §5.4 rows 1–3, 9, 10, 12–21, §5.8 geometry
  rows, §10.5 interior pole lines, §13.1 swept rules, §21.2 SIMD layer (first
  consumer).
- **Beads.** `.25.10` (revolution and extrusion), `.25.14` (torus flags).
- **Files.** `native/fast_mesher/carriers/` (`record.hpp`, `evaluate.{hpp,cpp}`,
  `curves.{hpp,cpp}`, `swept.{hpp,cpp}`, `simd.hpp`); tests
  `tests/native_fm_carriers_*.cpp`; the adapter fills version 3's swept kinds,
  conics and carrier flags in `native/step_source/main.cpp` `surface()`
  (`:304-322`) and `curve3`/`curve2` (`:271`, `:284`). The format is P1's.
- **Consumes.** `bgsrc::Store`, `bgfm::L1Definition`, `RecordSink`.
- **Produces.** `enum class CarrierKind : uint16_t {plane, cylinder, cone, sphere, torus, bspline, revolution, extrusion}`,
  `struct CarrierRecord` (§4.6 fields without span bounds),
  `struct SurfaceJet {Vec3 p, du, dv, duu, duv, dvv;}`,
  `SurfaceJet evaluate(const CarrierRecord&, Vec2 uv, int order)`,
  `enum class CurveKind : uint16_t {line, circle, ellipse, hyperbola, parabola, bspline}`
  (a polyline is a degree-1 B-spline, §5.4 row 16),
  `CurveJet evaluate(const CurveRecord&, double t, int order)`,
  `simd::Vec4d` (Clang vector extension wrapper, masked tails with the body's
  instruction sequence).
- **Oracle tier.** Spec: closed-form points at authored parameters (a rational
  quarter circle lies on its circle); model: V1's independent evaluators
  (`tests/oracles/geometry/eval.hpp`) agree within 2⁻⁴⁰·M on seeded points of
  every family.
- **M8 first.** None.
- **Fleet runs.** Evaluation CPU-s per point per kind (the §22.6 T0–T1 line).
- **Done when.** Every §5.8 geometry row refuses its authored fixture by name
  and the rest of the part loads; G5's revolution and extrusion profiles
  (B-spline, conic, line, touching the axis) evaluate on the file's own
  parameterisation; the adapter writes kinds 7 and 8 and the torus flags.

#### P5 span-bounds

- **Purpose.** The one span pass that every sizing and closure decision reads:
  Bézier extraction, second-derivative bounds D, M_S, σ̲, σ̄, Bernstein normal
  cones, irregular-point detection; the curve rules of §10.3; the closed-form
  angle terms of §6.2; integer work estimates.
- **Design.** §13.1 bounds (polynomial, rational quotient rule, torus,
  revolution, knot lines), §6.2 angle terms and normal cones, §10.3 item 1,
  §21.1 lane axis over spans and degree buckets, §4.6 work estimate, §5.8
  `undeclared_singularity`.
- **Beads.** `.25.21` (torus angle term at spindle and horn pole lines).
- **Files.** `native/fast_mesher/bounds/` (`bezier.{hpp,cpp}`,
  `span_bounds.{hpp,cpp}`, `curve_rules.{hpp,cpp}`, `angle_terms.{hpp,cpp}`,
  `work.{hpp,cpp}`); tests `tests/native_fm_bounds_*.cpp`; benchmark
  `benchmarks/fast_mesher/span_buckets.cpp`.
- **Consumes.** `CarrierRecord`, `CurveRecord`, `simd::Vec4d`, `KeyRequest`.
- **Produces.** `struct SpanBounds {double d_uu, d_uv, d_vv, m_s, sigma_lo, sigma_hi, min_w; NormalCone cone; Box3 box; bool irregular;}`,
  `std::vector<SpanBounds> span_pass(const CarrierRecord&)`,
  `double curve_sag(const CurveRecord&, Interval t)`,
  `double delta_theta_eps(double eps, double R)`, `double rho_eps(double eps, double R)`,
  `WorkEstimate estimate(const L1Definition&, std::span<const SpanBounds>, const KeyRequest&)`.
- **Oracle tier.** Property: every bound holds against dense evaluation of its
  span on the falsification families of §23.5 (high weight variation,
  multi-span with multiplicities p − 1 and p, degrees 4–25); spec: Δθ_ε and
  ρ_ε closed forms across their domains (ε ≥ 2R, ε ≥ R).
- **M8 first.** None.
- **Fleet runs.** Degree-bucket kernel microbenchmarks (§27 #50); span-pass
  CPU-s per span on the strata.
- **Done when.** No bound is beaten by dense evaluation on any family; the
  bucket benchmark confirms or revises §21.1's buckets in the unit report.

#### P6 task-runtime

- **Purpose.** The global task graph and the CPU lane that runs it: pending
  counters over declared reads, refusal release, the finiteness gate, typed
  healer queues and staging buffers, per-worker deques with LPT seeding and
  stealing, per-worker slabs and per-definition chunk lists, cancellation at
  dequeue, deadlines, budgets and the run's memory limits.
- **Design.** §4.1 (readiness, refusal release, finiteness, purity), §4.2
  mechanisms 1–10, §4.5, §4.7 count backstops, §4.8, §20.1 items 1, 2, 8,
  §20.5.
- **Beads.** None.
- **Files.** `native/fast_mesher/runtime/` (`graph.{hpp,cpp}`,
  `lane.{hpp,cpp}`, `deque.hpp`, `slab.{hpp,cpp}`, `chunks.hpp`,
  `queues.{hpp,cpp}`, `cancel.hpp`, `budget.{hpp,cpp}`); tests
  `tests/native_fm_runtime_*.cpp`.
- **Consumes.** `RecordSink`, `bgfm::L1Definition`.
- **Produces.** `enum class TaskKind : uint16_t {t0_decode, t1_carrier, t1b_feature_size, t2_measure, t2o_orientation, t3_l1_solve, t4_l2_vertex, t5_l3_edge, t5p_deferred, t6_collapse, t7_clearance, t8_closure, t9_face, t10_publish, t11_place, t12_export}`,
  `class Graph` (`TaskId add_task(TaskKind, EntityKey, std::span<const TaskId> reads)`),
  `struct LaneConfig {CpuSet cpus; uint32_t node; std::optional<uint32_t> gpu; uint32_t workers;}`,
  `RunOutcome run(Graph&, const LaneConfig&, CancelToken)`,
  `template<class T> class ChunkList`, `class Slab`, `class HealerQueue`,
  `using TaskFn = TaskResult (*)(const TaskInput&)`, `void register_task(TaskKind, TaskFn)`.
- **Oracle tier.** Metamorphic: a synthetic graph's outputs are byte-identical
  across thread counts 1, 2, 7, 12, 24, randomized task order and injected
  delays; spec: a refused task releases its dependents; a cancelled run exits
  6 with the records of what completed.
- **M8 first.** None.
- **Fleet runs.** TSan on a full lane; per-task overhead against §13.4's
  o ≈ 1 µs.
- **Done when.** Both levels of F4 (`staging`: background healer, single pass)
  run; scheduling overhead per task is in the unit report.

#### P7 edge-measure

- **Purpose.** T2 per (edge, face occurrence): initial dyadic samples,
  evaluation from the prior, seeded projection onto each incident face,
  signed d_f, the per-edge move class and bound μ̄, and the runtime gap rows of
  the profile.
- **Design.** §8.1, §8.2, §10.2 (the dyadic tree, initial sampling only),
  §10.3 item 2 as the initial density, §4.7 seeded-projection row, §7.4 rows
  G-SH-LG, G-FA-EG, G-FA-VG, vertex–edge, G-LO-LG.
- **Beads.** `.25.30` (coincidence decisions when u_ctx is absent or unusable).
- **Files.** `native/fast_mesher/measure/` (`dyadic.{hpp,cpp}`,
  `measure.{hpp,cpp}`, `project.{hpp,cpp}`); tests `tests/native_fm_measure_*.cpp`.
  Seeded projection reuses `native/projection_solver.hpp` `project_local` with
  P4's evaluators [call: it is templated on the evaluator and already returns
  the §4.7 statuses].
- **Consumes.** `L1Definition`, `CarrierRecord`, `CurveRecord`, `evaluate`,
  `SpanBounds`, `KeyRequest`, `register_task`, `RecordSink`; version 3's
  `pcurve_presence`, `master` and CONTEXTS rows.
- **Produces.** `struct DyadicPath {std::array<uint64_t, 18> bits; uint16_t depth;}`
  (1,152 bits cover the subnormal depth of §11.2), `struct EdgeMeasurement`
  (§4.6 fields), `EdgeMeasurement measure(const L1Definition&, EdgeSlot, FaceUseSlot, const KeyRequest&)`,
  `enum class MoveClass : uint8_t {small, deferred}`, `double mu_bar`.
- **Oracle tier.** Property: V2's own projection agrees with every measured
  signed d_f on the reconciliation families; spec: the signed |x| formula on
  authored dihedrals, and no full-rank |x| below σ_cut.
- **M8 first.** None.
- **Fleet runs.** T2 CPU-s per (sample, occurrence) on the strata; projection
  status and iteration distributions against the backstop (§27 #44).
- **Done when.** Missing pcurves cost one projection per sample with no special
  path; every §4.7 projection status maps to `projection_failed`; T2 is
  registered in the graph.

#### P8 face-charts

- **Purpose.** T2o and the charts it needs: one UV chart per face occurrence
  from T2's UVs, loops lifted into one period, seams as two lifts, poles and
  vertex loops, multi-loop faces on curved carriers and faces on closed
  B-spline carriers (the two gate lifts), and S1 per face by an owned Green's
  sum with its rounding bound.
- **Design.** §5.3 (orientation without `source_uv`, S1; the gate rows for
  `.25.13`, `.25.12`), §10.5.
- **Beads.** `.25.13`, `.25.12`.
- **Files.** `native/fast_mesher/charts/` (`chart.{hpp,cpp}`,
  `orientation.{hpp,cpp}`); tests `tests/native_fm_charts_*.cpp`.
- **Consumes.** `EdgeMeasurement`, `L1Definition`, `CarrierRecord`; I3's
  `--new-path` stores (no reconstructed pcurves).
- **Produces.** `struct FaceChart`, `FaceChart chart(const L1Definition&, FaceSlot, std::span<const EdgeMeasurement>)`,
  `S1Verdict orient(const FaceChart&)` (a correction with its record, or
  refusal reason 7).
- **Oracle tier.** Property: V1's trim domain agrees with the chart's loop
  nesting and winding on the closure families F1, F3, F10, F11; V8 later
  compares S1 with the adapter's verdicts.
- **M8 first.** None.
- **Fleet runs.** Corpus admission census in I3's `--new-path` mode: the 39
  multi-wire and 12 closed-spline refusals are gone, and every face of those
  files charts or refuses by name.
- **Done when.** Gate lifts `.25.13` and `.25.12` hold on the corpus; T2o is
  registered in the graph.

#### P9 feature-size

- **Purpose.** T1b: a lower bound lfs_e per edge and vertex, PSLG-style in 3D,
  from a box tree over T1 bounds of the entities disjoint from each item's
  closure, floored at ε_ref; collapse candidates; recorded query regions.
- **Design.** §8.8, §9 candidates, §19.3.
- **Beads.** `.25.29` (the query's algorithm and cost).
- **Files.** `native/fast_mesher/lfs/` (`box_tree.{hpp,cpp}`, `lfs.{hpp,cpp}`);
  tests `tests/native_fm_lfs_*.cpp`.
- **Consumes.** `L1Definition`, `SpanBounds` (boxes), curve-span boxes,
  `KeyRequest` (ε_ref, r_c).
- **Produces.** `struct LfsSlice`, `LfsSlice lfs(const L1Definition&, std::span<const SpanBounds>, const KeyRequest&)`,
  `struct CollapseCandidate`, `struct QueryRegion`.
- **Oracle tier.** Property: lfs_e never exceeds the exact distance V1 computes
  to every disjoint entity, on authored neighbourhoods and the thin-feature
  families T1–T4.
- **M8 first.** None.
- **Fleet runs.** T1b CPU-s per edge and query nodes per edge on the strata
  (the §22.6 T1b line).
- **Done when.** The bound holds on every family; the query's cost is stated
  per edge in the unit report (closing `.25.29`).

#### P10 reconcile

- **Purpose.** T3–T5′: the L1 point solve per sample and full-rank vertex, L2
  for rank-deficient vertices, the L3 blend, the measured residual, the rung
  ladder 0–5 in the background healer, the structural-first branch choice
  and their records; the resolved edge x_e(t) as a pointwise function.
- **Design.** §8.2–§8.7, §3.2 (resolved edge, boundary displacement g_f), §4.5
  healer, §4.7 Newton rows.
- **Beads.** None.
- **Files.** `native/fast_mesher/reconcile/` (`point_solve.{hpp,cpp}`,
  `levels.{hpp,cpp}`, `ladder.{hpp,cpp}`, `branch.{hpp,cpp}`); tests
  `tests/native_fm_reconcile_*.cpp`; a chain dump for V2 until P18's container
  exists.
- **Consumes.** `EdgeMeasurement`, `LfsSlice`, `KeyRequest`, `HealerQueue`,
  `register_task`, `RecordSink`.
- **Produces.** `struct EdgeResolution`, `ResolvedSample resolve(const EdgeResolution&, double t)`
  (xyz, rung, measured residual and g_f per occurrence), `struct ResolvedVertex`
  (§4.6).
- **Oracle tier.** Property: V2 (oracle 10) on every reconciliation family of
  §23.5; model: oracle 10's global solve on a seeded corpus sample.
- **M8 first.** None.
- **Fleet runs.** B-ED-CL deferred-item counts per class (before batch sizes are
  tuned, §4.5); Newton count distribution against the backstop (§27 #44);
  equal-tier near-tangent class count (§27 #41).
- **Done when.** Oracle 10 passes on every reconciliation family; both levels
  of every flag it owns run: `reconciliation` (the M2 level; P31 builds the
  other), `thresholds`, `theta_cut`, `r2_linear_step`,
  `transverse_leap_by_conditioning`, `tangent_split`, `l3_blend`,
  `equal_tier_tangent`.

#### P11 clearance

- **Purpose.** T7: clearance budgets in construction. E1 candidates, E2 lower
  bounds (closed forms, generic analytic, Bézier hull recursion), E3 budgets
  per face per zone, the wedge and vertex rules, material cusps with signed
  credit, the thin-wall rule and c_floor.
- **Design.** §12, §3.7, §4.7 E2 row.
- **Beads.** None.
- **Files.** `native/fast_mesher/clearance/` (`candidates.{hpp,cpp}`,
  `bounds.{hpp,cpp}`, `budgets.{hpp,cpp}`, `wedge.{hpp,cpp}`); tests
  `tests/native_fm_clearance_*.cpp`; the census probe
  `scripts/census_clearance_candidates.py`.
- **Consumes.** `SpanBounds`, `EdgeMeasurement` (signed θ, μ̄), `LfsSlice`,
  `CollapseCandidate`, `KeyRequest`; E1(a) uses the existing pair discovery
  (`include/brepgpu_pairs.h`, `bg4_discover_pairs`) per definition, and P9's
  box tree above its caps (65,536 boxes, 1,048,576 pairs) [call].
- **Produces.** `struct ClearanceSlice` (ε_loc per zone, `WedgeBudget` per
  shared edge, vertex budgets, reported source contact pairs).
- **Oracle tier.** Property: V5 (oracle 5) on the embedding families K1–K7,
  T1–T4, U1–U2, V1, R1, C1, X1; spec on the closed-form pairs.
- **M8 first.** E1 candidate counts per class and closed-form coverage on the
  watch and the corpus from the existing AABB path (§12.6, §27 #36), as the
  unit's first task; runnable early (§9).
- **Fleet runs.** The census; T7 CPU-s per candidate against §22.6; G4 H4
  (under 2% of meshing time, under 5% of triangles).
- **Done when.** Oracle 5 finds no forbidden pair on every embedding family;
  both levels of `signed_clearance` and `local_pair_test` run.

#### P12 closure

- **Purpose.** T8 and the chain: the dyadic chain model on doubles (the old
  `DefinitionBoundaryForest`, `native/mesher/boundary.hpp:147`, as
  specification), the sample tests of §10.3, P1–P4 on sound bounds, cusps, the
  stop rule and its refusals, the sample-count floor, seams, poles and
  quotients; the frozen chain L5.
- **Design.** §10, §11, §4.7 closure row.
- **Beads.** `.25.22` (zero-speed parameterisations), `.25.23` (carrier
  extension outside the UV domain), `.25.31` (closed edges below δ_exp or the
  CDT quantum).
- **Files.** `native/fast_mesher/closure/` (`chain.{hpp,cpp}`,
  `predicates.{hpp,cpp}`, `close.{hpp,cpp}`); tests `tests/native_fm_closure_*.cpp`.
- **Consumes.** `EdgeResolution`, `FaceChart`, `ClearanceSlice` (ε_loc for P4),
  `SpanBounds`, `curve_sag`, `KeyRequest`.
- **Produces.** `struct Chain` (L5, §4.6), `CloseResult close_edge(...)` (a chain
  or a §11.5 refusal record).
- **Oracle tier.** Property: V4 oracles 2 and 3 on families F1–F14; spec:
  oracle 17's sample counts on untrimmed analytic families.
- **M8 first.** None.
- **Fleet runs.** Closure splits and depth per edge against the packing bound;
  T8 CPU-s per segment test (§22.6).
- **Done when.** Oracles 2, 3 and 17 pass on F1–F14; every §11.2 stop case has
  a family that fires it and gets its refusal record; both levels of
  `boundary_closure`, `split_point`, `tau0`, `biased_split` and
  `closed_form_counts` run.

#### P13 band-cdt

- **Purpose.** The band and every planar face: `direct_cdt` adapted to
  fixed-point scaled-chart input taken straight from doubles, its pairwise
  hole-containment validation dropped, components and separators, interior
  refinement only, the per-triangle ledger, clearance, A0 and C1 checks; plain
  double predicates as F1's other level.
- **Design.** §13.2, §13.3, §21.3, §18.1 C1, §4.7 band rows.
- **Beads.** `.25.24` (the fixed-point UV is the UV of record).
- **Files.** `native/fast_mesher/band/` (`fixed_chart.{hpp,cpp}`,
  `band.{hpp,cpp}`, `refine.{hpp,cpp}`); tests `tests/native_fm_band_*.cpp`; the
  adapter calls `native/direct_cdt/core.hpp` `solve_directed_regions` [call:
  the C++ core, not the C ABI, which forces `bgfd_validate_domain`,
  `native/direct_cdt/direct_cdt.cpp:109`, the pairwise check of
  `native/face_domain/face_domain.cpp:363-371`].
- **Consumes.** `KeyRequest`, `SpanBounds` (the scaled chart), `ClearanceSlice`.
- **Produces.** `struct FixedChart`, `struct BandInput`, `BandResult triangulate_band(const BandInput&)`,
  `struct TriangleChecks` (ledger, clearance, A0, C1 per triangle).
- **Oracle tier.** Property: V4 oracle 2 (constraint segments present,
  coverage without gap or overlap, orientation, the CDT property where
  claimed).
- **M8 first.** None.
- **Fleet runs.** The many-hole plate family N = 8–4,096 against the old
  pairwise cost (`docs/DIRECT_CDT.md` L70); band sites, flips and CPU-s per
  triangle.
- **Done when.** Oracle 2 passes; cost on the many-hole family grows as
  n log n, not n·H; both levels of `predicates` (F1) run.

#### P14 faces-analytic

- **Purpose.** T9 for planes, cylinders, cones, spheres and extrusions: the
  closed-form generators (slabs, strips, the sphere lattice with pole fans),
  bands where trims cut them, and every triangle on its ε ledger.
- **Design.** §13.1 rows 1–4, §3.4, §3.6, §6.2 closed forms.
- **Beads.** None.
- **Files.** `native/fast_mesher/faces/` (`analytic.{hpp,cpp}`, `ledger.{hpp,cpp}`);
  tests `tests/native_fm_faces_analytic_*.cpp`.
- **Consumes.** `Chain`, `FaceChart`, `ClearanceSlice`, `SpanBounds`,
  `triangulate_band`, `KeyRequest`, `delta_theta_eps`, `rho_eps`.
- **Produces.** `struct FaceOutput` (L6, §4.6), `FaceOutput mesh_face(const FaceInput&)`.
- **Oracle tier.** Property: V3 oracle 1 and V4 oracles 2 and 3; spec: oracle
  17 counts (2n triangles per strip of n slabs).
- **M8 first.** None.
- **Fleet runs.** T9 CPU-s per triangle and per face on analytic faces
  (§22.6's 40% line).
- **Done when.** MS1's inputs mesh and pass (§11); T9 is registered for its
  carriers.

#### P15 faces-lattice

- **Purpose.** T9 for tori, revolutions and B-splines: the 2:1-balanced dyadic
  quadtree sized by the anisotropic Waldron rule, conforming quads and the
  packet zipper (`native/mesher/adaptive_packets.cpp:366` as specification),
  the lattice interior as its own produced entity with conservative cell
  classification, the C1 clusters, C4 iso-line edges and the split rule.
- **Design.** §13.1 freeform rows and knot lines, §13.4, §4.3.
- **Beads.** None.
- **Files.** `native/fast_mesher/faces/` (`lattice.{hpp,cpp}`,
  `zipper.{hpp,cpp}`, `clusters.{hpp,cpp}`); tests
  `tests/native_fm_faces_lattice_*.cpp`.
- **Consumes.** `SpanBounds`, `Chain`, `FaceChart`, `triangulate_band`,
  `ClearanceSlice`, `WorkEstimate`, `KeyRequest`.
- **Produces.** `struct LatticeInterior` keyed by integer lattice coordinates;
  `LatticeInterior build_lattice(const FaceInput&)`; `FaceOutput mesh_face` for
  its carriers.
- **Oracle tier.** Property: V3 oracle 1 on the entity families (spindle and
  horn tori, high-degree and multi-span B-splines, high weight variation);
  metamorphic: mesh(K = 1) ≡ mesh(K = k) for every cluster count (oracle 14,
  run by V9).
- **M8 first.** The per-face predicted work distribution and the share of
  freeform faces with an empty band, on the strata, the families and the
  watch separately, before C1/C2 ship in the default (§13.4, §27 #9).
- **Fleet runs.** That census; T9 CPU-s per triangle on freeform faces.
- **Done when.** Oracle 1 passes on the entity families; both levels of
  `in_face_split` (F3) run; the C1/C2 default is set from the census.

#### P16 shading

- **Purpose.** Corner normals by Rule B, one-sided limit normals at poles,
  apexes and collapsed edges (freeform included, which
  `native/om10/math.hpp:126,151` refuses today), the A1 cap repair by interior
  means, the optional A2 bound, and edge classification records.
- **Design.** §3.10, §15.1, §15.2.
- **Beads.** None.
- **Files.** `native/fast_mesher/shading/` (`normals.{hpp,cpp}`, `caps.{hpp,cpp}`,
  `edge_class.{hpp,cpp}`); tests `tests/native_fm_shading_*.cpp`.
- **Consumes.** `FaceOutput`, `CarrierRecord`, `evaluate`, `Chain`.
- **Produces.** `Vec3 corner_normal(const CarrierRecord&, Vec2 uv, const LimitContext&)`,
  `CapRepair repair_caps(FaceOutput&, const KeyRequest&)`,
  `enum class EdgeClass : uint8_t {smooth, crease, vanishing, defect_band, feature_line}`.
- **Oracle tier.** Spec/property: V6 oracle 6 (a)–(d) on G13's seven exact
  surfaces.
- **M8 first.** None.
- **Fleet runs.** Cap repairs and unrepaired caps per report class (G13 H3:
  at most 2% triangles).
- **Done when.** Oracle 6 passes on the seven surfaces and on MS1's outputs;
  freeform collapsed-edge limit normals exist; both levels of `cap_c` run.

#### P17 collapse

- **Purpose.** T6: edge collapse E′ with the merged vertex, sliver collapse
  S′, boundary-only faces F′, the seven checks in order, reverts, records and
  the `absorbed` disposition.
- **Design.** §9.
- **Beads.** None.
- **Files.** `native/fast_mesher/collapse/` (`clusters.{hpp,cpp}`,
  `checks.{hpp,cpp}`); tests `tests/native_fm_collapse_*.cpp`.
- **Consumes.** `CollapseCandidate`, `EdgeResolution`, `Chain`, `FaceChart`,
  `ClearanceSlice`, `KeyRequest`.
- **Produces.** `struct CollapseSlice` (clusters, survivors, records).
- **Oracle tier.** Property: V3 oracle 1 with r_c on the collapse families
  A1–A16; each family's expected record present (§23.5).
- **M8 first.** None.
- **Fleet runs.** F6's effect on triangles and time per stratum (G10 H3: below
  1% everywhere drops E′ and S′).
- **Done when.** A1–A16 give their expected records and pass oracle 1; both
  levels of `collapse` (F6) and the `thresholds` r_c level run.

#### P18 publish

- **Purpose.** T10: canonical order and global numbering from canonical keys,
  the publication tables, the classification map as ranges, S2 per shell by
  signed volume, accounting of every face, definition, key and occurrence,
  and the internal output container.
- **Design.** §3.8, §4.4, §5.3 S2, §7.2 roll-up, §17.1, §17.2, §26.1 I9 slot.
- **Beads.** `.37.14` (mesh a void-bearing source end to end:
  `tests/fixtures/void-block.step` and its void-free twin, signed volumes 936
  and 1000).
- **Files.** `native/fast_mesher/publish/` (`publish.{hpp,cpp}`,
  `container.{hpp,cpp}`); tests `tests/native_fm_publish_*.cpp`,
  `tests/test_fm_mesh.py`; CLI subcommand `mesh`.
- **Consumes.** `FaceOutput`, `L1Definition`, `RecordSink`, `FaceStatus`,
  `register_task`.
- **Produces.** `struct Publication` (L8), the container `BGFMOUT1` (tagged
  table; byte layout per §29), `brepgpu-fm mesh STORE --eps MM [--profile P] OUT`.
- **Oracle tier.** Property: V4 oracles 3 (by vertex key, per-shell Euler) and
  4; spec: void-block signed volumes 936 and 1000.
- **M8 first.** None.
- **Fleet runs.** MS1's measurements (§9).
- **Done when.** MS1 (§11).

#### P19 placements

- **Purpose.** T11 and the placement table: rigid, mirrored and uniform-scale
  occurrences sharing keys as §16.1 says, scale classes, refusals per
  occurrence, mapped-item solids published as occurrences, per-occurrence
  attributes and accounting per key.
- **Design.** §16, §22.5 exclusions (`.56`).
- **Beads.** `.25.4` (with its note: the cofactor normal transform in
  `native/om10/math.hpp:177-178` changes when mirrors are admitted), `.25.11`.
- **Files.** `native/fast_mesher/place/` (`table.{hpp,cpp}`,
  `occurrences.{hpp,cpp}`); tests `tests/native_fm_place_*.cpp`;
  `native/rigid_placement.hpp` widened per §16.1; `native/om10/math.hpp:177-178`;
  `native/step_source/main.cpp` mapped-item admission (`:425`, `:477`).
- **Consumes.** `Publication`, the store's USAGES and OCCSTYLE rows, `KeyRequest`.
- **Produces.** `struct PlacementTable`, `enum class PlacementKind : uint8_t {rigid, mirrored, uniform_scale, refused}`,
  `struct ScaleClass`, `std::vector<KeyRequest> keys_for(const PlacementTable&, const RequestInput&)`.
- **Oracle tier.** Spec: the placement families (mirrored chiral part, one
  curved definition at 32 scales over [0.5, 2], rigid instances whose σ
  differs by ulps give one key); the existing placement oracle
  (`python/brepgpu/imported_oracles.py:1056`).
- **M8 first.** None.
- **Fleet runs.** G9 H5 over every raw placement in the corpus (§27 #29); the
  3MF mirrored-item consumer check with P20 (§23.3, §27 #21).
- **Done when.** The placement families give their expected keys and refusals;
  `mapped-item-solid.step` is admitted; mirrored normals point outward.

#### P20 export

- **Purpose.** T12: quantization-aware frames and reserves, recentring or
  refusal per format, the STL, 3MF and glTF writers with identity carriage,
  the 3MF pair refusal, and the offline contact fit report.
- **Design.** §18, §17.3, §16.3 hidden occurrences.
- **Beads.** None.
- **Files.** `native/fast_mesher/export/` (`stl.cpp`, `threemf.cpp`, `gltf.cpp`,
  `frames.{hpp,cpp}`, `contact_fit.cpp`); tests `tests/native_fm_export_*.cpp`.
- **Consumes.** `Publication`, `PlacementTable`, `KeyRequest` (δ_exp, R_max).
- **Produces.** `ExportResult write_stl(...)`, `write_3mf(...)`, `write_gltf(...)`,
  per-format status rows.
- **Oracle tier.** Model: V7 oracle 7 per consumer emulation.
- **M8 first.** None.
- **Fleet runs.** The 100 m family's reserve and refusal counts (§27 #49); 3MF
  pair refusals on the corpus (§18.3).
- **Done when.** Oracle 7 passes for every consumer on the families; the
  per-format statuses of §7.1 are written.

#### P21 native-source-record

- **Purpose.** Owned operations (cut, extrusion, revolution, fillet, offset,
  feature bore) write their results as source records, so the mesher has one
  input path.
- **Design.** §5.4 row 28.
- **Beads.** None.
- **Files.** `python/brepgpu/` beside each operation (`cut_meshing.py`,
  `extrusion.py`, `revolution.py`, `fillet.py`, `offset.py`, `feature_bore.py`),
  through `imported_source.write_source(..., version=3)`; tests
  `tests/test_native_source_record.py`.
- **Consumes.** The operations' result types; P1's version 3 writer.
- **Produces.** `to_source(result) -> ImportedSource` per operation, with u_ctx
  declared as a CONTEXTS row and the rounding residual per entity recorded.
- **Oracle tier.** Spec: oracle 8's fixtures per operation kind (V8).
- **M8 first.** None.
- **Fleet runs.** Each operation's authored results meshed by `brepgpu-fm`.
- **Done when.** Every operation kind's result meshes and passes oracles 1–4.

#### P22 consumers

- **Purpose.** Every consumer of the old meshers moved to the new path: the
  watch render and exploded diagrams (`brepgpu-adaptive-model`,
  `native/mesher/adaptive_model_main.cpp:492-520`), the scene and display
  through om10, `brepgpu-mesh` callers, `mesh_imported` callers
  (`python/brepgpu/imported_meshing.py:1227`: `examples/render_om10.py`,
  `scripts/qualify_om10.py`, `scripts/benchmark_om10.py` and their tests),
  `nvidia_reference.load_native_model_match` (`:197`), and the Python result
  meshers (`cut_meshing.py`, `extrusion_meshing.py`,
  `feature_bore_meshing.py`, `meshing.py`) through P21.
- **Design.** §25 Retired (its list is incomplete; gap G-6).
- **Beads.** None.
- **Files.** The callers named above; `native/om10/scene.hpp` reads `BGFMOUT1`.
- **Consumes.** `brepgpu-fm mesh`, `BGFMOUT1`, P20's writers, P21.
- **Produces.** No new interface; the callers' outputs come from the new path.
- **Oracle tier.** The callers' tests move onto oracles 1–6 (V3–V6); none keeps
  an old original-source reader.
- **M8 first.** None.
- **Fleet runs.** The watch render and exploded diagram from the new path, CPU
  and both P100s.
- **Done when.** No tracked caller reaches the old mesher.

#### P23 retire

- **Purpose.** The old mesher leaves the build; old reports gain one
  historical line each; certification language leaves the repository docs;
  the old original-source readers are rewritten or retired (§25 Moved); store
  version 2 leaves the adapter and the Python reader.
- **Design.** §25.
- **Beads.** `.25.3`.
- **Files.** `native/mesher/` (removed from every build), `python/brepgpu/imported_meshing.py`,
  `python/brepgpu/imported_oracles.py` (old-contract checks), `AGENTS.md`,
  `docs/ALPHA_QUALIFICATION.md`, every `results/*/REPORT.md` of the old regime
  (one line each).
- **Consumes.** P22.
- **Produces.** Nothing new.
- **Oracle tier.** The full suite green without the old mesher.
- **M8 first.** None.
- **Fleet runs.** The full suite.
- **Done when.** `native/mesher` and the Python mesher entry points are gone
  from the build; `AGENTS.md` and `docs/ALPHA_QUALIFICATION.md` describe the
  new regime; version 2 is gone.

#### P24 gpu-kernels

- **Purpose.** The GPU-eligible classes as device kernels with resident span
  tables: span sizing, lattice evaluation, cell classification, emission and
  normals, freeform seeded projection; pinned staging on the GPU's node; the
  FP-contraction decision for host and device.
- **Design.** §20.1 items 4–7, §21.1 GPU layout, §21.3 (no device predicate
  until an arm runs one), §21.4 F7 FP32 arm, §2 floating point.
- **Beads.** `.25.2` (with its note: `--fmad=false` halves P100 FP64 mul-add
  throughput).
- **Files.** `native/fast_mesher/gpu/`; `find_package(CUDAToolkit 12.8 EXACT REQUIRED)`
  in its CMake (defect D-1); tests `tests/native_fm_gpu_*.cu`.
- **Consumes.** `SpanBounds` tables, `LatticeInterior`, P7's projection, `Lane`.
- **Produces.** One device batch function per class; the device residency
  plan; the contraction flags per target, recorded in `backend_id`.
- **Oracle tier.** Metamorphic/differential: FI-1 and FI-2 against the CPU
  backend (V9 oracle 12) on both GPUs; property: every V oracle on device
  output.
- **M8 first.** Kernel microbenchmarks per class and device, each GPU on its
  own (never set side by side, `.68`).
- **Fleet runs.** Both GPUs, every test.
- **Done when.** FI-1 and FI-2 hold on both GPUs; `.25.2`'s decision is
  recorded with the measured throughput; both levels of `eval_precision` (F7)
  run.

#### P25 hetero-placement

- **Purpose.** The cost model and its calibration pre-pass per device,
  HeteroPrio at batch granularity with spoliation, the placement log, sticky
  placement, the CPU-only fallback, device-fault re-placement and whole-host
  mode.
- **Design.** §20.1 item 3, §20.2–§20.4, §20.6, §4.8 device faults.
- **Beads.** `.25.28` (item-level cancellation for spoliated CPU batches).
- **Files.** `native/fast_mesher/runtime/` (`cost_model.{hpp,cpp}`,
  `heteroprio.{hpp,cpp}`, `placement_log.{hpp,cpp}`); `brepgpu-fm calibrate`.
- **Consumes.** P24's batch functions, P6's `Graph` and `Lane`.
- **Produces.** `struct CostModel`, the calibration file (tagged table, one per
  device), `struct PlacementLog`, `brepgpu-fm calibrate`.
- **Oracle tier.** Metamorphic: replaying a placement log reproduces the run's
  bytes (V9).
- **M8 first.** The calibration pre-pass per (class, device, host class,
  binary digest).
- **Fleet runs.** F8 per lane, never pooled; G0–G5 per lane and report class.
- **Done when.** Replay holds; every fallback condition of §20.3 is exercised;
  both levels of `gpu` (F8), `placement`, `smt_workers`, `gpu_driver_core`
  and `cross_socket_steal` run.

#### P26 cache-incremental

- **Purpose.** Cache keys over dependency output digests, the opt-in store with
  verified hits, invalidation by key miss within the static halo, recorded
  query regions, the edit-disposition record, channels F and Δ, and oracle 13
  with its edit generator.
- **Design.** §19, §24.1 row 13, §24.3.
- **Beads.** None.
- **Files.** `native/fast_mesher/cache/`; `tests/oracles/incremental/` (the edit
  generator).
- **Consumes.** Every task's output bytes and digests; P6's graph.
- **Produces.** `CacheKey key(const TaskInput&)`, `class CacheStore`,
  `struct EditDisposition`, the edit generator.
- **Oracle tier.** Metamorphic: oracle 13 on every edit class of §24.3.
- **M8 first.** None.
- **Fleet runs.** Hit and miss counts per edit class; halo sizes.
- **Done when.** Oracle 13 passes every class; undo restores the bytes with
  every task a hit.

#### P27 lod

- **Purpose.** LOD by re-tessellation at ε_k = 4^k·ε under the mesh key,
  mixed per-face levels by chain identity, one level per collapse component,
  and the oracle the LOD promises lack.
- **Design.** §15.3.
- **Beads.** `.25.25`.
- **Files.** `native/fast_mesher/publish/lod.{hpp,cpp}`; `tests/oracles/lod/`.
- **Consumes.** P26, `Chain`, `CollapseSlice`.
- **Produces.** `Publication lod_level(const Publication&, uint32_t k)`.
- **Oracle tier.** Property: switching level k to k − 1 moves no point by more
  than 1.25·ε_k; coarse samples are a subset of fine ones; oracle 3 on mixed
  levels.
- **M8 first.** None.
- **Fleet runs.** Pyramid cost against 4/3 of the finest level.
- **Done when.** The LOD oracle passes on the families and the watch.

#### P28 pmi

- **Purpose.** The PMI subset read, margins, consumption and the four warnings
  (warn and comply), per-feature reports, and oracle 9.
- **Design.** §6.3, §6.4, §24.1 row 9.
- **Beads.** None.
- **Files.** `native/step_reader/pmi.{hpp,cpp}`; `native/fast_mesher/records/pmi.cpp`;
  `tests/test_pmi.py`.
- **Consumes.** I1's entity table; I6's identity tables; `KeyRequest`.
- **Produces.** `struct PmiTolerance`, the B-RQ-PM and B-RQ-UN warnings.
- **Oracle tier.** Spec/metamorphic: oracle 9 (counts against the senders'
  validation properties; mm → inch rewrite leaves every margin unchanged;
  nominal-on-limit holes give PMI-OUT at every ε).
- **M8 first.** None.
- **Fleet runs.** The 17 PMI-bearing NIST models.
- **Done when.** Oracle 9 passes on the NIST models.

#### P29 postprocess-arm, P30 metric-cdt-arm, P31 healing-classes-arm

- **Purpose.** P29: §14's in-core operations under check, arena flag
  `postprocess`, default off. P30: F2's other level, metric CDT of the whole
  face for torus, revolution and B-spline carriers. P31: F5's other level, the
  healing paper's R1–R6 classes with distance split.
- **Design.** §14 (P29), §23.2 F2 (P30), §23.2 F5 (P31).
- **Beads.** None.
- **Files.** `native/fast_mesher/faces/postprocess.cpp` (P29),
  `native/fast_mesher/faces/metric_cdt.cpp` (P30),
  `native/fast_mesher/reconcile/healing_classes.cpp` (P31).
- **Consumes.** The mechanism each arm replaces (P16, P15, P10), and I9.
- **Produces.** The other level of its flag.
- **Oracle tier.** The full stack; an arm whose rule states no hard-clause
  bound yields `indeterminate` (§7.1), never `valid`.
- **M8 first.** None.
- **Fleet runs.** Its flag's arena runs.
- **Done when.** Its flag runs in the arena at both levels.

### 6.3 Ingress, measurement and arena units

#### I1 step-parse

- **Purpose.** The owned Part 21 parse: STEP bytes to an entity table (ids,
  types, parameter trees, complex instances, header) with parallel record
  splitting, deterministic across thread counts; and the adapter phase split
  that sets the owned reader's budget before it is written.
- **Design.** §5.6, §5.7 header strings, §5.8 syntax rows.
- **Beads.** `.25.7` (first half; I5 closes it).
- **Files.** Create `native/step_reader/` (`CMakeLists.txt`, `lexer.{hpp,cpp}`,
  `entity_table.hpp`, `parse.{hpp,cpp}`, `header.{hpp,cpp}`, `container.{hpp,cpp}`,
  `main.cpp`), `tests/native_step_reader_*.cpp`, `tests/step_parse_reference.py`,
  `tests/test_step_parse.py`, `tests/step_families.py`. Modify
  `native/step_source/main.cpp` (phase times), `tests/test_step_reader_raw.py`
  (imports `step_families`), `scripts/check_native_face_domain.py`.
- **Consumes.** `bgtt`, `bgsrc::Reason` (P1 Task 1).
- **Produces.** `bgstep::EntityTable`, `bgstep::ParseResult parse(std::span<const std::byte>, const ParseOptions&)`,
  `bgstep::HeaderSection`, the container `BGP21001`, `brepgpu-step-parse`.
- **Oracle tier.** Spec (ISO 10303-21 grammar on authored records; decimal
  spellings to exact binary64 bits); model (the independent exact reader
  `python/brepgpu/imported_oracles.py:285` `_OriginalValues`, every number a
  `Fraction`, on every corpus file, plus "every file the adapter accepts, the
  owned parse reads"); metamorphic (identical tables for thread counts 1, 2,
  7, 12, 24).
- **M8 first.** The adapter's phase split on the watch and the corpus, and
  NVIDIA's minimal ingress (`native/nvidia_ingress`) on the same inputs and
  lanes (§5.6, §27 #45). It is the unit's first task.
- **Fleet runs.** Phase split; parse throughput per file and thread count on
  both sockets; the model-oracle differential over the corpus.
- **Done when.** Unit plan [plans/I1-step-parse.md](plans/I1-step-parse.md)
  green; the per-phase budget table is in the unit report.

#### I2 units

- **Purpose.** Lift the mm-only gate: UR-1–UR-10 per context, exact constants
  (`.65`), the caller's unit against the file's (`.64`), and per-context units
  and uncertainty written into version 3's CONTEXTS rows.
- **Design.** §5.5, §5.2 units, §5.3 gate row.
- **Beads.** `.25.6`, `.25.15`.
- **Files.** `native/step_units.hpp`, `native/step_source/main.cpp:426-551`,
  the oracle placement translations (`python/brepgpu/imported_oracles.py`
  `original_frame`, `original_cartesian_operator`: the `.25.6` note); tests in
  `tests/test_step_ingress.py`.
- **Consumes.** P1's version 3 CONTEXTS table and `SOLIDS__.context`.
- **Produces.** Admitted non-mm inputs; CONTEXTS rows.
- **Oracle tier.** Metamorphic: the 86 Rat Rig inch/mm twins equal ×25.4
  within 1e-9 (oracle 8's row); spec: the 41 files whose 0.0174532925 degree
  snaps to π/180, both values recorded.
- **M8 first.** None.
- **Fleet runs.** Corpus admission census before and after (155 non-mm files).
- **Done when.** The non-mm files are admitted, or refused per context by
  name; the twins pass.

#### I3 raw-authority

- **Purpose.** The authority rule at the adapter: pcurves and declared masters
  read raw, missing pcurves written as absent, D-shapes representation
  selection on raw entities, shell closure computed from edge uses, raw values
  written wherever the adapter has them (gap G-4), and `--new-path` (version
  3, `source_uv` off).
- **Design.** §5.1, §5.2, §5.3 (`source_uv` leaves the new path), §5.4 row 11.
- **Beads.** `.25.5` (with its notes: the adapter's location at
  `native/step_source/main.cpp:1238-1245`, and `tests/test_placement_oracles.py:111`
  flipping when the raw-placement reader lands).
- **Files.** `native/step_source/main.cpp` (`:941` pcurves, `:666` the
  `source_uv` call, representation selection); tests in
  `tests/test_step_reader_raw.py`.
- **Consumes.** P1's version 3 COEDGES fields.
- **Produces.** `--new-path` stores; COEDGES `pcurve_presence` and `master`.
- **Oracle tier.** Spec: oracle 8's fixtures per override class produce the raw
  value and a record naming the class.
- **M8 first.** None.
- **Fleet runs.** Corpus census (the 14 leaf-product files); OM10's tables
  before and after.
- **Done when.** `.25.5` holds on the corpus; every raw value the adapter
  reads is the one it writes.

#### I4 refusal-catalog

- **Purpose.** Every row of §5.8 as a typed record with its reason code, in
  check order, per entity; exit 4 for a file-level refusal and exit 1 only for
  internal, I/O and device faults; the adapter's CPU-time and address-space
  rlimits (`ingress_budget`); one malformed-value fixture per row.
- **Design.** §5.8, §5.3 bounding the bridge, §7.2.
- **Beads.** `.25.1`.
- **Files.** `native/step_source/main.cpp` (`:1279-1280`, exit 1 today),
  `native/source_store/`, `native/fast_mesher/carriers/` (T1 rows);
  `tests/test_refusal_catalog.py`.
- **Consumes.** `bgsrc::Reason`, `RecordSink`, P4's T1 checks.
- **Produces.** Typed refusal records end to end; the exit codes of §7.2.
- **Oracle tier.** Spec: each fixture refuses exactly its entity with its
  reason, and the rest of the part meshes (§23.5 malformed values).
- **M8 first.** None.
- **Fleet runs.** Corpus refusals by reason, before and after.
- **Done when.** Every §5.8 row has its fixture and passes.

#### I5 step-decode

- **Purpose.** The owned reader's second half: entity table to source records
  without OCCT (geometry, topology, units, placements, styles), streamed per
  definition into L1 so that a definition's T1 starts as it decodes; the C++
  store writer; the ingress gate.
- **Design.** §5.6 (decision, streaming, what the owned path keeps),
  §5.1–§5.5, §22.3 ingress gate.
- **Beads.** `.25.7` (closes it).
- **Files.** `native/step_reader/decode/`; `native/source_store/writer.{hpp,cpp}`;
  `tests/test_step_decode.py`.
- **Consumes.** `bgstep::EntityTable`, I2's and I3's semantics, `bgfm::build_l1`,
  I1's budget table.
- **Produces.** `DecodeStream decode(const EntityTable&, const DecodeOptions&)`
  (one definition's records at a time), `std::vector<std::byte> write_store(const Store&)`.
- **Oracle tier.** Spec/metamorphic, oracle 8: the owned store is byte-identical
  to the bridge's (`--new-path`) on every accepted corpus file, and where the
  bridge has no raw value, equals the raw value.
- **M8 first.** I1's budget table.
- **Fleet runs.** Owned ingress against NVIDIA's minimal ingress per input and
  lane, under I8's protocol once it exists.
- **Done when.** The §22.3 ingress gate holds on the watch on both lanes.

#### I6 identity

- **Purpose.** The identity pass at ingress: header strings, product, revision
  and occurrence records, names, UUID sets read verbatim, `shape_aspect`
  groups, all in side tables keyed by `#N`; the decision for
  `uuid_relationship`, `uuid_provenance` and `uuid_context`.
- **Design.** §5.7, §17.3 group and occurrence rows.
- **Beads.** `.25.33`.
- **Files.** `native/step_reader/identity.{hpp,cpp}`; `tests/test_identity.py`.
- **Consumes.** `bgstep::EntityTable`; `Publication` (for the carriage rows).
- **Produces.** `IdentityTables identity(const EntityTable&)` (tagged table);
  their rows in `BGFMOUT1`.
- **Oracle tier.** Spec: authored UUID and `shape_aspect` fixtures; oracle 4's
  "every UUID read reaches output".
- **M8 first.** None.
- **Fleet runs.** G16 H1–H4 on the corpus (under 1% time, under 2% bytes, §27 #35).
- **Done when.** Every identity field of §5.7 reaches the internal output;
  `.25.33` is decided (a reader named, or the three entities dropped with a
  record).

#### I7 nvidia-harness

- **Purpose.** The NVIDIA harness generalised: arbitrary inventories and
  per-key chords, one definition-frame `.brep` per mesh key, the arms of
  §22.2, the screening protocol, chord tightening with achieved fidelity from
  oracles 1 and 16, validity as topology and finite points.
- **Design.** §22.2.
- **Beads.** `.25.20`.
- **Files.** `scripts/benchmark_om10_nvidia.py` (`:258`, `:324`, `:418-420`,
  `:461`, `:474` before B3's changes land), `scripts/qualify_om10_nvidia.py`,
  `examples/render_om10_nvidia.py` (`:806`); `tests/test_om10_nvidia.py`.
- **Consumes.** B3's scoring; V3 (oracle 1) and V6 (oracle 16); P19's key set.
- **Produces.** Arm outcomes per input with NVIDIA's measured δ and final chord.
- **Oracle tier.** Property: validity and achieved fidelity through V3 and V6;
  spec: the chord rule chord·max(½, 0.9·ε/δ) on authored measured δ.
- **M8 first.** None.
- **Fleet runs.** NVIDIA's arms on the watch and the corpus strata, both lanes.
- **Done when.** The harness takes corpus inventories; every arm's outcome is
  scored `pass`, `fail` or `not_evaluated`.

#### I8 headline-protocol

- **Purpose.** The owned headline timed fork to exit with ingress from the
  same bytes; one schedule per lane alternating owned and NVIDIA runs; the
  context row inside the timed interval (cycles and wall per lane CPU; GPU SM
  clock, throttle reasons, PCIe generation and width); the per-lane quiet
  gate with its measured threshold; the §22.5 scoring and claim; the
  reporting tool's refusal to mix link widths.
- **Design.** §22.3–§22.5.
- **Beads.** `.25.9`.
- **Files.** `scripts/measurement_lane.py`, `scripts/headline.py`;
  `tests/test_measurement_lane.py`, `tests/test_headline.py`.
- **Consumes.** I5, I7, `brepgpu-fm mesh`, B3.
- **Produces.** The headline series and its score table per stratum and lane.
- **Oracle tier.** Spec: the §22.5 score table as literal cases; the bootstrap
  claim on synthetic paired costs with known ratios.
- **M8 first.** The idle noise floor per lane (§22.4 item 4, §27 #47), as the
  unit's first task; runnable early (§9).
- **Fleet runs.** MS2's series.
- **Done when.** MS2 (§11).

#### I9 arena-runner

- **Purpose.** The arena as a standing mechanism: the 2^(8−2) fraction from its
  generators, follow-up arms as one-factor runs, strata and report classes
  fixed before results, seeded input samples, the per-run record, the
  adoption rule with fresh confirmation, the admitted fraction per stratum,
  and the expensive oracles run once per distinct output digest.
- **Design.** §23.1–§23.4, §23.6.
- **Beads.** `.25.27`.
- **Files.** `scripts/arena.py`; `tests/test_arena.py`.
- **Consumes.** I8, every unit's flags, V1–V9.
- **Produces.** Per-run records (tagged table) and adoption verdicts.
- **Oracle tier.** Spec: the generated fraction has resolution V (every word of
  the defining relation of length ≥ 5) and its 64 runs; the adoption rule on
  synthetic effects.
- **M8 first.** None.
- **Fleet runs.** The exploratory arena after MS1; the adoption arena after the
  gate lifts.
- **Done when.** The first exploratory arena has run, labelled "admitted set
  at T".

#### I10 family-catalogue

- **Purpose.** The authored families of §23.5 as runnable generators, each
  stating its parameters, the branch it forces, its expected outcome and the
  record that shows the branch fired, gathered from the units that wrote them.
- **Design.** §23.5.
- **Beads.** None.
- **Files.** `tests/families/` (one module per group), built on
  `tests/step_families.py` and the store writers.
- **Consumes.** The families each unit wrote.
- **Produces.** `families.generate(group, seed) -> [Family]`.
- **Oracle tier.** Spec: a family whose expected record is absent fails.
- **M8 first.** None.
- **Fleet runs.** Every family through `brepgpu-fm` and the oracles.
- **Done when.** Every §23.5 group is present and reaches its branch.

#### I11 fuzzer

- **Purpose.** Mutations over STEP and B-rep parameters, NaN and Inf injected
  after ingress, with the oracle of §23.5 and the timeout of §4.8; failures
  minimised into permanent regressions.
- **Design.** §23.5 fuzzer, §4.8 timeout.
- **Beads.** None.
- **Files.** `tests/fuzz/`; the `fuzz` profile of `tooling/native_cpu_rigor.cmake`.
- **Consumes.** I10, `brepgpu-fm`.
- **Produces.** Seeds, failures, minimised regressions.
- **Oracle tier.** Property: every output passes the stack, or carries refusal
  records for exactly the entities that read an injected value; no face that
  read one is `valid`.
- **M8 first.** None.
- **Fleet runs.** Fuzz campaigns on the fleet.
- **Done when.** A campaign runs with seeds logged and every failure minimised
  into a regression.

### 6.4 Verification units

#### V1 oracle-geometry

- **Purpose.** The oracles' own view of source geometry, independent of the
  producer: evaluators for every carrier and curve kind, a globally
  safeguarded projection returning distance bounds, the trim-domain builder
  of §24.1, and a trimmed-face distance query with bounds.
- **Design.** §24 (independence; the oracle's trim domain), §24.1 rows 1, 2,
  10 (what they will need).
- **Beads.** None.
- **Files.** Create `tests/oracles/` (`CMakeLists.txt`, `geometry/`),
  `tests/native_oracle_geometry_*.cpp`, `tests/native_oracle_geometry_fixture.py`,
  `tests/test_oracle_geometry.py`.
- **Consumes.** `bgsrc::read_store` (P1 Task 3).
- **Produces.** `bgor::OracleCarrier`, `bgor::OracleCurve`, `bgor::OracleFace`,
  `bgor::Bound {double lower, upper;}`,
  `bgor::PointBound distance_to_carrier(const OracleCarrier&, Vec3, const UvBox&, const Budget&)`,
  `bgor::PointBound distance_to_curve(...)`,
  `bgor::TrimDomain build_trim_domain(const OracleFace&, const Budget&, double width_target)`,
  `bgor::Membership classify(const TrimDomain&, Vec2)`,
  `bgor::PointBound distance_to_face(const OracleFace&, const TrimDomain&, Vec3, const Budget&)`,
  `bgor::OracleDefinition load_definition(const bgsrc::Store&, uint64_t definition_id)`.
- **Oracle tier.** Spec (closed forms; the exact fixtures of
  `papers/2026-10-13-projection-oracle-study.md` A–F); property (bounds hold
  against dense sampling).
- **M8 first.** None.
- **Fleet runs.** Trim-domain census over the watch's store and the admitted
  corpus: every face builds a domain or returns an outcome with a reason.
- **Done when.** Unit plan [plans/V1-oracle-geometry.md](plans/V1-oracle-geometry.md)
  green; the census is in the unit report.

#### V2 oracle-reconciliation

- **Purpose.** Oracle 10: for every chain sample and resolved vertex, the
  oracle's own projection onto each incident source carrier, the move from the
  source position, lfs recomputed from exact distance queries, and the fail
  conditions of §24.1 row 10; the model tier (a global solve with the same
  weights) offline on corpus items.
- **Design.** §24.1 row 10, §8.3–§8.5 (what it checks), §8.8.
- **Beads.** None.
- **Files.** `tests/oracles/reconcile/`, `brepgpu-oracle-reconcile`;
  `tests/native_oracle_reconcile_*.cpp`.
- **Consumes.** V1; the written chains (`BGFMOUT1`; until P18, P10's chain dump).
- **Produces.** `CheckOutcome` per sample and vertex with its record.
- **Oracle tier.** Property; model (the global solve).
- **M8 first.** None.
- **Fleet runs.** P10's families; the seeded corpus sample for the model tier.
- **Done when.** It decides (`pass` or `fail`, not `not_evaluated`) every
  sample and vertex of P10's families and of MS1's outputs. Its fail branches
  are reached by real outputs only (arena arms that drop a guarantee);
  hand-made defective chains wait for mutation testing (§24, §24.4).

#### V3 oracle-deviation

- **Purpose.** Oracle 1: two-sided deviation against the target rebuilt from
  source and the chains V2 verified, over V1's trim domain, every triangle
  including boundary, pole and apex triangles, collapse geometry against r_c,
  on the FP64 output at ε_f − δ_exp and on each written format at ε_f;
  NVIDIA's output against the source faces with d_e allowed within ℓ_f.
- **Design.** §24.1 row 1, §3.2, §3.3.
- **Beads.** None.
- **Files.** `tests/oracles/deviation/`, `brepgpu-oracle-deviation`;
  `tests/native_oracle_deviation_*.cpp`.
- **Consumes.** V1, V2; `BGFMOUT1`; NVIDIA's meshes (`nvidia_reference` BGREFM01).
- **Produces.** Per-face two-sided deviation bounds and the outcome.
- **Oracle tier.** Property.
- **M8 first.** None.
- **Fleet runs.** NVIDIA's recorded outputs (§9); MS1's outputs.
- **Done when.** It decides (`pass` or `fail`, not `not_evaluated`) every face
  of MS1's inputs and of NVIDIA's watch output.

#### V4 oracle-structure

- **Purpose.** Oracles 2 (exact per-face UV against V1's trim domain), 3
  (topology by vertex key, use counts, `open_by_refusal`, per-shell Euler,
  accounting), 4 (classification map) and 17 (counts on untrimmed analytic
  families), with the shared-edge checks of oracles 3 and 4 implemented once.
- **Design.** §24.1 rows 2, 3, 4, 17.
- **Beads.** `.25.32`.
- **Files.** `tests/oracles/structure/`, `brepgpu-oracle-structure`;
  `tests/native_oracle_structure_*.cpp`.
- **Consumes.** V1; `BGFMOUT1`; P12's chain dump before P18.
- **Produces.** Outcomes per face, shell and definition.
- **Oracle tier.** Property; spec (oracle 17's closed forms).
- **M8 first.** None.
- **Fleet runs.** NVIDIA's `void-block.step` outputs in 8 configurations and the
  definition-1808 duplicate class (§9).
- **Done when.** It fails NVIDIA's recorded invalid outputs for the reasons
  recorded (B2-ex) and decides every face of MS1's inputs.

#### V5 oracle-embedding

- **Purpose.** Oracle 5: offline self-intersection with the contact BVH and
  filtered predicates moved into the test suite (`native/mesher/contact.hpp`
  as specification), exemptions re-checked against the oracle's own source
  clearance.
- **Design.** §24.1 row 5, §3.7, §12.5.
- **Beads.** None.
- **Files.** `tests/oracles/embedding/`, `brepgpu-oracle-embedding`.
- **Consumes.** V1 (source clearance); `BGFMOUT1`.
- **Produces.** Forbidden pairs and failed exemptions.
- **Oracle tier.** Property.
- **M8 first.** None.
- **Fleet runs.** The embedding families; NVIDIA's inside-out fillet class.
- **Done when.** It decides every pair on the families and MS1's outputs.

#### V6 oracle-shading

- **Purpose.** Oracles 6 (normals (a)–(d)) and 16 (the angle term per triangle
  and per chain segment, with a covering bound from the span's normal cone,
  computed by the oracle).
- **Design.** §24.1 rows 6, 16; §15.1.
- **Beads.** None.
- **Files.** `tests/oracles/shading/`, `brepgpu-oracle-shading`.
- **Consumes.** V1 (normals and normal cones of its own); `BGFMOUT1`; glTF
  readback for (d) once P20 lands.
- **Produces.** Outcomes per triangle class.
- **Oracle tier.** Spec/property.
- **M8 first.** None.
- **Fleet runs.** G13's seven surfaces; NVIDIA's 25° arm before it is called
  matched (§22.2).
- **Done when.** It decides every triangle of the seven surfaces and of MS1's
  outputs.

#### V7 oracle-export

- **Purpose.** Oracle 7: per format and named consumer, parse and rebuild what
  PrusaSlicer, Cura, lib3mf, three.js and fTetWild hold; degeneracy,
  orientation, manifoldness by index, pinched vertices, exact
  self-intersection against the FP64 set, displacement within δ_exp.
- **Design.** §24.1 row 7, §18.5.
- **Beads.** None.
- **Files.** `tests/oracles/export/`, `brepgpu-oracle-export`.
- **Consumes.** P20's files; `BGFMOUT1`.
- **Produces.** New defects per consumer emulation.
- **Oracle tier.** Model.
- **M8 first.** None.
- **Fleet runs.** The 100 m family; G3's five placements.
- **Done when.** It decides every written file of the families.

#### V8 oracle-ingress

- **Purpose.** Oracle 8 whole: override-class fixtures, one fixture per §5.8
  row, native source records per operation kind, the meshed boundary against
  the raw master with a stated tolerance, owned S1/S2 against the adapter's
  verdicts, validation properties computed by us, the Rat Rig twins, and the
  owned reader against the bridge.
- **Design.** §24.1 row 8.
- **Beads.** `.25.26` (the boundary tolerance [call]: η of the key's ε_ref where
  the master is the prior; u_ctx where it is not).
- **Files.** `tests/test_oracle_ingress.py`, `tests/oracles/ingress/`.
- **Consumes.** I2–I5, P8, P21 outputs.
- **Produces.** Outcomes per fixture and per corpus file.
- **Oracle tier.** Spec/metamorphic.
- **M8 first.** None.
- **Fleet runs.** The corpus.
- **Done when.** Every row of §24.1 row 8 has its runner and passes.

#### V9 oracle-metamorphic

- **Purpose.** Oracles 11 (O-bytes per placement function with the
  perturbations of §24.2), 12 (FI-1, FI-2 and replay), 14 (scale by 4^k, far
  translation, rigid rotation, cluster counts) and 15 (each §4.7 row forced to
  fire), as fleet runners over the mesher CLI.
- **Design.** §24.1 rows 11, 12, 14, 15; §24.2.
- **Beads.** None.
- **Files.** `tests/oracles/metamorphic/` (runners), `tests/test_metamorphic.py`;
  a test-hook build option for oracle 15 (`BGFM_FORCE_STOP`), as
  `BREPGPU_ENABLE_TEST_HOOKS` does in `CMakeLists.txt`.
- **Consumes.** `brepgpu-fm mesh`, P6's `Lane`, P25's placement log (oracle 12
  replay).
- **Produces.** Byte-difference and status-difference reports.
- **Oracle tier.** Metamorphic; spec (oracle 15's rows).
- **M8 first.** None.
- **Fleet runs.** Thread counts 1, 2, an odd count, 12, 24 and 48; GPU0 against
  GPU1 once P24 lands.
- **Done when.** Oracles 11, 14 and 15 pass on MS1's inputs; 12 once P24 lands.

## 7. Sequencing and critical path

### 7.1 Dependencies that bind

| Constraint | Source | How the order meets it |
|---|---|---|
| Oracle 1, its trim-domain builder and oracle 10 are built before the code they judge | §24, §25 | V1 runs in wave 1; V2 lands before P10 (P10 starts after V2); V3 lands before P14 |
| The owned reader is on the critical path; no headline before it | §5.6, §22.3 | I1 runs in wave 1; I5 lands before I8's headline series; MS2 requires I5's gate |
| Measurements before the defaults that rely on them | §5.6, §12.6, §13.4, §22.6, §27 | adapter phase split is I1's first task; E1 census is P11's first task (runnable early); per-face work distribution precedes P15's C1/C2 default; stage budgets are revised from MS1's measurements before MS2; idle noise floor is I8's first task |
| Gate lifts before any arena run that adopts defaults | §23.4 | I2 (`.25.6`), I3 (`.25.5`), P8 (`.25.13`, `.25.12`) land before I9's first adoption run; earlier arena runs are labelled "admitted set at T" and re-run as gates lift |
| The old mesher retires only once the new path serves every consumer | §25 | P23 starts after P22 |
| At most three units at once | operator | §8 |
| The store format changes once | this plan, call 3 | P1 defines version 3 in wave 1; later units fill its fields in disjoint regions of the adapter (§3 shared edit points) |

### 7.2 The critical path

The producer chain is the long pole; ingress and the oracles run beside it with
slack, and the oracles stay one step ahead of the code they judge.

```
P1 ─▶ P4 ─▶ P5 ─▶ P7 ─▶ P8 ─▶ P10 ─▶ P12 ─▶ P14 ─▶ P18 ═▶ MS1
 │           │      ▲          ▲  ▲     ▲      ▲
 ├▶ P2 ─▶ P6 ┘      │          │  │     │      │
 │    └▶ P3 ────────┘   P9 ────┘  │     P13 ───┘
 │                      P11 ──────┘
V1 ─▶ V2 (before P10) ─▶ V3 (before P14)    V4 (before P12)
I1 ─▶ I2 ─▶ I3 ─▶ I4 ─▶ I5 (ingress gate) ─────────────┐
MS1 ─▶ P15, P16, P17, P19 ─▶ V5, V6, V9 ─▶ I7, I8 ─▶ MS2 (watch headline)
MS2 ─▶ I9 (gates I2, I3, P8 landed) ─▶ MS3 (corpus stratum)
P20, P21 ─▶ P22 ─▶ P23 (retire)
```

## 8. Waves

At most three units run at once. A slot frees when its unit lands; the next
unit in its queue whose dependencies have landed starts. Waves below are the
expected snapshots; the queues are the binding order.

**Wave 1 (named):** P1 source-view, V1 oracle-geometry, I1 step-parse. Two
hand-offs inside the wave: I1 consumes P1 Task 1 (`bgtt`, `bgsrc::Reason`)
from its Task 3, and V1's store loader (its Task 6) consumes P1 Task 3
(`bgsrc::read_store`). If P1 has not landed when either reaches that task, it
rebases onto P1's branch at the named commit. P1 runs those two tasks first.

| Slot | Queue after wave 1 |
|---|---|
| A (producer chain) | P4, P5, P7, P8, P10, P12, P14, P18, P15, P16, P17, P19, P20 |
| B (producer support) | P2, P3, P6, P9, P11, P13, V5, V6, V9, P21, P22 |
| C (ingress, oracles ahead of the code they judge, measurement) | I2, V2, I3, V4, V3, I4, I5, I6, V8, I7, I8, I9, I10, I11 |

Later units (P23–P31, V7) join the first free slot once their dependencies
land. The order puts each oracle ahead of the code it judges with slack: V2
lands before P10 starts, V4 before P12, V3 before P14. If a producer unit is
blocked on an oracle anyway, the next free slot takes that oracle first.

Expected snapshots: wave 2 {P4, P2, I2}; wave 3 {P5, P3 then P6, V2}; wave 4
{P7, P9, I3}; wave 5 {P8, P11, V4}; wave 6 {P10, P13, V3}; wave 7 {P12, I4 or
the next free unit}; then P14 and P18 to MS1.

## 9. Fleet work alongside delivery

The fleet runs measurements and arena permutations as their mechanisms land,
while the laptop keeps editing (§23, §2 execution). Every run is lane-bound
through `scripts/measurement_lane.py` (`make_lane`, `bound`) and recorded in
its unit's report; nothing is planned around calendar time.

| When | Run | Owner | Feeds |
|---|---|---|---|
| Wave 1 | Adapter phase split, watch and corpus, n = 3 per file, both sockets; NVIDIA minimal ingress on the same inputs | I1 Task 1 | owned reader budget (§5.6, §27 #45) |
| Wave 1 | Owned parse throughput and thread scaling; model-oracle differential over the corpus | I1 | I5 budget |
| Wave 1 | L1 census and trim-domain census over the watch's store and the corpus stores | P1, V1 | coverage of the oracle and the L1 layer |
| Wave 2 (once B3 lands) | NVIDIA watch arms (chord-only and 25°) on both lanes at n ≥ 10 under the fixed harness, labelled pre-oracle (no chord tightening yet) | coordinator, with B3's harness | the gap to beat, per lane |
| Wave 2 | E1 candidate census per class and closed-form coverage from the existing AABB pair path | P11 Task 1, run early | T7 per-pair target (§12.6, §27 #36) |
| Wave 2 | Idle noise floor per lane | I8 Task 1, run early | quiet-gate threshold (§27 #47) |
| After each gate lift | Corpus admission census per stratum and reason | I2, I3, P8 | admitted fraction (§23.4) |
| As each oracle lands | NVIDIA's recorded invalid outputs (`void-block.step` in 8 configurations, the definition-1808 duplicate class, the inside-out fillet class) scored by the new oracles | V3, V4, V5 | the fail branches of the output oracles (§24) |
| After P5 | Degree-bucket kernel microbenchmarks | P5 | §21.1 buckets (§27 #50) |
| After P5 + P13 | Per-face predicted work distribution and empty-band share on the strata, families and watch | P15 Task 1 | C1/C2 default (§13.4, §27 #9) |
| After P10 | B-ED-CL deferred-item counts per class; Newton and projection count distributions | P10 | batch sizes, §27 #44 |
| After MS1 | CPU-s per task class on the strata and the watch separately; per-task peak RSS against §4.6's arithmetic; `move_bound_exceeded` and S2-reversal counts | coordinator | stage budgets (§22.6, §27 #46), slab sizes (§20.1), §27 #27 |
| After MS1 + I9 | First arena, exploratory, labelled "admitted set at T" | I9 | follow-ups; nothing adopted |
| After the gate lifts | Adoption arena, then fresh confirmation at ≥ 10 processes on held-out inputs | I9 | defaults (§23.6) |
| After P24 + P25 | Calibration per device; F8 per lane | P25 | GPU arm configuration |
| After the stack settles | Mutation testing on the fleet | — | §24.4 |

## 10. Bead map

Every open bead under `brepgpu-12s.25`, plus `.37.14`.

| Bead | Title (short) | Unit |
|---|---|---|
| `.25.1` | per-entity refusal | I4 (P1 contributes the reader layer) |
| `.25.2` | release FP-contraction flags | P24 |
| `.25.3` | excise certification | P23 |
| `.25.4` | placements per D-placements | P19 |
| `.25.5` | raw-entity overrides | I3 |
| `.25.6` | lift mm-only admission | I2 |
| `.25.7` | owned STEP reader | I1, closed by I5 |
| `.25.8` | canonical CSR order in GPU adjacency if reused | **Deferred.** The new path does not reuse `native/topology_cuda.cu`: topology stays on CPU (§20.1 item 4). The bead applies to the ABI5 adjacency operation, which is revisited with the other operations after the mesher (§25) |
| `.25.9` | owned headline counts ingress | I8 |
| `.25.10` | revolution and extrusion evaluators | P4 |
| `.25.11` | mapped-item solids | P19 |
| `.25.12` | closed B-spline carriers with edge loops | P8 |
| `.25.13` | multiple-wire faces on curved carriers | P8 |
| `.25.14` | torus flags | P4 |
| `.25.15` | snap unit factors (`.65`) | I2 |
| `.25.16` | harness scores our failures as NVIDIA's | B3 (in flight) |
| `.25.17` | pre-fork cost charged to NVIDIA | B3 (in flight) |
| `.25.18` | own writeback counted as foreign load | B3 (in flight) |
| `.25.19` | discarded loaded runs recorded | B3 (in flight) |
| `.25.20` | harness for arbitrary inventories | I7 |
| `.25.21` | torus angle term at pole lines | P5 |
| `.25.22` | closure depth, zero-speed parameterisations | P12 |
| `.25.23` | carrier extension outside the UV domain | P12 |
| `.25.24` | fixed-point UV of record | P13 |
| `.25.25` | LOD oracle; mixed levels | P27 |
| `.25.26` | oracle 8 boundary tolerance | V8 |
| `.25.27` | expensive oracles once per digest | I9 |
| `.25.28` | item-level cancellation for spoliation | P25 |
| `.25.29` | definition-level lfs query | P9 |
| `.25.30` | coincidence decisions without u_ctx | P7 |
| `.25.31` | closed edges below δ_exp or the CDT quantum | P12 |
| `.25.32` | oracle 3 and 4 shared-edge checks once | V4 |
| `.25.33` | readers for the UUID relation entities | I6 |
| `.25.34` | per-frame cost for flexing parts | **Deferred.** Physics phase (§26.1, `.44`): no unit before the first corpus claim consumes per-frame remeshing, and the design reserves only the interfaces |
| `.37.14` | mesh a void-bearing source end to end | P18 |

## 11. Milestones

- **MS1, the first owned end-to-end mesh of a corpus part, checked by the
  oracles.** After P1–P14, P18 and V1–V4. Inputs: `void-block.step` and its
  void-free twin (`.37.14`), and one corpus part drawn by seed [call] from the
  analytic-only report class of the production-exports stratum among the
  files admitted at the time, never the watch. Checked by oracles 1, 2, 3, 4,
  10 and 17, each `pass` (not `not_evaluated`). Recorded: per-stage CPU-s,
  triangles, peak RSS. Then the first stage-budget revision (§22.6).
- **MS2, the first steel-manned headline on the watch's own stratum.** After
  MS1, P15, P16, P17, P19, V5, V6, V9 (oracles 11 and 14), I5 (the ingress
  gate holds), I7, I8 and B3. One declared CPU-only configuration (call 7),
  Render and Fabricate profiles for the two arms of §22.2, ≥ 10 fresh
  processes per side per lane, the §22.5 score, NVIDIA's measured δ and final
  chord beside every ratio. A loss is published as a loss.
- **MS3, the first corpus-stratum claim.** After MS2, I2, I3, P8 (the gate
  lifts), I4, I9 (defaults adopted after the gate lifts and confirmed on
  held-out inputs) and all of oracles 1–10 and 14–17 green per report class.
  The stratum [call]: production exports. The claim is a win only under §22.5's
  rule: the ratio's interval wholly above 1 and owned coverage no lower than
  NVIDIA's.

The old mesher's retirement (P23) follows P22 and does not gate any milestone.

## 12. Design gaps and code defects found while planning

**Design gaps** (each resolved here by a call; the design is not edited by M7):

- **G-1 (§5.2, §5.4, §8.1).** BGSRC001 version 2 cannot represent the new path's
  input: every COEDGES row carries a reconstructed pcurve (`reconstructed` = 1,
  `native/step_source/main.cpp:941`), with no declared master, no "absent"
  pcurve, no swept carriers, conics beyond the ellipse, polylines or torus
  flags, and units only as a file-wide `units_mm` = 1.0. The design names the
  store as the new mesher's input without its schema change. Call 3: P1
  defines version 3 whole; I2, I3 and P4 fill its fields.
- **G-2 (§24).** Oracle language, location and the independence boundary
  (may the oracle share the store reader? the evaluators?) are not stated.
  Call 4.
- **G-3 (§24.1 row 1).** "Sound deviation with a covering-radius correction or
  bounded Hausdorff" gives no rule for the oracle's own evaluation error or
  for the trim domain's approximation width. Call 5 (three-valued with
  bounds, refine, else `not_evaluated(budget)`); V1 carries a width bound per
  trim polyline and decides near-boundary points by 3D distance to the edge
  curves.
- **G-4 (§5.6 against §5.2).** The owned reader's oracle is "byte-identical
  source store against the bridge where the bridge agrees with raw", but the
  adapter writes OCCT's value wherever it agrees within the context's
  resolution (`placements::agree`, `native/step_placements.hpp:424`), so an
  owned reader writing raw bits differs by design. I3 makes the adapter write
  the raw value wherever it has one, which makes the byte oracle meaningful.
- **G-5 (§5.3 against the beads).** `.25.12` and `.25.13` describe `source_uv`
  refusals; the new path does not run `source_uv`. Their lift is I3's
  new-path mode plus P8's charts.
- **G-6 (§25).** The retired-consumer list omits `brepgpu-mesh` callers,
  `mesh_imported` and its scripts, `nvidia_reference.load_native_model_match`,
  `feature_bore_meshing.py` and the canonical `meshing.py`. P22 lists them.
- **G-7 (citations).** §19.1 cites `native/step_source/main.cpp:244` for the
  copying hash; the copy is at `:240`. Bead `.25.12` cites
  `native/source_uv/source_uv.cpp:306`; the refusals are at `:310`. Bead
  `.25.1` cites `native/mesher/source.cpp:642-643` for per-file refusal; those
  lines are coedge lookups, and every `invalid()` in that file throws for the
  whole file. The S1 and S2 corrections the adapter writes are at
  `main.cpp:1185-1192` (§5.3 cites the `source_uv` preparation they rely on).
- **G-8 (§24 against BLACKBIRD's JSMNTL).** "Nothing tests an oracle
  separately" sits beside "oracle 1, its trim-domain builder and oracle 10
  are build units", and every build unit owes red tests. Call 12 draws the
  line.

**Code defects:**

- **D-1.** CUDA 12.8 is a design constraint (§2) that no build enforces:
  `CMakeLists.txt:63`, `native/om10/CMakeLists.txt:45` and
  `native/mesher/CMakeLists.txt:158,617` call `find_package(CUDAToolkit REQUIRED)`
  with no version, unlike `tooling/native_cpu_rigor.cmake`, which refuses any
  Clang but 18.1.3. A different toolkit on the path builds silently. P24 pins
  it for the new targets (`find_package(CUDAToolkit 12.8 EXACT REQUIRED)` [call]);
  the old targets retire with P23. To be filed as a bead.
- **D-2.** The adapter copies its whole input before hashing
  (`native/step_source/main.cpp:240`, `auto data=input;`) after reading it whole
  (`:409`, 1 GiB cap), and `native/reference_source/main.cpp:119` does the same.
  Known to the design (§19.1); P1 fixes both with the streaming hash.

**Library proposals:** none.

## 13. Self-review record

Run per the writing-plans skill on this master plan.

- **Spec coverage.** §6.1's coverage paragraph maps every design section to a
  unit; §27's open questions are each settled by the arm or run its row names,
  and each of those runs belongs to a unit above (§9).
- **Step scan.** No bite-sized steps in this file, by the brief.
- **Type consistency.** Names used across units: `bgsrc::Store`,
  `bgsrc::read_store`, `bgsrc::Reason`, `bgfm::L1Definition`, `bgfm::ContentDigest`,
  `bgfm::CarrierRecord`, `bgfm::SpanBounds`, `bgfm::KeyRequest`,
  `bgfm::EdgeMeasurement`, `bgfm::EdgeResolution`, `bgfm::Chain`,
  `bgfm::FaceOutput`, `bgfm::Publication`, `bgor::TrimDomain`,
  `bgor::load_definition`; each is produced by exactly one unit.
- **Review focus.** Held per unit plan.
- **Proportion.** The master plan holds interfaces and order only.
