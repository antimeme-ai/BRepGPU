# Actual CutResult mesh independent oracle and direct tests

2026-10-15. After accepted design/plan reviews, wrote separate
python/brepgpu/cut_meshing_oracles.py and tests/test_cut_meshing.py. The qualifier
reuses only independent generic exact source/patch/chain/ring predicates and
calls the existing independent Boolean result qualifier once. It reads original
result geometry and raw cutter UVs, verifies complete exact/rounded domains,
source identity, manifold vertex links and actual rounding/error bounds.
Twelve direct tests cover the actual Boolean family, original history/cutter
frames, thresholds/invalid requests, source maps and selected altered-output
fault sensitivity. They do not claim broad mutation coverage.

Fleet red executed before cut_meshing.py existed: 12 tests ran in1.177s;
16 errors all reported ModuleNotFoundError for the real future module
brepgpu.cut_meshing (including four scale subtests). Real owned Boolean
construction completed in entry/case paths before import failed. No proxy,
fallback or skip. Private execution log retained in ignored context. No native
rebuild and no local product execution. Constructor owner may now implement.

After parent constructor implementation, reviewed actual cut_meshing.py,
CutResult adapter, independent qualifier and direct suite. Accepted without
blocking code findings; saved implementation oracle review. Parent owns the
active fleet green run; no overlapping execution or product edits by this owner.

Reviewed actual four-case and standalone saved artifacts without product/test
reruns. Both18-entry source manifests match; raw counts, original wall V/seams,
retained cutter Frames and full85/81 all-kind original history agree. Accepted
saved result review at papers/2026-10-15-cut-mesh-results-oracle-review.md. Parent
reports direct12 and compatibility46 green; no extra execution needed.
