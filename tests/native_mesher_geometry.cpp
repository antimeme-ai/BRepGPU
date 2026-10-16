#include "../native/mesher/geometry.hpp"
#include "../native/mesher/geometry_store.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using bgfd::Budget;
using bgmesh::Rational;

void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
Rational fraction(Budget &budget, int64_t numerator, int64_t denominator = 1) {
  return Rational(budget, numerator).divide(Rational(budget, denominator));
}
void equal(const Rational &actual, int64_t numerator, int64_t denominator,
           const char *message) {
  require(actual.compare(fraction(actual.budget(), numerator, denominator)) ==
              0,
          message);
}
template <class Function>
void invalid(Function function, const char *message,
             bgmesh::ErrorCode expected = bgmesh::ErrorCode::invalid_source) {
  bool rejected = false;
  try {
    function();
  } catch (const bgmesh::Error &error) {
    rejected = error.code == expected;
  }
  require(rejected, message);
}

void rational_curve(Budget &budget) {
  const std::array knots{0., 0., 0., 1., 1., 1.};
  const std::array controls{0., 0., 1., 2., 0., 2., 2., 2., 1.};
  const auto spans = bgmesh::extract_curve(budget, {2, 2, knots, controls});
  require(spans.size() == 1, "clamped quadratic span inventory");
  const auto point = bgmesh::evaluate_curve(spans[0], fraction(budget, 1, 2));
  equal(point[0], 1, 1, "rational curve x at one half");
  equal(point[1], 1, 3, "rational curve denominator recovery");
  const auto restricted = bgmesh::restrict_curve(
      spans[0], fraction(budget, 1, 4), fraction(budget, 3, 4));
  const auto same = bgmesh::evaluate_curve(restricted, fraction(budget, 1, 2));
  require(point[0].compare(same[0]) == 0 && point[1].compare(same[1]) == 0,
          "rational restriction preserves original parameter evaluation");
}

void unclamped_curve(Budget &budget) {
  const std::array knots{0., 1., 2., 3., 4., 5., 6.};
  const std::array controls{0., 0., 1., 2., 0., 1., 2., 2., 1., 0., 2., 1.};
  const auto spans = bgmesh::extract_curve(budget, {2, 2, knots, controls});
  require(spans.size() == 2, "unclamped natural span inventory");
  equal(spans[0].begin, 2, 1, "natural range starts at U[p]");
  equal(spans[1].end, 4, 1, "natural range ends at U[ncontrols]");
  const auto first = bgmesh::evaluate_curve(spans[0], fraction(budget, 5, 2));
  equal(first[0], 7, 4, "unclamped Cox extraction x");
  equal(first[1], 1, 4, "unclamped Cox extraction y");
  const auto second = bgmesh::evaluate_curve(spans[1], fraction(budget, 7, 2));
  equal(second[0], 7, 4, "unclamped second span x");
  equal(second[1], 7, 4, "unclamped second span y");
  const auto cut = bgmesh::restrict_curve(spans[0], fraction(budget, 9, 4),
                                          fraction(budget, 11, 4));
  equal(cut.controls[0], 23, 16, "restricted coefficient start x");
  equal(cut.controls[1], 1, 16, "restricted coefficient start y");
  equal(cut.controls[3], 29, 16, "restricted coefficient middle x");
  equal(cut.controls[4], 3, 16, "restricted coefficient middle y");
  equal(cut.controls[6], 31, 16, "restricted coefficient end x");
  equal(cut.controls[7], 9, 16, "restricted coefficient end y");
}

void knot_continuity(Budget &budget) {
  const std::array knots{0., 0., 0., .5, .5, 1., 1., 1.};
  const std::array controls{0., 0., 1., 1., 0., 1., 1., 1.,
                            1., 2., 1., 1., 2., 2., 1.};
  const auto spans = bgmesh::extract_curve(budget, {2, 2, knots, controls});
  require(spans.size() == 2, "C0 repeated knot keeps both spans");
  const auto left = bgmesh::evaluate_curve(spans[0], fraction(budget, 1, 2));
  const auto right = bgmesh::evaluate_curve(spans[1], fraction(budget, 1, 2));
  equal(left[0], 1, 1, "left C0 limit");
  require(left[0].compare(right[0]) == 0 && left[1].compare(right[1]) == 0,
          "C0 original one-sided point limits match");
  const std::array discontinuous_knots{0., 0., 0., .5, .5, .5, 1., 1., 1.};
  const std::array matching{0., 0., 1., 1., 0., 1., 1., 1., 1.,
                            2., 2., 2., 4., 2., 2., 4., 4., 2.};
  require(bgmesh::extract_curve(budget, {2, 2, discontinuous_knots, matching})
                  .size() == 2,
          "homogeneous-scale-independent exact point continuity");
  auto bad = matching;
  bad[9] = 3.;
  invalid(
      [&] {
        static_cast<void>(
            bgmesh::extract_curve(budget, {2, 2, discontinuous_knots, bad}));
      },
      "positionally discontinuous original curve rejected");
}

