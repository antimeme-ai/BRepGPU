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
