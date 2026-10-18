# I1 step-parse Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task (one unit per agent, at most three units at once, one independent review of the unit before it lands). Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Measure where the adapter's ingress time goes and set the owned reader's budget from it, then build the owned reader's first half: STEP bytes to an entity table, with parallel record splitting, identical across thread counts, at lexing speed.

**Architecture:** First the adapter (`native/step_source`) gains per-phase timers, and NVIDIA's minimal ingress (`native/nvidia_ingress`) is timed on the same inputs and lanes; the unit report turns both into the owned reader's per-phase budget (design §5.6). Then a new standalone CMake project `native/step_reader` (namespace `bgstep`, no OCCT) splits the DATA section into chunks, settles each chunk's lexical entry state by composing per-chunk state-transfer functions, parses records in parallel into a structure-of-arrays entity table sorted by entity id, and writes it as a tagged-table container for the oracle. The model oracle is the repository's independent Part 21 reader in `python/brepgpu/imported_oracles.py`, which converts every number to an exact `Fraction`.

**Tech Stack:** C++20, pinned Clang 18.1.3 with libstdc++ 13 (`std::from_chars` for binary64), `std::jthread`, CMake 3.28 + Ninja, CTest; Python 3 `unittest`; `scripts/measurement_lane.py` for lane-bound fleet timing; the pinned OCCT 8.0.1 adapter for the phase split only.

**Spec:** `docs/FAST_MESHER_DESIGN.md` §5.6 (the owned reader, its budget, its measurement), §5.7 (header strings read at ingress), §5.8 syntax rows (`nonfinite_value`, `malformed_record`), §4.2 (L1 is D0: identical bytes everywhere), §22.3 (ingress gate); master plan `docs/fast-mesher-campaign/M7-plan.md` (calls 6, 10; unit I1). Part 21 grammar: ISO 10303-21:2016 (edition 3) §5–§9, read for the exchange structure, value and string encodings.

