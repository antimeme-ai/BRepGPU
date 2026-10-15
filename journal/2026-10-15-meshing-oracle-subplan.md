# Actual red and whole independent mesh qualifier subplan

After all plan reviews were accepted, wrote the small entry test and ran that
single test on the owned Linux fleet. Existing native cylinder sampling and exact
plate-hole classification passed before the actual missing BodySnapshot.tessellate
raised AttributeError. One test0.054s; this is the intended missing API red.
Reported it to root, releasing the held product module/adapter implementation.

Now own meshing_oracles.py and test_meshing.py as one complete qualification unit.
Reconstruct original generators/parameters/maps and exact geometry independently;
check full directed boundary chains for every oriented planar face and actual
chord strip; enforce shared-index opposite incidence, connected shell and cyclic
vertex links. Repeat geometric chain checks for actual rounded coordinates with
linear quadrant-monotone/simple convex ring/extrusion prerequisites. Independently
bound each original circle chord, exact whole-triangle radial distance, actual
binary64 displacement and reported/requested error budget. Keep overall work
linear in returned mesh size, without pairwise triangle overlap enumeration.

Direct tests bind actual body/tolerance requests and span literal coarse graphs,
all admitted scales/translations/subnormal origins, threshold and segment budget,
source/corner/seam occurrences, immutable records and direct altered-output
geometry witnesses. Local work is static AST/whitespace only; root coordinates
subsequent fleet execution. No Git operations or dependency adoption.