void high_degree_and_zero(Budget &budget) {
  std::vector<double> knots(66, 0.);
  for (std::size_t i = 33; i < knots.size(); ++i)
    knots[i] = 1.;
  std::vector<double> controls(33 * 3, 0.);
  for (std::size_t i = 0; i < 33; ++i)
    controls[3 * i + 2] = 1.;
  controls[32 * 3] = 1.;
  const auto spans = bgmesh::extract_curve(budget, {2, 32, knots, controls});
  const auto point = bgmesh::evaluate_curve(spans[0], fraction(budget, 1, 2));
  equal(point[0], 1, int64_t{1} << 32, "degree32 original t^32 exact answer");
  equal(point[1], 0, 1, "degree32 zero channel");
  const std::array zero_knots{-3., 7.};
  const std::array zero_controls{6., -4., 2.};
  const auto constant =
      bgmesh::extract_curve(budget, {2, 0, zero_knots, zero_controls});
  const auto value = bgmesh::evaluate_curve(constant[0], fraction(budget, 0));
  equal(value[0], 3, 1, "degree0 homogeneous x");
  equal(value[1], -2, 1, "degree0 homogeneous y");
}

void quarter_circle(Budget &budget) {
  constexpr double radius = 1. / 256.;
  const std::array knots{0., 0., 0., 1., 1., 1.};
  const std::array controls{radius, 0., 0., 1.,          radius, radius,
                            0.,     1., 0., 2. * radius, 0.,     2.};
  const auto spans = bgmesh::extract_curve(budget, {3, 2, knots, controls});
  const auto point = bgmesh::evaluate_curve(spans[0], fraction(budget, 1, 2));
  equal(point[0], 3, 5 * 256, "rational quarter circle x");
  equal(point[1], 4, 5 * 256, "rational quarter circle y");
  equal(point[2], 0, 1, "3D curve uses homogeneous stride4");
}

void tensor_surface(Budget &budget) {
  const std::array knots{0., 0., 1., 1.};
  const std::array controls{0., 0., 0., 1., 0., 2., 0., 2.,
                            1., 0., 0., 1., 2., 2., 2., 2.};
  const auto spans =
      bgmesh::extract_surface(budget, {1, 1, 2, 2, knots, knots, controls});
  require(spans.size() == 1, "bilinear original span inventory");
  const auto point = bgmesh::evaluate_surface(spans[0], fraction(budget, 1, 2),
                                              fraction(budget, 1, 2));
  equal(point[0], 1, 2, "rational tensor x");
  equal(point[1], 2, 3, "rational tensor y");
  equal(point[2], 1, 3, "rational tensor positive denominator");
  const auto cut = bgmesh::restrict_surface(
      spans[0], {fraction(budget, 1, 4), fraction(budget, 3, 4),
                 fraction(budget, 1, 4), fraction(budget, 3, 4)});
  const auto same = bgmesh::evaluate_surface(cut, fraction(budget, 1, 2),
                                             fraction(budget, 1, 2));
  for (std::size_t axis = 0; axis < 3; ++axis)
    require(point[axis].compare(same[axis]) == 0,
            "tensor restriction preserves map");
}

