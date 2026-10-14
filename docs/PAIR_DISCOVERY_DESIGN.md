# Conservative surface-pair discovery beachhead

2026-10-14. Breadth-first delivery following immutable bodies and trimmed
sampling. This operation produces a deterministic shortlist of untrimmed
carrier-chart pairs. A set bit means **closed AABB survivor**, not intersection,
contact, trim overlap, or a Boolean decision. A cleared examined bit safely
excludes both carriers when the supplied boxes enclose them. Arbitrary native
boxes make only a box-overlap claim; the owned body adapter supplies geometry
enclosure and source identity.

## Inputs, frame and identity

The native input is an immutable copied collection of `bg_pair_box` records:
three lower doubles followed by three upper doubles, 48 bytes. Each endpoint
must be finite; lower<=upper on every axis. Singleton axes, signed zero and
subnormals are admitted. Each index has at most 65,536 records. No world-range
clipping, trim classification, same-face filtering or duplicate removal occurs.
An index belongs to one existing context. Two operands must belong to that
same live context; native results retain both source objects after public index
close. Closing a context prevents further operations, as in existing APIs.

`prepare_scene(cpu_engine, bodies, face_ids=None)` builds an owned `PairScene`
from the existing canonical factory snapshots. Require a CPU engine and exact
`BodySnapshot` values; reject duplicate bodies and duplicate/unknown selected
face keys. `face_ids` contains `(body_id, face_id)` pairs, never bare IDs:
entity IDs are typed and local to a body. Bound body and selected-key iterables
at 65,536 entries each while copying; cap admitted charts at 65,536 before
control/hierarchy allocation. Selection preserves body order and canonical chart order. Empty
body/face selection is valid. Each source record retains its exact immutable
snapshot, source patch slot, typed face ID and chart ID. Scene construction is
factory-only; external arbitrary scene records are unsupported.

Use the existing CPU depth-zero AABB hierarchy on original patch controls.
Download actual local roots, verify their slot/leaf/cell/count correspondence,
then convert each endpoint to the single common world frame defined by the
source snapshot's origin and positive scale. Use exact `Fraction` arithmetic
on the submitted binary64 endpoint/origin/scale, round to binary64, compare
that rounded value with the exact value, and `nextafter` only if the result
rounded inward. Assert exact containment before publishing. The existing
hierarchy's supported rounding/gradual-underflow environment is required on
this setup thread. Frame restrictions stay those of `Frame`; no rotation or
general affine transform is added. Empty scenes do no arithmetic. Local roots
and world boxes are immutable tuples; do not upload body topology.

Full rectangular carrier domains enclose their trimmed subsets, including
plate holes. Each cylinder wall quarter remains a chart of one face. Its seams
and adjacent charts remain separate source occurrences. A face can be excluded
only after every required chart pair has been examined and excluded; one zero
chart bit cannot exclude the whole face. Self queries include diagonal and both
directions. Different local frames are never compared directly.

## Exact predicate and bounded pages

For nL/nR indices, checked uint64 multiplication defines T=nL*nR. Cartesian
flat index q maps to i=q/nR, j=q%nR. A page examines
`[start, start+count)`, count=min(max_pairs,T-start), in that order. Admit
start in [0,T] and max_pairs in [1,2^20]; reject zero, cap overflow and flags
other than zero. Empty side and start==T return a valid empty page; never divide
by zero. Cursors count **examined pairs**, independent of survivors.

For page-local k, bit k%32 of word k//32 is one iff
`L.lower[a]<=R.upper[a] && R.lower[a]<=L.upper[a]` on all three axes.
Equality is retained. Each CPU/CUDA thread owns a complete word, initializes
zero, checks at most 32 valid pairs and writes once. Tail bits are exactly zero
padding; they are neither examined pairs nor exclusions. No atomics, compaction,
sorting, BVH traversal or candidate counter is needed in this beachhead.

Implement finite admission and comparison through shared exact IEEE-bit order:
inspect exponent bits for finiteness, normalize both signed zeros, and map
negative/positive finite bit patterns to monotone uint64 keys. Host loads use
memcpy; device loads use CUDA bitcast. This comparison-only operation is
independent of inherited rounding and DAZ/FTZ; it performs no FP arithmetic.
Do not change existing jet/projection/discovery arithmetic or compilation flags.

Page metadata exposes total_pairs/start/count/next_start/left_count/right_count/
word_count, backend and flags. REACHED_END=1 iff next_start==T. FULL_DOMAIN=2
iff start==0 && count==T; an end page starting later is not full coverage.
PROFILED=4 only for an actual nonempty CUDA event measurement; kernel_ms is
NaN otherwise, including empty pages and CPU. A sequence starting zero and
advancing next_start contiguously to T establishes full traversal, never one
unexamined prefix. At max page size the output is 32,768 u32 words, 131,072 bytes.

