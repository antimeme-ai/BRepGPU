# Invalidation and equivalence under edits

Research lane M4-G12 of the fast-mesher campaign, 2026-10-17. Briefs:
[M4-common](../docs/fast-mesher-campaign/briefs/M4-common.md) and
[M4-G12](../docs/fast-mesher-campaign/briefs/M4-G12.md); bead `brepgpu-12s.13`.
Stub: [lifecycle map](2026-10-17-geometry-lifecycle-map.md) §4.1 G12 (stages
S16, S24; contradiction X10). Builds on
[G7, determinism and cache keys](2026-10-17-determinism-cache-keys.md) and
[G2, face-to-edge feedback](2026-10-17-face-edge-feedback.md), which this lane
read first. Sources and read depth:
[2026-10-17-edit-invalidation-sources.tsv](2026-10-17-edit-invalidation-sources.tsv).

Nothing was built, run, meshed or measured, here or on the fleet. The lane read
literature in `papers/pdfs/`, OCCT 8.0.1 at `b8f597c6` in `quarantine/`, and
this repository's papers, design draft and code.

Tags: **[src]** a source establishes it (section, page or file:line given);
**[code]** read in this repository at file:line; **[own]** this lane's
derivation; **[inf]** inferred from a source but not stated by it. Design
section numbers refer to `docs/FAST_MESHER_DESIGN.md` (working draft). G7 and
G2 section numbers refer to their papers.

## 0. The four rules

**Rule C, the change description.** The host hands the mesher *content*, never
verdicts. Either the whole new model (channel F, full), or a record delta
(channel Δ): the new bytes of every changed geometry and topology record, the
IDs of deleted records, the records of created entities, changed placements
and changed requests. History relations (OCCT `BRepTools_History`
Modified/Generated/Removed, Parasolid bulletin-board events, our own
`EntityKey` revisions) are optional and feed only the old-to-new
correspondence the mesher reports. They never enter a key and never decide
reuse. The mesher derives everything else itself: which tasks changed (by
content digest), the refined-edge neighbours, the halo, and which faces are
reused, re-meshed, new or deleted (§2).

**Rule I, invalidation.** A task is recomputed exactly when its G7 key misses.
The key is built over the output digests of the task's dependencies, so the
invalidation set is not declared; it is the forward closure of the changed
records through the task graph, cut off wherever a recomputed task reproduces
its old bytes. Its static upper bound, the halo, is (§3):

- topological: the changed faces, the faces within two vertex-adjacency rings
  of them, and nothing further. The second ring arises only when G2 closure
  adds samples to an edge of a first-ring face;
- spatial: every task whose recorded query region (thin-feature ε-inflated
  pair query, ¼-local-feature-size bound, collapse candidate search) meets the
  old or new bounds of a changed entity;
- component: every small-feature collapse cluster that touches a changed
  entity, in the old or the new model.

Four global couplings are forbidden, because each would make the halo the
whole model: run-global counters inside task outputs (vertex IDs), tie-breaks
by entity number, a per-run output-size guard that picks which faces to
refuse, and a global placement optimizer for shared entities (§3.5).

**Rule R, reuse acceptance.** Reuse is a key hit and nothing else. A face is
reused when its own source-record digest, its ordered loop structure, the
output digest of every chain in loop order (positions, parameters, dyadic
paths, one UV per occurrence), its collapse slice, its thin-feature slice and
its request (ε_F and every flag it reads) all match, under the same
`backend_id`. Achieved or requested deflection is never a reuse criterion
(§4). OCCT's 1.1×/0.9× band is rejected: it ignores geometry changes and keeps
history.

**Rule E, equivalence.** On one `backend_id` with the same placement rule,
incremental output equals a cold fresh run of the edited model byte for byte,
in every D0–D2 layer. It holds for any edit sequence, any cache eviction state
and any thread count. Undo restores the original bytes with every task a hit.
Across backends, or under load-reactive placement, the two runs are
functionally identical (G7 FI-1 to FI-5). Under load-reactive placement the
incremental run additionally equals a fresh run replayed with its recorded
placement log. D3 and the edit-disposition records are excluded (§5). The
oracle is G7's O-bytes "incremental versus fresh" arm, driven by an edit
generator and made non-vacuous: every case must both hit and miss (§5.4). It is
the same oracle as G7's key-completeness test, not a second one.

The reason the rules are this simple: incremental remeshing here is memoized
fresh meshing. No code path patches an old mesh. Every source read in §6
patches (mesh morphing, node copying, incremental triangulation, a reuse
band), and every one of them reports, or implies, that the incremental result
differs from the fresh one.

## 1. What binds, and the terms used

- **D-determinism** [operator]: identical output across thread counts on one
  backend; functionally identical across backends; one model may mix backends.
- **D-provisional** [operator]: provisional meshes are display only; physics
  consumes only final meshes.
- **G7** [src G7 §1–§5]: determinism classes D0 (canonical decode), D1
  (per-backend reproducible), D2 (pure functions of D0/D1 bytes on a fixed host
  path), D3 (timing-dependent, never keyed); the one-producer rule for shared
  entities; the key

  ```
  key = H( key_schema ‖ task_kind ‖ backend_id ‖ request ‖ digest(own source records)
           ‖ for each consumed dependency: role ‖ digest(that dependency's OUTPUT bytes) )
  ```

  with identity (file names, entity numbers) kept out of keys in a separate
  identity-binding layer; and G7 §5.7's three conditions for incremental =
  fresh.
- **G2** [src G2 §0, §3]: four hereditary segment predicates P1–P4; the chain is
  the unique minimal dyadic refinement in which every segment passes every
  predicate in every incident occurrence. The predicates read source arcs and
  carriers, never other edges' chains.
- **M2** [src neighbour-reconciliation §5.3]: resolution in three acyclic levels.
  L1 solves every edge sample and every full-rank vertex. L2 resolves the
  rank-deficient vertices from the L1-resolved incident edge ends. L3 blends each
  edge's end corrections along its chain.
- **Edit.** A pair (M, M′) of models with the same request schema, together
  with a cache populated by any earlier runs. A *fresh* run of M′ is a cold run
  in a new process with an empty cache. An *incremental* run of M′ reads the
  cache.

