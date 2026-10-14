# Deliver exact classification as one breadth-first unit

The three semantics/runtime/oracle design reviews accept the bounded design.
Runtime clarification makes `same_domain` false for empty/transverse results.
This plan and its review precede product implementation. No new libraries,
native ABI changes, GPU claims or deferred Python/integration investigation.

1. Publish studies, design verdicts and this written whole-unit plan. Review
   the plan adversarially for numerical, semantic and oracle completeness.
2. Root writes a direct first test that uses the existing qualified body/native
   sampler successfully, then calls the absent `BodySnapshot.classify_point`.
   Copy to the owned fleet checkout and execute there, retaining the real
   assertion/attribute failure and healthy prerequisite. Commit the actual red.
3. One owner implements the complete exact classification operation in
   `python/brepgpu/classification.py`: strict query conversion, immutable point
   results and batch publication, feasible closed material/all original boundary
   faces, validated source rectangular geometry, complete closed face contacts,
   exact vertices/both UVs, carrier and finite-set labels. Root adds the two lazy
   BodySnapshot entry points. No file-split of one geometry invariant.
4. An independent owner supplies `classification_oracles.py` and direct tests.
   Oracle reads original source geometry, derives convex half-space constraints,
   enumerates feasible edge/plane or rank-three support intersections and checks
   exact affine rank, full vertex set, source UV mapping and normal orientation.
   It does not call product point/rectangle helpers to create expected answers.
   Literal tests anchor point labels/all face IDs and each contact dimension and
   relation, swapped input source maps and identity. Exercise every rectangular
   face orientation, exact translations/scales, rational and extreme float inputs,
   subnormal radial/frame traps, outside-but-on-carrier, hole void/rims, seams,
   malformed/unsupported cases, result ownership and atomic batch failure.
5. Root integrates and runs direct green plus existing body/pair/intersection/
   adjacency compatibility suites on the fleet against preserved native libraries.
   Review complete product code, independent oracle and caller unit adversarially;
   fix all valid findings before publishing results. Since no native code changes,
   old host/device sanitizer campaigns need no repetition solely for this unit.
6. Run five or more distinct assertion-sensitive product mutants on isolated
   fleet source copies: hole material sign, radial float rounding, frame float
   normalization, boundary provenance, finite boundary contact, complete corner
   set/both UV mapping. An actual geometry assertion earns a kill; syntax/import/
   infrastructure/crash errors do not. Keep healthy baseline/retry and preserve
   earlier native binaries. Root owns the focused execution utility.
7. Root owns the actual caller and cost unit: export exact rational point labels,
   original source identity, contact vertices and both UVs to TSV; derive an SVG
   from these actual outputs. Independently qualify every caller before reporting.
   Three point sizes (12, 1,536, 12,288) across all three primitives, plus three
   contact sizes (8, 128, 512), one cold/one excluded warmup/five repeated runs.
   Retain prepared bodies/queries across reused runs; every call returns fresh
   owned results and the caller reads every label/face/vertex/UV. Report input
   preparation, operation, consumption and complete wall spans separately;
   independent qualification runs outside those spans and its cost is explicit.
   Include ordinary/boundary/near-boundary/exponent traps in each point batch;
   one representative extreme case may be separately measured if warranted.
   CPU single-process only: no thread-count comparison or GPU speedup claim.
   Product execution and perf are serial on the fleet; no sync/tests/mutants during
   measurement. Publish source hashes and machine/runtime facts without private
   hostnames/paths. Independent offline artifact review checks exact exported
   records and source diagram content, not just process status.
8. Publish API/design/plan, research/bibliography/reviews, qualification/mutations,
   actual callers and cost report. Update README graphic/theme status, roadmap,
   active instructions, issue index, journal and return-pass notes. Mark PR26 ready,
   merge preserving meaningful commits and the branch, close issue25, and sync
   clean main to the owned fleet checkout preserving protected native binaries.

The root coordinates serial fleet execution and records each milestone. Each
delegated unit is conceptual: the complete product, independent truth, and
semantic review. All source authors wait for the direct red before implementation.
