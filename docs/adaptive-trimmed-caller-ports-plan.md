# Remaining complete selected-face caller ports

2026-10-17. Finite subplan for root review BEFORE test implementation. This lane
owns `tests/native_adaptive_trimmed_caller.py` plus this plan and its journal.
Production, CMake, shared geometry, runtime and build integration remain their
existing owners. All execution is fleet-only in isolated new output paths. No
new dependency, second coverage mechanism, watch case, or expanded benchmark.

## Grounding and existing gap

Read the current complete-caller plan, adaptive face/construction design and
actual reader, source fixtures, source-boundary reader and direct real-view
region fixture. The pinned sources already read in
`papers/native-adaptive-boundary-regions.md` remain the mechanism grounding:
NVIDIA c9c5979f04cea8c38213004d4f04df0dcfb40d05 SmTess AddQuadBoundaries
5334–5430/5610–5840, TraceLaminaLoop4641–4745/ProcessTree4860–4910;
OCCT b8f597c677811d1f9f4d8a97f5ae2825c0353a42 protected-wire triangulator
86–270; Ma/Hewitt2002 §§3.6,4.1–4.4 and Balazs2004 §§5.2,6,7 for hanging
interfaces/holes. These motivate the chosen fault classes; our ORIGINAL power
functions and oriented atomic chain supply the independent output oracle.

Existing full callers cover tiny/many holes, reversed loops, whole-use line
identity, bowl/weighted/saddle. Existing saved .01/.005 CPU1 bowl, weighted and
saddle query rows ALL have zero packet-target count. The direct real-view
region fixture genuinely exercises a forced target but manually requests it;
that is not an observed full caller bound-failure path. Do not rename it.

## Three preselected new original-source families

1. `c0_hanging`: two original positive-weight degree(2,2) spans joined C0 at
   u=1/2, natural domain [0,1]^2 and one tiny rectangular hole. Authored graph:

       left:  z=u/4+v^2/8
       right: z=(1-u)/4+2(u-1/2)^2+(1/8+4(u-1/2))*v^2

   Both sides agree at the interface; original normal derivatives jump. The
   stronger right-span V curvature should exercise unequal cache side schedules.
   Global U knots0,1/2,1 with multiplicities3,2,3; V knots0,1 with3,3.
   Emit exact dyadic homogeneous control rows from the authored local POWER
   polynomials; share the common U row. Bottom/top original spatial curves are
   degree2 C0 NURBS with the same original U break; side curves are quadratic
   Bezier. Guides are original affine UV edges, with source-node agreement
   checked independently by the existing curve reader. No carrier hint stands
   in for the original span partition.

2. `nested_island`: the authored bowl over [0,1]^2, with a positive full outer
   square, a negative square [3/16,13/16]^2 and a second positive material island
   [5/16,11/16]^2. Thus C=2,H=1, with known strict separation and alternating
   winding. The island should contain cached packet material at the selected
   fine quality. One original face retains every loop/use; neither a second
   face nor a separately qualified positive component is substituted. This
   exercises multiple original outers, reversed packet rims and nested bands.

3. `cylinder_seam`: port the existing literal original trace fixture as a pure
   source constructor callable here. Unit cylinder S=(cos(u),sin(u),v),
   u in [0,stored_binary64_2pi], v in [0,1]. Two circles share one physical
   vertical edge used twice, at literal u=0 and u=stored_2pi, shift0. Retain
   explicit curve radii/frames, use senses and chart occurrences. Do not infer
   mathematical equality of stored2pi and exact2pi, weld chart sites, or omit a
   periodic panel. Existing source node cap covers the tiny endpoint departure.

These are selected before running outputs. No source coefficients/tolerances
will be searched to force a desired counter. If they do not naturally produce
a full-caller packet target, report that unresolved port rather than injecting
an operation, mutating native bounds, or claiming its counter is positive.

## Extend the SAME reader

`read_face` returns its ALREADY decoded persistent packet-owner records so
feature assertions can inspect original source/leaf boxes and paths. The wire,
production artifact, and existing reader meaning do not change.

Coverage remains exactly the existing `qualify_chain`: positive actual chart
triangles + atomic reversed interior incidence + ORIGINAL selected boundary
chains, every actual site used, generalized Euler C/H. No all-pairs triangle
checker or second coverage path. Original source use traversal/senses, binding
inventory, physical keys and UV occurrences retain current checks.

* C0 whole-facet geometry: clip each actual triangle ONLY against authored
  ORIGINAL u=1/2. Intersections retain the SAME actual affine XYZ map by exact
  edge interpolation. Fan positive virtual polygons and pass each to the
  existing original POWER residual/Bernstein proof with its explicit side
  polynomial. Do not require emitted triangles to follow the crease; positional
  support permits band facets across original spans. Exact source-value callback
  chooses its original side at each UV; either side agrees at the break.
