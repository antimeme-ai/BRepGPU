# Actual Boolean-result mesh caller implementation checkpoint

Implemented examples/mesh_cut.py, benchmarks/cut_meshing/run.py and a small
explicit common mesh-data/SVG export split in examples/mesh_body.py after all
plan reviews accepted. Canonical export defaults retain original headers,
captions, coordinate paths and recipe. Cut export names actual result face/coedge
owners, intrinsic result-edge parameters and retained carrier UV explicitly.
Original operand packed tables live under named left-box/right-cutter directories;
no packed CutSolid is fabricated. Result graph retains controls/frames as exact
binary64 hex and rational curve/maps/history fields.

Indexed/history work runs in directly enclosing useful walls. Every forward and
reverse history relation is followed; original operand records and every origin
map/range/event field are read. Four-case first/warmup/five-repeat CPU benchmark
retains first qualified outputs and compares later complete frozen values and
actual consumer buffers outside timed wall. Source and AST checks only have run
locally; product execution awaits fleet-green from the owner.

After parent reported12 direct fleet methods green, reviewed final actual
CutMesh/CutResult API and qualifier against this caller. Added explicit fixed
segment cap and exact tolerance to standalone recipe. Saved caller-owner source
review in papers/2026-10-15-cut-mesh-caller-code-review.md. Caller/helper/benchmark
sources are stable for parent-owned serial qualification and costs. No geometry
execution was performed in this lane.
