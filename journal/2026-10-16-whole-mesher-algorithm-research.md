# Whole mesher algorithm research

The operator requested a coherent complete mesher rather than further isolated
Python/native hotfixes. Studied actual NVIDIA SMLib and OCCT edge/face tessellation
source, acquired/read anisotropic surface and restricted frontal-Delaunay papers,
and acquired/read WATER2025 with its actual public shaders. Acquired pinned
JIGSAW source for local-refinement queue/reference study. Existing kernel source
is substantial; saying only NVIDIA wrappers are available would be incorrect.

Research is in `papers/2026-10-16-whole-mesher-algorithms.md`; bibliography and
paper/source acquisition/restoration manifests accompany it. No product code,
build, installation, modeling, test or benchmark was run. Archives stay intact,
extracted references exclude nested metadata, and no dependency was adopted.

The study separates local bounds/indexing from output topology. It proposes
comparing adaptive carrier subdivision/trim embedding with geometry-aware
anisotropic placement/refinement under one shared-edge/error contract. It
records an explicit planar nonlinear-parameterization counterexample to blindly
using a same-UV residual for all geometry, and rotated-anisotropy workloads
against blindly selecting axis-aligned leaf grids.

Independent numerical discussion checked the rational Bernstein residual bound
and corrected double counting: using actual emitted XYZ corners already includes
their nodal displacement and rounding in that residual. Curved-domain coverage,
onto correspondence and physical embedding remain separate obligations. Shared
boundary/cell closure, small-feature protection and source discrepancy cannot be
deferred to a post-hoc mesh repair step.

Important applicability limits: Boissonnat2015 assumes closed smooth surfaces and
continuous SPD metrics; sharp features are future work. Engwirda2016's actual
implementation is polyhedral remeshing and its convergence section a sketch.
WATER explicitly defers trimming-induced cracks and its mesh-shader implementation
is not P100-ready. Aubry2015 and Piegl–Tiller1998 full texts remain acquisition
gaps; their previews do not count as full algorithm/proof reviews. These gaps and
unresolved boundary ownership choices feed the root design discussion, not an
authorization to implement a guessed substitute.