**Grounding:**
- Ingress costs: `papers/2026-10-17-occt-ingress-translation-edits.md` §6 (ReadFile 9.4 MB/s; lexing lower bound about 300 MB/s on one core; the watch: lex 0.055 s, ReadFile 2.12 s, raw walk 0.064 s, read plus transfer 3.06 s).
- NVIDIA's minimal ingress and the symmetry rule: `docs/NVIDIA_REFERENCE_PREPARATION.md` "What counts as NVIDIA's time"; `native/nvidia_ingress/main.cpp`.
- Parallel splitting: the lexer is a five-state automaton (code, code after `/`, string, comment, comment after `*`); each chunk's transfer function maps entry state to exit state, and composing them left to right fixes every chunk's entry state (the standard parallel-prefix treatment of a finite-state lexer; simdjson's quote-parity pass is the same idea for JSON). Apostrophe escaping (`''`) keeps string parity exact.
- Number conversion: `std::from_chars` (`<charconv>`, libstdc++ 13) is correctly rounded for binary64.
- Model oracle: `python/brepgpu/imported_oracles.py:270-357` (`_step_text`, `_OriginalValues`, `original_records`), independent of every native reader.
- Authored STEP text: `SolidBuilder` and its mutators in `tests/test_step_reader_raw.py:811`, extracted to `tests/step_families.py` here (master plan call 10).

## Global Constraints

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

Unit-specific, from the master plan:
- `native/step_reader` links no OCCT.
- The entity table is D0: identical bytes for every thread count and chunk size.
- Header strings that may carry personal data (author, organization) are parsed into the table but never printed by any tool of this unit and never written into tracked files.
- The container uses P1's `bgtt::write_sections` and the refusal codes use P1's `bgsrc::Reason` (both from P1 Task 1, `native/source_store`). If P1 Task 1 has not landed when this unit reaches its Task 3, rebase onto P1's branch at that commit (master plan §8). No second writer or code list is written.

## Review Focus

1. **A `;`, `#12=` or `/*` inside a string, or a string or comment that spans a chunk boundary.** A person expects records split exactly where Part 21 splits them, whatever the chunk size. Test: `boundary_adversaries_every_chunk_size` in Task 2.
2. **Decimal spellings at the edges of binary64.** Expected: the correctly rounded value for every legal spelling, subnormals kept, overflow refused as `nonfinite_value` for that entity only. Test: `real_conversion_bits` in Task 3.
3. **A complex entity instance `#5=(A(...)B(...));`.** Expected: one record with two parts in file order, each part's parameters intact. Test: `complex_instance_parts` in Task 3.
4. **Two records with one entity id, or a record that does not end.** Expected: the duplicate pair refused as `malformed_record` with both byte offsets; an unterminated final record refuses the file; nothing silently dropped. Tests: `duplicate_ids_refuse_both`, `unterminated_refuses_file` in Task 3.
5. **The same file at 1, 2, 7, 12 and 24 threads, and at chunk sizes down to 64 bytes.** Expected: byte-identical tables. Test: `table_bytes_identical_across_threads` in Task 4 (authored) and the corpus run in Task 5.

---

### Task 1: The adapter's phase split and NVIDIA's minimal ingress, measured

**Files:**
- Modify: `native/step_source/main.cpp` (a `--phase-times` option; phase timers around the stages below)
- Test: `tests/test_step_ingress.py` (add `PhaseTimes`)
- Create: `results/i1-step-parse-<date>/REPORT.md` (the budget table; finished in Task 5)

**Interfaces:**
- Consumes: nothing new.
- Produces: with `--phase-times`, the adapter prints one stdout row per phase, `phase\t<name>\t<wall_s>\t<cpu_s>` (presentation on request, beside its existing stats rows), for the phases, accumulated over solids where they interleave: `read_hash` (`:409`), `occt_readfile` (`:411`), `occt_transfer` (`:414-428`), `raw_units_placements_styles` (`:430-551`, `:581`, `:1221-1252`), `raw_topology_walk` (`:627-973` less the two below), `source_uv` (`:666`), `orientation` (`:1069-1194`), `store_write` (`:1263-1274`). Wall from `CLOCK_MONOTONIC`, CPU from `CLOCK_PROCESS_CPUTIME_ID`. The phases partition the process from `main` to exit, and their sum is checked against the process wall the lane records.

- [ ] **Step 1: Write the failing test**

```python
class PhaseTimes(_Adapter, unittest.TestCase):
    def test_phases_partition_the_run(self):
        # extract tests/fixtures/void-block.step with --phase-times:
        # exactly the eight phase names above, once each, in that order;
        # every wall and cpu value finite and >= 0; the store bytes identical to a run without the option
```

- [ ] **Step 2: Run it to make sure it fails**

Run (fleet lane `lanes/I1/`): `PYTHONPATH=python:tests BREPGPU_STEP_SOURCE=build/om10-step-source/brepgpu-step-source python3 -m unittest tests.test_step_ingress.PhaseTimes -v`
Expected: FAIL (the adapter rejects `--phase-times`).

- [ ] **Step 3: Add the timers and the option**

- [ ] **Step 4: Run it to make sure it passes, then measure**

Run: the Step 2 command (PASS) and `python3 -m unittest tests.test_step_ingress -v` (unchanged otherwise). Then on both lanes (node 0 and node 1), cold (`--ingress-file` drops the input's pages), n = 3 per input, the watch and every admitted corpus file:
`python3 scripts/measurement_lane.py run --node N --context out/i1-adapter-context.tsv --label adapter-<input> --ingress-file <input> -- build/om10-step-source/brepgpu-step-source --phase-times <input> out/<input>.bgsource`
and NVIDIA's minimal ingress on the same inputs and lanes, in both of its shape-processing modes:
`python3 scripts/measurement_lane.py run --node N --context out/i1-nvidia-context.tsv --label nvidia-ingress-<mode>-<input> --ingress-file <input> -- build/nvidia-ingress/brepgpu-nvidia-ingress --shape-processing <none|default> <input> out/<input>-<mode>-brep/`
Expected: one row per (input, lane, repetition, phase); the report's table per stratum gives the median of each phase, the adapter total and NVIDIA's minimal ingress wall. The watch is its own stratum.

- [ ] **Step 5: Write the budget table and commit**

The budget [call]: for each input and lane, the owned ingress budget B is the faster of NVIDIA's two minimal-ingress medians on that input and lane (the stricter target; the §22.3 gate in I5 compares against the ingress of the NVIDIA arm actually selected for the headline, and both are recorded). It splits into parse, decode and store write in proportion to the adapter's medians for `occt_readfile`, for (`occt_transfer` + `raw_units_placements_styles` + `raw_topology_walk`), and for `store_write`. The phases the owned path drops (`source_uv`, `orientation`, design §5.6) receive no budget. The table goes into the unit report; I5's gate reads it.

```bash
git add native/step_source/main.cpp tests/test_step_ingress.py results/i1-step-parse-*/REPORT.md
git commit -m "I1: adapter phase split and the owned reader's ingress budget

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 2: Project scaffold, the lexical automaton and parallel chunk splitting

**Files:**
- Create: `native/step_reader/CMakeLists.txt`, `native/step_reader/lexer.hpp`, `native/step_reader/lexer.cpp`
- Test: `tests/native_step_reader_lexer.cpp`
- Modify: `scripts/check_native_face_domain.py` (add `--unit step-reader`: module `native/step_reader`, lint globs `tests/native_step_reader*.cpp`)

**Interfaces:**
- Consumes: nothing.
- Produces:
  - `namespace bgstep { enum class LexState : uint8_t { code, code_slash, string, comment, comment_star }; struct Transfer { std::array<LexState, 5> exit; }; Transfer transfer(std::span<const std::byte> chunk); }`
  - `struct Sections { uint64_t header_begin, header_end; std::vector<std::pair<uint64_t, uint64_t>> data; /* byte ranges, file order */ };` and `enum class FileFault : uint16_t { none, no_iso_header, no_header_section, no_data_section, unsupported_section, unterminated_string, unterminated_comment, unterminated_record, oversized };` `struct SplitResult { Sections sections; std::vector<uint64_t> record_starts; FileFault fault; uint64_t fault_offset; };`
  - `SplitResult split_records(std::span<const std::byte> file, uint32_t threads, uint64_t chunk_bytes);` — record starts are the first non-blank byte after the previous record's `;` in code state, in file order; chunk boundaries never change them.
- Sections [call]: one or more DATA sections are read in file order into one table; ANCHOR, REFERENCE and SIGNATURE sections refuse the file (`unsupported_section`, reason `malformed_record` with the section's name as a typed field), and the census counts them.

- [ ] **Step 1: Write the failing tests**

```cpp
// tests/native_step_reader_lexer.cpp
void transfer_composition();          // for 1,000 seeded random byte strings over the alphabet {', /, *, ;, #, a, \n}:
                                      // transfer(whole) == compose(transfer(left), transfer(right)) at every cut
void boundary_adversaries_every_chunk_size(); // an authored DATA section whose strings hold ";", "#12=", "/*" and "*/",
                                      // a comment holding "'" and ";", a string of 10,000 bytes, a comment across a
                                      // line: for chunk_bytes in {1, 2, 3, 7, 64, 4096} and threads in {1, 2, 7}:
                                      // record_starts equal the single-threaded scan's
void sections_found();                // HEADER and DATA ranges of an authored file; two DATA sections both kept
void file_faults();                   // no "ISO-10303-21;" -> no_iso_header; an unterminated string -> unterminated_string
                                      // at its opening quote; "ANCHOR;" -> unsupported_section
```

- [ ] **Step 2: Run them to make sure they fail**

Run: `python3 scripts/check_native_face_domain.py --unit step-reader --profile debug`
Expected: build fails: `lexer.hpp` not found.

- [ ] **Step 3: Implement `transfer` and `split_records`**

Pass one runs `transfer` on each chunk on its own thread; the entry states come from composing the transfers left to right; pass two scans each chunk from its known entry state and emits the record starts that fall inside it. `CMakeLists.txt` follows `native/mesher/CMakeLists.txt:1-12` (rigor profile, CTest), one static library `brepgpu_step_reader`.

- [ ] **Step 4: Run them to make sure they pass**

Run: `python3 scripts/check_native_face_domain.py --unit step-reader --profile debug --lint`, then `--profile tsan`.
Expected: `100% tests passed`; TSan clean.

- [ ] **Step 5: Commit**

```bash
git add native/step_reader tests/native_step_reader_lexer.cpp scripts/check_native_face_domain.py
git commit -m "I1: Part 21 lexical automaton and parallel record splitting

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 3: Records into the entity table

**Files:**
- Create: `native/step_reader/entity_table.hpp`, `native/step_reader/parse.hpp`, `native/step_reader/parse.cpp`, `native/step_reader/header.hpp`, `native/step_reader/header.cpp`
- Test: `tests/native_step_reader_parse.cpp`

**Interfaces:**
- Consumes: Task 2; `bgsrc::Reason` (`native/source_store/reasons.hpp`).
- Produces:
  - `enum class ValueKind : uint8_t { integer, real, string, binary, enumeration, reference, unset /* $ */, derived /* * */, list, typed };`
  - `struct Value { ValueKind kind; uint32_t count /* list, typed: number of direct children that follow in preorder */; uint64_t payload; };` — integer: the int64 bits; real: the binary64 bits; string, binary, enumeration, typed keyword: offset and length into `text` packed as (u32, u32); reference: the entity id.
  - `enum class RecordStatus : uint8_t { ok, malformed, nonfinite, duplicate };`
  - `struct Part { uint32_t keyword; uint64_t first_value; uint32_t value_count; };`
  - `struct Record { uint64_t id; uint32_t keyword /* UINT32_MAX: complex */; uint64_t first_value; uint32_t value_count; uint32_t first_part, part_count; uint64_t offset; RecordStatus status; };`
  - `struct EntityTable { std::vector<Record> records /* sorted by id */; std::vector<Part> parts; std::vector<Value> values; std::vector<char> text; std::vector<std::pair<uint32_t, uint32_t>> keywords /* interned, offset and length */; };`
  - `struct HeaderSection { std::vector<std::string> description, schema; std::string implementation_level, name, time_stamp, preprocessor, originating_system, authorization; std::vector<std::string> author, organization; };`
  - `struct ParseOptions { uint32_t threads = 1; uint64_t chunk_bytes = uint64_t{1} << 20; uint64_t max_bytes = uint64_t{1} << 32; };`
  - `struct Refusal { uint64_t id; bgsrc::Reason reason; uint64_t offset; };`
  - `struct ParseResult { EntityTable table; HeaderSection header; FileFault fault; uint64_t fault_offset; std::vector<Refusal> refusals; };`
  - `ParseResult parse(std::span<const std::byte> file, const ParseOptions&);`
  - `std::u8string decode_string(std::string_view raw);` (the `\X\`, `\X2\…\X0\`, `\X4\…\X0\`, `\S\`, `\P?\`, `\N\`, `\T\` directives and `\\`; apostrophes are already un-doubled in `text`)
- Rules: a real token is a Part 21 real (digits, `.`, optional digits, optional exponent); `.5` and a `D` exponent are not Part 21 and make the record malformed; overflow to ±∞ makes the record `nonfinite` (refusal `nonfinite_value`); underflow keeps the correctly rounded subnormal or zero; `.T.`, `.F.`, `.U.` are enumerations; an id seen twice marks both records `duplicate` (refusal `malformed_record`, both offsets).

- [ ] **Step 1: Write the failing tests**

```cpp
void real_conversion_bits() {   // each spelling inside "#1=A(<x>);": the value's payload bits
  expect_bits("0.1", 0x3FB999999999999Aull);
  expect_bits("-0.0", 0x8000000000000000ull);
  expect_bits("1.E3", 0x408F400000000000ull);
  expect_bits("1.", 0x3FF0000000000000ull);
  expect_bits("2.2250738585072011E-308", 0x000FFFFFFFFFFFFFull);   // largest subnormal
  expect_bits("2.2250738585072012E-308", 0x0010000000000000ull);   // DBL_MIN
  expect_bits("1.E-320", 0x00000000000007E8ull);                   // subnormal kept
  expect_bits("1.7976931348623157E308", 0x7FEFFFFFFFFFFFFFull);    // DBL_MAX
  expect_refused("1.7976931348623159E308", bgsrc::Reason::nonfinite_value); // rounds to infinity
  expect_refused("1.E400", bgsrc::Reason::nonfinite_value);
  expect_refused(".5", bgsrc::Reason::malformed_record);
  expect_refused("1.0D0", bgsrc::Reason::malformed_record);
}
void integer_and_reference_values();   // "#7=B(42,-3,#12,$,*,.T.,'it''s',\"0FF\");" -> kinds and payloads in order
void nested_lists_and_typed();         // "#8=C((1.,(2.,3.)),LENGTH_MEASURE(5.));" -> preorder with counts 2, 2 and typed count 1
void complex_instance_parts();         // "#5=(A(1)B(#2,'x'));" -> keyword UINT32_MAX, two parts A then B, values intact
void duplicate_ids_refuse_both();      // two "#9=" records: both duplicate, two refusals malformed_record with their offsets
void unterminated_refuses_file();      // last record without ";" -> FileFault::unterminated_record, empty table
void strings_decode();                 // decode_string("\\X2\\00E9\\X0\\") == u8"é"; "\\S\\a" == u8"á"; "\\X\\41" == u8"A"
void header_fields();                  // FILE_DESCRIPTION, FILE_NAME and FILE_SCHEMA of an authored header into HeaderSection
void records_sorted_by_id();           // records written #30, #2, #11 come out 2, 11, 30
```

- [ ] **Step 2: Run them to make sure they fail**

Run: `python3 scripts/check_native_face_domain.py --unit step-reader --profile debug`
Expected: build fails: `parse.hpp` not found.

- [ ] **Step 3: Implement `parse`, `decode_string` and the header reader**

`native/step_reader/CMakeLists.txt` gains `add_subdirectory(../source_store source_store)` and links `brepgpu_source_store`. Each thread parses the records whose starts fall in its chunk into its own buffers; the buffers are concatenated in chunk order, then records are sorted by id with value ranges remapped. Keyword interning is per chunk, then merged in sorted keyword order, so keyword indices do not depend on the thread count.

- [ ] **Step 4: Run them to make sure they pass**

Run: `python3 scripts/check_native_face_domain.py --unit step-reader --profile debug --lint`, then `--profile asan`.
Expected: `100% tests passed`.

- [ ] **Step 5: Commit**

```bash
git add native/step_reader tests/native_step_reader_parse.cpp
git commit -m "I1: Part 21 records into a sorted structure-of-arrays entity table

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 4: The table container, the CLI and determinism across threads

**Files:**
- Create: `native/step_reader/container.hpp`, `native/step_reader/container.cpp`, `native/step_reader/main.cpp` (`brepgpu-step-parse INPUT.stp [OUTPUT.bgp21] [--threads N] [--chunk-bytes N] [--stats]`)
- Create: `tests/step_families.py` (moved from `tests/test_step_reader_raw.py:811` onward: `SolidBuilder`, `substituted`, `appended`, `with_unit`, `moved`, `turned` and the other text mutators, unchanged)
- Modify: `tests/test_step_reader_raw.py` (import them from `step_families`)
- Test: `tests/native_step_reader_container.cpp`, `tests/test_step_parse.py`

**Interfaces:**
- Consumes: Tasks 2–3; `bgtt::write_sections`, `bgtt::split_sections` (P1 Task 1).
- Produces:
  - The container `BGP21001` version 1 (tagged-table layout of master plan §5): header (magic, version, SHA-256 of the input, record count) and sections `RECORDS_`, `PARTS___`, `VALUES__`, `TEXT____`, `KEYWORDS`, `HEADER__`, `REFUSALS`, each row the struct's fields in declaration order, little-endian. The header section never carries `author` or `organization` into any printed output.
  - `std::vector<std::byte> write_table(const ParseResult&);` `std::optional<ParseResult> read_table(std::span<const std::byte>);`
  - `brepgpu-step-parse` exits 0 on a parsed file (entity refusals are records, not exit codes), 4 on a file-level fault; `--stats` prints MB, records, values, refusals, threads, wall and CPU (presentation).

- [ ] **Step 1: Write the failing tests**

```cpp
void container_roundtrip();                    // write_table(parse(x)) then read_table: every vector equal
void table_bytes_identical_across_threads();   // every authored file of Tasks 2-3 and a 2 MB generated file of 20,000
                                               // records with long strings: write_table bytes equal for threads
                                               // {1, 2, 7, 12, 24} and chunk_bytes {64, 4096, 1 MiB}
```

```python
# tests/test_step_parse.py  (fleet only; BREPGPU_STEP_PARSE)
class Families(unittest.TestCase):
    def test_step_families_unchanged(self):
        # every SolidBuilder family used by tests/test_step_reader_raw.py produces the same text as before the move
        # (digests of the generated texts pinned from the base commit)
    def test_cli_exit_codes(self):
        # an authored file -> exit 0; the same file without "ENDSEC;" after DATA -> exit 4
```

- [ ] **Step 2: Run them to make sure they fail**

Run: `python3 scripts/check_native_face_domain.py --unit step-reader --profile debug`; `PYTHONPATH=python:tests BREPGPU_STEP_PARSE=build/step-reader-debug/brepgpu-step-parse python3 -m unittest tests.test_step_parse -v`
Expected: build fails (`container.hpp` not found); the Python module fails to import `step_families`.

- [ ] **Step 3: Implement the container and the CLI; move the builders**

- [ ] **Step 4: Run them to make sure they pass**

Run: the Step 2 commands, then `python3 -m unittest tests.test_step_reader_raw -v` (unchanged after the move).
Expected: all PASS.

- [ ] **Step 5: Commit**

```bash
git add native/step_reader tests/native_step_reader_container.cpp tests/step_families.py tests/test_step_reader_raw.py tests/test_step_parse.py
git commit -m "I1: entity-table container, brepgpu-step-parse and thread-count determinism

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 5: The model oracle over the corpus, and throughput

**Files:**
- Create: `tests/step_parse_reference.py` (reads `BGP21001`; parses the same file with `imported_oracles._OriginalValues` over every record, without `original_records`' type filter, and compares)
- Modify: `tests/test_step_parse.py` (add `ReferenceDifferential`, `Corpus`)
- Modify: `results/i1-step-parse-<date>/REPORT.md`

**Interfaces:**
- Consumes: `brepgpu-step-parse`, `python/brepgpu/imported_oracles.py` (`_OriginalValues`, `_Ref`, `_Call`, `_step_text`).
- Produces: `compare(step_path, table_path) -> Differences` with one row per mismatching record (id, what differs). Comparison rules: record ids and keywords equal; parameter trees equal in shape; a reference equals `_Ref(id)`; `$` and `*` both equal `None`; a string equals the reference's decoded text through `decode_string`; an integer equals its `Fraction`; a real's binary64 equals `float(Fraction)` exactly (correctly rounded on both sides); an owned refusal is a difference unless the reference also rejects the record.

- [ ] **Step 1: Write the failing tests**

```python
class ReferenceDifferential(unittest.TestCase):
    def test_authored_files_agree(self):
        # every family text from step_families and every fixture in tests/fixtures/*.step:
        # compare(...) returns no differences
    def test_difference_is_reported(self):
        # a table whose one real payload is flipped by one bit (written by the test into a temporary file):
        # exactly one difference, naming that record
class Corpus(unittest.TestCase):          # skipUnless BREPGPU_CORPUS_LIST names the admitted corpus files on the fleet
    def test_corpus_agrees_and_parses_every_bridge_file(self):
        # for every file: brepgpu-step-parse at --threads 12 exits 0 whenever the adapter accepts it;
        # compare(...) has no differences; the table bytes equal those at --threads 1
```

- [ ] **Step 2: Run them to make sure they fail**

Run: `PYTHONPATH=python:tests BREPGPU_STEP_PARSE=build/step-reader-release/brepgpu-step-parse python3 -m unittest tests.test_step_parse.ReferenceDifferential -v`
Expected: FAIL (`step_parse_reference` not importable).

- [ ] **Step 3: Implement `tests/step_parse_reference.py`**

- [ ] **Step 4: Run them to make sure they pass, then the corpus and throughput runs**

Run: the Step 2 command (PASS); then `BREPGPU_CORPUS_LIST=out/corpus-admitted.txt python3 -m unittest tests.test_step_parse.Corpus -v` (PASS, every difference either fixed or shown to be the reference's own error and filed as a bead). Then throughput, cold, on both lanes, n = 3, threads in {1, 2, 6, 12, 24}:
`python3 scripts/measurement_lane.py run --node N --context out/i1-parse-context.tsv --label parse-<input>-t<threads> --ingress-file <input> -- build/step-reader-release/brepgpu-step-parse <input> --threads <threads> --stats`
Expected: the report gives MB/s per stratum and thread count, the watch on its own, against the lexing bound of G1 §6 and against the parse budget of Task 1. A file the adapter accepts and the owned parse refuses is a defect of this unit, fixed here.

- [ ] **Step 5: Finish the unit report and commit**

`results/i1-step-parse-<date>/REPORT.md`: grounding, per task red and green evidence, the phase split and budget table (Task 1), the differential result, the throughput table, suites and counts, fleet-relative paths only, no header author or organization values.

```bash
git add tests/step_parse_reference.py tests/test_step_parse.py results/i1-step-parse-*/REPORT.md
git commit -m "I1: model-oracle differential over the corpus and parse throughput

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

## Self-review

- **Spec coverage.** §5.6's measurement before the reader (Task 1) and its budget (Task 1 Step 5), parallel record splitting (Task 2), the owned parse (Task 3), the oracle "against the bridge where the bridge agrees, else the raw value" is I5's for the store; this unit's parse oracle is the exact model reader (Task 5) plus "every file the bridge reads, the owned parse reads" (Task 5); §5.7 header strings (Task 3); §5.8 `nonfinite_value` and `malformed_record` at entity scope (Task 3); D0 determinism (Tasks 4, 5).
- **Step scan.** Each code step names its files and signatures; each test step carries its expected values; each run step its command and expected outcome.
- **Type consistency.** `bgstep::EntityTable`, `bgstep::ParseResult`, `bgstep::parse`, `bgstep::HeaderSection` match the master plan's I1 block; refusals use P1's `bgsrc::Reason` and the container P1's `bgtt`.
- **Review focus.** Each line has its test in the task named.
- **Proportion.** Signatures, rules and assertions only.
