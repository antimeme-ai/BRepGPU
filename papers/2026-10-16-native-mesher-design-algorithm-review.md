# Native whole-mesher design: algorithm construction review

2026-10-16. Read `docs/NATIVE_MESHER_DESIGN.md` in full and the shared geometry
contract study. Scope: actual whole construction, optimizer, derived-feature/
atlas ownership and progress. No product edits, tests, modeling or runs.

**CHANGES REQUESTED.** The direction matches the research: geometry-aware
proposals, hierarchy as auxiliary machinery, original feature authority and
actual-cell bounds. Three choices remain unconstructed and block an
implementation plan. Line references describe the reviewed draft, not later
revisions.

## B1. The optimizer's mathematical function is undefined

Face construction step5, lines244–249, names a metric-shape/error potential but
does not define it. Finite fixed-site configurations and exact comparison of
dyadic outputs prove no oscillation only after every triangle has one immutable
potential for those fixed sites. The draft does not choose the formula, metric
evaluation point, normalization, weighting, exceptional/nonfinite policy or the
actual bound invalidation/update stencil. A metric recomputed from an evolving
restriction cache can change a triangle's potential without any flip; that also
breaks the stated global potential argument.

Resolution: choose the actual formula and its source-defined metric sample,
freeze its fixed-site meaning independently of cache refinement, choose a stable
equal-potential tie policy and check exact positivity/convexity before flipping.
Recompute and enqueue every changed approximation cell. Alternatively omit
optional optimization from the first delivered algorithm; do not leave an
implementation owner to invent the function while claiming reviewed design.

This is not a demand for global anisotropic Delaunay or a mesh-quality theorem.
It is the missing definition of the actual local operation and its cycle argument.

## B2. C0 and atlas cuts lack a constructing owner and compatible domains

Lines126–130,198–199 and228–231 name conforming C0/atlas chains but do not say
who creates and clips them, intersects cut×trim/cut×cut, creates shared junctions,
or publishes opposite chains for adjacent chart domains. An exact simple-loop
initial CDT cannot directly consume an arbitrary internal-constraint graph.
Saying reuse applies where its contract fits leaves the principal cut case open.

Countercase: a C0 line `u=1/2` intersects a curved pcurve at an original spatial
edge parameter that is not dyadic (potentially algebraic). A private chart
intersection cannot become a canonical boundary sample without the original
edge owner and a defined representation of the parameter/root bracket. A chosen
approximate sample need not land exactly on both the true cut and true trim.
Source tolerances make the simultaneous geometric constraints harder, not easier.

Resolution: define a FaceAtlas/DerivedFeature owner and its finite embedded
feature graph, including root/junction representation, shared physical node
generators, chart transition maps, C0 side ownership and original-edge split
requests. Specify whether it partitions a face into compatible loop domains
accepted by the existing constructor, or whether the native constructor's
internal constraint contract is deliberately extended. A source knot is not
automatically an output feature record just because a bounds query found it.

Artificial atlas cuts and actual C0 creases need separate semantics: an atlas
cut may carry two chart occurrences of one smooth geometric chain; a crease
must retain side-specific geometry/normal meaning. Both must preserve original
source ancestry and cannot independently duplicate a physical junction.

## B3. The stated fallback is not yet a progress algorithm

Step6, lines250–260, does not choose whether longest means UV, XYZ or the current
metric, nor specify labels, tie policy and the conforming propagation algorithm.
The diameter-shrink argument for a bisection process does not automatically
survive arbitrary optimization flips that reconnect refined patches. Monotone
vertex count and fixed-site flip termination do not prove refinement convergence.
Repeated invalid anisotropic proposals can also prevent entry into the fallback
unless proposal attempts are explicitly bounded per cell generation.

Resolution: choose a persistent per-chart conforming refinement scheme with a
fixed norm/labels and a shrinking-cell invariant, including original/derived
feature boundary propagation. Prevent later optimizer operations from undoing
that invariant. One possible restriction is to permit a reconnection only when
all its replacement actual cells already satisfy the bound; another is to retain
protected refinement lineage/edges for unresolved patches. Define generation
invalidation and a finite proposal attempt rule before fallback. Do not claim
the guarantee of a pure bisection algorithm for an unspecified mixed algorithm.

## Other choices to close with the three blockers

The metric expression also needs an actual positive `h`, a selected normal/
curvature sample and a stable evaluation rule. These are proposal policies, so
they need not be universal correctness oracles, but they determine the shipped
algorithm and its behavior on the rotated-ridge counterfamily. The declared
metric cannot remain a list of alternatives inside the implementation plan.

The source realization and curved/polygonal-domain distinction are substantially
better specified than the legacy mesher. Keep their constructor data attached
to actual cells, particularly physical-plane inverse branches, quotient fan
coverage and source traces. Neither exact polygon coverage nor positional
closeness proves physical embedding; the design correctly gives actual-facet
contact checking its own operation. Do not replace these substantive contracts
with old mesher fallthrough when constructing the atlas is difficult.

The study's nonlinear planar-square and rotated ruled-parabola counterfamilies
remain mandatory general workloads. No watch-specific dispatch, default capacity
or tolerance share is proposed. The review does not require another review of
the same geometric oracle or an additional certification framework.

## Revised design review: scoped ACCEPT

Reread the actual revised `docs/NATIVE_MESHER_DESIGN.md` after integration of
`2026-10-16-native-mesher-algorithm-resolution.md`. **ACCEPT for the algorithm
construction scope and for proceeding to the implementation plan.** The earlier
B1/B2/B3 findings are resolved by the selected construction, not deferred to
implementation inventing a fallback:

- B1 now defines a pure source/site metric sample, explicit shape potential,
  exact rational evaluation with one specified binary64 rounding, strict exact
  dyadic sum decrease and a fixed-site optimizer after all bounds/contacts pass.
- B2 no longer promises invented C0 output chains/general atlas cuts. Exact
  source-span clipping bounds the same actual emitted affine facet. Existing
  finite charts and implemented singular quotients have explicit scope;
  unresolved general charts cannot silently use legacy mesher fallthrough.
- B3 now has one fixed nonsingular dyadic norm, initial transformed CDT,
  terminal-priority LEPP, sticky bounded heuristic ancestry, explicit target/
  unspent-neighbor admission, exact midpoint splits and a separate original-edge
  generation restart. No optimizer can reconnect the refinement forest.

The linked resolution supplies the factor construction, replacement stencil,
closure/shrink argument and qualifications. I accept the conditional progress
argument; this is not an unconditional success or output-optimality theorem for
arbitrary B-reps. The admission section's broad surface-family sentence is read
together with its later explicit regular-chart/singularity restrictions.

Implementation-plan obligations are concrete ownership and execution details:
one native exact/dyadic site/factor representation; the terminal incoming-edge
tie and spent-neighbor rules; transactional split/requeue/BVH updates; finite
work and arithmetic accounting; original-edge owner restart without stale
publication; and optimizer contact/bound acceptance before strict potential
decrease. Direct development cases must attack those actual mechanisms, with
the already selected nonlinear planar and rotated-ridge kappa/epsilon workloads
measuring output density/work before watch tuning. This adds no independent
certification layer and requests no further literature or benchmark campaign
before the plan.
