# B2-oracles: placement oracle and the one placement rule

This is unit B2-oracles of maneuver B2. Its brief is
[docs/fast-mesher-campaign/briefs/B2-lane-defects.md](../../docs/fast-mesher-campaign/briefs/B2-lane-defects.md),
and it fixes beads `brepgpu-12s.37.12` and `.37.13`. The lane also fixed
further findings of its own, and the findings of two adversarial reviews.

**Where it ran.** Code was edited in the laptop checkout. Builds and tests ran
only on the fleet host, in a lane directory holding HEAD plus this unit's files.

**Who wrote this report.** The dispatcher wrote it from the lane's returns.

**Review.**
- First review: no blocker, three should-fix items.
- Second review: no blocker, one should-fix item. It was resolved by restoring
  a named refusal.
- Every finding is fixed.

## 1. The oracle now reads `CARTESIAN_TRANSFORMATION_OPERATOR_3D` (`.37.12`)

**The defect.** `_original_usage_transforms` in
`python/brepgpu/imported_oracles.py` read only `ITEM_DEFINED_TRANSFORMATION`.
Any usage placed by a cartesian operator therefore could not be checked at all.

**Why it matters.** The readers take placements from OCCT's located result
(`native/step_source/main.cpp:435`). Lane M4-G1 measured three ways OCCT 8.0.1
misreads these operators (`StepToGeom.cxx:2153-2213`):
- a mirror becomes a rotation;
- an omitted `axis3` defaults to `axis1 × axis2`;
- scale S becomes 1/S.

The first two yield proper rotations, so both readers admit the wrong placement
silently. Only an independent raw reading can catch them.

**Mechanism.** The operator is read per ISO 10303-42, as given in the AP242 MIM
long form:
- **Scale:** `scl = NVL(scale, 1)`. WR1 requires `scl > 0`.
- **Axes:** `u = base_axis(3, …)`, with `first_proj_axis` as written.
  `second_proj_axis` is evaluated as `y = sign(arg·(z×x))·(z×x)`, which equals
  the standard's projection in exact arithmetic and keeps `u` orthonormal to
  rounding. `x` is projected off `z` twice.
- **Map:** P → origin + scl·u·P, from rep_1 into rep_2.
- **Mirrors:** `u[2]` keeps the sense of `axis2`, so a mirror has determinant −1.
- **Named refusals:**
  - "axis1 is parallel to axis3";
  - "axis2 lies in the axis1-axis3 plane";
  - WR1;
  - zero direction;
  - 2D operator.

**Fixtures.** Authored edits of `tests/fixtures/source-uv.step`:

| Fixture | Expected oracle result |
|---|---|
| Rigid twin | passes |
| All defaults | passes |
| Mirror | refused; OCCT reads it as a rotation |
| Omitted `axis3` | refused; OCCT turns it 45° |
| Scale 2 | oracle reads 2; OCCT reads 0.5 |
| Near-plane `axis2` | passes, with `u` orthonormal to 4ε |
| Each invalid case | refused by name |

**Before:** every case failed with "original geometry reference missing", and
the readers admitted the misread cases as rigid.

**After:** rigid cases pass within 2^-32, and the misread cases refuse with
"placement differs".

## 2. The stricter placement rules are folded into the one predicate (`.37.13`)

**The defect.** Three consumers refused placements that both readers admit:
- `imported_assembly.rigid_transform`: 1e-10 on Gram entries and on |det−1|;
- the legacy display profile's `orthogonal`: 2e-10;
- `native/om10_reference/raw.hpp:93`: 2e-10.

The readers' rule, `brepgpu::rigid_placement_status`, is Gram entries within
τ = 1e-9 and determinant > 0.

**Justification.** This is now written into the header of
`native/rigid_placement.hpp`. Gershgorin on MMᵀ gives |σ²−1| ≤ 3τ, so:
- |σ−1| ≤ 3τ/(1+√(1−3τ)) = 1.5τ(1+0.75τ+…);
- |det−1| ≤ (1+1.5τ)³−1 = 4.5τ + 6.75τ² + 3.375τ³;
- normals carried by `rotate(M, n)` and renormalised satisfy tan θ ≤ 3τ/(1−3τ).

Every consumer applies M as given. None uses Mᵀ as an inverse; the header lists
the consumers checked. The tolerance therefore only decides whether a placement
belongs to the rigid class. Nothing downstream needs a tighter bound, so every
rule was folded and none kept stricter.

Composed occurrences go through the same predicate. A deep assembly can
therefore refuse at composition, by name, even when every usage was admitted.

**Tests.**
- The corpus `tests/fixtures/rigid-placements.tsv` (13 rows) is checked against
  the reader, the source display, the legacy display, the new raw-reference
  admission test and the Python assembly.
- A composed chain: 5e-10 then 1e-9 is admitted; 1.5e-9 is refused.

**Before:** the legacy display and the raw reference disagreed on 2 rows each,
and the Python assembly refused both rows.

**After:** all five consumers agree on all 13 rows.

## 3. Relation orientation in the oracle

**The defect.** The oracle assumed rep_1 is always the child. OCCT does two
things it did not:
- it inverts relations whose representations' products reverse the NAUO
  (`STEPConstruct_Assembly::CheckSRRReversesNAUO`);
- it swaps misassigned frames (`STEPControl_ActorRead.cxx:2425-2463`).

