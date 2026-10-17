# Captured cap facet work

2026-10-17. Finite implementation PLAN, pending peer/root review. This unit binds
the already qualified original-source cap arithmetic to produced domain/view/
construction rows. It adds no sampler, inverse, partition checker or new error
inequality. The low-level geometry unit remains independent of region/session
implementation.

## Source and numerical contract

The bound is the reviewed finite `Fsrc -> Fideal -> SAME actual L` procedure in
[the geometry plan](adaptive-seam-wedge-geometry-plan.md), grounded in
Filip–Magedson–Markot1986 §2 Theorems2/4 and the explicit source-map derivation.
Pinned USD/BRep `SmTess.cpp::sms_FillGapUVWithPolyEdge/MakePolygon` motivates
retaining distinct chart sectors at one physical vertex; our produced
`CapDomainPiece` owns the finite quad and original source roles. Existing
`adaptive_trimmed_geometry.cpp::prepare_trim_facet_work` supplies the immutable
native work/subset/packing pattern, without its ordinary triangle clipping.

Inputs are the SAME produced `FaceDomainData::caps`, captured `SurfaceBoundView`
and immutable `FaceConstructionResult`. A mapped `RegionFacet::mapped` names
cap/piece plus `source_role_to_facet`: source POLE/LEFT/RIGHT to the facet's
site positions. This is the inverse of the positive-chart role order, before
original face orientation. Original locator UV at the pole is not source
coverage and is not used as the cap reference triangle.

## Literal native API

Own new `adaptive_cap_work.hpp/.cpp`. Header includes `adaptive_cap_geometry.hpp`,
`adaptive_trimmed_geometry.hpp` for existing limits/allowances/key types, and the
lightweight produced cap/domain rows. It includes no session or CUDA API header.

```cpp
struct CapFacetTask { CapSourceDescriptor source; CapFacetInput input; };
struct CapFacetRecord {
  uint32_t construction_facet, cap, piece, prepared_source;
  std::array<ExactRef,4> cut_uv;
  std::array<ExactRef,2> finite_v;
  std::array<double,9> actual_xyz; // SOURCE roles pole,left,right
  std::array<uint32_t,3> positive_chart_roles;
  int32_t regular_sheet_sign;
  bool physical_nonzero;
};
struct CapPreparationStats {
  uint64_t facets=0, prepared_sources=0, exact_entries=0;
};
class CapFacetWork { // immutable value handle, private producer constructor
public:
  const TrimFacetWorkKey &key() const;
  const ExactTable &values() const;
  std::span<const CapFacetRecord> facets() const;
  CapPreparationStats stats() const;
};
struct CapFacetBatch {
  CapFacetWork work;
  std::pmr::vector<uint32_t> indices; // indices in work.facets()
  std::pmr::vector<CapFacetTask> inputs;
};
CapFacetWork prepare_cap_facet_work(JobContext &, SurfaceBoundView,
  FaceConstructionResult, FaceDomainLease, FacetAllowances,
  std::span<const uint32_t> selected_facets, ExactTableKey, TrimFacetLimits = {});
CapFacetBatch pack_cap_facets(JobContext &, const CapFacetWork &,
                            std::span<const uint32_t> work_facets);
PreciseCapResult compare_cap_work_facet_precise(JobContext &,
                                              const CapFacetWork &,uint32_t);
```

`FaceConstructionResult` is already a shared handle, never double-wrapped.
`PreciseCapResult` remains owner-local: its `PoleFanPlan` Rational members must
be consumed/sealed in the same JobContext. Runtime converts packed task fields
to its existing `DeviceCapFacet`; geometry does not depend on that device type.

## Preparation, admission and bounded work

1. Admit shared resource, SAME source bank, original SourceStore, quotient
   purpose, profile, selected face carrier and full source/query/definition/
   face/assignment/boundary/chart/independent face/domain generations. Require
   captured view key AND retained selection identity, exact classified-domain
   identity and construction-domain identity. Produced cap plan must retain
   the SAME boundary snapshot, source bank and full enclosing identity. No
   public boolean or equal numeric generation substitutes for these leases.
2. Admit positive exact positional allowance, distinct face-owned result table
   key, valid finite limits and copied selected subset. Validate indices before
   sorting; canonical sort by FacetKey; duplicates refuse. Only mapped-cap
   facets enter this work; ordinary work must refuse them explicitly. Preserve
   subset identity so unchanged accepted ordinary facets need no new work.
