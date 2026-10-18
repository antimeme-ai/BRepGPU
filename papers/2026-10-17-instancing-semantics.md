# Instancing key and per-occurrence semantics

Research lane M4-G9 of the fast-mesher campaign, 2026-10-17. Brief:
[docs/fast-mesher-campaign/briefs/M4-G9.md](../docs/fast-mesher-campaign/briefs/M4-G9.md)
with [M4-common.md](../docs/fast-mesher-campaign/briefs/M4-common.md). Stub:
[lifecycle map](2026-10-17-geometry-lifecycle-map.md) §4.1 G9 (stage S10;
contradictions X3, X6). Bead `brepgpu-12s.10`.

Nothing was built, meshed or benchmarked, and no NVIDIA run was started. The
lane read the doctrine, the operator-decision journal (gate G1), the campaign
plan, the lifecycle map (S3, S10, S12, S13, S17, S18, §3, §4), the working-draft
design, the sibling contact-interfaces paper (G17), the repository's assembly,
reader, display and NVIDIA-driver code, OCCT 8.0.1, the pinned NVIDIA usd-brep
source, the format specifications and two slicers' import code. Existing NVIDIA
reference artifacts (`results/om10-nvidia-complete-2026-10-17/`) and the fleet
corpus were read with `grep`/`awk` tallies only. Sources, read depth and hashes
are in [2026-10-17-instancing-semantics-sources.tsv](2026-10-17-instancing-semantics-sources.tsv).

Tags as in the lifecycle map: **[src]** a source establishes it; **[code]** read
in this repository or a pinned reference at file:line; **[data]** tallied from an
existing artifact; **[own]** this lane's derivation; **[inf]** inferred from a
source but not stated by it. Line numbers in files that other lanes are editing
today (`native/mesher/source.cpp`, `native/om10/validation.hpp`,
`python/brepgpu/imported_source.py`) are as read on 2026-10-17.

Operator decisions that bind this lane: **D-placements** (rigid and mirrored
placements share one mesh; uniform scale gets a mesh per scale with ε scaled
to match; non-uniform scale and shear are refused explicitly), **D-determinism**
(identical output across thread counts on one backend; functionally identical
across backends; one model may mix backends), **D-formats** (STL; 3MF with
separate items, not united components; glTF with face ids and normals; USD and
AP242 tessellated later), and **D-measure** (account per definition and per
occurrence).

## 0. Answer

### 0.1 Placement table

A is the 3×3 linear part of the composed occurrence placement (root to leaf,
in definition units after D-units), t its translation, σ_max ≥ σ_min the
singular values of A. "Key" is the mesh key of §2. "Emit" is what each writer
does with the shared definition-frame mesh; triangles in that mesh are
outward-wound in the definition frame and carry per-corner unit normals n.

| Placement kind | Test on raw A (§1.3) | Admitted | Mesh key | Owner | STL (flattened) | 3MF (one build item per occurrence) | glTF (one node per occurrence) | Normals at emit |
|---|---|---|---|---|---|---|---|---|
| Rigid | det > 0, κ = σ_max/σ_min ≤ 1+τ, \|σ−1\| ≤ τ | yes | (definition, ε, request lengths, versions) | one mesh task per key | x′ = Ax+t; vertex order kept | item → key object, transform = [A\|t] | node → key mesh, TRS | n′ = A n |
| Mirrored | det < 0, otherwise rigid | yes, **same key as rigid** | unchanged: handedness is not in the key | same task | x′ = Ax+t; **reverse each triangle** (a,c,b) | default: item → the key's opposite-hand object (reflected once by a fixed M, reversed winding) with transform A·M (det > 0); spec path (item transform with det < 0) behind a flag until consumers are measured (§3.3) | node with negative-determinant TRS (negative scale component); **no** re-wind (glTF §3.7.4 makes the consumer reverse) | n′ = A n (outward stays outward; never the cofactor) |
| Uniform scale s ≠ 1 | det ≠ 0, κ ≤ 1+τ, \|σ−1\| > τ | yes | ε and every length-valued request divided by σ_max; angles unchanged | one mesh task per (definition, scale class) | x′ = Ax+t; reverse iff det < 0 | item → that scale class's object (definition frame), transform = [A\|t] | node TRS with uniform scale | n′ = normalise(A n) |
| Mirrored and scaled | det < 0, κ ≤ 1+τ | yes | as uniform scale | as uniform scale | as uniform scale, reversed | as mirrored, using the scale class's objects | as mirrored | as uniform scale |
| Non-uniform scale | κ > 1+τ and AᵀA diagonal in the definition axes | **refused** per occurrence with a record | none | — | — | — | — | — |
| Shear or oblique stretch | κ > 1+τ, otherwise | **refused** per occurrence with a record | none | — | — | — | — | — |
| Singular or non-finite | σ_min ≤ τ_abs or any non-finite entry | **refused** per occurrence | none | — | — | — | — | — |

Colour, visibility and occurrence identity never enter the mesh key; they
change only what the emitter writes (§5). Two consequences of the format rules:
a per-occurrence colour variant costs a duplicated object (geometry copy) in
3MF, but only a duplicated mesh object with shared accessors (no geometry copy)
in glTF.

### 0.2 Accounting rule for comparisons with NVIDIA

1. **The unit of meshing work is one mesh key.** The meshing stage of an input
   is the set K of keys implied by its admitted occurrences: one per
   (definition, scale class). Mirrors and colours add no keys.
