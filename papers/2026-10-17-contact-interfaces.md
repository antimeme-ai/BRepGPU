# Contact interfaces for mechanisms

Research lane M4-G17 of the fast-mesher campaign, 2026-10-17. Brief:
[docs/fast-mesher-campaign/briefs/M4-G17.md](../docs/fast-mesher-campaign/briefs/M4-G17.md)
with [M4-common.md](../docs/fast-mesher-campaign/briefs/M4-common.md). Stub:
[lifecycle map](2026-10-17-geometry-lifecycle-map.md) §4.1 G17 (stage S22).
Bead `brepgpu-12s.18`. Nothing was built, run, meshed or measured. The lane
read the doctrine, the operator-decision journal (gate G1), the campaign plan,
the lifecycle map (S10, S16, S17, S21-S24, §3, §4), the working-draft design,
the watch source papers, the mesher's assembly code, and the sources in
[2026-10-17-contact-interfaces-sources.tsv](2026-10-17-contact-interfaces-sources.tsv).
Open PDFs are in ignored `papers/pdfs/`.

Tags as in the lifecycle map: **[src]** a source establishes it; **[code]** read
in this repository at file:line; **[own]** this lane's derivation; **[inf]**
inferred from a source but not stated by it.

Operator decisions that bind this lane: **D-imprint** (none for alpha; matched
discretization across declared contacts comes with physics), **D-provisional**
(physics consumes only final meshes), **D-placements** (rigid and mirrored
placements share one mesh; uniform scale gets a mesh per scale with ε scaled;
non-uniform scale and shear refused).

## 0. Scoped statement

1. **Matched discretization is not what mechanism physics needs.** Every
   formulation that handles sliding contact works on non-matching meshes: CAE
   segment-to-segment and mortar methods, and every physics engine read
   (PhysX, MuJoCo, Bullet, Drake, Rigid IPC, affine body dynamics). Node-to-node
   matching is valid only for "negligible" relative sliding with "small"
   rotations [src: Ansys node-to-node chapter; Liu et al. 2021 §2]. Gear flanks,
   pallets and pivots slide and rotate by construction, so no fixed node
   pairing survives one step of motion. The 28-gear affine-body benchmark
   (2.45M triangles, every tooth contact resolved) used independent per-body
   triangle meshes [src: Lan et al. 2022 §5.3, Fig. 1].
2. **What mechanism physics does need from the mesher is fidelity relative to
   clearance, plus access to the exact surface.** At the watch's default
   ε = 0.005 mm, a vertices-on-surface mesh of a watch pivot jewel hole can
   take up the whole per-side side shake that traditional practice allows
   (1/5000 in = 5.1 µm) [src: Hall 1910; own §1.3]. A watch-module tooth flank
   would carry two or three facets, with normal jumps of 26-51° between them
   [own §1.3]. CAE contact literature names these normal jumps as the source of
   "unphysical oscillations" in sliding contact. Its remedy is a smooth master
   surface, which for rigid bodies "can be defined directly from CAGD models"
   [src: De Lorenzis–Wriggers–Hughes 2014 §3.2]. We hold those CAGD models.
3. **Instancing is unaffected for sliding contacts.** The requirement becomes
   a per-face tolerance on the *definition*: the tightest request over every
   occurrence that touches that face. That keeps one mesh per definition, as
   D-placements requires. Matched discretization would conflict with
   instancing ("each copy would have to be a unique part" [src: Ansys conformal
   meshing across parts]), but it is needed only for bonded, non-sliding
   interfaces in a CAE profile. Even there, CAE practice couples non-matching
   meshes with mortar or tie constraints.
4. **Decision for M5.** Do not reserve a matched-discretization stage. Reserve
   the nine interfaces in §3. The two that are expensive to retrofit are the
   per-face tolerance map in the mesh key (I1) and the triangle-to-(face, uv)
   classification map (I3). The other seven are records and reports that
   physics can add later without changing the mesher's core. The §11
   contact-density constraint is stated in §4, with its hypotheses and
   falsifiers.
5. **D-imprint's wording needs an operator answer.** D-imprint says matched
   discretization "comes with physics". The evidence says physics will not ask
   for matching. It will ask for per-face density and exact-surface access, and
   matching applies only to a future bonded-CAE profile. That reading changes
   what the operator decided, so it goes back as question Q-1 (§8). This lane
   does not settle it.

