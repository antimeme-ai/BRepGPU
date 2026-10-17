# Owned mesher audit: where the 847.7 s goes

Read-only lane of the fast-mesher study (branch `research/fast-mesher-study`).
Nothing was built, run, profiled or benchmarked. The inputs are the code under
`native/mesher/`, `native/direct_cdt/`, `native/face_domain/`, the design
documents, and data that was already recorded:

- `results/om10-watch-2026-10-17/{cpu12,p1001}-phases.tsv` and `timings.tsv`.
- `context/adaptive-watch/watch-{cpu12,p1001}-attempt8/queries.tsv` and
  `faces.tsv`. These hold the per-definition counts. Their `timings.tsv` files
  are byte-identical to the published phase files, checked with `diff`.
- `results/om10-nvidia-complete-2026-10-17/heal0/{producer-stages,physical}.tsv`
  for per-definition NVIDIA tessellate times and triangle counts.
- `results/adaptive-trimmed-2026-10-17/queries.tsv`, which records the only
  per-phase face timings that exist (authored faces at revision `4b67f3d`).
- `context/adaptive-watch/attempt2-stage-stack.log`: one earlier-binary gdb
  sample. It is used only as a check that agrees with the other evidence.

Each conclusion is tagged with its confidence:

- **[code]**: read in the code, with file:line.
- **[data]**: measured in recorded data.
- **[inf]**: inferred from code plus data, not directly measured.

Section 5 lists the fleet measurements that would settle each [inf].

## 0. Findings that bear on the design

1. **The gap is two multipliers.** [data]
   - Owned meshing used about 1,578 CPU-seconds. That is the process user time
     of 1,605.28 s minus about 27 s of non-meshing wall time. NVIDIA's
     per-definition `tessellate` rows sum to 22.86 s. Per unit of work, owned
     costs about **69× more**.
   - Owned meshing kept on average about **1.86 of 12 cores** busy
     (1,578 / 847.7).
   - Per output triangle: owned takes 3.09 ms of wall time per facet. NVIDIA
     takes 0.099 ms of single-thread time per triangle, which is **31×** less.
     Owned emits 1.18× NVIDIA's triangle count overall, and 0.61–3.91× per
     definition.
2. **Perfect parallelism of today's code is still far too slow.** [data+inf]
   Packing the current work perfectly onto 12 cores gives 70–131 s. That is
   11–21× NVIDIA's 6.13 s batch. Running definitions concurrently is bounded
   below by definition 1713's 139 s. Per-facet work has to fall by more than
   an order of magnitude whatever the scheduling.
3. **The three stages are well predicted by three counts.** [data]
   - **faces**: 541.1 s (63.8%). About 2–3 ms per output facet; the slope
     differs by definition.
   - **contacts**: 164.9 s (19.5%). 64–67 µs per candidate pair (r=0.977).
   - **boundary**: 139.9 s (16.5%). About 0.30 ms of wall time per evaluated
     curve point (R²=0.976).

   None of these per-unit costs is set by evaluating geometry. Surface
   evaluation is less than one device point per facet watch-wide.
4. **contacts constructs nothing.** [code+data] It is a whole-component
   self-intersection check sitting in the hot path. It found 0 forbidden pairs
   in all 107 definitions. Its floating-point filter cannot certify one common
   case: coplanar neighbours that share only a vertex. That case falls to
   `Rational` exact classification.
5. **Most of the time goes to certification and exactness.** [inf, wide band]
   My best estimate is about 65–90% of wall time (central about 75%):
   certification, proof and exact-keying work. Essential construction takes
   about 8–34% (central about 20%), and directly separable infrastructure about
   1%. Section 3 gives the derivation.
6. **The P100 changes nothing in meshing.** [data] P100 total is 845.8 s against
   847.7 s on CPU. Device work watch-wide: 465,553 curve points, 215,809
   surface points, 229 cap facets. Host→device traffic is about 2.7 KB per
   surface point.
7. **Why the cores idle.** [code] Three mechanisms:
   - Definitions run one at a time.
   - Faces advance in barrier-synchronised waves of at most 12 single-thread
     steps.
   - The boundary stage and contact preparation run on one coordinator thread.

   Every exact value is a heap `Rational`. Each allocation does two atomic
   compare-and-swaps on a shared run-wide counter, and every read of a stored
   value runs a GCD.

## 1. Per-component data flow

### Before the component loop

`adaptive_model_main.cpp`, once per process:

1. **Decode.** `load_source` reads the `.bgsource` snapshot into `SourceStore`
   (0.19 s) [code `adaptive_model_main.cpp:229-232`; data `timings.tsv`].
2. **Envelopes and assembly allowances.** `prepare_adaptive_assembly` computes
   exact per-definition envelopes and positional/node allowances that include
   the composed placement transforms (9.77 s, serial) [code `:236-243`,
   `adaptive_assembly.cpp`].
3. **Bank compilation.** One quotient-trim surface `SourceCache` covers all
   needed carriers, plus one `CurveSourceCache` per definition (1.37 s)
   [code `:270-306`].
