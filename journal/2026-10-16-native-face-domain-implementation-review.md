# Native face-domain implementation review

Independent read-only numerical/algorithm/resource review of the first native
implementation, binding, direct fixture/test sources and root's target tooling.
No tests, CAD or benchmarks were run by this reviewer. Actual fleet results are
owned by the implementation/tooling lanes. Performance acceptance remains
separate from this code reading.

Findings and observed dispositions:

- **Fixed:** binding normalized all coordinates before checking cumulative
  encoded limb bytes. One large common denominator and many compact integer
  inputs could allocate many expanded integers before rejecting the payload.
  It now reserves distance-weight storage and checks cumulative projected limbs
  before each multiplication. Owner reports a genuine stale-wrapper fleet RED;
  that runtime record belongs to the owner, not this review.
- **Fixed:** snapshot encoding called `to_bytes`/extended buffers before checking
  its limit. It now checks before those allocations; reads are bounded as well.
- **Fixed:** uint32 predecessor initialization `(i+n-1)%n` could wrap at the
  permitted large counts. Conditional predecessor/successor construction avoids
  the addition rather than relying on the default small workload.
- **Resolved design/implementation mismatch:** the initial code had only raw
  integers despite claiming typed slot separation. Geometry point operations now
  accept an explicit `NodeSlot`; raw occurrence/segment indices cannot implicitly
  become point arguments. The design explicitly leaves other ordinals checked
  raw rather than claiming comprehensive strong typing.
- **Fixed in tooling:** checking default compiler macros did not enforce
  libstdc++13 in actual compilation units with overridden toolchain flags.
  The target now force-includes the small `<version>` profile guard. Root reports
  that an actual wrong-release fault is refused; no extra probe was run here.
- **Fixed after initial review:** resolve the selected shared-library path before
  `CDLL`, and expose it for exact binary pins. A bare filename can pass the
  current-directory file check while the dynamic loader searches another file.
  Current fleet selection is an absolute path; no wrong-library run is claimed.
- **Performance measurement finding:** sparse `segment_candidates` counts only
  box-overlap survivors after filtering. A quadratic enumeration with a later
  AABB filter could also have a small count. Requested crossing-only BVH node
  visits and leaf-object box tests so the actual broadphase work, separate from
  ear/bridge work, supports the scaling claim. This refines the direct mechanism;
  it is not another qualification layer.

Final reread resolves the measurement finding: tree queries now count leaf-box
tests before overlap filtering, crossing captures its node/test deltas, and the
sparse gate uses those actual boundary-search counts. The binding resolves an
absolute path and `require_available` loads the DSO and checks ABI record sizes
and required symbols before expensive production setup.

**ACCEPT the current implementation on this read-only review.** No unresolved
concrete geometry/numerical/resource finding remains from this reviewer. This
does not substitute for the owning lane's actual release, sanitizer, static
analysis, difficult-face performance or complete-part outcomes.

Inspected exact carry/borrow/product bounds, allocator propagation and Engine
member destruction order, signed canonical limbs, closed rank overlap,
lexicographic crossing selection, doubled midpoint winding, bridge endpoint
exemptions, duplicate occurrence handling, convex fast path and final ear order.
No additional concrete geometry defect found in those mechanisms on reading.
The generic ear tree can still visit dead or broad-box candidates; actual work
and complete wall, not correctness green, determine practical acceptance.

Named timers omit some solve preparation: index rebuilding, area/containment
validation and the convex test are outside crossing/bridge/ear clocks. Full
native wall must include them; report the scopes honestly rather than treating
their sum as the complete native operation.

The proposed consumer integration is sound with deterministic loaded-library
identity: select native once, route circle checks and final face domains,
retain explicit Python reference/no fallback, pin actual code/library and run a
complete independently qualified part before a fresh whole-watch attempt.
No integration product edit was reviewed or authorized by this journal itself.

Subsequently reviewed the root's dispatch/provenance diff in imported_meshing,
render_om10, qualify_om10 and benchmark_om10. Native default, explicit legacy
backend, every circle check/recheck, final domain calls, metadata and actual
library/source pin additions route as designed. Suggested actual DSO load/symbol
preflight before expensive edge setup: a file-presence check alone cannot detect
missing runtime dependencies. That does not weaken the current missing-file
rejection or imply a wrong library was loaded. Acceptance of the conceptual
connection still depends on standalone performance and complete-part evidence.

Deferred, without additional implementation scope: root reports approximately
25.8 million dead ear occurrences among34.4 million ear candidates on the actual
8300-node trial. That supports studying live-subtree counts after this delivery,
not an unmeasured claim that exact arithmetic dominates. Small-inline integer
storage/reusable predicate scratch is another later candidate if allocation
profiling warrants it. Keep both behind the current unit's practical target and
whole-application measurements; neither is needed to accept a successful bounded
first transition unit.