2. **NVIDIA gets exactly K.** For each key, the NVIDIA arm receives one
   definition-frame `.brep` and the chord ε_key = ε/σ_max (angle settings
   unchanged, since angles are invariant under similarity). This is what the
   existing driver already does for the rigid watch: 107 definitions, 107
   `tessellate` calls, 166 occurrences placed afterwards by our own code
   (§6). It is NVIDIA at its strongest: both of NVIDIA's native assembly paths
   tessellate once per occurrence (§6.2).
3. **The speed ratio compares owned and NVIDIA over the same K**, in CPU-s and
   in wall time, with the per-key rows retained. Per-occurrence expansion
   (transforming, re-winding, colour variants, writing items, nodes or
   flattened facets) is the **emit stage**, timed separately, and runs through
   the same owned emitter for both producers' meshes, as the shared display
   already does for images.
4. **Counts reported with every comparison:** definitions, keys, occurrences,
   and occurrences per placement kind, plus per-key and per-occurrence
   triangle totals. Occurrence-weighted work is context only and never the
   denominator of a ratio whose other side is per key. On the watch the
   mismatch would be 17% in NVIDIA tessellation CPU-s and 21% in triangles
   (§6.3).
5. **Refused placements** (non-uniform, shear, singular) are excluded from the
   speed ratio and listed. How §9.6 scores them when NVIDIA has a valid arm is
   an operator question (§10).
6. D-measure's "per definition" is per key; its "per occurrence" is the emit
   stage plus the occurrence-weighted context in item 4.

## 1. How placements reach us

### 1.1 What the exchange standards allow

- **Assembly transforms are rigid in STEP practice.** "Scaling and mirroring
  shall not be applied in the context of assemblies"; the CTO scale "shall be
  empty … or shall be 1.0" and its matrix determinant "shall equal 1"
  [src: PDM Schema Usage Guide 4.3 §3.4.2.1, §4.4, §4.4.2, §4.7.2]. The 2025 AP242 Ed.4
  assembly-structure practice repeats it: "The use of mirroring in the 3D
  transformation from a component part in an assembly part is not allowed"
  (§7.3); the scale of a CartesianTransformation "shall be omitted or set to
  1.0" (§7.3.1, Table 47) [src: CAx-IF/JT-IF RP AP242 XML Assembly Structure
  v4.0].
- **A mirrored part is a different part.** The same RP (§7.7): mirroring "is
  forbidden along the assembly structure (transformation matrix determinant
  shall be always +1), because in a bill of materials, any Part modified this
  way can never be the same Part as the original". A "fully" mirrored part is
  either two CAD files (the derived file references the original) or **one CAD
  file whose opposite-hand part "references the common geometry together with
  a mirroring matrix"** (§7.7, §7.7.2). That matrix includes "translation,
  rotation, and mirroring, i.e., inversion …; scaling is not included"
  (§7.7.2). Mirroring of whole assemblies is out of scope (§7.7, §7.7.2.1). The
  CAx-IF alternative-shapes RP also treats mirrored parts as separate products
  [src: lifecycle map S3, L2].
- **Where scale and mirror can appear at all.** Part 42's
  `cartesian_transformation_operator` "also allows for scaling and mirroring"
  [src: PDM UG §4.4.2]; `axis2_placement_3d` pairs in
  `item_defined_transformation` are right-handed by construction, so
  placements built from them are rigid [inf: Part 42 `build_axes`, via the PDM
  UG §3.4]. 3MF items/components carry any affine 3×4 matrix ("SHOULD NOT be
  singular") [src: 3MF Core 1.4.0 §3.3]; glTF node matrices "MUST be
  decomposable to TRS" and "cannot skew or shear" [src: glTF 2.0 §3.5.3]; USD
  composes any xform; Parasolid instance transforms "must be a rigid motion or
  a reflection: no scale, shear or perspective" [src: Parasolid V35 FD §14.2.9].

### 1.2 What the inputs we hold contain

- **The watch is all rigid.** All 166 composed occurrence matrices in the
  NVIDIA reference BOM have det ∈ [1 − 1.1e-16, 1 + 4.4e-16] and
  orthonormality error ≤ 4.4e-16 [data: `heal0/bom.tsv`]. 107 definitions,
  166 occurrences; 20 definitions are used more than once, at most 12 times
  [data]. The STEP holds 196 `ITEM_DEFINED_TRANSFORMATION`, one `MAPPED_ITEM`
  and no `CARTESIAN_TRANSFORMATION_OPERATOR` [data: grep of the private
  source].
- **The whole fleet corpus has no mirror or scale source.** None of the 500
  STEP files under `corpus/sources/` contains
  `CARTESIAN_TRANSFORMATION_OPERATOR_3D`; 27 files are assemblies (4,250
  `NEXT_ASSEMBLY_USAGE_OCCURRENCE` in total), the largest being the Voron 2.4r2
  (1,551 NAUO) and Trident (1,619 NAUO, 585 solids) full-assembly TDP files
  [data: grep on the fleet host]. Mirrored, scaled, non-uniform and sheared
  placements are therefore reachable only through authored families (design
  §9.4 already lists "mirrored and scaled placements"). Every claim in this
  paper about them is untested on real input until those families exist, and
  the watch cannot overfit them.

### 1.3 Classifying a placement

**What OCCT does to it first.** OCCT's `StepToGeom` builds the CTO frame as
`gp_Ax3(P, D3, D1)`, which reconstructs Y as D3×D1 and discards the file's
`axis2`, so a mirroring CTO becomes a rotation; it then applies the scale factor
[code: `quarantine/new-iteration/occt/src/DataExchange/TKDESTEP/StepToGeom/StepToGeom.cxx:2160-2210`].
Our STEP adapter takes usage matrices from OCCT's located result
(`transform(s.Location().Transformation())`, which includes the gp_Trsf scale)
[code: `native/step_source/main.cpp:156, 308`]. A mirrored opposite-hand part
written per RP §7.7.2 would therefore be meshed and placed unmirrored, silently.
This is lifecycle-map finding 1 (S3 "mirror read as rotation") and belongs to
G1 and the raw-entity reading it calls for; it is restated because this table
depends on it: **the placement kind must be computed from the raw STEP
placement, not from OCCT's location.**

**The test [own].** From the raw composed A (3×3, double):

- reject non-finite entries;
- G = AᵀA; σ_max² and σ_min² from G's eigenvalues (closed-form for 3×3, or the
  bounds already used: σ_max ≤ √(max row sum of |G|), as in
  `native/mesher/assembly.cpp:40-75`);