4. **Services.** One `NativeScheduler` with 12 threads and task_capacity 12
   [code `:329-330`, `scheduler.cpp:29-52`]. One `ComponentMesher` keeps a pool
   of `FaceTrimSession` owners [code `adaptive_component.cpp:300-332`].
5. **Serial loop.** Definitions are meshed one after another in source-ID
   order [code `adaptive_model_main.cpp:399-450`]. `mesh()` requires
   `scheduler.idle()` [code `adaptive_component.cpp:369`], so only one
   component is ever active.

`complete_meshing` is the sum of the `complete_component` walls [code `:415`,
`:453`]. The per-definition rows add up exactly to 847.712 s [data].

### Inside one component

`ComponentMesher::mesh`, `adaptive_component.cpp:359-560`:

**owner_setup** (0.009 s total on CPU, 0.44 s on P100) [data]. Grows the
FaceTrimSession pool to `min(12, faces, task_capacity)` owners. On P100 each
owner also opens a device session [code `:401-409`].

**boundary_setup + boundary** (139.9 s) [data]. The code builds an
`AdaptiveBoundarySession` per component [code `:411-415`], with its own
12-thread `NativeScheduler` [code `adaptive_curves.cpp:386-389`]. The
coordinator then busy-loops `boundary.step()` with `std::this_thread::yield()`
until the session finishes [code `adaptive_component.cpp:419-423`]. The step
state machine is at `adaptive_curves.cpp:1766-1862`:

1. *evaluate / restrictions / prepare_horizon.* Each original edge curve is
   held as a binary forest of intervals with **Rational** endpoints
   (`CachedCurveNode{Rational begin,end}`, `adaptive_curves.cpp:73-80`).
   `prepare_horizon` walks **every** edge of the definition on every epoch and
   queues endpoint samples [code `:693-735`]. The shared-edge authority is
   `DefinitionBoundaryForest` [code `boundary.hpp`]. It holds physical node
   keys, per-edge interval keys, root-relative sample path bits, vertex aliases
   and per-face boundary generations.
2. *check_source_guards*, once per session. Each original vertex must lie
   within the node allowance of its curve endpoint. The check is an interval
   test, then a `Rational` squared distance, then precise curve evaluation at
   up to 4 arithmetic stages [code `:736-817`]. This validates the source; it
   does not construct anything.
3. *nodes / node_fold.* Interval chord-deviation bounds per live interval run
   as kernels on the 12 workers, partitioned by degree² cost
   [code `:565-600`, `:469-532`]. The fold runs on the coordinator. Every
   acceptance converts the double upper bound with `Rational::from_double` and
   compares it with the Rational allowance [code `:885-891`]. Near-threshold
   cases get `bound_curve_node_precise` [code `:912-934`]. Precision promotions
   re-evaluate points at higher stages [code `:943-964`].
4. *commit.* All failing intervals are bisected at Rational midpoints. A forest
   epoch opens, every face of the definition "arrives", and the loop returns
   to evaluate [code `:1700-1764`]. `faces.tsv` records boundary epochs of 4–8
   per definition; 1713 has 7 [data].
5. *build_physical.* Writes a `PhysicalCurveInterval` row per interval with
   exact original parameters, a chord upper bound and the selected
   parameter-versus-image correspondence. Line images get exact curve scalars
   [code `:993-1059`].
6. *trace_source / carriers / trace_bounds / fold_traces.* For each face whose
   boundary generation changed, `prepare_use_trace` maps the shared chain into
   that face's UV chart. **The per-face loop runs serially on the
   coordinator** [code `:1395-1430`]. Carrier samples, precise fallbacks and
   affine-trace bounds go to the workers [code `:510-529`]. A failed trace
   bound sends the loop back to *commit*.
7. *freeze.* Every exact value is copied through `import_exact` into a new
   `ExactBuilder` table. The resulting `SharedCurveSnapshot` carries uses,
   sites, intervals and seam wedges [code `:1525-1640`].

In short, the boundary stage samples every source edge once, adaptively, for
all incident faces. It certifies each chord against the edge allowance and
produces each face's UV loop sites, with the identity of their physical
nodes.

**faces** (541.1 s) [data]. Each face owner gets a `FaceStep`, a
`NativeTask` [code `adaptive_component.cpp:30-44`]. A `wave` submits up to 12
steps, calls `drain()` (a full barrier), sorts the completions by key and
charges the work [code `:65-95`]. Ready steps are resubmitted; finished faces
hand their slot to the next pending face [code `:477-507`]. One
`FaceTrimSession::step` advances a single phase of a per-face state machine
[code `adaptive_trimmed.cpp:692-991`]. Surface work runs inline because the
coordinator forces `limits.face.surface.cpu_threads = 1`
[code `adaptive_component.cpp:324`]. Workers exist only when
`cpu_threads > 1` [code `adaptive_cache_detail.hpp:252-255`]. The phases:

