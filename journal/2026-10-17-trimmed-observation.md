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