## 1. Question 1: contact meshing in CAE and physics engines

### 1.1 CAE contact discretizations

| Formulation | Mesh requirement | Sliding | Source |
|---|---|---|---|
| Node-to-node (NTN) | "the nodes of the two opposing surfaces match up geometrically" | "relative sliding ... negligible"; "rotations ... must remain small" | [src] Ansys MAPDL Contact Technology Guide ch. 9 |
| NTN, academic statement | "can only be used with conforming discretizations along master/slave surfaces and only for infinitesimal sliding" | infinitesimal | [src] Liu et al. 2021 §2 |
| Node-to-segment (NTS), one pass | non-matching | finite | [src] Liu §2: biased master/slave choice, "fails to prevent interpenetration at times", fails the patch test |
| Segment-to-segment, Gauss-point-to-surface | non-matching | finite | [src] De Lorenzis 2014 §3.1: pass the patch test (STS) or pass it to integration error (GPTS) |
| Mortar | non-matching | finite, large | [src] De Lorenzis §3.1: "variationally consistent treatment of non-penetration and frictional sliding"; patch test and LBB satisfied |
| Small vs finite sliding (Abaqus) | small sliding fixes the node-to-segment pairing "for the whole analysis"; finite sliding must "continually track" it | — | [src] Abaqus small and finite sliding page |
| Tie / bonded | tie constraints are "often used to connect incompatible meshes" | none | [src] CalculiX user forum (secondary); Liu §1 on mortar and multipoint-constraint coupling of non-conforming meshes |
| Shared topology / imprint-merge | geometry made conformal before meshing | none (one body afterwards) | [src] Ansys conformal meshing within and across parts; Cubit imprint/merge [lifecycle map S22]; Clark–Hanks–Ernst 2008 |
| Contact match (Ansys) | post-mesh node matching "between topologically disconnected solids within a specified tolerance"; editing a match "only requires local re-meshing" | none | [src] Ansys Contact Matches |

Gear stress analysis is the CAE use closest to a watch train. It uses
non-matching surface-to-surface contact elements with an augmented Lagrangian.
The dense mesh zone "moves along the line of action", and each position is
generated from points of a simulated hobbed profile [src: Lahtivirta–Lehtovaara 2016,
abstract and model section]. Element size is set by Hertzian agreement [src: Coy–Chao
1981, abstract]. These meshes are analysis meshes, built from the profile and
re-densified as the contact point moves. A CAE gear solver would remesh our B-rep rather than
consume a display mesh [inf].

**Where matching matters [src, synthesis].** Matching matters where the
interface does not move: bonded or tied interfaces, multi-region conformal
meshes (shared topology) and small-sliding NTN contact. Even for bonded
interfaces, mortar and tie methods make matching a convenience, not a
necessity.

### 1.2 Physics engines and collision proxies

| System | Geometry it collides | Matching across bodies? | Source |
|---|---|---|---|
| MuJoCo | mesh geoms "are convexified for collision purposes"; non-convex through SDF plugins (one is a gear extrusion) or flex machinery | no | [src] MuJoCo modeling guide; plugin/sdf README |
| Bullet | convex decomposition (V-HACD) or hand-made proxies; its concave triangle-mesh path showed "large 'ghost' forces" | no | [src] Ferguson et al. 2021 §6 |
| PhysX 5 | dynamic (non-kinematic) triangle meshes require an SDF, "only available when using the GPU collision detection pipeline"; convex meshes limited to 255 vertices and faces | no | [src] PhysX 5.4.1 Geometry guide |
| PhysX SDF guidance | resolution ~250 usually enough; features thinner than "about twice the grid spacing" may be lost; large faces on dynamic SDF meshes give "suboptimal results", remesh to smaller triangles | no | [src] Omniverse Physics collision guide |
| Factory (PhysX SDF) | SDF against triangle mesh, "one contact per triangle-mesh face"; gear assembly and nut-and-bolt at ISO clearances; gear shaft clearances 0.005-0.014 mm; listed limitation: low-tessellation meshes allow "penetration on large flat surfaces" | no | [src] Narang et al. 2022 §III.A, Tables VI-VII, §VII |
| Drake hydroelastic | compliant bodies are tetrahedral meshes (VTK) carrying a pressure field; rigid bodies are triangle surface meshes; `resolution_hint` is a target edge length for tessellated primitives | no | [src] Drake hydroelastic user guide |
| Rigid IPC | input triangle meshes directly; barrier active within "a user-defined geometric accuracy d̂"; requires "a non-interpenetrating starting configuration"; tested mechanisms "with tight conforming contact" (bike chain on sprocket, bolt in nut, lock box from STL files) | no | [src] Ferguson et al. 2021 §1, §4, §5 |
| Affine body dynamics | 28 toothed gears, 2.45M surface triangles, "well over a quarter" of them in contact each step; Rigid IPC, MuJoCo and Bullet all failed this scene | no | [src] Lan et al. 2022 §5.3 |
| Joint-level gears (PhysX) | `PxGearJoint` "uses a gear ratio to constrain the relative angular velocity"; rack-and-pinion likewise; no tooth geometry involved | n/a | [src] PhysX 5.4.1 Joints guide |