void unclamped_surface(Budget &budget) {
  const std::array ku{0., 1., 2., 3., 4., 5., 6.};
  const std::array kv{0., 0., 1., 1.};
  const std::array controls{0., 0., 0., 1., 0., 0., 1., 1., 2., 0., 0.,
                            1., 2., 0., 1., 1., 2., 2., 0., 1., 2., 2.,
                            1., 1., 0., 2., 0., 1., 0., 2., 1., 1.};
  const auto spans =
      bgmesh::extract_surface(budget, {2, 1, 4, 2, ku, kv, controls});
  require(spans.size() == 2,
          "unclamped tensor complete natural root inventory");
  const auto point = bgmesh::evaluate_surface(spans[0], fraction(budget, 5, 2),
                                              fraction(budget, 1, 4));
  equal(point[0], 7, 4, "unclamped tensor Cox U transform");
  equal(point[1], 1, 4, "unclamped tensor source ordering");
  equal(point[2], 1, 4, "unclamped tensor V transform");
}

void cubic_surface(Budget &budget) {
  const std::array knots{0., 0., 0., 0., 1., 1., 1., 1.};
  std::vector<double> controls;
  for (uint32_t i = 0; i < 4; ++i)
    for (uint32_t j = 0; j < 4; ++j) {
      controls.push_back(i == 3 && j == 3 ? 1. : 0.);
      controls.push_back(i == 3 ? 1. : 0.);
      controls.push_back(j == 3 ? 1. : 0.);
      controls.push_back(1.);
    }
  const auto spans =
      bgmesh::extract_surface(budget, {3, 3, 4, 4, knots, knots, controls});
  const auto point = bgmesh::evaluate_surface(spans[0], fraction(budget, 1, 2),
                                              fraction(budget, 1, 4));
  equal(point[0], 1, 512, "tensor cubic mixed monomial u^3v^3");
  equal(point[1], 1, 8, "tensor cubic u^3");
  equal(point[2], 1, 64, "tensor cubic v^3");
}

void triangular_coefficients(Budget &budget) {
  const std::array knots{0., 0., 1., 1.};
  const std::array controls{0., 0., 0., 1., 0., 2., 0., 2.,
                            1., 0., 0., 1., 2., 2., 2., 2.};
  const auto spans =
      bgmesh::extract_surface(budget, {1, 1, 2, 2, knots, knots, controls});
  const std::array uv{fraction(budget, 0), fraction(budget, 0),
                      fraction(budget, 1), fraction(budget, 0),
                      fraction(budget, 0), fraction(budget, 1)};
  const auto triangle = bgmesh::restrict_to_triangle(spans[0], uv);
  require(triangle.degree == 2 && triangle.controls.size() == 24,
          "tensor bilinear becomes triangular total degree2");
  std::size_t coefficient = 0;
  for (int64_t i = 0; i <= 2; ++i)
    for (int64_t j = 0; j <= 2 - i; ++j) {
      equal(triangle.controls[4 * coefficient], i + i * j, 2,
            "independent monomial u+uv triangular coefficients");
      equal(triangle.controls[4 * coefficient + 1], j, 1,
            "independent monomial 2v triangular coefficients");
      equal(triangle.controls[4 * coefficient + 2], i * j, 1,
            "independent monomial 2uv triangular coefficients");
      equal(triangle.controls[4 * coefficient + 3], 2 + j, 2,
            "independent monomial 1+v triangular denominator");
      ++coefficient;
    }

  const std::array cubic_knots{0., 0., 0., 0., 1., 1., 1., 1.};
  std::vector<double> cubic_controls;
  for (uint32_t i = 0; i < 4; ++i)
    for (uint32_t j = 0; j < 4; ++j) {
      cubic_controls.push_back(i == 3 && j == 3 ? 1. : 0.);
      cubic_controls.push_back(i == 3 ? 1. : 0.);
      cubic_controls.push_back(j == 3 ? 1. : 0.);
      cubic_controls.push_back(1.);
    }
  const auto cubic = bgmesh::extract_surface(
      budget, {3, 3, 4, 4, cubic_knots, cubic_knots, cubic_controls});
  const auto degree6 = bgmesh::restrict_to_triangle(cubic[0], uv);
  require(degree6.degree == 6 && degree6.controls.size() == 112,
          "tensor cubic becomes triangular degree6 with28 coefficients");
  coefficient = 0;
  for (int64_t i = 0; i <= 6; ++i)
    for (int64_t j = 0; j <= 6 - i; ++j) {
      equal(degree6.controls[4 * coefficient], i == 3 && j == 3 ? 1 : 0, 20,
            "u^3v^3 requires multinomial coefficient1/20");
      equal(degree6.controls[4 * coefficient + 1], i * (i - 1) * (i - 2), 120,
            "u^3 degree elevation to6");
      equal(degree6.controls[4 * coefficient + 2], j * (j - 1) * (j - 2), 120,
            "v^3 degree elevation to6");
      equal(degree6.controls[4 * coefficient + 3], 1, 1,
            "degree6 denominator partition unity");
      ++coefficient;
    }
}

