# Conservative surface-pair discovery

The fourth native operation returns bounded pages of closed-AABB overlap bits
over two resident immutable indexes. The body consumer builds those bounds from
the existing box/cylinder/one-hole-plate charts in a common world frame and
retains exact body/face/chart identities. Set bits are **survivors for later
intersection work**. They do not establish surface intersection, trim overlap,
contact or a Boolean result.

## Body caller

Build the optional CUDA plugin with the existing CUDA12 sm60 configuration and
point `BREPGPU_LIBRARY` at the new core. Scene preparation uses a CPU context;
query execution can use CPU1/CPU24 or either P100.

```python
from brepgpu import Engine
from brepgpu.body import Frame, make_body
from brepgpu.pair_scene import prepare_scene, PairDiscovery

left_body = make_body("plate")
right_body = make_body("cylinder", Frame((.375, .25, .125), 1.))
with Engine(backend="cpu") as cpu:
    left = prepare_scene(cpu, (left_body,))
    right = prepare_scene(cpu, (right_body,))

with Engine(backend="cuda", device=0) as gpu:
    with PairDiscovery(gpu, left, right) as pairs:
        start = 0
        while True:
            page = pairs.page(start=start, max_pairs=1 << 20)
            for flat_index, left_source, right_source in page.survivors():
                print(flat_index, left_source.face_id, right_source.face_id)
            if page.info.flags & 1:  # REACHED_END of this page sequence
                break
            start = page.info.next_start
```

`prepare_scene(cpu,bodies,face_ids=None)` takes exact factory snapshots. Select
faces with `(body_id,face_id)` keys; face IDs alone repeat across bodies. Empty
selection is valid. Bodies, selected keys and admitted charts each have an
incremental cap of65,536. Duplicate bodies/keys and unknown keys reject the
whole request. Body order and canonical chart order remain stable. A cylinder
wall is one face with four charts; no coordinate welding or same-face filtering.

`PairScene` construction is factory-only. Its bodies/sources/local_boxes/boxes/
timings are frozen owned tuples. Each source retains snapshot/patch_slot/face_id/
chart_id. Local depth0 AABB roots enclose the original rational carrier controls.
Positive source scale and origin transform exact binary64 endpoints through
Fraction arithmetic, with outward binary64 rounding. Supported setup FP modes
remain those of the existing hierarchy builder. Current Frame restrictions
remain origin[-16,16], power-of-two scale .25..4 and no rotation.

`PairDiscovery(engine,left,right)` borrows Engine, retains exact scenes and owns
two uploaded indexes. `download_boxes()` returns actual resident inputs for
qualification. `page()` downloads/copies/checks metadata and tail bits, then
closes its native child before publishing a frozen `ScenePairPage`. Returned
pages remain readable after consumer/engine close. Failure publishes no page;
cleanup retains a primary exception even if close raises an interrupt.

## Page semantics

Index records are flat six doubles `(lx,ly,lz,ux,uy,uz)`. Native admission
requires finite ordered endpoints; singleton axes, subnormals and both zero
signs are valid. The predicate uses exact finite-bit ordering, so its
comparison-only CPU/CUDA operation does not depend on inherited FP modes.

Cartesian order is `flat=left_slot*right_count+right_slot`. A page examines
`count=min(max_pairs,total_pairs-start)` pairs; start in[0,total], budget in
[1,2^20]. Terminal start and empty sides yield valid empty pages. Every bit
indexes `start+local_bit`, including unaligned starts. Masks are u32 words,
LSB-first; zero tail bits are padding, never exclusions. The native predicate
retains closed-boundary touching and rejects only strict axis separation.

| Flag | Meaning |
| --- | --- |
| REACHED_END=1 | next_start equals total_pairs; a prior prefix may still be unexamined |
| FULL_DOMAIN=2 | this single page started zero and examined the whole Cartesian domain |
| PROFILED=4 | actual nonempty CUDA event measurement; otherwise kernel_ms is NaN |

Metadata includes total_pairs/start/count/next_start, both index counts,
word_count/backend/flags/kernel_ms. A complete multi-page traversal starts
zero and advances contiguously to total on the same immutable index pair.
One cleared chart-pair bit cannot exclude an entire face with other charts.
Self queries include diagonals and both ordered directions.

## Native and Python primitive

Public [ABI4 header](../include/brepgpu_pairs.h) adds PairIndex and PairPage
resource kinds, all closed through bg1_close. Versioned records are ABI4;
diagnostics remain ABI1. Exact sizes: boxes48B, options24B, index info24B,
page info80B, ABI info40B. Numeric downloads require exact count/byte extents;
record capacities may be larger but canonical struct_size stays exact.
Alias/unrepresentable-span rejection preserves outputs and diagnostics;
other creator failures zero a valid disjoint output handle. Backend copy
failure invalidates destinations. Same-context inputs required. Closing public
indexes preserves a live result's retained storage; closing Engine invalidates
future native operations. Fatal CUDA diagnostics poison all operation families.

Primitive usage is `Engine.create_pair_index(boxes)` and
`Engine.discover_pairs(left,right,start=0,max_pairs=1048576)`. Index info/download
expose copied actual storage; page info/download expose owned metadata/words.
The primitive accepts arbitrary boxes and promises only closed-box overlap.
An optional v4 plugin table requires exact retained v1 base identity, with no
v2/v3 coupling. Missing/refused v4 fails pair calls explicitly unsupported;
older operations remain usable. Transient/fatal failure is not capability refusal.

## Costs and scope

Current execution is all-pairs, with one CPU/CUDA owner per output word, at
most32 comparisons per word. Fresh result allocation/synchronization/cleanup
remain in resident call latency. No scan, atomics, compaction, BVH, spatial grid
or work queue. Index payload is48*(nL+nR) bytes uploaded once; a page downloads
4*ceil(count/32) mask bytes, at most131,072. Body topology upload is zero.
These are host/device transfers; SSH/network traffic is separate. Retained
host controls/boxes, metadata and Python survivor objects consume additional
memory and work. Bounds construction/world transform and decoding remain CPU.

Run [the actual caller](../examples/discover_pairs.py) for a deterministic
mixed-body scene and output source/bounds TSV, packed mask, typed shortlist,
qualification, timings and an SVG of actual bits:

```sh
PYTHONPATH=python python3 examples/discover_pairs.py --backend cuda --device 0 \
  --size 4 --library-path build/pair-release/libbrepgpu.so \
  --output-directory context/my-pair-scene
```

The caller's1..128-body recipe fits one complete page; direct tests separately
qualify continuation and total2^32. Larger SVGs show only the labelled first100
charts per side; mask and TSV retain full results. The
[serial matrix](../benchmarks/pairs/run.py) compares4/128 bodies per side on
CPU1/CPU24/P1000/P1001, reporting cold setup, first use and five reused complete
shortlist calls separately. No full-CAD speedup follows from this operation.

[Design](PAIR_DISCOVERY_DESIGN.md), [plan](PAIR_DISCOVERY_PLAN.md),
[independent truth](../python/brepgpu/pair_oracles.py) and
[literature](../papers/2026-10-14-pair-literature.md) explain the chosen boundary.
Sparse-scene pruning, tighter bounds and tiling belong to return passes;
intersection initialization/representation is the next major theme.
