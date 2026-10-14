# Matched CUDA mode comparison

28 selected reviewed conditions per card, cards 0,1, seven repetitions per variant. Reverse variant order: False. Case IDs use the full28-condition cases.tsv.

Each card runs sequentially; variant order alternates by stable case ID. No CPU job is launched. The staged core/native benchmark bytes match; plugin identities are recorded and may differ. Original per-invocation samples.tsv, summary.tsv and work files are retained. comparison.tsv retains full condition keys, both timing medians/min/max and all reported quality/work extrema. Quality columns and raw work histograms must match; no numerical certificate is inferred from timing.

Resident, stream-event bracket and cached-hierarchy host-to-host phases are distinct. Event duration may include host submission delay. Host-to-host includes the native benchmark's analytic consume checks. Setup/first-use/cleanup phases retain their existing scopes. Independent phase medians cannot be added. Positive median_improvement_percent means specialized is faster; a single run does not decide retention. Confirm selected same-case benefits/regressions in a fresh reverse-order run on both cards. No concurrent owned build/copy/qualification/performance job is permitted by the reviewed experiment.