1. *domain.* `prepare_face_domain` builds the face's UV boundary loops from
   the shared traces. Along the way it checks: each original coedge used
   exactly once; joint occurrence identity; equal settled joint UV, physical
   key and bit-identical XYZ at loop joints; wedge sector bindings
   [code `adaptive_regions.cpp:298-420`]. Quotient (cap) faces first run
   `prepare_cap_domain` [code `adaptive_trimmed.cpp:728-741`]. A
   `FaceDomainClassifier` is then built [code `:771-776`].
2. *start / select.* The adaptive surface cache subdivides carrier roots into
   leaves using interval bounds. Classification calls go back to the face
   classifier [code `:800-810`].
3. *cuts / anchors.* Points are sampled for cut plans and band anchors
   [code `:806-857`].
4. *construct.* `build_packet_mesh` builds the interior packets from
   wholly-interior leaves, as a zipper across hanging sides
   [code `adaptive_packets.cpp`, no `Rational` at all]. Then
   `build_face_regions` or `rebuild_face_regions` triangulates the connected
   boundary regions with the owned direct CDT
   [code `adaptive_trimmed.cpp:887-940`]. Sites are exact Rational UV. Each
   region is cleared to a common denominator and solved with integer
   predicates behind an interval filter
   [code `adaptive_region_cdt.cpp:131-186`, `direct_cdt/predicates.cpp`].
5. *bound / cap_bound.* `prepare_trim_facet_candidates` handles every changed
   facet [code `adaptive_trimmed_geometry.cpp:274-545`]:
   - Rational UV is read through `import_exact`; each read runs a GCD
     [code `exact_store.cpp:60-63`].
   - Rational orientation is computed [code `:398`], and each vertex's carrier
     root is located.
   - Plane facets get 12 Rational envelope comparisons.
   - Curved facets that fall back are clipped in Rational arithmetic against
     every overlapping leaf, emitting 15 exact entries per piece
     [code `:488-538`].

   Next come batched interval bounds. On CPU they run inline in this face's
   step; on P100 they are submitted to the device and polled. Every acceptance
   again compares a double upper bound against the Rational allowance
   [code `adaptive_surface_cache.cpp:183-185`]. Ambiguous cases get exact
   precise comparisons [code `:265-279`] or materialised Rational clip
   fallbacks [code `:328-365`].
6. *finish_horizon.* Each facet's accepted upper bound is re-imported and
   compared with tau [code `adaptive_trimmed.cpp:408-505`]. If every facet
   passes, the face is complete. Otherwise the failing facets either request
   precision floors or get a refinement plan
   (`plan_construction_refinement`). The plan can request new boundary
   intervals, which yields `needs_boundary`, or new anchors and leaf splits.
   The state machine then goes back to *select* and runs another full
   construct → bound horizon.

So the faces stage triangulates each face and certifies each emitted facet's
positional error against the original source. When a certificate fails, it
refines and rebuilds.

**Boundary rounds.** If any face asked for boundary refinement, the component
refines the shared boundary, settles it again, and resets every face whose
boundary generation moved [code `adaptive_component.cpp:509-531`]. Watch-wide
this happened 3 times (definitions 1731, 1755, 1798, one round each). There
were 9,169 assignments for 9,162 faces and 186 retained faces, so rework from
rounds is negligible [data].

**contacts** (164.9 s) [data]. `check_contacts` [code
`adaptive_component.cpp:127-273`] runs these steps:

1. Serial: key every face site to a physical node and sort; same key must mean
   bit-identical XYZ [code `:134-170`].
2. Serial: per face, sort the source-edge provenance [code `:178-240`].
3. Serial: `ContactEpochOwner::prepare` builds or updates a `FacetBVH`, queries
   it once per facet, freezes the candidate pairs and sorts them
   [code `contact_epoch.cpp:486-586`]. The BVH's SAH cost comparison falls back
   to `Rational` areas when its interval filter is ambiguous
   [code `contact.cpp:706-740`].
4. Parallel waves: up to 12 batches of 256 pairs each run
   `classify_contact_batch` [code `adaptive_component.cpp:243-265`,
   `contact_epoch.cpp:60-73`]. Each pair goes through an interval
   separation/shared-edge/shared-vertex/projection/SAT filter
   [code `contact.cpp:400-476`]. If unresolved, it falls to `classify_exact`
   with `Rational` plane cuts or coplanar polygon clipping
   [code `contact.cpp:237-267`].
5. Any forbidden pair throws [code `adaptive_component.cpp:266-271`].

This is a verification pass over the finished mesh. All 107 components
reported 2,218,277 candidate pairs and 0 forbidden [data].

**boundary_close, output.** Session close (0.03 s total). `write_component`
writes the BGACRV/BGATRM artifacts and the `faces.tsv` rows (2.76 s, outside
`complete_meshing`) [code `adaptive_model_main.cpp:441-446`; data].

**After the loop.** `make_adaptive_scene` takes 12.09 s, serial. Then come
presentation, display and PNG [code `:458-520`; data].

## 2. Attributing the 847.7 s

### Stage totals (CPU12; P100 within 1.5% on every stage) [data]

