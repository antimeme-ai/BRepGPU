# Pair caller and campaign source review

2026-10-14. Reviewed [discover_pairs.py](../examples/discover_pairs.py) and
[the campaign runner](../benchmarks/pairs/run.py). Source inspection only;
no native execution or performance measurements were performed.

## Findings

**PC1 — preserve an active BaseException during engine cleanup.** Both the
caller and benchmark use shared `Engine.__exit__`, including the CPU engine
in `prepare_inputs`. The current shared implementation catches `Exception`,
not `BaseException`, and does not guard cleanup-note attachment. Consequently
engine cleanup raising `KeyboardInterrupt` or `SystemExit` can replace a
primary failure, even though `PairDiscovery` and scene-child cleanup preserve
it. Use a caller-owned guarded engine scope that attempts close, preserves an
active primary and guards attachment; normal close failure remains an error.
The same helper can serve both scripts because the runner imports the caller.

**PC2 — label the matrix crop.** `render()` draws only the first 100 charts
per side for a larger scene, while its captions give complete scene counts and
complete survivor counts. Those counts are true, but the drawing does not
identify its omitted rows/columns. State the shown dimensions and that the
full mask/shortlist are exported. A cropped dark matrix cell still correctly
means an excluded carrier pair; nothing outside the drawing is depicted.

**PC3 — include geometry preparation in source provenance.** The runner's
source list includes pair service/client files but omits `body.py` and the
existing discovery builder used to create actual carrier roots. Add these
sources, or retain a complete source-commit identity alongside the listed
hashes. Core/plugin hashes already identify the actual binaries. This affects
reproducibility of geometry preparation, not the mask predicate contract.

## Accepted behavior

The deterministic recipe uses canonical box/cylinder/plate snapshots with
different body namespaces, admitted dyadic origins and positive power-of-two
scales. At size128, the sides contain 936 and940 charts, giving879,840 pairs,
below the1,048,576 page cap. A complete-page flag is required after exact
qualification; this bounded caller therefore covers the entire selected
Cartesian domain. Direct tests separately exercise pagination.

The caller qualifies original-control/root/world enclosure and the actual
resident input boxes, then qualifies every actual mask bit and source. The
export binds flat indices to typed body/face/chart occurrences, retains local
and common-world bounds as binary64 hex, and writes an explicit little-endian
page-local u32 mask. Colored cells are actual survivor bits; no intersection
curve, trim overlap, contact or Boolean is invented.

The campaign has eight fresh-process cases, one excluded warmup and five
repeated calls. Actual shortlist wall includes page creation, copied mask
download, child cleanup and CPU survivor materialization. Initial exact truth
is outside reused timings; repeated pages retain metadata/source checks and
exact-qualified mask identity. Cold body/root/world preparation and uploads
remain separate. Event profiling is absent from the wall matrix. The caller
also records export and a full preparation-through-export wall interval,
ending before final timing-file publication. Independently measured phases
are not summed into an asserted wall latency.

Source and graphic findings were sent to root for integration before execution.

## Corrected source disposition

Re-read the integrated fixes. **Accepted; PC1, PC2 and PC3 are resolved.**
All CPU-preparation, caller and benchmark engine scopes now use the caller's
`engine_scope`, which catches BaseException, guards cleanup-note attachment and
uses bare `raise` to preserve the active primary; ordinary close failure still propagates.
The large-scene SVG states the first shown chart dimensions and bounds its
right-body caption with a pointer to the complete source export. Provenance
now includes body construction, discovery builder/math and CMake sources,
alongside pair fragments and actual binary hashes. The caller also records
actual qualification-input download separately. No remaining source blocker
was found; native execution remains root-owned.
