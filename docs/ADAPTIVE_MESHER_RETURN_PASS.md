# Adaptive mesher notes after the complete construction pass

Finish shared source curves, chosen domains, connected boundary regions and
complete component/assembly publication before starting a tuning campaign.
The first untrimmed patch operation exposes these concrete opportunities:

- Separate ordinary sample misses from cached/promoted-result imports. Current
  DeviceSample carries both payloads, so ordinary uploads include unused supplied
  result fields. Preserve the source/tier promotion transaction when packing it.
- Keep immutable node geometry/bounds resident by full point-generation/profile
  key. Current node commands carry actual corners that ordinary device kernels
  replace from resident points, plus cached bounds that could be installed once.
  Measure bytes avoided before selecting a narrower command ABI.
- Revisit the child-command interface together with the native exact-key
  coordinator. Current device scan/compact determines canonical split ranks;
  native code still builds exact child identities/frontiers. Avoid extra queue
  materialization/publication work when a consumer needs only prefix ranks.
- Cache selected precise original-root compilation and demanded numerical tiers
  if actual fallback counts make them material. A conservative geometric upper
  failing tolerance is not proof that expensive exact arithmetic will help.
  Preserve numeric-floor versus geometric-refinement decisions and cold-policy
  replay across queries. Do not return to eager all-source exact compilation.
- Publish observed aggregate run reservation peaks separately from sums of
  component peaks; include module metadata, pinned staging and device growth
  overlap. CUDA context/driver memory is a different scope.
- Compare cold process, empty-adaptive-cache query, repeated actual remesh and
  quality-change workloads. GPU setup and close dominate tiny patches; retained
  cache timings cannot stand in for cold operation. Preserve failed/censored
  observations and original implementation hashes.

The initial coupled-quadratic observation exercises both parameter directions.
Its first 16,384-facet outputs match across CPU1/4 and both P100s, while the finer
old-revision operation is censored at 120s on every backend. A reviewed
quarter-twist/actual-vertex transfer strengthening addresses one conservative
facet-bound gap; its separate affected observation must not overwrite the old
cohort or imply a measured hotspot where profiling was unavailable.

The shared-curve checkpoint adds these concrete observations, deferred until
complete trimmed construction/publication:

- Tiny shared-boundary operations lose to CPU after device open and still have
  native exact identity/trace/output coordination. Batch enough useful work and
  measure complete operations before tuning launch or lifecycle details.
- Ordinary curve sample commands still carry an unused supplied-result payload;
  dynamic bound tasks stage their current flat nets. Consider retained exact-key
  bound tiers and a narrower command ABI only with measured traffic/work benefit.
- Preserve early original endpoint/alias guards, including wires. Source-node
  mismatch must never be diagnosed by exhausting geometric split depth.
- Original and prepared root identity, physical owner identity and occurrence
  identity remain separate. Multiple faces sharing a carrier compile one united
  demanded source envelope; numerical reuse does not merge physical edge owners.
- Full facet/domain error needs an actual interior map. A boundary-only domain
  upper or max edge displacement cannot become a whole-face theorem. The next
  numerical/construction studies spell the real remaining dependencies.

The first complete selected-face unit adds these concrete opportunities:

- For facets wholly inside one ORIGINAL source span, evaluate the existing
  original `N-WL`/analytic SAME-L task before materializing cache-leaf clips.
  Passing facets then avoid virtual Fraction geometry entirely. A crossing of
  original spans needs a source partition; a crossing of numerical cache leaves
  does not inherently require one for an original-source bound. Retain cached
  bilinear clipping as a tightening route for nonaccepts. Delivered in
  [the lazy-work unit](adaptive-trimmed-lazy-work-plan.md): known-leaf packets,
  original-span Band prepass and canonical fallback. The real watch now passes
  its first complete438-face component; whole-watch completion is still pending.
- Numerical refinement must not weaken actual construction progress. The first
  BAND rule placed points in the first clipped sliver and exhausted work on
  weighted/saddle inputs. Central points in the actual failing triangle repair
  those cases under unchanged caps. Preserve that independence when making
  clipped work lazy or reordering bounds. The exact central margin constrains
  direct-star areas, not global CDT diameter or eventual termination.
- The native bound stage now counts many more virtual pieces than emitted
  facets in the weighted case. Separate exact clipping/preparation from GPU
  arithmetic in the next measured scheduling study; do not blame integer
  operand size merely because a sampled stack is in GCD. Bounded diagnostics
  falsified the large-denominator hypothesis in this actual cohort.
- Shared-boundary and face sessions currently open their own numerical device
  services. Component-level orchestration must account for these banks, streams,
  source uploads and retained host/device peaks rather than treating selected
  face timings as a complete component/render result. Complete publication and
  the remaining caller-family ports still precede a tuning campaign.

The subsequent watch sample reaches repeated binary64-to-rational normalization
in candidate preparation. The [dyadic constructor unit](native-binary64-dyadic-unit.md)
removes general GCD/division from that known power-of-two conversion, without
changing geometry or generic arithmetic. This is not a general GCD diagnosis.
If preparation still dominates afterward, measure actual allocation/operand
counts before considering broader small/dyadic arithmetic or parallel input
packing. Those remain follow-up contenders; no generic arithmetic rewrite or
new mesher architecture is selected by a sampled stack.

One concrete ownership distinction to study if exact preparation remains costly:
ExactBuilder::append accepts canonical Rational values and ExactTable::seal
retains an immutable limb arena, but import_exact(ExactView) currently calls
general from_ratio on every read. ExactView is also publicly constructible from
raw input, so bypassing its validation globally would be unsound. A separately
reviewed ExactTable-owned canonical clone could avoid redundant normalization
while preserving validation for raw views. This is a source-derived avoided-work
contender, not a measured hotspot or selected change; do not replace generic
GCD or weaken raw-word admission on its account.