| Stage | CPU12 s | share | P1001 s |
|---|---:|---:|---:|
| faces | 541.108 | 63.8% | 541.014 |
| contacts | 164.948 | 19.5% | 163.963 |
| boundary (incl. refine rounds) | 139.937 | 16.5% | 138.326 |
| boundary_setup + owner_setup + boundary_close | 0.23 | <0.1% | 0.93 |
| complete_component sum | **847.712** | | **845.785** |

### Count totals for the watch [data]

- 9,162 faces and 273,978 facets (29.9 per face).
- 2,218,277 contact candidate pairs, 8.10 per facet. The per-definition range
  is 6.43–11.51, which points to mesh-adjacent neighbours [inf].
- 465,553 curve device points and 420,028 curve bounds.
- 215,809 surface device points and 169,649 surface bounds (per-definition
  deltas of the cumulative P100 columns, which are monotone).
- 229 cap facet tests and 11 domain replacements, all in definitions 1717,
  1731, 1755 and 1798.
- Reservation-inclusive run peak 7.23 → 8.36 GB; process peak RSS 1.99 GB.

### Cost model fitted over the 107 definitions [data]

Each stage is fitted against its count, on CPU12 wall time:

| Stage | Model | Fit |
|---|---|---|
| contacts | 64–67 µs × candidate pairs | r=0.977 (R²=0.955) |
| boundary | 0.30 ms × curve points (0.38 ms/pt − 3.8 ms/face with a face term) | R²=0.976 (0.987) |
| faces | 3.13 ms × facets, all definitions | R²=0.85 |
| faces | 2.1 ms × facets, excluding 1713 | R²=0.45 |
| total | ≈3.3 ms × facets | r=0.950 |

Per-definition face cost runs from 0.53 to 11.4 ms per facet. The worst are:

| Definition | ms/facet |
|---|---:|
| 1731 | 11.4 |
| 1762 | 6.1 |
| 1717 | 5.0 |
| 1755 | 4.2 |
| 1748 | 4.2 |
| 1764 | 4.2 |

All four definitions with mapped analytic caps (1717, 1731, 1755, 1798) also
have cap-driven domain replacements. Three of them (1731, 1755, 1798) are the
only definitions with boundary rounds, and they sit among the largest
positive residuals. 1731 alone has a +22.9 s residual. Together these four
take about 70 s [data]. The facet count is a weak predictor outside 1713
because the cost per facet depends on what kind of face it is. The recorded
data has no surface family per face, so this cannot be separated further [inf].

Even the "unit costs" are huge [data+inf]:

- 0.30 ms of wall time per curve point. Evaluating a point is about 0.1 µs.
- 64 µs per candidate pair. A floating-point triangle–triangle test is about
  0.1 µs.
- 2–3 ms per facet. NVIDIA's whole tessellation costs 0.099 ms per triangle.

The time therefore goes to the work around evaluation, which Section 3
classifies.

### Why 1713 takes 139.3 s [data+inf]

1713 is the main plate, "Platine brute" (`labels.tsv`). It is the largest
definition by every count:

| Count | 1713 | Share of watch |
|---|---:|---:|
| faces | 438 | 4.8% |
| facets | 36,572 | 13.3% |
| contact pairs | 299,414 | 13.5% |
| curve points | 37,191 | 8.0% |
| boundary epochs | 7 | |

Its stages split as boundary 12.0 s, faces 109.0 s and contacts 18.3 s. The
boundary and contact times match the cross-watch slopes; the models predict
11.3 s and 19.1 s.

Its faces stage takes 2.98 ms per facet. The rest of the watch would predict
about 77 s for that many facets, so 1713 costs about 1.4× more per facet.
NVIDIA tessellates it in 1.995 s with 24,078 triangles: owned is 70× slower and
emits 1.52× the triangles.

The recorded data cannot explain the extra 1.4× per facet: per-face timings
were not emitted. Two candidates, unmeasured:

1. The wave barrier holds 11 threads idle behind a few large faces with many
   holes, whose region and bound steps run on one thread [inf].
2. The mix of surfaces in this definition is costlier per facet [inf].

Because definitions run serially, 1713 alone is 16.4% of the meshing wall.

### Against NVIDIA, per definition (heal0 producer rows) [data]

- Summed owned component wall ÷ summed NVIDIA single-thread tessellate:
  847.7 / 22.86 = **37×**.
- The per-definition ratio has a median of 32.5×. Its 10–90% range is 18–87×,
  and the maximum is 279× (1783: 5 faces, owned 2,018 facets against NVIDIA's
  546).
- Owned and NVIDIA per-definition times correlate at r=0.78.
- The ratios are worst on small curved parts, where owned emits 2–4× NVIDIA's
  triangles. That signals over-refinement relative to NVIDIA's chord estimate.
  Whether NVIDIA understates its error is the NVIDIA lane's question.
  Equal quality is **not** established.

### Decomposing the gap [data+inf]

- **Work per core.** Owned meshing used about 1,578 CPU-seconds against
  NVIDIA's 22.86 s of tessellate, which is about **69×**. On owned's side this
  includes spin and yield time (see §5), so the true useful-work ratio is
  somewhat lower.
