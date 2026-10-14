# Broad-phase literature applied to the pair beachhead

Read 2026-10-14. These references inform owned machinery; none is adopted as
a dependency or copied into product source.

Scott Le Grand's [GPU Gems 3 chapter32](https://developer.nvidia.com/gpugems/gpugems3/part-v-physics-simulation/chapter-32-broad-phase-collision-detection-cuda)
separates conservative bounding-volume culling from exact contact work. It
describes quadratic brute force, sweep sorting and spatial subdivision. Its
grid scheme needs duplicate control when objects occupy several cells, and
large size differences motivate hierarchical grids. Our first resident page
uses direct Cartesian work and one owner per packed word. This intentionally
avoids duplicate scheduling and unbounded candidate append while creating an
honest baseline. A future grid or hierarchy must outperform this complete
caller on sparse and mixed-scale scenes, retaining exact coverage semantics.

The [CGAL box-intersection manual](https://doc.cgal.org/Manual/latest/doc_html/cgal_manual/Box_intersection_d/Chapter_main.html)
distinguishes closed intervals from half-open intervals: closed boxes admit
zero-width domains and boundary touches. It also separates bipartite queries
from self intersection and uses explicit identity independent of coordinates.
Its warning about shrinking boxes during numeric conversion directly applies
to our root-to-world transformation. We use outward enclosure, closed tests,
retained typed chart identity and explicitly full Cartesian self queries. We
do not import its identical-ID suppression or reorder source slots. Its
output-sensitive box algorithm is a reference for the later sparse-scene pass.

Tero Karras's [Thinking Parallel, part1](https://developer.nvidia.com/blog/thinking-parallel-part-i-collision-detection-gpu/)
analyzes execution/data divergence in sweep and grid work. Many parallel
threads do not make a serial algorithm's work distribution efficient. This
supports measuring actual distributions and end-to-end consumers before
selecting a BVH/grid. Our bounded word tasks provide a simple work allocation;
integer division, repeated box reads and lack of tiling remain measurable
costs. No published example's speedup is transferred to our hardware or API.

Links and integration decisions supplement the three source/oracle studies.
The first page service is preparation for SSI; no intersection or Boolean
correctness follows from broad-phase culling alone.
