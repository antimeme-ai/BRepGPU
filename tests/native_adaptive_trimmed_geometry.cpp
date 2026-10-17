#include "../native/mesher/adaptive_trimmed_geometry.hpp"
#include "native_adaptive_device_test.hpp"
#include <array>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

int main(int argc, char **argv) {
  try {
    if (argc != 2 && argc != 5)
      throw std::runtime_error("original geometry fixture required");
    using namespace bgmesh;
    using namespace bgmesh::adaptive;
    auto resource = std::make_shared<RunResource>(uint64_t{1} << 28);
    const auto source = load_source(argv[1], resource);
    bgfd::Budget budget({uint64_t{1} << 27, uint64_t{1} << 34});
    const auto carrier = source.view().carrier_index.find(CarrierId{9});
    const std::array requests{CarrierRequest{carrier, {0., 1., 0., 1.}}};
    const auto cache = prepare_source_cache(source, requests, resource, {}, {}, budget);
    std::unique_ptr<adaptive_test::DeviceFixture> device;
    if (argc == 5) {
      if (std::string_view(argv[2]) != "--cuda")
        throw std::runtime_error("expected --cuda module device");
      device = std::make_unique<adaptive_test::DeviceFixture>(cache, argv[3], std::stoi(argv[4]));
    }
    const auto line = source.view().curve_index.find(CurveId{3000});
    const auto curved = source.view().curve_index.find(CurveId{7});
    std::vector<CurveRequest> curve_requests{CurveRequest{line, {0., 1.}},
                                             CurveRequest{curved, {0., 1.}}};
    for (uint32_t i = 0; i < source.view().curves.size(); ++i)
      if (source.view().curves[i].id.value >= 50000)
        curve_requests.push_back({{i}, {0., 1.}});
    const auto curve_cache =
        prepare_curve_source_cache(source, curve_requests, resource, {}, {}, budget);
    std::cout << std::hexfloat;
    for (const bool reverse : {false, true}) {
      const std::array interval{Rational(budget, reverse ? 1 : 0),
                                Rational(budget, reverse ? 0 : 1)};
      const auto image = original_curve_line_image(curve_cache, line, interval, budget);
      if (!image || image->endpoints[0].compare(interval[0]) != 0 ||
          image->endpoints[2].compare(interval[1]) != 0 || image->endpoints[1].sign() != 0 ||
          image->endpoints[3].sign() != 0 || image->scalars[1].compare(image->scalars[0]) <= 0)
        throw std::runtime_error("exact original line image traversal");
      std::cout << "trim-line-image\t" << (reverse ? -1 : 1);
      for (const auto &value : image->endpoints)
        std::cout << '\t' << value.to_double();
      for (const auto &value : image->covector)
        std::cout << '\t' << value.to_double();
      for (const auto &value : image->scalars)
        std::cout << '\t' << value.to_double();
      std::cout << '\n';
    }
    const std::array interval{Rational(budget), Rational(budget, 1)};
    if (original_curve_line_image(curve_cache, curved, interval, budget))
      throw std::runtime_error("curved source received exact line-image profile");
    for (uint32_t i = 0; i < source.view().curves.size(); ++i) {
      const auto id = source.view().curves[i].id.value;
      if (id < 50000)
        continue;
      const auto image = original_curve_line_image(curve_cache, {i}, interval, budget);
      if (static_cast<bool>(image) != (id != 50001))
        throw std::runtime_error(
            "stationary/degree32/weighted/backtracking original image profile");
    }
    for (double displacement : {0., .125}) {
      SourceTriangleInput input;
      input.original = cache.roots()[0].original;
      const double uv[]{0., 0., 1., 0., 0., 1.};
      double xyz[]{0., 0., displacement, 1., 0., displacement, 0., 1., displacement};
      if (cache.original_roots()[input.original.value].family != GeometryFamily::rational_tensor)
        for (uint32_t vertex = 0; vertex < 3; ++vertex) {
          SampleInput point;
          point.original = input.original;
          point.uv[0] = {uv[vertex * 2], uv[vertex * 2]};
          point.uv[1] = {uv[vertex * 2 + 1], uv[vertex * 2 + 1]};
          const auto value = evaluate_sample(cache, point);
          if (value.status != NumericStatus::ready)
            throw std::runtime_error("analytic original triangle corner");
          for (uint32_t axis = 0; axis < 3; ++axis)
            xyz[vertex * 3 + axis] = value.actual_xyz[axis];
          xyz[vertex * 3 + 2] += displacement;
        }
      for (size_t i = 0; i < 6; ++i)
        input.normalized_uv[i] = {uv[i], uv[i]};
      for (size_t i = 0; i < 9; ++i)
        input.affine_xyz[i] = {xyz[i], xyz[i]};
      input.allowance_down = 2.;
      const auto result =
          device ? device->source_triangle(input) : bound_source_triangle(cache, input);
      if (result.status != NumericStatus::ready)
        throw std::runtime_error("original rational actual-triangle bound");
      std::cout << "trim-source-triangle\t" << displacement << '\t' << result.position_upper;
      for (double value : uv)
        std::cout << '\t' << value;
      for (double value : xyz)
        std::cout << '\t' << value;
      std::cout << '\n';
      if (cache.original_roots()[input.original.value].family == GeometryFamily::plane &&
          cache.original_roots()[input.original.value].frame[0] == 3.) {
        std::array<Rational, 6> exact_uv{Rational(budget), Rational(budget), Rational(budget),
                                         Rational(budget), Rational(budget), Rational(budget)};
        std::array<Rational, 9> exact_xyz{Rational(budget), Rational(budget), Rational(budget),
                                          Rational(budget), Rational(budget), Rational(budget),
                                          Rational(budget), Rational(budget), Rational(budget)};
        for (uint32_t i = 0; i < 6; ++i)
          exact_uv[i] = Rational::from_double(budget, uv[i]);
        for (uint32_t i = 0; i < 9; ++i)
          exact_xyz[i] = Rational::from_double(budget, xyz[i]);
        const std::vector<double> caps =
            displacement == 0. ? std::vector<double>{0.} : std::vector<double>{.125, .0625};
        for (const auto cap : caps) {
          const auto allowance = Rational::from_double(budget, cap);
          const auto exact = compare_original_triangle_precise(cache, input.original, exact_uv,
                                                               exact_xyz, allowance, budget);
          if (exact.certified != (cap >= displacement))
            throw std::runtime_error("exact plane zero/equality threshold comparison");
          std::cout << "trim-precise-plane\t" << cap << '\t' << exact.certified << '\t'
                    << exact.bounds.position_upper;
          for (const auto value : uv)
            std::cout << '\t' << value;
          for (const auto value : xyz)
            std::cout << '\t' << value;
          std::cout << '\n';
        }
      }
    }
    for (double displacement : {0., -.125}) {
      ActualFacetInput input;
      const double corners[]{0., 0., 0., 1., 0., 0., 0., 1., 0., 1., 1., 1.};
      for (size_t i = 0; i < 12; ++i)
        input.actual_corners[i] = corners[i];
      const double uv[]{0., 0., .125, 0., 0., .125};
      for (size_t i = 0; i < 6; ++i)
        input.uv[i] = {uv[i], uv[i]};
      for (size_t i = 0; i < 3; ++i) {
        input.affine_xyz[3 * i] = {uv[2 * i], uv[2 * i]};
        input.affine_xyz[3 * i + 1] = {uv[2 * i + 1], uv[2 * i + 1]};
        input.affine_xyz[3 * i + 2] = {displacement, displacement};
      }
      const auto result = device ? device->facet(input) : bound_actual_facet(input);
      if (result.status != NumericStatus::ready || result.total_upper >= .004 - displacement)
        throw std::runtime_error("skinny actual-piece covariance bound");
      std::cout << "trim-skinny-facet\t" << displacement << '\t' << result.total_upper << '\n';
    }
    if (device)
      device->close();
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
