# P1 source-view Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task (one unit per agent, at most three units at once, one independent review of the unit before it lands). Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give the new path its source input: store version 3, one owned C++20 reader of it that refuses per entity, content digests, and the mesher's L1 layer of decoded definitions with topology, use counts and shell closure.

**Architecture:** A new standalone CMake project `native/source_store` (namespaces `bgsha`, `bgtt`, `bgsrc`) holds the streaming SHA-256, the tagged-table container I/O, the §5.8 reason codes and the store reader. A new project `native/fast_mesher` (namespace `bgfm`) starts with `l1/`: Merkle content digests over raw IEEE bits and the L1 build. The adapter writes version 3 only under `--store-version 3`; version 2 stays its default for the old mesher's reader until P23.

**Tech Stack:** C++20, pinned Clang 18.1.3 with libstdc++ 13, CMake 3.28 + Ninja, CTest; Python 3 `unittest` fleet drivers; the pinned OCCT 8.0.1 adapter (`native/step_source`) for the version 3 writer.

**Spec:** `docs/FAST_MESHER_DESIGN.md` §4.2 (L1, D0), §4.4, §4.6, §3.5, §3.11, §5.4 rows 4, 9, 10, 12, 13, 15, §5.5 UR-1 and UR-7, §5.8, §7.3, §16.2, §19.1; master plan `docs/fast-mesher-campaign/M7-plan.md` (calls 2, 3, 8, 9, 10; unit P1).

