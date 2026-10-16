#include "geometry_candidate.hpp"
#include "geometry_domain.hpp"
#include "geometry_inverse.hpp"
namespace bgmesh {
FaceCandidateResult evaluate_geometry_candidate(GeometryContext &context,
                                                const FaceCandidateWork &work) {
  if (!work || !work->boundary)
    throw Error(ErrorCode::invalid_source, "geometry candidate lacks source generation");
  const auto &seed = *work->boundary;
  const auto &source = seed.geometry.view().source.view();
  if (seed.face.value >= source.faces.size() ||
      seed.definition.value >= source.definitions.size() ||
      seed.source_generation != seed.geometry.view().table.key().generation)
    throw Error(ErrorCode::invalid_source, "candidate source face/definition invalid");
  const OwnerKey owner{OwnerKind::face, seed.definition.value,
                       source.faces[seed.face.value].id.value};
  if (work->target.face.owner != owner ||
      work->target.face.face_generation != seed.face_generation ||
      work->target.face.boundary_generation != seed.boundary_generation)
    throw Error(ErrorCode::invalid_source,
                "candidate belongs to foreign source/face/boundary generation");
  if (!seed.protection)
    throw Error(ErrorCode::unsupported, "candidate source protection is not prepared");
  const auto &protection = *seed.protection;
  if (&protection.geometry.view() != &seed.geometry.view() ||
      protection.definition != seed.definition || protection.face != seed.face ||
      protection.source_generation != seed.source_generation ||
      protection.face_generation != seed.face_generation ||
      protection.boundary_generation != seed.boundary_generation)
    throw Error(ErrorCode::invalid_source,
                "candidate feature protection belongs to foreign geometry generation");
  auto &budget = context.budget();
  const std::array<Rational, 2> chart{import_exact(budget, work->values.view(work->chart[0])),
                                      import_exact(budget, work->values.view(work->chart[1]))};
  Vec3 xyz{};
  std::optional<CandidateLocator> locator;
  if (seed.chart_kind == WorkingChartKind::physical_plane_convex_q) {
    if (!seed.plane)
      throw Error(ErrorCode::invalid_source, "candidate plane chart lacks source map");
    const auto reserved = work->result_table_key;
    if (!reserved)
      throw Error(ErrorCode::invalid_source, "plane candidate lacks reserved result table key");
    const auto &result_key = *reserved;
    if (result_key.owner != owner || result_key.generation != seed.face_generation ||
        result_key.ordinal <= work->values.key().ordinal)
      throw Error(ErrorCode::invalid_source, "plane candidate result table identity mismatch");
    const auto inverse = recover_plane_chart_locator(context, seed, chart);
    ExactBuilder result_values(seed.resource, result_key);
    const std::array<ExactRef, 2> uv{result_values.append(inverse.chosen[0]),
                                     result_values.append(inverse.chosen[1])};
    const std::array<ExactRef, 4> enclosure{
        result_values.append(inverse.enclosure[0]), result_values.append(inverse.enclosure[1]),
        result_values.append(inverse.enclosure[2]), result_values.append(inverse.enclosure[3])};
    locator.emplace(CandidateLocator{result_values.seal(), uv, enclosure});
    const auto &p = *seed.plane;
    std::array<Rational, 8> q{Rational(budget), Rational(budget), Rational(budget),
                              Rational(budget), Rational(budget), Rational(budget),
                              Rational(budget), Rational(budget)};
    for (std::size_t i = 0; i < q.size(); ++i)
      q[i] = import_exact(budget, seed.values.view(p.quadrilateral[i]));
    for (std::size_t i = 0; i < 4; ++i) {
      const auto j = (i + 1) % 4;
      if (q[2 * j]
              .subtract(q[2 * i])
              .multiply(chart[1].subtract(q[2 * i + 1]))
              .subtract(q[2 * j + 1].subtract(q[2 * i + 1]).multiply(chart[0].subtract(q[2 * i])))
              .sign() < 0)
        throw Error(ErrorCode::unsupported, "candidate outside admitted plane inverse image");
    }
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto o = import_exact(budget, seed.values.view(p.origin[axis]));
      const auto x = import_exact(budget, seed.values.view(p.basis[2 * axis])),
                 y = import_exact(budget, seed.values.view(p.basis[2 * axis + 1]));
      xyz[axis] = o.add(x.multiply(chart[0])).add(y.multiply(chart[1])).to_double();
    }
  } else {
    if (seed.chart_kind != WorkingChartKind::original_uv &&
        seed.chart_kind != WorkingChartKind::analytic_unwrapped_uv)
      throw Error(ErrorCode::unsupported, "candidate requires an admitted working chart");
    const std::array<Rational, 2> uv{import_exact(budget, work->values.view(work->original_uv[0])),
                                     import_exact(budget, work->values.view(work->original_uv[1]))};
    for (std::size_t axis = 0; axis < 2; ++axis) {
      if (chart[axis].compare(uv[axis]) != 0)
        throw Error(ErrorCode::invalid_source,
                    "ordinary candidate chart/source coordinates differ");
      const auto lo =
          import_exact(budget, seed.values.view(seed.prepared_original_rectangle[2 * axis]));
      const auto hi =
          import_exact(budget, seed.values.view(seed.prepared_original_rectangle[2 * axis + 1]));
      if (uv[axis].compare(lo) < 0 || uv[axis].compare(hi) > 0)
        throw Error(ErrorCode::unsupported, "candidate leaves prepared source rectangle");
    }
    const auto carrier = source.carrier_index.find(source.faces[seed.face.value].carrier_id);
    xyz = canonical_original_surface(context, seed.geometry, carrier, uv[0], uv[1]).xyz;
  }
  auto feature = encroached_boundary(budget, protection, xyz);
  if (const auto mapped = protected_domain_point(budget, seed.domain_map, chart);
      mapped && (!feature || *mapped < *feature))
    feature = mapped;
  return {work->target,
          work->ordinal,
          feature ? FaceCandidateDecision::protected_feature : FaceCandidateDecision::admitted,
          xyz,
          feature,
          std::move(locator)};
}
} // namespace bgmesh
