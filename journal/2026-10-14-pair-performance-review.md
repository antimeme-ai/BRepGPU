# Pair performance artifact review

Independently recomputed all 112 summary rows from 304 finite measurements in
the eight recorded cases. Checked complete singles/repeats/iteration sets,
unique summaries, raw aggregation, quality/count metadata, transfer bytes and
timer nesting; all passed. Verified 17 current source hashes and matched native
manifest identities to the preserved-binary compatibility artifact, without
loading product/native code.

Accepted the scoped headline: large cached caller medians 19.182/9.631/7.776/
6.920 ms on CPU1/CPU24/P1000/P1001; CPU1 wins the small case. Sent the precise
cold-to-first-page boundary and native-page host-span distinction to root, and
noted that final report must provide scopes since matrix METHODS.md is absent.
Saved papers/2026-10-14-pair-performance-review.md. No product imports, native
execution, tests, benchmarks, fleet jobs or Git mutations.
