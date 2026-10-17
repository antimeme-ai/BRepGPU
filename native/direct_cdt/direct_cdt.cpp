#include "direct_cdt.h"
#include "core.hpp"
#include "predicates.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <limits>
#include <memory>
#include <memory_resource>
#include <span>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;
uint64_t elapsed(Clock::time_point start) {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count());
}
void set_error(bgfd_error *error, bgfd_status status, const char *message) noexcept {
  if (error == nullptr)
    return;
  error->abi = BGFD_ABI;
  error->size = sizeof(bgfd_error);
  error->status = status;
  error->reserved = 0;
  std::strncpy(error->message, message, sizeof(error->message) - 1);
  error->message[sizeof(error->message) - 1] = '\0';
}
bool valid_error(const bgfd_error *error) noexcept {
  return error == nullptr ||
         (error->abi == BGFD_ABI && error->size == sizeof(bgfd_error) && error->reserved == 0);
}
template <class Operation> bgfd_status guarded(bgfd_error *error, Operation operation) noexcept {
  if (!valid_error(error))
    return BGFD_INVALID_INPUT;
  try {
    operation();
    set_error(error, BGFD_OK, "");
    return BGFD_OK;
  } catch (const bgfd::Failure &failure) {
    set_error(error, failure.status, failure.what());
    return failure.status;
  } catch (const std::bad_alloc &) {
    set_error(error, BGFD_RESOURCE_EXHAUSTED, "native direct CDT allocation failed");
    return BGFD_RESOURCE_EXHAUSTED;
  } catch (...) {
    set_error(error, BGFD_INTERNAL_ERROR, "unexpected native direct CDT exception");
    return BGFD_INTERNAL_ERROR;
  }
}
} // namespace

struct bgcdt_context final {
  uint64_t token, seed, validation_work, validation_peak, validation_max_limbs, validation_ns;
  uint64_t prepare_ns = 0;
  uint64_t construct_ns = 0, copy_ns = 0;
  bgfd::Budget budget;
  bgcdt::Predicates predicates;
  std::pmr::vector<uint64_t> ids;
  std::pmr::vector<uint32_t> slots;
  std::pmr::vector<bgfd_loop> loops;
  std::pmr::vector<std::array<uint32_t, 3>> triangles;
  bgcdt::CoreStats core{};
  bool attempted = false, ready = false;

  bgcdt_context(const bgcdt_input &input, const bgfd_stats &validation, uint64_t validation_time)
      : token(input.domain.token), seed(input.seed), validation_work(validation.work),
        validation_peak(validation.memory_peak), validation_max_limbs(validation.max_limbs),
        validation_ns(validation_time),
        budget({.bytes = input.domain.memory_limit - sizeof(bgcdt_context),
                .work = input.domain.work_limit - validation.work}),
        predicates(budget, input.domain), ids(&budget), slots(&budget), loops(&budget),
        triangles(&budget) {
    const auto &source = input.domain;
    ids.reserve(static_cast<std::size_t>(source.node_count));
    for (uint64_t i = 0; i < source.node_count; ++i) {
      budget.charge(1);
      ids.push_back(source.nodes[i].id);
    }
    slots.assign(source.slots, source.slots + source.slot_count);
    loops.assign(source.loops, source.loops + source.loop_count);
    budget.charge(source.slot_count + source.loop_count);
  }
};

extern "C" bgfd_status bgcdt_create(const bgcdt_input *input, bgcdt_context **output,
                                    bgfd_error *error) {
  if (output != nullptr)
    *output = nullptr;
  if (!valid_error(error))
    return BGFD_INVALID_INPUT;
  if (input == nullptr || output == nullptr || input->abi != BGCDT_ABI ||
      input->size != sizeof(bgcdt_input) || input->reserved != 0 || input->domain.abi != BGFD_ABI ||
      input->domain.size != sizeof(bgfd_input) || input->domain.reserved != 0) {
    set_error(error, BGFD_INVALID_INPUT, "direct CDT input ABI/version mismatch");
    return BGFD_INVALID_INPUT;
  }
  if (input->domain.memory_limit < sizeof(bgcdt_context)) {
    set_error(error, BGFD_RESOURCE_EXHAUSTED, "direct CDT context exceeds memory budget");
    return BGFD_RESOURCE_EXHAUSTED;
  }
  const auto start = Clock::now();
  bgfd_context *raw = nullptr;
  auto status = bgfd_create(&input->domain, &raw, error);
  std::unique_ptr<bgfd_context, decltype(&bgfd_destroy)> validation(raw, bgfd_destroy);
  if (status != BGFD_OK)
    return status;
  status = bgfd_validate_domain(validation.get(), error);
  if (status != BGFD_OK)
    return status;
  bgfd_stats measured{};
  measured.abi = BGFD_ABI;
  measured.size = sizeof(measured);
  status = bgfd_get_stats(validation.get(), &measured, error);
  if (status != BGFD_OK)
    return status;
  validation.reset();
  const auto validation_time = elapsed(start);
  return guarded(error, [&] {
    if (measured.work > input->domain.work_limit)
      throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "input validation exceeds work budget");
    const auto prepare_start = Clock::now();
    auto context = std::make_unique<bgcdt_context>(*input, measured, validation_time);
    context->prepare_ns = elapsed(prepare_start);
    *output = context.release();
  });
}

