# Intersection caller and campaign: topology/source review

2026-10-14. Reviewed [the caller](../examples/intersect_faces.py) and
[campaign](../benchmarks/intersections/run.py) as the downstream half of the
same result/source/lifetime invariant. No native execution or measurements.

**IC1 — medium timing scope clarification.** `actual_whole_caller_ms` currently
stops after geometry/sample/pair exports and SVG, before `quality.tsv`,
`input.tsv` and `timings.tsv`. Move quality/input publication before the stop
and exclude only final timings, or explicitly name/document a through-SVG
scope. No individual phase median should stand in for this wall interval.

**IC2 — low source provenance omission.** The campaign imports engine cleanup,
timer and TSV helpers from `examples/discover_pairs.py`, but omits that file
from `sources.tsv`. Add the actual helper source to the manifest. The explicit
unchanged core/plugin hashes correctly identify native code.

Other source behavior is coherent. Both literal recipes consume and qualify
the complete four chart-pair bits and actual resident bounds, group into one
face-pair operation, and independently check full-circle/four-arc counts.
Unsupported or empty unexpected geometry is not silently a complete scene.
The caller engine scope and owned consumers preserve BaseException failures
through cleanup. Fresh output directories prevent receipt replacement.

Exports retain original controls, exact map coefficients and Scalar parameters,
physical components/spans, raw chart occurrences, events/incidences, original
source slots and actual returned native descriptor/slot/UV buffers. Approximate
samples do not decide graph identity. The three SVG panels use actual returned
source positions/UVs, show the owned wall atlas lift and label physical versus
representational counts. Their reference circle/rectangle is appropriate only
to the explicitly supported two literal recipes; no general meshing/chord or
Boolean claim is made.

Geometry upload of 976 bytes follows five source patches and 28 homogeneous
controls. Each two-sided candidate uploads 40 logical query bytes and downloads
56 position/status bytes. Pair index upload/resident-box download is 240 bytes
each and the four-bit mask occupies four bytes; CPU receipts report zero CUDA
traffic. Host descriptors and Python records remain separate host work.

The sixteen fresh-process cases preserve recipe/resolution/backend/device/
threads/component/piece/candidate/native-row condition keys. One excluded
warmup precedes five reused calls; product grades and provenance checks remain
inside every returned sample operation. Independent initial qualification,
construction/request preparation and final teardown are separate. No profile
events alter the main wall matrix and no claimed GPU win follows before data.
Findings sent to root for integration before the measured campaign.

## Corrected source disposition

**Accepted; IC1 and IC2 resolved.** Re-read the corrected caller: all non-timing
exports, qualification/input files and used-source/binary manifests precede
the full wall stop. Only final timing-file and console publication are excluded,
with an explicit source comment. The campaign includes the imported
`discover_pairs.py` helper and body oracle source in its manifest. Its added
before/after source and binary checks preserve the advertised matched-byte
campaign without altering worker timers. No remaining source blocker found.
