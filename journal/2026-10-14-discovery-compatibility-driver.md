# Actual preserved discovery compatibility driver

- Read the prior owned projection compatibility utility and reviewed iteration3
  design/plan. Added scripts/check_discovery_compatibility.py, reusing owned
  identity/demand/legacy-module helpers and existing exact analytic fixture oracles.
- Driver runs23 fresh actual-binary combinations for CPU/bothGPUs: preserved
  v1/v2 core+plugin with current adapter, currentcore+oldplugins, oldcores+newplugin,
  actual old adapters+newlibraries, and current positive control. Every row checks
  three evaluator orders before/after, supported exact seeded projection or
  repeated nonfatal refusal, and supported discovery certificate/provenance or
  repeated nonfatal refusal. No invalid-input timing or fallback claim.
- V2 adapter is the protected actual pre-iteration file with recorded main
  revisionc3f70613526fc2d150ef992663ba373374f87759 and SHA256
  ae708f5cf7138706bff4350a4a5515bea6dc3e90e7c45f6b49fcedec366d9fd0.
  V1 prefers actual preserved bytes matching the earlier recorded revision;
  if absent, extracts that exact committed owned adapter only into ignored context.
  Baseline input hashes must remain unchanged, including on worker failure;
  output/log paths cannot write inside protected baseline directories.
- Old absent-v3 binaries cannot produce a transient factory error. Existing
  meaningful native discovery-extension-transient modeled scenario covers that
  distinct fault; this driver does not mislabel it as actual-binary evidence.
- Local Python syntax check passed. Parent executes all native combinations on
  the qualified fleet; this lane ran no native code and modified no product,
  native or benchmark source. Execution outcomes are pending.
- Disabled bytecode writes in driver and fresh workers (-B), so importing actual
  protected adapter sources cannot create __pycache__ inside baselines. Legacy
  extraction uses exclusive creation, and each compatibility worker has a60s
  timeout. No baseline is rebuilt, overwritten or used as an output directory.