- **Utilisation.** Owned kept 1.86 of 12 cores busy. NVIDIA keeps 26.04 s of
  per-definition complete time inside a 6.13 s batch, about 4.2 cores
  including setup.
- **Floor of perfect scheduling today.** Packing today's work onto 12 cores
  would take 1,578/12 = 131 s using CPU-seconds, or 847.7/12 = 71 s if wall
  scaled. Concurrent definitions alone cannot beat 1713's 139 s.

## 3. Classifying the machinery

### (a) Certification, proof, verification, exact keys (candidates for opt-in)

| Mechanism | Where | Watch evidence |
|---|---|---|
| Per-facet certified positional upper bounds: candidate prep in Rational UV/orientation/envelopes, Rational clipping fallback, interval bound kernels, Rational accept compare, precise fallbacks, point floors | `adaptive_trimmed_geometry.cpp:274-545,856-1182`; `adaptive_surface_cache.cpp:183-418` | Authored faces at `4b67f3d`: `bound_seconds` is 66–97% of face phase time (plane 0.0022 of 0.0033 s; bowl 0.333 of 0.367 s; saddle 0.859 of 0.934 s; weighted 3.159 of 3.270 s). Watch value unknown. |
| Horizon refinement driven by failed certificates: refine, rebuild regions, re-bound | `adaptive_trimmed.cpp:408-505` | Authored saddle 7 horizons, weighted 10. Watch counts not emitted. |
| Mapped-cap facet bounds and cap domain continuation | `adaptive_cap_work.cpp`, `adaptive_trimmed.cpp:457-482` | 229 tests; ~70 s across the 4 cap definitions [inf] |
| Curve chord certification: Rational accept compare, precise node bounds, stage promotions; Rational midpoints | `adaptive_curves.cpp:858-992,1700-1764` | Part of boundary's 0.30 ms per point |
| Trace correspondence bounds | `adaptive_curves.cpp:1470-1524`, `adaptive_traces.cpp` (94 `Rational` uses) | Part of boundary |
| Source guards: vertex vs curve endpoint, staged precise | `adaptive_curves.cpp:736-817` | Once per definition; validates source |
| Contact self-intersection classification: serial BVH with Rational SAH costs, filtered pairs, `classify_exact` | `contact.cpp`, `contact_epoch.cpp`, `adaptive_component.cpp:127-273` | **164.9 s**; 0 forbidden pairs |
| Exact keys and copies: every site UV is an `ExactRef`; snapshot freeze copies all tables; `import_exact` GCD on every read | `adaptive_trim_types.hpp:111-116`, `adaptive_curves.cpp:1525-1640`, `exact_store.cpp:60-63` | Spread through all stages |

Why contacts falls to exact arithmetic so often [code + inf]. Take two
coplanar triangles that share only a vertex:

- The plane distances are zero, so `only_vertex` fails.
- In the projected test, the shared vertex lies exactly on two of each
  triangle's edge lines. `separating()` therefore never sees all three points
  strictly outside.
- The cross-edge SAT axes are parallel to the shared normal.

The pair reaches `classify_exact` [code `contact.cpp:323-370,400-476`]. Edge
neighbours are resolved by the projected edge filter. On planar faces with
fans, vertex-only neighbours make up most of the candidates. Planar faces are
plentiful in a watch, so a large share of the 2.2 M pairs probably take the
Rational path. `ContactPredicateStats.exact_pairs` exists but was not written.

### (b) Essential construction

| Mechanism | Where |
|---|---|
| One shared sample chain per source edge, physical node keys, vertex aliases, face boundary generations | `boundary.hpp/.cpp` (`DefinitionBoundaryForest`), `adaptive_curves.cpp:693-735,993-1059` |
| Use traces into each face chart | `adaptive_traces.cpp` |
| Face UV loops with occurrence and joint bindings (seams, wedges) | `adaptive_regions.cpp:298-420+` |
| Adaptive surface subdivision (leaf selection) | `adaptive_cache.cpp`, `adaptive_cache_detail.hpp`, `adaptive_surface_cache.cpp` (select path) |
| Interior packets and zipper | `adaptive_packets.cpp` |
| Boundary-region CDT | `adaptive_regions.cpp`, `adaptive_region_cdt.cpp`, `direct_cdt/core.cpp`, `direct_cdt/predicates.cpp` |
| Pole/apex wedge construction | `adaptive_cap_domain.cpp`, `adaptive_cap_geometry.cpp` |
| Face and component state machines; output records (sites, facets, edge provenance) | `adaptive_trimmed.cpp`, `adaptive_component.cpp`, `adaptive_trim_types.hpp` |

Several (b) units are implemented with (a)-style exact machinery: Rational
curve parameters, Rational UV sites, and common-denominator integer CDT
inputs. Their function is essential; the exact representation is a choice.

### (c) Infrastructure

