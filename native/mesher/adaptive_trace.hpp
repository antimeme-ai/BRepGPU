#ifndef BREPGPU_MESHER_ADAPTIVE_TRACE_HPP
#define BREPGPU_MESHER_ADAPTIVE_TRACE_HPP
#include "../face_domain/exact.hpp"
#include <exception>
#include <iostream>
#include <syncstream>

namespace bgmesh::adaptive::diagnostic {
// Synchronous owner-local counters only. No exact values, geometry or async lease.
struct Context final {
  uint64_t face = 0, query = 0, assignment = 0, span = 0, horizon = 0, revision = 0;
  uint64_t source_sites = 0, sites = 0, facets = 0, pending = 0, failed = 0;
  uint64_t anchors = 0, leaves = 0, cumulative_work = 0;
};
inline thread_local const Context *current = nullptr;
class Scope final {
  const Context *previous_;

public:
  explicit Scope(const Context *context) noexcept : previous_(current) { current = context; }
  ~Scope() { current = previous_; }
  Scope(const Scope &) = delete;
  Scope &operator=(const Scope &) = delete;
};
struct Record final {
  const char *event = "", *phase = "";
  uint64_t component = UINT64_MAX, n = 0, boundary = 0;
  uint64_t work = 0, peak = 0, byte_limit = 0, work_limit = 0, spent = 0;
};
inline void emit(const Record &row) noexcept {
  if (!current)
    return;
  try {
    const auto &c = *current;
    std::osyncstream out(std::cerr);
    out << "native_trace\t" << row.event << '\t' << row.phase << '\t' << c.face << '\t' << c.query
        << '\t' << c.assignment << '\t' << c.span << '\t' << c.horizon << '\t' << c.revision << '\t'
        << c.source_sites << '\t' << c.sites << '\t' << c.facets << '\t' << c.pending << '\t'
        << c.failed << '\t' << c.anchors << '\t' << c.leaves << '\t' << c.cumulative_work << '\t'
        << row.component << '\t' << row.n << '\t' << row.boundary << '\t' << row.work << '\t'
        << row.peak << '\t' << row.byte_limit << '\t' << row.work_limit << '\t' << row.spent
        << '\n';
  } catch (...) {
    // Diagnostic formatting/I/O must never replace a native result or exception.
  }
}
class BudgetScope final {
  const bgfd::Budget &budget_;
  Record row_;
  int exceptions_;
  bool paired_;

public:
  BudgetScope(const bgfd::Budget &budget, Record row, bool paired = false) noexcept
      : budget_(budget), row_(row), exceptions_(std::uncaught_exceptions()), paired_(paired) {
    if (paired_)
      emit(row_);
  }
  void phase(const char *name) noexcept { row_.phase = name; }
  ~BudgetScope() {
    const bool failed = std::uncaught_exceptions() > exceptions_;
    if (!failed && !paired_)
      return;
    row_.event = paired_ ? (failed ? "cdt_failure" : "cdt_end") : "budget_failure";
    row_.work = budget_.work();
    row_.peak = budget_.peak();
    emit(row_);
  }
  BudgetScope(const BudgetScope &) = delete;
  BudgetScope &operator=(const BudgetScope &) = delete;
};
} // namespace bgmesh::adaptive::diagnostic
#endif