// Independent original-knot model: de Boor interpolates the active homogeneous
// control polygon directly. It never calls the extracted Bernstein routines.
std::vector<Rational> deboor(Budget &budget, uint32_t degree,
                             std::span<const double> knots,
                             std::span<const Rational> controls,
                             std::size_t stride, const Rational &parameter) {
  const auto count = controls.size() / stride;
  std::size_t span = count;
  for (std::size_t k = degree; k < count; ++k)
    if (knots[k] < knots[k + 1] &&
        parameter.compare(Rational::from_double(budget, knots[k])) >= 0 &&
        parameter.compare(Rational::from_double(budget, knots[k + 1])) <= 0) {
      span = k;
      break;
    }
  require(span != count, "independent de Boor natural parameter admission");
  std::vector<Rational> work;
  for (uint32_t j = 0; j <= degree; ++j)
    for (std::size_t axis = 0; axis < stride; ++axis)
      work.push_back(controls[(span - degree + j) * stride + axis]);
  for (uint32_t r = 1; r <= degree; ++r)
    for (uint32_t j = degree; j >= r; --j) {
      const auto i = span - degree + j;
      const auto left = Rational::from_double(budget, knots[i]);
      const auto right =
          Rational::from_double(budget, knots[i + degree - r + 1]);
      const auto alpha = parameter.subtract(left).divide(right.subtract(left));
      for (std::size_t axis = 0; axis < stride; ++axis) {
        const auto index = stride * j + axis;
        work[index] = work[index - stride]
                          .multiply(Rational(budget, 1).subtract(alpha))
                          .add(work[index].multiply(alpha));
      }
    }
  return {work.begin() + static_cast<std::ptrdiff_t>(degree * stride),
          work.end()};
}

void independent_original_model(Budget &budget) {
  const std::array knots{-2., -2., -2., -2., 0., .5, .5, 2., 2., 2., 2.};
  std::vector<double> controls;
  std::vector<Rational> original;
  for (int64_t i = 0; i < 7; ++i) {
    const int64_t weight = 1 + i % 3;
    const std::array values{static_cast<double>((i - 3) * weight),
                            static_cast<double>((i * i - 3) * weight) / 8.,
                            static_cast<double>((i % 2) * weight) / 4.,
                            static_cast<double>(weight)};
    controls.insert(controls.end(), values.begin(), values.end());
    for (double value : values)
      original.push_back(Rational::from_double(budget, value));
  }
  const auto curves = bgmesh::extract_curve(budget, {3, 3, knots, controls});
  require(curves.size() == 3,
          "nonuniform repeated-knot complete curve inventory");
  for (const auto &curve : curves)
    for (int64_t k = 0; k <= 4; ++k) {
      const auto t = curve.begin.multiply(fraction(budget, 4 - k, 4))
                         .add(curve.end.multiply(fraction(budget, k, 4)));
      const auto expected = deboor(budget, 3, knots, original, 4, t);
      const auto actual = bgmesh::evaluate_curve(curve, t);
      for (std::size_t axis = 0; axis < 3; ++axis)
        require(actual[axis].compare(expected[axis].divide(expected[3])) == 0,
                "original de Boor versus extracted curve exact value");
    }

  const std::array ku{-3., -2., -1., 0., .25, 2., 3., 4.};
  controls.clear();
  original.clear();
  for (int64_t i = 0; i < 5; ++i)
    for (int64_t j = 0; j < 7; ++j) {
      const int64_t weight = 1 + (i + 2 * j) % 3;
      const std::array values{static_cast<double>((i - 2) * weight),
                              static_cast<double>((j - 3) * weight) / 2.,
                              static_cast<double>((i * j - 3) * weight) / 8.,
                              static_cast<double>(weight)};
      controls.insert(controls.end(), values.begin(), values.end());
      for (double value : values)
        original.push_back(Rational::from_double(budget, value));
    }
  const auto surfaces =
      bgmesh::extract_surface(budget, {2, 3, 5, 7, ku, knots, controls});
  require(surfaces.size() == 9,
          "unclamped nonuniform tensor complete grid inventory");
  for (const auto &surface : surfaces) {
    const auto u =
        surface.bounds[0].add(surface.bounds[1]).divide(fraction(budget, 2));
    const auto v =
        surface.bounds[2].add(surface.bounds[3]).divide(fraction(budget, 2));
    std::vector<Rational> rows;
    for (std::size_t i = 0; i < 5; ++i) {
      const auto expected_v = deboor(
          budget, 3, knots,
          std::span<const Rational>(original).subspan(i * 7 * 4, 7 * 4), 4, v);
      rows.insert(rows.end(), expected_v.begin(), expected_v.end());
    }
    const auto expected = deboor(budget, 2, ku, rows, 4, u);
    const auto actual = bgmesh::evaluate_surface(surface, u, v);
    for (std::size_t axis = 0; axis < 3; ++axis)
      require(actual[axis].compare(expected[axis].divide(expected[3])) == 0,
              "original tensor de Boor versus extracted surface exact value");
  }
}