3. Validate each mapped record's cap/piece ranges, piece membership, nonrepeated
   facet site indices, original carrier/root/kind, complete role permutation,
   source-only positive-chart roles and regular-sheet sign. Normal angular
   refs must equal that piece's actual original angular endpoints; kind/sheet
   provenance must match. Do not interpret proxy/pole locator UV as map input.
4. Resolve actual XYZ through `source_role_to_facet` without source evaluation.
   Pole authority is the produced row's current `pole_boundary_node`; exact
   stored XYZ bits and construction revision match that captured forest row.
   Each cut UV equals the produced `CapPointPlan` UV. A physical endpoint also
   matches its expected physical key, source edge/sector and exact original t
   in the captured master chain; stored XYZ/revision match it. A surface cut
   retains its captured `SurfaceSiteSource` original/stage/revision and the
   producer's SAME PointPool construction contract. One constructor-owned
   `cap_point_bindings` vector, indexed by plan point, retains
   `{construction_site,sample_row}`; physical points have sample_row UINT32MAX.
   The work reads only selected endpoints. It does not invert/scan the whole
   sampler result on every sparse horizon, add a second point map, or reconstruct
   point authority independently.
5. Prepare one private `PreparedCapSource` per distinct original-root/kind in
   this work, not per facet. Import/validate map refs and retained precision,
   then append six exact parameters to the work table. The produced domain
   already performed source-only admission; this adapter does not repeat its
   proof or precise trig for each facet. It checks binding and role consistency.
   Source-specific descriptors retain the source lease. Record
   common exact/filter `actual_facet_nonzero`; false routes construction in
   runtime and is never accepted merely because its positional bound passes.
6. Apply `budget.scoped_work(limits.work)` across the whole operation. Meter
   index/range/authority visits; bound subset, prepared sources and table entries
   by existing `pieces`, `exact_entries`, `candidate_visits` and batch caps.
   Retained rows use RunResource; no JobContext Rational or borrowed input span
   survives return. Errors produce no partial work or source mutation.

## Packing and precise completion

`pack_cap_facets` copies a bounded validated work-facet subset, retaining work.
Exact cut/finite values become outward intervals; fixed actual XYZ bits are
copied verbatim. Downward exact tau may be0 and means an ambiguous fast filter,
never an invalid positive native allowance. Descriptor/input share the same
original root. CPU calls the existing common HD function; P100 dispatches the
same fields through the already qualified cap extension and owner/ticket.

Far geometric failures return to construction's angular/radial cut control.
The existing finite uncertainty/near-threshold policy calls the precise adapter
only on selected nonzero rows. It uses the same exact cut/finite refs, actual
XYZ, retained cap precision and exact allowance with
`compare_cap_facet_precise`. It neither changes L nor samples Q. Total includes
the finite source band once. Runtime may retain exact tau as an accepted upper
after a certified comparison, with diagnostic components separately labelled.
An upper pole/band term alone is not a lower floor or infeasibility witness.

## Delivery and direct checks

Root registers `brepgpu_mesher_adaptive_cap_work` linking regions and low-level
cap geometry, then SurfaceCache links this library. Regions does not depend on
SurfaceCache; no reverse dependency is introduced. Low-level geometry/CUDA
source libraries stay unchanged except a shared packed task definition if root
prefers it. No CMake edit belongs to this lane.

After plan/schema review, write a direct missing-header/operation RED using the
real source-backed construction fixture. Publish declarations early so runtime
can compile against the service, then implement. Once produced cap domain and
real view/facets are available, use that SAME fixture to exercise initial/full
and reordered-subset work, exact cap/role/actualQ packing, matching source bounds,
precise result, stale view/domain/assignment refusal, changed stored physical Q,
wrong generator and work-limit refusal. Invalid preparation must not mutate the
source/session or publish work. No fabricated view/cap handle is allowed.

Reuse the existing independent original Fsrc/SAME-L reader on packed native work
rows, plus the single reference-quad/ordinary chain mechanism for actual complete
construction. Do not repeat the closed six-source numerical kernel cohort.
Select one focused actual-work sanitizer path for new retained-table/lifetime
faults and actual-source review. Runtime owns complete fold/cancellation/device
lifetime checks. No full watch or new performance campaign is part of this unit.
