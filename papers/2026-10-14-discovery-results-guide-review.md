# Discovery renderer and operation guide review

Source review, 2026-10-14, of
[render_discovery_results.py](../scripts/render_discovery_results.py) and
[ITERATION_3.md](../docs/ITERATION_3.md), against the actual benchmark summary
producer, native phases, public ABI and query counter implementation. No native
execution or implementation edits. Retained final performance summaries were
not present in this checkout at review time, so this review does not attest to
any measured value or plotted outcome.

## R-V1 — medium, fix before publication: summary conditions are dropped

The producer groups by backend/device/threads/count/corpus/mode/depth plus
`max_visits`, `max_refinements`, `tolerance_squared` and phase. Renderer
`precision()`/`comparisons()` dictionary keys omit the three work/accuracy
options and silently overwrite duplicate shortened keys. Consequently valid
summary rows from different budgets/tolerances can be paired as though they
were one admitted operation. The renderer is also usable on a native run with
`--max-refinements 0`, but its matrix caption states eight refinements. Timing
always states seven samples and tolerance2^-24 without checking either from
selected rows; precision checks samples7 but not its fixed budget/tolerance
labels. Series length alone does not prove every count occurs once.

Correction: key rows by the full recorded condition. Reject duplicate full keys
and ambiguous/mixed-condition plot populations; verify every backend has exactly
one row for each selected count. Derive caption budgets/tolerance/sample count
from actual rows or validate the specific published constants before labeling.
Check capped's max_visits=1 explicitly rather than infer it only from corpus
name. Export requested visit/refinement/tolerance fields in comparisons.tsv.
The current default matrix may satisfy these assumptions, but the renderer must
check them rather than silently change an interpretation when input differs.

## R-V2 — low, publication clarity: ratio and amortization need visible conditions

The exported latency ratio includes GAP and coverage populations, which is useful,
but omits achieved width ranges and a statement that equal requested options do
not imply equal achieved precision. A GPU/CPU ratio is not automatically
throughput at equivalent attained accuracy when some rows remain unresolved.
Report minimum GAP/coverage populations alongside maximum/mean attained widths
and work/reason populations; label the value as a recorded phase-latency ratio
rather than an unqualified accuracy-equivalent speedup.

`estimated_hierarchy_reuse_calls` computes ceil(max(0,Gsetup−Csetup)/(Cphase−Gphase))
from independent medians. Its source comment correctly limits it to hierarchy
setup and says it is not a measured first-use host operation. Make that
conditional scope visible in the export/caption/report too. It excludes other
first-use/context/patch costs; resident phase additionally assumes reused resident
queries and excludes download/consumption. Host-to-host phase has a different
per-call scope. No savings, zero extra hierarchy cost, precision-population
mismatch and nonpositive phase savings need explicit interpretation. Do not
promote this estimate to a measured end-to-end break-even.

## R-V3 — low, guide wording and reproduction

The guide's flags, independent precision/coverage meaning, original-input upper
certificate, no nearest-UV guarantee, eight fields, node/cache bytes and ordinary
versus interval evaluation counts agree with the code.32 updates and24
backtracks means an initial trial plus up to24 halvings, not24 total trials.
No correction is required for the current backtrack wording.

Replace “Hierarchy creation snapshots patch data” with “retains the patch set's
existing immutable snapshot”; the runtime does not copy original descriptors/
controls again when making a hierarchy. Its CPU builder produces new enclosure
arrays from the already-owned carrier. Query slots/targets do receive a private
copy, and downloaded result sources remain owned after public parent close.

The guide currently says “after building” without a build link or environment
setup. Add a link to the existing Linux-x64/CMake/CPython/CUDA toolchain guide
and provide a matching BREPGPU_LIBRARY setting for the actual build folder used
by its qualification commands. Default adapter lookup searches build/release
and build/libbrepgpu.so, whereas delivery builds may be build/discovery-release.
This avoids accidentally loading an old ABI1/2 core and getting an expected
capability refusal when a reader is trying to reproduce discovery. Final
qualification/measured-phase wording should be updated only after root results.

## Other reviewed behavior

The figures use medians and observed min/max ranges rather than confidence
intervals. The resident scope includes new output allocation, traversal/refinement
and synchronization; download/consumption are separate, matching native timers.
The precision matrix deliberately shows CPU24/P1000 and different modes with
GAP populations; it does not silently call all methods equally accurate. Setup
cache sizes are native payloads and exclude original controls, allocations and
simultaneous host/device ownership, as the guide says. Node80 plus mode stride
0/20/490 yields the documented approximately40/120KiB/2MiB depth8 tree payload.

The guide correctly limits bounds to one named untrimmed patch/local metric,
explains exact global squared-distance bracket versus descriptive XYZ, and
separates arithmetic/invalid rows from valid unresolved results. Visit/prune/
refinement/evaluation prose matches the accepted implementation. Figures cannot
claim observed traversal paths without a trace, and this performance renderer
contains no such claim. No product/native math correction arises from this review.

## Disposition

Findings sent to root before publication. Append source correction re-review once
the renderer/guide owner integrates them. No implementation files edited here.

## Integrated correction re-review

Re-read owner corrections: full operation keys now include visit/refinement
limits and bit-exact parsed tolerance; duplicate keys/mixed chart conditions
reject; plotted sample count is validated and budget/tolerance labels derive
from recorded data. Comparisons pair full conditions and export actual gap/
coverage populations and achieved mean/max widths. R-V1 is resolved apart from
the small requested seven-sample check on both setup rows used by the estimate.

R-V2 is resolved: exported estimate-scope text and generated FIGURES.md explain
independent-median, hierarchy-only conditional savings, exclusions and absence
of break-even when per-phase savings are nonpositive. R-V3 retained-snapshot
wording is fixed; explicit build/library-path reproduction link remains a low
completion note. Added28-byte input/88-byte output explanation correctly refers
to same-host RAM/VRAM movement rather than network traffic. No new API or counter
finding. Root notified of the two small remaining edits.

## Final substantive disposition

Re-read setup-row validation and generated figure scope: both CPU/GPU hierarchy
setup medians now require seven actual samples. Resident estimates explicitly
assume resident query reuse and exclude download/consumption; host-to-host
estimates include fresh query snapshot/download/native consumption. R-V1 and
R-V2 are fully resolved. Guide retained-snapshot language and explicit
BREPGPU_LIBRARY/build links resolve R-V3's substantive reproduction ambiguity.
The source/phase/quality review is accepted with no remaining blocker. Suggested
copy/paste conveniences are explicit PYTHONPATH and CUDA=OFF in the CPU-only
block; the linked existing build guide already supplies them. No performance
value or native qualification is asserted by this review.