Two further facts bear on watch scale. Bullet and MuJoCo degraded or slowed at
0.1× scale, and Chrono's bolt scene worked only after scaling the scene up 10×
[src: Ferguson §6]. The tested engines are tuned for metre-scale scenes, while
a watch train is millimetre-scale with micrometre clearances [inf].

**Where a collision proxy suffices [src/inf].**
- Contact geometry is not simulated at all. Gear trains can be gear joints
  with ratios from tooth counts, and pivots can be revolute joints (PhysX
  joints). Mesh density is then display only.
- Clearance is large against the proxy error, as with exploded-view
  separation, which already uses stored mesh bounds
  [docs/om10-exploded-assembly-unit.md].
- Convex shapes, where a convex hull is exact.

Contact-level simulation of teeth, pallets or pivots at watch clearances rules
out convex proxies. Convex hulls cannot represent the zigzag of teeth: Bullet's
convex proxy "does not capture the zigzag geometry at the gear tooth"
[src: Lan §5.3]. That leaves an SDF or the triangle mesh itself, and both
inherit the mesh's fidelity.

### 1.3 What fidelity contact needs instead [own unless tagged]

**Pin in hole.** With vertices on the surface, both polygons are inscribed. A
pin of radius R_p keeps its circumscribed size R_p. A hole of radius R_h shrinks
to the inradius R_h − δ_h, where δ_h = R_h(1 − cos(π/n_h)) is its sag. The pin
fits at every relative rotation only if the per-side clearance satisfies
c = R_h − R_p ≥ δ_h. Otherwise the faceted pin interferes at some angles: it
cogs or jams under rigid contact, and it violates IPC's intersection-free
starting state. When it fits, the radial play it allows varies with rotation
between c − δ_h and c + δ_p, so the pin's axis wanders by up to about
δ_h + δ_p. This is the feature-of-size bias the lifecycle map states for holes
(S2 finding 5), applied to a mating pair.

Hall (1910) gives the traditional watch pivot side shake. A pivot is made
"1/2500 of an inch smaller than the hole", and "1/5000 on each side of the
pivot, is as little space as it is desirable to leave", for pivots of 12/2500 to
16/2500 in [src: Hall, ch. on pivot measurement]. That is c ≈ 5.1 µm per side on
pivots of 0.12-0.16 mm diameter. The modern ISO 286 gear-shaft fits in Factory
are 0.005-0.014 mm diametral [src: Narang Table VII]. For a pivot of radius
R = 0.061 mm, δ = R(1 − cos(π/n)) ≈ π²R/(2n²) gives:

| Sag δ on the jewel hole | Sides around the hole | Per-side clearance left (c = 5.08 µm) |
|---|---|---|
| 5 µm (watch default ε) | 8 | 0.08 µm |
| 1 µm | 18 | 4.08 µm |
| 0.5 µm | 25 | 4.58 µm |
| 0.1 µm | 55 | 4.98 µm |

At the default ε, a pivot and its jewel hole mesh as octagons, and the hole's
faceting takes up the whole minimum side shake.