void immutable_original_store(const char *path) {
  const auto resource =
      std::make_shared<bgmesh::RunResource>(uint64_t{1} << 26);
  {
    const auto store = [&] {
      const auto source = bgmesh::load_source(path, resource);
      Budget compiler({uint64_t{1} << 25, uint64_t{1} << 32}, resource.get());
      return bgmesh::compile_geometry(
          source, resource, {{bgmesh::OwnerKind::geometry, 0, 123}, 0, 0},
          compiler);
    }(); // compiler Budget and the caller's mutable source handle die here
    require(store.view().curves.size() == 10 &&
                store.view().carriers.size() == 2,
            "immutable store retains complete original geometry inventory");
    require(store.view().curves[0].roots.count == 2 &&
                store.view().curves[1].roots.count == 0,
            "rational roots and retained analytic formula are distinct");
    Budget reader({uint64_t{1} << 24, uint64_t{1} << 30});
    const auto curve_root = bgmesh::find_curve_root(
        store, bgmesh::Slot<bgmesh::CurveTag>{0}, fraction(reader, 3, 4));
    const auto curve = bgmesh::import_curve_root(reader, store, curve_root);
    const auto point = bgmesh::evaluate_curve(curve, fraction(reader, 3, 4));
    equal(point[0], 7, 4, "frozen root import outlives compiler budget x");
    equal(point[1], 5, 4, "frozen root import outlives compiler budget y");
    const auto surface_root = bgmesh::find_surface_root(
        store, bgmesh::Slot<bgmesh::CarrierTag>{0}, fraction(reader, 3, 4),
        fraction(reader, 3, 4));
    const auto surface =
        bgmesh::import_surface_root(reader, store, surface_root);
    const auto sample = bgmesh::evaluate_surface(
        surface, fraction(reader, 3, 4), fraction(reader, 3, 4));
    equal(sample[0], 3, 4,
          "original natural domain ignores adapter envelope/hint net x");
    equal(sample[1], 6, 7,
          "original controls retained despite wrong saved span hint");
    equal(sample[2], 9, 14,
          "original rational source denominator from frozen roots");
    const auto candidates = bgmesh::intersecting_surface_roots(
        reader, store, bgmesh::Slot<bgmesh::CarrierTag>{0},
        {fraction(reader, 3, 4), fraction(reader, 1), fraction(reader, 3, 4),
         fraction(reader, 1)});
    require(candidates.size() == 1 && candidates[0] == surface_root,
            "indexed closed source rectangle query");
    const auto interior = bgmesh::intersecting_surface_roots(
        reader, store, bgmesh::Slot<bgmesh::CarrierTag>{1},
        {fraction(reader, 1, 8), fraction(reader, 3, 8), fraction(reader, 1, 8),
         fraction(reader, 3, 8)});
    require(interior.size() == 1 && interior[0].value == 1,
            "source span index visits the literal interior root");
    const auto closed_touch = bgmesh::intersecting_surface_roots(
        reader, store, bgmesh::Slot<bgmesh::CarrierTag>{1},
        {fraction(reader, 1, 2), fraction(reader, 1, 2), fraction(reader, 1, 2),
         fraction(reader, 1, 2)});
    require(closed_touch.size() == 4,
            "closed span-box query includes all four knot contacts");
    const auto tie = bgmesh::find_surface_root(
        store, bgmesh::Slot<bgmesh::CarrierTag>{1}, fraction(reader, 1, 2),
        fraction(reader, 1, 2));
    require(tie.value == 1, "point query uses stable lower source span tie");
    const auto upper =
        bgmesh::find_surface_root(store, bgmesh::Slot<bgmesh::CarrierTag>{1},
                                  fraction(reader, 1), fraction(reader, 1));
    require(upper.value == 4,
            "point query retains upper natural-domain boundary");
    invalid(
        [&] {
          static_cast<void>(bgmesh::intersecting_surface_roots(
              reader, store, bgmesh::Slot<bgmesh::CarrierTag>{1},
              {fraction(reader, -1, 4), fraction(reader, 1, 4),
               fraction(reader, 1, 4), fraction(reader, 3, 4)}));
        },
        "partial outside source-cell query must not silently clip coverage",
        bgmesh::ErrorCode::unsupported);
    invalid(
        [&] {
          static_cast<void>(bgmesh::intersecting_surface_roots(
              reader, store, bgmesh::Slot<bgmesh::CarrierTag>{1},
              {fraction(reader, -1), fraction(reader, -1, 2),
               fraction(reader, -1), fraction(reader, -1, 2)}));
        },
        "fully outside source-cell query is unsupported",
        bgmesh::ErrorCode::unsupported);
  }
  require(resource->live() == 0,
          "immutable geometry/source/exact pool ownership released");
}

