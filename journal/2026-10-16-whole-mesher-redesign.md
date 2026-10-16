# Whole native B-rep mesher: operator-selected redesign

The operator rejects continuing the mesher as a succession of independently
chosen local changes. The requested unit is the whole mesher: study literature,
reference implementations and alternatives, form a coherent theory and design,
review its implementation plan, then deliver that design. This authorizes the
native implementation after that work; a research-only halt no longer prevents
the selected delivery. It does not authorize a library dependency or imported
reference source as owned product code.

The existing native CDT is a studied component, not the architecture around
which every other decision must be made. The existing source-to-mesh path mixes
global derivative bounds, very fine shared-edge sampling, analytic UV recovery,
native connectivity and Python refinement. The local changes have not delivered
a coherent native source-to-mesh owner. Correct individual components and a
qualified main plate do not resolve that architectural problem.

Research owners now address three whole-system concerns in parallel:

1. Complete meshing algorithms: shared edge/vertex authority, adaptive surface
   approximation, trim embedding, conformity, progress and termination; compare
   local geometry/control-net hierarchies with anisotropic surface refinement.
2. The geometric/numerical contract: original source uncertainty and maps,
   approximate pcurves, curved trims versus polygonal coverage, exact topology,
   physical embedding, bounded original-to-native approximation and independent
   actual-output oracles across general input families.
3. Native ownership and execution: immutable source and geometry caches,
   packed topology and full-fidelity mesh results, deterministic parallel
   ownership/publication, native callers and explicit CPU/P100 placement.

Root integrates these into one reviewed design before implementation begins.
Python may remain an optional boundary and independent research/oracle tool;
the selected production mesher must own source decoding, geometry processing,
shared boundaries, face construction/refinement and full output natively.
Existing exact predicates, source formats and CPU/CUDA display are reused only
where their contracts fit. Display-only BGMESH01 cannot silently replace a
complete source-linked mesh result.

Performance hypotheses concern avoided work, local approximation, cached
geometry, representations and parallelism. The30-minute shell cap was root's
experiment limit, not evidence of a kernel stall or an intended performance
target. No further unchanged whole-watch run or isolated hotspot patch is
selected. The retained watch and completed independent families are comparison
material; new admission, scheduling and tolerances must follow general geometry.

The complete107-definition watch remains the actual consumer. Its assembled/
exploded CPU/P100 graphics and actual NVIDIA reference outcomes remain required
after the native mesher's integrated output passes its direct checks. Community
alpha obligations are unchanged. Research/design/plan/review and meaningful
implementation checkpoints use coherent commits and retained feature branches.
