# Native presentation peer review

Read actual presentation.hpp/.cpp against docs/native-mesher-cli-unit.md before
CLI integration. Definition boxes use stored XYZ; every occurrence transforms
all eight corners with its actual retained matrix. The generic outward
explosion uses occurrence centers and a shared assembly center. Camera basis
orientation and frame-specific bounds are consistent; frame positions remain
presentation values, independent of semantic mesh positions/keys. Text/labels
are keyed by canonical occurrence ordinal. PNG stored-DEFLATE block lengths,
last-block flag, RGB row mapping, CRC32, Adler32 and chunk sizes read sound.

Found a real small-height label issue: negative available label height was
converted to size_t before capacity/placement math. Sent it to root; actual
source now rejects nonpositive height before unsigned conversion. Root also
bounded text drawing at canvas width. These fixes preserve geometry and avoid
cosmetic integer overflow/long walks. Root independently qualified literal
multiblock PNG bytes through standard zlib and small-height refusal. Scoped
actual-source review ACCEPT after rereading the fixes. No independent duplicate
PNG campaign was launched by this reviewer.
