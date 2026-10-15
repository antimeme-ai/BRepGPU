# Extrusion-result mesh oracle study

Read the working doctrine, actual extrusion/mesh contracts and independent chain
helpers. Recorded a bounded direct qualification/test matrix in
`papers/2026-10-15-extrusion-mesh-oracles.md`. Highlighted the important new fault
class: rounded non-axis side planes must be checked as an actual vertical
extrusion, without inheriting axis-only assumptions from the cut-result oracle.
Await reviewed design and plan before writing qualifier or tests. No product
geometry, tests or native work executed locally; no dependency or mutation work.

Reviewed and accepted both the design and plan, then wrote the independent actual
mesh qualifier and ten direct methods (profile/frame subcases included). Fleet
red ran before any production mesher existed:10 methods in0.661s,23 errors
including subcases, all genuine `ModuleNotFoundError: brepgpu.extrusion_meshing`.
Private command output is retained in context/extrusion-mesh/red.txt. No skips,
proxy/fallback or geometry execution on the workstation. A local shell variable
capture used zsh's readonly `status` name after the completed remote run; read the
retained actual log directly, with no test rerun.