* Cylinder whole-facet geometry: for a virtual triangle, ||S-L_interpolant|| is
  at most range(u)^2/8 because ||S_uu||=1, mixed/vv derivatives0. Add actual
  endpoint departure once. Compute original sin/cos intervals from exact
  Fraction Taylor sums,32 terms, with next-term tail bound on |u|<=7; cached by
  exact u. Require max nodal squared departure <=(tau-range(u)^2/8)^2. If that
  sufficient upper is loose, the SAME existing recursive SAME-L triangle
  subdivision tightens it; no source sample replaces an actual virtual L node.
  The existing original seam boundary reader checks physical chords/source
  node guards and shared keys. Additionally require two distinct chart seam
  chains, same physical node set, opposed traversal, u=0/stored2pi, consistent
  face orientation and complete normalized loop chain.
* Nested island uses the existing exact bowl POWER proof, unchanged.

Feature observations use the admitted artifacts, not native bounds as expected
geometry: two positive source cycles/one negative; packet material inside the
known island; complete original C0 span inventory; unequal packet side
subdivisions where present. Existing atomic incidence is what checks hanging
conformity. No leaf-box feature statistic is a second mesh correctness oracle.

Long-cross-leaf: an actual C0 BAND facet whose exact UV range straddles the
ORIGINAL u=1/2 has positive area in both original spans, hence crosses at least
two numerical leaves. Inspect that geometry and retain cumulative numerical
bound-piece counts separately as a route observation. Saved packet-owner boxes
are NOT background band leaf boxes (their interiors cannot meet a correct band
triangle), so never use them to invent a stronger overlap count. No claim of a
specific large number of crossed numerical leaves follows from this artifact.

Forced packet: record cumulative counter deltas for EACH query. Assert a real
positive full-caller counter only after a preselected family demonstrates it;
otherwise report no observation and ask root to select the next bounded source
experiment. Existing direct forced-target unit remains the concrete lower-level
coverage. No caller hook or fake failed numerical result is introduced.

## History/placement procedure and bounded execution

Extend this caller with optional `--history-check` to run exactly
fine(.005),coarse(.02),fine(.005),repeat(.005) in one retained session, plus one
fresh coarse process. Every output is checked against ORIGINAL source.
Use fixed existing memory/node/depth/default node-source caps and120s per
process; censor/refusal remains failure and no partial output is accepted.

Compare BGACRV physical/source semantics and BGATRM exact UV/XYZ/oriented
connectivity/source associations. Query ID and monotonically advancing cover
revision are envelope counters, so exclude ONLY those fields from same-session
semantic equality; retain source/boundary/chart/connectivity identities and all
actual source point revisions. Fresh coarse must equal warm coarse under that
same normalization. Matched backend/query-position artifacts should remain byte
identical; preserve both comparisons distinctly. Read actual query TSV rows to
check backend/threads/source/profile/quality and nonzero device work where the
admitted family actually has it; analytic circles have no rational restriction
net, so never demand a fake restriction count there.

Order:

1. Implement original fixtures and independent reader extensions; record direct
   old caller/reader missing-family or genuine operation RED on fleet. No local
   geometry/test execution. Run ONE CPU1 full operation for each new family in
   `context/adaptive-trimmed-ports-2026-10-17/initial-<family>`; do not retry an
   exhausted/unsupported case without analysis and owner plan.
2. Close real findings with the production owner; this lane never edits product
   dispatch or loosens the unchanged source/tolerance contract. Rerun only the
   affected original-output operation. Do not broaden numerical/sanitizer suites
   for these test-only ports.
3. Report exact exercised feature counts and any missing packet/long-cross-leaf
   port. Coordinate with root BEFORE final cohort. Proposed bounded closure is
   these3 sources x CPU1/CPU4/P1000/P1001, one four-query history process and one
   fresh-coarse process each (24 processes/60 queries), no extra quality/source
   sweep. Reuse completed initial CPU1 history if scope/artifacts match.
4. Root reviews actual reader/source generation. Save scoped results and gaps;
   root owns CMake/benchmark report and commits. Stop when relevant claims are
   answered. No new speed claim, whole assembly, renderer or watch operation.

## Initial fixed-source observation

All three native CPU1 initial .005 operations and ORIGINAL geometry checks pass.
The new C0 feature assertion first wrongly assumed expanded knot ordinals0/1;
corrected to authored root boxes (actual ordinals2/4), with no product change.
The SAME saved C0 output has792facets,146 unequal packet interfaces, one band
facet crossing the original knot, and TEN real full-caller packet targets. The
source was fixed before execution; no coefficient/tolerance search was used.
Fine-query regression assertions now require these actual fault-class routes,
without requiring historical triangle counts. Nested island has16packet facets
inside its material island; cylinder has4chart seam sites sharing2physical keys.
Numerical peer source-read ACCEPT covers C0 source/virtual-SAME-L proof and
cylinder exact-tail/variance proof. Matched history/placement closure remains
pending root's cohort signal; no broader claim follows from initial CPU1.

## Completion

The bounded closure is complete:24successful native processes/60original-checked
queries across three sources and all four placements, repeated fine and fresh
coarse semantics unchanged, matched complete artifacts byte-identical across
placements. It found one real anchor ancestry history defect; root reset only
fresh-query semantic horizon and the affected history passed without weaker
normalization. Failed prefix and binary versions remain explicit in the journal.
No new source/quality search, dependency, product dispatch or broader cohort was
added. The full-caller packet target is now genuinely exercised by the ORIGINAL
preselected C0 source, not inferred from lower-level unit coverage.