**Tooth flanks.** A chord of length L on a curve of radius ρ has sag
δ = ρθ²/8, where θ = L/ρ. Adjacent facets therefore differ in normal by
θ = √(8δ/ρ). OM10's train modules are 0.13, 0.11 and 0.08 mm
[src: OM10 technical sheet p2, via
[watch-drawing-source-frumentarius](2026-10-15-watch-drawing-source-frumentarius.md)].
Take flank radii of curvature of the order of the module (assumption, not
read; the census in §5 measures them):

| ρ | δ = 5 µm | δ = 1 µm | δ = 0.1 µm |
|---|---|---|---|
| 0.05 mm | 51° | 23° | 7.2° |
| 0.10 mm | 36° | 16° | 5.1° |
| 0.20 mm | 26° | 11.5° | 3.6° |

At ρ = 0.1 mm and δ = 5 µm the chord is 63 µm, so an addendum flank of about
1.5 m ≈ 0.17 mm carries two or three facets. The contact normal, and so the
direction of the transmitted force, jumps by tens of degrees as the contact
crosses a facet edge. De Lorenzis et al. describe exactly this for faceted
master surfaces: "The resulting abrupt change in the direction of the contact
forces may generate unphysical oscillations in the results as well as serious
iterative convergence problems" [src §3.2.1]. Normal averaging, Hermite,
Bézier, Gregory-patch and NURBS smoothing are the remedies. For a rigid
obstacle, smooth surfaces "can be defined directly from CAGD models"
[src §3.2.2]. Tightening ε alone converges slowly: θ ∝ √δ, so halving the
normal jump costs four times tighter sag. On these faces the triangle count per
unit length grows as 1/√δ.

**Planes are exact.** Pallet-stone impulse and locking faces, and other planar
contact faces, mesh with zero sag. The constraint bites only on curved contact
faces: cylinders such as pivots and holes, flank arcs and B-splines, and cams.

**The source floor.** OM10 declares a model-connectivity uncertainty of
0.01 mm on every context [src:
[om10-step-source-study](2026-10-15-om10-step-source-study.md)]. That is twice
Hall's per-side side shake. The study also found no fits, tolerances or
assembly clearances in the file. Whether OM10 models pivots at nominal
zero-clearance size, as CAD often does [inf], is not known. The census in §5
would show it. If pivots are nominal, physics needs a clearance declared from
outside the B-rep, and no mesher setting can supply one.

### 1.4 Answer to question 1

| Situation | Matched discretization? | What the mesher must provide |
|---|---|---|
| Display, explosion animation | no | ε, closed bodies |
| Joint-level mechanism (gear joints, revolute pivots, contact only at the escapement) | no | contact density on the escapement's curved faces only |
| Contact-level rigid physics (SDF or mesh barrier) on teeth, pallets, pivots | no; sliding defeats any pairing | per-face ε and an angular cap on contact faces, tied to clearance; closed, non-merged bodies; exact-surface access for normals; facet-induced penetration report |
| Deformable parts (hairspring) | no | the hairspring is a structural element, not a contact interface; a volume or beam mesh for it is a consumer's job [inf] |
| CAE stress on teeth | no (surface-to-surface, mortar) | the consumer remeshes from the exact profile; we supply the B-rep and classification |
| Bonded multi-material or tied CAE interface, conformal region meshes (AMF, CAE) | sometimes; a convenience even then (tie and mortar exist) | the constraint-curve hook (I5) and per-occurrence variants (I6), later, behind a CAE profile |

## 2. Question 2: coexistence with instancing

**Problem.** The design meshes each definition once and places it many times
(design §5.1; code: one mesh per definition per query,
`native/mesher/adaptive_model_main.cpp:249-270, 368-449` [lifecycle map S10]).
A definition such as a screw, a jewel or a repeated wheel can touch different
neighbours in different occurrences.

**Literature.**
- Shared topology between instance copies forces "each copy ... to be a unique
  part" [src: Ansys conformal meshing across parts].
- Ansys's alternative is post-mesh: mesh connections, node merge and contact
  matches. Base meshes are stored and only local re-meshing follows a match
  edit [src: Ansys Contact Matches].
- Parasolid keeps local tolerances per entity on the body
  [src: lifecycle map S19].
- OCCT meshes in the first occurrence's frame and silently reuses the result
  for every other occurrence [src: lifecycle map S10]. That is the failure to
  avoid.

**Resolution for sliding contacts [own].** Contact density is a property of a
definition's faces, set by the worst occurrence:

  ε_f(D) = min over occurrences o of D, and over declared contacts touching
  (o, f), of ε_contact(pair) / s_o