## 2. Question 1: the change description

### 2.1 What the hosts offer

- **OCCT `BRepTools_History`** [src `BRepTools_History.hxx:27-88`, OCCT 8.0.1]:
  - three relations: Generated G(Sᵢ), Modified M(Sᵢ) and Removed R(Sᵢ);
  - "kept only for shapes of types vertex, edge, face, and solid". Wires,
    shells, coedges, surfaces and curves have no entries;
  - an unchanged output *is* the input shape (`Tj == Si`), so "unchanged" is
    pointer identity of a `TShape`, which is not persistent across processes
    (map S17);
  - "modified" covers changes "of the location, the tolerance, the bounds of the
    parametric space …, the parametrization and/or by applying of an
    approximation". Some of these change no mesh (tolerance is metadata, design
    §5.1). Others change every sample without changing the geometry (a
    reparametrization moves every dyadic key).
- **OCCT in-place edits** [src `BRep_Builder.cxx:564-578`; `TKMesh` grep]:
  `UpdateFace` replaces a face's surface in place and sets `TShape()->Modified(true)`.
  No file in `TKMesh` reads a shape's `Modified()` flag; the only `IsModified`
  there is `BRepMesh_IncrementalMesh`'s own result flag
  (`BRepMesh_IncrementalMesh.hxx:72`). So an in-place geometry change keeps
  its old triangulation (the map's S16 stale-reuse case, now confirmed in code).
- **Parasolid bulletin board** [src FD V35 §99.2.1–§99.3, pp. 1718–1721]:
  - events create, delete, split, merge, transform, copy, attribute change,
    transfer and unspecified change;
  - recorded for bodies, topology, groups, *and geometry* (surfaces, curves,
    points);
  - with merge rules such as create-then-delete leaving nothing, and
    change-then-transform staying a change;
  - and the caveat "the bulletin board records what actually happened
    internally within Parasolid during a modeling operation, this may sometimes
    give unexpected entries" (§99.2.5).

  Partition marks give the same answer by rollback:
  `PK_PMARK_ask_entities` lists the entities "created, modified, or deleted"
  between marks (§99.1 note; §97.2.4). Incremental graphics redraws "anything
  that has been modified or added" (§99.5.1).
- **Parasolid incremental faceting** [src FD §107.2.16, pp. 1825–1828]:
  - refacets faces "not already faceted, … modified, or that need to be refaceted
    due to refinement of their boundaries";
  - the attribute method marks faceted faces with a system attribute and deletes
    it from changed faces. Such attributes are deleted on split events (§48
    note, p. 750);
  - a rigid- or reflection-only transform can be exempted
    (`incremental_transformation`);
  - "changes to other facet controls may not be honoured while incremental
    faceting is switched on". That is hysteresis by documentation.
- **Wu and Sarma 2001** [src, abstract only via Crossref, JCISE 1(4):320–329]:
  transfer "parts of b-reps (not the complete b-rep)" between collaborating
  applications, by extraction and embedding on cellular models. This is a
  record-delta channel. Their 2004 paper on incremental editing of *faceted*
  models (CAD 36(9):821–833) is closed access and was not read (shopping
  list).
- **Our own feature operations** [code `docs/BOOLEANS.md:58-64`]: every original
  entity carries `EntityKey(body_id, revision, kind, entity_id)` with status
  retained, modified or deleted, and every output has reverse origins,
  including generated ones. That is `BRepTools_History` plus an explicit
  "retained".
- **The build-system view** [src Mokhov et al. 2018 §2.1, Def. 2.1]: Make is
  minimal "only under the assumption" that modification times tell the truth.
  Host "modified" flags are modification times. Content hashing removes the
  assumption (G7 §5.1).
- **Names in incremental computation** [src Hammer et al. 2015, abstract, §2.4,
  §4]: Nominal Adapton lets programs name computations so a changed run can
  match prior work. Its from-scratch-consistency theorem shows that "(mis)use of
  names cannot interfere with the meaning of programs; while a poor use of names
  may have negative impact on performance, it will not cause a program to
  compute the wrong result". The theorem rests on a check that detects one name
  used for two computations ("nominal ambiguity") and signals an error.

### 2.2 The rule [own, from the sources above]

The sources split cleanly. Kernel histories are *names*: they say which new
entity continues which old one. Only content says whether bytes can be reused.
OCCT's in-place path and Parasolid's caveat show that names can be wrong.
Nominal Adapton shows how names can be used safely: let them steer matching
and performance, and keep them out of correctness.

**Channel F (full).** The host submits the whole new model: a file, or an
in-memory record set. The host need not say anything else. The mesher decodes
it (L1, D0) and computes every key (G7 §5.2). Hits are reused; misses are
computed. This channel is self-checking: a host that lies about history cannot
cause a stale hit.

**Channel Δ (delta).** For an in-process host (animation, physics, a
complication added by owned feature operations), resubmitting and re-hashing
the whole model on every frame may cost too much (H5 measures it). The host
then supplies:

1. for every changed record, its new bytes: surface, curve, pcurve and point
   records, and the topology records that changed (loop order, coedge senses,
   seam pairing, face sense, use structure). A flag saying "face 42 modified"
   is not enough;
2. the IDs of deleted records;
3. created entities with their records;
4. changed occurrence transforms;
5. changed requests: global ε, the per-face tolerance map (G17 I1), flags,
   extra constraint curves (G17 I5);
6. optionally, history relations (modified, generated, deleted) between old and
   new entity IDs, for the correspondence output only.

The mesher patches its retained decoded store with items 1–3. It recomputes
the digests of the patched records only, and propagates dirtiness forward
through the retained task graph (Adapton-style change propagation). Channel Δ
is correct only if the delta is complete. That premise is tested, not trusted
(H6: channel-Δ output equals channel-F output on every generated edit). In
production a host that cannot guarantee a complete delta uses channel F.

**What the mesher derives itself**, under either channel:

- the set of tasks whose keys changed (§3), and from it the refined-edge
  neighbours, the halo and the faces to re-mesh. Hosts never name faces to
  re-mesh;
- early cutoff: a recomputed edge whose bytes are unchanged leaves its faces
  reused (G7 §5.2);
- each face's **disposition**: reused (key hit), re-meshed (miss, with a
  corresponding old face), new (miss, no correspondence) or deleted (old face
  with no new counterpart);
