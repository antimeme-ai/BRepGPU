#include "plane_cell_bounds.hpp"

namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
void owner(bgfd::Budget &budget, const Rational &value) {
  if (&value.budget() != &budget)
    throw Error(ErrorCode::internal_error, "plane cell arithmetic owner mismatch");
}
PlaneCellProfileView checked(bgfd::Budget &budget, PlaneCellProfileView view) {
  budget.charge(0);
  for (const auto &v : view.origin)
    owner(budget, v);
  for (const auto &v : view.basis)
    owner(budget, v);
  for (const auto &v : view.quadrilateral)
    owner(budget, v);
  owner(budget, view.height_upper);
  if (view.height_upper.sign() < 0)
    invalid("negative source plane height");
  return view;
}
std::array<Rational, 3> edge(PlaneCellProfileView view, std::size_t i) {
  const auto &q = view.quadrilateral;
  const auto j = (i + 1) % 4;
  const auto dx = q[2 * j].subtract(q[2 * i]), dy = q[2 * j + 1].subtract(q[2 * i + 1]);
  return {dy.negative(), dx, dy.multiply(q[2 * i]).subtract(dx.multiply(q[2 * i + 1]))};
}
Rational evaluate(const std::array<Rational, 3> &line, const Rational &x, const Rational &y) {
  return line[0].multiply(x).add(line[1].multiply(y)).add(line[2]);
}
[[noreturn]] void translated(const bgfd::Failure &failure) {
  throw Error(failure.status == BGFD_RESOURCE_EXHAUSTED ? ErrorCode::resource_exhausted
                                                        : ErrorCode::internal_error,
              failure.what());
}
} // namespace
PlaneCellProfile::PlaneCellProfile(bgfd::Budget &budget, PlaneCellProfileView source) try
    : budget_(&budget), source_(checked(budget, source)),
      edges_{edge(source_, 0), edge(source_, 1), edge(source_, 2), edge(source_, 3)} {
  const auto &q = source_.quadrilateral;
  for (std::size_t i = 0; i < 4; ++i)
    for (std::size_t j = 2; j < 4; ++j) {
      const auto k = (i + j) % 4;
      if (evaluate(edges_[i], q[2 * k], q[2 * k + 1]).sign() <= 0)
        invalid("source plane quadrilateral is not strictly positive convex");
    }
} catch (const bgfd::Failure &failure) {
  translated(failure);
} catch (const bgfd::Cancelled &) {
  throw Error(ErrorCode::cancelled, "source plane profile cancelled");
}
CellBoundResult bound_plane_cell(const PlaneCellProfile &profile, PlaneTriangleView view) {
  try {
    auto &budget = *profile.budget_;
    budget.charge(0);
    if (view.map_kind != CellMapKind::plane_xy_triangle)
      throw Error(ErrorCode::unsupported, "plane bound requires explicit physical XY cell map");
    if (view.header.bound_revision != 1 || view.header.key.ordinal != 0)
      invalid("invalid plane cell parent/policy");
    for (const auto &v : view.chart_xy)
      owner(budget, v);
    for (const auto &v : view.actual_xyz)
      owner(budget, v);
    owner(budget, view.allowance);
    if (view.allowance.sign() < 0)
      invalid("negative plane cell allowance");
    const auto &q = view.chart_xy;
    if (q[2].subtract(q[0])
            .multiply(q[5].subtract(q[1]))
            .subtract(q[3].subtract(q[1]).multiply(q[4].subtract(q[0])))
            .sign() <= 0)
      invalid("plane cell XY roles are not positive");
    for (std::size_t i = 0; i < 3; ++i)
      for (const auto &line : profile.edges_)
        if (evaluate(line, q[2 * i], q[2 * i + 1]).sign() < 0)
          invalid("plane cell leaves admitted convex source image");
    const auto &source = profile.source_;
    if (view.allowance.compare(source.height_upper) < 0)
      return {view.header, BoundDecision::bound_fail};
    const auto remaining = view.allowance.subtract(source.height_upper),
               threshold = remaining.multiply(remaining);
    for (std::size_t i = 0; i < 3; ++i) {
      Rational squared(budget);
      for (std::size_t axis = 0; axis < 3; ++axis) {
        const auto residual = source.origin[axis]
                                  .add(source.basis[2 * axis].multiply(q[2 * i]))
                                  .add(source.basis[2 * axis + 1].multiply(q[2 * i + 1]))
                                  .subtract(view.actual_xyz[3 * i + axis]);
        squared = squared.add(residual.multiply(residual));
      }
      if (squared.compare(threshold) > 0)
        return {view.header, BoundDecision::bound_fail};
    }
    return {view.header, BoundDecision::accept};
  } catch (const bgfd::Failure &failure) {
    translated(failure);
  } catch (const bgfd::Cancelled &) {
    throw Error(ErrorCode::cancelled, "plane actual cell cancelled");
  }
}
} // namespace bgmesh
