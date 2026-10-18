# B2-reader-adapter: raw entities and orientation in the owned STEP source adapter

**Lane:** B2-reader-adapter. Brief: [`docs/fast-mesher-campaign/briefs/B2-lane-defects.md`](../../docs/fast-mesher-campaign/briefs/B2-lane-defects.md).
**Date:** 2026-10-18.
**Beads:** `.37.20`, `.37.26`, `.37.27`, `.37.28`, `.37.29`, `.37.30`, `.37.31`, `.37.32`, `.37.36`, `.37.39` (all `brepgpu-12s.*`).
**Who wrote this:** the dispatcher, from the lane's consolidated return. An independent review ran for eight rounds; the operator then stopped further review rounds.

## Summary

- **Authority rule (G1):** the owned adapter (`native/step_source/main.cpp`) now treats the raw Part 21 entity as authoritative and OCCT 8.0.1 only as an evaluator. This applies wherever the two disagree, and covers placements, B-spline nets, UV domains and face orientation.
- **Corrections are declared and recorded:** every correction is named in the label (`raw-entity-corrections=P1,P2,G5,G6,S1,S2`) and gets a typed per-entity record. Every case that cannot be decided is refused by name.
- **Admission is unchanged:** rigid-only and mm-only, as the brief requires.
- **Format:** BGSRC001 is still version 2, so no reader changed.
- **Corpus (500 files):** 232 accepted and 268 refused, the same split as HEAD. Exactly one file changed its refusal reason. No face is refused in any accepted file.
- **OM10:** reads with 0 refused faces and 6 recorded S1 corrections. It takes 25.0 s against about 17 s at HEAD.

**How it ran**
- **Code:** edited in the laptop checkout; built and tested on the fleet with the pinned OCCT 8.0.1 and CUDA 12.8.1.
- **Trees:** the lane tree is HEAD plus this lane's files only. The two shared test files are HEAD plus this lane's hunks.
- **Red runs:** each red run used the previous build, or HEAD.
- **Merged check:** a tree holding both lanes' current files ran `test_step_ingress`.
- **OM10:** read in place; only counts are reported.

---

## 1. Beads

### `.37.20` Usage placements read raw
- **Cause.** The adapter wrote OCCT's located placement for every usage, and OCCT misreads three things:
  - It composes an `item_defined_transformation` as item_1⁻¹·item_2, where the PDM Usage Guide r4.3 Example 14 gives item_2·item_1⁻¹.
  - It reads a mirror `cartesian_transformation_operator` as a rotation, defaults an omitted `axis3` to axis1 × axis2, and inverts the scale.
- **Fix** (`native/step_placements.hpp`, new). A C++ transcription of the independent oracle's Part 42 reading.
  - **Reading:** usage orientation comes from the representations' products. Frames in the swapped order are swapped, as WR2 allows; frames in neither order are refused. A reversed relation is read through its exact inverse. Part 42's default `first_proj_axis` is (0,1,0) for an axis along ±x.
  - **Agreement:** OCCT's value is written only where `placements::agree` holds. The raw value is written otherwise, with a `source_placement_raw` record.
  - **What `agree` requires:** every entry within 2⁻³² relative, and every translation entry within the resolution. The resolution is the smaller usable uncertainty of the two contexts, or 2⁻³² mm when neither declares one. In addition, the rotation difference must move no point of the child's extent by more than the resolution.
  - **Extent:** each face's carrier is boxed over its UV hint, and carried into the parent's frame through the raw placements.
  - **Refused by name:**
    - a usage with no placing relation, or with several;
    - a placing relation without a transformation;
    - an unorientable relation;
    - frames in neither representation;
    - any other operator, and WR1.
  - **Mirrors and scales:** written as read, and refused by the readers' one rigid rule.