| Mechanism | Where |
|---|---|
| Thread pool, wave coordinator | `scheduler.cpp`, `adaptive_component.cpp:65-95` |
| Metered memory: `RunResource` atomics plus `bgfd::Budget` per job, work charges with a stop-token check on every `charge` | `resources.hpp:15-55`, `face_domain/exact.hpp:29-115` |
| Exact tables | `exact_store.*`, `ExactBuilder`, `ExactTable` |
| Source decode, banks, caches | `source.cpp`, `source_cache.cpp`, `curve_source_cache.cpp` |
| Device ABI/modules | `cuda/adaptive_cuda.cu`, `cuda/adaptive_curve_ops.cuh`, `cuda/bound_cuda.cu`; dlsym handshakes |
| IO and schema | `adaptive_component_io.cpp`, `adaptive_caller_io.cpp`, `mesh_io.cpp`, BGACRV3/BGATRM2 |
| Scene and display | `adaptive_scene.cpp`, `native_display.cpp` |
| Diagnostics tracing | `diagnostic::` scopes in `adaptive_trimmed.cpp`, `adaptive_surface_cache.cpp` |

### Fraction estimate [inf; the band is wide on purpose]

- **contacts, 164.9 s.** The whole stage is (a) verification; it produces no
  geometry.
- **boundary, 139.9 s.** Sampling edges and mapping them to UV is cheap in
  floating point. NVIDIA's entire tessellation, edges included, costs
  0.1 ms per triangle, while ours costs 0.3 ms per curve point. I attribute
  about 80–95% to (a), meaning exact compares, precise fallbacks, trace
  certification and freeze copies, and 5–20% to (b).
- **faces, 541.1 s.** On the authored families, bound time was 66–97%
  [data, older revision]. The later lazy-work unit removed part of the curved
  bound work, and the watch is plane-heavy. I take 50–90% of faces as (a).
  The remainder, region CDT, packets, domain and selection (10–50%), is (b);
  even (b) there runs on exact sites. On the authored plane, region time was
  about 0.11 ms per facet, which already equals NVIDIA's whole per-triangle
  cost.
- **(c), directly separable:** owner setup, close and output, about 3 s
  (<1%).

| Category | Seconds | Share |
|---|---:|---:|
| (a) | ~550–770 | 65–90% (central ~75%) |
| (b) | ~70–290 | 8–34% (central ~20%) |
| (c) separable | ~3 | <1% |

The real cost of (c) is not a fraction of time. It multiplies the cost of (a)
and (b): heap Rational allocation through shared atomics, GCD on every read,
and barrier waves.

## 4. What to keep for a fast path, and what would fight it

### Keep: types, invariants and algorithms worth porting

- **`DefinitionBoundaryForest`** (`boundary.hpp/.cpp`). It owns the physical
  node keys, the per-edge interval keys, the root-relative sample path bits
  (`sample_paths`/`path_bits`), vertex aliases from declared-degenerate edges,
  and the face-generation bumps. Keep the model. Store the parameters as
  dyadic paths or doubles instead of `Rational`.
- **Identity and provenance types** (`adaptive_trim_types.hpp`):
  - `PhysicalNodeKey`, `BoundaryKey`, `FacetKey{generator, owner, revision}`.
  - `ConstructionSite.source`, the variant `BoundarySiteSource` /
    `SurfaceSiteSource{original root, stage, departure}`.
  - `SourceOccurrenceBinding`, `WedgeSectorBinding`.
  - Edge provenance (source edge, face cut, internal).

  These carry the source/UV/history maps and feed display normals, contacts
  and incremental remeshing.
- **The occurrence and joint logic in `prepare_face_domain`**
  (`adaptive_regions.cpp:298-420`), as the specification of how loops, seams
  and wedges are assembled.
- **`adaptive_packets.cpp`.** The interior packet zipper across hanging sides
  uses no Rational at all, and its output is regular, which suits a GPU.
- **`direct_cdt`.** Owned CDT with interval-filtered predicates and an exact
  fallback, plus extensive stats counters (`bgcdt_stats`). It is the right
  shape for boundary regions. Feed it fixed-point integers taken directly from
  doubles; do not build Rational sites and clear denominators.
- **Numeric kernels** (`adaptive_numeric.hpp`, `adaptive_curve_numeric.hpp`,
  `adaptive_trimmed_numeric.hpp`, the CUDA ops). Batched evaluation and
  interval bounds. These are the natural home for cheap error bounds that do
  not understate.
- **`ContactEpochOwner` deltas** (`contact_epoch.cpp`) and the filtered
  predicates (`contact.cpp`). Keep them as the self-intersection test oracle,
  opt-in in production. They can later serve incremental remeshing, collision
  and mechanism work, because deltas and epochs are already modelled.
- **SourceStore, source banks, IO codecs, scene and display**, and the
  independent original-source readers in the tests (the strong oracles).
- **Determinism**: completion order never changes the output; completions are
  sorted by key (`adaptive_component.cpp:84-87`). CPU1, CPU4 and both P100s
  give byte-identical artifacts. Keep the property and get it from canonical
  ordering, not from barriers.

### Fights a fast design

1. **Serial definitions with one active component**
   (`adaptive_model_main.cpp:399`, `adaptive_component.cpp:369`).
