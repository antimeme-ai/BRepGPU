# Source identity passthrough and the classification map

Research lane M4-G16 of the fast-mesher campaign, 2026-10-17. Briefs:
[M4-common](../docs/fast-mesher-campaign/briefs/M4-common.md) and
[M4-G16](../docs/fast-mesher-campaign/briefs/M4-G16.md). Bead `brepgpu-12s.17`.
Stub: [lifecycle map](2026-10-17-geometry-lifecycle-map.md) §4.1 G16 (stages S1,
S23; also S12, S13, S17). Sources, read depth and hashes are in
[2026-10-17-source-identity-passthrough-sources.tsv](2026-10-17-source-identity-passthrough-sources.tsv).

Nothing was built, run, meshed or measured. The lane read the doctrine, the
journal's operator decisions, the plan, the lifecycle map, the design draft, the
owned-mesher audit, the 2026-10-16 runtime study, the code, the held CAx-IF
recommended practices, Shephard–Beall–O'Bara 1998 and Parasolid's Functional
Description, and it fetched the open format texts at pinned revisions (AP242
Edition 4 MIM long form, 3MF Core 1.4.0 and Production 1.2, glTF 2.0 and three
glTF extensions, the OpenUSD `usdGeom` schema, the STL format text). One
reading-only count was taken from the private OM10 STEP file with `grep`
(entity-name counts only; nothing of its content is reproduced).

Tags: **[src]** a source states it; **[code]** read in this repository or a
pinned reference at file:line; **[own]** this lane's derivation; **[inf]**
inferred from a source but not stated by it.

Operator decisions that bind: **D-formats** (STL; 3MF with separate items; glTF
with face ids and normals; USD and AP242 tessellated later), **D-features**
(never drop features above ε), **D-determinism** (identical bytes across thread
counts on one backend; functionally identical across backends), and the
**internal-data rule** (JSON only at external boundaries; owned binary inside).

## 0. Answer

1. **Source identity is a small, fixed set of fields, and today's reader
   already keys every topological entity by its Part 21 instance number.**
   `native/step_source/main.cpp:266` maps every entity to `model->IdentLabel(e)`
   (the `#N` of the file) and refuses an entity without one; faces, loops,
   coedges, edges, vertices, carriers, curves, products and usages are all
   written under that key [code]. What is missing is everything a CAD system
   adds for persistence and meaning: UUIDs (`v5_uuid_attribute`,
   `v4_uuid_attribute`), their split/merge relationships, `shape_aspect`
   groupings with their `geometric_item_specific_usage` links, the product
   revision string, and the face and edge name strings [code: no reader for any
   of these]. `#N` is file-local and changes on every re-export
   [src PID RP v1.8 §4.1: UUIDs are external identifiers, never internal file
   references; inf for re-export]. §2.7 is the field list.
2. **The classification map already exists in the code as the physical node
   key.** `PhysicalNodeKey{definition, generator, source, generation, ordinal}`
   with `NodeGenerator ∈ {original_vertex, original_edge, face_site,
   declared_quotient}` (`native/mesher/mesh_values.hpp:21-34`) is Shephard's
   classification (mesh vertex on a model vertex, edge or face; Shephard et al.
   1998 §3) plus a fourth class for surface singularities. Keep it. What the
   design lacks is collapse history and two derived requirements (§3.2, §3.3).
3. **Collapse history: classify once, against the virtual entity, and keep a
   sparse tree of collapse records on the survivor.** Shephard et al. 1998 §5
   reject multiple classification for its cost in every algorithm and store the
   history "on the surviving entity … the history of the operations form a tree"
   [src]. Parasolid does the same for zipped spikes: points go to the "dominant"
   edge [src Parasolid FD §109.6.4]. Today's `SourceVertexAlias` is the
   one-level case: the minimum vertex id survives and no endpoint is erased
   (`native/mesher/boundary.hpp:89-94`) [code].
4. **Per-format carriage (§4):**
   - *STL* carries nothing. The format's attribute bytes "should be set to
     zero" [src STL text, Burns §6.5 after 3D Systems 1989]. Identity survives
     only through the internal schema, which records each face's triangle range
     in the written STL.
   - *3MF* carries face identity per definition mesh as core triangle sets, one
     `refrange` per face, plus extra sets for UUID groups and `shape_aspect`
     groups [src 3MF Core 1.4.0 §4.1.5]. Per-vertex classification cannot ride
     in 3MF.
   - *glTF* carries a per-vertex `_FACEID` FLOAT attribute (exact to 2²⁴) and a
     per-definition face table in `extras` [src glTF 2.0 "Vertex Attributes",
     `extras.schema.json`]. B-rep edges ride on `EXT_mesh_primitive_edge_visibility`
     (Draft), optionally, as hard-edge bits.
   - *USD (later)* carries `GeomSubset` families for faces (partition), edges
     and points [src `usdGeom/schema.usda`].
   - *AP242 tessellated (later)* carries the complete map natively:
     `tessellated_vertex.topological_link`, `tessellated_edge.geometric_link`,
     `tessellated_face.geometric_link` [src AP242 Ed.4 MIM].
5. **Identity costs almost nothing if it is carried as ranges, not lanes.**
   Today's display `Triangle` spends 8 of its 24 bytes on the face id
   (`native/om10/types.hpp:27-30`) [code]. A face-contiguous triangle order with
   one offset per face cuts that to 36.7 KB against 2.19 MB for the 273,978-triangle
   watch [own arithmetic]. Every per-element hot loop then carries no identity at
   all; identity is a batch descriptor. The literature gives no measured cost of
   identity carriage in a mesher; §5.3 states the arena hypothesis and its
   falsifier.
6. **The steel-manned baseline carries less than this.** NVIDIA's USD
   tessellation emits `GeomSubset` indices only paired with material paths and
   labels faces internally with `SmFace` pointers
   (`include/BREP_SM_USD/SmuTessellate.h:39-52, 74-81` in the held 1.0.0
   package) [code]. Its UsdSolid proposal defers "Brep↔Mesh correlation"
   (`proposals/UsdSolid/README.md:467` at `c9c5979f`) [src]. OCCT's glTF writer
   carries face identity only by writing one primitive per face, its default
   (`RWGltf_CafWriter.hxx:130-135`, `myToMergeFaces(false)` at `.cxx:252`)
   [code].

## 1. What binds, and what exists