- **Tests** (`tests/test_placement_oracles.py` (this lane's hunks) and `tests/test_step_reader_raw.py`):

| Case | Before | After |
|---|---|---|
| `axis3` omitted | matrix off by 0.29 | the standard's matrix, with a record |
| mirror | admitted as a rotation | det −1, refused by both readers |
| rotated item_1 | off by 0.35 | the guide's matrix to within 1e-6 |
| inventory and operator refusals (5 cases) | 3 exited 0 | all refused by name |
| scaled occurrence (`tests/fixtures/rigid-placements.tsv`, now 2·I) | OCCT's 0.5 | raw scale 2, refused |
| translation difference of 5e-7 mm at 5000 mm | OCCT's value, no record | raw value, with a record |
| rotation of 1.5e-10 rad over a child 5000 mm away | OCCT's value | raw value, with a record (moves points 7.5e-7 mm) |
| the same rotation over a near cylinder part (no pcurves) | raw, wrongly (OCCT's box spanned the whole carrier) | OCCT's value, no record |
| the oracle given OCCT's misread value | — | the oracle rejects it |

- **Corpus and OM10:** 0 placements written raw.

### `.37.26` Solid-bearing mapped items that no usage publishes
- **Cause.** A mapped item that brings a solid becomes OCCT geometry that never reaches `USAGES__`, so the occurrence was dropped with no record.
- **Fix.** `refuse_unpublished_mapped_items` refuses such a mapped item by name before the leaf binding, naming the item, its holder and the mapped representation. "Brings a solid" follows OCCT's own traversal: plain SRRs, and both sides of an MDADR.
- **Test.** `MappedItemTests`, covering an item placed directly, one placed through a relationship, and a part's own solid placed through a mapped item. Before, these exited 0 silently or failed with an unrelated message. After, each is refused by name. A mapped item with no solid is still admitted.
- **Points to:** admitting these solids is `.25.11`.

### `.37.27` Mapped-item cycles that OCCT recurses on
- **Cause.** OCCT transfers mapped items and the relationships sharing their representation recursively, binding nothing until each call returns. A cycle overflowed the stack, and the adapter died with SIGSEGV.
- **Fix.** `refuse_unbounded_transfer` runs a depth-first search before the transfer. It follows mapped items, exactly-typed plain SRRs and both sides of an MDADR, and refuses each cycle naming its representations, items and relationships. The constructive-geometry relationship is off by default in OCCT, so it is not modelled.
- **Tests:**
  - a relationship cycle, a pure mapped-item cycle, and a cycle through an MDADR: before, exit −11; after, refused by name;
  - holders at any depth (see `.37.32`).

### `.37.28` Closed B-spline carriers made periodic
- **Cause.** OCCT's `ConvertToPeriodic` turns closed B-spline surfaces periodic. The carrier table has no periodic form, so both readers refused the file.
- **Fix.**
  - **Raw net:** where OCCT's carrier is periodic, the adapter writes the file's own net, built from the raw entity in the same parameters. A `source_carrier_raw_net` record keeps the largest sampled distance between the two.
  - **Refusals:** a periodic raw knot vector is refused. So is any explicit-knot carrier whose knots, multiplicities or pole counts OCCT edits; that check builds the raw net independently of the face's location.
- **Test.** `test_closed_bspline_carrier_is_the_files_own_net`. Before: "invalid knot/control relation". After: both readers read the file, and the net equals the STEP text. A repeated knot that OCCT merges is now refused.
- **Scope:** only faces whose sole bound is a vertex loop. A face with edge loops on a closed B-spline carrier still stops at "unsupported closed/periodic spline chart" (`.25.12`).

### `.37.29` Carrier UV hint over every face
- **Cause.** Each carrier row was written at the first face on that surface, so later faces fell outside the hint.
- **Fix.** Hints are accumulated per carrier and written after every face, as the union of the faces' hints.
- **Test.** `test_carrier_hint_covers_every_face_on_it`, on `tests/fixtures/half-cylinders.step`. Before, a pcurve sample at u = 3.34 lay outside [0, π]. After, the hint covers every sample.
- **Corpus.** Six accepted files had 533 carriers that missed their own pcurves; now none do.

### `.37.30` Closed B-spline edge curves and file pcurves made periodic
- **Cause.** `StepToGeom` makes closed curves periodic, and does the same to closed 2D pcurves read from the file.
- **Fix.**
  - **3D curves:** the raw clamped net is written, with a `source_curve_raw_net` record.
  - **File pcurves:** the file's own clamped 2D net is written, with a `source_pcurve_raw_net` record. It must match OCCT's curve in the edge's parameter, or the file is refused by name.
  - **OCCT's own periodic constructions:** written unperiodized, which is an exact change of representation.
- **Tests.** `test_closed_bspline_edge_curve_is_the_files_own_net` (before: invalid knot relation; after: the raw rational net). `test_a_file_pcurve_made_periodic_is_written_as_its_raw_net` (a lens fixture: before, no records; after, two records, both nets equal to the STEP text).

### `.37.31` UV curves outside a B-spline carrier's natural domain
- **Cause.** Projected UV curves are fits that overshoot the knot rectangle between their sample points. OCCT's `AddUVBounds` then clipped the hint to the rectangle. On the corpus the overshoot reached 1.1e-5 in u and 6.6e-8 in v.
- **Fix** (`native/source_uv/source_uv.cpp`, `domain_source_chart`).
  - **Correction:** control points are moved into the rectangle (hull property). The 3D change is charged to the chord allocation, and the corrected curve must keep its sampled residual. Each corrected coedge gets a `source_chart_domain_correction` record.
  - **Refusal:** forms that cannot be corrected (rational, periodic, trimmed, other) are refused by name with typed fields, through `require_natural_domain` and `SourceUVDomainEscape`.
  - **Deferral:** a curve whose own residual already fails is left to the C/S residual check, so that check names the cause, as HEAD does.
- **Tests:**
  - `test_projected_uv_curves_stay_in_the_natural_domain`: before, a sample at u = −3.1e-9, outside the rectangle and the hint; after, every sample is in both, with one record.
  - `test_uncorrectable_escaping_uv_curves_refuse_by_name` (fixture-level: rational, periodic, trimmed and untrimmed circle): before, admitted; after, refused with typed fields.
  - `test_an_endpoint_off_its_vertex_is_not_charged_to_the_domain_correction`: admitted, as at HEAD.
- **Corpus:**
  - 5 accepted files had 190 UV curves corrected and 55 hints changed.
  - Carriers whose UV curves fell outside their hint went from 50, in 4 files, to 0.
  - One refused file changed reason: `Catheter.step`, from C/S residual to "natural domain: complete domain correction exceeds chord allocation", on a face processed earlier.
- **OM10:** 12 corrections; no carrier's UV curves fall outside its hint.

### `.37.32` Holder search depth
- **Cause.** `transfer_path` stopped at depth 8. A holder deeper than that counted as not transferred, so it escaped the unit gate and the `.37.26` check.
- **Fix** (`native/step_units.hpp`). The depth bound is removed and the visited set kept, so the search still terminates.
- **Tests.** `test_deep_holder_in_inches_refuses_by_its_factor`: before, admitted; after, refused with its 25.4 factor. `test_deep_holder_in_millimetres_is_admitted`: admitted before and after.

### `.37.36` Face orientation: same_sense against its bound and its shell
- **Cause.** The adapter wrote the raw use (same_sense composed with the shell) for every face. Two mechanisms produced inward faces:
  - OM10: 6 faces whose same_sense contradicts their own outer bound.
  - Cold section fan: two void shells inside out as a whole.
- **Fix.** The orientation design in §2.
- **Tests.** `OrientationTests`, `OrientationReviewTests`, `OrientationRound2Tests` through `OrientationRound6Tests`, `OrientationRound4Tests.test_a_neighbour_is_held_to_contact_outside_its_strip`, and the fixture `tests/face_regions_fixture.cpp`. Every one was red on the build before its change. Key results:

| Case | Before | After |
|---|---|---|
| cube, one face's same_sense flipped | volume 333.3 | S1, volume 1000 |
| cube, every face inside out | volume −1000 | S2, volume 1000 |
| cube, 4 of 6 faces' bounds flipped | 2 correct faces flipped | every face refused, with the shell record |
| apex cone, lateral face flipped | written −1, no record | S1; owned volume 261.80 |
| hemisphere bounded by its equator alone | refused (OCCT's volume 0) | pole face, read as written |
| two parts in one shell, one inside out | admitted inward / correct part turned | each part decided on its own |
| inward cube under a dome or apex | turned outward (S2) | refused by name |
| reviewer's torus-bump fillet reversed | written −1, no record | S1 by the contact test |
| boss fillets at three sizes, and the apex boss, reversed | contact test, 5–24 s (round 7) | S1 by the fold test, 0.04 s |
| boss fillets authored | contact test, 5–24 s (round 7) | read as written by the fold test, 0.04 s |
| gaps of 1e-9 to 5e-8 mm under the 1e-7 mm resolution | stood, unlike the touching case (round 6) | decide as touching (fixture level) |

- **Cross-check against the export lane:** the faces this lane corrects are exactly their "raw points into material" faces: 6 on OM10 and 13 on Cold section fan.

### `.37.39` UR-7 diagonal
- **Cause.** The test of a declared uncertainty against the definition's diagonal boxed the translated solid with BRepBndLib. That box spans an unbounded carrier where pcurves are missing, and misses a cone's apex where they are present.
- **Fix** (`native/shape_extent.hpp`, new). A sound bound from carriers and 3D edges only, needing no pcurves:

| Carrier | Bound |
|---|---|
| plane | its edges' box |
| cylinder | its edges' axial range |
| cone | its edges' axial range plus the apex |
| sphere, torus | the whole surface |
| B-spline, Bezier | the hull of its poles |
| revolution | the generatrix's box swept about the axis |
| extrusion | the generatrix's box along the direction over the edges' range |
| offset | the basis surface's bound, grown by the offset |
| any other | unbounded: recorded `diagonal_bounded = 0`, not usable |

  Curves are bounded the same way: conics exactly, splines by their poles.
- **Tests:**
  - `test_the_floor_is_taken_over_the_whole_cone`: before, diagonal 14.14 and 1.5e-14 mm usable; after, at least 17.32 and unusable.
  - The merged exporter test `test_sole_vertex_loop_on_a_trimmed_arc_revolution_refuses_that_face`: red with the first version, green with the revolution bounds.
- **Validation:** every accepted corpus solid (246), meshed from its prepared copy, lies inside its bound, and so does every OM10 solid (107).
- **Usability changes:** 0 corpus solids and 0 OM10 contexts.
- **For the exporter:** usable iff `shape_extent::shape_diagonal` is finite and `units::uncertainty_usable(mm, diagonal)` holds. `shape_extent::shape_bound` also serves as the extent for `placements::agree`.

---

## 2. Orientation design, as it stands

### 2.1 Per face: how its side and its volume share are computed
Every face is classified once. Its side is decided by one of these kinds, or the face is undecided. All volume shares are taken about the box centre of the face's edge-connected part.

| Kind | Region and side | Volume share |
|---|---|---|
| Edge face | OCCT's classifier: does its outer bound run counter-clockwise about its normal | OCCT's Gauss integration |
| Pole face | Exactly one edge loop winds a singular iso-line once, and holes do not wind. The pole is the face's vertex loop, or for edge loops alone a singular iso-line of the surface (a cone's apex, a sphere's pole, a degenerate row). Owned Green's sums are taken from that iso-line. On a surface with two poles, same_sense and the loop together pick the cap ("two-pole"). | Owned quadrature |
| Natural face | Vertex loops only; no bound, so the shell decides | Owned quadrature over the natural domain |
| Closed-surface ("two-region") face | A face on a closed surface (sphere, torus, closed spline). Its loops wind nothing, or form one band of two opposite winding loops. It reads the region to the left of its loops about its normal, so same_sense picks the region as well as the side, and it never contradicts its bound. A sphere band's complement is two caps, so the band is the only reading and S1 may apply. | Owned quadrature: inside, or natural domain less inside |

The UV hint, and the face's extent, reach the pole for every pole face, and cover the whole domain for every complement reading.

**Refusals at this level** (reason 7 `orientation_undecided`, by name):
- the edge loop does not wind the pole once;
- several loops wind the pole;
- a loop is irregular, changing the free coordinate by neither one period nor zero;
- loops that enclose no area;
- the region of the winding loop between the surface's two poles is not determined;
- region-ambiguous: a loop winds a closed surface other than once in one direction, windings do not cancel in one band, or a loop's UV curves do not join in one chart;
- no finite natural domain;
- several vertex loops.

### 2.2 Per edge-connected part of a shell
A shell is split into edge-connected parts. A shell with more than one part gets a `source_shell_components` record; that is connected_face_set's informal proposition, recorded but not refused. Each part is then decided in this order:

1. **Closure.** Every edge must be used exactly twice, once in each direction. Otherwise every face of the part is refused, with `source_shell_orientation_refused` and typed counts: edges used once, edges used more than twice, and edges used twice the same way.
2. **Nesting.** If a point of this part lies inside the box that holds another part's faces, it may be a cavity or an island, and every face of it is refused.
3. **A face already refused, or undecided.** The volume is no evidence, so every face of the part is refused with the shell record.
4. **Two-pole or two-region face, and the raw reading not verified.** Correcting that face alone and turning every face are both readings, so every face of the part is refused.
5. **Raw reading verified, no face contradicting, a two-region or two-pole face present.** The region test of §2.3 runs.
6. **S1.** Every face takes the side its own bound gives, written only if the part's signed volume on those uses has the sign its role requires: outer shell positive, void negative (ABSR WR4).
7. **S2.** No face contradicts its bound but the part encloses its region inside out: every face is turned, verified the same way.
8. **Otherwise.** The part is refused with the shell record.

Every correction writes a typed `source_face_orientation_raw` record. Every refused face carries reason 7, and its definition is marked incomplete.

### 2.3 The region test for two-region and two-pole faces
It runs per face, when the raw reading is the complement of its loops, either side of a band, or one side of a two-pole surface. Every such face gets a typed `source_face_region_choice` record, whether or not the test runs. It holds the region kind, the written and raw regions, `decided_by`, the fold evidence, whether the contact test ran and what it found, the blind band, the blind-band target and the resolution.

**Fold test first** (`tangent_fold`, `native/step_source/face_contact.hpp`).
- **When:** at an edge the face shares with a neighbour where the two are tangent (|n1·n2| ≥ cos 0.01 rad at all 17 samples).
- **Rule:** a reading whose outward normal opposes the neighbour's folds back over it, a zero dihedral angle. The edge uses fix the loop direction, so the other reading has the opposite normal there.
- **Outcomes:**
  - The raw reading is fold-free and the other folds: the raw reading stands (`decided_by = 1`).
  - The raw reading folds: the other reading is written as S1, if its volume verifies. Otherwise every face of the part is refused by name.
  - Both readings fold (at different edges, or with mixed signs along one): every face of the part is refused by name.

**Contact test** (`region_meets`). It runs only when no shared edge is tangent and the other reading also verifies (`decided_by = 2`). It asks whether the reading meets another face of its part within the resolution, away from its own edges.
- **Edge exclusion:** 10 × resolution plus the edge tolerance, the tolerance counted only up to 100 × resolution.
- **Measures:** distance (BRepExtrema), crossings (a non-destructive fuzzy BOPAlgo section, sampled at 1/1024 of the surface's size, each sample held to the resolution by its own measured distance), and 16×16 UV cells refined three levels near any contact.
- **Neighbour boxes:** `shape_extent::face_bound`. A pole face is measured on its carrier over its UV hint, because BRepExtrema, like BRepBndLib, uses a face's pcurve box.
- **Strips:** along each edge or vertex shared with a neighbour, a strip of half-width equal to the edge exclusion plus twice the smaller of resolution / sin(angle) and √(2·resolution / Δκ). Δκ is the difference of normal curvatures across the edge, at 17 samples.
- **Unbounded strips:** a strip with no bound, or one wider than ¼ of the surface's size, holds that neighbour to crossings only (`neighbours_crossings_only`).
- **Blind band:** cells touching a strip are split without a distance call until they are no larger than the target, max(10 × resolution, ¼ of the face's smaller principal radius), up to 10 levels. The band is recorded as `blind_strip_half_width_mm` plus `blind_cell_mm`.
- **Outcomes:**
  - The raw reading meets nothing: it stands.
  - The raw reading meets another face with certainty and the other reading meets nothing: S1, with a record.
  - The raw reading's only meets lie within 2× a strip's half-width (uncertain): refused by name, never corrected.
  - Cells larger than the target were dropped (undecided): refused by name, never corrected.
  - The other reading also meets a face, or is not shown to meet none: refused by name.
  - The test fails to complete: refused by name.

---

## 3. Corpus (500 files, read-only)

**Reasons, against HEAD**

| Reason | HEAD | Now |
|---|---|---|
| **Accepted** | **232** | **232** |
| Non-mm unit factor | 155 | 155 |
| C/S residual | 45 | 44 |
| Nonplanar multiple-wire | 39 | 39 |
| Leaf-product binding | 14 | 14 |
| Closed/periodic spline chart | 12 | 12 |
| Conic chart grade | 2 | 2 |
| Natural domain: complete domain correction exceeds chord allocation | — | 1 (`Catheter.step`) |
| Unowned solid | 1 | 1 |

- Every accepted file passes the C++ reader.

**Across the 232 accepted files**
- **Faces decided:** 14,861 edge faces and 549 closed-surface faces, with 0 refused.
- **No region records:** none of those faces is a complement, band or two-pole reading, so there are 0 region records and 0 contact tests.
- **Corrections:** 13 faces written by S2, all in Cold section fan's two void shells.
- **No other corrections or refusals:** 0 placements raw, 0 raw nets, 0 parts refused for closure or nesting, 0 shells with several parts.
- **Changes against HEAD:**
  - carrier hints in 6 files (`.37.29`);
  - UV curves and hints in 5 files (`.37.31`);
  - Cold section fan's 13 faces (`.37.36`);
  - UR-7 `diagonal_mm` values (`.37.39`), with no usability change;
  - the label.
- **Time:** 31.4 s total for the accepted files.

## 4. OM10 (counts only)

| | Value |
|---|---|
| Exit / C++ reader | 0 / reads |
| Definitions, faces, usages, products, `OCCSTYLE` rows | 107, 9,162, 196, 138, 9 |
| Faces by kind | 9,091 edge, 66 closed-surface, 5 pole; 0 refused |
| Corrections | 6 S1, in one definition (the export lane's 6) |
| Placements raw / raw nets | 0 / 0 |
| Natural-domain corrections | 12 |
| Usable uncertainty | 107 of 107 contexts |
| Region records | 36 |
| — decided by fold, read as written | 22 (21 bands, 1 two-pole side) |
| — contact test, meeting nothing | 12 bands |
| — neither test (the other reading does not verify) | 2 bands |
| — neighbours held to crossings only | 2 bands |
| Blind band, at most | 0.20 mm strip + 0.10 mm cell (target at least 0.1 mm) |
| Time | 25.0 s (about 17 s at HEAD) |

- Compared with HEAD, the changes are the 6 corrected face rows and their records, the 12 UV curves and 1 hint of `.37.31`, two cone hints widened by ≤ 5e-15 to reach their apex, the region records and the label.

## 5. Known limits, and where this is stricter than HEAD

**Stricter than HEAD, by design**
- **CLOSED_SHELLs that fail closure,** including sloppy exports with unshared duplicate edges: every face is refused, where HEAD admitted them. The corpus and OM10 have 0 such shells.
- **Nested parts:** a part with a point inside another part's face box, in the same shell, is refused, whether it is a cavity or an island. Box containment is conservative.
- **Unverified shells with a two-pole or two-region face:** for example, an inside-out hemisphere is refused, because correcting the one face and turning every face are both valid readings.
- **Uncertain or undecided contact:** refused by name, never corrected.

**Known limits**
- **Two valid readings:** where both readings verify, meet nothing and fold nowhere, the raw reading stands (G1). Example: the reversed ring on a block reads as a groove, and no recorded property tells it from the ring.
- **Blind band:** contact with a neighbour within its strip plus one dropped cell of the shared edge is not seen; every tested record states that band. A neighbour whose strip has no bound is held to crossings only, and counted.
- **Tangency threshold:** the fold test's tangency tolerance is 0.01 rad. Edges more creased than that use the contact test.
- **Pole faces with holes, and several winding loops:** implemented, but unreachable. source_uv refuses every nonplanar face with more than one edge loop (39 corpus files). The authored fixtures in `OrientationRound3Tests` record where they stop. → **`.25.13`**
- **Faces with edge loops on closed B-spline carriers:** still "unsupported closed/periodic spline chart" (12 corpus files). → **`.25.12`**
- **Solids placed through mapped items:** refused by name (`.37.26`); the reference exporter admits them. → **`.25.11`**
- **Two-region faces cut off the iso-lines:** for example the reviewer's x-axis truncated sphere stops at source UV, C/S residual. Its rule is tested at function level only.
- **Carrier types the source format lacks** (offset, revolution, extrusion): the adapter refuses them by name. `shape_extent` bounds them for the exporter.
- **Non-mm sources:** still refused (155 files). → `.25.6`
- **Owned for the exporter to mirror:** `placements::agree`, `shape_extent::shape_diagonal` and `shape_bound`.

## 6. Tests and runs

- **Suites (final):**
  - All pass: `test_step_reader_raw` (51 tests, 24 s), `test_step_ingress` (72), `test_placement_oracles` (20), `test_source_uv` (8), `test_imported_source` (5), `test_imported_assembly` (5), `test_om10_scene` (2), `test_imported_geometry` (6), `test_imported_meshing` (35), `test_imported_domain_sweep` (8), `test_reference_source` (4), `test_om10_nvidia` (11), `test_measurement_lane` (38).
  - om10 ctest passes 5/5.
  - The native mesher ctest passes 130/132; the two failures are the retired cylinder-seam pair (`.37.11`).
  - `test_nvidia_prepared_pipeline` stops at its environment check (no `pxr` module), unchanged throughout.
- **Merged tree:** `test_step_ingress` passes 72/72 with both lanes' current files. The final merged run of both lanes is reported in [`../b2-reader-export-2026-10-18/REPORT.md`](../b2-reader-export-2026-10-18/REPORT.md).
- **Fixture times:** every boss fillet, authored or reversed, in every variant, takes 0.03–0.04 s with no contact test. The ring on block takes 1.5 / 4.4 s and the reversed torus bump 0.37 s, both on the contact test.

## 7. Files

**New**
- `native/step_placements.hpp`: raw placements, the agreement predicate, mapped-item and cycle refusals.
- `native/shape_extent.hpp`: sound shape, face and curve bounds.
- `native/step_source/face_regions.hpp`: pole and two-region readings, Green's sums, quadrature, face extents.
- `native/step_source/face_contact.hpp`: fold test, contact test, strips, blind band.
- `tests/test_step_reader_raw.py`
- `tests/face_regions_fixture.cpp`
- `tests/fixtures/half-cylinders.step`
- `tests/fixtures/half-cone.step`

**Changed**
- `native/step_source/main.cpp`
- `native/step_source/CMakeLists.txt`: links TKBO; adds the face-regions fixture target.
- `native/source_uv/source_uv.cpp`
- `native/source_uv/source_uv.hpp`
- `native/step_units.hpp`
- `native/step_vertex_loops.hpp`: reason 7.
- `tests/source_uv_fixture.cpp`
- `tests/step_styles_fixture.cpp`
- `tests/fixtures/rigid-placements.tsv`
- This lane's hunks only, in the shared `tests/test_placement_oracles.py` and `tests/test_step_ingress.py`.

**Unchanged, because the format did not change**
- `native/mesher/source.cpp`
- `native/mesher/source.hpp`
- `python/brepgpu/imported_source.py`
- `native/step_styles.hpp`
