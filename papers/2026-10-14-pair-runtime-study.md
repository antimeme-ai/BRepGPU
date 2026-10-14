# Resident box-pair discovery: runtime source study

2026-10-14. Recommended first delivery: a minimal additive public operation
and optional CUDA v4 table, using the existing context/registry/lifetime code.
This is broad-phase carrier-box candidate discovery, not intersections or
trimmed-face contact. No product implementation or native execution here.

## Existing machinery and extension choice

Read native/runtime.cpp: Object/Context/Registry, Admission, publish_owned,
cleanup_child, backend_status/final_cuda_check and the lazy v2/v3 loaders.
Read native/cuda_backend.cu: Operation, DeviceScope, guarded, checked allocate/
upload/download, finish, Pending, hierarchy/discovery callbacks and factories.
The public discovery headers show the current exact-record/alias/diagnostic
conventions. These are active owned source entry points, not adopted references.

Use separate public pair descriptors and a v4 optional backend table whose
base is the exact v1 pointer already retained by the context. Preserve v1/v2/v3
layouts and exports. Probe lazily; missing symbol or stable unsupported refusal
affects pair operations only. Fatal/runtime failures pass through backend_status
and are never cached as capability refusal. The factory is static and pure.
A standalone helper would duplicate device selection, streams, error mapping,
ownership and poisoning while complicating borrowed contexts; it offers no
clear benefit for this resident public operation.

Two new resource kinds suffice: BoxIndex owns a copied immutable six-double
world AABB per item and an optional resident device buffer; PairPage owns a
fresh u32 mask and retains both index objects. No separate query upload/token
is needed for two index handles plus scalar page options. bg1_close handles
both kinds. Result metadata supplies index counts, start, examined count,
word count, next start, total pairs and optional profile scope; downloading
actual index storage supports the existing independent input qualification
pattern. Body IDs/face/chart lineage can stay in the immutable Python mapping,
without importing body topology into this native box operation.

## Deterministic bounded page contract

Choose Cartesian order k=i*nB+j. Checked uint64 multiplication defines total
T=nA*nB before any allocation or division. Require start<=T and a positive
requested capacity <=2^20 pairs; count=min(capacity,T-start). Empty indexes
or terminal start yield a valid zero-word page. Never divide by nB when count
is zero. next=start+count is safe because count<=T-start.

Each thread owns a word and initializes its local mask to zero. For bit b,
k=start+32*word+b is evaluated only when 32*word+b<count. Set that bit exactly
when the two closed boxes overlap in all three axes; touching endpoints and
degenerate boxes are admitted. Write the word once. CPU workers partition word
ranges with the same mapping; CUDA uses a bounded grid-stride word loop.
Tail bits are zero by construction. No atomic append, device candidate counter,
compaction order or output-overflow truncation is needed. At maximum capacity,
32,768 words occupy 131,072 bytes. Fresh page allocation remains inside the
public resident-call wall timing.

Continuation concerns examined Cartesian pairs, not the number of set bits.
A caller obtains complete coverage only by traversing contiguous pages from
zero to T for the same immutable index pair; one page's completion is not full
coverage. Expose next/total rather than silently declaring a partially paged
operation complete. The mask preserves all candidates even when every bit is
set. Enumeration is a bounded host consumer of the mask, not part of native
candidate admission. Start/count/products and host/device byte extents need
checked arithmetic; item limits remain an explicit design choice.

## Admission, thread and failure details

Reject the whole index for any NaN/infinity or lower>upper; no per-item status
slab or arbitrary coordinate clipping. A finite ordered binary box is accepted
as supplied. Generic comparisons require an explicit decision about subnormals:
CPU SSE comparisons under inherited DAZ can differ from ordinary binary64
ordering. Either validate the existing RN/gradual-underflow environment on the
caller and every CPU worker, or use exact finite-bit ordering with both signed
zeros mapped to the same key. The latter makes the comparison-only predicate
independent of rounding/DAZ and is worth considering before introducing an
environment restriction. It is not a justification for omitting the body
adapter's separate outward-arithmetic environment contract.

Acquire one index through Admission, then resolve/check the second index under
the same context lock. Require identical retained Context identity and the
correct kind. Do not lock two arbitrary contexts in input order: opposite calls
would introduce a deadlock. Recheck registry/public-open semantics consistently
with current admission; wrong/closed/foreign handles fail before work.

