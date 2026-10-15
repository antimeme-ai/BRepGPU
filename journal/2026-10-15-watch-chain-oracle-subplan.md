# Watch chain direct oracle and tests: subplan

Purpose: check the actual original profile → extrusion → specialized bore →
mesh chain and delivered parts directly, without a second certification system.
Depends on parent-reviewed design/implementation plan and agreed geometry API.

1. Add `feature_bore_oracles.py` with source-derived exact admission, material,
   retained carrier/topology and complete six-kind history expectations. Retain
   actual source-identity checks. Existing independent extrusion qualification
   checks that earlier operation once; it does not supply new bore answers.
2. Add `feature_bore_meshing_oracles.py`: reconstruct exact source generators,
   polylines/coedge occurrences, cap outer-minus-hole and side/wall strip chains;
   check exact/actual rounded geometry and full source-patch UV correspondence;
   inspect manifold incidence, links and connectivity. Reuse generic predicate/
   rational-patch utilities only, never product construction/triangulation.
3. Add bounded direct tests with missing API imports as genuine initial red on
   fleet. Geometry tests cover original profile families, strict support/cutter
   span, exact material/map interiors and full ancestry. Mesh tests cover bridge
   occurrences, non-axis boundaries, Frame/rounding/tolerance, source UV, complete
   domain and failure preservation. A few altered actual outputs expose distinct
   source/history/coverage/identity/rounding faults with intended diagnostics.
4. Sync only owned files for fleet direct execution; report results/failures to
   geometry owner, resolve valid findings, run affected compatibility once under
   parent coordination. Do not overlap fleet checkout mutations.
5. Review actual caller artifacts/qualifier results. Record outcomes and stop.

No generic property framework, mutant campaign, new dependency, external-suite
badge, sampled replacement for whole-domain checks, or broad GPU rerun. Stop
when the direct admitted obligations, affected compatibility and findings pass.
