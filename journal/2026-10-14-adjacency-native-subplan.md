# Native adjacency unit subplan

Purpose: implement the complete bounded ABI5 incidence constructor and its actual
mate/neighbor consumer, with CPU1/CPU24 and sm_60 CUDA paths, while leaving
existing ABI1–4 code and binaries intact. Plan/runtime review is accepted; native
implementation waits for root's direct missing-operation red on the fleet.

## Interface

The agreed `bg5_desc` is 48 bytes: uint32 version,size,backend; int32 device;
uint32 threads,reserved; uint64 edge_count,face_count,coedge_count. CPU device is
zero, CUDA device nonnegative, threads 1..24, reserved zero, counts <=2^22.
`bg5_info` is 24 bytes: version,size,desc_size,timings_size,diagnostic_size,
cuda_compiled. `bg5_timings` is 96 bytes: version,size and eleven doubles named
admission_ms,context_ms,allocate_ms,upload_ms,count_ms,scan_ms,fill_ms,reader_ms,
download_ms,cleanup_ms,total_ms.

`bg5_abi_info(info*,bytes)` writes output-only ABI information after checking
alignment, nonnull storage, known minimum capacity and pointer extent wrapping.
`bg5_adjacency(desc*,descBytes,edgeSlots*,faceSlots*,signs*,inputCount,offsets*,
offsetCount,members*,memberCount,kinds*,kindCount,mates*,neighbors*,readerCount,
timings*,timingBytes,diagnostic*,diagBytes)` accepts input slots uint32 and signs
int32; all payload outputs are uint32. Input count is C, offset count E+1,
member and reader counts C, kind count E. Struct byte capacities may exceed known
size; desc.struct_size stays canonical. Timings/info/diagnostic are output-only.
Diagnostic may be null only with zero bytes. Existing brepgpu.h contributes
status enums and the 288-byte ABI1 diagnostic, not a runtime dependency.

## Boundary and execution

1. Check known struct extents/alignment and checked pointer ranges without writes.
   Admit descriptor, inputs and outputs and all forbidden aliases before slot or
   sign reads. Unsafe diagnostic ranges receive no writes. Check descriptor
   version/size/reserved/backend/device/thread/counts and exact numeric extents.
   Validate all input edge/face slots and signs before output writes/allocation.
2. CPU counts and cursors use owned atomics for threaded ranges; prefix scan is
   serial and disclosed. A scoped worker group joins all launched workers on
   phase completion or partial launch failure. Worker tasks do not throw.
3. CUDA creates one stream under saved/selected device ownership; allocates all
   inputs/outputs/counts/cursors and recursively sized scan scratch as call-owned
   buffers. Every successful allocation is recorded immediately. Phase clocks
   synchronize owned stream; check launch and completion errors.
4. Count, recursive padded 256-lane exclusive scan, uniform-add and final C
   sentinel, atomic fill, then actual reader. Initialize mate/neighbor sentinels
   before reader. All padded lanes reach barriers. No zero-block launch.
5. Reader degree two inspects actual CSR members/signs/faces and writes each
   distinct coedge's mate/neighbor. Degree zero/one/>2 and sign conflict have
   explicit kinds and no fabricated unique mate. Physical row order unspecified.
6. Download all outputs, synchronize, attempt every cleanup and restore prior
   device. Primary failure wins; append secondary cleanup/restore failures.
   Runtime failures may leave partial output destinations but return no success.
   Publish timings only after successful cleanup/restoration.

E=0 emits offsets[0]=0. C=0/E>0 emits all E+1 zero offsets, E unused kinds and
empty member/reader payloads. Prefix/intermediate sums fit uint32 from C bound;
byte products, pointer extents and allocation hierarchy are checked before use.

## Files and verification

Own new standalone public header, native CPU/admission source, native CUDA source,
private shared declarations, root CMake target and native direct test executable.
Use a separate build/topology-release tree. A guarded tooling target includes
fault controls; production library has no exported test control or active fault
state. Direct native tests attack all admission no-write clauses, successful
literal semantics, partial CPU worker launch, allocation/phase failures and
secondary cleanup/restore diagnostics, with healthy retry for recoverable faults.
Faults simulate named branches while real allocations/resources still clean up;
they are not represented as observed environmental CUDA failures.

Root serializes fleet builds/runs. First literal cylinder green runs CPU1/CPU24,
then both P100s; independent suite includes 65,537 edges for multi-level scan,
high degree and empty cases. Run scoped ASan/UBSan tooling and each P100 under
Compute Sanitizer. Keep builder/reader functions explicit so direct production
CUDA and CPU mutants target real emitted values, not an unused test helper.
Adversarial code review and valid-finding fixes precede native qualification.
No persistent allocation pool, general resource registry or external scan library.
