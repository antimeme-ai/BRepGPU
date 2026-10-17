#include "../native/mesher/assembly.hpp"
#include <bit>
#include <iostream>
#include <stdexcept>

namespace {
using bgmesh::Rational;
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
void actual(const bgmesh::SourceStore &source, const std::shared_ptr<bgmesh::RunResource> &run) {
  bgfd::Budget budget({1u << 24, uint64_t{1} << 32}, run.get());
  const auto zero = Rational(budget, 0), one = Rational(budget, 1);
  const std::array bounds{zero, one, zero, one, zero, one};
  const std::array boxes{bgmesh::DefinitionEnvelopeView{source, {0}, bounds},
                         bgmesh::DefinitionEnvelopeView{source, {1}, bounds}};
  const auto epsilon = Rational::from_double(budget, .001);
  const auto delta = Rational::from_double(budget, .0001);
  const auto assembly = bgmesh::prepare_assembly(budget, source, run, boxes, {epsilon, delta},
                                                 {{bgmesh::OwnerKind::run, 0, 30}, 0, 0});
  const auto &data = assembly.view();
  require(data.instances.size() == 2 && data.definitions.size() == 2,
          "reused definitions produce both instances and retain unplaced definitions");
  const auto &nested = data.instances[0], &direct = data.instances[1];
  require(nested.path.count == 4 && direct.path.count == 1 &&
              data.paths[nested.path.first].value == 201 &&
              data.paths[direct.path.first].value == 205,
          "canonical usage-ID path order does not follow unsorted source rows");
  const auto s = one.add(one.divide(Rational(budget, int64_t{1} << 40)));
  const auto exact_scale = s.multiply(s).multiply(s);
  require(nested.matrix[0] == 0 && nested.matrix[1] == -1 &&
              std::bit_cast<uint64_t>(nested.matrix[4]) ==
                  std::bit_cast<uint64_t>(exact_scale.to_double()) &&
              nested.matrix[3] == 13 && nested.matrix[7] == -7 && nested.matrix[11] == .5,
          "nested source matrices compose exactly and store once-rounded entries");
  const auto local = bgmesh::import_exact(budget, data.values.view(data.definitions[0].positional));
  const auto node = bgmesh::import_exact(budget, data.values.view(data.definitions[0].node_source));
  const auto orphan =
      bgmesh::import_exact(budget, data.values.view(data.definitions[1].positional));
  require(
      local.sign() > 0 && local.multiply(exact_scale).compare(epsilon) <= 0 &&
          local.compare(epsilon.multiply(Rational(budget, 999)).divide(Rational(budget, 1000))) > 0,
      "world positional tolerance transports through the true composed scale");
  require(node.sign() > 0 && node.multiply(exact_scale).compare(delta) <= 0 &&
              orphan.compare(epsilon) == 0,
          "node cap scales separately and unplaced definitions retain their allowance");
  // Every actual composed-versus-stored displacement on the eight source-box
  // corners must fit what remains after local geometry error. This uses the
  // literal diagonal scale, not the production Gershgorin/eta implementation.
  for (int64_t x = 0; x <= 1; ++x)
    for (int64_t y = 0; y <= 1; ++y)
      for (int64_t z = 0; z <= 1; ++z) {
        const std::array point{Rational(budget, x), Rational(budget, y), Rational(budget, z)};
        const std::array expected{Rational(budget, 13).subtract(point[1]),
                                  exact_scale.multiply(point[0]).subtract(Rational(budget, 7)),
                                  point[2].add(one.divide(Rational(budget, 2)))};
        Rational squared_error(budget, 0);
        for (std::size_t row = 0; row < 3; ++row) {
          auto stored = Rational::from_double(budget, nested.matrix[row * 4 + 3]);
          for (std::size_t col = 0; col < 3; ++col)
            stored = stored.add(
                Rational::from_double(budget, nested.matrix[row * 4 + col]).multiply(point[col]));
          const auto error = expected[row].subtract(stored);
          squared_error = squared_error.add(error.multiply(error));
        }
        const auto remaining = epsilon.subtract(local.multiply(exact_scale));
        require(remaining.sign() >= 0 && squared_error.compare(remaining.multiply(remaining)) <= 0,
                "actual stored placement plus allowed mesh departure fits world epsilon");
      }
  bool limited = false;
  try {
    (void)bgmesh::prepare_assembly(budget, source, run, boxes, {epsilon, delta},
                                   {{bgmesh::OwnerKind::run, 0, 31}, 0, 0}, {}, {1});
  } catch (const bgmesh::Error &error) {
    limited = error.code == bgmesh::ErrorCode::resource_exhausted;
  }
  require(limited, "expanded instance limit fails without dropping a source path");
  bool precision = false;
  try {
    const auto tiny = Rational::from_double(budget, 0x1p-100);
    (void)bgmesh::prepare_assembly(budget, source, run, boxes, {tiny, delta},
                                   {{bgmesh::OwnerKind::run, 0, 32}, 0, 0});
  } catch (const bgmesh::Error &error) {
    precision = error.code == bgmesh::ErrorCode::unresolved;
  }
  require(precision, "stored transform error cannot consume an arbitrarily small world allowance");
  const auto rejected = [&](std::span<const bgmesh::DefinitionEnvelopeView> input) {
    bool invalid = false;
    try {
      (void)bgmesh::prepare_assembly(budget, source, run, input, {epsilon, delta},
                                     {{bgmesh::OwnerKind::run, 0, 33}, 0, 0});
    } catch (const bgmesh::Error &error) {
      invalid = error.code == bgmesh::ErrorCode::invalid_source;
    }
    require(invalid, "definition envelopes must cover exactly the original source inventory");
  };
  rejected(std::span(boxes).first(1));
  const std::array duplicate{boxes[0], boxes[0]};
  rejected(duplicate);
  const std::array reversed{one, zero, zero, one, zero, one};
  const std::array invalid_box{bgmesh::DefinitionEnvelopeView{source, {0}, reversed}, boxes[1]};
  rejected(invalid_box);
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "assembly source fixture argument required");
    auto run = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 27);
    const auto source = bgmesh::load_source(argv[1], run);
    actual(source, run);
    std::cout << "PASS\texact original placements and effective definition allowances\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL\t" << error.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &error) {
    std::cerr << "FAIL\t" << error.what() << '\n';
    return 1;
  }
}
