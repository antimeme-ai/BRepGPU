# Native adjacency implementation

Root recorded direct fleet red against the healthy old body consumer: the new
adjacency module was absent. All design and plan reviews were accepted before
implementation. Implemented the complete native unit described by the subplan:
standalone ABI5 header, structural admission, CPU1 direct counters, CPU24 atomic
count/fill and serial prefix, actual CSR reader, owned recursive CUDA scan and
call-owned stream/buffer cleanup. Added a separate CMake target and guarded fault
tooling plus direct native admission/semantic checks. Existing native ABI1–4
sources were not modified.

Root's early static review identified unnecessary atomic increments on CPU1 as
a valid baseline issue. CPU1 now owns ordinary uint32 counters/cursors; CPU24
retains atomics. Both write membership through the same emitted-value operation.
The GPU final offset is a device kernel, so upload bytes are exactly 12*C and
download bytes are 4*(2*E+1+3*C); no hidden final-offset host transfer.

Native checks include literal edge/use relations for every reader kind, malformed
descriptor/extents/slots/signs and unsafe aliases with unchanged outputs,
E=0/C=0 distinctions, 65,537-edge multi-level scan and concentrated high degree.
Guarded tooling injects allocation/phase/worker/cleanup/restore branches while
real resources are owned and cleaned up, checks first/secondary diagnostics,
unpublished timings and healthy retry. Injection is explicitly not an observed
environmental CUDA failure. Ordinary binaries have no hook exports or active
failure controls.

Static diff inspection completed. No local product execution, native compilation
or fleet build/test was run by this lane. Root now serializes fleet builds and
direct greens; subsequent compile/runtime findings and review dispositions will
be recorded before qualification.
