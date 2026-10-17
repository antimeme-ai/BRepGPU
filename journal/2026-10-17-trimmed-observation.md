# Complete selected-face observation

The reviewed external driver scripts/benchmark_native_trimmed.py runs the seven
existing authored face cases through CPU1,CPU4,P1000 and P1001 serially. Every
process requests .01 then .005mm with node cap1e-10; P100 has one host worker.
Sources, raw logs and binary outputs stay in ignored context. Public TSVs name
the source revision, executable/module/container hashes and all timing scopes.
Source metadata digest is recorded separately from actual container SHA256.

The existing boundary and whole-face readers run after the process timer.
Byte/decoded equality and UV/XYZ/connectivity comparison remain separate from
original-source geometry qualification. One observation per process supplies no
spread or general GPU-speedup claim. OS/file caches are uncontrolled, while each
process/session starts fresh and the second query retains numerical caches.

Source review removed JSON manifest/command persistence and JSON-based hashing;
the external wrapper now uses TSV/text and deterministic primitive-value repr.
Fleet syntax/help checks pass; actual observation awaits the final native build.
This is the same fixed functional cohort, not a new geometry oracle or another
certification layer. Broader caller-family ports remain in the face caller plan.

The retained run completed at source revision4b67f3d:28 serial processes,
56 original-source reader passes,42 nonbaseline complete byte/decoded/canonical
comparisons equal their14 CPU1 baselines. Public process/query/stage TSVs and
the report retain the complete costs. CPU wins these small complete processes;
the weighted process is3.331s CPU4 versus3.652s P1000. Bound-stage timing is about
95% of weighted face time and includes host preparation as well as numerical
execution; it does not isolate a GPU arithmetic bottleneck.

The first four-panel mesh view reads existing actual .005mm artifacts after the
run; it is explicitly an external view, not the GPU renderer. README, operation
guide, caller-plan status and repository instructions now distinguish this
selected-face subset from outstanding full-caller families, general curved maps,
complete imported components/assembly and OM10. NVIDIA reference timing and
external alpha qualification remain outstanding. No new geometry campaign was
added for publication. An older boundary-owner journal entry also had a private
host/path replaced with a public description of the isolated fleet checkout.

Publication review found that a fresh public checkout already contains the
retained result tables, so the driver's intentional no-overwrite guard prevents
using its default paths for reproduction. Add explicit repository-relative raw
and table output options, preserving defaults and guards, and document a new
ignored reproduction directory. This changes only output placement; it must not
change the fixed families, timing, native calls or readers. A fleet help check
is sufficient for these two argparse options; no new geometry run is warranted.
Fleet help/import check passes with both output options listed. Defaults, cohort,
no-overwrite checks, native command construction and reader calls are unchanged.
