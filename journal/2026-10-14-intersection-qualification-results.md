# Intersection qualification and caller results

Current16-method suite passed CPU1 with existing body/pair regression checks:
38 total,1 GPU-only skip,6.641s process. CPU24 passed16,3.362s. P1000/P1001
passed16 under Compute Sanitizer,4.953/7.547s process,zero errors and leaks.
Archived exact initial source/test/controller and protected binary hashes in
results/intersections-2026-10-14 alongside scoped environment metadata.

Root found a real consumer coverage gap: constructor tests covered generators/
points, but actual two-sided sampling primarily exercised horizontal sections.
Added one direct test for irrational x/y generators, cardinal/tangent duplicated
chart occurrences, height/trim points, swapped sources and maximum scale/origin
frames. Existing16 methods stayed unchanged. Ran only the new method: CPU1/
CPU24/P1000/P1001 all passed,1.447/1.130/1.551/2.024s process. Both GPU runs
again zero Compute Sanitizer errors/leaks. Separate source hash table retains
this exact scope; no repeated unchanged native campaign.

Five isolated actual-production mutants were killed by independent assertions:
missing component, ignored hole, split seam span, wrong source UV, reversed
component traversal. Unmutated baselines and preserved library bytes stayed
healthy; original mutant trees/logs remain in ignored context.

Four fresh actual callers passed: CPU1 circle, CPU24 clipped arcs, P1000 clipped
arcs and P1001 circle. Each emits260 paired samples/520 native rows. Standalone
serialized-artifact review checked1040 samples/2080 native rows, original28-
control nets/caller, exact maps/endpoints/graph, native descriptor/slot/UV bytes,
four full mask bits, resident enclosures and48 SVG polylines. Previewed the
owned SVG via artifact-only rendering. Reviewer found an export metric-name
collision (pair count overwritten by sample count), not a geometry failure.
Accepted correction will regenerate caller artifacts after the main matrix;
initial exports will be retained privately. No hand editing of measured data.

Started the serial sixteen-case CPU1/CPU24/P1000/P1001 matrix with all owned
qualification/mutation/copy jobs finished. Target source/binary bytes stay
unchanged during trials. Initial qualification is outside five reused calls;
cold construction/request, native sampling, host grades and final cleanup stay
distinct. Do not infer a GPU intersection-construction or whole-CAD speedup.
