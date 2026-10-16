#include "../native/mesher/bound_runtime.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace bgmesh;
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
template <class Function> void rejected(Function function, ErrorCode code) {
  bool caught = false;
  try {
    function();
  } catch (const Error &failure) {
    caught = failure.code == code;
  }
  require(caught, "bound runtime failure type disagrees");
}
constexpr BoundBatchLimits batch_limits{4, 2};
constexpr BoundRuntimeLimits runtime_limits{4, 2, 1ULL << 20, 1ULL << 30, 1ULL << 34};

BoundBatch packet(const std::shared_ptr<RunResource> &resource, BoundRootBank bank,
                  bool duplicate = false, bool foreign = false, bool wrong_root = false,
                  bool wrong_revision = false) {
  bgfd::Budget budget({1ULL << 20, 1ULL << 26}, resource.get());
  ExactBuilder values(resource, {{OwnerKind::face, 0, 9}, 3, 0});
  const auto exact = [&](int64_t value) { return values.append(Rational(budget, value)); };
  const auto zero = exact(0), one = exact(1), two = exact(2);
  const auto header = [&](uint64_t cell) {
    return BoundTaskHeader{{1, cell, 3, 0}, 7, 11, wrong_revision ? 99u : 1u};
  };
  SurfaceBoundInput surface{header(1),
                            {0},
                            {zero, zero, one, zero, zero, one},
                            {zero, zero, zero, one, zero, zero, zero, one, zero},
                            two};
  auto failing = surface;
  failing.header = header(duplicate ? 1 : 2);
  failing.tolerance = zero;
  CurveBoundInput exact_zero{
      header(3), {wrong_root ? 0u : 1u}, {zero, one}, {zero, zero, zero, one, zero, zero}, zero};
  const auto third = Rational(budget, 1).divide(Rational(budget, 3));
  const auto two_thirds = third.multiply(Rational(budget, 2));
  const auto rounded_third = Rational::from_double(budget, third.to_double());
  const auto rounded_two_thirds = Rational::from_double(budget, two_thirds.to_double());
  // Original line at rational source parameters, and actual once-rounded XYZ.
  // The exact maximum endpoint departure is the threshold: both interval
  // backends must retain this unresolved equality for the same exact fallback.
  CurveBoundInput curve{
      header(4),
      {1},
      {values.append(third), values.append(two_thirds)},
      {values.append(rounded_third), zero, zero, values.append(rounded_two_thirds), zero, zero},
      values.append(two_thirds.subtract(rounded_two_thirds))};
  if (foreign)
    ++curve.tolerance.table.generation;
  const std::array<BoundInput, 4> inputs{surface, failing, exact_zero, curve};
  const std::array tables{values.seal()};
  return seal_bound_batch(resource, std::move(bank), tables, inputs, batch_limits);
}
void qualify(const BoundCompleted &completed) {
  const auto answers = completed.answers();
  require(answers.size() == 4, "bound batch lost a task");
  require(answers[0].decision == BoundDecision::accept &&
              answers[1].decision == BoundDecision::bound_fail &&
              answers[2].decision == BoundDecision::accept &&
              answers[3].decision == BoundDecision::accept,
          "actual original geometry bound results disagree");
  for (std::size_t i = 0; i < answers.size(); ++i) {
    require(answers[i].key == BoundTaskKey{1, i + 1, 3, 0}, "bound task key changed");
    require(completed.headers()[i].boundary_generation == 11,
            "boundary generation was not retained");
  }
}
void exercise(const std::shared_ptr<RunResource> &resource, BoundRootBank bank,
              const char *module) {
  rejected([&] { static_cast<void>(packet(resource, bank, true)); }, ErrorCode::invalid_source);
  rejected([&] { static_cast<void>(packet(resource, bank, false, true)); },
           ErrorCode::invalid_source);
  rejected([&] { static_cast<void>(packet(resource, bank, false, false, true)); },
           ErrorCode::invalid_source);
  rejected([&] { static_cast<void>(packet(resource, bank, false, false, false, true)); },
           ErrorCode::invalid_source);
  const auto inputs = packet(resource, bank);
  {
    auto limits = runtime_limits;
    limits.total_work = limits.child_work + (1ULL << 20);
    BoundRuntime runtime(resource, bank, limits);
    auto filtered = runtime.filter_cpu(inputs);
    require(filtered.answers()[2].decision == BoundDecision::unresolved,
            "exact zero residual should exercise fallback");
    require(filtered.answers()[3].decision == BoundDecision::unresolved,
            "non-dyadic exact threshold should exercise fallback");
    qualify(runtime.resolve(std::move(filtered)));
    require(runtime.stats().exact_tasks >= 1, "fallback was not recorded");
    const auto used = runtime.stats().charged_work;
    require(used > 0 && used < limits.child_work, "child work cap was not settled/refunded");
    qualify(runtime.resolve(runtime.filter_cpu(inputs)));
    require(runtime.stats().charged_work > used, "repeated child work disappeared");
    runtime.close();
  }
  auto zero_work = runtime_limits;
  zero_work.total_work = 0;
  rejected(
      [&] {
        BoundRuntime runtime(resource, bank, zero_work);
        static_cast<void>(runtime.filter_cpu(inputs));
      },
      ErrorCode::resource_exhausted);
  auto tiny = runtime_limits;
  tiny.child_bytes = 1;
  rejected(
      [&] {
        BoundRuntime runtime(resource, bank, tiny);
        static_cast<void>(runtime.filter_cpu(inputs));
      },
      ErrorCode::resource_exhausted);
  {
    BoundRuntime runtime(resource, bank, runtime_limits);
    runtime.cancel();
    rejected([&] { static_cast<void>(runtime.filter_cpu(inputs)); }, ErrorCode::cancelled);
  }
  {
    std::stop_source stop;
    BoundRuntime runtime(resource, bank, runtime_limits, stop.get_token());
    stop.request_stop();
    rejected([&] { static_cast<void>(runtime.filter_cpu(inputs)); }, ErrorCode::cancelled);
  }
  if (module) {
    const auto cuda = load_bound_cuda(resource, module);
    for (int32_t device = 0; device < 2; ++device) {
      const auto baseline = resource->live();
      {
        BoundRuntime runtime(resource, bank, runtime_limits);
        runtime.open_device(cuda, {device, 1ULL << 20, 1ULL << 20});
        const auto ticket = runtime.submit(inputs);
        rejected([&] { static_cast<void>(runtime.submit(inputs)); }, ErrorCode::internal_error);
        std::optional<BoundFiltered> filtered;
        while (!filtered)
          filtered = runtime.poll(ticket);
        require(filtered->answers()[3].decision == BoundDecision::unresolved,
                "P100 non-dyadic threshold bypassed required exact fallback");
        qualify(runtime.resolve(std::move(*filtered)));
        require(runtime.stats().exact_tasks >= 1, "P100 unresolved task was not resolved exactly");
        rejected([&] { static_cast<void>(runtime.poll(ticket)); }, ErrorCode::invalid_source);
        const auto active = runtime.submit(inputs);
        static_cast<void>(active);
        runtime.cancel();
        runtime.close();
      }
      require(resource->live() == baseline, "healthy device owner leaked reservations");
    }
  }
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2 || argc == 3, "expected real source fixture [CUDA module]");
    const auto resource = std::make_shared<RunResource>(1ULL << 27);
    {
      const auto source = load_source(argv[1], resource);
      bgfd::Budget scratch({1ULL << 24, 1ULL << 32}, resource.get());
      const auto geometry =
          compile_geometry(source, resource, {{OwnerKind::geometry, 0, 0}, 7, 0}, scratch);
      const auto edge_curve = source.view().curve_index.find(CurveId{2000});
      const auto curve = find_curve_root(geometry, edge_curve, Rational(scratch));
      const std::array<BoundRootSelection, 2> roots{SurfaceRootSlot{0}, curve};
      const auto bank = prepare_bound_roots(scratch, resource, geometry, roots,
                                            {{OwnerKind::geometry, 0, 0}, 7, 1});
      exercise(resource, bank, argc == 3 ? argv[2] : nullptr);
    }
    require(resource->live() == 0, "bound runtime retained healthy owners");
    std::cout << "PASS native bound runtime real geometry and retained ownership\n";
  } catch (const std::exception &failure) {
    std::cerr << failure.what() << '\n';
    return 1;
  }
}
