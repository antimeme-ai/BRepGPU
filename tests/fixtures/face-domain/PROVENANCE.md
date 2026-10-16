# External sampled-domain fixtures

These are test inputs, not product implementations or a complete release corpus.
The selected cases and their limitations are documented in
`papers/2026-10-16-native-meshing-reference-test-plan.md`.

`manifold-polygon-corpus.txt` is the unmodified complete original file
`test/polygons/polygon_corpus.txt` from Manifold revision
`761661c15bacb6c8459d682c715de84591d51774`:
https://github.com/elalish/manifold/blob/761661c15bacb6c8459d682c715de84591d51774/test/polygons/polygon_corpus.txt

Original SHA-256:
`69b9709fb8608b827aedbf0c0afbe13e04364c84e1cb626385c7899a7bdac509`.
Copyright 2021–2026 The Manifold Authors, Apache-2.0; the retained repository
LICENSE is `APACHE-2.0.txt`. The adapter selects SimpleHole, Eberly, Concave2 and
AlignedHoles before observing BRepGPU outcomes. Remaining file contents are not
claimed executed, passing or excluded from alpha. Decimal input is parsed to
binary64 exactly as the original C++ corpus reader does, then those saved values
are retained as exact Fractions. Upstream counts/epsilon are preserved as metadata;
no upstream epsilon cleanup or triangulation implementation is used.

`nvidia-planar-trim.tsv` translates the complete literal geometry recipe in
`tests/SM_API_test/src/TestSmTrimmedSurfaces.cpp`, function
`TestSmTrimSurfaceWith3dCurves`, from NVIDIA USD/BRep revision
`c9c5979f04cea8c38213004d4f04df0dcfb40d05`:
https://github.com/NVIDIA-Omniverse/usd-brep/blob/c9c5979f04cea8c38213004d4f04df0dcfb40d05/tests/SM_API_test/src/TestSmTrimmedSurfaces.cpp

Original source-file SHA-256:
`33469f114eec7f2f91bfdb9fdf8cb1f81675bf1153764234d39804e861c2f33d`.
SPDX-FileCopyrightText: Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES.
All rights reserved. SPDX-License-Identifier: Apache-2.0.
The original source/archive remains intact in ignored quarantine, with restoration
metadata in `papers/reference-catalog.tsv`.

The owned test adapter samples the original circle at 32 exact rational points:
for each clockwise quadrant, `t=k/8`, `k=0..7`, and unit-circle point
`((1-t*t)/(1+t*t), 2*t/(1+t*t))`, rotated clockwise and scaled by radius 3/2.
There are no trigonometric evaluations or snapped points. The first quadrant
is reflected into clockwise traversal. The sampled polygon, not the analytic
disk interior, is the exact triangulation domain; circle chord approximation
is not presented as an upstream SDK tessellation or a qualified watch grade.
The L/triangle/circle source geometry is retained without dropping either hole.

The owned adapters and tests are repository implementation under its normal
license; retaining/reading these Apache-2.0 fixtures selects no linked library.
