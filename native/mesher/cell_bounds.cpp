#include "cell_bounds.hpp"
#include "geometry_enclosure.hpp"
#include <algorithm>

namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
[[noreturn]] void internal(const char *message) { throw Error(ErrorCode::internal_error, message); }
std::shared_ptr<RunResource> checked_resource(const std::shared_ptr<RunResource> &resource) {
  if (!resource)
    internal("null actual cell fold resource");
  return resource;
}
void owner(const Rational &value, bgfd::Budget &budget) {
  if (&value.budget() != &budget)
    internal("actual cell arithmetic owner mismatch");
}
Rational orient(std::span<const Rational, 6> uv) {
  return uv[2]
      .subtract(uv[0])
      .multiply(uv[5].subtract(uv[1]))
      .subtract(uv[3].subtract(uv[1]).multiply(uv[4].subtract(uv[0])));
}
std::array<Rational, 4> validate(bgfd::Budget &budget, ActualTriangleView view) {
  if (view.map_kind != CellMapKind::uv_triangle)
    throw Error(ErrorCode::unsupported, "actual cell map requires its specialized source consumer");
  if (view.header.key.ordinal != 0 || view.header.bound_revision != 1)
    invalid("invalid actual cell parent/revision");
  for (const auto &x : view.original_uv)
    owner(x, budget);
  for (const auto &x : view.actual_xyz)
    owner(x, budget);
  owner(view.allowance, budget);
  if (view.allowance.sign() < 0 || orient(view.original_uv).sign() <= 0)
    invalid("actual cell requires positive UV roles and nonnegative allowance");
  std::array<Rational, 4> box{view.original_uv[0], view.original_uv[0], view.original_uv[1],
                              view.original_uv[1]};
  for (std::size_t i = 1; i < 3; ++i)
    for (std::size_t axis = 0; axis < 2; ++axis) {
      const auto &x = view.original_uv[2 * i + axis];
      if (x.compare(box[2 * axis]) < 0)
        box[2 * axis] = x;
      if (x.compare(box[2 * axis + 1]) > 0)
        box[2 * axis + 1] = x;
    }
  return box;
}
struct ClipVertex final {
  std::array<Rational, 2> uv;
  std::array<Rational, 3> barycentric;
};
bool equal(const ClipVertex &a, const ClipVertex &b) {
  return a.uv[0].compare(b.uv[0]) == 0 && a.uv[1].compare(b.uv[1]) == 0;
}
void append(std::pmr::vector<ClipVertex> &out, ClipVertex point) {
  if (!out.empty() && equal(out.back(), point))
    return;
  if (out.size() == 7)
    internal("convex triangle/rectangle clipping vertex bound exceeded");
  out.push_back(std::move(point));
}
Rational interpolate(const Rational &a, const Rational &b, const Rational &t) {
  return a.add(t.multiply(b.subtract(a)));
}
struct ClipPlane final {
  std::size_t axis;
  const Rational &value;
  bool lower;
};
void clip(std::pmr::vector<ClipVertex> &polygon, ClipPlane plane, bgfd::Budget &budget) {
  if (polygon.empty())
    return;
  std::pmr::vector<ClipVertex> out(&budget);
  out.reserve(7);
  const auto distance = [&](const ClipVertex &p) {
    return plane.lower ? p.uv[plane.axis].subtract(plane.value)
                       : plane.value.subtract(p.uv[plane.axis]);
  };
  for (std::size_t i = 0; i < polygon.size(); ++i) {
    budget.charge(1);
    const auto &a = polygon[i], &b = polygon[(i + 1) % polygon.size()];
    const auto da = distance(a), db = distance(b);
    if (da.sign() >= 0)
      append(out, a);
    if ((da.sign() < 0 && db.sign() >= 0) || (da.sign() >= 0 && db.sign() < 0)) {
      const auto t = da.divide(da.subtract(db));
      ClipVertex crossing{{interpolate(a.uv[0], b.uv[0], t), interpolate(a.uv[1], b.uv[1], t)},
                          {interpolate(a.barycentric[0], b.barycentric[0], t),
                           interpolate(a.barycentric[1], b.barycentric[1], t),
                           interpolate(a.barycentric[2], b.barycentric[2], t)}};
      append(out, std::move(crossing));
    }
  }
  if (out.size() > 1 && equal(out.front(), out.back()))
    out.pop_back();
  polygon = std::move(out);
}
std::array<Rational, 6> corners(const ClipVertex &a, const ClipVertex &b, const ClipVertex &c) {
  return {a.uv[0], a.uv[1], b.uv[0], b.uv[1], c.uv[0], c.uv[1]};
}
Rational polygon_area(std::span<const ClipVertex> polygon, bgfd::Budget &budget) {
  Rational area(budget);
  for (std::size_t i = 1; i + 1 < polygon.size(); ++i)
    area = area.add(orient(corners(polygon[0], polygon[i], polygon[i + 1])));
  return area;
}
std::array<Rational, 3> actual(const ClipVertex &vertex, ActualTriangleView view) {
  auto &budget = view.allowance.budget();
  const auto coordinate = [&](std::size_t axis) {
    Rational out(budget);
    for (std::size_t i = 0; i < 3; ++i)
      out = out.add(vertex.barycentric[i].multiply(view.actual_xyz[3 * i + axis]));
    return out;
  };
  return {coordinate(0), coordinate(1), coordinate(2)};
}
BoundTaskHeader piece_header(BoundTaskHeader parent, uint64_t ordinal) {
  parent.key.ordinal = ordinal;
  return parent;
}
[[noreturn]] void translate(const bgfd::Failure &failure) {
  throw Error(failure.status == BGFD_RESOURCE_EXHAUSTED ? ErrorCode::resource_exhausted
              : failure.status == BGFD_INVALID_INPUT    ? ErrorCode::invalid_source
                                                        : ErrorCode::internal_error,
              std::string("actual cell: ") + failure.what());
}
} // namespace
struct PreparedCellData final {
  std::shared_ptr<RunResource> resource;
  BoundRootBank bank;
  ExactTable values;
  BoundTaskHeader parent;
  BoundBatchLimits batches;
  CellPreparationStats stats;
  std::pmr::vector<SurfaceBoundInput> pieces;
  PreparedCellData(std::shared_ptr<RunResource> run, BoundRootBank roots, ExactTable exact,
                   BoundTaskHeader header, BoundBatchLimits limits)
      : resource(std::move(run)), bank(std::move(roots)), values(std::move(exact)), parent(header),
        batches(limits), pieces(resource.get()) {}
};
const BoundTaskHeader &PreparedCellBounds::parent() const noexcept { return data_->parent; }
const ExactTable &PreparedCellBounds::values() const noexcept { return data_->values; }
std::span<const SurfaceBoundInput> PreparedCellBounds::pieces() const noexcept {
  return data_->pieces;
}
CellPreparationStats PreparedCellBounds::stats() const noexcept { return data_->stats; }
BoundBatch PreparedCellBounds::batch(CellPieceRange range) const {
  if (range.count == 0 || range.first > data_->pieces.size() ||
      range.count > data_->pieces.size() - range.first ||
      range.count > data_->batches.task_capacity)
    invalid("invalid actual cell piece range");
  std::pmr::vector<BoundInput> inputs(data_->resource.get());
  inputs.reserve(range.count);
  for (uint32_t i = 0; i < range.count; ++i)
    inputs.push_back(data_->pieces[range.first + i]);
  const std::array tables{data_->values};
  return seal_bound_batch(data_->resource, data_->bank, tables, inputs, data_->batches);
}
PreparedCellBounds prepare_actual_cell(bgfd::Budget &budget,
                                       const std::shared_ptr<RunResource> &resource,
                                       BoundRootBank bank, ActualTriangleView view,
                                       ExactTableKey key, ActualCellLimits limits) {
  try {
    if (!resource)
      internal("null actual cell resource");
    const auto box = validate(budget, view);
    const auto &geometry = bank.geometry();
    if (view.carrier.value >= geometry.view().carriers.size())
      invalid("foreign actual cell carrier");
    const auto &carrier = geometry.view().carriers[view.carrier.value];
    if (carrier.kind <= 5)
      throw Error(ErrorCode::unsupported, "analytic cell requires original jet bound");
    if (view.header.source_generation != geometry.view().table.key().generation ||
        key == geometry.view().table.key() || limits.max_pieces == 0 ||
        limits.batches.task_capacity == 0 || limits.batches.table_capacity == 0)
      invalid("invalid actual cell identity/capacity");
    // Validate the complete natural rectangle without materializing its root
    // Cartesian product. Root-axis binary searches remain the single index.
    static_cast<void>(find_surface_root(geometry, view.carrier, box[0], box[2]));
    static_cast<void>(find_surface_root(geometry, view.carrier, box[1], box[3]));
    const auto columns = intersecting_surface_roots(budget, geometry, view.carrier,
                                                    {box[0], box[1], box[2], box[2]});
    const auto v_count = carrier.v_axis.count - 1;
    if (v_count == 0)
      internal("rational cell has no original V knot intervals");
    ExactBuilder values(resource, key);
    std::pmr::vector<SurfaceBoundInput> pieces(resource.get());
    Rational area(budget);
    const Rational zero(budget), one(budget, 1);
    const auto tolerance = values.append(view.allowance);
    CellPreparationStats stats{columns.size(), 0, 0};
    uint64_t previous_column = UINT64_MAX;
    for (const auto representative : columns) {
      const auto column = (representative.value - carrier.roots.first) / v_count;
      if (column == previous_column)
        continue;
      previous_column = column;
      std::pmr::vector<ClipVertex> strip(&budget);
      strip.reserve(7);
      strip.push_back({{view.original_uv[0], view.original_uv[1]}, {one, zero, zero}});
      strip.push_back({{view.original_uv[2], view.original_uv[3]}, {zero, one, zero}});
      strip.push_back({{view.original_uv[4], view.original_uv[5]}, {zero, zero, one}});
      const auto &column_record = geometry.view().surface_roots[representative.value];
      const std::array u_bounds{import_exact(budget, geometry.exact(column_record.bounds[0])),
                                import_exact(budget, geometry.exact(column_record.bounds[1]))};
      clip(strip, {0, u_bounds[0], true}, budget);
      clip(strip, {0, u_bounds[1], false}, budget);
      const auto strip_area = polygon_area(strip, budget);
      if (strip_area.sign() < 0)
        internal("actual cell column lost orientation");
      if (strip_area.sign() == 0)
        continue;
      ++stats.positive_columns;
      Rational v_min(strip[0].uv[1]), v_max(v_min);
      for (const auto &point : strip) {
        if (point.uv[1].compare(v_min) < 0)
          v_min = point.uv[1];
        if (point.uv[1].compare(v_max) > 0)
          v_max = point.uv[1];
      }
      const auto u_mid = u_bounds[0].add(u_bounds[1]).divide(Rational(budget, 2));
      const auto rows =
          intersecting_surface_roots(budget, geometry, view.carrier, {u_mid, u_mid, v_min, v_max});
      for (const auto root : rows) {
        ++stats.root_candidates;
        std::pmr::vector<ClipVertex> polygon(strip, &budget);
        const auto &record = geometry.view().surface_roots[root.value];
        for (std::size_t bound = 2; bound < 4; ++bound) {
          const auto cut = import_exact(budget, geometry.exact(record.bounds[bound]));
          clip(polygon, {1, cut, bound % 2 == 0}, budget);
        }
        if (polygon.size() < 3)
          continue;
        const auto least =
            std::min_element(polygon.begin(), polygon.end(), [](const auto &a, const auto &b) {
              const auto x = a.uv[0].compare(b.uv[0]);
              return x < 0 || (x == 0 && a.uv[1].compare(b.uv[1]) < 0);
            });
        std::rotate(polygon.begin(), least, polygon.end());
        for (std::size_t i = 1; i + 1 < polygon.size(); ++i) {
          const auto uv = corners(polygon[0], polygon[i], polygon[i + 1]);
          const auto piece_area = orient(uv);
          if (piece_area.sign() < 0)
            internal("clipped actual cell lost positive orientation");
          if (piece_area.sign() == 0)
            continue;
          if (pieces.size() == limits.max_pieces)
            throw Error(ErrorCode::resource_exhausted, "actual cell piece cap exhausted");
          const auto a = actual(polygon[0], view), b = actual(polygon[i], view),
                     c = actual(polygon[i + 1], view);
          SurfaceBoundInput piece{
              piece_header(view.header, pieces.size()), bank.find(root), {}, {}, tolerance};
          for (std::size_t k = 0; k < 6; ++k)
            piece.original_uv[k] = values.append(uv[k]);
          for (std::size_t k = 0; k < 3; ++k) {
            piece.actual_linear_xyz[k] = values.append(a[k]);
            piece.actual_linear_xyz[3 + k] = values.append(b[k]);
            piece.actual_linear_xyz[6 + k] = values.append(c[k]);
          }
          pieces.push_back(piece);
          area = area.add(piece_area);
        }
      }
    }
    if (pieces.empty() || area.compare(orient(view.original_uv)) != 0)
      internal("actual cell clipped pieces do not cover the original positive triangle");
    auto data = std::allocate_shared<PreparedCellData>(RunAllocator<PreparedCellData>(resource),
                                                       resource, std::move(bank), values.seal(),
                                                       view.header, limits.batches);
    data->stats = stats;
    data->pieces = std::move(pieces);
    return PreparedCellBounds(std::move(data));
  } catch (const bgfd::Failure &failure) {
    translate(failure);
  } catch (const bgfd::Cancelled &) {
    throw Error(ErrorCode::cancelled, "actual cell preparation cancelled");
  }
}
CellBoundResult bound_analytic_cell(GeometryContext &context, const GeometryStore &geometry,
                                    ActualTriangleView view, GeometryBoundPrecision precision) {
  try {
    auto &budget = context.budget();
    const auto box = validate(budget, view);
    const auto &source = geometry.view().source.view();
    if (view.carrier.value >= source.carriers.size() ||
        view.header.source_generation != geometry.view().table.key().generation)
      invalid("foreign analytic actual cell source/generation");
    if (source.carriers[view.carrier.value].kind > 5)
      throw Error(ErrorCode::unsupported, "rational actual cells require direct span residuals");
    if (precision.transcendental_bits < 32 ||
        precision.transcendental_bits > context.maximum_bits() ||
        precision.norm_fractional_bits > context.maximum_bits())
      invalid("actual analytic cell precision exceeds profile");
    const auto jet = enclose_original_surface_jet(context, geometry, view.carrier, box,
                                                  precision.transcendental_bits);
    const auto norm = [&](const auto &axes) {
      return geometry_detail::enclosure_norm_upper(budget, axes, precision.norm_fractional_bits);
    };
    const auto mu = norm(jet.duu), muv = norm(jet.duv), mv = norm(jet.dvv);
    Rational departure(budget);
    for (std::size_t i = 0; i < 3; ++i) {
      const auto point = enclose_original_surface_point_jet(
          context, geometry, view.carrier, view.original_uv[2 * i], view.original_uv[2 * i + 1],
          precision.transcendental_bits);
      const auto difference = [&](std::size_t axis) {
        return geometry_detail::difference(point.position[axis],
                                           ExactEnclosure(view.actual_xyz[3 * i + axis]));
      };
      const std::array residual{difference(0), difference(1), difference(2)};
      const auto error = norm(residual);
      if (error.compare(departure) > 0)
        departure = error;
    }
    const auto du = box[1].subtract(box[0]), dv = box[3].subtract(box[2]);
    const auto curvature = mu.multiply(du.multiply(du))
                               .add(muv.multiply(du.multiply(dv)).multiply(Rational(budget, 2)))
                               .add(mv.multiply(dv.multiply(dv)))
                               .divide(Rational(budget, 8));
    return {view.header, departure.add(curvature).compare(view.allowance) <= 0
                             ? BoundDecision::accept
                             : BoundDecision::bound_fail};
  } catch (const bgfd::Failure &failure) {
    translate(failure);
  } catch (const bgfd::Cancelled &) {
    throw Error(ErrorCode::cancelled, "analytic actual cell bound cancelled");
  }
}
CellBoundFold::CellBoundFold(const std::shared_ptr<RunResource> &resource,
                             PreparedCellBounds prepared)
    : resource_(checked_resource(resource)), prepared_(std::move(prepared)),
      seen_(resource_.get()) {
  seen_.resize(prepared_.pieces().size());
}
void CellBoundFold::add(const BoundCompleted &completion) {
  const auto answers = completion.answers();
  const auto headers = completion.headers();
  if (answers.size() != headers.size() || answers.empty())
    invalid("invalid actual cell completion");
  for (std::size_t i = 0; i < answers.size(); ++i) {
    const auto &key = answers[i].key;
    auto parent = key;
    parent.ordinal = 0;
    const auto &original = prepared_.parent();
    if (parent != original.key || key.ordinal >= seen_.size() || seen_[key.ordinal] != 0 ||
        answers[i].decision == BoundDecision::unresolved || headers[i].key != key ||
        headers[i].source_generation != original.source_generation ||
        headers[i].boundary_generation != original.boundary_generation ||
        headers[i].bound_revision != original.bound_revision ||
        headers[i].request_revision != original.request_revision ||
        headers[i].face_generation != original.face_generation)
      invalid("stale/duplicate/foreign actual cell completion");
  }
  for (const auto &answer : answers) {
    seen_[answer.key.ordinal] = 1;
    ++completed_;
    failed_ = failed_ || answer.decision == BoundDecision::bound_fail;
  }
}
bool CellBoundFold::complete() const noexcept { return completed_ == seen_.size(); }
CellBoundResult CellBoundFold::result() const {
  if (!complete())
    internal("actual cell result requested before all clipped pieces completed");
  return {prepared_.parent(), failed_ ? BoundDecision::bound_fail : BoundDecision::accept};
}
} // namespace bgmesh
