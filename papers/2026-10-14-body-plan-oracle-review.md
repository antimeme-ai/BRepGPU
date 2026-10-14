# Body sampling plan: oracle/test review

Reviewed [BODY_SAMPLING_PLAN.md](../docs/BODY_SAMPLING_PLAN.md) against the
accepted design, exact analytic model and current adapter. The corrected plan
is accepted; no remaining blocking finding. Review only, with no implementation
or native execution.

**R-BP1 — canonical plane parameter maps, resolved.** The initial plan named
faces but omitted their precise tangential UV maps. An independent position
oracle would otherwise guess or reverse-engineer production controls. Root added
the explicit position/orientation table, cylinder cap scale and chart-slot/native
slot relationship. The oracle can now derive each ideal position and outward
normal from the declared body role, independently of production evaluators.

**R-BP2 — mutable Patch controls, invalid finding retracted.** I incorrectly
inferred that retaining Patch values retained mutable control arrays. The actual
`_flat` returns tuples, and `Patch.__post_init__` freezes those values. Existing
Patch records meet the immutable snapshot requirement. No new patch-spec type
or product change is needed; earlier study wording has been corrected.

The prescribed Candidate/Request/Sampler/Batch interface supports exact submitted
UV labels, foreign-snapshot refusal, canonical order and compact-native/scattered
rows. Named Vertex.position, immutable timing pairs and bounded iterable copying
remove practical test ambiguity. `qualify_batch` should check every row using
the canonical maps, integer-ratio trim truth, fixed analytic outward normals and
source records; it must not call product classification/curve evaluation as truth.
Typed graph references, loop/seam incidence and complete symbolic correspondence
belong to the same narrow body oracle, separate from native numerical probes.

Three small grids/adversarial sets, explicit frame and ownership cases, CPU/both
P100 calls, and a few fleet-only host mutants attack the stated fault classes.
The first missing body operation supplies a genuine initial failure. One shared
qualifier serves tests/caller/initial measured outputs; repeated cache/degree,
native compatibility or mesh campaigns are unnecessary. The plan retains full
pipeline cost and all rejected/invalid records, with no imported-model, mesh,
Boolean or general B-rep claim. Ready for direct oracle/tests before production.
