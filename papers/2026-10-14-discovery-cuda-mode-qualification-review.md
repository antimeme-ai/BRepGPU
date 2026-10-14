# CUDA mode experiment qualification and compiled-observation review

2026-10-14. Focused read-only review of retained qualification captures, exact
qualification artifacts, local integration logs, static disassembly/ELF text and
current/restored source. [Experiment report](../results/discovery-modes-2026-10-14/REPORT.md).
No native execution, tests, builds, fleet access or product edits. Timing/data/
figure/acceptance-decision review belongs to the separate performance reviewer.

Disposition: no qualification or compiled-observation reporting blocker found.
The report accurately separates finite-case correctness from a numerical theorem,
static resources from occupancy/performance, and the rejected candidate from the
retained dynamic product.

## Actual output and input identity

Read both variants' actual retained binary payloads and TSV indices for each card
under [qualification/identity](../results/discovery-modes-2026-10-14/qualification/identity/).
Each variant/card contains 132 payload files, 2,367 indexed parts and 946,319 total
payload bytes. Every old/new payload file matches exactly, with identical indices,
contiguous checked extents and no unindexed trailing bytes. This is raw byte
comparison, not a tuple/hash-only comparison; it preserves signed zeros and NaN
payloads along with the six integer output slabs.

Payloads include declared deterministic info fields, descriptors, slots, targets/
UV, projection seeds, options and original input controls/labels. Metadata agrees
for source/client identities, core, benchmark, device and native byte order/width;
only variant label and actual plugin identity differ. Durations remain excluded
from numerical identity but all 132 records per capture have the required
profile/count/flag and finite-nonnegative-versus-NaN duration shape. The current
retained client/oracle source hashes match the capture snapshots.

| Role | Actual SHA-256 |
|---|---|
| Preserved CPU core | `3515b86d0aad1f4bac816b8cc720e2f9452759f2806b6e1938854eaf4fb355de` |
| Preserved native benchmark | `39ded902fc6b005204357524a29449b28e07b3766b10b3fb7e05d7a36be1d8c1` |
| Dynamic plugin | `8a6a861c17c7c2c22a0fafe40ce8cc2b46b33272322c8b48af9ce5af24d27efe` |
| Specialized plugin | `80d4e829091f5b332b806b2200d9ef09f874c9775cd9adf83fb924567b9ad8da` |

The two cards' index files have the same SHA-256
`daf3aa0ea1daf7cd7d7a29a34963c8d52a0e274375698e37ceea22957a0c49e8`.
Metadata is intentionally different across cards and variants where device/plugin/
label differs. The capture client hashes the selected binaries before and after
collection and preserves source snapshots in metadata. Retained integration
comparison logs report the same 309 corpus rows and 132 total batches on each
card, consistent with the read payloads. These observations establish the promised
old/new identity for these inputs, not correctness by CPU parity.

## Independent geometry and device checks

The candidate [card 0](../results/discovery-modes-2026-10-14/qualification/cuda-0/QUALIFICATION.md)
and [card 1](../results/discovery-modes-2026-10-14/qualification/cuda-1/QUALIFICATION.md)
qualification each contains 309 request records, 11,415 downloaded node records,
zero per-row violations, and a failure TSV containing only its header. Summaries
agree with these cardinalities and declared explicit CUDA device selection.
The retained exact qualifier source uses supplied-binary64 Fraction power
restriction/convolution, method-specific flat/root lower ceilings, original-UV
upper inequalities and analytic global cases; its allowances apply only to
descriptive XYZ/distance. No CPU output or old/new equality substitutes for those
geometry checks.

The original plugin's actual pre-change logs show all three focused tests passed
on each card. Candidate logs show those same three tests passed under Compute
Sanitizer separately on each card, with error summary 0 and leak summary 0 bytes
in 0 allocations. These are integration executions read from local retained logs,
not executions by this reviewer. They target mode dispatch, tails/invalid rows,
profile, retention/device restoration and v1/v2 analytic smoke; they are not a
complete proof of every possible CUDA fault or input. The experiment report
states that finite limitation correctly.

## Static compiled observations

Read retained ptxas output and candidate ELF parameter/resource metadata. They
agree on registers 206/254/210 for modes AABB/DOP26/Bernstein, each with a
448-byte stack and zero spill stores/loads. Projection retains 166 registers,
224-byte stack and zero spills. No reduced-register claim is made for DOP26.

Independently counted decoded instruction records in the retained SASS text:
dynamic 24,246; specialized AABB 21,594; DOP26 22,932; Bernstein 22,812, matching
the public table. Instruction counts exclude formatting/control-word-only lines;
they describe static decoded kernel bodies, not executed instructions per query.

The kernel signature makes the bound-cache pointer argument ordinal 6. ELF
parameter metadata records offset 0x30, and the sm_60 parameter-bank addressing
maps it to constant bank 0 location 0x170. Retained SASS contains 18 such pointer
loads in dynamic, 3 in DOP26, 15 in Bernstein and zero in AABB. This supports the
specific compiled observation that the AABB cache-pointer loads disappear.
It does not establish every abstract mode path was eliminated, actual occupancy,
memory traffic, dynamic work or useful performance. Zero compiler spills does not
mean no local stack storage. The report preserves those distinctions.

## Candidate preservation and restoration

Read archived [candidate commit ba44794](https://github.com/antimeme-ai/BRepGPU/commit/ba44794)
CUDA source: SHA-256
`0823748fbec55d21f8b3c76be06d101edaa0b3a8082b08d59a5af2a2488447bd`, matching
the independently accepted code review. The current worktree CUDA source is
restored to SHA-256
`3da09294aa1d291218344fa004ea0db638f331681ceb0fe633811e2430cda4ca`, the qualified
dynamic source. Both actual plugin identities remain separately present in
the retained stage provenance/captures; no mixed dispatch is introduced.

Restoration of the execution-host default plugin is attributed to integration's
reported action; this reviewer verified current local source and preserved stage
identities, not a live target filesystem. The archived candidate remains available
despite product rejection. The report's negative-experiment conclusion is therefore
consistent with the checked qualification/source state. Performance decision
arithmetic and figures are intentionally left to the independent data reviewer.
