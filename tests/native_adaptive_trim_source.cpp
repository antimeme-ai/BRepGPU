#include "../native/mesher/adaptive_geometry.hpp"
#include <array>
#include <iostream>
#include <stdexcept>

using namespace bgmesh;
using namespace bgmesh::adaptive;
namespace {
void require(bool yes, const char *message) {
  if (!yes)
    throw std::runtime_error(message);
}
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 2)
      throw std::runtime_error("original analytic source required");
    const auto resource = std::make_shared<RunResource>(uint64_t{1} << 28);
    const auto source = load_source(argv[1], resource);
    bgfd::Budget budget({uint64_t{1} << 27, uint64_t{1} << 32}, resource.get());
    const auto slot = source.view().carrier_index.find(CarrierId{9});
    const auto kind = source.view().carriers[slot.value].kind;
    require(kind >= 2 && kind <= 5, "regular curved analytic fixture required");
    const bool torus = kind == 5;
    const std::array requests{
        CarrierRequest{slot, {0., 8., 0., torus ? 8. : 1.}}};
    const auto cache =
        prepare_trim_source_cache(source, requests, resource, {}, {}, budget);
    require(cache.purpose() == SourceCachePurpose::regular_patch,
            "tiled source is not regular");
    require(cache.original_roots().size() == 1,
            "analytic original evaluator was duplicated");
    const auto &original = cache.original_roots()[0];
    require(original.original_box[0].lower == 0. &&
                original.original_box[1].upper == 8. &&
                original.original_box[2].lower == 0. &&
                original.original_box[3].upper == (torus ? 8. : 1.),
            "original evaluator envelope changed");
    require(cache.roots().size() == (torus ? 64 : 8),
            "source angular panel inventory");
    for (uint32_t i = 0; i < 8; ++i)
      for (uint32_t j = 0; j < (torus ? 8U : 1U); ++j) {
        const auto &root = cache.roots()[i * (torus ? 8U : 1U) + j];
        require(root.original.value == 0 && root.span_u == i &&
                    root.span_v == j,
                "prepared panel original/path identity changed");
        const std::array expected{double(i), double(i + 1), double(j),
                                  double(j + 1)};
        for (uint32_t axis = 0; axis < 4; ++axis)
          require(root.prepared_box[axis].lower == expected[axis] &&
                      root.prepared_box[axis].upper == expected[axis] &&
                      root.original_box[axis].lower ==
                          original.original_box[axis].lower &&
                      root.original_box[axis].upper ==
                          original.original_box[axis].upper,
                  "prepared grid is not exact expected full cover");
        for (auto owner : root.sample_owners)
          require(owner.value == 0, "tile sample changed original evaluator");
      }
    bool old_profile_refused = false;
    try {
      (void)prepare_source_cache(source, requests, resource, {}, {}, budget);
    } catch (const Error &error) {
      old_profile_refused = error.code == ErrorCode::unsupported;
    }
    require(old_profile_refused, "old patch profile was silently widened");
    SourceCacheLimits limited;
    limited.roots = 7;
    bool exhausted = false;
    try {
      (void)prepare_trim_source_cache(source, requests, resource, {}, limited,
                                      budget);
    } catch (const Error &error) {
      exhausted = error.code == ErrorCode::resource_exhausted;
    }
    require(exhausted, "panel capacity did not refuse");
    const std::array no_cut{CarrierRequest{slot, {1e16, 1e16 + 2., 0., 1.}}};
    bool unresolved_cut = false;
    try {
      (void)prepare_trim_source_cache(source, no_cut, resource, {}, {}, budget);
    } catch (const Error &error) {
      unresolved_cut = error.code == ErrorCode::unresolved;
    }
    require(unresolved_cut,
            "unrepresentable angular cut was silently accepted");
    std::cout << "original-analytic-tiles\t" << kind << "\tPASS\t"
              << cache.roots().size() << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