The old oracle therefore refused legal reversed or swapped files. It also
silently accepted relations that cannot be oriented.

**Mechanism.**

*Which products a representation holds.*
- A representation's products are its own shape definitions whose product
  definition shape names a `PRODUCT_DEFINITION`.
- Plain shape representation relationships extend that set, except one that is
  itself a placing relation.

*Direction.*
- A reading needs at least one side with products.
- Every side that has products must hold its expected definition: child on
  rep_1 and parent on rep_2 for forward, the reverse for backward.
- Exactly one reading must hold. Otherwise the relation refuses with "does not
  orient its usage".
- A reversed relation is read through its exact inverse.

*Frames.*
- An item-defined transformation's frames are read as written if they meet WR2.
- They are swapped if only the swapped order meets WR2.
- Otherwise they refuse by name.

*Other named refusals.*
- A placing relation with no transformation, simple or complex form: "original
  placing relation carries no transformation". The rework had briefly read it
  as the identity; the second review found that ungrounded, so the refusal was
  restored.
- A placement whose product definition shape names a non-NAUO relationship.

**Spec oracle.** PDM Schema Usage Guide r4.3 §3.4.3, Examples 14 and 15, where
the guide states C = B·A⁻¹ and v = u − C·t. Example 15 as printed gives
`axis2`'s z component as +0.866, which would make it a mirror. Its own C, and
`axis1 × axis2 = axis3`, require −0.866, so the fixture corrects the sign. The
reviewer confirmed the correction by recomputing C from Example 14.

**Tests.** `RelationOrientationTests`:
- reversed, swapped and reversed-operator relations, at records level and end to
  end;
- the guide examples;
- a reversed scale-2 inverse;
- an unrelated product's representation;
- one representation on both sides;
- unowned representations;
- a both-readings ambiguity;
- a frame in neither representation;
- the non-NAUO placement;
- the untransformed placing relation, at records level and end to end.

**Effect on G1's P3 placements.**
- The 43 swapped placements in 5 files now read as OCCT reads them.
- The 54 whose frames belong to neither representation, in 4 files, refuse by
  name; OCCT uses them as given.

## 4. Reference display build

The reference display executable did not build at HEAD with GCC 13:
- `raw.hpp` used `std::set` without including `<set>`;
- `cpu.cpp` used five functions from `om10/math.hpp`. It used to get them through
  `om10/scene.hpp`, which commit 862c26d stopped including.

Both includes are added. **Before:** the build failed, and `test_om10_reference`
had 20 errors. **After:** it builds with no warnings and passes 6/6.

## Read-only oracle check over OM10 and the corpus

For each file, on the fleet:
1. The adapter extracts the source.
2. `assemble` and `qualify_original_geometry` run, then
   `qualify_original_assembly`.
3. Step 2 runs once with HEAD's Python and once with this unit's.

Derived tables were deleted after each file. Nothing from the OM10 source was
copied or recorded beyond its outcome.

| Source | Before | After |
|---|---|---|
| OM10 | ok | ok, including the final round |
| Corpus: 500 files, 230 accepted by the adapter | 144 root assembly differs; 78 geometry reference missing; 4 complete IDs; 3 uncertainty differs; 1 carrier IDs | identical, file by file |

There were no new refusals. The corpus does not exercise placements: all 25
assembly files fail before the placement code. OM10 is the only real assembly
that reaches it.

## Suites (fleet lane directory, final)

| Suite | Result |
|---|---|
| `test_placement_oracles` (new) | 18 OK (red on earlier code at every round) |
| `test_om10_reference` | 6 OK (did not build at HEAD) |
| `test_step_ingress`, `test_source_uv`, `test_imported_source`, `test_imported_assembly`, `test_reference_source`, `test_om10_scene`, `test_imported_geometry`, `test_imported_meshing`, `test_imported_domain_sweep` | 22, 8, 5, 5, 4, 2, 6, 35, 8 OK |
| `test_om10_native`, `test_face_domain`, `test_face_domain_external`, `test_native_face_domain_integration`, `test_direct_cdt_oracle` | 10, 11, 3, 6, 12 OK |
| Native mesher ctest | 127/129 |
| om10 ctest (CPU and both P100s) | 3/3 |
| om10_reference ctest (new) | 1/1 |

- The two native mesher failures are the old mesher's cylinder-seam pair
  (`.37.11`, retired). They are unchanged from unmodified code.
- The four NVIDIA suites need the NVIDIA builds and could not run in this lane.

## Further findings, filed

- `brepgpu-12s.37.20`: the reader misplaces item-defined placements whose first
  frame is not the identity.
  - OCCT computes item_1⁻¹∘item_2 (`STEPControl_ActorRead.cxx:2488`;
    `gp_Trsf.cxx:172-194`), where the standard and the PDM guide give
    item_2∘item_1⁻¹.
  - 349 corpus placements are affected, all in the CR-10 package. That file is
    refused today for another reason.
  - The reader also admits misread operator mirrors as rigid.
  - The oracle now refuses all of these by name. This is a follow-on unit in the
    reader.
- `brepgpu-12s.37.21`: `qualify_original_geometry`'s `representation_links`
  still joins placing relations.

## Notes for M8

- Placement translations in the oracle are read without the context length
  factor. That is correct only while admission is mm-only (bead
  `brepgpu-12s.25.6`).
- Reader-side raw placement reading belongs to bead `brepgpu-12s.25.5`.