## Public and plugin boundary

Add public ABI4 `include/brepgpu_pairs.h`; existing layouts/exports unchanged.
Versioned ABI4 records: page options24B (version,size,start:u64,max_pairs:u32,
flags:u32), index info24B (version,size,count:u64,backend:u32,reserved:u32),
page info80B (version,size,seven named u64 fields,backend:u32,flags:u32,
kernel_ms:double). ABI info40B reports box/options/index/page/diagnostic and
double/integer/handle sizes. Byte capacities may be larger; canonical record
sizes and numeric extents are exact. Diagnostics remain ABI1.

Exports: bg4_pair_abi_info, bg4_create_pair_index(context,boxes,count,box_bytes,
out,diag), bg4_pair_index_info, bg4_pair_index_download, bg4_discover_pairs(left,
right,options,options_bytes,out,diag), bg4_pair_page_info and
bg4_pair_page_download. bg1_close consumes the two new kinds. Each download
uses exact count/byte extent; alias/span preflight precedes all writes, including
diagnostics. Other creator failures zero a valid disjoint handle destination.
Borrowed memory is stable for the synchronous call. Backend copy failure
invalidates host destinations; failure cannot publish an all-zero success page.

Optional CUDA v4 table has base-v1 pointer plus six callbacks: index upload,
index close, actual resident index download, page discovery, page close and word
download (72 bytes including version/size). Its static factory
matches the exact existing base pointer and exact table size; no v2/v3 coupling.
Lazy missing/refused v4 is stable unsupported for pair operations only. Fatal
and transient failures are not cached as capability refusal. Reuse existing
Admission, registry publication, cleanup diagnostics, device restore and global
CUDA poisoning. Retain left admission and resolve right under the same context
lock; reject cross-context inputs without acquiring a second context lock.
Creation/upload/kernel/download/restore failures publish no handle; pending
tokens are consumed once, including publication allocation failure.

Python primitive: `Engine.create_pair_index(boxes)` accepts six-value records,
incrementally bounded/copied; `Engine.discover_pairs(left,right,start=0,
max_pairs=1048576)` returns `PairPage`. `PairIndex.info()/download()` expose
actual resident input, `PairPage.info()/download()` expose actual words and
metadata. Lazy declarations let an older core retain existing operations and
fail pair calls explicitly unsupported. Returned downloads own their values.

Body consumer: `PairDiscovery(engine,left_scene,right_scene)` owns two uploaded
native indexes, borrows the engine and retains exact scenes. Its
`page(start=0,max_pairs=1048576)` returns an immutable `ScenePairPage` with
copied metadata, words and exact scene identities after successful child
cleanup. `survivors()` lazily yields `(flat_index,left_source,right_source)`
records; no per-pair allocation for exclusions. Guard BaseException cleanup
without replacing a primary failure. Record native_page_ms, download_ms,
child_cleanup_ms and reused_wall_ms, plus setup/upload separately in consumers.

## Independent truth and actual use

The oracle reads original homogeneous controls as exact Fractions, proves
local-root enclosure of their projected extrema, then verifies actual world
boxes enclose exact transformed extrema **and actual local endpoints**. It
does not call production rounding helpers. Expected words come from exact
closed-box comparisons on downloaded actual boxes. Check every bit, padding,
cursor, flag and original source identity. Analytic contact witnesses and
deliberate false positives explain the narrower geometric scope.

Direct tests cover subnormal/signed-zero/extreme boxes; touching/degenerate/
separated cases; different-frame coincidence/separation; hole and concentric
cylinder false positives; asymmetry, empty/full/partial/terminal pages; selected
faces; index/result ownership; foreign/wrong/stale handles; exact extents and
alias-preserving rejection; missing/refused/malformed/transient/fatal v4 and
publication/cleanup/restore failure. Existing native fault injection is reused.

Runnable caller builds a multi-body scene, walks all pages, qualifies enclosure
and bit truth, exports source/bounds/mask/shortlist TSV or binary and an owned
SVG of candidate relationships. Benchmark small scenes and larger scenes with
at most 2^20 Cartesian pairs: CPU1/CPU24 and each P100, fresh process cold,
excluded warmup, five repeated calls; report setup, CPU bounds, upload, native
page, download, CPU survivor decoding, cleanup and actual caller wall. Initial
exact qualification is outside reused timings; all later calls keep source and
status checks. No own copying/builds/qualification overlap performance trials.
This is an all-pairs first delivery, not an acceleration claim until measured.

Study anchors: [runtime](../papers/2026-10-14-pair-runtime-study.md),
[topology](../papers/2026-10-14-pair-topology-study.md),
[oracle](../papers/2026-10-14-pair-oracle-study.md). BVH/sweep-and-prune, tighter
bounds, SSI and editable topology stay later themes/return passes. No dependency
is adopted.
