# Pair scene implementation

2026-10-14. Implement the accepted scene/consumer unit in
`python/brepgpu/pair_scene.py`, following
[the plan](../docs/PAIR_DISCOVERY_PLAN.md). The fleet direct red reached a
healthy CPU engine and failed at the missing `create_pair_index` operation.
Root owns all native execution and integration.

Subplan: copy bounded canonical snapshots and selected face keys; gather chart
sources in canonical order; construct and download actual CPU depth-zero roots;
check their identity; transform root endpoints outward with exact Fractions;
publish a factory-only immutable scene after guarded child cleanup. The
consumer uploads two indexes once, retains exact scenes, exposes actual input
downloads and returns copied immutable page words/metadata only after child
cleanup. Every cleanup path attempts its owned resources and preserves an
active BaseException. No topology upload or intersection claim is added.

Local verification is source/AST inspection only. No dependencies, native
changes, builds, native tests, fleet work or Git operations are owned here.

Implementation is on disk and AST parsing passed. `PairScene` exposes tuples
`bodies`, `sources`, `local_boxes`, `boxes` and named `timings`; sources retain
`snapshot`, `patch_slot`, `face_id`, `chart_id`. Boxes are flat-six records.
`PairDiscovery` owns index adoption and both-index cleanup, exposes actual
`download_boxes()`, and creates frozen `ScenePairPage` records with named
immutable metadata, words, exact left/right scenes and timings. Survivors are
lazy `(flat_index, left_source, right_source)` triples in Cartesian order.

CPU hierarchy setup checks patch/node counts, mode, backend, depth, root
uniqueness, leaf children, full parameter cells and finite ordered endpoints.
World conversion compares each rounded endpoint with its exact Fraction and
steps outward only when needed. Page copying checks retained-scene counts,
cursor, extent, coverage/profile flags, event duration and padding. These are
consumer admission/shape checks; the independent oracle still owns geometric
enclosure and exact bit truth.

Root's initial source review found selection, root/world binding and cleanup
coherent. Public wrapper download shapes are the only integration seam:
index download yields flat-six box records; page download yields `info` and
`words`. Native execution and independent oracle/code review remain root-owned.
