# Delivered pair scenes, guide and graphics

2026-10-14. **Accepted; no geometry, scene-coverage or presentation blocker.**
Reviewed [the API guide](../docs/PAIR_DISCOVERY.md), retained
[caller outputs](../results/pairs-2026-10-14/caller) and
[matrix](../results/pairs-2026-10-14/matrix). Source and local TSV/binary/SVG
inspection only; no product imports, native execution or fleet activity.

The CPU24 and P1000 callers each retain 28×28 charts, 784 examined pairs,
43 survivors and 741 exclusions. Their complete masks contain 25 words with
zero padding. The larger P1001 caller retains 116×116 charts, 13,456 pairs and
228 survivors. Its labelled first 100×100 SVG shows 190 actual survivor cells;
the full shortlist and mask retain all 228. Decoding the retained masks matched
every exported flat index, left/right source index and body/face/chart tuple.
SVG cells match precisely the shown subset of those bits.

The source exports retain body revision, original patch slot, chart identity,
shape, frame and local/world bounds; separate patch/control tables preserve
original rational geometry and chart parameter rectangles. Four quarter-wall
charts still share one face ID. No coordinate welding, invented body edge or
GPU topology upload appears in these receipts. The guide correctly requires
every relevant chart pair to be examined and excluded before excluding a
whole face pair, and distinguishes a suffix reaching the end from full coverage.

The larger measured scene has 128 bodies per side: 936×940 charts and 879,840
examined pairs, with 1,823 survivors. This is a useful multi-body shortlist.
Its 878,017 exclusions count chart pairs,
not faces or bodies. Hole, tangent and concentric false-positive behavior is
explicitly preserved; survivor counts do not count true intersections.

The bar graphic reports actual reused shortlist caller wall, including native
page creation, mask download, cleanup and Python decoding. Its eight labels
retain scene size, CPU thread count or GPU ordinal; each reused summary has
five samples. CPU1 is faster than either GPU for the 784-pair case. For 879,840
pairs the GPU shortlist medians are 7.776/6.920 ms versus CPU24's 9.631 ms and
CPU1's 19.182 ms. The graphic separately names cold bounds/upload/qualification,
shared-host min/max ranges and the absence of event timing. Neither it nor
the guide infers full modeling speed or a CPU performance ceiling.

Two small guide clarifications were sent to root: qualify the byte-transfer
paragraph as CUDA-specific because CPU receipts have zero CUDA transfer bytes;
state that body/revision IDs are process-local snapshot provenance, consistent
with the [body guide](../docs/BODY_SAMPLING.md), rather than cross-session
persistent names. The retained control/frame/source tables already distinguish
the actual geometry of each exported scene.