- **Design draft.** §5.5 promises "per-vertex/per-facet source maps (source edge
  node or surface root, generator, owner, revision, edge provenance)", per-edge
  records and canonical facet order. §5.2 item 5 collapses small features
  "virtually and bottom-up, in the mesh only". Nothing in the draft reads
  UUIDs, groupings or names, or says how identity reaches each format
  (lifecycle map S1 "absent", S23 "partial").
- **Code (the old mesher is retired, but its identity types are the audit's
  keep list, `papers/2026-10-17-owned-mesher-audit.md` §4).**
  - Reader tables `FACES___`, `EDGES___`, `VERTICES`, `LOOPS___`, `COEDGES_`,
    `CARRIERS`, `CURVES__`, `SOLIDS__`, `PRODUCTS`, `USAGES__`, `STYLES__`,
    `META____` are keyed by `#N` (`native/step_source/main.cpp:263-423`). A face
    row is (`#face`, `#solid`, `#surface`, face style, sense, tolerance, loops)
    (`:418`). Product rows keep `#product_definition`, solid, style, name and
    `product.id` (`:305`); usage rows keep `#NAUO`, relating, related, style,
    name and transform (`:308`) [code].
  - Mesh side: `PhysicalNodeKey` and `NodeGenerator` (above); `FacetKey{generator,
    owner, revision, ordinal}` and `ConstructionSite.source` as a variant of
    boundary node or original surface root (`native/mesher/adaptive_trim_types.hpp:95-116`);
    scene `AdaptiveSceneVertexSource{component, face, site, original root,
    original_uv}` and `AdaptiveSceneFacetSource{component, face, facet}`
    (`native/mesher/adaptive_scene.hpp:16-26`); display `Triangle{vertex[3],
    material, face}` (`native/om10/types.hpp:27-30`) [code].
- **The pinned OCCT 8.0.1 bridge.** It has entity classes for `shape_aspect`,
  `composite_group_shape_aspect`, `geometric_item_specific_usage` and
  `id_attribute` (`StepRepr_ShapeAspect.hxx`, `StepRepr_CompositeGroupShapeAspect.hxx`,
  `StepAP242_GeometricItemSpecificUsage.hxx`, `StepAP242_IdAttribute.hxx`) but
  none for any `uuid_*` entity (no match for `uuid_attribute` anywhere under
  `src/`) [code]. An unrecognised record becomes `StepData_UndefinedEntity` with
  its type string and parameters, and entity references inside sub-lists stay
  resolved (`StepData_StepReaderTool.cxx:232-243`;
  `StepData_UndefinedEntity.cxx:69-147`) [code]. A UUID record is therefore
  readable from the raw model, as the reader already does for faces [inf; the
  typed parameter `UUID_SET_ITEM((…))` needs a probe on a real PID file, §7].
  The lexer has no Part 21 Edition 3 `ANCHOR` section token
  (`StepFile/step.lex:135-150`) [code]. A file with one probably fails to parse,
  which costs nothing here, because the PID RP forbids using anchor-section
  UUIDs [inf; probe in §7].

## 2. Question 1: the identity fields and their sources

### 2.1 Part 21 instance numbers

Every reader table uses `#N`, and the reader throws if an entity has none
(`main.cpp:266`) [code]. `#N` identifies an entity inside one file. The PID RP
separates UUIDs, "only permitted to be provided for use as external
identifiers", from "internal file reference", which UUIDs may not replace
[src PID RP v1.8 §4.1]. Nothing in any read source makes `#N` stable across
exports [inf]. So `#N` together with the source digest is exact
provenance for one input, and it is not identity across edits. G12 has to know
this: on files without UUIDs (the OM10 watch has none, §2.2) the mesher cannot
claim a persistent face id. It can offer a content key, labelled as content,
following "cache keys contain zero identity fragments; identity contains zero
geometry" (brepjs ADR-0014, cited in the lifecycle map S17) [src via map].

### 2.2 Persistent IDs: CAx-IF PID RP v1.8 (13 March 2026)

What the RP establishes [src, section given]:

- **Entities.** Preprocessors "shall" assign UUIDs to each product and product
  version, each semantic PMI entity, and "faces, edges, as well as supplemental
  geometry entities that are used as reference for the above Semantic PMI
  entities". "Topological vertices shall not be assigned UUIDs" (§4.3.3).
  `vertex_point` was removed as a UUID target in v1.30 (history table). The
  bold (tested) subset of `id_attribute_select` is product structure,
  `advanced_brep_shape_representation`, `shape_representation`, `shape_aspect`,
  `dimensional_location`, `advanced_face`, `closed_shell`, `open_shell`,
  `edge_curve` (§4.3.2). For datums, the UUID goes on `datum_feature`, not on
  `datum` (footnote 2).
- **Encoding.** `uuid_attribute` (abstract) with subtypes `v5_uuid_attribute`
  and `v4_uuid_attribute`. `identifier : STRING(36)` is UNIQUE.
  `identified_item` is a `uuid_set_item` (an unordered SET) or a
  `uuid_list_item` (an ordered LIST of LISTs) (§4.3.2, Figure 1). The same
  definitions are in the AP242 Ed.4 MIM (`mim_lf.exp` lines 37188-37248)
  [src]. Both subtypes' WR1 tests `identifier[14]`. That is the hyphen of a
  36-character UUID, not the version digit, and the RP says this "shall be
  ignored during instance data validation" (§4.3.2.2, §4.3.2.4) [src]. The RP's
  own NX example UUID `D6DEF23B-76ED-A5DE-…` does not have a 5 in the version
  position (§4.3.5.1) [src]. **Consequence [own]:** parse the UUID text to 128
  bits without testing version or variant, compare case-insensitively, and keep
  the declared subtype (v4 or v5) as a separate field.
- **One UUID, several faces.** A split hole carries one UUID on the set of its
  two half-cylinders (`UUID_SET_ITEM((#124,#125))`, §4.3.2.6.2, Figures 2–3).
  Since R57J, NX, CATIA and Creo all emit three UUIDs per logical hole (one face
  set and two edge sets), and Creo concatenates two internal ids into one
  namestring (§4.3.5.1). Disjoint planar faces with one Creo id share one UUID
  set (§4.3.5.3). A face therefore has **zero or more UUID memberships**, and
  one UUID has **one or more member entities**.
