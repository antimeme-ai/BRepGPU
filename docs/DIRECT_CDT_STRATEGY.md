# Connectivity design reopened around stronger algorithms

The operator clarified that aggressive performance means reaching for strong
algorithms before metrics exist. This supersedes choosing a native translation
of the current ear-plus-Lawson pass as the next constructor by default. The
existing engine remains useful as a baseline and delivered capability; the next
choice must attack the work being generated as well as its execution overhead.

## Selected direction and serious contenders

Select **direct constrained Delaunay construction** for the next architecture
study and implementation design. Start from the exact sites and source boundary
segments, build connectivity deliberately, and avoid manufacturing a large poor
ear fan whose repair then dominates. The current API contract stays one outer
loop plus disjoint holes with protected collinear boundary samples, no Steiner
insertion and a constant positive diagonal metric. Stronger algorithms do not
require widening that public contract to arbitrary intersecting PSLGs.

| Contender | Structural reason to study it | Conditions and unresolved costs |
| --- | --- | --- |
| Full constrained divide-and-conquer, Chew1989 | Complete CDT has deterministic O(n log n) construction in its model | Constrained strip regions/merge and degeneracies are substantive machinery; ordinary initial DT followed by arbitrary recovery does not inherit this bound. |
| Alternating-cut initial DT plus Shewchuk–Brown cavity recovery | Static point set avoids incremental location; segment recovery is expected linear in crossed edges; randomized complete construction has an analyzed bound | Implement the actual special-cavity algorithm, including its secondary convex-fan case, and the required random segment order. Source exactness and degeneracies remain obligations. |
| BRIO incremental point construction plus strong recovery | Random rounds permit spatial locality while retaining the applicable incremental analysis | A genuine history/hierarchy location scheme and careful round randomization are required. Spatial order plus previous-triangle walking alone is not equivalent. |
| Parallel GPU CDT pipeline, including2026 gCDT | Bulk insertion/history, load-balanced constraint traversal and region-local flips attack serial work and repeated global traffic | Need exact-source predicate treatment, P100 compatibility, safe region ownership and batching of small faces. Quantized4090 results do not establish this contract. |
| Fixed-connectivity native Lawson | Useful control; packed topology and exact lift caching can reduce interpreter/container/arithmetic cost | Preserves potentially large flip work caused by the initial triangulation. It earns use by comparison or as a bounded component, not familiarity. |

The concrete CPU lead for a bounded owned prototype is alternating-cut initial
DT plus the actual randomized cavity algorithm, with full constrained
divide-and-conquer retained as the stronger global contender. The GPU contender
is part of architecture selection now. No native direct constructor or GPU CDT
is implemented by this document, and no timing winner is declared.

The literature's complexity bounds count geometric operations under its
arithmetic assumptions. Our exact rational bit complexity, normalization,
allocation and publication remain measured costs. The purpose of the comparison
is to select algorithms that avoid bad work on general inputs, rather than
attach an O(n log n) label to a different implementation.

## Numerical and topology direction

Select a filtered-exact predicate architecture from the outset: certified scalar
or interval decisions followed by owned exact arithmetic for uncertain signs
and zeros. Lossless integer views retain original rational inputs; anisotropic
metric coefficients are part of the predicate. Cached anchored lifts reduce
repeated arithmetic for fixed points. A checked fixed-width path is a serious
contender when actual intermediate bounds permit it. An expansion routine on
rounded rational coordinates answers a different geometric question.

Use compact typed triangle/neighbor/source-slot records, explicit constraint
ownership and bounded storage for cavity occurrences/work. Preserve original
uint64 identities through sorting and compaction. Hull/ghost entities remain
internal; avoid an arbitrary huge floating-point supertriangle. Declared exact
zero and cocircular policy replace epsilon topology. A direct constructor can
choose different legal diagonals from the historical improver; an actual input's
repeatability is separate from source-renaming invariance.

## Actual oracles and the missing property

The existing exact boundary-chain reader checks positive complete polygonal
coverage. Source-ID incidence/embedding checks preserve all protected sites and
segments. Physical-shell/source-distance readers check the eventual3D consumer.
The old Python Lawson function provides a differential baseline, with several
hand-derived diagonal/metric tests. None of those alone is a general CDT oracle.

The new [semantic oracle design](DIRECT_CDT_ORACLE_DESIGN.md) adds the missing
property: independently rebuild actual output adjacency and evaluate a full4x4
Fraction determinant in original metric coordinates on every unconstrained
interior edge. Require the local constrained-Delaunay condition after checking
domain validity, constraints and embedding. Accept either legal cocircular
diagonal; do not require matching a reference triangle list. The oracle has its
own [focused implementation plan](DIRECT_CDT_ORACLE_PLAN.md).

Reference inputs are useful difficult geometries, not answer keys. The selected
NVIDIA/Manifold cases already check our actual domain output. Isolated Triangle
or CGAL comparisons can supplement the semantic reader only when their input
conversion preserves the exact sites, metric, constraints and no-Steiner scope.
No new production library is selected.

## Experiments chosen before tuning

Use independent convex/cocircular rings, collinear strips, concave combs and
corridors, one/many holes, irregular isotropic domains, long recovery segments,
near-degenerate rational inputs and anisotropic metrics. Vary sizes, source-ID
orders and independent random seeds. The watch contributes captured regressions
after these families have been selected. Record unfavorable cases and resource
failures as outcomes. Do not replace constraints, remove protected nodes or
relax tolerance to obtain a result.

Compare complete source-input-to-triangles calls, plus ordering/construction/
recovery/predicate/output costs. Track crossed edges, location/merge work,
cavity sizes, flip counts, filter failures, operand widths, bytes and process
RSS. GPU calls additionally include packing, transfers, synchronization and
exact fallback. Independent output qualification is timed separately and must
complete before a construction is presented as qualified.

The current300s cutoff bounds an experiment; it is not the aspiration. The
aspiration is structurally efficient construction with source-correct numerics,
local memory access and useful parallelism. Controlled work counts and actual
end-to-end results test whether the owned implementation realizes that design.

## Reading behind the choices

- [Algorithm selection and primary bibliography](../papers/2026-10-16-direct-cdt-algorithm-selection.md).
- [Filtered exact predicates and derivations](../papers/2026-10-16-direct-cdt-predicate-design.md).
- [Current GPU CDT paper and actual source](../papers/2026-10-16-gpu-cdt-frontier.md).
- [Existing native-domain results, including the complete-part timeout](../results/native-face-domain-2026-10-16/REPORT.md).

Seidel1988 and Chin–Wang1998 full texts remain acquisition gaps. Their reported
results are leads, not claimed implemented/read algorithms. Neither a theorem
from a different algorithm nor a published hardware throughput becomes our
performance result.