- singular if σ_min ≤ τ_abs; uniform if κ = σ_max/σ_min ≤ 1+τ; rigid if also
  |σ_max − 1| ≤ τ and |σ_min − 1| ≤ τ; mirrored if det A < 0.

The tolerance τ only decides admission, not correctness: the emitter applies
the actual A and the key's ε uses the actual σ_max (§4.1), so an
almost-uniform A that is admitted still meets ε. What τ guards is the
D-placements intent (non-uniform and shear refused) and the angle criteria,
which a non-similarity distorts by up to about κ−1 radians [own]. Today's
readers use an absolute 1e-9 on |AᵀA − I| [code: `native/mesher/source.cpp:648-657`;
`python/brepgpu/imported_source.py:296-301`]. Proposed default τ = 1e-6
**[call]**, with hypothesis H5 (§8): no real placement has κ in (1+1e-9,
1+1e-3).

## 2. Question 1: the mesh key

### 2.1 What mature systems key on, and how they fail

- **OCCT keys on the TShape.** BRepMesh visits each face once with its location
  stripped (`aFaceNoLoc.Location(aEmptyLoc); if (!aFaceMap.Add(...)) continue`)
  [code: `occt/src/ModelingAlgorithms/TKMesh/IMeshTools/IMeshTools_ShapeExplorer.cxx:75-97`],
  meshes it in the first occurrence's located frame and stores the result in
  the definition frame by applying the inverse location
  [code: `BRepMesh_ShapeTool.cxx:191-207`]. A later call reuses a stored
  triangulation when its deflection passes `IsConsistent` against the request,
  with no scale term [code: `BRepMesh_ModelPreProcessor.cxx:49-90`;
  `BRepMesh_Deflection.cxx:158-168`]. Under a scaled second occurrence the
  world error is s₂/s₁ times the request, silently [inf; lifecycle map S10].
  Its 2012 data race (Mantis 0023260) came from two occurrences locking
  separately; the fix keyed the lock on the TShape [src: lifecycle map S10,
  L7, commit f67d051].
- **USD keys prototypes on composition.** UsdStage computes an "instancing key"
  from the instance's composition arcs and metadata; descendants of instances
  "cannot be overridden", which "guarantee[s] that every prim with the same key
  will have the same scenegraph hierarchy beneath them"
  [src: OpenUSD Scenegraph Instancing]. Transforms are outside the key.
- **Parasolid reuses facets under rigid motions and reflections only**, and
  only when "the application is able to transform the facets displayed"
  (`PK_facet_incr_tf_reflection_c`) [src: Parasolid V35 FD §107.2.16.1];
  instance transforms cannot scale [src: FD §14.2.9].
- **Identity is not content.** "Cache keys contain zero identity fragments;
  identity contains zero geometry" (brepjs ADR-0014) [src: lifecycle map S17,
  L6].

### 2.2 The key [own, from the above and D-placements]

A mesh key is the tuple

1. **definition content identity**: the source digest and the definition's
   source id (lifecycle map S3 requirement: file SHA-256 plus Part 21 `#id`,
   plus UUIDs where the persistent-ID RP supplies them), never the product
   name or the occurrence;
2. **effective request in definition units**: ε_def = ε / σ_max, and every
   other length-valued request parameter (maximum edge length, small-feature
   thresholds η and β, proximity limits) divided by σ_max; angle-valued
   parameters unchanged. With G17's per-face tolerance map, the map itself is
   part of this field: ε_f = min over the class's occurrences and declared
   contacts of ε_contact/σ_max [src: contact-interfaces §2]. **Correction to
   G17's formula:** the minimum runs over the occurrences of one scale class,
   not over all occurrences of the definition, or D-placements' "mesh per
   scale" collapses back into "mesh at the tightest scale";
3. **ingress interpretation**: the healing threshold set and the master-
   representation and units policy, since they change the geometry being
   meshed;
4. **producer identity**: mesher code and algorithm version, flag
   configuration, and the backend and determinism class when a key crosses
   runs (G7 defines this field; within one run there is one producer per key,
   item 2.3).

Not in the key: placement translation and rotation, handedness, colour,
material, visibility, occurrence id, explosion offset. Two definitions with
byte-identical geometry but different source ids get two keys; content
deduplication across definitions is a cache question for G7, not instancing.

### 2.3 Ownership

One mesh task per key, created when the task graph is built, before any
meshing. Occurrences hold read-only references to the finished, immutable key
result; emission tasks read it concurrently. No occurrence-level locks exist,
so the OCCT 0023260 race has no analogue, and the shared-edge "one producer per
edge" rule (G7) is unaffected because B-rep edges are definition-local (no
imprint, D-imprint). Per D-determinism, the key result is bit-identical across
thread counts on one backend, so occurrences emitted from it are too.