- the **correspondence** old → new, in priority order:
  1. equal host persistent identifier: a CAx-IF PID UUID (G16 §2.2), a
     Parasolid identifier (map S17: identifiers persist, tags do not), or our
     `EntityKey`;
  2. a host history relation (modified);
  3. equal content key, unique within the definition. A renumbered or
     re-exported file without UUIDs gets its correspondence this way;
  4. none.

  Two candidates for one entity is the nominal-ambiguity case: no
  correspondence is claimed, and the ambiguity is reported. The rule used for
  each match is recorded. Correspondence never changes a byte of mesh output.

Rigid and mirrored occurrence transforms change the placement table only
(D-placements; Parasolid's `incremental_transformation` option is the same
idea [src]). A uniform scale changes ε/s and so the definition's keys. Animation
and exploded views of the watch are therefore remesh-free. A flexing
hairspring changes its carriers every frame and re-meshes its faces every frame,
and D-provisional lets display show provisional meshes in between.

## 3. Question 2: the invalidation set and its halo

### 3.1 The task graph and each task's footprint

The table refines G7 §5.2's task list along M2's levels and G2's closure
stage. "Reads" is the complete input bundle: anything not listed is not
reachable from the task (G7 §5.3 item 1).

| Task (per item) | Reads | Coupling kind |
|---|---|---|
| T0 decode (record) | source bytes | none |
| T1 carrier (surface or curve) | own record | none |
| T2 measure (edge) | curve record and range; pcurves; T1 of each incident face; request | topological (incident faces) |
| T3 L1 solve (edge samples; full-rank vertex) | T2 of the edge, or of the vertex's incident edges | topological |
| T4 L2 vertex (rank-deficient) | T3 end samples of every incident edge | topological (vertex star) |
| T5 L3 edge | T3 of the edge; T3/T4 of both end vertices | topological |
| T5′ deferred rungs 1–4 (M2 §5.4) | T5; the arcs of the other trims of both faces (crossing check, rung 2); the ¼-lfs bound | topological (incident faces) + spatial (lfs) |
| T6 collapse cluster (G10) | T5/T5′ of the cluster's members; request (ε, r_c) | component |
| T7 thin-feature pair (G4) | T1 and bounds of the two faces; request | spatial (ε-inflated box query) |
| T8 closure (edge; G2) | T5/T5′ arc of the edge per occurrence; the arcs of every other feature of each incident face (D(s)); T1 of incident faces (M̄ for P4); request | topological (incident faces) |
| T9 face | face sense; ordered loop structure; T1; every T8 chain in loop order; T6 slice; T7 slice; request ε_F, flags, constraint curves | topological |
| T10 publish (definition) | every T9 output in canonical key order; identity binding | whole definition, D2 |
| T11 place (occurrence) | T10; transform | D2 |
| T12 export | T11; format | D2 |

Two choices in this table are [own] and go to M5:

