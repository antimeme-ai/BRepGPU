#include "native_adaptive_cap_work_test.hpp"
#include "../native/mesher/adaptive_surface_cache.hpp"
#include <bit>
#include <numeric>
#include <stdexcept>

using namespace bgmesh;
using namespace bgmesh::adaptive;
CapFacetBatch check_captured_cap_work(JobContext &context, SurfaceBoundView view,
                                       FaceConstructionResult construction,
                                       FaceDomainLease domain, FacetAllowances allowances,
                                       ExactTableKey key) {
  if (!construction)
    throw std::runtime_error("cap work fixture requires a real captured construction");
  std::pmr::vector<uint32_t> selected(&context.budget);
  for (uint32_t i = 0; i < construction->facets.size(); ++i)
    if (construction->facets[i].key.generator == FacetGenerator::mapped_cap)
      selected.push_back(i);
  if (selected.empty())
    throw std::runtime_error("cap work fixture has no mapped source facet");
  const auto work = prepare_cap_facet_work(context, std::move(view), construction,
                                           domain, allowances, selected, key);
  if (work.facets().size() != selected.size())
    throw std::runtime_error("cap work omitted a selected mapped facet");
  std::pmr::vector<uint32_t> indices(work.facets().size(), &context.budget);
  std::iota(indices.begin(), indices.end(), uint32_t{0});
  auto packed = pack_cap_facets(context, work, indices);
  if (packed.inputs.size() != indices.size() || packed.indices.size() != indices.size())
    throw std::runtime_error("cap work pack omitted a selected numerical task");
  for (uint32_t i = 0; i < packed.inputs.size(); ++i) {
    const auto &record = work.facets()[packed.indices[i]];
    const auto &facet = construction->facets[record.construction_facet];
    if (!facet.mapped)
      throw std::runtime_error("cap work lost captured mapped generator");
    for (uint32_t role = 0; role < 3; ++role)
      for (uint32_t axis = 0; axis < 3; ++axis) {
        const auto truth = construction->sites[facet.sites[facet.mapped->source_role_to_facet[role]]]
                               .actual_xyz[axis];
        if (std::bit_cast<uint64_t>(packed.inputs[i].input.actual_xyz[3 * role + axis]) !=
            std::bit_cast<uint64_t>(truth))
          throw std::runtime_error("cap pack changed SAME actual Q or source-role order");
      }
  }
  const auto refused = [&](FaceConstructionResult mesh, std::span<const uint32_t> subset,
                            ExactTableKey table) {
    try {
      (void)prepare_cap_facet_work(context, mesh->surface, mesh, domain, allowances, subset, table);
    } catch (const Error &error) {
      if (error.code == ErrorCode::invalid_source)
        return;
      throw;
    }
    throw std::runtime_error("cap work accepted changed captured authority or invalid subset");
  };
  const std::array duplicate{selected.front(), selected.front()};
  refused(construction, duplicate, key);
  refused(construction, selected, construction->values.key());
  refused(construction, selected, construction->curves->values.key());
  refused(construction, selected, construction->curves->boundary->parameters.key());
  if (construction->cap_points)
    refused(construction, selected, construction->cap_points->values.key());
  auto changed = std::allocate_shared<FaceConstructionResultData>(
      RunAllocator<FaceConstructionResultData>(construction->resource), *construction);
  const auto &first = changed->facets[selected.front()];
  const auto pole = first.sites[first.mapped->source_role_to_facet[0]];
  changed->sites[pole].actual_xyz[0] = std::bit_cast<double>(
      std::bit_cast<uint64_t>(changed->sites[pole].actual_xyz[0]) ^ uint64_t{1});
  refused(std::move(changed), selected, key);
  for (uint32_t i = 0; i < construction->facets.size(); ++i)
    if (construction->facets[i].key.generator != FacetGenerator::mapped_cap) {
      const std::array ordinary{i};
      refused(construction, ordinary, key);
      break;
    }
  if (work.facets().front().physical_nonzero) {
    const auto precise = compare_cap_work_facet_precise(context, work, 0);
    if (precise.bounds.facet_roles != work.facets().front().positive_chart_roles ||
        precise.bounds.regular_sheet_sign != work.facets().front().regular_sheet_sign)
      throw std::runtime_error("precise cap work changed captured map roles");
  }
  return packed;
}
