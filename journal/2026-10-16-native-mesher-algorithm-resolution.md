# Whole-mesher constructive algorithm resolution

Wrote `papers/2026-10-16-native-mesher-algorithm-resolution.md` for the design
owner to integrate before the implementation plan. It resolves the earlier
algorithm review's B1/B3 with one fixed exact dyadic anisotropic norm per face
generation, terminal-priority LEPP closure, a sticky one-attempt heuristic
lineage flag, explicit replacement/requeue sets, shared boundary-generation
restart, and a separate fixed-site optimizer with an explicit strictly
decreasing shape potential. No refinement and flipping are interleaved.

The transformed-norm diameter/closure argument is an owned derivation. The
current Li--Yang2026 primary paper's abstract and Sections1--3 were read for the
actual terminal-priority LEPP mechanism and single-mark termination; the
cumulative charging proof was not reviewed. Added the exact acquired version,
hash and reading scope to `papers/whole-mesher.bib`; the original and acquisition
row remain in `papers/whole-mesher/`. A separate Bedregal--Rivara institutional
download returned HTML and was not read as a paper.

The revised C0 choice is coherent for a positional contract: clip a triangle
against source spans for bounds while retaining its actual emitted affine map,
rather than inventing exact output crease/trim junctions. Existing finite charts
and constructed analytic singular quotients remain explicit support profiles;
positive rational weights alone do not imply an admitted regular chart.

Sent the complete note to the design owner. Success and complexity claims remain
conditional on stated chart/feasibility/separation assumptions; no theorem for
the pure LEPP algorithm is transferred to the whole mixed mesher. No product
code, modeling, builds, tests, dependency adoption or new benchmark campaign.

Reread the actual integrated algorithm section and its source/span/chart scope.
Returned scoped ACCEPT for B1/B2/B3 and proceeding to the implementation plan;
recorded the resolved findings and concrete plan obligations in the original
algorithm-review report. The circumcenter is now one exact 2x2 solve with
nearest-binary64 ties-to-even chosen sites; it cannot enter an unrelated or
already-spent lineage. Phi is one rounded exact-rational expression. Exact
midpoint refinement remains arbitrary-precision dyadic. These are design
choices, not claims that product implementation or watch delivery is complete.
