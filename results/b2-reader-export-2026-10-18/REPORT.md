# B2-reader-export: the reference exporter, its oracles and NVIDIA's admission

Unit B2-reader-export of maneuver B2 ([brief](../../docs/fast-mesher-campaign/briefs/B2-lane-defects.md): sections "Follow-on unit B2-reader" through "B2-reader-export review round 2 decisions", plus the dispatcher's later rulings).

**Beads:** `brepgpu-12s.37.19`, `.37.21`, `.37.22`, `.37.23`, `.37.25`, `.37.32` (oracle part), `.37.33`, `.37.34`, `.37.35`, `.37.36` (exporter and oracle parts), `.37.37`, `.37.38`, `.37.39` (exporter adoption), and review rounds 1 to 4.

**Who wrote this:** the dispatcher, from the lane's consolidated return.

**How it ran.**
- Code was edited in the laptop checkout. Builds and tests ran only on the fleet, against pinned OCCT 8.0.1 and the NVIDIA USD/BRep 1.0.0 package, with B2-ingress's build configurations.
- The final run used a fresh tree: HEAD plus every modified and untracked file of both B2-reader lanes, with every source touched so that everything rebuilt. The dispatcher checked by checksum that the fleet tree's 40 changed files match the commit.
- HEAD's tree from the earlier merged run was checked file by file to equal HEAD's code; it is the baseline.
- The OM10 source was read in place, read-only. Only counts appear here; derived files were deleted.
- The corpus is M4-G6's 500 files, read-only.
- No NVIDIA benchmark ran. Fixture-level tests drive the genuine NVIDIA producer on authored fixtures and on `void-block.step`.
- Every fix below has a test that was run red on the code before the fix and green after.
- **Review:** self-review plus four independent review rounds. Every finding is closed below. The operator then ended review rounds for B2.

## Final merged run (both B2-reader lanes)

### Suites (final tree, library variables set)

| Suite | HEAD | Final |
|---|---|---|
| The other 60 unittest modules | same outcome | all OK (one CUDA-device skip in each of 6 modules, by design) |
| `test_reference_source` | 4 | 37 OK |
| `test_placement_oracles` | 18 | 21 OK |
| `test_measurement_lane` | 38 | 44 OK |
| `test_native_mesher_display` | 15, 3 failures | 16 OK (`.37.37`) |
| `test_step_ingress` | 72 | 72 OK |
| `test_step_reader_raw` (adapter lane) | absent | 51 OK |
| New: `test_oracle_bounds`, `test_oracle_face_sides`, `test_oracle_frames`, `test_oracle_holders`, `test_physical_shells`, `test_nvidia_face_sides` | absent | 3, 3, 1, 1, 6, 3 OK |
| Scripts: `test_occt_reference`, `test_reference_periodic` | PASS | PASS |
| ctest: root, om10 (CUDA), direct CDT | 60/60, 5/5, 4/4 | 60/60, 5/5, 4/4 |
| ctest: native mesher | 130/132 | 130/132 |

- **The only red** is the native mesher ctest's two cases `native_adaptive_trimmed_cylinder_seam_cpu1` and `_cpu4`. They are the retired old mesher's cylinder-seam pair, closed under `.37.11`, and fail identically on HEAD.
- **Builds:** all exit 0. `native/reference_source` and its fixture tool build with no warnings.

### Corpus (500 files, against HEAD)

| Reader | HEAD accepted / refused | Final accepted / refused | Changes |
|---|---|---|---|
| Adapter | 232 / 268 | 232 / 268 | nothing moves; one refusal reason changes to the adapter's new natural-domain refusal |
| Exporter, translation-only | 329 / 171 | 327 / 173 | see below |
| Exporter, default read | no default mode | 327 / 173 | the screw and umbilical faces undecidable; the shroud made into several solids |

**Translation-only changes:**
- `Cold section fan.step`: refused → accepted (voids).
- `M8X25 Pan Head Screw.step` and `umbilical_mount_bottom.step`: accepted → refused (an undecidable face).
- `ratrig_eva3_shroud.step`: accepted → refused (its face sides cannot be checked).

**Across the corpus:**
- Every accepted export passes the Python reader.
- No file writes a raw placement in either mode.
- **Face-side cross-check, file by file:** the only file where any reader corrects a face is `Cold section fan.step`. There the adapter, the translation-only export (`S2`) and the default read's repair give the same 13 faces.

### OM10 (counts only)

| | Adapter | Exporter, translation-only | Exporter, default read |
|---|---|---|---|
| Exit | 0 | 0 | 0 |
| Definitions / faces / occurrences | 107 / 9,162 / 166 | same | same |
| Occurrence styles | 9 | 9 | 9 |
| Raw placements written | 0 | 0 | 0 |
| Face sides corrected or repaired | 6 (S1) | 6, `S1` declared | 6 repaired, recorded |
| Same faces as the adapter | — | yes; all 9,162 face sides equal | yes; all 9,162 face sides equal |
| `.brep` identical to the minimal NVIDIA ingress (same arm) | — | 106 / 107 (see note) | 107 / 107 |
| Published solids that verify | — | 105 / 107 (the 2 are OCCT's own cone-apex fault, under Known limits) | 107 / 107 |
| Header uncertainty (usable) | 0.01 mm | 0.01 mm | 0.01 mm |
| Wall time (ours, outside NVIDIA's timed region) | 25.2 s | 25.9 s (HEAD 4.6 s) | 24.8 s |

**Note on byte identity:** the one differing definition, 1808, is the declared correction. Its uncorrected translation is byte-identical to the minimal ingress's (1 / 1).

- **Oracle:** geometry and assembly qualification pass on the adapter's source in 56.9 s; the largest placement error is ≤ 2⁻³².
- **Native match:** all four source-level checks of `load_native_model_match` pass in both modes: graph and styles, occurrence styles, definition and face inventory, and face metadata.
- **Not run:** no full native mesh model of OM10 was built, so the mesh-transport half of that function was not exercised.

## Per bead: cause, fix, test, before → after

### `.37.19` Default-read exporter

- **Cause.** The exporter always cleared ShapeProcess and declared translation-only, so the harness's default-read arm refused (exit 3).
- **Fix.**
  - New CLI: `brepgpu-reference-source [--shape-processing translation-only|default] IN OUT`.
  - Default mode supplies no flags or parameters, asserted before and after transfer, and declares `shape-processing=default; shape-process-operations=<bits>`.
  - Its face map binds each original face to the processed pieces OCCT made of it (`XSAlgo_ShapeProcessor::MergeTransferInfo`), with every piece claimed once.
  - The harness runs the exporter in the arm's mode and passes the prepared worker each input's translated face count.
- **Test.**
  - Default exports are byte-identical to the minimal ingress's default read on every fixture, with equal operation bits.
  - Named refusals: a nested cube (shell count) and disjoint cubes (several solids).
  - Split-face reader cases.
- **Before → after.** The arm exits 3 → it runs. OM10 BRepCheck-valid solids under the default read go from 0 to 107 / 107.

### `.37.22` Voids

- **Cause.** Only the outer shell was walked, so a `BREP_WITH_VOIDS` refused as "face inventory differs".
- **Fix.** Every shell is walked in the adapter's order. Each face's use is composed with its shell's orientation, and the shell ordinal is written per face. ABSR WR3/WR4 cases refuse by name.
- **Test.** `void-block.step` in both modes: 12 faces in 2 shells; the per-face table equals the adapter's; BRepCheck valid; volume 936.
- **Before → after.** Refused → exported. In the corpus, `Cold section fan.step` is now accepted.

### `.37.23` Occurrence styles and vertex-loop faces

- **Fix.**
  - The format gains the OCCSTYLE table, with the adapter's rows; `load_native_model_match` and `qualify_om10_nvidia.py` compare it.
  - Translation-only keeps OCCT's vertex-loop faces (that is the arm's claim) and records each one that is BRepCheck-invalid. The default read rebuilds them validly.
- **Test.**
  - The nested styled assembly's rows equal the adapter's, and the reader refuses broken rows by name.
  - The apex cone is invalid, with a record, under translation-only, and has volume 261.80 under the default read.
- **Before → after.** OM10 occurrence styles go from records only to 9 rows. The format is BGOCC001 version 2; version 1 refuses by name ("re-export the source").

### `.37.25` Mapped-item solids

- **Cause.** Leaf binding used `IsSame`, so a solid located by a `MAPPED_ITEM` bound nothing.
- **Fix.** A solid-bearing mapped item is published as an occurrence:
  - the holder product becomes an assembly node;
  - the mapped representation becomes a part product;
  - the mapped item becomes a usage, with its placement read raw as target∘origin⁻¹.

  Unread forms refuse by name.
- **Test.** Hand-derived placements on `mapped-item-solid.step`:

  | Case | Result |
  |---|---|
  | As authored | (0,5,0) |
  | Origin moved to (1,0,0) | (−1,5,0) |
  | Origin turned a quarter about z | raw (0,6,0), declared `P2` (OCCT reads (5,1,0)) |

- **Before → after.** Refused → exported. The legacy matched mode now refuses such a file by name (exit 3): the adapter does not read mapped items (`.37.26`), so there is no adapter source to compare faces with. Admitting them is `.25.11`.

### `.37.21` and `.37.32`: oracle representation links and holders

- **Fix.**
  - `qualify_original_geometry` skips placing relations, through one shared helper.
  - `_RepresentationGraph.transferred` runs breadth-first with no depth bound.
- **Test.**
  - The untransformed placing relation gives the placement's own refusal instead of "correspondence differs".
  - The adapter's `deep_holder` (nine levels) is followed, and its inch variant refuses by its unit.

### `.37.33` and `.37.34`, with review round 1 item 8: raw placements and the one agreement predicate

- **Cause.** Usages took OCCT's located transform, which misreads some placements. Later, the exporter's agreement test was not the adapter's.
- **Fix.**
  - Usages and mapped items are read raw (`native/step_placements.hpp`).
  - OCCT's value is written only where `step_placements::agree` accepts it: 2⁻³² relative, translation within the placing contexts' resolution, and no point of the child's extent moved by more than that resolution.
  - The extent is `native/shape_extent.hpp`'s `shape_bound` in both modes (`.37.39`), carried up through the raw placements.
  - Elsewhere the raw value is written, recorded with its four agreement values, and declared (`P1`, `P2`).
  - The oracle reads mapped occurrences from the STEP text (`original_mapped_occurrences`, `qualify_original_reference`).
- **Test.**
  - The adapter's fixtures: an omitted axis3 gives `P1`; a rotated item_1 gives `P2`.
  - The adapter's three round-2 cases in both modes: the 5000 mm translation and the far rotation are written raw; the near cylinder is written as OCCT's.
  - A far, turned mapped item.
  - The apex cone's extent: the old box from the pcurve copy missed its apex by 10 mm; `shape_bound` holds it.
- **Before → after.** Misreads that were written undeclared are now written raw and declared. OM10 writes 0 raw placements in both modes.

### `.37.35` and `.37.36`, with review rounds 1 and 2: face sides

- **Cause.** The default read changes some faces' orientation flags. The raw file's `same_sense` was found to point into the material on OM10 (6 faces of one definition) and in the corpus.
- **Decisions.** Admit a repair with a record; correct the raw side in translation-only (S1, S2); refuse what is not decided.
- **Fix** (`native/reference_source/orientation_verdict.hpp`), for each face whose use the default read changed:
  - **Which side the raw use faces,** over every one of OCCT's pieces of the face:
    - ray parity from probes on both sides, with offsets scaled to the piece, seven generic directions and a strict majority;
    - confirmed by `BRepClass3d_SolidClassifier`;
    - a mixed or undecided piece, or an OCCT exception, makes the face undecidable by name.
  - **S1 or S2,** decided from the face's own bound, never by counting:
    - S1 when the outer bound runs against the face. It is classified on a completed copy; on a seam face whose raw pairing does not close, it is classified on the S1 candidate.
    - S2 when the whole shell is inside out.
    - Each seam use of a corrected face is put on the pcurve under which its wires close, as OCCT's default read pairs the repaired face.
  - **Verification reads the published bytes back:**
    - BRepCheck must be valid, and each shell's signed volume must have its role's sign.
    - The check runs on a copy that only adds missing pcurves: projected, moved by whole periods to meet a neighbour, and with a missing seam paired and placed.
    - An edge carrying an added pcurve may take that pcurve's measured deviation as its tolerance. The raise is capped at the definition's usable declared uncertainty, or else at the adapter's source residual grade (0.001 mm); above the cap, verification fails by name.
    - `source/verification.tsv` records the largest raise, deviation and cap for each verified definition.
  - **Refusals in both modes:** an undecidable face, or a definition whose sides cannot be checked (the default read is not one solid, or a shell is open), refuses as ingress (exit 4, ours). Translation-only writes a decided outward raw use with a record.
  - **The oracle** accepts a written side other than the raw use only with a `source_face_orientation_raw` record, confirmed by the exporter's independent `--face-sides` check. Its per-face bound checks run for every face again.
- **Test.**
  - Function level: box configurations (24 verdicts) and fused-box multi-piece cases.
  - The void block with one face flipped, and the reviewer's all-six and void-face cases.
  - The cylinder seam face with every pcurve, with the circles stripped, and with all stripped: S1, with seam uses equal to the default read's, and volume π·0.75²·2.
  - The inside-out stripped cylinder: S2.
  - The inside-out torus with a vertex loop: S2, volume 2π²·5·4.
  - The reviewer's tolerance cases at radius 0.8 and 0.76: refused above the cap.
  - Oracle corruptions refused by name.
- **Before → after.**
  - OM10 default read: refused → 6 faces repaired, recorded.
  - OM10 translation-only: 6 faces written inward, BRepCheck-invalid → `S1`, verified, the same faces as the adapter's.

### Review rounds 1 and 2: harness rulings

- **Translation-only arm.**
  - The byte check is per definition.
  - A corrected definition reaches NVIDIA as our corrected `.brep`, declared. That happens only after our uncorrected translation of it (`source/uncorrected/`) is shown byte-identical to the minimal ingress's; otherwise the harness refuses (exit 3).
  - Placement corrections exempt no definition.
- **Default-read arm.** Every definition must be byte-identical to our default export, and both reads must report the same operations; otherwise exit 3.
- **Legacy matched mode.**
  - It now compares every face's carrier, side and style with the adapter's source, as the native match does.
  - It reads that source's definition status, so a definition the adapter leaves incomplete refuses before NVIDIA's ingress (exit 4) and is never scored.
- **Test.** Unit and caller-level tests for each, each red on the code before. For example, the legacy incomplete definition used to reach NVIDIA and exit 1.

### Review rounds 3 and 4: physical admission

- **Cause.**
  - `qualify_physical_solid` required one connected shell, so every void part failed and was scored as NVIDIA FAIL.
  - Then malformed multi-shell meshes passed.
  - The expected Euler number assumed disc faces.
  - Errors in our own code were scored against NVIDIA.
- **Fix.**
  - **Shells.** The mesh is split into edge-connected shells, each closed and edge-consistent.
    - The outer shell (the largest by magnitude) must enclose a positive volume; every other shell is a void with negative volume.
    - Shells must be pairwise disjoint, by exact triangle–triangle tests on candidates from a uniform grid.
    - Given disjointness, one vertex decides containment: every void must lie inside the outer shell and inside no other void.
    - Euler is taken over all shells.
    - Owned and NVIDIA meshes use the one function.
  - **Euler expectation.** Each face counts from its own topology, χ = 2 − 2g − b − p: a natural torus gives 0, a band 0 and a natural sphere 2. Both the owned and NVIDIA paths use it.
  - **Whose fault.** Only an admission verdict on NVIDIA's mesh (`AdmissionVerdict`), or a fault in the mesh content NVIDIA produced (`ReferenceMeshContent`), is an NVIDIA FAIL. An error in our admission code, or in our wrapper's `.bgrefmesh` container, is ours (exit 5).
- **Test.**
  - Authored cube meshes: the cube with an inward void passes, and 13 malformed cases each fail by name (a void inside a void, overlapping voids, a void crossing the outer face, outside voids, lumps, inside-out shells).
  - A natural torus face expects Euler 0.
  - A forced admission exception and a broken container are ours; a nonfinite NVIDIA point is a FAIL.
  - An inside-out NVIDIA mesh (volume −3.53) fails, and our S2-corrected file of it passes. A run with an S1-corrected face completes with physical PASS.
- **Cost.**
  - On 103 existing OM10 NVIDIA meshes (212,832 triangles, all single-shell, so no intersection test runs), admission takes 14.3 s against 14.1 s before.
  - No owned OM10 mesh was available on the fleet, and OM10 has no void definition.

### `.37.37` and `.37.38`: suites that never ran

- **Cause.**
  - The suite runner never set the topology and direct CDT library variables, and the direct CDT library had never been built.
  - `test_native_mesher_display`'s fixture used a sheared instance, which the decoder has correctly refused since B1.
- **Fix.**
  - Build the library and set the variables.
  - The display tests use a rigid instance, and a new test checks that the shear refuses by name.
- **Before → after.**
  - The nine modules that errored now all pass.
  - `test_native_mesher_display` goes from 3 failures to 16 OK.

## NVIDIA fails a correct void part

`void-block.step` is a 10 mm cube with a 4 mm cube void, volume 936. The input is correct: OCCT's translation, the file NVIDIA meshes, is BRepCheck-valid, with a positive outer shell, a negative void and volume 936.

NVIDIA's mesh of it is wrong in all 8 configurations tried: minimal ingress with no shape processing and with the default read, each also with both shells forward, at heal 0 and heal 1.
- **NVIDIA's own report:** input manifold 1, output manifold 0, 12 lamina edges.
- **Winding:** three void faces are wound against the material, and 6 directed edges are used twice in the same direction.
- **Volume:** the signed volume is 840 against 936.

Our admission fails it by name ("physical edge needs two opposite distinct uses"). The same function passes a correctly wound mesh of the same solid. This is a genuine NVIDIA FAIL on that fixture, not an admission fault.

Test: `test_nvidias_mesh_of_the_void_block_fails_admission_on_correct_input`.

## Known limits

- **Translation-only verification gates only definitions with a written correction.**
  - Two uncorrected OM10 definitions fail it on their own. Each has a cone face where OCCT's translation without ShapeProcess builds no degenerate edge at the apex, so no seam pairing closes the wire there.
  - They are OCCT's own translation and within the arm's claim; the default read adds the edge.
- **The tolerance cap** covers the translation-only arm only, by design. The default read's solid carries OCCT's own pcurves and tolerances.
- **Genus is counted only for torus carriers** (no seam). A doubly closed B-spline face would be counted as a disc. This is unreachable today, because the adapter refuses edge-loop faces on closed B-spline carriers (`.25.12`). It is filed with `.25.12`.
- **Seam pairing** searches at most two missing and three present seams per face. Beyond that the face is not decided.
- **Void containment** uses float ray parity with a majority of three directions, after the exact disjointness test.
- **The "raw use agrees with the shell" verdict** is reached by no authored STEP file. It is tested at function level. It was reached by `umbilical_mount_bottom.step` before the ray-parity rewrite made that face undecidable.
- **Export time** on OM10 rose from 4.6 s to about 25 s in both modes, for a second OCCT read for the face sides plus verification. It is ours, and outside NVIDIA's timed region.
- **Mapped-item files** cannot pass the harness until the adapter reads mapped items (`.25.11`), because the face comparison needs its source.

## Files (this lane)

- `native/reference_source/main.cpp`, `native/reference_source/orientation_verdict.hpp` (new), `native/reference_source/CMakeLists.txt`
- `python/brepgpu/imported_oracles.py`, `python/brepgpu/nvidia_reference.py`, `python/brepgpu/reference_source.py`
- `examples/render_om10_nvidia.py`, `scripts/benchmark_om10_nvidia.py`, `scripts/qualify_om10_nvidia.py`
- `docs/NVIDIA_REFERENCE_PREPARATION.md`
- `tests/reference_source_fixture.cpp`, `tests/test_reference_source.py`, `tests/test_measurement_lane.py`, `tests/test_native_mesher_display.py`
- New tests: `tests/test_oracle_bounds.py`, `tests/test_oracle_face_sides.py`, `tests/test_oracle_frames.py`, `tests/test_oracle_holders.py`, `tests/test_physical_shells.py`, `tests/test_nvidia_face_sides.py`
- Hunks in shared files:
  - `tests/test_step_ingress.py`: the S7 block of `test_reference_export_writes_the_same_styles`, and the exporter path of `test_sole_vertex_loop_on_a_trimmed_arc_revolution_refuses_that_face`;
  - `tests/test_placement_oracles.py`: `test_untransformed_placing_relation_joins_no_representations_in_geometry`;
  - `tests/test_om10_nvidia.py`: the legacy fixture writes the adapter's source.

The exporter includes these adapter headers, unchanged by this lane: `native/step_units.hpp`, `native/step_styles.hpp`, `native/step_vertex_loops.hpp`, `native/step_placements.hpp`, `native/shape_extent.hpp` and `native/source_uv/source_uv.hpp`.
