# Adjacency independent qualification and actual callers

The complete independent suite passed on the fleet: CPU1 49 combined new/body/
pair/intersection tests (9.589s process), CPU24 ten adjacency tests (1.484s),
P1000 ten (2.123s) and P1001 ten (2.493s). Both GPU campaigns reported zero
memory errors and zero leaked bytes. Qualification TSV retains actual unittest
counts/skips and scopes. Source relations, literal directed loops/UV occurrences,
all reader kinds, storage permutations, owned input/result bytes, malformed
preparation and uneven multi-level scans are directly checked.

Separate native ASan/UBSan/LSan tooling passes CPU1 and CPU24 without diagnostics.
Separate guarded CUDA tooling passes both P100s under Compute Sanitizer with
zero errors/leaks. Allocation, post-upload/scan/reader, cleanup-report and
restore-report injections retain primary failures, withhold timings, release
recoverable resources and support healthy retries. The actual device probe
selects the other card as prior device and observes restoration after calls and
faults. These are scoped injected-branch tests, not observed driver failures.

All eight direct production mutants earned real assertion kills: two CPU
builder/reader, five actual CUDA fill/recursive-prefix/reader/sign/seam changes,
and one public Python mate-identity change. Every mutant had a healthy baseline;
compilation errors/status failures/crashes are not credited. Sources/builds/logs
are isolated in ignored fleet context, with broader dependency guards corrected
following controller review. Protected old pair/discovery binary hashes are
unchanged; new libraries occupy separate trees.

Four actual callers independently qualify and export box/CPU1, globally permuted
mixed/CPU24, permuted hole plate/P1000 and permuted cylinder/P1001. Actual CSR,
mate/neighbor arrays, original directed loops/UVs, typed sources and closed-edge
records are exported. Diagram review corrected branch layout before these runs;
data and timing scopes are unchanged. Saved artifact review follows. The serial
24-case cold/repeated benchmark is next, with no other owned fleet computation
or copying overlapping measured workers.