void invalid_inputs(Budget &budget) {
  const std::array knots{0., 0., 1., 1.};
  const std::array controls{0., 0., 1., 1., 1., 1.};
  invalid(
      [&] {
        static_cast<void>(
            bgmesh::extract_curve(budget, {4, 1, knots, controls}));
      },
      "invalid curve dimension");
  auto bad = controls;
  bad[2] = 0.;
  invalid(
      [&] {
        static_cast<void>(bgmesh::extract_curve(budget, {2, 1, knots, bad}));
      },
      "nonpositive original weight");
  bad = controls;
  bad[0] = std::numeric_limits<double>::infinity();
  invalid(
      [&] {
        static_cast<void>(bgmesh::extract_curve(budget, {2, 1, knots, bad}));
      },
      "nonfinite original homogeneous control");
  const std::array bad_knots{0., 1., 0., 1.};
  invalid(
      [&] {
        static_cast<void>(
            bgmesh::extract_curve(budget, {2, 1, bad_knots, controls}));
      },
      "unordered original knot vector");
  const auto spans = bgmesh::extract_curve(budget, {2, 1, knots, controls});
  invalid(
      [&] {
        static_cast<void>(
            bgmesh::evaluate_curve(spans[0], fraction(budget, 2)));
      },
      "no source extrapolation");
  invalid(
      [&] {
        static_cast<void>(bgmesh::restrict_curve(spans[0], fraction(budget, 1),
                                                 fraction(budget, 0)));
      },
      "reversed restriction rejected");
  auto malformed = spans[0];
  malformed.controls.pop_back();
  invalid(
      [&] {
        static_cast<void>(bgmesh::restrict_curve(malformed, fraction(budget, 0),
                                                 fraction(budget, 1)));
      },
      "malformed owner-local net rejected before restriction indexing");
}
} // namespace

int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected original geometry source fixture path");
    Budget budget({uint64_t{1} << 28, uint64_t{1} << 34});
    rational_curve(budget);
    unclamped_curve(budget);
    knot_continuity(budget);
    high_degree_and_zero(budget);
    quarter_circle(budget);
    tensor_surface(budget);
    unclamped_surface(budget);
    cubic_surface(budget);
    triangular_coefficients(budget);
    independent_original_model(budget);
    immutable_original_store(argv[1]);
    invalid_inputs(budget);
    std::cout << "PASS\toriginal rational geometry\twork=" << budget.work()
              << "\tpeak_bytes=" << budget.peak() << '\n';
    return 0;
  } catch (const bgfd::Failure &error) {
    std::cerr << "FAIL\t" << error.what() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "FAIL\t" << error.what() << '\n';
  }
  return 1;
}