**Grounding:**
- SHA-256: FIPS 180-4 (2015) §6.2; test vectors from NIST's SHA examples (the four used below). The adapter's own round function (`native/step_source/main.cpp:229-252`) is the specification being made streaming.
- Store layout: `native/step_source/main.cpp:212-225` (`Bytes`), `:1271-1274` (tags, header, atomic rename); the old reader `native/mesher/source.cpp:182-404` (`decode_row`) and `:405-470` (structural checks) as specification; `python/brepgpu/imported_source.py:17-19, 537-795`.
- Edge-use direction: the adapter writes COEDGES `sense` as `forward = (oriented_edge orientation == bound orientation)` (`main.cpp:914-966`) and checks closure per shell on those raw senses (`:1094-1118`), never composed with `same_sense`. L1 uses the same convention.
- Canonical loop rotation: Booth (1980), "Lexicographically least circular substrings", IPL 10(4), the O(n) least rotation, applied to the sequence of coedge digests.
- Content identity: §16.2 (Merkle digest of the definition's records by content, raw IEEE bits, no `#id`).

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
- `sha256.hpp` must also compile as C++17 (the adapter and the reference exporter are C++17 projects).
- Version 3 is written only under `--store-version 3`; the adapter's default output and the old reader (`native/mesher/source.cpp`) stay at version 2.
- The new reader reads version 3 only. The Python reader reads 2 and 3 until P23.
- No `#id`, file digest, name or UUID enters any content digest (§16.2).

## Review Focus

1. **A loop given from a different starting coedge, or reversed.** A person expects the same loop from any start (equal digest) and a reversed loop to be a different loop (different digest). Test: `loop_rotation_invariant_reflection_distinct` in Task 4.
2. **A seam: one edge used twice by one face.** Expected class `seam`, never `manifold` or `non_manifold`; the same seam with both uses in one direction is `same_direction_pair`. Test: `seam_and_misoriented_seam` in Task 5.
3. **A face on a carrier the reader refused (a NaN frame or control value).** Expected: that face refused with reason `prerequisite`, its definition `incomplete`, every other face of the definition intact. Test: `refused_carrier_refuses_only_its_faces` in Task 5.
4. **A pole face: a vertex loop with no coedges.** Expected: carried, not refused, no edge uses, its pole vertex in the definition. Test: `vertex_loop_face_carried` in Task 5.
5. **Trailing bytes after the last section, or a section cut short.** Expected: a file-level fault naming the offset, and no partial store. Test: `trailing_and_truncated_refuse_file` in Task 3.

---

### Task 1: Project scaffold, streaming SHA-256, tagged-table container, reason codes

**Files:**
- Create: `native/source_store/CMakeLists.txt`
- Create: `native/source_store/sha256.hpp`
- Create: `native/source_store/reasons.hpp` (created here because I1 and V1 consume it early)
- Create: `native/source_store/tagged_table.hpp`, `native/source_store/tagged_table.cpp`
- Test: `tests/native_source_store_sha256.cpp`, `tests/native_source_store_container.cpp`
- Modify: `scripts/check_native_face_domain.py` (add `--unit source-store`: module `native/source_store`, lint globs `tests/native_source_store*.cpp`)

**Interfaces:**
- Consumes: nothing.
- Produces:
  - `namespace bgsha { using Digest = std::array<std::byte, 32>; class Sha256 { public: void update(const void* data, std::size_t size) noexcept; void update(std::span<const std::byte>) noexcept /* C++20 only */; Digest finish() noexcept; }; Digest sha256(const void* data, std::size_t size) noexcept; }`
  - `namespace bgtt { using Tag = std::array<char, 8>; struct SectionView { Tag tag; uint64_t rows; std::span<const std::byte> payload; }; enum class Fault : uint16_t { none, truncated, section_order, row_overrun, oversized, trailing_bytes }; struct SplitResult { std::vector<SectionView> sections; Fault fault; uint64_t offset; }; SplitResult split_sections(std::span<const std::byte> body, std::span<const Tag> expected_order, uint64_t max_rows); class RowReader { /* u32 i32 u64 f64 text vec_f64 vec_u32 vec_u64 take end; each records a fault instead of throwing */ }; class RowWriter; std::vector<std::byte> write_sections(std::span<const Tag>, std::span<const std::vector<std::vector<std::byte>>> rows); }`
  - The section layout is the BGSRC one: 8-byte tag, u64 row count, u64 payload bytes, then rows of u64 length plus bytes, little-endian.
  - `namespace bgsrc { enum class Reason : uint16_t { nonfinite_value = 1, malformed_record, unit_unresolved, invalid_request, translation_mismatch, empty_edge_range, coincident_source_entities, placement_cycle, occurrence_budget, nonpositive_radius, cone_angle_out_of_range, degenerate_frame, zero_extrusion, nonpositive_axis, nonpositive_weight, degree_above_reader_limit, invalid_knots, undeclared_singularity, ingress_budget, prerequisite }; std::string_view name(Reason); }` — every §5.8 code, append-only, plus `prerequisite` for propagated refusals. A test checks `name()` for every value.

- [ ] **Step 1: Write the failing tests**

```cpp
// tests/native_source_store_sha256.cpp
#include "sha256.hpp"
// hex(Digest) -> std::string helper, require(cond, msg) throws
void fips_vectors() {
  require(hex(bgsha::sha256("", 0)) ==
          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "empty");
  require(hex(bgsha::sha256("abc", 3)) ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "abc");
  const std::string m448 = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
  require(hex(bgsha::sha256(m448.data(), m448.size())) ==
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", "448-bit");
  bgsha::Sha256 h; const std::string a(1000, 'a');
  for (int i = 0; i < 1000; ++i) h.update(a.data(), a.size());
  require(hex(h.finish()) ==
          "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0", "million a");
}
void streaming_split_invariant() {   // every split point of m448 gives the one digest
  for (std::size_t cut = 0; cut <= m448.size(); ++cut) { /* update(prefix); update(suffix) */
    require(hex(split_digest(m448, cut)) == hex(bgsha::sha256(m448.data(), m448.size())), "split"); }
}
```

```cpp
// tests/native_source_store_container.cpp
void roundtrip_two_sections();        // write A(2 rows), B(0 rows); split gives tags, counts, exact row bytes
void truncated_payload_faults();      // cut 1 byte inside B's payload -> Fault::truncated, offset of B's header
void section_order_faults();          // B before A -> Fault::section_order
void row_overrun_faults();            // a row length larger than the remaining payload -> Fault::row_overrun
void trailing_bytes_fault();          // one extra byte after the last section -> Fault::trailing_bytes
void rowreader_records_fault();       // reading f64 past the row end sets a fault, returns 0, never throws
void every_reason_named();            // name(r) is non-empty and distinct for every Reason value 1..20
```

- [ ] **Step 2: Run them to make sure they fail**

Run (fleet lane `lanes/P1/`, after `git ls-files | rsync --files-from=- . <lane>`): `python3 scripts/check_native_face_domain.py --unit source-store --profile debug`
Expected: the build fails: `sha256.hpp` and `tagged_table.hpp` not found (the unit and its CTest entries `native_source_store_sha256`, `native_source_store_container` exist; their sources do not).

- [ ] **Step 3: Implement `bgsha::Sha256` in `native/source_store/sha256.hpp` and `bgtt` in `tagged_table.{hpp,cpp}`**

`Sha256` keeps the 64-byte block buffer and the bit count; `update` consumes the caller's bytes in place (no copy of the input). `split_sections` never throws; every fault returns with its byte offset. `CMakeLists.txt` follows `native/mesher/CMakeLists.txt:1-12`: `include(../../tooling/native_cpu_rigor.cmake)`, `include(CTest)`, one static library `brepgpu_source_store`, test executables built from `../../tests/native_source_store_*.cpp`, each registered with `add_test`.

- [ ] **Step 4: Run them to make sure they pass**

Run: `python3 scripts/check_native_face_domain.py --unit source-store --profile debug --lint`
Expected: `100% tests passed`, clang-format and clang-tidy clean. Then `--profile asan`: same.

- [ ] **Step 5: Commit**

```bash
git add native/source_store tests/native_source_store_sha256.cpp tests/native_source_store_container.cpp scripts/check_native_face_domain.py
git commit -m "P1: streaming SHA-256, tagged-table container and 5.8 reason codes

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 2: Store version 3 in Python and in the adapter

**Files:**
- Modify: `python/brepgpu/imported_source.py` (read versions 2 and 3; write either; version 3 fields with defaults)
- Modify: `native/step_source/main.cpp` (`--store-version 3` at the writer, `:1263-1274`; row fields appended where each row is built)
- Test: `tests/test_source_store_v3.py`

**Interfaces:**
- Consumes: the version 2 layout as written today.
- Produces the version 3 layout. Every version 2 row keeps its fields in order; version 3 appends:

| Section | Appended fields (little-endian) | Default the adapter writes until the filling unit lands |
|---|---|---|
| header | version u32 = 3; section count 15 | — |
| `SOLIDS__` | u64 `context` (a `CONTEXTS` id, 0 = none) | 0 (I2 fills) |
| `COEDGES_` | u32 `pcurve_presence` (0 absent, 1 file, 2 reconstructed); u32 `master` (0 unspecified, 1 the edge's 3D curve, 2 this coedge's pcurve, 3 another face's pcurve; I3 maps the raw `pcurve_s1`/`pcurve_s2` onto 2 or 3 per coedge) | 2, 0 (I3 fills). Rule: presence 0 ⇔ pcurve id 0, range (0, 0), map kind 0, empty map |
| `CARRIERS` | u32 `flags` (bit 0 major ≤ minor radius, bit 1 degenerate torus, bit 2 `select_outer`, bit 3 offset rewritten); u64 `profile_curve` (a `CURVES` id, nonzero iff kind 7 or 8); 3×f64 `sweep` (kind 8: the extrusion vector with its raw magnitude, else 0) | 0, 0, 0 (P4 fills) |
| `CARRIERS` kinds | 7 revolution (frame origin on the axis, frame z the axis direction), 8 extrusion | not written (P4) |
| `CURVES__` kinds | 6 hyperbola (radii = semi-axis, imaginary semi-axis), 7 parabola (radii = focal distance, 0), in dimension 2 or 3 | not written (P4) |
| `CONTEXTS` (new, last) | u64 id; f64 `length_factor_mm`; f64 `angle_factor_rad`; f64 `uncertainty_mm`; u32 `uncertainty_usable`; u32 `snapped` (bit 0 length, bit 1 angle, UR-3); text `length_unit`; text `angle_unit` | no rows (I2 fills) |

  Python: `SolidDefinition.context_id = 0`, `Coedge.pcurve_presence = 2`, `Coedge.master = 0`, `Carrier.flags = 0`, `Carrier.profile_curve_id = 0`, `Carrier.sweep = (0., 0., 0.)`, `ImportedSource.contexts = ()`, `@dataclass(frozen=True) class Context`; `load_source(path)` accepts version 2 or 3 and returns `source.version`; `write_source(source, path, version=2)`.

- [ ] **Step 1: Write the failing tests**

```python
# tests/test_source_store_v3.py  (unittest; adapter cases skipUnless linux and BREPGPU_STEP_SOURCE is a file)
class Version3Python(unittest.TestCase):
    def test_v3_roundtrip_with_every_appended_field(self):
        # author an ImportedSource with one Context, one coedge presence 0 (pcurve id 0), one coedge
        # presence 1 master 2, a kind-8 carrier with profile_curve_id and sweep (0., 0., 2.5), a kind-6 curve
        # write version 3, load it: every field equal, source.version == 3
    def test_v2_write_unchanged(self):
        # the all-section fixture of tests/native_mesher_source_fixture.py written at version 2
        # is byte-identical to its bytes before this change (digest pinned in the test from the base commit)
    def test_absent_pcurve_rule(self):
        # presence 0 with a nonzero pcurve id -> ValueError('absent pcurve carries a curve')
class Version3Adapter(unittest.TestCase):
    def test_v3_equals_v2_on_v2_fields(self):
        # run brepgpu-step-source on tests/fixtures/void-block.step twice: default and --store-version 3
        # load both; every version-2 field of every row equal; appended fields at their defaults; contexts == ()
```

- [ ] **Step 2: Run them to make sure they fail**

Run (fleet lane): `PYTHONPATH=python:tests BREPGPU_STEP_SOURCE=build/om10-step-source/brepgpu-step-source python3 -m unittest tests.test_source_store_v3 -v`
Expected: FAIL (`Context` not importable; `write_source` has no `version`; the adapter rejects `--store-version`).

- [ ] **Step 3: Implement version 3 in `imported_source.py` and `--store-version 3` in the adapter**

Python: one `_TAGS_V3` tuple (version 2's plus `b'CONTEXTS'`), field-append branches keyed on the version in `_read_row` and `_write_row`, validation of the absent-pcurve rule in `validate_source`. Adapter: parse `--store-version {2,3}` beside the existing options (`main.cpp:398`); the row builders append the version 3 fields at their defaults when the version is 3; the header writes the version and section count. Rebuild: `cmake -S native/step_source -B build/om10-step-source -DOpenCASCADE_DIR=$OCCT_PREFIX/lib/cmake/opencascade && cmake --build build/om10-step-source`.

- [ ] **Step 4: Run them to make sure they pass**

Run: the Step 2 command, then `python3 -m unittest tests.test_step_ingress tests.test_imported_source -v` (version 2 behaviour unchanged).
Expected: all PASS.

- [ ] **Step 5: Commit**

```bash
git add python/brepgpu/imported_source.py native/step_source/main.cpp tests/test_source_store_v3.py
git commit -m "P1: source store version 3 (pcurve presence, masters, swept kinds, conics, contexts)

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 3: The version 3 reader, refusing per entity

**Files:**
- Create: `native/source_store/store.hpp`, `native/source_store/store.cpp`
- Test: `tests/native_source_store_read.cpp`, `tests/native_source_store_fixture.py`
- Modify: `native/source_store/CMakeLists.txt` (fixture command and test)

**Interfaces:**
- Consumes: `bgtt::split_sections`, `bgtt::RowReader`, `bgsrc::Reason`.
- Produces:
  - `enum class EntityKind : uint16_t { product, solid, usage, vertex, edge, coedge, loop, face, carrier, curve, style, diagnostic, occurrence_style, context };`
  - `struct EntityRefusal { EntityKind kind; uint64_t id; Reason reason; uint64_t byte_offset; };`
  - `struct Store` holding one row struct per section (fields as version 3; variable-length fields as `std::vector`), each row with `bool refused`, plus sorted `(id, slot)` indexes per kind with `std::optional<uint32_t> find(EntityKind, uint64_t id) const`.
  - `struct ReadLimits { uint64_t max_bytes = uint64_t{1} << 30; uint64_t max_rows = 100'000'000; };`
  - `enum class FileFault : uint16_t { none, bad_magic, bad_version, bad_length, container, oversized, duplicate_id, zero_id };`
  - `struct ReadResult { std::optional<Store> store; FileFault fault; bgtt::Fault container_fault; uint64_t fault_offset; std::vector<EntityRefusal> refusals; };`
  - `ReadResult read_store(std::span<const std::byte>, const ReadLimits& = {});`
- Scope: file-level faults are structural (header, container, duplicate or zero ids). Entity-level: a non-finite scalar in a row refuses that entity (`nonfinite_value`); a reference to an id that does not exist refuses the referencing entity (`malformed_record`). Geometry checks (radii, knots, weights) are T1's (P4, I4), not the reader's.

- [ ] **Step 1: Write the failing tests**

```cpp
// tests/native_source_store_read.cpp  (argv[1]: the all-section version 3 fixture)
void literal_fields(const bgsrc::Store& s) {        // values authored in native_source_store_fixture.py
  const auto v = s.vertices[*s.find(bgsrc::EntityKind::vertex, 302)];
  require(v.xyz == std::array{1., 1., 0.} && v.tolerance == .0003, "vertex 302");
  const auto c = s.carriers[*s.find(bgsrc::EntityKind::carrier, 1001)];
  require(c.kind == 8 && c.profile_curve == 600 && c.sweep == std::array{0., 0., 2.5}, "extrusion carrier");
  const auto u = s.coedges[*s.find(bgsrc::EntityKind::coedge, 503)];
  require(u.pcurve_presence == 0 && u.pcurve == 0 && u.master == 0, "absent pcurve");
  require(s.contexts.size() == 1 && s.contexts[0].length_factor_mm == 25.4 && s.contexts[0].snapped == 1, "context");
}
void nan_control_refuses_only_the_carrier();   // patch one control word to NaN: refusals == {carrier 1000, nonfinite_value}; store present; carrier 1001 not refused
void dangling_carrier_reference();             // face 900 names carrier 9999: refusals == {face 900, malformed_record}
void trailing_and_truncated_refuse_file();     // +1 byte -> fault container/trailing_bytes, no store; -1 byte -> container/truncated, no store
void duplicate_id_refuses_file();              // two VERTICES rows with id 300 -> FileFault::duplicate_id
void version_2_refused();                      // a version 2 header -> FileFault::bad_version (the new reader reads 3 only)
```

`native_source_store_fixture.py` writes the all-section fixture of `tests/native_mesher_source_fixture.py` at version 3, adding: vertex 302 at (1, 1, 0) tolerance .0003 (as authored there), carrier 1001 of kind 8 (profile curve 600, sweep (0, 0, 2.5)), coedge 503 with presence 0, one context (25.4, π/180 nearest double, .001, usable, snapped 1, "INCH", "DEGREE").

- [ ] **Step 2: Run them to make sure they fail**

Run: `python3 scripts/check_native_face_domain.py --unit source-store --profile debug`
Expected: build fails: `store.hpp` not found.

- [ ] **Step 3: Implement `bgsrc::read_store` in `store.cpp`**

Decode with `RowReader`; check every f64 for finiteness before storing it and refuse the entity on the first non-finite value; resolve references after all sections are decoded and refuse the referencing entity on a miss; never throw for input content. Port the structural checks of `native/mesher/source.cpp:405-470` that are structural (ids, counts); leave the knot and weight checks to T1.

- [ ] **Step 4: Run them to make sure they pass**

Run: `python3 scripts/check_native_face_domain.py --unit source-store --profile debug --lint`, then `--profile asan`.
Expected: `100% tests passed`.

- [ ] **Step 5: Commit**

```bash
git add native/source_store tests/native_source_store_read.cpp tests/native_source_store_fixture.py
git commit -m "P1: version 3 store reader with per-entity refusals and the 5.8 reason codes

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 4: Content digests

**Files:**
- Create: `native/fast_mesher/CMakeLists.txt`, `native/fast_mesher/l1/digest.hpp`, `native/fast_mesher/l1/digest.cpp`
- Test: `tests/native_fm_l1_digest.cpp`, `tests/native_fm_l1_fixture.py`
- Modify: `scripts/check_native_face_domain.py` (add `--unit fast-mesher`: module `native/fast_mesher`, lint globs `tests/native_fm_*.cpp`)

**Interfaces:**
- Consumes: `bgsrc::Store`, `bgsha::Sha256`.
- Produces:
  - `namespace bgfm { using ContentDigest = bgsha::Digest; struct DigestSet { std::vector<ContentDigest> vertex, curve, carrier, edge, coedge, loop, face, definition; /* indexed by store slot */ }; DigestSet content_digests(const bgsrc::Store&); }`
- Encoding [call; fixed here]: each digest is SHA-256 over a one-byte kind (`V`, `C`, `S`, `E`, `U`, `L`, `F`, `D`), a u16 encoding version 1, then the fields below little-endian, f64 as raw IEEE bits, variable-length fields as u64 count plus elements, an absent child as 32 zero bytes. No id, name, style or diagnostic enters.
  - vertex: xyz, tolerance.
  - curve: dimension, kind, frame, radii, range, degree, knots, multiplicities, controls (the store's span rows are derived and excluded).
  - carrier: kind, frame, radii, degrees, control shape, both knot vectors and multiplicities, controls, flags, profile-curve digest, sweep (the uv hint is a union over faces and excluded).
  - edge: curve digest, range, same_sense, same_parameter, degenerate, tolerance, vertex digests v0, v1.
  - coedge: edge digest, sense, pcurve presence, master, pcurve digest, pcurve range, periodic shift, map kind, map.
  - loop: outer, status, pole-vertex digest, then the coedge digests in their least rotation (Booth).
  - face: carrier digest, orientation, shell ordinal, tolerance, status, outer-loop digests, then the other loop digests sorted.
  - definition: status, then its face digests sorted.

- [ ] **Step 1: Write the failing tests**

```cpp
// tests/native_fm_l1_digest.cpp  (argv[1]: base fixture; argv[2]: its renumbered twin; argv[3]: one-bit twin)
void renumber_invariant(const bgsrc::Store& a, const bgsrc::Store& b) {
  // b: every id remapped by a fixed permutation, rows shuffled, names and styles changed
  require(sorted(content_digests(a).face) == sorted(content_digests(b).face), "faces");
  require(content_digests(a).definition == content_digests(b).definition, "definition");
}
void one_bit_changes_exactly_its_ancestors(const bgsrc::Store& a, const bgsrc::Store& c) {
  // c: the lowest mantissa bit of vertex 301's x flipped
  // changed: vertex 301, the edges ending at it, their coedges, the loops holding them, those faces, the definition
  // unchanged: every other vertex, curve, carrier, edge, coedge, loop, face
}
void loop_rotation_invariant_reflection_distinct();   // the same loop from each of its 4 starts: one digest; reversed order with flipped senses: a different digest
void negative_zero_is_distinct();                     // a vertex at (-0., 0, 0) and one at (0, 0, 0): different digests (raw bits, §16.2)
void no_identity_field_enters();                      // changing product names, style ids and diagnostics leaves every digest unchanged
```

`native_fm_l1_fixture.py` writes, through `imported_source.write_source(..., version=3)`: the base fixture (a unit cube, 8 vertices, 12 line edges, 6 planar faces, one shell), its renumbered and shuffled twin, and its one-bit twin.

- [ ] **Step 2: Run them to make sure they fail**

Run: `python3 scripts/check_native_face_domain.py --unit fast-mesher --profile debug`
Expected: build fails: `l1/digest.hpp` not found.

- [ ] **Step 3: Implement `bgfm::content_digests` in `l1/digest.cpp`**

Compute bottom-up in the dependency order vertex, curve, carrier, edge, coedge, loop, face, definition, each in one pass over the store. `native/fast_mesher/CMakeLists.txt` follows `native/source_store`'s: rigor profile, CTest, static library `brepgpu_fm_l1` linking `brepgpu_source_store` (added with `add_subdirectory(../source_store source_store)`), fixture commands as in `native/mesher/CMakeLists.txt:212-216`.

- [ ] **Step 4: Run them to make sure they pass**

Run: `python3 scripts/check_native_face_domain.py --unit fast-mesher --profile debug --lint`
Expected: `100% tests passed`.

- [ ] **Step 5: Commit**

```bash
git add native/fast_mesher tests/native_fm_l1_digest.cpp tests/native_fm_l1_fixture.py scripts/check_native_face_domain.py
git commit -m "P1: Merkle content digests over raw IEEE bits

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 5: L1 definitions

**Files:**
- Create: `native/fast_mesher/l1/definition.hpp`, `native/fast_mesher/l1/definition.cpp`
- Test: `tests/native_fm_l1_definition.cpp` (fixtures from `tests/native_fm_l1_fixture.py`)

**Interfaces:**
- Consumes: `bgsrc::Store`, `bgfm::DigestSet`.
- Produces:
  - `enum class EdgeUseClass : uint8_t { unused, free, manifold, seam, same_direction_pair, non_manifold };`
  - `enum class ShellClass : uint8_t { closed, open };`
  - `struct EdgeUse { uint32_t face, loop, coedge; int8_t direction; };` (definition-local indices; direction is the COEDGES `sense`)
  - `struct VertexAlias { uint32_t original, representative; };` (declared-degenerate edges; representative is the alias class's member with the least digest)
  - `struct FaceRefusal { uint32_t face; bgsrc::Reason reason; };`
  - `struct DuplicateGroup { bgsrc::EntityKind kind; std::vector<uint32_t> members; };`
  - `struct L1Definition { ContentDigest content; bool incomplete, refused; std::vector<uint32_t> faces, edges, vertices, loops, coedges /* store slots, in content-digest order */; std::vector<ContentDigest> face_digest, edge_digest, vertex_digest; std::vector<uint64_t> face_id, edge_id, vertex_id /* identity columns */; std::vector<uint32_t> use_offsets; std::vector<EdgeUse> uses; std::vector<EdgeUseClass> edge_class; std::vector<uint32_t> face_shell; std::vector<ShellClass> shell_class; std::vector<VertexAlias> aliases; std::vector<FaceRefusal> refused_faces; std::vector<DuplicateGroup> duplicates; };`
  - `struct L1Model { std::vector<L1Definition> definitions; std::vector<bgsrc::EntityRefusal> refusals; };`
  - `L1Model build_l1(const bgsrc::Store&, const DigestSet&);`
- Rules [call, from master plan calls 8 and 9]:
  - Edge class per edge from its uses within one shell ordinal: one use `free`; two uses of opposite direction in two faces `manifold`; two uses of opposite direction in one face `seam`; two uses of one direction `same_direction_pair`; three or more `non_manifold`. A shell is `closed` iff every edge it uses is `manifold` or `seam`.
  - A face is refused (`prerequisite`) when its carrier, any of its loops, coedges, edges, curves, pcurves or vertices is refused; a face with source status refused keeps that status. `incomplete` iff some face is refused; `refused` iff no face is produced.
  - Duplicate faces or edges (equal digests in one definition) refuse those faces with `coincident_source_entities`; duplicate vertices are reported in `duplicates` and refuse nothing.
  - Ties in content-digest order occur only among duplicate vertices; they are broken by the digests of the edges using them, then left in store order with the group recorded.

- [ ] **Step 1: Write the failing tests**

```cpp
// tests/native_fm_l1_definition.cpp
void closed_cube();                              // 12 edges manifold, 1 shell closed, no refusal, incomplete == false
void open_sheet();                               // the cube without its top face: 4 free edges, shell open, not refused
void non_manifold_t_edge();                      // three faces on one edge: non_manifold; the shell open
void seam_and_misoriented_seam();                // a cylinder face using seam edge 402 twice (+1, -1): seam; with (+1, +1): same_direction_pair
void refused_carrier_refuses_only_its_faces();   // the cube with face 900's plane frame word patched to NaN: face 900 refused prerequisite, the other five produced, incomplete == true
void vertex_loop_face_carried();                 // a cone with an apex vertex loop: face produced, no uses for that loop, the apex vertex present
void void_shell_classes();                       // outer and void shell both closed (two shell ordinals)
void duplicate_face_refused();                   // the cube with face 905 repeated as face 906: both coincident_source_entities
void renumber_gives_identical_l1();              // the renumbered twin: every vector except the identity columns byte-identical
```

- [ ] **Step 2: Run them to make sure they fail**

Run: `python3 scripts/check_native_face_domain.py --unit fast-mesher --profile debug`
Expected: build fails: `l1/definition.hpp` not found.

- [ ] **Step 3: Implement `bgfm::build_l1` in `l1/definition.cpp`**

One pass per definition: gather its entities through SOLIDS rows, sort slots by digest, build the use CSR by counting then filling (no atomics, no hash-ordered iteration: §4.2 mechanisms 5 and 8), classify, propagate refusals in the dependency order of Task 4.

- [ ] **Step 4: Run them to make sure they pass**

Run: `python3 scripts/check_native_face_domain.py --unit fast-mesher --profile debug --lint`, then `--profile asan`.
Expected: `100% tests passed`.

- [ ] **Step 5: Commit**

```bash
git add native/fast_mesher/l1 tests/native_fm_l1_definition.cpp tests/native_fm_l1_fixture.py
git commit -m "P1: L1 definitions with edge-use classes, shell closure and refusal propagation

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 6: The adapter and the reference exporter hash in place

**Files:**
- Modify: `native/step_source/main.cpp:229-252` (delete the copying `sha256`), `:409` (call `bgsha::sha256` on the read bytes)
- Modify: `native/reference_source/main.cpp:119` (delete its copy), `:210`
- Modify: `native/step_source/CMakeLists.txt`, `native/reference_source/CMakeLists.txt` (include `../source_store`)
- Test: `tests/test_source_store_v3.py` (add `test_header_digest_is_sha256_of_input`)

**Interfaces:**
- Consumes: `bgsha::sha256(const void*, std::size_t)` (the C++17 overload).
- Produces: no new interface. This removes a whole-input copy (design §19.1); behaviour is unchanged, so its test is a regression guard that passes before and after.

- [ ] **Step 1: Write the guard test**

```python
    def test_header_digest_is_sha256_of_input(self):
        # for void-block.step and source-uv.step: the store header's 32 digest bytes == hashlib.sha256(file bytes)
        # and the reference exporter's metadata digest == the same value
```

- [ ] **Step 2: Run it on the base commit**

Run: `PYTHONPATH=python:tests python3 -m unittest tests.test_source_store_v3.Version3Adapter.test_header_digest_is_sha256_of_input -v`
Expected: PASS (it guards the refactor).

- [ ] **Step 3: Replace both copies with `bgsha::sha256`**

- [ ] **Step 4: Run the guard and the adapter suites**

Run: the Step 2 command, then `python3 -m unittest tests.test_step_ingress tests.test_reference_source -v`.
Expected: all PASS.

- [ ] **Step 5: Commit**

```bash
git add native/step_source native/reference_source tests/test_source_store_v3.py
git commit -m "P1: adapter and reference exporter hash their input in place

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

### Task 7: `brepgpu-fm inspect`, the L1 census and the watch metamorphic run

**Files:**
- Create: `native/fast_mesher/cli/main.cpp` (`brepgpu-fm inspect STORE [--tsv]`)
- Test: `tests/test_fm_l1.py`

**Interfaces:**
- Consumes: `bgsrc::read_store`, `bgfm::content_digests`, `bgfm::build_l1`.
- Produces: `brepgpu-fm inspect` prints, per definition, its content digest, face, edge and vertex counts, edge-class counts, shell classes, refused faces by reason, duplicate groups, and the read and L1 wall and CPU times; text by default, TSV on request (presentation). Exit 0 when read, 4 on a file-level fault.

- [ ] **Step 1: Write the failing tests**

```python
# tests/test_fm_l1.py  (fleet only; BREPGPU_FM, BREPGPU_STEP_SOURCE; watch store path from BREPGPU_WATCH_STORE)
class Inspect(unittest.TestCase):
    def test_void_block(self):
        # adapter --store-version 3 on tests/fixtures/void-block.step -> inspect --tsv:
        # one definition, two shells closed, every edge manifold, no refusal
    def test_file_fault_exit_4(self):
        # a store with one trailing byte -> exit 4, nothing on stdout
class WatchMetamorphic(unittest.TestCase):
    def test_renumbered_watch_identical(self):
        # load the watch's version 3 store with imported_source, remap every id by a seeded permutation,
        # write it to a temporary file (never tracked), inspect both: per-definition digests and counts equal
```

- [ ] **Step 2: Run them to make sure they fail**

Run: `PYTHONPATH=python:tests BREPGPU_FM=build/fast-mesher-debug/brepgpu-fm python3 -m unittest tests.test_fm_l1 -v`
Expected: FAIL (`brepgpu-fm` not built).

- [ ] **Step 3: Implement `brepgpu-fm inspect`**

- [ ] **Step 4: Run them to make sure they pass, then the census**

Run: the Step 2 command (all PASS); then the census, lane-bound on one node:
`python3 scripts/measurement_lane.py run --node 1 --context out/p1-census-context.tsv --label p1-watch -- build/fast-mesher-release/brepgpu-fm inspect --tsv "$BREPGPU_WATCH_STORE"` for the watch, and the same command per admitted corpus store (label `p1-<stratum>`), writing each `inspect --tsv` into the lane's `out/`.
Expected: every admitted store reads; the unit report lists per stratum the definition count, edge-class counts, open shells, non-manifold edges, duplicate groups and read + L1 time per MB (design §22.6 T0 line).

- [ ] **Step 5: Commit and write the unit report**

Write `results/p1-source-view-<date>/REPORT.md` in the repository's report shape (`results/b2-harness-2026-10-17/REPORT.md`): plan, red and green evidence per task, suites and counts, the census table, fleet-relative paths only.

```bash
git add native/fast_mesher/cli tests/test_fm_l1.py results/p1-source-view-*/REPORT.md
git commit -m "P1: brepgpu-fm inspect, L1 census and the renumbered-watch check

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

## Self-review

- **Spec coverage.** §4.2 L1 (Task 5), §4.4 identity columns (Task 5), §3.5 and §3.11 use classes and open shells (Task 5), §5.4 row 4 closure computed from uses (Task 5), §5.8 reader-level rows (Task 3) and `coincident_source_entities` (Task 5), §7.3 container (Task 1), §16.2 content identity (Task 4), §19.1 hashing in place (Tasks 1, 6), the version 3 fields of master-plan call 3 (Task 2).
- **Step scan.** Every code step names its file and signature; test steps carry their assertions; commands carry expected output.
- **Type consistency.** `bgsrc::Store`, `bgsrc::read_store`, `bgsrc::Reason`, `bgfm::ContentDigest`, `bgfm::DigestSet`, `bgfm::L1Definition`, `bgfm::build_l1` match the master plan's P1 block and V1's Task 6.
- **Review focus.** Each of the five lines has its test in the task named.
- **Proportion.** Signatures, encodings and assertions only; no function bodies.
