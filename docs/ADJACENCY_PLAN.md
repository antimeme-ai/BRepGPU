# GPU adjacency implementation plan

Status: accepted and implemented; qualification and the 24-case cost campaign complete, 2026-10-14.
Issue [23](https://github.com/antimeme-ai/BRepGPU/issues/23).

## Complete units and their owners

1. Root integrates research, reviewed design and this plan. No implementation
   until design and plan reviews accept the complete contract.
2. Native construction owner implements the complete ABI5 CPU/CUDA operation:
   public standalone header, admission, allocation and cleanup, count/scan/fill,
   reader and separate CMake target. Owning one CPU/GPU semantic invariant avoids
   split owners at scan/fill boundaries. Add native direct admission and guarded
   fault tests as part of the same unit. Existing ABI1–4 code is not modified.
3. Root owns Python prepared source records, packing/permutations, ctypes
   synchronous invocation and immutable result reader as one interface unit.
4. Oracle owner independently implements literal primitive truth and dictionary
   relation qualification/tests as one oracle unit, after the exact interface is
   reviewed. Do not import native construction helpers into expected results.
5. Topology reader/caller owner implements a usable export caller and serial
   cost campaign after wrapper API is present. It consumes actual native CSR,
   kinds/mates/neighbors, writes typed source TSV/binary outputs and an owned SVG.
   Root integrates performance interpretation, README, journal and return notes.

## First red, then the whole operation

Before implementing native or wrapper code, add the direct cylinder use test:
prepare canonical cylinder, build adjacency, compare complete expected signed
membership and literal coedge4↔6 wall↔wall mates. Run on the fleet using the
healthy existing body factory/core environment. The expected initial failure
is the absent new operation, not a compiler, SSH, old-library or environment
failure. Record that exact failure. Write the whole prepared-interface subplan
and native subplan in the journal before implementing their units.

Implement CPU plus Python enough for the literal test; run direct green on CPU1
and CPU24. Implement CUDA count, owned recursive scan, fill and actual reader;
run the same direct truth on each P100. Independent conceptual units can
progress concurrently after plan acceptance, but fleet execution/builds are
serialized by root and no local product tests are run.

## Native boundary

ABI5 descriptor: version/size/backend/device/threads/reserved, then uint64 E,F,C.
Versioned ABI information reports descriptor/timings/diagnostic sizes and whether
CUDA is compiled. C call takes exact C-element slot/face/sign inputs; exact E+1
and C output extents; kinds E; mates/neighbors C; versioned timing output and
existing diagnostic shape. All extents are explicitly named and documented.
Maximum E,F,C = 2^22. Check all extents and checked pointer ranges/alignment,
including descriptors/timing/diagnostics and output aliases, before writes.
Read slot/sign contents only after that structural admission. Timings include
admission/context/allocation/upload/count/scan/fill/reader/download/cleanup/total.
CPU thread count 1..24, single CUDA device, no fallback or device pointers.

For CUDA, owned recursive 256-lane padded scan exercises multi-level block totals;
use one owned stream and synchronize phase boundaries for honest host-span
measurements. No degree-dependent sorting or quadratic reader. Degree>2 has
complete membership and invalid mates. Cleanup and restore happen before output
publication; a failed cleanup prevents success. Test guarded allocation/phase
failures where useful in separate tooling builds, not environmental mock claims.
Native output destinations can contain partial data after runtime failure and
must be discarded. ctypes keeps all input/output buffers alive during the call.

## Prepared Python interface

`Source(instance, body_id, revision, entity_id)` is frozen; containing source
arrays supply kind. `TopologyInput` validates full typed key uniqueness, one
body/revision per instance, all reference lineage, bounds and signs, then caches
owned immutable binary C-element input arrays. `pack_bodies(bodies)` flattens
owned immutable bodies with occurrence ordinals; a replication helper explicitly
repeats the same snapshot as separate instances. A permutation method remaps
edge, face and coedge storage without changing sources or original snapshots.
`AdjacencyLibrary(path).build(input, backend='cpu', device=0, threads=1)` returns
`AdjacencyResult` with owned binary offsets/members/kinds/mates/neighbors, retained
input and timings. Methods read row uses and mate/neighbor source identities.
Native calls receive compact binary arrays; no JSON persistence or IPC.

## Independent qualification and review

Direct tests cover all literal primitive pairs/signs/seams, mixed batches,
repeated source bodies in separate instances, independently permuted slots,
empty edges versus empty coedges, degree0/1/2/3, same-face pair and sign conflict,
high-degree concentration and a multi-level scan (at least 65,537 edges, with
uneven nonzero degrees across tile boundaries and empty interior/tail rows).
Random deterministic incidence models exercise membership and reader independently.
Malformed source/instance/lineage/sign/range inputs reject before native access.
Direct C tests verify bad byte/count/range/alias admission leaves payloads
untouched; runtime fault tooling checks primary failures, cleanup and healthy
subsequent invocation where the injected failure is recoverable.

After unit greens, adversarial code reviews attack topology identity/consumer,
native runtime/scan/cleanup, and independent oracle validity. Fix every valid
finding and record dismissed ones. Qualify CPU1/CPU24 and both P100s (Compute
Sanitizer with zero error/leak summaries), plus a scoped host ASan/UBSan native
campaign and representative old body/pair/intersection compatibility through
unmodified archived core binaries. Direct production mutants include actual
CUDA builder/reader changes, CPU changes and wrapper provenance changes. Isolate
sources/builds under ignored fleet context; compile each mutant on the fleet,
require a real assertion for a kill and healthy baseline beforehand. Campaign
scope is named; no universal mutation score claim.

## Caller, costs and delivery

Run four actual callers spanning CPU and both GPUs, including a seam body and
hole plate. Qualify outputs from source records; emit binary arrays, typed TSVs
and a seam incidence SVG. Then run a serial fresh-process matched campaign:
small body batch, 512 and 32,768 repeated plates, each ordered and globally
permuted coedge/edge/face storage, CPU1/CPU24/P1000/P1001. This is 24 cases.
Each worker measures input packing/permutation, library load, cold first call,
independent initial qualification, one excluded warmup and five repeated calls
with full actual reader consumption. Every result is independently checked
outside its measured full caller interval. Report phase medians/ranges, Python
allocation/publication/reader costs and setup/cleanup separately. Record actual
input/output byte formulas, environment and source/binary hashes before/after
campaign. No builds, tests or mutations overlap measured trials.

Interpret kernel-stage versus native-total versus complete-caller value honestly.
Cold first call includes device initialization; repeated calls only reuse host
prepared input/library/runtime, not resident topology allocation. Large replicated
batches are disjoint primitive occurrences and do not establish large-single-solid
performance. Preserve timed data; make native/full comparison SVGs from real TSVs.
Record opportunities in RETURN_PASS and dedicated round-two notes; do not tune
persistent buffers or integrate into Boolean construction during this delivery.

Publish coherent commits for research/design, accepted plan/red test, native and
wrapper implementation, oracle/reader caller, reviews/qualification, and measured
results/docs. Open a reviewable PR, preserve feature commits in an authorized
merge, retain branch and synchronize the owned fleet checkout with main while
leaving older binaries/log archives intact. Every action has a journal entry.
