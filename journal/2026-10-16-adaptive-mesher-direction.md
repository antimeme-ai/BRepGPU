# Cached adaptive subdivision direction

The operator selects cached adaptive subdivision because BRepGPU is intended
as a GPU-accelerated counterpart to USD/BRep. They explicitly clarify that we
must study thoroughly for additional advantage, rather than inherit NVIDIA's
implementation tradeoffs. The previous direct-refinement-primary design and
plan are marked superseded; their source/error invariants and reusable native
components remain inspectable.

Root resumed the three existing frumentarii for whole conceptual studies of
adaptive construction/trim ownership, original geometry and numerical cache
contracts, and GPU/runtime/benchmark design. Product implementation waits for
the revised coherent design and plan reviews. No new production library is
selected. The draft is docs/ADAPTIVE_MESHER_DESIGN.md.

Important early decisions: preserve original shared edge chains rather than
invent private trim/grid crossing nodes; consider regular interior packets plus
connected boundary regions; compare packed anisotropic binary splits and side
schedules against balanced quadtrees; consider deterministic bounded source
point construction rather than assuming correctly-nearest transcendental words
are required. All output error remains against original source geometry, with
actual emitted facets and original assembly placement. CPU/P100 arithmetic and
frontier order must be specified to avoid silently different leaf selection.
