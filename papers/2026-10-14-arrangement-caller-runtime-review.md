# Arrangement caller runtime review

2026-10-14. Static source review of `examples/arrange_face.py` against the accepted
arrangement design and plan; no product execution on the workstation.

The five recipes are the planned whole-operation inputs. The 64-circle recipes
derive exact dyadic centers inside the intended 32-by-32 cells, retain actual
prepared snapshots/sections, and construct a fresh arrangement for each call.
Preparation records body/frame and exact section construction separately. The
first call follows imports and preparation; it is not a fresh-process cold cost.

The complete timer covers construction, actual next/twin table traversal,
geometry/UV readers, and exact location calls. Each phase has disjoint wall-clock
boundaries. Independent geometry and closed-domain location qualification follow
the complete timer and have their own reported cost. Qualification explicitly
binds the retained target object, each submitted section object, and exact cuts
to the submitted inputs. Five repeated values, with separate first and warmup
rows, feed median/minimum/maximum summaries. The before/after source manifest
includes the product, imported owned geometry/oracles, tests and actual caller.

One concrete consumer finding was sent to the caller owner and root: boundary
line geometry is sampled through the arrangement readers, but directed original
curve parameters initially only contributed the length of a source tuple's
string representation. The source-parameter claim needs an actual original
coedge UV and original spatial curve evaluation at `(1-t)*t0+t*t1`, retaining
decreasing intervals without applying coedge orientation again. The caller owner
owns that integration; this reviewer made no concurrent caller edits. Follow-up
inspection accepts the integrated fix: it resolves original typed IDs, converts
record values before frame algebra, consumes both UV/world samples at the
directed interpolated parameter, and compares them with actual arrangement
readers without a second orientation reversal.

Full exact vertex/edge/use/loop/region/location tables are exported for every
recipe, including the large grid. Original frames, source sides, charts,
homogeneous controls, complete components/pieces/map coefficients/events and
original target boundary records accompany the output. The SVG reads emitted
loops, uses, curve readers and region witnesses; sampled display coordinates
do not claim a tessellation error bound. Its cell and disk fills depict the
actual complete regions rather than a selected Boolean material result.

The new oracle independently enumerates input cell and disk domains, directed
boundary words, complete source edge coverage and all incident closed regions.
It checks bijections and ownership, not just Euler counts or successful reader
execution. Oracle/test completeness receives a separate source review once
the complete test module is available. The caller is accepted for fleet
qualification. Performance and saved-artifact claims await the actual campaign.