Today's code already meets "one mesh per definition": it meshes each of the 107
definitions once and places occurrences only in display
[code: `native/mesher/adaptive_model_main.cpp:249-270, 368-449`;
`native/om10/math.hpp:164-178`] and takes the per-definition allowance as the
minimum over instances of (ε − storage error)/‖A‖ [code: `native/mesher/assembly.cpp:161-171`].
That minimum is the "tightest scale" rule; under D-placements it becomes a
minimum per scale class.

## 3. Question 2: mirrored placements

### 3.1 The geometry [own]

For x′ = Ax + t with det A < 0 and A orthogonal:

- A triangle (a, b, c) maps to (Aa, Ab, Ac), whose right-hand normal is
  cof(A)·((b−a)×(c−a)) = det(A)·A⁻ᵀ(…). With det A < 0 it points into the
  mirrored solid: **winding must be reversed** to keep outward orientation.
- The outward normal of the mirrored surface is A⁻ᵀn = A n (A orthogonal). The
  cofactor form that the display uses for positive determinants
  (`cofactor_normal`) [code: `native/om10/math.hpp:177-178`] would point
  **inward** under a mirror; normals must use A⁻ᵀ, never cof(A), once mirrors
  are admitted.
- UV charts do not change, so a mirrored occurrence's triangles are negatively
  oriented in their face's UV chart after re-winding. Design §4.4's
  "positively oriented in its face's UV chart" therefore holds for the
  definition mesh only; for a placed occurrence the binding statement is
  outward orientation in world space.
- Seams, shared-edge identity and edge-use counts are index relations and are
  unchanged by re-winding.

OCCT's mesh iterator does exactly this: it reverses a triangle when
`(face REVERSED) xor (det < 0)` and transforms normals by the location
[code: `occt/src/DataExchange/TKRWMesh/RWMesh/RWMesh_FaceIterator.hxx:96-122`;
`RWMesh_FaceIterator.cxx:108`]. OCCT's **STL writer does not**: it applies the
location to nodes and swaps vertices only for REVERSED faces
[code: `occt/src/DataExchange/TKDESTL/StlAPI/StlAPI_Writer.cxx:95-121`], so a
mirrored occurrence is written inside-out. That is a reference finding, not our
defect; it shows the rule must be explicit in our emitter and in its oracle.

### 3.2 Per format

| Format | Rule from the source | Our emit rule |
|---|---|---|
| STL | No instances; facet vertices counter-clockwise seen from outside, right-hand rule [src: lifecycle map S13; Burns, reproducing the 3D Systems spec; Hiller–Lipson 2009] | Flatten; x′ = Ax+t; reverse vertex order when det < 0; facet normal from the emitted vertices (readers recompute it) |
| 3MF | "When applying a transformation with a negative determinant, the resultant shape MUST NOT change the sign of its volume"; triangle normal is (B−A)×(C−A) outward [src: 3MF Core 1.4.0 §3.3, §4.1, §4.1.4.1]. Core 1.3 added "mirror"; 1.4.0 removed it as deprecated [src: change history] | Default: one opposite-hand object per (key, colour variant), made once by reflecting the key mesh with a fixed M = diag(1,1,−1) and reversing winding; each mirrored item references it with the transform A·M, whose determinant is positive (M is an involution, so A·M applied to M·x gives A·x). Flag: spec path, item transform with det < 0 and the original object. §3.3 decides the default |
| glTF 2.0 | "the determinant of the node's global transform defines the winding order … If the determinant is a positive value, the winding order triangle faces is counterclockwise; in the opposite case, the winding order is clockwise" [src: glTF 2.0 §3.7.4]; the same mesh may be used by many nodes (§3.7.4); TRS only (§3.5.3) | One mesh per key; mirrored node TRS with one negative scale component (a reflection composed with a rotation is TRS-decomposable); no re-winding; normals stay in the definition frame |
| USD (later) | "if the determinant is less than zero, then the gprim's orientation has been flipped, and therefore one must apply the opposite handedness rule" [src: OpenUSD `usdGeom/overview.dox`, UsdGeom_WindingOrder]; `orientation` defaults to rightHanded [src: `usdGeom/schema.usda:649-656`] | Prototype per key; mirrored instance xform with det < 0; leave `orientation` rightHanded |
| OBJ, PLY | No instances [src: lifecycle map S13] | Flatten as STL; per-corner normals A n |

Every writer's oracle is the reader-equivalent re-import of G3: after re-import
and the consumer's own transform, every closed shell has positive signed
volume. A writer that flattens mirrored occurrences without re-winding fails it.

### 3.3 Consumers and the 3MF default

- **PrusaSlicer 2.9.6** creates one `ModelInstance` per build item that
  references an object (`_create_object_instance`)
  [code: PrusaSlicer `src/libslic3r/Format/3mf.cpp:2399-2453`]; flips any
  imported mesh with negative signed volume (`:2651-2652`); swaps triangle
  indices when it merges a left-handed volume (`src/libslic3r/Model.cpp:826-829`);
  and `TriangleMesh::transform` flips triangles under det < 0 when asked to
  (`src/libslic3r/TriangleMesh.cpp:366-385`). It handles left-handed
  transforms by design [src/inf].
