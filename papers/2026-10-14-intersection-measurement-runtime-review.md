# Intersection caller and benchmark: runtime measurement review

2026-10-14. Accepted after the corrections below. Static review of examples/intersect_faces.py and
benchmarks/intersections/run.py against the accepted plan and actual sampler,
pair-scene and native upload/download paths. No product or performance execution.

## IR-M1 — source/binary attribution: resolved

The initial controller wrote source and binary hashes only before its matrix.
It did not reject drift before/after each fresh case or after the campaign;
changed code could therefore be attributed to the initial manifest. Imported
examples/discover_pairs.py also supplies the timing/cleanup/TSV helpers and must
be pinned. Root accepted before/per-case/after comparison and the extra source
pin. The standalone caller should retain source/binary manifests alongside its
actual descriptor/slot/UV buffers, without exposing operator paths.

## IR-M2 — complete raw case/iteration contract: resolved

Initial aggregation accepted any finite nonnegative raw rows. It did not require
complete phase sets, one setup row at iteration -1, each repeated phase exactly
once at iterations 0..4, or identity/counts matching the invoked literal case.
A missing phase or mislabeled backend could silently produce an apparently
complete comparison. Root accepted strict expected identity/count/phase checks.
This is a direct raw-data admission rule, not another benchmarking framework.

## Timing, lifetime and transfer disposition

The two literal recipes correctly produce a full circle and four clipped arcs.
The caller consumes/qualifies the full one-by-four broad-phase domain and groups
chart survivors to one face pair; construction then uses all wall charts.
Two-sided sampling keeps original slots/maps and exports span and occurrence IDs
plus actual native source artifacts. SVG plots actual sampled positions and the
two UV representations; it is an illustration, not the geometry oracle.

Borrowed Engine/resource scopes preserve primary failures across cleanup.
Pair indexes, sampling patches and fresh query/results close in their respective
scopes. Initial independent qualification is outside reused trials; every timed
sample still performs its actual provenance and exact model-grade checks.
The first retained sample and prepared request do not substitute for checking a
changed result in the direct test/mutation campaign.

Actual sample caller wall includes immutable batch publication and ordinary
caller assignment. Internal reused wall ends after child cleanup; native
evaluation timing is a host API span, not a kernel-only latency. Cold-to-first
batch spans body preparation through first sample, excluding later independent
qualification and final cleanup. Do not add phase medians into a total. Five
serial repeats on a shared host support only descriptive min/median/max, with
no universal GPU or statistical confidence claim.

The initially read actual_whole_caller_ms ended before writing final quality/
input/timing files. Root accepted moving quality/input/manifests inside the wall
and explicitly excluding only the final timings file, along with initial output
directory/CLI/import work. The public guide must name that scope.

Payload arithmetic matches the actual native paths: one plane plus four wall
charts supply 28 homogeneous records (896 bytes) and five compact device patch
records (80 bytes), totaling 976 geometry upload bytes. Each candidate supplies
two rows: 40 query upload bytes and 56 position/status download bytes. Full host
provenance adds 680 descriptor bytes and 40 slot/UV bytes per candidate; that is
not additional device jet traffic. The caller's one-plus-four pair indexes are
240 upload/download bytes and the complete four-pair mask is four bytes.
CPU construction, rational request preparation, world/scatter, exact grades,
source conversion and export stay explicitly host work. No native source change
or GPU intersection construction is selected.

## Corrected-source disposition

Re-read the actual controller/caller corrections. The controller records initial
source/binary hashes, checks them immediately before and after each serialized
fresh worker and after the full matrix, and includes the imported cleanup/TSV
helper and body oracle source. Its exact input dictionary checks requested
recipe/resolution/backend/device/threads, independent known component/piece and
candidate/native counts, profile/repeat count, and device/host byte fields.
Duplicate input keys are rejected. Raw admission requires the exact ten setup
phases at iteration -1 and exact ten reused phases at every iteration 0..4,
rejecting duplicates, unknown/missing rows and nonfinite/negative durations.
IR-M1 and IR-M2 are resolved; no remaining tooling blocker.

The standalone caller now exports relative source hashes and generic core/plugin
hashes without private paths. Quality/input/manifests are inside export and
actual whole-caller wall. The final timing-file write and console publication
are explicitly outside the wall, along with pre-timer output directory/CLI/import
work. All raw geometry and source-byte exports remain available.

Reviewed corrected source hashes:

- `examples/intersect_faces.py`: `c38c26d8b8567470703300536e60f5a4a5eadbb1b1d5e2475a797814b44ecd5d`.
- `benchmarks/intersections/run.py`: `93aa44edde2b787e7ff39607e4ae4244475ef6655792bbf00ded25b5d10002bf`.

No execution/performance outcome is asserted by this review. Root owns the
actual serial matrix and artifact qualification.
