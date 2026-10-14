# CUDA mode-specialization numerical code review

2026-10-14. Accepted source delta, no findings or unresolved blockers.
Reviewed against the [accepted plan](../docs/CUDA_DISCOVERY_MODE_PLAN.md) and
[numerical design review](2026-10-14-discovery-cuda-mode-specialization-numerical-review.md).
No product edit, test, native compilation, fleet operation or timing measurement
was performed by this reviewer.

## Source checkpoint and scope

`native/cuda_backend.cu` SHA-256:
`0823748fbec55d21f8b3c76be06d101edaa0b3a8082b08d59a5af2a2488447bd`.
The read-only source diff contains only this product file: 25 inserted and
5 removed lines. The dynamic source hash was
`3da09294aa1d291218344fa004ea0db638f331681ceb0fe633811e2430cda4ca`.

Checked unchanged identities:

| Source | SHA-256 |
|---|---|
| `native/discovery_math.hpp` | `7a3fe53a15a1e58613432b431ccf6ce5c638b46a40a043f7a42cd53f4e322f39` |
| `native/projection_solver.hpp` | `21bf3d9df559793f7df7c511e041378354a7efacaa8ce535062f1aa6d7671fd7` |
| `native/discovery_cpu.cpp` | `4968ac0064a0aa030369ec7358fbba6ed42c2ebbe908dcb7d80652b24d6a11df` |
| `native/evaluator_cpu.cpp` | `32233de7bc4caa15c89500843d044286fd5568aa2e239e43f4730bf2d46260f8` |
| `CMakeLists.txt` | `019935844713c66f263aee67b22f5854fea43351a6b10f70282049230b79ac00` |
| `python/brepgpu/__init__.py` | `f4e657f4608feb52721238d4a6051d6750fb86c012803a6df195e09e722f570e` |
| `benchmarks/discovery/benchmark.cpp` | `708f728f48563f2d5f866469b939f1c7711ff8467b4d13170413c5973244efd7` |

Public/private headers, tables/factories, CPU sources and benchmark/adapter sources
are outside the actual delta. Their selected core/benchmark binaries remain the
coordinator's preservation/identity responsibility.

## Numerical and result invariants

The kernel adds `template<unsigned Mode>`, removes the runtime mode argument and
passes Mode to the same `discover_local` call. The assertion restricts the unsigned
constant to 0/1/2; the host code instantiates exactly AABB/DOP26/Bernstein. No active
mode arithmetic expression, cache index, directed-rounding intrinsic, bound
combination or projector operation is edited.

The row loop keeps 64-bit grid-stride indexing, invalid-slot first precedence,
root/source membership checks, original `controls+patch.offset*4` address, degree
arguments and full query count. Every double write remains `fields[k*count+i]` for
k=0..7. The six integer writes retain status/flags/visits/pruned/refinements/
evaluations in exactly the old order and meaning. Failed payload NaNs still come
from the unchanged shared helper's canonical CUDA bit pattern.

Constant propagation can discard unused DOP/Bernstein paths and prepared-target
members, but does not change the written active primitive sequence. AABB still
does not read the zero-extent cache. DOP keeps its ten support pairs and norm
divisors. Bernstein keeps sign-dependent positive-denominator division and its
degree-dependent active extent rather than padding. Strict pruning, left-first
ties, root upper, capped root lower, child-pair visit reservation and root-once
refinement remain in the byte-identical shared source. Counter changes would
therefore be a qualification discrepancy, not a desired behavior change.

The small private `launch_discovery<Mode>` helper packs every former launch
argument once. It passes the same retained patch/hierarchy/query pointers,
counts, immutable options copy, context stream, output field pointer and integer
slabs 0..5. No new resource, callback ownership or numerical parameter is added.

## Host dispatch and callback boundary

After existing argument/context/count admission, `discover` reads the retained
hierarchy mode and rejects values above BG_BOUND_BERNSTEIN before pending result
construction, device selection, allocation or empty handling. Out remains null
and profile remains the initial zero-flags/NaN record. The private hierarchy
pointer is an existing retained valid-token precondition; public upload already
admits only the three modes. This defensive branch is source-reviewed, not claimed
to be covered by public invalid-option rows.

Inside the existing device scope, host dispatch has three explicit cases with
the correct corresponding template argument. Threads remain 128; block count
remains ceil(count/128), capped at 4096. An impossible default records Operation
error instead of fallback or an early return past cleanup. `cudaGetLastError` and
the stop event are conditioned on Operation success after the switch. The default
therefore cannot publish or claim a measured result.

Start/stop event creation/recording, stream finish, elapsed-event read, event
destruction, outer device restoration and final token release retain their prior
locations. Empty batches skip events and launch. Fatal/error aggregation and
Pending drainage are unchanged. The event bracket still includes any intervening
host submission/idle gap, as documented in the corrected design. This review found
no changed profile-on/off numerical scope or output-allocation timing scope.

## What remains for integration

The coordinator reports actual pre-delta focused baselines passed on both P100s
and captured 309 corpus rows plus 132 batches against each preserved dynamic
plugin/core. This is attributed baseline evidence, not execution by the reviewer.
Source acceptance does not establish compiled constant-path elimination, raw-bit
identity, independent containment, resource counts or speedup.

The accepted plan's CUDA-only build, raw field/slab and deterministic metadata
comparison, one existing Fraction qualifier per new plugin/card, focused
sanitizer/device/profile checks and v1/v2 smoke remain the appropriate direct
integration mechanisms. Preserve the scalar CPU measurement queue and retained
binary identities. No new numerical reference, CPU campaign or repeated mutation
matrix is required by this bounded code change.