extern "C" bgfd_status bgcdt_construct(bgcdt_context *context, bgfd_error *error) {
  return guarded(error, [&] {
    if (context == nullptr)
      throw bgfd::Failure(BGFD_INVALID_INPUT, "missing direct CDT context");
    if (context->ready)
      return;
    if (context->attempted)
      throw bgfd::Failure(BGFD_INVALID_INPUT, "direct CDT construction previously failed");
    context->attempted = true;
    const auto start = Clock::now();
    try {
      auto triangles = bgcdt::solve(context->predicates, context->budget, context->slots,
                                    context->loops, context->seed, context->core);
      const auto expected = context->ids.size() + 2 * (context->loops.size() - 1) - 2;
      if (triangles.size() != expected)
        throw bgfd::Failure(BGFD_INTERNAL_ERROR, "direct CDT source-domain triangle count differs");
      context->triangles = std::move(triangles);
      context->ready = true;
      context->construct_ns = elapsed(start);
    } catch (...) {
      context->triangles.clear();
      context->construct_ns = elapsed(start);
      throw;
    }
  });
}

extern "C" bgfd_status bgcdt_copy_triangles(bgcdt_context *context, uint64_t *ids,
                                            uint64_t capacity, uint64_t *required,
                                            bgfd_error *error) {
  return guarded(error, [&] {
    if (context == nullptr || required == nullptr || !context->ready)
      throw bgfd::Failure(BGFD_INVALID_INPUT, "no completed direct CDT triangles");
    if (context->triangles.size() > std::numeric_limits<uint64_t>::max() / 3)
      throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "direct CDT result size overflow");
    *required = 3 * static_cast<uint64_t>(context->triangles.size());
    if (ids == nullptr && capacity == 0)
      return;
    if (ids == nullptr || capacity < *required)
      throw bgfd::Failure(BGFD_INVALID_INPUT, "direct CDT output capacity too small");
    const auto start = Clock::now();
    uint64_t i = 0;
    for (const auto &triangle : context->triangles)
      for (uint32_t slot : triangle) {
        if (slot >= context->ids.size())
          throw bgfd::Failure(BGFD_INTERNAL_ERROR, "foreign direct CDT result slot");
        ids[i++] = context->ids[slot];
      }
    context->copy_ns += elapsed(start);
  });
}

extern "C" bgfd_status bgcdt_get_stats(const bgcdt_context *context, bgcdt_stats *output,
                                       bgfd_error *error) {
  return guarded(error, [&] {
    if (context == nullptr || output == nullptr || output->abi != BGCDT_ABI ||
        output->size != sizeof(bgcdt_stats))
      throw bgfd::Failure(BGFD_INVALID_INPUT, "direct CDT statistics ABI/version mismatch");
    const auto &core = context->core;
    const auto &predicates = context->predicates.stats();
    *output = {BGCDT_ABI,
               sizeof(bgcdt_stats),
               context->token,
               context->seed,
               context->ids.size(),
               context->triangles.size(),
               context->validation_work,
               context->validation_work + context->budget.work(),
               std::max(context->validation_peak, context->budget.peak() + sizeof(bgcdt_context)),
               std::max(context->validation_max_limbs, context->budget.max_limbs()),
               context->validation_ns,
               context->prepare_ns,
               context->construct_ns,
               context->copy_ns,
               core.merge_steps,
               core.crossed_edges,
               core.cavity_occurrences,
               core.cavity_insertions,
               core.secondary_deletions,
               core.max_cavity,
               core.quadedge_peak,
               core.triangle_peak,
               core.constraints,
               core.location_steps,
               core.hash_probes,
               core.secondary_calls,
               core.initial_triangles,
               core.void_triangles,
               core.order_ns,
               core.initial_ns,
               core.conversion_ns,
               core.recovery_ns,
               core.selection_ns,
               predicates.axis_compares,
               predicates.orientations,
               predicates.incircles,
               predicates.line_distance_compares,
               predicates.scalar_signs,
               predicates.interval_signs,
               predicates.exact_signs,
               predicates.exact_zeros,
               predicates.floating_disabled,
               predicates.enclosure_values,
               predicates.exact_lifts};
  });
}

extern "C" void bgcdt_destroy(bgcdt_context *context) { delete context; }
extern "C" uint32_t bgcdt_abi_size(uint32_t record) {
  switch (record) {
  case 0:
    return BGCDT_ABI;
  case 1:
    return sizeof(bgcdt_input);
  case 2:
    return sizeof(bgfd_error);
  case 3:
    return sizeof(bgcdt_stats);
  case 4:
    return sizeof(bgfd_node);
  case 5:
    return sizeof(bgfd_integer);
  case 6:
    return sizeof(bgfd_loop);
  case 7:
    return sizeof(bgfd_input);
  default:
    return 0;
  }
}
