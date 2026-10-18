# B2-display: canonical display tile bins on CUDA and CPU

This is unit B2-display of maneuver B2
([brief](../../docs/fast-mesher-campaign/briefs/B2-lane-defects.md)).

- **Bead:** `brepgpu-12s.37.3`.
- **In-phase additions:**
  - the CPU fill, which had the same arrival-order pattern;
  - a dated note in the display study;
  - two adversarial reviews (the first found no correctness defects; it asked
    for performance and coverage work);
  - two final nits.
- **Who wrote this:** the dispatcher, from the lane's returns.
- **Where it ran:** code edited in the laptop checkout; built and tested only on
  the fleet host.

Both bin builders now publish each 16×16-sample tile's list in ascending
primitive order, for every run, GPU and worker count. Neither defect was visible
in the output, and display bytes are unchanged by both fixes.

**Red evidence.** Every red run below used `build_bins` extracted but not yet
fixed. A generated `prefix/` tree was built from the final files:
- on CUDA, without the ordering stage;
- on CPU, with the original atomic body.

## 1. CUDA: slots assigned in `atomicAdd` arrival order

**Study.**
- `fill_kernel` placed each primitive at
  `offsets[tile] + atomicAdd(cursor + tile, 1)`. The order inside a tile's list
  was therefore the order the atomics arrived in.
- The only consumer, `visibility_kernel`, takes per sample the largest z, with
  ties going to the lowest primitive index. That is the maximum over a total
  order, so it does not depend on list order.
- The failure was therefore in internal state, not output. It was still against
  [G7](../../papers/2026-10-17-determinism-cache-keys.md) §2.2 items 4–5: canonical
  publication, and no slot-allocating atomics.

**Fix.**
- Keep the count, scan and atomic fill. Then sort each tile's list ascending in a
  new `tile_bin_sort` stage (Batcher bitonic networks).
- `order_warp_bins_kernel` sorts lists of 2–256 entries in registers, one warp per
  tile, at one or eight entries per lane. In the same pass it flags tiles with
  more than 256 entries.
- The existing scan of those flags and `gather_long_kernel` collect the long tiles
  in tile order, with no atomics.
- `order_long_bins_kernel` runs a persistent grid of SMs × 4 blocks over the
  gathered tiles only:
  - lists of up to 4096 entries sort in shared memory, with 32-bit indices;
  - longer lists sort in place in global memory, with 64-bit indices.
- Comparator indices use bit masks, not 64-bit divide.
- The SM count is read once per session, and the launch shape actually used is
  exposed and printed (N4).

**Test.** `tests/native_om10_cuda_bins.cu` (ctest `cuda_bins_canonical_p100_0_1`).
A sequential model is checked on every run on both GPUs. Display bytes (G7 §3.2)
are compared over fresh sessions on both GPUs, plus an empty-scene frame.

| | Bins equal to the model | Display |
|---|---|---|
| Red | 0/8 runs per GPU, 8 distinct digests, on `dense_corner`, `skewed`, `mixed_random`, `long_bins` and `max_grid`; `tiny_three` 0/8 with one stable but non-ascending digest; `tiny_empty` 8/8 | identical 6/6 per GPU and across GPUs (digest `2ea6f47eaa33d233`) |
| Green | 8/8 per GPU, one digest, every family | digest unchanged; empty scene shows background only |

**Cost.** Bin stage on GPU1, minimum of 15 renders. GPU0 was thermally
throttling, so its numbers were not used.

| Scene | Samples | No sort (pre-fix) | First fix | Final | Visibility |
|---|---|---|---|---|---|
| 98k primitives | 1024² | 0.245 ms | 0.524 ms | 0.402 ms | 6.5 ms |
| 98k primitives | 8192² | 6.78 ms | 9.29 ms | 8.43 ms | 204 ms |
| 1.57M primitives | 1024² | 1.82 ms | 4.35 ms | 2.84 ms | 26.7 ms |
| 1.57M primitives | 8192² | 11.44 ms | 14.15 ms | 13.28 ms | 298 ms |

- **Overhead of canonical order:** +0.16 to 1.85 ms. This is the first P100
  measurement of G7 §7 item 2.
- **Warp kernel:** it stays on the full grid, because most tiles hold 2–256
  entries (85% at 8192²). This is reasoned, not measured.

## 2. CPU: slots assigned in `fetch_add` arrival order

**Study.**
- At `native/om10/cpu.cpp:250`, pool workers claimed 32 primitives at a time and
  took slots with `fetch_add`. Bins therefore varied with worker count and run.
- Visibility uses the same total order as CUDA, so this was not visible in the
  output either.

**Fix.** A fixed-order count, scan and fill, with no atomics. A per-tile sort
after the old fill was measured slower and dropped.
- **Chunk boundaries** come from a prefix over per-primitive reference counts, so
  each chunk holds about the same number of tile references. They are found by
  block sums of 4096 primitives plus a binary search.