- **Relationships.** `uuid_relationship(identifier, uuid_1, uuid_2, role)` with
  roles SUPERSEDES, MERGE, SPLIT, DERIVE_FROM, SAME_AS, SIMILAR_TO; and
  `uuid_provenance` (an ordered list of relationships) (§4.3.2.6-7). The MIM
  adds `uuid_context(identifier, frame_of_reference, target)` and an acyclicity
  rule on relationships (MIM lines 37205-37238) [src].
- **Ownership.** A postprocessor decides whether a received UUID is its own by
  recomputing it. If it is not its own, the UUID and its object are references
  and "the receiving system shall not modify it". Postprocessors "shall retain
  incoming UUIDs for all identified geometry and topological entities … and
  product and product version entities read" (§4.3.3).
- **Where.** Data-section UUIDs only. Anchor-section UUIDs "shall not be
  used". `id_attribute`/`aggregate_id_attribute` carrying UUIDs "shall" be
  ignored (§4.1, §4.3.3).
- **Version string.** `product_definition_formation.id` holds the revision
  (`-`, `A`, `A.1`, `A-1` …), defaulting to `-` (§4.3.1).
- **Formation (informative).** v5 from a publisher namespace UUID and a
  namestring built from part number, serial, entity type or path, and the
  internal persistent id (§4.2). Namespace and namestring are not exchanged
  (§4.3.2.2).
- **Features (R58J and later).** A `composite_group_shape_aspect` "Logical
  Hole" groups child `shape_aspect`s by `shape_aspect_relationship`. Each child
  references faces through GISU. UUIDs live on the faces and on the group, and
  "the PMI and PID structures are parallel — connected only through shared
  geometry references" (§4.3.5.4, Appendix B).

Two things follow for BRepGPU [own]:

- The mesher is a postprocessor of source entities and a preprocessor of mesh
  entities. It retains source UUIDs exactly and never mints a UUID for a source
  entity. Any UUID it mints (3MF Production items, §4.2) names its own output
  objects, is v5 from a fixed BRepGPU namespace and a namestring of source
  digest plus entity path, and is never v4. A random UUID would break
  D-determinism.
- The OM10 watch, an Inventor 2022 AP214 export, has 0 `UUID_ATTRIBUTE`, 0
  `ID_ATTRIBUTE`, 0 `SHAPE_ASPECT` and 0 `GEOMETRIC_ITEM_SPECIFIC_USAGE`
  records, all 9,162 `ADVANCED_FACE` names empty, and 121 `STYLED_ITEM`
  records [grep count, private file]. The watch exercises none of this path,
  and the code must not lean on the watch to decide it (anti-overfitting rule).

### 2.3 `shape_aspect` groupings: "one hole across two faces"

[src PMI RP v4.1 §5.1, §6.1; PID RP App. B]:

- A toleranced feature is a `shape_aspect`. One face links through
  `geometric_item_specific_usage(…, definition=shape_aspect,
  used_representation, identified_item=advanced_face)`. Several faces link
  through `item_identified_representation_usage`. "A cylindrical hole
  represented by 2 faces" is the RP's own example.
- The `(shape_aspect, GISU, advanced_face)` combination is unique and must be
  reused by every PMI that references the same face (§5.1).
- The RP prefers that the sender merge the two half-faces. "Otherwise
  postprocessors might combine neighboring advanced_face entities". The mesher
  must not merge them: merging is a geometry change (lifecycle map S1
  requirement), and the grouping already says which faces form one feature
  [own].
- Groups nest: `composite_group_shape_aspect` → `shape_aspect_relationship` →
  child `shape_aspect` → GISU → faces (PID RP App. B.2 Part 21 fragment).

Fields: `#N`, name, description, `product_definitional`, member faces and edges
(through GISU and IIRU), parent group, UUID. PMI semantics (what the group is
toleranced for) belong to G15; G16 only carries the grouping. The NIST MBE PMI
corpus files carry semantic PMI and are the in-hand test family for this path
(corpus/MANIFEST.md row 3a) [inf: their GISU structure is not yet read].

### 2.4 Cross-domain RP v1.0 (CAx-IF/DMSC, 29 January 2026)

[src §5.3-5.6, Table 1]:

- STEP→QIF mapping uses exactly the IDs above. The `product_definition_shape`
  UUID becomes the QIF Part `EntityId`. `advanced_face` UUIDs become
  `<SurfaceFeatureNominal>` external ids, `edge_curve` UUIDs become line or arc
  feature nominals, and `datum_feature` (not `datum`), `dimensional_size`,
  `dimensional_location` and `geometric_tolerance` become characteristics.
- QIF postprocessors "shall follow the topology links from ADVANCED_FACE or
  EDGE_CURVE to underlying geometry", meaning `face_surface.face_geometry`
  (§5.6, footnote 17).
- Shells and `advanced_brep_shape_representation` are "[ignore]" for QIF. The
  `manifold_solid_brep` maps to `<Body form=SOLID>`.

For us [own]: a triangle reaches a metrology system as triangle → face → face
UUID → QIF `EntityExternalId`, and nothing else is needed. The face's surface
(kind and canonical parameters) is the "underlying geometry" the QIF side
follows.

### 2.5 The geometric link per face: 3D tessellated RP v1.1 §5.4 and the AP242 Ed.4 MIM

[src RP §5.4-5.4.5, §6; MIM lines 35379-35725, 36621-36628]:

- Structured tessellation keeps "one tessellated entity per exact geometric
  entity". `tessellated_face.geometric_link : OPTIONAL face_or_surface`, where
  `face_or_surface = SELECT(face_surface, surface)`, and `advanced_face` is a
  `face_surface`. "Provided this information is used only on canonical
  surfaces and curves, its effect on the compactness … is minimal. It is
  recommended … to ensure exact measurement and positioning on the tessellated
  entities" (§5.4).
- `tessellated_edge.geometric_link : OPTIONAL edge_or_curve` and `line_strip`.
  `tessellated_connecting_edge` adds `smooth : LOGICAL`, `face1`, `face2` and
  equal-length `line_strip_face1/2`.
- `tessellated_vertex.topological_link : OPTIONAL vertex_point` and
  `point_index`. `tessellated_shell.topological_link : OPTIONAL
  connected_face_set`. `tessellated_solid.geometric_link : OPTIONAL
  manifold_solid_brep`.
- Watertightness practice: one `coordinates_list` per solid and a connecting
  edge for every edge (§6).

