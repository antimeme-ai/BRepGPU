# Deferred scene reuse notes

2026-10-14. Reviewed the delivered pair report first; found no concrete
record mistake. Studied current body construction, CPU scene bounds, exact
world conversion and resident index ownership. Saved
[the study](../papers/2026-10-14-scene-residency-reuse-study.md).

Recorded measured setup/reused costs without attributing an aggregate phase
to an unmeasured subcomponent. Separated body-independent geometry templates
and local bounds from frame/selection/source maps and context-owned uploads.
Equal controls do not merge body IDs or trim semantics; cache keys/leases must
preserve typed revisions, exact snapshots, source order and shutdown behavior.

Latest operator steering defers all integration/residency/setup optimization
until after the breadth-first major-theme pass. These are deferred notes only,
not parallel implementation or a prerequisite for intersection work. No
product edits/imports, native execution, performance runs, new dependencies,
agents, fleet work or Git operations occurred.
