# C++ migration: exact geometry and ownership seams

Study only, following the operator's request to diagnose and report before
proceeding. No implementation, execution, benchmark, installation or dependency
adoption. This complements the root review of the actual Blackbird C++ rigor
stack and the [measured mesher diagnosis](2026-10-15-om10-mesher-surgical-diagnosis-algorithms.md).

## Recommended first complete native unit

Build an **owned sampled-face domain engine**, rather than translating the
current all-pairs and ear loops verbatim. It consumes an immutable exact
boundary-occurrence snapshot; constructs conservative spatial indexes; finds
the first unsupported boundary intersection; and bridges/triangulates admitted
outer/hole domains into triangles of original occurrence IDs. It supplies both
`_coordinate_circle_schedules`' candidate-domain checks and finalized faces'
`_boundary_intersection`/`_triangulate` work. These are the observed expensive
phases in `python/brepgpu/imported_meshing.py`.

The unit owns its input arrays, exact-number storage, indexes, active occurrence
links and result lifetime. Its API operates on batches/owned sessions, avoiding
one process or foreign-function crossing per predicate. Python initially
supplies source-derived boundaries and consumes source-keyed output; native
ownership expands afterward to shared edge scheduling and carrier refinement.
This is a bounded beginning of phaseout, not a claim that the first unit makes
the complete watch mesher native or that C++ alone removes quadratic work.

## Numbers that cannot silently change

`_plane_uv` computes an exact inverse of the saved binary64 frame using
`Fraction`. Its coordinates can be non-dyadic rationals. The existing oblique
plane test requires exact collinearity and exactly equal U values. `_orient`
uses exact rational signs for those coordinates; boundary touch tests,
bridge visibility, inclusive ear containment, area and coordinate ordering
depend on them. Float-only charts separately use a filtered sign with exact
saved-binary64 fallback. Original UVs are rounded only at the display packing
boundary, with `_uv_rounding_bound` charged to the existing quality budget.

Neither `double`, `long double` nor a fixed `__int128` substitutes for this
unbounded contract. Arbitrary exponents, rational inverse denominators and
products can exceed a fixed integer width. The existing Lawson stage already
shows one useful representation change: positive common per-axis denominators
turn fixed-coordinate signs into exact integer signs. It does not bound the
required bit width, and a large LCM can itself cost work.

Before implementation, choose the exact-number machinery explicitly: an owned
minimal arbitrary-integer/rational or homogeneous-coordinate implementation;
or a discussed dependency with its concrete benefits, costs and maintenance
tradeoffs. The C++ standard library supplies no arbitrary-precision rational.
No library is selected here. Exact expansion predicates over original dyadics
are another design direction, but a binary64 orientation routine alone cannot
replace comparisons of the constructed rational plane coordinates. Keeping
Python's Fraction-based producer temporarily requires lossless sign/limb/
denominator input, not a double conversion disguised as a thin binding.

## Source, occurrence and slot ownership

Preserve distinct strong types for original definition/face/edge/coedge/vertex
IDs, physical sample keys, local boundary occurrences and dense array slots.
Two seam uses and duplicated bridge coordinates remain distinct occurrences;
one physical vertex may have multiple UV/chart/display corners. Keep original
t ranges, direction, pcurve parameters/raw UVs, recovery kinds and declared
pole/chart closures. Triangles reference occurrence IDs and recover their
physical keys; neither coordinate equality nor an address is entity identity.

`ImportedSource` owns immutable typed tables and ID maps. Its BGSRC001 envelope
includes original source SHA and adapter revision, but no mutable body revision.
Prepared source bytes can change while the original STEP SHA stays constant.
Bind a native domain owner to the actual source snapshot/preparation identity,
quality and exact edge-schedule generation. Reject stale indexes/results after
schedule replacement. Do not invent an implicit numeric body revision from
a slot. ABI5's existing `Source(instance,body_id,revision,entity_id)` and slot
permutation contract in `docs/ADJACENCY.md` are the applicable ownership model
when connecting imported results to that subsystem; the correspondence needs
an explicit design.

`native/om10/types.hpp` and `scene.hpp` already separate owned arrays from a
borrowed `SceneView`, validate sizes and use dense 32-bit display slots with
64-bit original IDs/physical keys. They are useful patterns. BGMESH01 is not
a complete domain interchange: `_write_scene` rounds UVs, splits spline
carriers into spans, duplicates display corners, and omits boundary-use/recovery
records. Preserve its stable display consumer while adding the earlier exact
domain boundary. Do not reinterpret packed bytes as pointer-rich native objects.

## CPU concurrency and independent evidence

Circle candidates alter shared edge schedules; preserve that global ordering
until a reviewed conflict/dependency design replaces it. Once schedules are
stable, face domains can use independent worker-owned scratch against one
immutable source owner. Bound simultaneous memory, catch worker failures,
join/cancel safely, and discard incomplete results. Merge results in canonical
source order; allocate generated identities deterministically rather than by
atomic task completion. Shared foreign OCCT objects stay in their isolated
adapter, not worker scratch. The existing display worker pool is a pattern,
not proof of a safe meshing scheduler.

Keep `imported_oracles.py` in Python: its original STEP reader, Fraction-based
positive boundary-chain checker, physical shell/vertex-link checks and
source/error checks are independent of production triangulation. It must read
actual native output including exact coordinates/provenance, rather than
calling native predicates to confirm themselves. Bounded old/new triangle or
first-crossing comparisons check the preservation claim; independent complete
domain/source checks determine correctness when a later algorithm legitimately
changes diagonals. Preserve zero/touching, holes, bridge duplicates, seams and
pole cases. Measure spatial candidate reduction and one actual difficult face
before a full-watch rerun; sanitizer/static-analysis policy follows the actual
Blackbird stack, not a new certification campaign invented for migration.
