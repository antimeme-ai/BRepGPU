# Pair shortlist performance artifact review

2026-10-14. Accepted; no numerical-record blocker. Read
[matrix artifacts](../results/pairs-2026-10-14/matrix) using standalone stdlib
CSV/statistics/hash analysis. No product imports, native execution or fleet work.

All 304 finite nonnegative raw rows and 112 summaries are consistent. Every
median/minimum/maximum recomputes exactly. The eight cases occur in the planned
4/128-bodies-per-side and CPU1/CPU24/P1000/P1001 order; each has eight single
observations and six repeated phases at exact iterations 0–4. Summary keys are
unique/complete; per-case raw files match the aggregate. Metadata and qualified
counts match: 784 examined/43 survivors, or 879,840 examined/1,823 survivors.
Every repeated component span fits within internal wall, and internal wall plus
that call's decoding fits within external actual-shortlist wall. No independent
phase medians were added to construct latency.

## Calibrated headline

Actual returned-and-decoded shortlist medians in milliseconds:

| Bodies per side / examined pairs | CPU1 | CPU24 | P1000 | P1001 |
| --- | ---: | ---: | ---: | ---: |
| 4 / 784 | 0.144 | 1.586 | 0.295 | 0.341 |
| 128 / 879,840 | 19.182 | 9.631 | 7.776 | 6.920 |

Large-case caller ranges are 18.860–20.595, 9.181–10.321, 7.305–8.774 and
6.635–7.260 ms respectively. These five-call observations show lower cached
large-case caller latency on both P100s; descriptive CPU24/GPU median ratios
are 1.24 and 1.39. CPU1 wins the small case. Neither result supports an unconditional
GPU advantage or full-CAD speedup.

For the large case, native-page host-span medians are 12.330/1.864/0.144/0.125 ms;
download/copy medians are 4.391/4.977/4.925/4.268 ms; host survivor decoding
medians are 2.159/2.687/2.707/2.214 ms. Download includes actual mask retrieval,
metadata checks and immutable word copying. Native-page timing includes the
synchronous public call, fresh output allocation/publication and wrapper work,
not pure kernel time. No events are enabled in this matrix. These phase values
explain why the caller gain is smaller than the native-span difference, without
reconstructing its wall from phase medians.

## Cold/setup, transfer and provenance scopes

Large CPU scene/root/world preparation costs 188.745–202.314 ms in the four
single observations for that size; large index setup/upload is
3.588–5.231 ms. Cold-to-first-returned-page is 393.143/344.965/640.247/839.898 ms.
That interval begins before body construction and ends after the first returned
page, before survivor decoding, exact qualification and final index/Engine
cleanup. It is not a cold complete-shortlist/export caller. Large initial exact
qualification costs 3.353–3.506 seconds separately. These are single observations,
not averaged setup estimates or a cold filesystem/driver-cache claim.

Transfer arithmetic is correct: small indexes have 28+28 boxes, hence 2,688 CUDA
upload bytes and 25 words/100 download bytes. Large indexes have 936+940 boxes,
hence 90,048 upload bytes and 27,495 words/109,980 download bytes. Each world box
is 48 bytes; each mask word is 4 bytes. Topology upload is zero. Upload occurs once
for cached indexes; mask allocation/download recurs per call. Python/host
snapshots and decoding add traffic beyond these payloads.

All 17 source identities match current local bytes, including both owned native
implementation fragments. The recorded core/plugin hashes match the actual new
library identities in the preserved-binary compatibility artifact. Actual native
hashing and protected-artifact preservation remain coordinator-attributed;
no native binaries were loaded by this reviewer. Public artifacts contain
relative labels/hashes/numeric values, with operational logs private.

The matrix lacks a standalone METHODS.md at review time; the final report must
state the scopes above. The reviewed driver runs blocking subprocesses serially;
the coordinator owns the no-overlapping-owned-jobs condition. A fixed-order,
unreserved, clock-uncontrolled shared host and five samples support descriptive
ranges only. Carrier-box survivors remain candidates, not trim contact,
intersection or Boolean results. Export is outside this matrix.
