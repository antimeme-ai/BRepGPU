#include "../native/mesher/adaptive_packets.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>
#include <stop_token>
#include <string>

namespace {
using namespace bgmesh;
using namespace bgmesh::adaptive;
struct Fraction final {
  int64_t numerator = 0, denominator = 1;
  Fraction() = default;
  Fraction(int64_t n) : numerator(n) {} // NOLINT(google-explicit-constructor)
  // Numerator and denominator are intentionally ordered arithmetic operands.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  Fraction(int64_t n, int64_t d) : numerator(n), denominator(d) {
    const auto divisor = std::gcd(n, d);
    numerator /= divisor;
    denominator /= divisor;
  }
  auto operator<=>(const Fraction &other) const {
    return numerator * other.denominator <=> other.numerator * denominator;
  }
  bool operator==(const Fraction &) const = default;
};
using XY = std::array<Fraction, 2>;
struct Rectangle final {
  std::array<Fraction, 4> box;
  bool enabled = true;
};
struct Input final {
  std::string name;
  std::vector<Rectangle> rectangles;
  bool overflow_score = false;
};
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
void fraction(Fraction value) { std::cout << '\t' << value.numerator << '\t' << value.denominator; }
Rational exact(bgfd::Budget &budget, Fraction value) {
  return Rational(budget, value.numerator).divide(Rational(budget, value.denominator));
}
std::array<XY, 4> corners(const Rectangle &rectangle) {
  const auto &b = rectangle.box;
  return {{{b[0], b[2]}, {b[1], b[2]}, {b[0], b[3]}, {b[1], b[3]}}};
}
Input junction(uint32_t count, bool overflow = false) {
  Input input{"junction" + std::to_string(count), {{{0, Fraction(1, 2), 0, 1}}}, overflow};
  for (uint32_t i = 0; i < count; ++i)
    input.rectangles.push_back({{{Fraction(1, 2), 1, Fraction(i, count), Fraction(i + 1, count)}}});
  return input;
}
void execute(const Input &input, uint64_t ordinal, const std::shared_ptr<RunResource> &resource,
             const SourceCache &source) {
  bgfd::Budget scratch({1ULL << 27, 1ULL << 30}, resource.get());
  ExactBuilder builder(resource, {{OwnerKind::run, 0, ordinal}, 1, 0});
  std::vector<Fraction> values;
  const auto append = [&](Fraction value) {
    values.push_back(value);
    return builder.append(exact(scratch, value));
  };
  std::map<XY, uint32_t> slots;
  for (const auto &rectangle : input.rectangles)
    for (const auto &point : corners(rectangle))
      slots.emplace(point, 0);
  uint32_t next = 0;
  std::vector<PointRecord> points;
  for (auto &[point, slot] : slots) {
    slot = next++;
    const std::array uv{append(point[0]), append(point[1])};
    const auto u = exact(scratch, point[0]).to_double(), v = exact(scratch, point[1]).to_double();
    const std::array<double, 3> xyz =
        input.overflow_score
            ? std::array<double, 3>{u < .5 ? -1.e308 : 1.e308, v < .5 ? -1.e308 : 1.e308, 0.}
            : std::array<double, 3>{2. + 2. * u + v, -1. + 3. * v, .5};
    points.push_back({{{0}, uv, ArithmeticStage::rn64, 0}, uv, uv, xyz, 0.});
  }
  std::vector<SelectedLeaf> leaves;
  std::vector<uint32_t> enabled;
  for (const auto &rectangle : input.rectangles) {
    const auto index = static_cast<uint32_t>(leaves.size());
    const auto input_corners = corners(rectangle);
    std::array<PointSlot, 4> point_slots{};
    for (std::size_t i = 0; i < point_slots.size(); ++i)
      point_slots[i] = PointSlot{slots.at(input_corners[i])};
    const auto &b = rectangle.box;
    leaves.push_back({{{0}, 32, {index, 1}},
                      {append(b[0]), append(b[1]), append(b[2]), append(b[3])},
                      point_slots});
    if (rectangle.enabled)
      enabled.push_back(index);
  }
  auto selection =
      std::allocate_shared<PatchSelection>(RunAllocator<PatchSelection>(resource), resource, source,
                                           builder.seal(), SelectionKey{1, ordinal, 1, 1});
  selection->points.assign(points.begin(), points.end());
  selection->leaves.assign(leaves.begin(), leaves.end());
  for (uint64_t i = 0; i < leaves.size(); ++i)
    selection->path_words.push_back(i);
  const auto mesh = build_packet_mesh(scratch, resource, selection, enabled);
  require(mesh && mesh->selection == selection, "packet result lost input lease");
  if (input.name == "nondyadic")
    require(mesh->stats.rational_comparisons > 0, "general rational ordering was not exercised");
  if (input.name == "junction32")
    require(mesh->stats.dyadic_comparisons > 0, "dyadic shifted-word ordering was not exercised");
  std::cout << "case\t" << input.name << '\n';
  for (const auto &[point, slot] : slots) {
    std::cout << "point\t" << slot;
    fraction(point[0]);
    fraction(point[1]);
    std::cout << '\n';
  }
  for (uint32_t i = 0; i < input.rectangles.size(); ++i) {
    std::cout << "leaf\t" << i;
    for (const auto value : input.rectangles[i].box)
      fraction(value);
    for (const auto point : selection->leaves[i].corners)
      std::cout << '\t' << point.value;
    std::cout << '\t' << static_cast<int>(input.rectangles[i].enabled) << '\n';
  }
  for (uint32_t i = 0; i < mesh->lines.size(); ++i) {
    const auto &line = mesh->lines[i];
    require(line.constant.table == selection->values.key() && line.constant.index < values.size(),
            "packet line lost original exact coordinate");
    std::cout << "line\t" << i << '\t' << line.axis;
    fraction(values[line.constant.index]);
    for (uint64_t j = 0; j < line.events.count; ++j)
      std::cout << '\t' << mesh->line_points[line.events.first + j].value;
    std::cout << '\n';
  }
  for (const auto &packet : mesh->packets)
    for (uint32_t side = 0; side < 4; ++side) {
      const auto &use = mesh->sides[packet.sides.first + side];
      std::cout << "side\t" << packet.leaf << '\t' << side << '\t' << use.line << '\t' << use.first
                << '\t' << use.count << '\t' << static_cast<int>(use.reverse) << '\n';
    }
  for (const auto &triangle : mesh->triangles)
    std::cout << "triangle\t" << triangle.leaf << '\t' << triangle.points[0].value << '\t'
              << triangle.points[1].value << '\t' << triangle.points[2].value << '\n';
  for (const auto &rim : mesh->rims)
    std::cout << "rim\t" << rim.leaf << '\t' << rim.points[0].value << '\t' << rim.points[1].value
              << '\n';
  for (const auto leaf : mesh->demoted_leaf_slots)
    std::cout << "demote\t" << leaf << '\n';
  std::cout << "end\n";
  if (!enabled.empty()) {
    const std::array duplicate{enabled.front(), enabled.front()};
    bool rejected = false;
    try {
      static_cast<void>(build_packet_mesh(scratch, resource, selection, duplicate));
    } catch (const Error &error) {
      rejected = error.code == ErrorCode::invalid_source;
    }
    require(rejected, "duplicate packet request accepted");
    bgfd::Budget empty({1ULL << 20, 0}, resource.get());
    rejected = false;
    try {
      static_cast<void>(build_packet_mesh(empty, resource, selection, enabled));
    } catch (const bgfd::Failure &failure) {
      rejected = failure.status == BGFD_RESOURCE_EXHAUSTED;
    }
    require(rejected, "zero-work packet request accepted");
  }
  if (input.name == "square") {
    const auto expect_invalid = [&](const auto &action) {
      bool rejected = false;
      try {
        action();
      } catch (const Error &error) {
        rejected = error.code == ErrorCode::invalid_source;
      }
      require(rejected, "invalid packet request did not produce its declared error");
    };
    const std::array missing{static_cast<uint32_t>(selection->leaves.size())};
    expect_invalid(
        [&] { static_cast<void>(build_packet_mesh(scratch, resource, selection, missing)); });
    const auto other_resource = std::make_shared<RunResource>(1ULL << 20);
    expect_invalid(
        [&] { static_cast<void>(build_packet_mesh(scratch, other_resource, selection, enabled)); });
    auto alias =
        std::allocate_shared<PatchSelection>(RunAllocator<PatchSelection>(resource), resource,
                                             source, selection->values, selection->key);
    alias->points = selection->points;
    alias->leaves = selection->leaves;
    alias->path_words = selection->path_words;
    alias->points[0].chart_xy[0].index = selection->values.size();
    expect_invalid(
        [&] { static_cast<void>(build_packet_mesh(scratch, resource, alias, enabled)); });
    alias->points = selection->points;
    alias->points[0].actual_xyz[0] = std::numeric_limits<double>::infinity();
    expect_invalid(
        [&] { static_cast<void>(build_packet_mesh(scratch, resource, alias, enabled)); });
    bgfd::Budget tiny({1, 1ULL << 20}, resource.get());
    bool rejected = false;
    try {
      static_cast<void>(build_packet_mesh(tiny, resource, selection, enabled));
    } catch (const bgfd::Failure &failure) {
      rejected = failure.status == BGFD_RESOURCE_EXHAUSTED;
    }
    require(rejected, "one-byte scratch did not exhaust atomically");
    std::stop_source cancellation;
    cancellation.request_stop();
    bgfd::Budget stopped({1ULL << 20, 1ULL << 20}, resource.get(), cancellation.get_token());
    rejected = false;
    try {
      static_cast<void>(build_packet_mesh(stopped, resource, selection, enabled));
    } catch (const bgfd::Cancelled &) {
      rejected = true;
    }
    require(rejected, "packet ignored pre-existing cancellation");
  }
  if (input.name == "junction2") {
    auto alias =
        std::allocate_shared<PatchSelection>(RunAllocator<PatchSelection>(resource), resource,
                                             source, selection->values, selection->key);
    alias->points = selection->points;
    alias->leaves = selection->leaves;
    alias->path_words = selection->path_words;
    const auto original = alias->leaves[1].corners[0];
    auto duplicate = alias->points[original.value];
    ++duplicate.key.constructed_generation;
    alias->leaves[1].corners[0] = PointSlot{static_cast<uint32_t>(alias->points.size())};
    alias->points.push_back(duplicate);
    bool rejected = false;
    try {
      static_cast<void>(build_packet_mesh(scratch, resource, alias, enabled));
    } catch (const Error &error) {
      rejected = error.code == ErrorCode::invalid_source;
    }
    require(rejected, "same-coordinate conflicting PointPool identities were welded");
  }
}
} // namespace