This is Shephard's classification written into the standard: a point index is
on a model vertex if a `tessellated_vertex` names it, on a model edge if it is
in that edge's `line_strip`, and on a face otherwise [own]. The "geometric link
per face" field is therefore the face's surface reference plus, for canonical
surfaces, its parameters (kind, frame, radii, semi-angle) in FP64, the values
`CARRIERS` already holds (`main.cpp:372`) [code].

### 2.6 Shephard et al. 1998: classification

[src §3]: "The unique association of a topological mesh entity of dimension dᵢ
… to a topological geometric model entity of dimension dⱼ where dᵢ ≤ dⱼ, is
termed classification … Mesh entities are always classified with respect to
the lowest order object entity possible." The mesh has three restrictions that
make classification well defined: regions and faces have no interior holes;
each entity uses a lower-order entity at most once; and an interior entity is
uniquely specified by its bounding entities. Entities on the model boundary may
have non-unique bounding sets (Figure 1: two mesh edges on a hole boundary with
the same two vertices) [src].

### 2.7 The field list

"Today" is the current reader or mesher; "M5" is what the design must add.

| Entity | Field | Source authority | Today |
|---|---|---|---|
| Input | SHA-256 of original bytes; `FILE_NAME`, `FILE_SCHEMA`; `FILE_DESCRIPTION` RP id strings (e.g. `CAx-IF Rec.Pracs.---Persistent IDs---1.8---2026-03-13`) | PID RP §3; lifecycle S17 | digest and META yes; RP strings no |
| Product (definition) | `#product_definition`; `product.id` (part number); name; `product_definition_formation.id` (revision, default `-`); UUID; nominal-model flag (`product_definition_shape.name`) | PID §4.3.1; PMI RP §4 (via map S1) | `#N`, id, name yes; revision, UUID, flag no |
| Occurrence | `#NAUO` path from root; NAUO name/id; transform; UUID if present (`assembly_component_usage` is a UUID target) | PID §4.3.2 list | `#N`, name, transform yes; path implicit; UUID no |
| Body / shell | `#manifold_solid_brep` (and void shells, B1 defect); `#closed_shell`/`#open_shell` UUID | PID §4.3.2 bold list | solid `#N` yes; shell UUID no |
| Face | `#advanced_face`; name string; UUID memberships (set id → members); `#surface`, kind, sense; style; geometric link (canonical parameters) | PID §4.3.3, §4.3.5; 3D tess RP §5.4 | all but name and UUID |
| Edge | `#edge_curve`; name; UUID memberships; `#curve`, kind; per-edge resolution record (design §5.5) | PID §4.3.3 | `#N` yes; name, UUID no |
| Coedge, loop | `#oriented_edge`, `#edge_loop` (no UUID target) | PID §4.3.2 | yes |
| Vertex | `#vertex_point` only (no UUIDs by rule) | PID §4.3.3, v1.30 | yes |
| Grouping | `#shape_aspect` (and `composite_group_shape_aspect`, `datum_feature`); name; description; members via GISU/IIRU; parent via SAR; UUID | PMI RP §5.1, §6.1; PID App. B | no |
| UUID records | `uuid_relationship`, `uuid_provenance`, `uuid_context` kept verbatim, uninterpreted | PID §4.3.2.6-8 | no |
| Mesh triangle | owning face (definition-local ordinal → face table); `FacetKey` | design §5.5; code | yes, as explicit per-triangle lanes |
| Mesh vertex | classification = `PhysicalNodeKey` (dimension, entity, ordinal); edge `t` or per-face-occurrence (u,v) in FP64; resolution class and displacement through the edge record | Shephard §3; design §4 item 3 | yes |
| Mesh edge | on model edge E or interior (derived, §3.2); continuity class (G1 or crease) | Shephard §3; design §5.2 item 6 | partial |
| Collapse | record {survivor, absorbed original, op, parent record} | Shephard §5.1; Parasolid §109.6.4 | one-level alias only |
| Face outcome | status plus triangle range; `absorbed` with collapse record (§3.3) | design §4 item 5 | no `absorbed` |

## 3. Question 2: per-vertex classification and collapse history

### 3.1 The classification of a mesh vertex

Mature practice [src]:

- Shephard: lowest-dimension model entity (above).
- Parasolid tabular faceting emits `facet_face` (facets of one face grouped
  together), `fin_edge` (only fins on model edges), `point_topol` and
  `fin_topol`. `point_topol` returns, for each face-boundary facet point, the
  vertex if it is one; "otherwise, the model edge … if that edge is accurate";
  and "if the model edge is tolerant, then one of the edge's fins", the fin
  "whose SP-curve was used in the discretisation" (FD §109.6.1-109.6.5,
  pp. 1861-1868).
- At a singularity, `fin_topol` returns a vertex if a model vertex is present,
  and nothing otherwise (§109.6.5).

Our rule, adapting both [own]:

- **On a model vertex** (`original_vertex`): `#vertex_point`. The stored
  position is the resolved vertex (design §5.2 item 4). The displacement from
  the source point is a recorded fact, never a change of classification.
