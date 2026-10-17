#include "../native/mesher/adaptive_assembly.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace {
using namespace bgmesh;
using namespace bgmesh::adaptive;
void require(bool good, const char *message) {
  if (!good)
    throw std::runtime_error(message);
}
void exact(const ExactTable &table, ExactRef ref) {
  const auto value = table.view(ref);
  std::cout << '\t' << value.sign << ':';
  const auto words = [](std::span<const uint32_t> values) {
    if (values.empty())
      std::cout << '0';
    for (auto it = values.rbegin(); it != values.rend(); ++it)
      std::cout << std::hex << std::setw(8) << std::setfill('0') << *it;
    std::cout << std::dec;
  };
  words(value.numerator);
  std::cout << ':';
  words(value.denominator);
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "original assembly source argument required");
    auto run = std::make_shared<RunResource>(uint64_t{1} << 28);
    const auto source = load_source(argv[1], run);
    bgfd::Budget budget({uint64_t{1} << 27, uint64_t{1} << 36}, run.get());
    const auto epsilon = Rational::from_double(budget, .001);
    const auto delta = Rational::from_double(budget, .0001);
    const AdaptiveAssemblyKeys keys{{{OwnerKind::run, 0, 90}, 1, 0},
                                    {{OwnerKind::run, 0, 90}, 1, 1}};
    const auto output = prepare_adaptive_assembly(budget, source, run, {epsilon, delta}, keys);
    require(output->definitions.size() == source.view().definitions.size() &&
                output->faces.size() == source.view().faces.size(),
            "complete source envelope inventory");
    for (const auto &row : output->definitions) {
      std::cout << "definition\t" << source.view().definitions[row.source.value].id.value;
      for (auto ref : row.xyz)
        exact(output->envelope_values, ref);
      std::cout << '\n';
    }
    for (const auto &row : output->carriers) {
      std::cout << "carrier\t" << source.view().carriers[row.source.value].id.value;
      for (auto ref : row.uv)
        exact(output->envelope_values, ref);
      for (double value : row.cache_domain)
        std::cout << '\t' << std::hexfloat << value;
      std::cout << '\n';
    }
    const auto &assembly = output->assembly.view();
    for (const auto &row : assembly.definitions) {
      std::cout << "allowance\t" << source.view().definitions[row.definition.value].id.value;
      exact(assembly.values, row.positional);
      exact(assembly.values, row.node_source);
      std::cout << '\n';
    }
    for (const auto &row : assembly.instances) {
      std::cout << "instance\t" << row.product.value << '\t'
                << source.view().definitions[row.definition.value].id.value;
      for (double value : row.matrix)
        std::cout << '\t' << std::hexfloat << value;
      for (uint64_t i = 0; i < row.path.count; ++i)
        std::cout << '\t' << assembly.paths[row.path.first + i].value;
      std::cout << '\n';
    }
    const auto refuses = [&](const auto &resource, AdaptiveAssemblyKeys selected,
                             AdaptiveAssemblyLimits limits) {
      bool rejected = false;
      try {
        (void)prepare_adaptive_assembly(budget, source, resource, {epsilon, delta}, selected, {},
                                        limits);
      } catch (const Error &) {
        rejected = true;
      } catch (const bgfd::Failure &) {
        rejected = true;
      }
      require(rejected, "resource/key/work admission must refuse");
    };
    refuses(std::make_shared<RunResource>(uint64_t{1} << 28), keys, {});
    refuses(run, {keys.envelopes, keys.envelopes}, {});
    refuses(run, keys, {0, uint64_t{1} << 38, {}});
    refuses(run, keys, {100000000, 1, {}});
    return 0;
  } catch (const Error &error) {
    std::cerr << "FAIL\t" << (error.code == ErrorCode::unresolved ? "unresolved" : "error") << '\t'
              << error.what() << '\n';
    return 1;
  } catch (const std::exception &error) {
    std::cerr << "FAIL\t" << error.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &error) {
    std::cerr << "FAIL\t" << error.what() << '\n';
    return 1;
  }
}