- **Chunk count:** min(workers, 2^24 / tiles, primitives), and at least 1.
- **Fill:** each chunk counts into a private row; a per-tile scan in chunk order
  gives each chunk its start; each chunk then fills ascending.
- **Determinism:** the chunk count follows the worker count, the result does not
  (G7 §2.2 items 3–5).
- **Buffers** are held in the session and zeroed once per frame.
- `Pool::parallel` takes a claim size and requires `step > 0`.

**Test.** `tests/native_om10_cpu_bins.cpp` (ctest `cpu_bins_canonical_threads`),
at worker counts 1, 2, 3, 12, 24, 48 and 96.
- It prints the chunk count actually used.
- It compares against an independent one-worker display session.
- It checks the empty scene at every worker count.

| | Bins equal to the model | Display |
|---|---|---|
| Red | 0/3 runs, 3 distinct digests, at every count from 2 up, on every non-empty family; 3/3 at one worker | identical across counts (digest `2ea6f47eaa33d233`) |
| Green | 3/3, one digest, every family at every count | unchanged |

Chunk counts actually used:

| Family | Chunks used |
|---|---|
| `tiny_empty` | 1 |
| `tiny_three` | clamped at 3 |
| `skewed`, `dense_corner`, `mixed_random`, `long_bins` | equal to the worker count |
| `max_grid` | capped at 64 by the row budget |

**Cost.** `build_bins`, minimum over two rounds of 11. Each cell gives the old
atomic fill, then the first fix (equal-count chunks), then the final fill
(reference-balanced chunks).

| Family | 12 workers | 24 workers |
|---|---|---|
| `skewed` | 21.7–24.6 / 61.8–64.7 / **13.6–14.0** ms | 17.9–18.2 / 50.3–52.4 / **11.2–11.7** ms |
| `dense_corner` | 10.5–11.6 / 1.10–1.22 / 1.55–1.68 ms | 6.3–11.5 / 1.17–1.31 / 1.33–1.61 ms |
| `mixed_random` | 7.95–9.14 / 3.38–5.72 / 4.21–6.88 ms | 5.17–5.76 / 3.68–3.84 / 3.84–4.23 ms |
| `long_bins` | 6.0–11.8 / 0.50–0.52 / 0.53–0.61 ms | 8.2–9.5 / 0.40–0.47 / 0.46–0.55 ms |
| `max_grid` | 9.56–9.77 / 5.17–5.66 / 4.84–5.22 ms | 9.25–10.0 / 11.7–12.1 / 8.62–9.60 ms |

The final fill beats the old atomic fill on every family. The host was loaded by
other lanes, so treat these figures as indicative.

## 3. Display study note

A dated note (2026-10-18) is appended after
`papers/2026-10-15-om10-native-display-study.md:177`; the original text is
unchanged. It says:
- published bins are canonical on both backends;
- the CUDA scatter is transient within the bin stage;
- display bytes are unchanged.

## Fixtures

`tests/om10_display_fixtures.hpp` holds everything both tests share:
- the families, all authored: `tiny_empty`, `tiny_three`, `skewed`,
  `dense_corner`, `mixed_random`, `long_bins` (every kernel boundary) and
  `max_grid` (512×512 tiles);
- the sequential model;
- a 98,688-primitive display scene;
- an empty scene;
- an sRGB encoder written from IEC 61966-2-1.

Nothing comes from the corpus or OM10. An argument-evaluation-order defect in an
earlier fixture draw is fixed, so Release, ASan and TSan builds now generate the
same data.

## Suites

| Suite | Result |
|---|---|
| om10 ctest | 5/5 |
| `test_om10_native` | 10/10 on CPU with 1 and 12 workers, CUDA GPU0, CUDA GPU1 |
| `test_om10_native_failures` (CUDA checks) | 8/8 |
| Native mesher ctest with CUDA | 206/210 |
| compute-sanitizer memcheck, racecheck, synccheck | 0 errors, 0 hazards, 0 leaks |
| ASan and UBSan (CPU) | clean |
| ThreadSanitizer (CPU, 1 to 96 workers) | no warnings |

The four native mesher failures are the cylinder-seam family of the retired old
mesher (`native_adaptive_trimmed_cylinder_seam_{cpu1,cpu4,p100_0,p100_1}`,
`.37.11`). They fail the same way on unmodified HEAD.

## Review

| Pass | Correctness findings | Other findings |
|---|---|---|
| First | none | CUDA: one block per tile, and 64-bit divide. CPU: one chunk per worker, so skewed scenes are unbalanced. Test coverage short of G7 §3.2. Nits. All fixed. |
| Second | none, in every path checked | asked for the red and timing evidence above; three nits, two fixed (SM count cached, launch shape exposed) |

The third nit was the thin sRGB spec oracle. The reviewer judged it acceptable as
it stands, so it was left.

## Further defects

`native/topology_cuda.cu:109` has the same arrival-order pattern. It is filed for
M8 (`brepgpu-12s.25.8`).
