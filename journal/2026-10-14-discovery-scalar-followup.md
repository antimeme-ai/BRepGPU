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