- **Cura 5.13.0** keeps the 3MF transform on the scene node
  (`plugins/3MFReader/ThreeMFReader.py:133-135`) and, when slicing, multiplies
  vertices by the world transform with no winding change
  (`plugins/CuraEngineBackend/StartSliceJob.py:485-506`). Whether CuraEngine's
  layer polygons depend on triangle winding was not established from
  `src/slicer.cpp` by this lane [code read; result open].
- The literature does not say how consumers in D-consumers behave on a
  negative-determinant item. Until H3 (§8) measures it, the default is the
  baked opposite-hand object: it is correct for any consumer, it costs one
  geometry copy per (key, colour variant) with mirrored uses rather than per
  occurrence, and it matches the RP's model of a mirrored part as its own
  part (§1.1).

## 4. Question 3: uniform scale

### 4.1 ε per occurrence [own]

For A = sR (R orthogonal, s > 0), distances scale by s, so a mesh within ε_def
of the definition's surfaces lies within s·ε_def of the placed surfaces. The
key uses ε_def = ε/σ_max with σ_max the actual largest singular value (equal
to s for an exact similarity). For an admitted near-similarity the deviation
of the placed mesh is at most σ_max·ε_def = ε, because any linear map
stretches distances by at most σ_max; correctness does not depend on τ.

- Angle-valued criteria (curve and surface angle, normal deviation caps,
  dihedral thresholds for G1 classification) are invariant under similarity
  and stay unchanged in the key.
- Lengths that drive density but are not ε (maximum edge length, η, β,
  proximity and contact limits) divide by σ_max like ε.
- Normals: A⁻ᵀn = R n / s, normalised to R n. glTF consumers normalise after
  their normal-matrix transform [inf: glTF §3.9.3 states normalisation after
  scaling for normal textures; normal-matrix use is renderer practice].
- Transform rounding: applying a double-precision A adds at most about
  3u‖A‖‖x‖ to a position (u = 2⁻⁵³), far below any ε in the corpus; the
  current exact storage-error term [code: `native/mesher/assembly.cpp:40-75`]
  belongs to the certified regime. Single-precision node matrices and
  instance attributes (glTF `EXT_mesh_gpu_instancing` stores TRANSLATION,
  ROTATION and SCALE as FLOAT [src: extension README, attribute table]) are an
  export-precision question for G3.

### 4.2 May nearby scales share one mesh?

- **Correctness: yes.** A mesh made at ε/s_max serves every occurrence with
  σ_max ≤ s_max, since its world deviation is σ_max·ε/s_max ≤ ε [own].
  Today's code applies exactly this "tightest" rule per definition
  [code: `native/mesher/assembly.cpp:161-171`], and it is what G17's formula
  would do across scales.
- **Cost: over-refinement.** With chord δ ≈ h²κ/8 on a curved face, triangle
  count per unit area grows as κ/δ [own; the standard chord estimate used in
  fast-tessellation §1], so the occurrence at scale s carries about s_max/s
  times the triangles it needs, and its emitted file grows with it.
- **What the decision says.** D-placements says "uniform scale gets a mesh per
  scale with ε scaled to match". Exact per-scale keys are the default. Two
  exact-equal scales give bit-identical ε_def and share a key; two scales one
  ulp apart get two keys, which is correct but wasteful. Bucketing scales onto
  a fixed geometric grid of ratio r (mesh at ε/s_grid, s_grid the bucket's upper
  edge) bounds over-refinement by r and removes ulp-level splits. Whether that
  is within "a mesh per scale" is an operator question (§10); it is offered as
  an arena flag with hypothesis H2.
- The literature does not say which is cheaper on real assemblies; no input we
  hold has a scaled placement (§1.2). H2's authored family settles it.

## 5. Question 4: per-occurrence attributes

### 5.1 Identity

- **Occurrence id** = the path of usage ids from the root (NAUO `#id`s, plus
  their UUIDs when present). The CAx-IF identifies a specific instance by "a
  top-down list of representation_relationships that unambiguously define the
  path from the root node to the target leaf node instance" [src: CAx-IF Model
  Styling v1.10 §5.1], and AP242's SpecifiedOccurrence exists to "individually
  identify the front axle of the second bogie of the fifth wagon"
  [src: AP242 XML Assembly RP v4.0 §7.2]. The persistent-ID RP lists
  `assembly_component_usage` and `product_definition_occurrence` among the
  entities that may carry UUIDs [src: CAx-IF Persistent IDs v1.8 §4.3 list].
  Our assembly code already stores the usage path per instance
  [code: `native/mesher/assembly.cpp:172-174`].
- **Face identity** is per definition (triangle → source face) and therefore
  shared by every occurrence; (occurrence id, face id) is the global identity.
- **Oracles exist in the source.** CAx-IF assembly validation properties carry
  the "number of children" per node and a "notional solids centroid position"
  per instance, to verify "that the number of instances found at each node is
  correct" and that "the position and orientation information for each
  instance is correct" [src: CAx-IF GVP v4.6 §7, §7.1, §7.2]. They are
  specification-tier oracles for occurrence accounting and placement in every
  writer, where the sender provides them.

### 5.2 Colour, material and visibility: precedence

- **CAx-IF.** "The styling information is always inherited from top to bottom",
  with priority assembly node, then component part, then solid, then shell,
  then faces; "If an assembly node is styled with a specific color, then all
  its constituents will be displayed with that color, regardless of how they
  were styled at part-level" [src: Model Styling v1.10 §4.2.1]. Face colours
  "should be styled by overriding the solid style" (`over_riding_styled_item`)
  [src: §4.2.1, §4.3.1]. A per-instance style is a
  `context_dependent_over_riding_styled_item` (CDORSI) whose styles "override
  any similar styles defined on the overridden styled_item"; a CDORSI may
  itself be overridden to restyle one face of one instance; invisibility is a
  CDORSI style too [src: §5.1, §5.2.1, §5.2.2, Figures 13-14].
