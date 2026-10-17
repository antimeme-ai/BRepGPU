# Regular analytic panels for selected trimmed faces

Implemented the accepted trimmed-runtime source-preparation subplan in
source_cache.hpp/.cpp. Grounding: pinned NVIDIA SmSurfaceCache::BuildTreeBase
and SubdivideByBlock initial panel/shared-grid construction, with the retained
original-versus-prepared source contract from ADAPTIVE_MESHER_DESIGN.md. The
pi/2-lower width is our explicit numerical preparation profile, not an error
theorem or inherited source approximation. No analytic surface becomes a new
rational approximation and no trace-purpose cache is relabeled regular.

The new factory retains one original analytic evaluator for its full finite
envelope, checks original frame/radius/sheet/pole admission, and creates exact
stored-cut regular panels. Exact midpoint locators choose nearest-even binary64
cuts, checked against strict interior/middle-half conditions. Axis/tensor counts
and work/resource limits refuse rather than publishing a partial cache. Natural
rational source spans keep their existing path. Existing patch/trace factories
retain their original angular admission policy.

Direct RED on the fleet was the absent prepare_trim_source_cache operation.
Strict Clang18/C++20 release build and actual source-backed exact-cover tests passed
for cylinder, cone, sphere and torus:8/8/8/64 prepared panels, one original
evaluator each. Same tests preserve old wrapped-patch refusal and capacity7
refusal. The final four scoped CTests also passed a two-ULP angular span with no
representable interior cut, returning unresolved as required. All four tests
completed in0.02s total; this is test execution time, not a meshing benchmark.

The numerical peer reviewed actual source and accepted exact width/cut/capacity,
source-frame admission, full evaluator preservation and internal tile-point
ownership. Actual surface sampling on those tiled faces still belongs to the
integrated face CPU/P100 operation; this source-preparation test claims no mesh
completion or speedup. Build commands and raw logs remain in ignored context.
