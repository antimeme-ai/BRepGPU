# Complete original main plate through native direct CDT

The complete438-face main plate now passes the existing independent original-
source, UV-domain and physical-shell reader. Explicit `domain_backend='direct'`
constructs95,766 physical vertices and191,688 triangles in208.258s; independent
qualification takes169.985s separately. No rendering or GPU geometry consumer
is included in either time. This is one complete part, not the complete watch.

The part retains all532 original loops,2,532 coedges,1,266 edges and844 source
vertices. Selection removes no foreign edge incidents or carrier faces and
preserves original immutable source records. Original whole-STEP geometry and
assembly outcomes are reused only with their matching source hashes; a selected
part is not relabeled as the whole9,162-face STEP inventory.

## What passed

The existing reader checks complete source/occurrence inventories, original
curve/carrier positions and recovery maps, exact simple sampled domains,
positive complete UV triangle boundary chains, shared physical keys, opposite
edge uses, closed vertex links and connected physical shells. The resulting
shell has Euler characteristic−78 and no physically collapsed UV cells. All
191,688 actual triangles receive the existing finite interpolation probes;
their largest observed error is.001874112mm. Those finite probes supplement
the mesher's separate approximation bounds; they are not a new global bound.

Quality remains5μm mesh tolerance and1μm selected source residual, with the
unchanged20-level refinement and16,384 per-edge sample limits. Maximum reported
surface-plus-canonical bound is.003749526mm. The exact acceptance criteria and
comparison grades were not loosened. See [quality](quality.tsv), full
[independent outcomes](qualification.tsv) and [source inventory](source-inventory.tsv).

## Separate construction and qualification

| Trial | Construction | Independent qualification | Outcome |
| --- | --- | --- | --- |
| Original all-pairs reader |209.608s|censored at300s|Constructed; no completed independent pass|
| Ranked-sweep reader |208.258s|169.985s|Complete part qualified|

The product sources and release DSO are unchanged between these trials. The
reader's candidate enumeration changed to the independently reviewed
[exact ranked sweep](../../docs/IMPORTED_DOMAIN_ORACLE_SWEEP_PLAN.md); original
Fraction geometry and containment checks remain. The old reader's three-second
nonblocking sample located its Fraction AABB pair loop (188 samples/108 sampling
errors). The new run's bounded sample reaches original-source interpolation
checks (148 samples/151 errors). Neither sample estimates whole-run percentages.
The old censored observation does not yield an exact qualifier speedup ratio.

Internal walls for the qualified construction are145.838s validation/sampling
and62.390s connectivity/refinement, nested inside the208.258s external product
wall. Whole-source decode7.066s and complete-part selection.244s are outside it.
Peak process RSS is514,976KiB after construction and776,424KiB after qualification;
the latter includes the Python independent reader. These are process peaks,
not the native arena's tracked allocation payload.

Both trials used one CPU on the shared Xeon E5-2697v2 fleet host, Clang18.1.3/
libstdc++13 release DSO and seed0. Other project work remained active. These are
two construction observations with different readers, not a statistical timing
study or fresh STEP-to-image benchmark. The first qualification bound was300s;
the second was explicitly600s and finished in169.985s. Machine-readable
[observations](observations.tsv) and actual [source/binary pins](pins.tsv) retain
the scope. Private STEP, source snapshots and operational paths are excluded.

The [general five-family CDT report](../direct-cdt-2026-10-16/REPORT.md) supplies
independent geometry/performance breadth. The [reader report](../domain-oracle-2026-10-16/REPORT.md)
records genuine work-count RED/GREEN,80 affected methods and two saved-domain
admissions. This part adds the actual complete consumer. It establishes no
complete-watch render, GPU CDT or alpha release qualification.
