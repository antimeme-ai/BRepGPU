# Scalar discovery measurement follow-up

- Merged iteration-3 PR #10 with its substantive commits and retained branch;
  the remaining scalar measurements continue on `perf/discovery-scalar-baselines`.
- Plan: retain the scalar main field separately, combine controlled off-grid
  replacements with the existing CPU24/P100 field, and regenerate comparisons.
  Let the owned renderer accept repeated summary paths so the main figure reads
  both retained fields directly, without publishing duplicated raw samples or
  a copied combined summary. Existing condition-uniqueness checks still reject
  duplicate operation/phase records across all inputs.
- This changes artifact ingestion only. Check existing single-input renders for
  byte-identical output and exercise duplicate-input rejection locally. Native
  source, binaries and the running fleet measurements remain unchanged.
- Local artifact check: all six existing single-input files remain byte-identical;
  supplying the same field twice fails before creating output files.
- Published draft PR #11 for the scalar field and draft PR #12 for the reviewed
  specialization experiment. Merged the independent off-grid study in PR #13,
  retaining its branch and substantive commit; brought that research into both
  follow-up branches. Reconciled the literature index's stale preimplementation
  language and linked the new proposed bound without claiming measured value.
- Scalar queue exited successfully before any source upload or artifact copy.
  Collected all 84 main CPU1 cases and six controlled off-grid replacements;
  merged those six with the existing 18 cases without replacing their samples,
  summaries or work files. All fields now total 444 completed native cases.
- Regenerated figures directly from disjoint retained main summaries. Added the
  CPU1 curve, actual scalar timings and specific small-batch crossovers; retained
  the CPU24 headline and unresolved off-grid precision. No source/binary oracles
  changed. Independent artifact review recomputed summaries, work histograms and
  existing-data preservation; the original source/binary manifests are unchanged.