2. **Barrier waves of fixed capacity**, `wave()` → `drain()`
   (`adaptive_component.cpp:65-95`), at state-machine-step granularity. One
   long step stalls up to 11 workers. On P100, faces that are only polling the
   device take wave slots.
3. **No parallelism inside a face.** `cpu_threads = 1`
   (`adaptive_component.cpp:324`); a whole face's bound batch (up to 4,096
   records) or region rebuild runs on one thread.
4. **Serial boundary coordinator.** The per-face trace loop and all folds and
   compares run on one thread with a yield spin
   (`adaptive_component.cpp:419-423`, `adaptive_curves.cpp:1395-1430`).
5. **Value semantics through `Rational`/`Integer`.** Each holds a
   `std::pmr::vector<uint32_t>` of limbs allocated through `bgfd::Budget` →
   `RunResource` (two CAS operations on a shared cache line per allocation,
   `resources.hpp:19-45`) → `new/delete`. `import_exact` runs a GCD on every
   read. `ExactRef` indirection on every coordinate rules out SoA float
   pipelines on the GPU.
6. **Global horizons that re-scan everything.** `prepare_horizon` walks every
   edge on every epoch. Each face horizon rebuilds regions and re-bounds the
   changed facets. Every acceptance converts a double to Rational
   (`adaptive_surface_cache.cpp:183-185`, `adaptive_curves.cpp:885-891`).
7. **A device ABI and transfer pattern sized for small, chatty batches.**
   About 2.7 KB is uploaded per surface point and about 766 B per curve point.
   Polling goes through scheduler waves.
8. **Memory reservations.** The reservation-inclusive peak is 7.2–8.4 GB
   against a 2 GB RSS. Scheduler scratch of 256 MiB per thread
   (`scheduler.cpp:41-42`) times many concurrent components would exhaust the
   32 GiB metered limit before definition-level concurrency could help [inf].

## 5. Parallelism: what the code does, and what the data shows

### What runs in parallel [code]

| Stage | Parallel part | Serial part |
|---|---|---|
| faces | Up to `min(12, faces)` FaceTrimSession steps per wave; one face per thread; barrier per wave | Within a face; completion sort; everything between waves |
| boundary | Numeric kernels (samples, node bounds, restrictions, carrier samples, trace bounds) on a per-component 12-thread scheduler, cost-partitioned | Horizon prep, folds and Rational accepts, source guards, per-face `prepare_use_trace`, commit, freeze, on the coordinator |
| contacts | Pair classification in waves of ≤12×256 pairs | Site keying and sort, provenance sorts, BVH build and insert, per-facet query, pair freeze and sort |
| model | — | All 107 definitions; scene construction (12.1 s); envelopes (9.8 s) |

The process has 24 worker threads (two 12-thread schedulers). The one retained
stack sample, an earlier binary, shows 23 of them in `cond_wait`, the
coordinator in `NativeScheduler::drain` under `wave`, and one worker in
`Rational::multiply` inside `prepare_trim_facet_work`
(`context/adaptive-watch/attempt2-stage-stack.log`).

### Idling [data]

- Process user/wall: CPU12 1,605.28 s / 875.16 s gives 1.83 cores; P100
  1,571.69 / 872.50 gives 1.80.
- System time is 7.24 s, so yield spinning is not a large share.
- Assume each stage keeps at least one thread busy (the boundary coordinator
  spins, and a wave always has at least one running step). Then:
  - faces averaged **≤2.35 cores**: (1,578 − 140 − 165)/541;
  - contacts ≤5.4 cores.
- At least 80% of the 12-core capacity sat idle during meshing.

### Fleet measurements that would settle the unknowns (not run)

All of these are counter or timer emissions plus standard tools. None needs a
new mechanism; the counters marked "exists" are already computed in code.

1. **CPU-seconds per stage.** Read `clock_gettime(CLOCK_PROCESS_CPUTIME_ID)`
   at the five stage boundaries in `ComponentMesher::mesh`
   (`adaptive_component.cpp:400-548`) and write `*_cpu_seconds` beside the
   existing walls. CPU/wall gives effective parallelism per stage and
   definition directly.
2. **Face-phase split for each definition.** Write the per-owner
   `TrimFaceStats` deltas (exists: `adaptive_trimmed.hpp:45-52`):
   - seconds for classification, packet, region, bound and anchor;
   - counts of region builds, facet horizons, bound pieces, band anchors and
     packet targets;
   - `CacheStats` `exact_fallbacks`, `actual_facet_tests`,
     `actual_facet_failures` and `source_triangle_tests`.

   Today `surface_totals` emits only device and byte fields
   (`adaptive_component_io.cpp:39-64`). Also write `ComponentStats.face_steps`
   and `requested_intervals`. This settles the (a)/(b) split of faces.
3. **Per-face timing rows.** Record the elapsed time from assignment to
   completion and the total step time per face in `faces.tsv`, to find the
   heavy faces in 1713 and the cap definitions.
4. **Wave efficiency.** Per wave, record the wall time, Σ step durations,
   max step and number of tasks. Each worker already wraps `task->step`
   (`scheduler.cpp:76-88`); add steady-clock stamps.
   Waste = Σ(wall×12 − Σsteps).