1. **T8 reads source-derived arcs, not chains.** That is G2's purity
   requirement (G2 §3, "D(s) must be a lower bound computed from *source*
   arcs"). It is what keeps the graph acyclic and the halo finite. A closure
   that read neighbouring chains would form a cycle (e's chain ← f's chain ←
   e's chain).
2. **T8's key is static and coarse.** G2 computes D(s) by descending only the
   lens hierarchies the query needs, a *dynamic* read set. A key over a dynamic
   read set needs verifying traces that record what was read (Mokhov §2.2:
   dynamic dependencies complicate minimality). The simpler applicative key is
   the digest of *every* other feature's arc in each incident face. It
   recomputes the closure of every edge in a face whose arc set changed, and
   the chain digest then cuts off: an unchanged chain leaves every downstream
   face a hit. Downstream invalidation is therefore identical; only the cheap
   closure stage recomputes more (G2 H3 puts closure at ≤ 5% of the edge
   stage). H7 tests this; the dynamic variant is kept only if closure
   recompute shows up in the profile.

### 3.2 The rule

Build the task graph of M′ (D2, cheap) and compute keys bottom-up. A task runs
iff its key misses. Nothing else invalidates. This is G7 §5.2 applied to an
edit, and it gives Mokhov's minimality (Def. 2.1) up to early cutoff. A task
whose inputs changed is recomputed; one whose inputs' *bytes* did not change is
not, even when its upstream was recomputed.

The invalidation set I(E) is an output of the run, recorded per task kind. The
halo B(E) below is a static upper bound used for three things: scheduling
estimates, the H3 oracle (any task recomputed outside B(E) names a hidden
coupling), and the claim that edits stay local.

### 3.3 The topological halo [own, from the footprint table]

Let F₀ be the faces whose own records changed, together with the faces incident
to changed edge or vertex records. Write R₁ for the faces sharing a vertex with
F₀ (the vertex-adjacency ring) and R₂ for the faces sharing a vertex with R₁.

- **Resolve layer (T2–T5′).** A changed carrier of F ∈ F₀ changes T2 of F's
  edges, T3 of their samples and of F's vertices, and T4 of those vertices. A
  changed vertex changes T5 of *every* edge at that vertex, including edges of
  faces that touch F only at the vertex. So resolved arcs can change on edges
  of F₀ ∪ R₁ and nowhere else.
- **Closure layer (T8).** The chain of edge g changes only if its own arc
  changed, or if an arc of another feature in one of g's incident faces F′
  changed. Every changed arc lies on a face in F₀ ∪ R₁, so g lies on such a
  face F′ too.
- **Face layer (T9).** A face is re-meshed iff some chain in its loops, or its
  own record, slice or request, changed. Changed chains lie on edges of faces in
  F₀ ∪ R₁. The face on g's *other* side, F″, can lie in R₂.
- **Termination.** F″ is re-meshed, but F″'s re-meshing changes no arc, and T8
  never reads chains. So no third ring is reachable through topology.

So the topological halo is F₀ ∪ R₁ ∪ R₂ (faces), and the edges and vertices of
F₀ ∪ R₁ (resolve and closure). R₂ is reached only through a closure split
caused by a moved arc in a shared face. Without vertex moves (every V class V1
with unchanged bits), R₁ shrinks to the faces sharing an *edge* with F₀.

This agrees with the literature where they overlap, and is sharper:

- Parasolid's automatic method refacets "any faces adjacent to an edge that has
  been refined" [src FD §107.2.16]. That is the F″ case in our terms. Parasolid
  does not say whether moved vertices or near features also propagate.
- Sheffer and Üngör: changing a boundary entity removes "the mesh on that
  entity and all higher dimensional entities that rely on it" [src §1, p. 366].
  That is the dependents rule. Our table makes "rely on" explicit and adds the
  reads that are not containment (vertex stars, closure reads, spatial queries).
- Sypkens Smit frees two layers of fixed nodes adjacent to the changed region,
  after iterations 1 and 4 of the variational optimisation, because "not
  expanding the node set leads to a visible disparity between the fixed and
  free nodes … and it might even result in failure to properly represent the
  boundary" [src §6.6 pp. 103–105, §6.8 p. 111]. That halo exists because a
  variational mesher couples every node to its neighbours' quality. Our
  per-face construction has no such coupling. Both halos are two layers deep,
  but for different reasons, and ours is a bound derived from the read sets,
  not a tuning choice.
- G2 §4: "the faces on changed edges, plus the faces on any edge whose closure
  query read a changed feature". That is the closure part of the above. This
  lane adds the vertex-star propagation through M2 L2/L3, which G2 did not need.

### 3.4 Spatial and component couplings [own]

Three reads are not bounded by topology:

1. **T7 thin-feature pairs** (G4, design §5.6). The query finds face pairs
   whose ε-inflated bounds meet. Faces far apart in the topology can be close in
   space (gear teeth, a pallet and its wheel).
2. **The ¼-local-feature-size bound** that limits M2's transverse leaps (M2
   rung 2, adopted at G1). With G10's collapse-scale floor it is a 3D
   nearest-feature query over the definition.
3. **G10 collapse candidates**, found on source positions with margin r_c plus
   the measured discrepancy.

A spatial query certifies something about a region, including an *absence*
("no feature within r"). Recording the features read is not enough, because a
newly inserted feature near the item changes the answer without changing any
recorded dependency. So **each spatial query records its query region** (the
inflated box or ball) as part of its trace. The task is invalidated when any
created, deleted or changed entity's bounds, **old or new**, meet a recorded
region. The old bounds matter because a deletion can *raise* an lfs or remove a
thin pair, which changes a leap bound or loosens a local tolerance. The
check is a box-tree query of the changed entities' bounds against the
recorded regions: one query per changed entity.

**Collapse clusters (T6).** G10 defines clusters as union-find components over a
local predicate on resolved positions [src small-feature-collapse §4]. An edit
can merge or split components. Recompute every component that contains a
changed entity in the old model or in the new one. G10 also requires
collapse to be re-run when ε tightens, because eliminated features "may
reappear" [src G10 §4, Shephard 1998 §2]. In the key, ε does this
automatically.

**G2 arm B** (check-and-split rounds) is order-free only as synchronous rounds
over a definition (G2 §3). Its chain set is therefore a function of the whole
interacting component of the round graph, so its invalidation unit is that
component. This is one more reason arm A stays the default.

### 3.5 Couplings that would make the halo the whole model, and are forbidden [own]

1. **Run-global counters inside D1 outputs.** The draft issues vertex IDs when
   sampling edges (design §5.2 item 1). If a chain's bytes contain a global
   counter, inserting one edge renumbers every later chain, every chain digest
   changes, and early cutoff never fires. Rule: D1 outputs contain no run-global
   integers. A chain refers to its samples by (role, dyadic path) and to its
   ends by role (start, end vertex). A face refers to boundary vertices by
   (loop position, coedge, sample path) and to interior vertices by face-local
   index. Global numbering happens only in T10 (D2), from canonical keys. G2's
   sample key (edge + dyadic path) already has this shape. The edge itself
   enters through the identity binding, never through the key (G7 §5.2).
2. **Tie-breaks by entity number** (draft R4 "then lower ID"; M2's branch rule
   step 6 "by source identity"). They make geometry depend on numbering, and a
   renumbering then changes bytes. Break ties by content digest (G7 §2.2 item
   10).
3. **A per-run output-size guard that picks faces.** If the budget refuses
   "whichever faces exceed it", an edit anywhere changes which faces are
   refused elsewhere. The guard must refuse the request as a whole (G2 §2.3 stop
   case 4 already does this before refinement), or be a per-face budget derived
   from that face's own work estimate.
4. **Global placement of shared entities.** See §5.3. A placement optimizer over
   the whole model (HEFT from a cost model) moves an unchanged edge to another
   backend when an unrelated edit changes the load. Its bytes then change, and
   so do its faces'.

Two couplings already ruled out elsewhere: a global grading field (G14 §3 item
6 rejects it for this reason), and M2 branch step 5 ("least change in end
displacement relative to already-resolved neighbours"). Step 5 must name a
fixed level (L1/L2 outputs of the current run), never "whatever resolved
first", and never a previous run's result. Otherwise it is order-dependent
and carries history.

## 4. Question 3: reuse acceptance and the digests

### 4.1 Why OCCT's rule is unsafe [src, OCCT 8.0.1]

- **Faces.** `TriangulationConsistency` (`BRepMesh_ModelPreProcessor.cxx:46-98`)
  reuses a face's stored triangulation when `IsConsistent(deflection, required,
  allowDecrease)`, i.e. `current < 1.1·required` and, only with
  `AllowQualityDecrease`, `current > 0.9·required`
  (`BRepMesh_Deflection.cxx:158-168`, default ratio 0.1 at `.hxx:60-63`). The
  only other check is that triangle indices are in range (:74-86). The surface,
  the trims and the boundary are never compared.
- **Edges.** `BRepMesh_EdgeDiscret::process` reuses a 3D polygon or a polygon on
  triangulation under the same deflection test (`BRepMesh_EdgeDiscret.cxx:144-166,
  182-221`). Among the incident faces it picks "the most smooth discretization",
  the one with the smallest deflection (:110-133).
- **Outdating.** An edge that fails the test is `Outdated`. Its faces become
  `Outdated` (`BRepMesh_ModelHealer.cxx:346-349`), and the preprocessor then
  nullifies each outdated face's triangulation and every edge polygon on it
  (`BRepMesh_ModelPreProcessor.cxx:303-350`). Reused faces skip meshing
  (`BRepMesh_FaceDiscret.cxx:102`).

Consequences:

- A geometry edit that keeps the stored deflection value is reused stale
  (§2.1: `UpdateFace` is not seen).
- **Hysteresis.** With quality decrease off (the default) any finer mesh is
  kept for every coarser request. Request ε, then 10ε: the 10ε run keeps the ε
  mesh. A fresh 10ε run produces a different mesh. G7 §5.7 condition 2 names
  this band as the documented hysteresis case.
- Picking the smoothest stored polygon makes the result depend on which faces
  happened to be meshed earlier, at which deflection.

### 4.2 The rule [own, composed with G7 §5.2]

Reuse is a G7 key hit. Spelled out for the two task kinds the brief names:

- **Geometry digest** = digest of the task's own decoded source records, as
  raw IEEE bits, with no normalization (G7 §5.2). For a face: surface record,
  face sense, ordered loop structure.
- **Boundary digest** = for each coedge in loop order, the digest of the
  consumed chain's output bytes: resolved positions, edge parameters, dyadic
  paths, the UV for this occurrence (both for a seam), and the R- and V-class
  records that the face reads.
- plus the slices: the T6 collapse decisions for this face and the T7
  thin-feature result for this face. Each slice enters by the digest of what
  this face reads, not by the digest of the whole definition-wide task output.
  Otherwise one changed pair anywhere would miss every face.
- plus `request` (ε_F in definition units, every flag the face task reads, the
  speculative flag, constraint curves) and `backend_id`.

Nothing measured enters: no achieved deflection, no "close enough" band. A
coarser or finer request is a different key. Reusing a finer mesh for a
coarser request is a *request* decision ("serve the finest final mesh at or
below ε"). It would be legal as an explicit request mode, keyed as such, but it
is not the default and not a cache behaviour. This lane recommends against it:
it brings back OCCT's hysteresis in user-visible form.

**Sub-face reuse** (option, not default). Design §5.3 makes each band component
an independent triangulation, and G2 §4 lets a speculative face rebuild only the
band components that touch split segments. A band component could therefore be
its own task, keyed by its chain pieces and lattice rim, so that an edit to one
hole of a large face re-meshes one band. The lattice is not independent of the
trims (cells crossed by the trim are deleted; C4-aligned edges force lattice
levels, G2 §4), so the lattice stays per face. Adopt this only if the arena's
single-part latency case shows large faces dominating (parallel-growth-clusters
§6). It is correct whenever each sub-task is pure and keyed. mesh(K=1) ≡
mesh(K=k) (design §5.4) is the oracle of the same kind.

### 4.3 The edit-disposition record

Per face: disposition (reused / re-meshed / new / deleted), the correspondence
rule used (§2.2), old and new content keys. Per run: hit and miss counts per
task kind, the halo bound |B(E)| and the invalidation set |I(E)|. These are D2
functions of *two* runs and the identity binding. They are deterministic, but
they are not mesh output. They sit in their own record section, beside G11's
status tables, and are excluded from every equality oracle. Statuses (G11
refused / indeterminate / valid_goal_unmet / valid) are D1 values and are
included.

## 5. Question 4: what "incremental equals fresh" means, and its oracle

### 5.1 The statement

Let Inc(M′; S) be the output of an incremental run of M′ from cache state S,
and Fresh(M′) a cold run. "Output" means every D0–D2 layer (G7 §0 L1–L10),
records included, D3 and the edit-disposition section excluded.

- **E1, bytes per backend.** For the same `backend_id`, request and placement
  rule: Inc(M′; S) = Fresh(M′) byte for byte, for every reachable S. Reachable
  states include any earlier edit sequence, any eviction pattern and any
  thread count of the run that filled S.
- **E2, path independence.** For any edit sequence M₀ → M₁ → … → M_k, the run
  on M_k equals Fresh(M_k). In particular, undo restores the original bytes: M
  → M′ → M yields Fresh(M) with every task a hit, provided nothing was evicted.
  This is Hoppe's requirement that a selectively refined mesh be "unique,
  regardless of the sequence of (legal) transformations that leads to it" [src
  Hoppe 1997 §3], and Nominal Adapton's from-scratch consistency [src Hammer
  et al. 2015 §4, App. C].
- **E3, functional identity.** If the two runs differ in `backend_id`, or
  placement is load-reactive (§5.3), each passes the full validity stack on its
  own (G7 §3.1), and FI-1 to FI-5 (G7 §3.2) hold between them.

**Why E1 holds.** An incremental run differs from a fresh one only in that some
tasks return cached bytes. A cached value is returned only when its key
matches. Under G7 §5.3 a key match means the same inputs, the same code and the
same arithmetic, so recomputing would produce the same bytes. This is
Mokhov's correctness definition (Def. 3.1, via G7 §5.1) made per task, and
early cutoff preserves it because dependencies enter by output digest. Hammer
et al. prove the same property for a general language, under the condition
that names cannot alias [src §4]. Here keys are content, so aliasing is a
digest collision (SHA-256, G7 §5.3 item 4).

**Conditions E1 needs** (each is a design rule for M5):

1. **Pure tasks, complete keys.** G7 §5.3. Violations are what the oracle
   finds.
2. **No history in any task.**
   - No chain keeps samples requested by a face that no longer exists (G7 §5.7
     item 2). Under G2 arm A there is no request state at all: the chain is a
     function of the current arcs and carriers.
   - No reuse band (§4.1), and no "keep the finer mesh".
   - No collapse decided under an old ε (§3.4).
   - No branch choice relative to a previous run (§3.5).
   - No generation counters as cache keys. Today's retired coordinator retains a
     face when `boundary_generation` did not move
     (`native/mesher/adaptive_component.cpp:520-529`) [code]. That is correct
     within one run, and it is the template the audit names
     (owned-mesher-audit §6 item 5). Across runs the generation becomes a chain
     digest.
3. **No code path that patches.** Incremental is a fresh run with lookups.
   Kumar and Manocha's incremental refinement drifts from the uniform
   tessellation and must be "resynchronized", together "for all patches
   adjacent to the boundary" [src 1994 §6.1]. Sheffer–Üngör's copy-and-smooth
   and Sypkens Smit's node copying differ from fresh by design (§6). Any
   in-place patch must be shown equal to the fresh computation, or it is not
   used.
4. **No run-global integers in D1 outputs** (§3.5 item 1). Without this, E1
   still holds but early cutoff, and so locality, is lost.
5. **The speculative variant is provisional [own, correcting G7 §5.7 item 3].**
   G7 keys the speculative flag and states E1 per variant. But *whether* a
   given face was meshed speculatively depends on whether its R2 chain settled
   before the face was scheduled, and that is timing. If the speculative bytes
   could be final, the final bytes would be D3. So either speculation is a
   content rule (fixed classes of items always mesh against the provisional
   chain, independent of timing), or the speculative mesh is D3 provisional and
   the final face is always meshed against the final chain. The second matches
   D-provisional and keeps one byte stream per request. It is recommended.
6. **Placement is a function of content**, or both runs share one placement map
   (§5.3).

### 5.2 Display between edit and remesh [own]

Final meshes are what E1 is about. Between an edit and the completion of its
re-meshing, a display consumer may show something (D-provisional: display
only). The hazard is a crack: an old face mesh beside a neighbour that already
uses a new chain. Kumar and Manocha retessellate a boundary curve "on all
adjacent patches when its associated patch is resynchronized" [src §6.1].
G18 and the map's S24 swap per edge-closed region. The rule here:

- the invalidation set's faces form components linked by shared changed
  chains;
- each component swaps atomically, all old to all new;
- until then the old meshes of that component stay on display, marked stale.

Physics never sees stale or provisional meshes (D-provisional; G17 I7).
Sharpen-in-place is a sequence of request edits (ε_F decreasing). G18's nesting
gives every fine chain sample an ancestor for geomorphs. E2 makes the final
sharpened mesh equal to a fresh mesh at the final ε: by purity, not because of
the nesting.

### 5.3 Mixed backends: rules under both answers to G7's open question

G7 §9 Q1 asks whether a mixed-backend run must be byte-reproducible run to run
(fixed placement) or may react to load. Not settled here. The rules for each
answer:

**(a) Fixed placement.** The producer of each task is a deterministic function
of the run configuration and the device inventory.

- E1 holds for mixed runs. O-bytes compares Inc and Fresh with the same
  placement rule.
- **Locality requires the rule to be per item:** a function of the task's own
  content key, kind and cost estimate, not of the whole model's load. If a
  global optimizer (HEFT over a cost model, map S18) chooses placement, an
  unrelated edit can move an unchanged edge to another backend. The fresh run
  of M′ does the same, so E1 still holds, but the edge's bytes change, its
  faces miss, and the halo becomes the model. So under (a): per-item placement
  for shared entities (vertices, chains, carriers) at least. Faces may be
  placed globally only if their own bytes are allowed to change with
  unrelated edits, which defeats reuse. This trade-off belongs to G8: global
  scheduling quality against edit locality.
- Keys carry the producer's `backend_id` (G7 §5.2). Cross-backend reuse stays
  off (G7 §5.4).

**(b) Load-reactive placement.** The producer is chosen at run time.

- The bytes of a run are a function of (M′, placement log). The placement log
  (task key → `backend_id`) becomes a D2 record of the run. It is no longer
  D3, because the bytes depend on it.
- **Lookup** tries the task's key under each `backend_id` in the run's
  inventory, in a fixed order. The first hit fixes that task's placement and is
  written to the log. Misses are placed by load. A reused chain produced on the
  GPU and consumed by a face recomputed on the CPU is a legal mixed run (G7 §4:
  consumers read bytes by ID). A Frankenbuild stays impossible: a reused face's
  key contains its chains' output digests (G7 §5.2).
- **Early cutoff needs sticky placement.** A recomputed task with unchanged
  inputs reproduces its old bytes only on the same backend. A load-reactive
  scheduler should therefore prefer the backend that produced the task's
  previous version, which the identity binding can supply. This is history, but
  under (b) bytes already depend on the log. It affects reuse, never validity.
- **Equivalence.** E1 becomes "Inc(M′; S) = Fresh(M′) replayed with Inc's
  placement log", byte for byte (O-bytes-replay), plus E3 against any
  unconstrained fresh run. Cache transparency (G7 §5.6) becomes FI-level
  across runs too.

Either way the one-producer rule (G7 §4) is unchanged. An edit never makes two
backends produce one entity in one run.

### 5.4 The oracle

**Form.** Metamorphic (tier 4): two computations that must agree, the cold one
taking no cached path. It is G7's O-bytes arm "incremental edit versus fresh
run of the edited input" and G7's key-completeness test, driven by an edit
generator. It is one oracle, not two (BLACKBIRD tenet 5).

**Procedure** for each base model M (the corpus, and G2's and G10's authored
families) and each generated edit E:

1. Run M cold and keep the cache.
2. Apply E (channel F, and separately channel Δ for H6) and run M′ from that
   cache: Inc.
3. Run M′ cold in a new process with an empty cache: Fresh.
4. Compare all D0–D2 bytes. Any difference fails, and names the first
   differing task kind and key.
5. Check the halo: every recomputed task lies in B(E) (H3). A recompute
   outside it fails and names the coupling.
6. Non-vacuity, so the test can fail: Inc must report at least one hit and at
   least one miss. An edit with zero hits compares two fresh runs, which G7's
   N5 forbids (it compares an artifact with itself); an edit with zero misses
   tests nothing about invalidation. The test fails the case instead of
   passing it.

**Perturbations**, crossed with the above: thread counts and launch shapes as in
G7 §3.2 (reported as actually used, N4); random cache eviction before step 2;
for mixed runs, the placement seeds of G7 H3.

**Edit generator** (independent of the watch):

| Class | Example | Expected disposition |
|---|---|---|
| identity only | renumber entities; rename file; re-export | every task hits; bytes equal (G7 minimality) |
| transform only | rigid or mirrored occurrence move | no T1–T9 miss; T11 only |
| scale | uniform scale of one occurrence | new definition keys at ε/s |
| request | global ε; one face's ε_F (G17 I1); one flag | global: all miss; per face: F and faces on chains whose count changed |
| carrier, interior | B-spline interior control point by 1 ulp and by 10% | F; neighbours only where an edge's sample count, class or a closure split changed |
| carrier, boundary | plane offset; cylinder radius | F₀ ∪ R₁ at most; R₂ only through closure splits |
| vertex | move one vertex by < β and by > β | vertex star; V-class changes |
| pcurve / parametrization | replace a pcurve; reparametrize a curve | the edge's chain and its faces |
| topology | add or remove a hole; split a face; delete a face; fillet an edge | new and deleted faces; R₁ |
| spatial | move a face to within 2ε of another, then away (two edits) | T7 pairs both ways; old bounds invalidate (§3.4) |
| collapse | shorten an edge below r_c, then lengthen it | cluster merge and split |
| sequence | 10 random edits, then undo all | E2: final bytes = Fresh(M₀), all hits |

The generator reuses the STEP and B-rep mutation fuzzer the journal accepted for
coverage, and our own feature operations (extrusion, bore, fillet), which emit
real topology-changing edits with `EntityKey` history. Watch edits may appear
as one family among these, never as the only one.

**Fault classes it catches:** missing key inputs; identity fragments in keys;
history in tasks; non-local reads not in the footprint table; run-global
counters (as an H3 or reuse failure); delta incompleteness (H6). **What it does
not catch:** a geometry fault present in both runs. Both runs execute the same
task code, so the per-backend validity stack (G7 §3.1, design §10) remains the
oracle for geometry. The correspondence output has its own direct check: every
face reported "reused" has face-local bytes identical to its old face.

**Comparisons that cannot fail and must not be used:**

- comparing Inc against a "fresh" run that read the same cache (G7 N5);
- edits that miss nothing, or hit nothing (step 6);
- face counts or triangle counts instead of bytes;
- Hausdorff between Inc and Fresh under ε. Both runs pass the deviation
  oracle, so that comparison is implied (G7 N1).

## 6. Question 5: the literature, and what each source contributes

| Source | Mechanism | Invalidation | Reuse test | Incremental = fresh? |
|---|---|---|---|---|
| Sypkens Smit 2011 §3.3, ch. 6 [src] | Feature difference model; copy nodes of persistent-identical volume; refill new cells; VTM on free nodes | Free nodes near "active faces" plus two layers grown after iterations 1 and 4 | Feature correspondence (host feature model) | No: "the quality of the regular meshing is slightly better … [one case] the opposite" (§6.7); runtime 10–45% of fresh |
| Sheffer–Üngör 2001 §1–§5 [src] | Parametric (dual) model maps entities; copy mesh, smooth, adjust connectivity | Most systems remove an entity's mesh "and all higher dimensional entities that rely on it" | Re-check that suppression criteria "still apply"; topology changes excluded | No (morphing); a suppressed feature may need re-editing |
| Parasolid V35 FD §107.2.16, §99, §97 [src] | Bulletin board or pmarks give changed entities; incremental faceting | Modified faces plus faces adjacent to refined edges (auto); attribute deleted on change | Presence of the faceting attribute | Not claimed; changed facet controls "may not be honoured" |
| OCCT 8.0.1 BRepMesh [src] | Reuse stored triangulations and polygons | Outdated edge outdates its faces and their edge polygons | Deflection within 1.1× (0.9×) | No: hysteresis; geometry edits invisible |
| OCCT `BRepTools_History` [src] | Generated / Modified / Removed for V, E, F, solid | — | — | — (names, not content) |
| Kumar–Manocha 1994 §4.5, §6 [src] | Frame-to-frame point reuse; incremental tracing and triangulation | Changed intervals; v/u-lines | Interval bound tests | No: drift, periodic resynchronization of all patches sharing a boundary |
| Hoppe 1997 §3 [src] | Selective refinement with legality preconditions | Vertex hierarchy dependencies | — | Required: the refined mesh is unique regardless of the transformation sequence |
| Mokhov et al. 2018 §2–§4, §6 [src, via G7 and re-read §2] | Build systems: traces, early cutoff | Transitive dependents of changed inputs (minimality) | Key or trace match | Correctness Def. 3.1 = from-scratch equality |
| Hammer et al. 2015 (Nominal Adapton) [src] | Names for memo matching; change propagation | Dirty edges in the dependence graph | Name match plus clean dependencies | Proven (from-scratch consistency); names affect only performance, given the ambiguity check |
| Wu–Sarma 2001 [src, abstract] | Incremental B-rep exchange by extraction and embedding | — | — | — |
| Wu–Sarma 2004 | Incremental editing of faceted models | not read (closed access) | | |
| Mezentsev–Woehler 1999 [src, abstract, §1–§2] | CAD repair for "incremental" surface meshing | — | — | Not bearing: "incremental" there names the meshing algorithm ("incremental algorithms of the surface meshing (for example, Delaunay method)", §2), not edits |

What the meshing sources share: each patches a previous mesh, and each pays
for it in equivalence. What the computation sources share: each recomputes
through memoization and gets equivalence by construction. This design is in
the second group. The cost moves from patching to hashing and lookups (H5).

## 7. Hypotheses and falsifiers

| # | Hypothesis | Measurement | Falsifier |
|---|---|---|---|
| H1 | E1: Inc = Fresh bytes per backend | §5.4 over the corpus and families × edit generator × perturbations | Any byte difference |
| H2 | E2: edit sequences end at Fresh; undo is all hits | Sequence class of the generator | Any byte difference; any miss on undo without eviction |
| H3 | The recomputed set lies inside the static halo B(E) | Per-edit task logs against B(E) | Any recompute outside B(E) |
| H4 | Edits stay local in practice: for single-face carrier edits the median re-meshed faces ≤ 1 + (faces sharing an edge with F); identity-only and rigid-transform edits re-mesh nothing | Disposition records over the generator | Any re-meshed face for identity-only or rigid edits; the median above the bound |
| H5 | Key overhead is small: computing every key of M′ (channel F, decode excluded) costs ≤ 2% of a fresh mesh of M′ | Arena timing per channel with lane context (common brief) | Above 2% on any corpus class; then channel Δ is required for interactive hosts |
| H6 | Channel Δ = channel F | Same edits through both channels | Any byte difference (an incomplete delta or a propagation bug) |
| H7 | The static T8 key costs ≤ 2× the closure recomputes of a dynamic read-set key, with identical downstream invalidation | Closure task counts, both arms, on the generator | Above 2×, or any downstream difference |
| H8 | Incremental remesh time scales with \|I(E)\|, not with model size, for channel Δ | Time against \|I(E)\| and model size, single-part edits | Time grows with model size at fixed \|I(E)\| |

## 8. What the literature does not say, and the measurement that would

1. **No published tessellator claims incremental = fresh.** None read states the
   property; the meshing sources read report the opposite (§6). G7 §7 item 4
   found no published content-keyed tessellation cache either. The derivation
   here is ours; H1 and H2 test it.
2. **Halo sizes on real CAD.** No source measures how far an edit's
   re-meshing reaches in B-rep models. Measurement: H3 and H4 over the edit
   generator on the corpus, after M8.
3. **Hashing cost against meshing cost.** Not in the literature for meshers.
   Bazel and Nix pay it for builds (G7 §5.1). Measurement: H5.
4. **How complete kernel histories are.** Parasolid warns of "unexpected
   entries"; OCCT omits wires, shells and geometry. No source quantifies missed
   or spurious entries. It only matters for correspondence, never for bytes,
   under Rule C. Measurement: compare host history against content-key
   correspondence on our feature-operation edits (expected agreement: every
   retained `EntityKey` with an unchanged content key).
5. **Wu–Sarma 2004** may contain a faceted-model delta scheme relevant to
   channel Δ. Not obtained (shopping list).

## 9. Consequences for M5

Design draft statements to change:

- §7 "Settling or editing an edge invalidates only its incident faces": replace
  it with Rule I. The incident faces are necessary but not sufficient: vertex
  stars, closure reads, spatial queries and collapse components also
  propagate (§3.3–§3.4; map S16 contradiction (a)).
- §5.1 "Cache keyed by source identity": key by content (G7 §5.2), with identity
  only in the correspondence output.
- §5.2 item 1 "Vertex IDs are issued here": no run-global IDs in D1 outputs;
  number at publication (§3.5 item 1).
- §5.5 output-size refusal: whole-request or per-face, never "pick faces
  across the run" (§3.5 item 3).
- §7 speculative variant: provisional (D3) unless the speculate/not decision is
  a content rule (§5.1 condition 5).
- §10 "fresh versus incremental remesh equivalence": the §5.4 oracle, with
  non-vacuity and the edit generator; one oracle with G7's completeness test.
- M2 branch-choice step 5 names a fixed level, not "already-resolved" (§3.5).
- New: the T8 closure reads source-derived arcs and takes a static key (§3.1);
  spatial queries record their regions (§3.4); the edit-disposition record
  (§4.3); the display swap per invalidation component (§5.2); channel F as the
  default host interface and channel Δ for trusted in-process hosts (§2.2).

## 10. Return items

### The four rules

Rules C, I, R and E as stated in §0, with the halo derivation in §3.3–§3.5,
the digests in §4.2, the equivalence statement in §5.1, the mixed-backend
rules in §5.3, and the oracle in §5.4.

### Defects found

None in current code. Read and judged not defects:

- `native/mesher/adaptive_component.cpp:520-529` retains faces by
  `boundary_generation`, a run-local counter. That is correct within one run,
  and the code is retired. The replacement is a chain digest, noted for M5
  (§5.1 condition 2).
- Design-draft contradictions listed in §9 are design errata for M5, not code
  defects.

Upstream, for the record and not for beads: OCCT `BRepMesh` never reads a
shape's `Modified()` flag, so `BRep_Builder::UpdateFace` with a new surface
keeps a stale triangulation (`BRep_Builder.cxx:564-578`; no reader in
`TKMesh`).

### Operator questions

- None new. G7 §9 Q1 (fixed versus load-reactive mixed-backend placement) bears
  on this lane. It stays open. Rules for both answers are in §5.3. A related
  trade-off goes to G8 rather than the operator: global placement optimization
  against edit locality (§5.3 (a)).

### Shopping items

- Wu D., Sarma R., "The incremental editing of faceted models in an integrated
  design environment", Computer-Aided Design 36(9):821–833, 2004,
  doi:10.1016/j.cad.2003.09.009. Closed access (Unpaywall: no open copy).
- Wu D., Sarma R., "Dynamic segmentation and incremental editing of boundary
  representations in a collaborative design environment", ACM SMA 2001,
  pp. 289–300, doi:10.1145/376957.376990; JCISE 1(4):320–329, 2001,
  doi:10.1115/1.1433485. Abstract only was read.

## For SHOPPING_LIST.md

- Wu D., Sarma R., *The incremental editing of faceted models in an integrated
  design environment*, CAD 36(9):821–833, 2004, doi:10.1016/j.cad.2003.09.009
  (Elsevier, closed; wanted for channel Δ over faceted models).
- Wu D., Sarma R., *Dynamic segmentation and incremental editing of boundary
  representations in a collaborative design environment*, ACM SMA 2001,
  doi:10.1145/376957.376990, or JCISE 1(4), 2001, doi:10.1115/1.1433485
  (ACM/ASME, closed; abstract read; wanted for the B-rep delta format).
- Already on the list from M1 and still relevant to S16: François–Cuillière
  2000; Cuillière et al. 2009/2011; Raghothama–Shapiro 1997/1998; Kripac
  1995/1997. Not repeated here.