Use existing owned host snapshots, device_children accounting and publish_owned.
Closing public index handles after page creation must not invalidate the page's
retained source objects or device buffers. Context close invalidates public
operations while private ownership keeps its stream alive through final cleanup.
CPU worker construction failure joins started workers before unpublished output
is released; zero-word pages need no workers or CUDA launch.

CUDA upload/evaluate/download/close follow guarded + Pending + DeviceScope:
checked allocation, async work on the owned stream, launch-error check,
finish, restore the previous device, then publication. A failed partial upload,
kernel, copy, restoration or cleanup produces no successful page. Preserve the
first diagnostic while propagating later fatal flags. Existing public alias
preflight writes nothing on rejection; backend copy failure invalidates all
host destinations. Exact word capacities and masked tail are part of the result
contract. Optional events remain off by default and their stream bracket is
not automatically pure-device busy time.

## Existing-body adapter and cost scopes

For each immutable source snapshot, create depth0/AABB hierarchies using the
existing CPU path, copy roots once and transform local lower/upper to world
coordinates. A temporary CPU Engine avoids a pointless hierarchy GPU upload
and download before uploading the final world index; alternatively reuse an
existing hierarchy, but record that setup/transfer work. Cache the resulting
world boxes and exact snapshot-to-slot mapping. Never rebuild them per page.

For positive common scale s and origin o, enclose each endpoint with outward
elementary operations: lower=down(o+down(s*l)), upper=up(o+up(s*h)). RN-nextafter
must operate in the admitted nearest/gradual-underflow environment with no
implicit fused multiply-add. The current CPU hierarchy creator validates that
environment for nonempty inputs; performing world conversion on that same
validated thread without intervening environment changes is a narrow usable
recipe. Make that linkage explicit or provide a small named environment check;
do not assume Python nextafter alone repairs arbitrary rounding/FTZ settings.
Current body Frame bounds make world overflow irrelevant, but still reject
nonfinite transformed endpoints. Empty snapshots perform no conversion.

Use the same copied world-box bytes for CPU and CUDA indexes. Different source
frames become comparable after conversion into their common world coordinate
system. A carrier AABB includes trimmed-out portions, so overlaps are candidates
and may be false positives. Disjoint conservative boxes can exclude carrier
pairs; the adapter's enclosure qualification supports that geometric claim,
whereas the native generic box operation only promises the exact box predicate.

Measure CPU hierarchy setup, world conversion, native index upload and Python
mapping once; per-page resident evaluation includes fresh output allocation;
download, bit enumeration and lineage expansion are separate host scopes.
Cold and export-inclusive caller wall remain separate from cached traversal.
No speedup or GPU topology claim follows from this design study.

## Coordinator scope decisions

The integrated direction chooses finite-bit ordering for validation and the
shared host/device overlap predicate, normalizing both signed zeros to one key.
Host memcpy/device bit-cast avoids arithmetic and comparison dependence on the
FP environment. Each index is capped at 65,536 items; T<=2^32, but retain uint64
counts/cursors so terminal 2^32 is representable. Pages have max_pairs in
1..2^20 and start in 0..T. Explicit flags distinguish REACHED_END (next==T)
from FULL_DOMAIN (start==0 and count==T); PROFILED describes the optional event
scope. This avoids treating a suffix page as full-domain coverage. Empty-domain
flag behavior follows those same equalities.

For the body adapter, exact Fraction endpoint transformation is a useful
alternative to two outward operations: form x=exact(o)+exact(s)*exact(endpoint),
convert to a float y, compare exact(y) to x, and move y one representable value
toward the required side only if it lies inward. Verify exact containment after
correction before admitting the world box. This reduces unnecessary widening
and makes the containment check explicit. The existing nonempty depth0 builder
still supplies its RN/gradual-underflow environment guard on that same caller
thread; no unchecked arbitrary-environment guarantee for float conversion is
claimed. Setup-only integer/Fraction cost must be measured separately. No
Fraction computation belongs in the native pair kernel. The primitive ABI
accepts only validated copied boxes; body source provenance stays in the owned
immutable Python adapter.
