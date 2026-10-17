# Connectivity preparation after the complete OM10 delivery

Read Shewchuk, *Triangle: Engineering a 2D Quality Mesh Generator and Delaunay
Triangulator* (1996),
[section 2](https://www.cs.cmu.edu/~quake/tripaper/triangle2.html).
The unchanged acquired HTML and its hash/restoration are recorded in the
[import study](2026-10-15-om10-import-mesh-study.md).

The paper compares algorithms within one implementation rather than comparing
unrelated libraries. Its divide-and-conquer implementation wins that historical
comparison; incremental insertion spends substantial time locating points.
Alternating coordinate cuts improve the divide-and-conquer variant. Triangle
records also reduce pointer traffic compared with its earlier quad-edge layout;
constraint metadata has a distinct storage cost. These are findings from that
implementation and hardware, not projected BRepGPU speedups.

Our current fixed-vertex ear/Lawson construction completes actual ring and
spiral cases that previously exhausted refinement. Development observations
still show substantial preparation cost:2050-node ring connectivity34.6s and
4504-node spiral193.1s. Exact common-denominator integer arithmetic already
replaced repeated Fraction operations without changing predicate signs.

After full target qualification, actual images and quiet measurements, compare
owned direct constrained triangulation, better initial ear ordering, and compact
triangle/neighbor arrays against the current complete caller. Keep constrained
source segments, coedge/chart occurrences and physical keys explicit. The
existing exact UV-chain and physical/source checks must consume actual changed
output. This is return-pass research; no new dependency, architecture rewrite
or second qualification mechanism is selected.
