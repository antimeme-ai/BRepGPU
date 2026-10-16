#include "../native/mesher/run_preparation.hpp"
#include <iostream>
#include <stdexcept>
namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
} // namespace
int main(int argc, char **argv) {
  try {
    using namespace bgmesh;
    require(argc == 2, "expected authored original geometry source");
    const auto resource = std::make_shared<RunResource>(1ULL << 29);
    NativeScheduler scheduler(resource, {1, 4, 1ULL << 27, 1ULL << 38});
    auto job =
        std::allocate_shared<RunPreparationJob>(RunAllocator<RunPreparationJob>(resource), resource,
                                                RunPreparationInput{argv[1], .125, .0625});
    bool incomplete = false;
    try {
      static_cast<void>(job->result());
    } catch (const Error &) {
      incomplete = true;
    }
    require(incomplete, "source preparation published before complete original inventory");
    scheduler.submit(job);
    scheduler.drain();
    auto completion = scheduler.take();
    if (!completion)
      throw std::runtime_error("native preparation completion missing");
    completion->rethrow();
    require(completion->state == StepState::done, "native preparation did not finish");
    const auto result = job->result();
    require(result->geometry.view().source.view().faces.size() == 1 &&
                result->definitions.size() == 1 && result->assembly.view().instances.size() == 1,
            "whole native source/definition/usage inventory changed");
    require(result->bank.size() == 9,
            "root bank must include all original surface roots and spatial rational roots");
    bgfd::Budget b({1ULL << 23, 1ULL << 32}, resource.get());
    const auto &box = result->definitions[0];
    for (std::size_t axis = 0; axis < 3; ++axis) {
      require(import_exact(b, result->envelopes.view(box.xyz[axis * 2])).sign() == 0,
              "literal whole-source B0 lower corner");
      require(
          import_exact(b, result->envelopes.view(box.xyz[axis * 2 + 1])).compare(Rational(b, 1)) ==
              0,
          "literal original positive-weight hull B0 upper corner");
    }
    const auto &allowance = result->assembly.view().definitions[0];
    require(import_exact(b, result->assembly.view().values.view(allowance.positional))
                        .compare(Rational::from_double(b, .125)) == 0 &&
                import_exact(b, result->assembly.view().values.view(allowance.node_source))
                        .compare(Rational::from_double(b, .0625)) == 0,
            "identity usage changed exact requested local tolerances");
    require(completion->exact_work > 0 && result->times.geometry_seconds >= 0,
            "native preparation cost missing");
    const std::array<BoundRootSelection, 0> no_rational_roots{};
    const auto analytic_bank = prepare_bound_roots(b, resource, result->geometry, no_rational_roots,
                                                   {{OwnerKind::run, UINT32_MAX, 19}, 0, 0});
    require(analytic_bank.size() == 0,
            "pure analytic source must not fabricate a resident rational root");
    std::cout << "PASS\tcomplete native source, source envelope, assembly allowance and shared "
                 "root preparation\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