- **On a model edge** (`original_edge`): `#edge_curve`, the sample's dyadic
  edge-path key (design §5.2; `DefinitionBoundaryForest` keep list), its FP64
  `t`, and one (u,v) per incident face occurrence; seams carry two (design §4
  item 3). Resolution class R1–R6 comes from the edge record. For R4 (owner
  face's surface) the record names the owner face. That is Parasolid's
  "tolerant edge → the fin whose curve was used" [src analogue].
- **In a face** (`face_site`): the face, its (u,v) in FP64, and the generator
  (packet, band, mapped cap) through `FacetKey`
  (`adaptive_trim_types.hpp:93-99`) [code].
- **At a quotient** (`declared_quotient`; poles, apexes): classify on the
  model vertex when the source has one at the singularity, else on the face
  with the UV locus kept. This follows Parasolid's `sing_topol` rule [src] and
  the runtime study's "the source/cell/physical relation must express that
  quotient directly" (`papers/2026-10-16-whole-mesher-native-runtime.md`, "Full
  mesh versus display binary") [src].

### 3.2 Mesh edges, and a topological floor the draft does not state

The classification of a mesh edge follows from its vertices only under
Shephard's restrictions [src §3]. A mesh edge lies on model edge E if and only
if it joins two consecutive samples of E's chain; otherwise it is classified in
the face [own]. The map stays well defined, and 3MF's by-index manifold rule
("every triangle edge … shares common vertex endpoints with the edge of
exactly 1 other triangle", 3MF Core §4.1 [src]) can only hold, if no two
distinct mesh edges share the same pair of vertex ids [own]. Two cases break
this:

- **A closed edge** (start vertex = end vertex) sampled with one or two
  segments. Two segments V→s→V give two mesh edges with vertex pair {V, s}.
  With the design's circle rule (closed-form sagitta, §5.2 item 1), a circle of
  radius R ≤ ε has sagitta R ≤ ε at n = 2, so the rule allows two segments
  [own arithmetic].
- **Two model edges with the same end vertices** (a lens: an arc and a line, or
  the two semicircles of a split hole between the same two vertices), each
  sampled with no interior point. Both become the single pair {V₁, V₂}. "Lines
  need no interior samples" (§5.2 item 1) and an arc whose sagitta is below ε
  also gets none [own].

**Requirement for M5 [own]:** a topological floor on sample counts. A closed
edge needs at least three segments. Among edges sharing both end vertices, at
most one may have a single segment. Both cases occur only for features near or
below ε, so the floor interacts with G10's collapse rules, and either the floor
applies or the feature collapses with a record (§3.3), never neither.
Falsifier: the classification oracle (§3.4, check 3) fails on an authored
small-circle or lens family at ε ≥ R.

### 3.3 Collapse history

[src Shephard et al. 1998 §5, §5.1]:

- Small-feature removal yields mesh entities that span several model entities.
  "Multiple classification" (each mesh entity classified on everything it
  spans) works but complicates "the data structures and data operators" and
  forces support "in all the mesh generation and modification procedures".
- The alternative classifies the mesh against a modified (virtual) topology and
  keeps the original topology beside it.
- Five reduction operators: collapse edge to a vertex; collapse a degenerate
  face to an edge; collapse a single-edge face to a vertex; and two region
  operators.
- "The history information about each operation is stored on the surviving
  entity … If this surviving entity is eliminated in a subsequent operation,
  the surviving entity from the subsequent operation will refer to it … the
  history of the operations form a tree." A vertex is the minimum; deleting it
  would sever the link to the original.

Parasolid's zipped spikes take the same choice: points and fins are classified
on the "dominant" edge (FD §109.6.4-5) [src].

Our rule [own]:

- **Single classification against the virtual entity.** Hot loops see one key
  per vertex.
- **One collapse record per absorbed original entity:** {survivor key,
  absorbed `#N` with its dimension, op ∈ {degenerate-edge→vertex (today's
  alias), small-edge→vertex (§5.2 item 5), degenerate-face→edge,
  small-face→vertex}, parent record}. The records form Shephard's tree.
  Consumers flatten it to a "stands for" set when they need one.
- **Survivor choice is deterministic and source-keyed:** the lowest-dimension
  original entity with the minimum source id. This is today's
  `SourceVertexAlias` rule extended (`boundary.hpp:89-94`) [code], and it
  respects D-determinism.
- **Accounting.** A face whose triangles all collapse produces zero triangles.
  Design §4 item 5 allows only "triangles or an explicit refusal". M5 needs a
  third outcome, `absorbed`, with an empty triangle range and its collapse
  record, so the face is still accounted for and D-features ("never drop
  features above ε") stays checkable, because absorption is legal only below
  the threshold G10 sets [own].
- **Faces meshed from boundary vertices alone** (§5.2 item 5) are not
  collapses. They keep their triangles and classification, and they record that
  the face has no interior vertices.
- **Validity is G10's.** Whether a collapse keeps triangles within ε,
  preserves dimension and disperses symmetrically is G10's question; G16 fixes
  only the record.

### 3.4 Oracle for the map

A specification-tier oracle that changes exit status, run offline against the
original source and the written internal schema, with no producer status
trusted [own; tier from BLACKBIRD]. Checks:

1. Every vertex classified on a model vertex is within the request of that
   vertex's resolved position.
2. Every vertex classified on E lies on E's resolved curve at its recorded `t`
   within the request, and its per-face (u,v) maps to the same point on each
   incident face.
3. Every pair of consecutive chain samples on E is a mesh edge used by exactly
   the faces that use E, and no other mesh edge joins two samples of one chain.
   This also checks the §3.2 floor.
4. Every triangle in face F's range has its vertices classified on F or on F's
   boundary entities, or on a collapse survivor whose record names one of
   them.
5. Every source face has a range or an `absorbed` or `refused` record.
6. Every source UUID read has at least one member that reaches output.
7. Format readback: the per-format carriage (§4) decodes back to the same
   face-to-triangle map.

Negative controls: swap two face ranges, shift one edge sample's `t` by 10ε,
drop a collapse record. Each must fail.

## 4. Question 3: carriage per format

The internal schema (the 2026-10-16 runtime study's sectioned full-mesh binary)
carries every field of §2.7 in owned binary. Exports carry what each format can
hold. Identity never changes geometry, so a consumer that ignores it loses
nothing it would otherwise have [own].

### 4.1 STL (D-formats: alpha)

- **Format [src STL text].** A facet is a normal and three vertices. There is
  no unit and no shared vertex, and "the attribute syntax is not documented in
  the formal specification. It is specified that the attribute byte count
  should be set to zero".
- **Reader [code].** OCCT's binary reader reads the 50-byte facet's normal and
  vertices and skips the attribute (`RWStl_Reader.cxx:38, 440-467`). It accepts
  several concatenated solids as separate solids (`:152-176`).
- **Carriage [own]:**
  - Write attribute bytes as zero.
  - Do not split a part into per-face `solid` blocks. That turns one closed
    body into many open solids in readers such as OCCT [code: `AddSolid` per
    block].
  - Write triangles face-grouped in canonical order, and record in the internal
    schema each face's triangle range in that exact STL.
  - The 80-byte header may carry a short BRepGPU run tag so a file can be
    matched to its internal schema. The header must not begin with `solid`,
    which readers use to detect ASCII (`RWStl_Reader.cxx:215`) [code].
  - Identity in STL is "by order in this file", the JT failure mode the
    lifecycle map names (S17) [src via map]. It is valid only for the bytes as
    written.
- **The literature does not say** whether PrusaSlicer or Cura keep facet order
  on import. That matters only if a consumer writes STL back. Measurement:
  G3's re-import probe can record facet order beside welding.

### 4.2 3MF (D-formats: alpha, separate items)

- **Format [src 3MF Core 1.4.0 §4.1, §4.1.5, §3.4.1, §3.4.3.1].**
  - Meshes for type "model" must be manifold by index, so per-vertex identity
    cannot ride on duplicated vertices.
  - Triangle sets (since Core 1.3.0) live in namespace
    `http://schemas.microsoft.com/3dmanufacturing/trianglesets/2021/07`. Each
    has a required non-empty `name`, and an `identifier` of type `xs:QName`
    that is unique in the mesh, with `<t:ref index>` and
    `<t:refrange startindex endindex>` (inclusive).
  - A triangle may be in several sets. "A producer SHOULD try to output
    disjoint sets"; a consumer making a disjoint segmentation gives "priority
    … to the last set in which a triangle occurs"; "a consumer MAY ignore the
    information".
  - Objects have `name` and `partnumber` ("SHOULD maintain"); items have
    `partnumber`. Both can hold a `metadatagroup` whose vendor names must be
    namespace-prefixed.
- **Production extension 1.2 [src §4].** UUIDs are required on `<build>`,
  `<item>`, `<object>` and `<component>` when the extension is used (RFC 4122
  variants).
- **Carriage [own]:**
  - One object per definition mesh. Every item of that definition shares its
    triangle sets, and this fits D-placements: rigid and mirrored placements
    share one mesh.
  - Face sets: one per source face with one `refrange` (O(faces) bytes), since
    the triangles are face-contiguous. `identifier="f<#N>"` (an NCName needs a
    letter first), and `name` is human-readable (face `#N`, surface kind, UUID
    text if any).
  - Group sets, overlapping the face sets: one per UUID set (`identifier="u" +
    UUID text`, because a UUID may begin with a digit) and one per
    `shape_aspect` (`identifier="a<#N>"`). Write group sets **before** face
    sets, so a consumer that forces a disjoint segmentation by "last set wins"
    recovers faces, not groups.
  - Omit sets for `absorbed` faces (no triangles).
  - Object `partnumber` = `product.id`. Object and item metadata:
    `brepgpu:source_digest`, `brepgpu:definition` (`#N`), `brepgpu:occurrence`
    (NAUO path), `brepgpu:source_uuid`, `brepgpu:revision`.
  - If the Production extension is emitted (operator question 1), BRepGPU
    mints v5 UUIDs for items and objects (§2.2) and never random ones.
- **Not carried:** per-vertex classification, mesh-edge classification, collapse
  records. These live in the internal schema, keyed by the 3MF object and
  triangle index.

### 4.3 glTF 2.0 (D-formats: alpha, "with face ids and normals")

- **Format [src glTF 2.0 Specification.adoc, pinned].**
  - Application-specific attribute semantics start with `_` ("Vertex
    Attributes", lines 1576-1595).
  - "Accessors of the uint32 effective component type SHOULD NOT be used for
    any vertex attribute semantic, including application-specific semantics"
    (line 1595).
  - Vertex-attribute elements "MUST be aligned to 4-byte boundaries" (line
    1309), so a lone `UNSIGNED_SHORT` id costs 4 bytes per vertex anyway unless
    it is interleaved.
  - `extras` is "Application-specific data" and SHOULD be a JSON object
    (`schema/extras.schema.json`).
  - One index stream serves all attributes (lifecycle map S13).
- **Extensions [src, each pinned].**
  - `EXT_mesh_features` (Cesium; Draft on the proposal branch): `_FEATURE_ID_n`
    SCALAR attributes; FLOAT gives integer ids "up to 2²⁴"; an optional
    `propertyTable` through `EXT_structural_metadata` (not read).
  - `EXT_mesh_manifold` (Complete): `mergeIndices`/`mergeValues` recover the
    oriented 2-manifold without position welding. Manifold's own `MeshGL`
    keeps the same merge vectors "avoiding unreliable floating-point
    comparisons" and a per-triangle `faceID` (`manifold/include/manifold/mesh.h:50-60,
    150-153` at `761661c1`) [code].
  - `EXT_mesh_primitive_edge_visibility` (Bentley; Draft): 2 bits per triangle
    edge (hidden, silhouette, hard), where "a hard edge is any edge … between
    two logical faces".
- **Carriage [own]:**
  - The design duplicates vertices per face occurrence for per-corner normals
    (design §4 item 7; lifecycle map S13), so every vertex belongs to exactly
    one face. A per-vertex face id therefore *is* a per-triangle face id, at no
    topological cost.
  - Emit `_FACEID` as SCALAR FLOAT, the definition-local face ordinal. It is
    exact to 2²⁴ = 16,777,216 faces per mesh and costs 4 bytes per vertex,
    which the alignment rule charges even for UNSIGNED_SHORT. Refuse export, or
    split the mesh, above 2²⁴ faces in one definition.
  - Mesh `extras.brepgpu.faces`: per face ordinal, the source `#N`, UUID text,
    surface kind, canonical parameters (the geometric link, so a viewer can
    measure a hole's diameter exactly) and status. Add `extras.brepgpu.groups`
    for UUID sets and `shape_aspect`s, and source digest and occurrence paths
    on nodes. JSON is allowed here because glTF JSON is the external protocol.
  - Optionally add `EXT_mesh_features` naming `_FACEID` as a feature id set, so
    feature-aware viewers pick faces. Treat it as a vendor Draft (§7).
  - B-rep edges: `EXT_mesh_primitive_edge_visibility` hard bits from the §3.2
    mesh-edge classification, and silhouette bits on G1 edges. This draws the
    CAD edges the "stunning" goal needs without stippling (the extension's
    Figure 4 rationale) [src]. Optional, Draft.
  - Manifold recovery is G3's (`EXT_mesh_manifold`). The merge pairs come from
    physical node keys, never from positions.
  - Per-vertex (u,v) and `t` stay in the internal schema. glTF has no FP64
    attribute.
- **Reference behaviour [code].** OCCT writes one primitive per B-rep face
  unless merging is requested. Identity is then positional, and the draw-call
  count equals the face count (9,162 for the watch). `_FACEID` keeps one
  primitive per material.

### 4.4 USD (D-formats: later)

- **Format [src `usdGeom/schema.usda` at `25f3d3d8`, class GeomSubset and the
  Mesh primvar note].**
  - `GeomSubset` children of a Mesh with `elementType` ∈ {face, point, edge,
    segment, tetrahedron}. `edge` is "each pair of elements … a pair of points
    … connected as an implicit edge".
  - `familyName`, with `familyType` ∈ {partition, nonOverlapping,
    unrestricted}.
  - `indices` is an explicit `int[]`; there are no ranges.
  - A `uniform` primvar has "one element for each face of the mesh".
- **Carriage [own]:**
  - Family `brepgpu:face`, `partition`: one subset per source face. Custom
    attributes on each subset prim hold `#N` (int64) and the UUID string.
  - Family `brepgpu:group`, `unrestricted`: UUID sets and `shape_aspect`s.
  - Family `brepgpu:edge` (`elementType = "edge"`) per model edge, and
    `brepgpu:vertex` (`elementType = "point"`) per model vertex. This is the
    whole classification in standard schema terms.
  - Alternatively, a `uniform int primvars:brepgpu:faceId`, also O(triangles).
  - Material binding stays on its own subsets, as NVIDIA's writer does
    (`SmuTessellate.h:51-52`) [code].
- **Cost.** USD has no range form, so face carriage is O(triangles) ints, the
  same as `_FACEID`.

### 4.5 AP242 tessellated (D-formats: later)

- **Carriage [own, from §2.5]:**
  - One `tessellated_solid` per body with `geometric_link` → the
    `manifold_solid_brep`, and one `coordinates_list` per solid (RP §6).
  - One `triangulated_face` per face with `geometric_link` → its
    `advanced_face`.
  - One `tessellated_connecting_edge` per two-use edge with `geometric_link` →
    its `edge_curve`, `smooth` from the continuity class, and
    `line_strip_face1/2` from the chain.
  - One `tessellated_vertex` with `topological_link` → its `vertex_point`.
  - Source UUIDs stay on the exact entities in the same file, so a
    postprocessor reaches them through the links.
- **This is the only target format that carries the whole classification map
  natively, in double precision.**
- **Gaps [src].**
  - One-use edges (sheets, open shells) cannot be connecting edges: "face2 is
    not optional" (RP §5.4.2, §6). Use a plain `tessellated_edge`.
  - Accuracy parameters are untested in MBx-IF (RP §4.3).
  - `tessellated_face_surface` WR1 restricts its `geometric_link` to a bare
    `surface`, while `tessellated_face` itself allows `face_surface` (MIM
    35449-35468). Use the plain `tessellated_face` link to the
    `advanced_face`.

### 4.6 Summary table

| Field | Internal schema | STL | 3MF | glTF | USD (later) | AP242 tess (later) |
|---|---|---|---|---|---|---|
| Triangle → face | range per face | range in internal schema only | triangle set, 1 `refrange` per face | `_FACEID` FLOAT per vertex | `GeomSubset` face partition | `triangulated_face` per face |
| Face → source (`#N`, UUID, kind, link) | face table | — | set name/identifier; object metadata | `extras` face table | subset custom attrs | `geometric_link` + exact entity UUIDs |
| Groups (UUID sets, `shape_aspect`) | group table | — | extra triangle sets, written before face sets | `extras` groups (+ optional `EXT_mesh_features`) | unrestricted family | through exact entities |
| Vertex classification | physical key + `t`/(u,v) FP64 | — | — | — | edge and point subsets | `tessellated_vertex`, `line_strip` |
| Mesh edge on B-rep edge | derived | — | — | `EXT_mesh_primitive_edge_visibility` (optional) | edge subsets | `tessellated_connecting_edge` |
| Collapse history | record tree | — | — | `extras` (optional) | — | — |
| Product/occurrence | tables | header tag only | `partnumber`, metadata, Production UUIDs | node `extras`, names | prim paths, custom attrs | assembly structure |

## 5. Question 4: the cost of identity, and keeping it off the hot path

### 5.1 What costs, in principle

- **Ingress side tables.** One pass over the StepData model collects
  `uuid_*`, `shape_aspect`, GISU, IIRU and SAR records and the face and edge
  name strings into tables keyed by `#N`. The reader already loops over every
  entity once (`main.cpp:273-279`) [code]. This is O(entities) work off the
  mesher, and it never enters a mesher data structure [own].
- **Per-element identity lanes.** These are the real cost. Today the display
  `Triangle` spends 8 of 24 bytes on `face` [code `types.hpp:27-30`]. The scene
  keeps a separate `AdaptiveSceneFacetSource` per triangle and an
  `AdaptiveSceneVertexSource` per vertex (`adaptive_scene.hpp:16-26`) [code].
  The retired constructor carried a `std::variant` source on every
  `ConstructionSite` (`adaptive_trim_types.hpp:111-116`) [code]. Every extra
  lane is bytes moved per element and registers held in SIMD and warp loops
  [own].

### 5.2 The range layout [own]

- **Triangles are emitted face-contiguous.** Faces are already the unit of
  work, and the design's canonical facet order (§5.5) already sorts by key. A
  face's interior clusters (§5.4) concatenate in canonical order inside the
  face. Then triangle → face is one offset per face (F+1 u32), not one id per
  triangle.
- **Each face's vertex block is ordered [model vertices, then edge-chain
  samples per coedge use in loop order, then interior vertices].** The
  classification of every vertex is then one row per coedge use (edge `#N`,
  start, count, direction) plus one interior range per face. `t` and (u,v) are
  values the mesher stores anyway for normals and seams (design §4 items 3 and
  7).
- **Exceptions are sparse side tables keyed by vertex index:** quotient
  vertices, collapse survivors and their records, and second seam UV
  occurrences. Their counts are bounded by the number of singular points,
  collapsed features and seam samples, not by triangles.
- **GPU and SIMD batches carry identity once per batch.** The batch descriptor
  holds the face slot (and cell range), and lanes hold geometry only. Device
  output is compacted per face, so classification arrives with the compaction
  the device already does. The one SoA batch layout (design §6.2) needs no
  identity lane.
- **Arithmetic, OM10 watch (273,978 unique triangles, 9,162 faces;
  `results/om10-watch-2026-10-17/REPORT.md:16`).**
  - Per-triangle u64 face ids take 2,191,824 B; per-face u32 offsets take
    36,652 B, about 60× less.
  - If vertices are about T/2 plus per-face duplication, the per-vertex
    `AdaptiveSceneVertexSource` lane is of the same order as the triangle lane.
  - The range form of vertex classification is O(coedges + faces).
- **Determinism.** Identity tables are integers derived from source keys and
  canonical order, so they are bit-identical across thread counts by
  construction (D-determinism). Across backends, *which* face and edge each
  element belongs to is identical. Range *lengths* may differ, because a face's
  sample set may differ by backend (G7). This is input for G7's per-layer
  determinism classes.

### 5.3 Hypotheses and falsifiers (arena)

The literature read gives no measured cost of carrying identity in a CAD
mesher. Parasolid and OCCT document what they emit, not what it costs. Each of
the following is a measurement to run, not a claim [own]:

- **H1 (layout is free).** Face-contiguous triangles and entity-ordered vertex
  blocks fall out of canonical ordering with no extra pass.
  - Falsifier: an arena configuration in which emitting ranges needs a pass
    whose time exceeds 1% of complete meshing on any corpus family, or the §3.4
    oracle finds a non-contiguous face range.
- **H2 (carriage is cheap).** Complete identity carriage (ranges, sparse
  records, ingress side tables) adds under 1% to complete meshing wall time and
  under 2% to internal-schema bytes on every corpus family, on the CPU lane and
  on the P100 lane.
  - Falsifier: an arena flag `identity = {ranges, per-element lanes, none}`
    exceeds either bound on any family. `none` is the control, not a shippable
    mode.
- **H3 (UUID harvest is cheap).** Reading UUID, `shape_aspect`, GISU and SAR
  records adds under 1% to ingress.
  - Falsifier: the timed ingress probe on the NIST MBE PMI files (in the
    corpus) and on a PID test file (§7) exceeds 1%.
- **H4 (format cost).** `_FACEID` adds exactly 4 bytes per glTF vertex. 3MF
  face sets add O(faces) XML.
  - Falsifier: written-byte counts on corpus exports differ from 4·V and
    O(F) by more than the JSON or XML framing.

## 6. Implications for M5 (design contradictions and additions)

1. §5.5's per-vertex and per-facet "source maps" should be range tables with
   sparse exceptions, not per-element lanes (§5). The draft does not say which.
2. Ingress reads UUIDs, `shape_aspect` groupings, face and edge names and the
   revision string into side tables (§2.7). Today none is read.
3. The face outcome set gains `absorbed` (§3.3). Design §4 item 5's
   "triangles or an explicit refusal" would otherwise make a legal sub-ε
   collapse an accounting failure.
4. The sample-count rule gains the topological floor of §3.2. The draft's
   "Lines need no interior samples" and sagitta rule can emit duplicate mesh
   edges, breaking 3MF by-index manifoldness and the classification.
5. Collapse records follow Shephard's survivor tree with a deterministic,
   source-keyed survivor (§3.3). Validity comes from G10.
6. Per-format carriage follows §4. Exports write face-contiguous triangles in
   canonical order so ranges stay O(faces).
7. BRepGPU never mints a UUID for a source entity and never emits a random
   UUID. Minted UUIDs are v5 over source digest and path (§2.2).
8. Files without UUIDs (the watch) get exact per-input provenance (`#N` plus
   digest), never a claimed persistent identity. G12 owns cross-edit matching.

## 7. Defects, operator questions, open probes

**Defects found.** None in our code. The reader's missing identity entities are
an absent capability, not a fault (§1). One standard defect is recorded for
readers' sake: AP242 Ed.4 `v4_/v5_uuid_attribute` WR1 tests character 14, the
hyphen; the RP says to ignore it (§2.2).

**Operator questions.**

1. **3MF Production extension.** D-formats names 3MF with separate items but
   not the Production extension. That extension *requires* UUIDs on build,
   items, objects and components, and lets per-occurrence identity survive
   3MF. Should alpha 3MF emit it, as a recommended (not required) extension so
   slicers that do not support it ignore it? This lane's default would be yes,
   with v5 UUIDs (§4.2), but it widens the format scope, so it is the
   operator's.
2. **Draft glTF extensions.** `_FACEID` and `extras` satisfy "glTF with face
   ids" with core glTF alone. Should alpha glTF also emit
   `EXT_mesh_primitive_edge_visibility` (CAD edge lines) and `EXT_mesh_features`
   (both vendor Drafts)? They change no geometry and a consumer may ignore
   them. The question is whether to depend on drafts in a published format.

**Open probes (fleet, small).**

- Read one CAx-IF PID file through the pinned OCCT bridge. Confirm that
  `V5_UUID_ATTRIBUTE` arrives as `StepData_UndefinedEntity` with `UUID_SET_ITEM`
  as a typed sub-record whose references resolve (§1), and what an `ANCHOR`
  section does to the parse.
- Read GISU and IIRU structure on the NIST MBE PMI files (in the corpus).

## For SHOPPING_LIST.md

- CAx-IF PID test-round files (R55J `pdi_0-e5-242.stp` and R57J/R58J
  split-hole and BOF cases) and the cross-domain RP's TEST7/TEST8 STEP+QIF
  packages. They are linked from the RP but held on member-only PDES and MBx-IF
  shares. They are needed to exercise UUID reading at all, since the public
  corpus has no PID file (corpus/MANIFEST.md 3b: "Test-round models are
  members-only").
- CAx-IF "complete list of Referenceable Entities by a UUID" for AP242 Ed.4
  (PID RP App. A.2; member-only nextcloud link).
- ISO 10303-21:2016 normative text (paywalled; already listed by M1) for the
  formal statement that instance names are local to one exchange structure. §2.1
  uses the PID RP's distinction and an inference instead.
- QIF 3.0 (free at qifstandards.org after registration; not fetched, since
  accepting terms is out of bounds) for §5.13 persistent identifiers. Needed
  only if a QIF consumer is added.
- `EXT_structural_metadata` specification (CesiumGS); not on the pinned
  proposal branch. Needed only if face tables move from `extras` to a property
  table.

## 8. Acquisition and scrub notes

- No PDF was fetched. All PDFs read were already held in `papers/pdfs/`.
- The open texts were fetched at pinned revisions into the lane's scratch space,
  with SHA-256 recorded in the sources TSV. The AP242 Ed.4 MIM is
  2,529,983 bytes. Nothing was written into `quarantine/`.
- One `grep -c` count was taken from the private OM10 STEP file. Only entity
  name counts are reported.
- No repository file other than this paper and its sources TSV was edited.
  Nothing was built, run, committed or pushed.
- Privacy scrub: no hostnames, user names, absolute workstation paths or
  private operational detail. Paths are repository-relative or pinned upstream
  revisions.