int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected actual source fixture path");
    const auto resource = std::make_shared<RunResource>(1ULL << 28);
    bgfd::Budget budget({1ULL << 27, 1ULL << 30}, resource.get());
    const auto source = load_source(argv[1], resource);
    const std::array request{
        CarrierRequest{source.view().carrier_index.find(CarrierId{10}), {0., 1., 0., 1.}}};
    const auto roots = prepare_source_cache(source, request, resource, {}, {}, budget);
    std::vector<Input> inputs{
        {"square", {{{0, 1, 0, 1}}}},
        junction(2),
        junction(8),
        junction(32),
        {"nondyadic",
         {{{0, Fraction(1, 3), 0, 1}},
          {{Fraction(1, 3), 1, 0, Fraction(1, 3)}},
          {{Fraction(1, 3), 1, Fraction(1, 3), 1}}}},
        {"negative", {{{-8, -6, 16, 24}}, {{-6, -4, 16, 20}}, {{-6, -4, 20, 24}}}},
        {"pinch",
         {{{0, Fraction(1, 2), 0, Fraction(1, 2)}, true},
          {{Fraction(1, 2), 1, 0, Fraction(1, 2)}, false},
          {{0, Fraction(1, 2), Fraction(1, 2), 1}, false},
          {{Fraction(1, 2), 1, Fraction(1, 2), 1}, true}}},
        {"empty", {{{0, 1, 0, 1}, false}}}};
    auto overflow = junction(4, true);
    overflow.name = "overflow";
    inputs.push_back(std::move(overflow));
    Input ring{"ring", {}};
    for (int64_t i = 0; i < 3; ++i)
      for (int64_t j = 0; j < 3; ++j)
        ring.rectangles.push_back(
            {{Fraction(i, 3), Fraction(i + 1, 3), Fraction(j, 3), Fraction(j + 1, 3)},
             i != 1 || j != 1});
    inputs.push_back(std::move(ring));
    for (std::size_t i = 0; i < inputs.size(); ++i)
      execute(inputs[i], i + 1, resource, roots);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
  } catch (const bgfd::Failure &error) {
    std::cerr << error.what() << '\n';
  }
  return 1;
}