- **OCCT.** `XCAFPrs_DocumentExplorer` starts from the parent's style, then the
  referenced definition's colour overrides it, then the instance's ("override
  Reference style with Instance style when defined (bad model?)")
  [code: `occt/src/DataExchange/TKXCAF/XCAFPrs/XCAFPrs_DocumentExplorer.cxx:27-81`];
  sub-shape (face) styles override the node's [code: `XCAFPrs.cxx:114-160`].
  OCCT is innermost-wins; the RP is outermost-wins for plain styles, explicit
  overrides winning in their context.
- **Ours.** Occurrence style = usage style, else the child product's, else the
  inherited parent's [code: `native/mesher/assembly.cpp:188-191`]; a face's
  own style beats the occurrence's [code: `native/mesher/adaptive_scene.cpp:460-461`].
  This follows OCCT, not the RP.
- **Resolution [own].** Follow the RP: it is the agreement CAx-IF-tested
  senders write to, and OCCT's own comment doubts its rule. Plain styles
  resolve outermost-first along the occurrence path; `over_riding_styled_item`
  and CDORSI chains apply in their stated context and beat what they override.
  Where a plain face style sits under a plain solid style with no override
  (the watch has 107 solid and 4 face styled items, no face overrides
  [data: grep]), the RP's list says the solid wins and OCCT says the face
  wins; that case is ambiguous, H4 (§8) counts it in the corpus, and until it
  is measured the face wins, recorded as a deviation from §4.2.1 **[call]**.
- **Material density** (STEP material property) is per definition in the RP
  and in our adapter [code: `native/step_source/main.cpp:296-303`]; no
  per-occurrence override exists in the sources read.

### 5.3 Where attributes live

| Attribute | Internal (output schema) | STL | 3MF | glTF |
|---|---|---|---|---|
| Occurrence id (path, UUIDs) | occurrence record | none (lost; sidecar only) | item `partnumber`; Production extension item/component/object UUID, which it REQUIRES on items and components [src: 3MF Production 1.2 ch. 4, §4.1.1, §4.2.1] | node `name` and `extras` |
| Placement | occurrence record (raw A, t, kind) | baked | item transform | node TRS |
| Colour / material | base style per definition face; per-occurrence override table keyed by (occurrence, face group) | none | object `pid/pindex` or per triangle; **items and component objects cannot carry properties** ("Producers MUST NOT assign pid or pindex attributes to objects that contain components") [src: 3MF Core §4], so each colour variant is its own object with copied geometry | per primitive `material` [src: glTF §5.24.3]; a colour variant is a separate mesh object reusing the same accessors |
| Visibility | occurrence record | emit or omit (§10) | emit or omit | node present; no core visibility flag |
| Face id | per definition triangle | none | triangle sets (Core 1.3+, §4.1.5), shared per object | `_FACEID`-style application attribute; application attributes "MUST NOT use unsigned int component type" [src: glTF §3.7.2.1], so ids above 2²⁴ need a non-float carrier (G13/G3) |

`KHR_materials_variants` maps materials per primitive per named variant, not
per node, so it cannot carry per-occurrence colour [src: extension README,
"Mappings"]. In USD, properties on an instance prim may be overridden but those
on its descendants may not [src: OpenUSD Scenegraph Instancing], so a
whole-occurrence colour rides on the instance prim, and a per-face override of
one occurrence needs its own prototype.

### 5.4 Per-occurrence colour is real input

The watch carries 9 CDORSIs, each assigning a surface colour to one component
instance through a two-entry `style_context` and overriding a null style on the
file's one `MAPPED_ITEM` [data: grep of entities `#11`-`#19`, `#1931`, `#20`].
The Trident TDP assembly carries 463 [data: fleet grep]. Our STEP adapter
loses all of them (defect 1, §9).

## 6. Question 5: how the NVIDIA baseline counts and emits instances

### 6.1 Our driver

- `brepgpu-reference-source` writes one `.brep` per definition and refuses any
  solid whose location is not the identity [code: `native/reference_source/main.cpp:176-179`].
- `render_om10_nvidia.py` groups definitions across 12 single-threaded workers
  largest-first and passes `--definition ID path` per definition
  [code: `examples/render_om10_nvidia.py:107-113, 210-246`]; each worker
  converts, imports and calls `SmApiTessellate` once per definition and writes
  a per-definition stage row [code: `native/om10_nvidia/main.cpp:134-178`].
- Placements are applied only afterwards by our code: `write_assembly` packs
  "genuine local corner bytes once; add only original placed instances", one
  record per occurrence with its matrix, and one palette material per
  definition [code: `python/brepgpu/nvidia_reference.py:63-95`]. NVIDIA never
  sees an occurrence.
- **Artifacts** [data: `results/om10-nvidia-complete-2026-10-17/heal{0,1}/producer-stages.tsv`]:
  107 rows of each per-definition stage; `tessellate` sums to 22.859 CPU-s
  (heal0) and 21.982 CPU-s (heal1). Weighting each definition's `tessellate`
  time by its occurrence count gives 26.803 and 25.910 CPU-s (×1.17).
  Triangles: 231,416 per definition, 281,156 per occurrence (×1.21); NVIDIA
  input faces after preparation: 9,172 per definition, 10,017 per occurrence
  [data: `definitions.tsv` × `bom.tsv`]. No existing NVIDIA run on the fleet
  host consumed a whole assembly: every NVIDIA input found there is a
  per-definition `.usdc`/`.brep` or the interchange test sheets [data: fleet
  `find`].
- The owned side is also per definition: `complete_component` times summed
  over the 107 definitions equal `complete_meshing` [src: journal 2026-10-17;
  owned-mesher-audit]. The recorded 847.7 s against 22.86 CPU-s is therefore a
  same-K comparison. It stays so only while every placement is rigid; with a
  scaled occurrence, today's driver would hand NVIDIA the definition at
  0.005 mm while the owned allowance divides by the norm. Rule 2 of §0.2 fixes
  that.

### 6.2 NVIDIA's own assembly paths do more work

- **OCCT `.brep` → USD flattens.** `OcctFileSource::AddComponentsFromRef` turns
  every located solid reached through compounds into its own entry with its
  accumulated location [code: `quarantine/sources/usd-brep/source/OCCT_BREP_IMPORT/src/OcctFileSource.cpp:194-262`],
  and the staging driver bakes that world transform into surfaces, curves and
  vertices ("World surface transform = surface-representation location, then
  the face's accumulated location") [code: `OcctStagingDriver.cpp:2234-2257, 3790-3791`].
  Its instance key is "storage index plus its accumulated world transform",
  so it merges only the same sub-shape reached twice at the same place, not
  repeated placements [code: `OcctStagingDriver.cpp:1973-1990`]. N occurrences
  become N B-reps and N tessellations, each in world units, so the chord
  applies in the world frame per occurrence [inf]. Reflected frames are
  handled by reversing the staged face sense [code: `OcctStagingDriver.cpp:101-155, 2244-2257`].
- **USD scene tessellation runs per instance proxy.** `SmApiUsdTessellateFile`
  collects BrepArray prims with `UsdTraverseInstanceProxies()`, tessellates
  each in its local frame and authors each mesh with its local-to-world matrix
  [code: `SM_API_USD/src/SmApiUsdTessellate.cpp:259-273, 286-293, 335`]. Every
  instance of one prototype is tessellated again [inf: instance proxies are
  visited once per instance]; the chord is in local units with no scale
  compensation; a mirrored transform is written as a plain matrix with no
  `orientation` change, relying on the USD rule [code: `:149-176`].
- **Hydra display** forwards `instancedBy` from the source prim to the generated
  meshes [code: `HYDRA_USD_BREP/src/HdUsdBrepArrayProcedural.cpp:362-385`], so
  interactive display may share one tessellation across instances [inf]. It is
  not a timed batch path.
- Converters flatten by default (Omniverse JT `instancingStyle 0`)
  [src: lifecycle map S10, L7].

Per-key work (rule 2) is therefore NVIDIA's fastest valid arm under the
steel-man rule: its native paths do the same tessellation once per occurrence.
Their per-occurrence output is also correct under scale (world-frame chord in
the flattened path), so they remain a valid, slower arm that may be reported.

## 7. Question 6: the CAx-IF AP242 assembly-structure practice

Acquired: *Recommended Practices for STEP AP242 Edition 4 Domain Model XML
Product & Assembly Structure*, Release 4.0, 20 October 2025, MBx-IF/JT-IF
(open download, 8.2 MB, `papers/pdfs/caxif-2025-rec-prac-ap242xml-assembly-structure-v4.0.pdf`).
Read: §7.1-7.3, §7.4, §7.7 and Table 47 (method+results). What it settles for
this lane:

- no mirroring and no scale in assembly transforms (§7.3, Table 47);
- mirrored parts are opposite-hand parts, either a separate CAD model or the
  original geometry plus a mirroring matrix (rotation, translation, inversion;
  no scale) on a part-to-part GeometricalRelationship; whole-assembly mirroring
  is out of scope (§7.7);
- multi-level occurrence identity through SpecifiedOccurrence (§7.2);
- a quantity can differ from the number of placed occurrences
  (NextAssemblyViewUsage, §7.4), so BOM quantity is never an occurrence count.

It is the XML (domain model) practice. Its Part 21 instance model for mirrored
parts is "T.B.S." (to be supplied) [src: §7.7.2.1], and the PDM Usage Guide 4.3
deferred mirror parts to "the next release" [src: §4.7.2]. No open Part 21
practice for mirrored parts was found (shopping list). For the mesher this
means: an opposite-hand part arrives as a product whose shape is the
original's shape plus a mirroring transform, and must become a mirrored
placement of the original's key, read from the raw transform (§1.3).

## 8. Hypotheses and the measurements that settle them

| # | Hypothesis | Measurement | Falsifier |
|---|---|---|---|
| H1 | Per-key meshing does the same work on both sides, so the ratio is unbiased | On the watch, owned and NVIDIA key sets both equal the 107 definitions; per-key rows match one to one | Any key meshed by one side and not the other |
| H2 | Scale bucketing with r = 2^(1/8) cuts meshing time on an authored family (one curved definition, 32 occurrences at scales spread over [0.5, 2]) by at least 2× against exact per-scale keys, while emitted triangles grow by at most r | Arena flag on the authored family, both settings, deviation oracle on every occurrence | Time saved < 2×, triangles grow > r, or any occurrence outside ε |
| H3 | PrusaSlicer 2.9.6 and Cura 5.13.0 produce the same sliced layers for a mirrored part written as a negative-determinant item and as the baked opposite-hand object | Slice both 3MF variants of an authored chiral part (e.g. a helical gear) on the fleet; compare layer polygons and reported volume | Any layer or volume difference: keep the baked default; if none, the spec path may become default for those consumers |
| H4 | Plain styles at two levels of one occurrence path disagree rarely in real files | Count in the corpus (CAx-IF, NIST, TDPs, watch) the faces whose solid and face carry different plain styles, and the paths where an assembly node and a part both carry plain styles | A non-trivial count: the precedence call in §5.2 needs the sender's own rendering (CAx-IF round screenshots, members only) |
| H5 | No real placement has κ in (1+1e-9, 1+1e-3), so τ = 1e-6 separates rigid/uniform from non-uniform without judgement | Read every raw composed placement in the corpus once the raw reader exists (G1) | Any placement in the band |
| H6 | Emission is small next to meshing for rigid assemblies but not for colour-variant-heavy 3MF | Time the emit stage separately for the watch and the Trident TDP to STL, 3MF and glTF | Emit stage > 10% of owned meshing on the watch |

## 9. Defects found by reading (not run)

1. **Per-occurrence styles are never read.** `native/step_source/main.cpp:279-284`
   loops over `STEPConstruct_Styles::NbStyles()`/`Style(i)`, which return only
   the non-root list; styled items whose item is a `SHAPE_REPRESENTATION`,
   which is how CAx-IF CDORSIs target a component instance (Model Styling v1.10
   §5.1), go to the root-style list and are never visited
   [code: `occt/src/DataExchange/TKDESTEP/STEPConstruct/STEPConstruct_Styles.cxx:529-547, 246-256`].
   The usage row writes `style_targets[NAUO id]` (`main.cpp:308`), which can
   never be set, because a styled item targets representation items or
   representations, never a NAUO. Result: every occurrence style is 0 and the
   watch's 9 instance colours (the Trident's 463) are dropped silently. Also,
   `style_targets[id(s->Item())]=d.id` (`:284`) keeps only the last styled
   item for an item, silently.
2. **Latent precedence mismatch.** `native/mesher/assembly.cpp:188-191` gives a
   child product's style precedence over an inherited parent style, and
   `native/mesher/adaptive_scene.cpp:460-461` gives a face style precedence
   over the occurrence style; CAx-IF Model Styling §4.2.1 and §5 give an
   assembly node's or an instance override's style precedence over part-level
   styles. Latent today because of defect 1 (no usage or assembly-node style
   ever arrives); it becomes live when defect 1 is fixed.
3. **Mirror-unsafe normal transform, latent.** `native/om10/math.hpp:177-178`
   transforms normals by the cofactor when the jet is flagged
   `COFACTOR_INSTANCE_NORMAL`; under det < 0 this yields inward normals. Latent
   while every admission path refuses det < 0 (`native/om10/validation.hpp:177`,
   `native/mesher/source.cpp:663`); it must change when mirrors are admitted
   (bead `brepgpu-12s.25.4`). Recorded so that the widening does not inherit it.

Reference finding, not ours: OCCT 8.0.1 `StlAPI_Writer` writes mirrored
occurrences inside-out (`StlAPI_Writer.cxx:95-121`). Already known and not
re-filed: OCCT reads a mirroring CTO as a rotation (lifecycle map finding 1;
G1), and the display's broader admission (B1 item 4).

## 10. Operator questions

1. **Scoring policy refusals.** Design §9.6 scores "owned refused, NVIDIA
   valid" as an owned loss. Non-uniform and sheared placements are refused by
   your decision (D-placements), and NVIDIA's flattening path would mesh them.
   Do such inputs count as owned losses, or are they excluded from the goal
   and only listed?
2. **"A mesh per scale."** Does it permit bucketing nearby scales onto a fixed
   grid and meshing each bucket at its tightest ε (correct for every member,
   fewer meshes, at most r× more triangles per occurrence), or must every
   distinct scale get its own mesh? Default until answered: every distinct
   scale value, bucketing as an arena flag (H2).
3. **Hidden occurrences.** CAx-IF instance styling can mark one occurrence
   invisible. For manufacturing exports (STL, 3MF), should an invisible
   occurrence be written, omitted, or written with a report? Default until
   answered: written, and the invisibility reported.

## 11. For SHOPPING_LIST.md

- A Part 21 (not XML) recommended practice for mirrored parts and their
  mirroring matrix. The AP242 XML RP's Part 21 instance model is "T.B.S."
  (§7.7.2.1) and the PDM Usage Guide 4.3 deferred it (§4.7.2); no open
  successor found.
- The 3D Systems StereoLithography Interface Specification (1988/1989)
  primary text; read here only through Burns' reproduction (lifecycle map).
- CAx-IF test-round results and screenshots for styled assemblies (members
  only; already on the list as "CAESAR round results"); needed only if H4
  finds conflicts.
- ISO 10303-42 normative text for `cartesian_transformation_operator` and
  `build_axes` (already on the list under ISO 10303-42).

## 12. Acquisition and scrub notes

- Saved one open PDF: the AP242 XML assembly-structure RP (§7). The glTF 2.0
  HTML, 3MF Core and Production markdown, glTF extension READMEs, OpenUSD
  schema and docs, and the slicer source files were read from pinned commits
  or tags and are recorded with SHA-256 in the sources TSV; they are not
  stored in the repository.
- The fleet host was used only for `ls`, `find` and `grep` over the corpus and
  existing artifact directories; no lane directory was needed or created.
- Scrub: no hostnames, user names or absolute workstation paths in this paper
  or its TSV; fleet paths are given relative to the BRepGPU root.