with an angular cap θ_f(D) taken the same way. Here s_o is the occurrence's
uniform scale under D-placements (1 for rigid and mirrored placements). This is
what today's code already does for one scalar per definition: the allowance is
the minimum over instances of (positional − storage error)/norm
(`native/mesher/assembly.cpp:160-169` [code]). The change is to make it
per face. One mesh per definition survives. The price is that occurrences
without that contact are refined on that face too, which affects only the
contact faces, and their triangle counts grow as 1/√δ along curved directions
(§1.3).

**Matched discretization under instancing (bonded CAE only) [own, from the
literature's options].**
1. **Consumer-side coupling** (mortar, tie): no mesh change; instancing is
   intact. This is the default whenever the consumer supports it.
2. **Per-occurrence face variants.** Only the faces in the interface are
   re-meshed with the partner's imprint curves in that occurrence's relative
   frame, keyed by (definition, face, partner signature). Every other face
   stays shared. This is the Ansys contact-match pattern restricted to faces.
3. **Union imprint on the definition.** All partners' curves from all
   occurrences are imprinted into one definition mesh. It stays shared but
   every occurrence carries every imprint, and the cost grows with the number
   of distinct neighbours. Rejected except for few, regular patterns.

No source compares options 2 and 3 on instanced assemblies. The literature
does not say which is cheaper; the measurement is in §5 (M-3).

**Motion.** Explosion while running and animation change which faces touch
over time [operator roadmap, journal]. A matched discretization fixed at
assembly time would be stale after the first frame of explosion. That is
another reason sliding and moving contacts must not depend on matching [own].

## 3. Question 3: interfaces M5 must leave open

Each line says what the interface is, why physics needs it, today's state, and
what retrofitting it later would cost.

| # | Interface | Why | Today | Retrofit cost if omitted |
|---|---|---|---|---|
| I1 | **Per-face tolerance map** (ε_f, optional angular cap θ_f, optional maximum edge length ℓ_f) in the definition mesh request and in the mesh key, in definition units | contact density (§4); PhysX/Factory want small triangles on contact faces; G14 sizing uses the same channel | one scalar allowance per definition (`native/mesher/assembly.hpp:23-26`, `assembly.cpp:160-169` [code]); design §5.1 cache keyed by source identity only | **high**: key, scheduler work estimate and density rules all assume one ε per definition |
| I2 | **Declared contact record** (input, not consumed in alpha): two (occurrence path, face set) sides; kind {sliding, rolling, intermittent, bonded}; declared per-side clearance or "measure"; requested ε_contact and θ_contact | lets physics state its needs without a mesher change | absent | low: a new input; only its reduction into I1 touches the mesher |
| I3 | **Classification map**: per triangle (source face id); per vertex (face, u, v) or (edge, t) or source vertex; source-carrier ids resolvable to evaluators | exact contact normals and closest points from the CAGD surface (De Lorenzis §3.2.2); SDF generation from the exact surface | partial: `FacetKey`, `ConstructionSite.source` (`native/mesher/adaptive_trim_types.hpp:95-116` [lifecycle map S23]); design §5.5 source maps; G16 owns the fields | **high** if facets lose their source link in the hot path; G16 decides carriage |
| I4 | **Bodies never merged; each occurrence a closed body with its own identity** and its transform kept apart from part-local geometry | SDF cooking wants watertight meshes (Omniverse guide); Drake compliant bodies need tet meshes made from closed surfaces; 3MF must carry mechanisms as separate items (D-formats) | design §4.3 closed-shell rules; D-formats separate items | low if kept now; merging is never proposed |
| I5 | **Extra constraint curves in face meshing**: a face accepts UV polylines whose vertices are keyed like edge samples, with no B-rep edit | the only route to matched or imprinted interfaces later (bonded CAE, AMF regions) | band CDT is constrained (design §5.3, `direct_cdt`); no external constraint input | medium: the band generator and the edge-forest key space need a "virtual edge" kind |
| I6 | **Mesh-key slot for variants**: (definition, ε map digest, constraint-set digest) | per-occurrence face variants (§2 option 2) without forking whole definitions | key not yet defined (G9 owns it) | medium: G9 should reserve the slot |
| I7 | **Final-only status for physics** | D-provisional: physics consumes only final meshes | design §7 provisional state is first class | low: a status check at the physics boundary |
| I8 | **Contact fit report**: per declared pair, the faceting's effect on clearance (c − δ_hole − d_consumer) and any facet-induced penetration between occurrences | IPC needs an intersection-free start; pin-in-hole jam (§1.3) | design §5.6 and the offline oracle cover pairs within one definition; between occurrences of different definitions, nothing | low as an offline oracle; it must exist before physics trusts a mesh |
| I9 | **One-sided tolerance per face** (ε⁺, ε⁻ about the surface) | lets a hole's mesh keep its clearance (vertices outside so chords lie on the surface) instead of shrinking it; G15 owns feature-of-size bias | fidelity contract is symmetric (design §4.2) | medium: the contract and the deviation oracle need a signed band |

**What M5 should do now.** Write I1 and I3 into the design as structural
requirements. Reserve the key slot in I6, coordinated with G9. State I4, I7 and
I8 as contract and oracle lines. Name I2, I5 and I9 as later extensions with
their attachment points (the density input, the band constraint set, the
deviation band). Build none of them for alpha beyond I1's per-face map, which
also carries G14's sizing terms.

## 4. The contact-density constraint for §11 (proposed text)

> On a face of a declared contact pair, the mesher meets the per-face request
> (ε_f, θ_f) reduced over occurrences (§2), in addition to the global ε.
> Planar contact faces need nothing more. For a curved contact face with
> declared or measured per-side clearance c, the default request is
> ε_f ≤ c/k with k = 10 (arena flag), and θ_f ≤ 10° (arena flag). Physics
> takes contact normals and closest points from the exact surface through the
> classification map, not from facet normals. The fit report (I8) states the
> remaining clearance per pair; a facet-induced penetration between
> occurrences is reported, never silently accepted. Matched discretization is
> not part of this constraint.

The values k = 10 and 10° are calls. No source ties sag or facet angle to
contact-force error for rigid bodies at these scales; the literature does not
say. They are flags so the measurements in §5 can set them.

## 5. Hypotheses, falsifiers, and the measurements the literature does not supply

| ID | Hypothesis | Falsifier | Measurement |
|---|---|---|---|
| H-1 | Sliding mechanism contact needs no matched discretization: independent per-body meshes give contact behaviour that converges with ε_f alone | a pin-in-hole or gear-pair simulation whose contact force or transmission error converges only when nodes are matched | M-1, physics phase |
| H-2 | A faceted pin in a faceted hole jams or cogs once δ_h approaches c, and is clean below δ_h ≤ c/k for some k ≤ 10 | no torque-ripple or jam difference between δ_h/c = 1 and 0.1 | M-1: authored pin-in-hole family, c ∈ {1, 2, 5, 10} µm, δ_h/c ∈ {1, 0.5, 0.2, 0.1}, rotated under a barrier contact model and under an SDF model; record ripple, jams and minimum distance |
| H-3 | Contact normals from the exact surface (via I3) remove the facet-edge force oscillation at the default ε, so θ_f can stay loose | force-direction jumps persist with exact normals because the penetration test still uses facets | M-1 with exact-normal and facet-normal arms |
| H-4 | Per-face ε on contact faces costs a small fraction of a definition's triangles | contact faces dominate the triangle count of OM10 train definitions at ε_f = c/10 | M-2: arena run with I1 on the watch's pivot and flank faces; per-face triangle counts |
| H-5 | For bonded interfaces under instancing, per-occurrence face variants (§2 option 2) beat union imprint (option 3) in time and triangle count | union imprint is cheaper on the corpus's instanced assemblies | M-3: only if a bonded-CAE profile is ever adopted |
| — | Census (not a hypothesis): OM10 coaxial cylinder pairs across occurrences, with radius differences, and the radii of curvature on tooth flank faces | — | M-4: a text-level census on the fleet; shows whether OM10 models clearance or nominal size, and gives the ρ for §1.3 |

M-1 needs a contact simulator, which belongs to the physics phase (D-consumers:
PhysX arrives with physics). M-2 and M-4 can run in the M8 arena.

## 6. Corrections to the existing record

- **Lifecycle map S22 / G17** frames the open question as "matched
  discretization across a declared contact without welding". For sliding
  contact the literature makes that the wrong axis (§1). The right axis is
  per-face density relative to clearance, plus exact-surface access.
- **Design §11** says contact surfaces "need faithful density" without a
  measure. §4 gives the measure: sag against clearance, facet angle, exact
  normals.
- **Design §5.6 and §4.9** cover intersections within a definition and the
  source's own self-intersections. Facet-induced penetration *between
  occurrences* at touching or nominal-fit interfaces is not covered. Inscribed
  faceting of a nominal pin and hole guarantees it (§1.3). This needs I8.
- **Brief M4-G15, question 3**, says "an inscribed chord makes holes smaller and
  bosses larger". With vertices on the surface, a boss's circumscribed (mating)
  size is unchanged and its material shrinks inward; it does not grow (§1.3).
  The lifecycle map's statement (hole mating diameter 2(R − δ)) is consistent
  with this. This is a note for G15, not a code defect.

## 7. Defects found

None in code. The single per-definition allowance (`native/mesher/assembly.cpp:160-169`)
is correct for what it does; per-face tolerance is a missing interface (I1), not
a bug.

## 8. Operator questions

- **Q-1 (D-imprint).** D-imprint says matched discretization across declared
  contacts comes with physics. The evidence says physics needs per-face density
  tied to clearance plus exact-surface normals, and that matching serves only a
  bonded-CAE profile. Should D-imprint be restated that way, or did "matched
  discretization" mean something specific that this lane has missed?
- **Q-2 (physics fidelity level).** For "correct mechanism physics" on the
  watch: is it joint-level (gear joints with ratios from tooth counts, revolute
  pivots, contact only at the escapement), or contact-level (every tooth and
  pivot contact simulated, as in the affine-body 28-gear benchmark)? The
  contact-density constraint applies to the escapement only in the first case
  and to every curved contact face in the second.
- **Q-3 (clearance source).** OM10 carries no fits or clearances, and its
  model uncertainty (0.01 mm) exceeds traditional pivot side shake. If the
  census (M-4) shows nominal zero-clearance pivots, where do clearances come
  from: a declared contact record (I2), a horological standard, or the
  operator?

## 9. For SHOPPING_LIST.md

| Item | Why | Access |
|---|---|---|
| Puso, Laursen, "A mortar segment-to-segment contact method for large deformation solid mechanics", CMAME 193 (2004) 601-629 | primary mortar source; read here only through De Lorenzis 2014 | paywalled |
| Puso, Laursen, "A 3D contact smoothing method using Gregory patches", IJNME 54(8) (2002) 1161-1194 | smoothing remedy for faceted masters | paywalled; abstract only |
| Wriggers, *Computational Contact Mechanics*, 2nd ed., Springer 2006 | standard reference for C1/C2 surface requirements in dynamics | book, paywalled |
| NIHS 20-02 (horological gear profiles; named by the OM10 sheet) | flank radii of curvature for §1.3 instead of the module-order assumption | Swiss horological standards, subscription |
| Reymondin et al., *The Theory of Horology* (WOSTEP/FH) | modern pivot and jewel fits to replace Hall 1910 | book, paywalled |
| Roda-Buch, Magnin, Maldonado, Mischler, "The role of geometrical parameters in the lubrication of the Swiss lever escapement", J. Appl. Mech. (2021), DOI 10.1115/1.4050278 | escapement contact forces and radii from a multibody model | paywalled; EPFL record seen, full text not checked |
| Zhang et al., isogeometric contact analysis of a cycloid pinwheel mechanism, *Coatings* 13(12):2029 (2023) | IGA vs FEM contact-force accuracy on toothed contact | open (MDPI) but returned HTTP 403 to the fetcher; retry |
| Liu et al., isogeometric contact simulation of gear meshing, J. CAD & CG (2024), DOI 10.3724/SP.J.1089.2024-00482 | exact-geometry gear contact | open preview; not read |
| Kim (KAIST), matched meshes enabling node-to-node contact with finite sliding (record 10203/264557) | the one counterexample direction: re-matching meshes during sliding | repository host did not resolve; retry |
| PhysX 5 GPU minimum compute capability | whether PhysX SDF collision runs on the fleet's P100 (sm_60) at all | not found in the pages read; physics-phase question |