5. **Contacts split.** Record wall and CPU time for keying, `prepare`
   (BVH build, query, freeze) and the classify waves separately. Write the
   `ContactEpochStats.bvh` counters `exact_costs`, `box_tests` and `visits`,
   and the `predicates` counters `filtered_*` and `exact_pairs` (exists:
   `contact.hpp:27-30,59-62`; not written).
6. **Boundary split.** Record wall and CPU time per boundary phase
   (evaluate, nodes, fold, traces, freeze, commit) and the `CurveStats`
   counters `exact_fallbacks`, `chord_tests`, `epochs`, `point_hits` and
   `requested_splits` (computed in `adaptive_curves.cpp`; only the device
   fields are written).
7. **Exact arithmetic volume.** Per stage, record the `bgfd::Budget` counters
   that already exist (`divisions`, `lehmer_batches`, `max_limbs`; see
   `face_domain/exact.hpp:91-114`), plus one relaxed atomic counter of
   `RunResource::do_allocate` calls.
8. **Serial fraction of 1713.** Run `brepgpu-adaptive-model --definitions 1713`
   with `--threads 1`, then `--threads 12`, with the same options. Compare
   `complete_component` and the stage walls.
9. **Whole-run profile.** Use
   `perf stat -e task-clock,context-switches,cpu-migrations` over the full run.
   Use `perf record -F 999 -g --call-graph dwarf` on a run of 1713 only, read
   as an aggregate profile and cross-checked against counters 2, 5 and 6
   (doctrine: not a single stack). Use `perf c2c` on the same run to measure
   false sharing on `RunResource::live_`/`peak_` and on the scheduler mutex.
10. **P100 polling waste.** Count the FaceStep completions that returned
    `pending` versus `progressed`, per wave.

## 6. Invariants: what to preserve, what is ceremony

### Preserve in any new path (these bear directly on manufacturing-grade output)

1. **One physical sample chain per source edge, used by every incident face.**
   Physical node identity is a key, never a coordinate weld. A shared key must
   carry bit-identical XYZ: this is the watertightness and shared-edge
   invariant (`boundary.hpp`; checks at `adaptive_component.cpp:146-147,
   159-163`; `ADAPTIVE_MESHER_DESIGN.md` "A source edge has ONE canonical
   physical sample sequence").
2. **Seam and occurrence correctness.** Two uses of one edge in one face, such
   as a periodic seam, keep two UV occurrences bound to the same physical
   nodes. Loop joints share settled UV and physical key. Pole/apex wedges keep
   both sectors at one stored pole (`adaptive_regions.cpp:330-420`,
   `docs/ADAPTIVE_CAPS.md`).
3. **Original vertex XYZ kept exactly.** Degenerate edges are joined only by
   explicit aliases (`boundary.hpp` `SourceVertexAlias`).
4. **Whole source inventory.** Every definition, face and use appears exactly
   once, with no skipped face (`adaptive_component.cpp:374-389,532-537`;
   9,162 rows in `faces.tsv`).
5. **Source, UV and history maps.** Each output site records its source (edge
   node or original surface root). Each facet records its generator, owner
   and revision; each edge its provenance. Face and boundary generations are
   recorded. Incremental remeshing (the complication roadmap) and correct
   display normals both need these. The generation and retention logic at
   `adaptive_component.cpp:521-529` is the template for remeshing only what
   changed.
6. **Orientation and positivity.** Facets are positive in their chart and the
   face sense is respected (orientation checks in `prepare_work_data` and the
   CDT).
7. **Deviation within the requested tolerance, never understated.** Keep the
   guarantee and change how it is reached. A bound computed in floating point
   that does not understate replaces the per-facet Rational certificate. The
   original-source readers in the tests stay as the strong oracle.
8. **Small features and sharp edges.** These follow from items 1–2. Original
   edges are always sampled and are constrained segments in every face's CDT.
9. **No self-intersections.** Keep the guarantee. Establish it with a test
   oracle: the contact BVH and filter, offline or opt-in, with exact fallback
   only when ambiguous.
10. **Determinism across thread counts and backends**, through canonical
    ordering.

### Ceremony that can move to opt-in or go away

- Rational representation of curve parameters and site UV: `CachedCurveNode`
  Rational bounds and `ConstructionSite.uv` `ExactRef`. Dyadic path bits
  already identify the samples.
- The per-facet certified upper bound as the hot-path stopping rule, with a
  Rational compare against tau and precise/materialised fallback cascades.
- Exact Rational SAH costs in the BVH build.
- The double → Rational conversion on every acceptance, the GCD on every
  `import_exact`, and the copy of every exact table at snapshot freeze.
- The hot-path whole-component contact classification (165 s).
- A stop-token check on every unit of `Budget::charge`.
- Run-wide atomic metering of every small allocation.
- The source endpoint guards, which validate the source and should run once
  per source rather than once per query.
- The repeated identity-generation `require` checks at every handoff. They are
  cheap individually; keep them in debug or test builds where they catch real
  faults.
