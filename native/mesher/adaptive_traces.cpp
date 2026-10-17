#include "adaptive_traces.hpp"
#include "adaptive_numeric.hpp"
#include <algorithm>
#include <bit>
#include <charconv>
#include <limits>

namespace bgmesh::adaptive {
namespace {
[[noreturn]] void invalid(const char *message) { throw Error(ErrorCode::invalid_source, message); }
const Coedge &original_use(const SourceStore &source, Slot<CoedgeTag> slot) {
  if (slot.value >= source.view().coedges.size())
    invalid("trace use outside original source");
  return source.view().coedges[slot.value];
}
} // namespace
Rational map_trace_parameter(const SourceStore &source, Slot<CoedgeTag> slot, const Rational &t) {
  auto &budget = t.budget();
  const auto &use = original_use(source, slot);
  const auto &edge = source.view().edges[source.view().edge_index.find(use.edge_id).value];
  auto a = Rational::from_double(budget, edge.parameter_range[0]);
  auto b = Rational::from_double(budget, edge.parameter_range[1]);
  const int direction = a.compare(b) <= 0 ? 1 : -1;
  if (t.compare(a) * direction < 0 || t.compare(b) * direction > 0)
    invalid("trace parameter outside original edge");
  if (use.parameter_map_kind == 1)
    return t;
  if (use.parameter_map_kind != 2)
    throw Error(ErrorCode::unsupported, "unproved original trace parameter continuation");
  const auto &data = source.view();
  if (use.parameter_map.first > data.parameter_maps.size() ||
      use.parameter_map.count > data.parameter_maps.size() - use.parameter_map.first ||
      use.parameter_map.count < 2)
    invalid("trace parameter map range invalid");
  const auto map = std::span<const Vec2>(data.parameter_maps)
                       .subspan(static_cast<std::size_t>(use.parameter_map.first),
                                static_cast<std::size_t>(use.parameter_map.count));
  if (Rational::from_double(budget, map.front()[0]).compare(a) != 0 ||
      Rational::from_double(budget, map.back()[0]).compare(b) != 0)
    invalid("trace parameter map omits original edge endpoint");
  std::size_t low = 1, high = map.size();
  while (low < high) {
    budget.charge(1);
    const auto middle = low + (high - low) / 2;
    if (Rational::from_double(budget, map[middle][0]).compare(t) * direction < 0)
      low = middle + 1;
    else
      high = middle;
  }
  if (low >= map.size())
    invalid("trace parameter map does not cover sample");
  auto x = Rational::from_double(budget, map[low - 1][0]);
  auto y = Rational::from_double(budget, map[low][0]);
  if (t.compare(x) * direction < 0 || t.compare(y) * direction > 0 || x.compare(y) * direction >= 0)
    invalid("trace parameter map interval has invalid order");
  auto p = Rational::from_double(budget, map[low - 1][1]);
  auto q = Rational::from_double(budget, map[low][1]);
  return p.add(t.subtract(x).multiply(q.subtract(p)).divide(y.subtract(x)));
}
bool trace_axis_is_periodic(const Carrier &carrier, uint32_t axis) noexcept {
  return axis == 0 ? carrier.kind >= 2 && carrier.kind <= 5 : axis == 1 && carrier.kind == 5;
}
std::optional<OriginalLineImage>
match_authoritative_line_image(const SourceStore &source, const CurveSourceCache &cache,
                               Slot<CoedgeTag> slot, const ExactTable &table,
                               std::span<const UseCurveSite> rows, bgfd::Budget &budget) {
  const auto &use = original_use(source, slot);
  if (use.reconstructed || rows.size() < 2 || use.periodic_shift[0] || use.periodic_shift[1])
    return std::nullopt;
  if (&source.view() != &cache.source().view())
    invalid("foreign original pcurve line-image cache");
  const std::array<Rational, 2> domain{import_exact(budget, table.view(rows.front().mapped_s)),
                                       import_exact(budget, table.view(rows.back().mapped_s))};
  const auto curve = source.view().curve_index.find(use.curve_id);
  auto image = original_curve_line_image(cache, curve, domain, budget);
  if (!image)
    return std::nullopt;
  for (uint32_t axis = 0; axis < 2; ++axis) {
    if (import_exact(budget, table.view(rows.front().chart_xy[axis]))
            .compare(image->endpoints[axis]) ||
        import_exact(budget, table.view(rows.back().chart_xy[axis]))
            .compare(image->endpoints[2 + axis]))
      return std::nullopt;
  }
  const int direction = image->scalars[1].compare(image->scalars[0]),
            parameter_direction = domain[1].compare(domain[0]);
  if (!direction || !parameter_direction)
    return std::nullopt;
  auto prior = image->scalars[0], prior_parameter = domain[0];
  const auto dx = image->endpoints[2].subtract(image->endpoints[0]),
             dy = image->endpoints[3].subtract(image->endpoints[1]);
  for (size_t i = 0; i < rows.size(); ++i) {
    budget.charge(1);
    const std::array<Rational, 2> point{import_exact(budget, table.view(rows[i].chart_xy[0])),
                                        import_exact(budget, table.view(rows[i].chart_xy[1]))};
    if (dx.multiply(point[1].subtract(image->endpoints[1]))
            .subtract(dy.multiply(point[0].subtract(image->endpoints[0])))
            .sign())
      return std::nullopt;
    const auto scalar =
        image->covector[0].multiply(point[0]).add(image->covector[1].multiply(point[1]));
    const auto parameter = import_exact(budget, table.view(rows[i].mapped_s));
    if ((i && scalar.compare(prior) * direction <= 0) ||
        scalar.compare(image->scalars[0]) * direction < 0 ||
        scalar.compare(image->scalars[1]) * direction > 0 ||
        (i && parameter.compare(prior_parameter) * parameter_direction <= 0))
      return std::nullopt;
    prior = scalar;
    prior_parameter = parameter;
  }
  return image;
}
namespace {
struct SourceSite final {
  UseCurveSite row;
  uint32_t spatial = 0, guide = 0, endpoint = UINT32_MAX, carrier = 0;
  Slot<CoedgeTag> use;
};
struct SourceInterval final {
  BoundaryKey key;
  std::array<uint32_t, 2> sites;
  uint32_t use = 0;
};
struct Joint final {
  std::array<uint32_t, 2> sites;
};
ExactTableKey stage_key(const TraceFaceRequest &request, uint32_t phase) {
  auto key = request.table_key;
  if (phase > UINT32_MAX - key.ordinal)
    throw Error(ErrorCode::resource_exhausted, "trace exact table ordinal exhausted");
  key.ordinal += phase;
  return key;
}
template <class T, class... Args>
std::shared_ptr<T> retained(const std::shared_ptr<RunResource> &resource, Args &&...args) {
  return std::allocate_shared<T>(RunAllocator<T>(resource), std::forward<Args>(args)...);
}
void require(bool condition, const char *message) {
  if (!condition)
    invalid(message);
}
void check_ready(NumericStatus status) {
  if (status == NumericStatus::invalid)
    invalid("invalid source trace numerical result");
  if (status != NumericStatus::ready)
    throw Error(ErrorCode::unresolved, "original trace sample requires wider source enclosure");
}
void check_intervals(const Interval *point, uint32_t dimension) {
  for (uint32_t axis = 0; axis < dimension; ++axis)
    require(numeric::valid(point[axis]), "invalid original trace sample enclosure");
}
Rational copy_ref(ExactBuilder &builder, const ExactTable &table, ExactRef ref,
                  bgfd::Budget &budget, ExactRef &output) {
  auto value = import_exact(budget, table.view(ref));
  output = builder.append(value);
  return value;
}
SourceSite copy_site(const SourceSite &source, const ExactTable &table, ExactBuilder &builder,
                     bgfd::Budget &budget, bool with_bound) {
  auto site = source;
  static_cast<void>(copy_ref(builder, table, source.row.original_t, budget, site.row.original_t));
  static_cast<void>(copy_ref(builder, table, source.row.mapped_s, budget, site.row.mapped_s));
  if (with_bound) {
    for (uint32_t axis = 0; axis < 2; ++axis) {
      static_cast<void>(copy_ref(builder, table, source.row.chosen_original_uv[axis], budget,
                                 site.row.chosen_original_uv[axis]));
      site.row.chart_xy[axis] = site.row.chosen_original_uv[axis];
    }
  }
  return site;
}
void validate_request(const TraceFaceRequest &r, bgfd::Budget &budget) {
  require(r.resource && r.source.view().resource == r.resource && r.curves &&
              &r.curves->source().view() == &r.source.view() &&
              &r.carriers.source().view() == &r.source.view() && r.boundary &&
              &r.boundary->source.view() == &r.source.view() &&
              r.boundary->resource == r.resource && r.allowances.resource() == r.resource,
          "trace request source/resource mismatch");
  require(r.identity.query > 0 && r.identity.face_generation > 0 &&
              r.face.value < r.source.view().faces.size(),
          "trace request identity invalid");
  const auto &source = r.source.view();
  require(r.boundary->definition.value < source.definitions.size() &&
              source.faces[r.face.value].definition_id ==
                  source.definitions[r.boundary->definition.value].id,
          "trace face outside requested physical definition");
  const bool face_table = r.table_key.owner.kind == OwnerKind::face &&
                          r.table_key.owner.definition == r.boundary->definition.value &&
                          r.table_key.owner.id == source.faces[r.face.value].id.value &&
                          r.table_key.generation == r.identity.face_generation;
  const bool session_table =
      r.table_key.owner.kind == OwnerKind::run && r.table_key.owner.definition == UINT32_MAX - 4 &&
      r.table_key.owner.id != 0 && r.table_key.generation == r.identity.query;
  require((face_table || session_table) && r.table_key.ordinal <= UINT32_MAX - 4,
          "trace exact table identity invalid");
  require(import_exact(budget, r.allowances.view(r.positional)).sign() > 0 &&
              import_exact(budget, r.allowances.view(r.node_source)).sign() > 0,
          "trace allowances must be positive");
  if (r.physical)
    require(r.physical->boundary == r.boundary, "trace physical bounds use a foreign boundary");
}
std::array<Rational, 3> original_vertex(bgfd::Budget &b, const SourceData &source, VertexId id) {
  const auto &v = source.vertices[source.vertex_index.find(id).value];
  return {Rational::from_double(b, v.xyz[0]), Rational::from_double(b, v.xyz[1]),
          Rational::from_double(b, v.xyz[2])};
}
std::array<Rational, 3> target_point(const TraceFaceRequest &request, const SourceSite &site,
                                     const ExactTable &values,
                                     std::span<const CurveSampleResult> samples, bgfd::Budget &b) {
  const auto &source = request.source.view();
  const auto &use = source.coedges[site.use.value];
  const auto &edge = source.edges[source.edge_index.find(use.edge_id).value];
  if (site.endpoint != UINT32_MAX)
    return original_vertex(b, source, edge.vertex_ids[site.endpoint]);
  static_cast<void>(values);
  const auto &point = samples[site.spatial];
  std::array<Rational, 3> result{Rational(b), Rational(b), Rational(b)};
  for (uint32_t a = 0; a < 3; ++a)
    result[a] = Rational::from_double(b, point.original_xyz[a].lower)
                    .add(Rational::from_double(b, point.original_xyz[a].upper))
                    .divide(Rational(b, 2));
  return result;
}
using UV = std::array<Rational, 2>;
using Point = std::array<Rational, 3>;
Rational dot(const Point &a, const Point &b) {
  return a[0].multiply(b[0]).add(a[1].multiply(b[1])).add(a[2].multiply(b[2]));
}
Point frame_axis(bgfd::Budget &b, const Carrier &carrier, uint32_t axis) {
  const auto offset = (axis + 1) * 3;
  return {Rational::from_double(b, carrier.frame[offset]),
          Rational::from_double(b, carrier.frame[offset + 1]),
          Rational::from_double(b, carrier.frame[offset + 2])};
}
Point cross(const Point &a, const Point &b) {
  return {a[1].multiply(b[2]).subtract(a[2].multiply(b[1])),
          a[2].multiply(b[0]).subtract(a[0].multiply(b[2])),
          a[0].multiply(b[1]).subtract(a[1].multiply(b[0]))};
}
Rational chosen(const Rational &value) {
  return Rational::from_double(value.budget(), value.to_double());
}
struct FaceLocator final {
  Point origin;
  std::optional<std::array<Point, 2>> plane_dual;
  std::optional<Point> cylinder_height;
  FaceLocator(bgfd::Budget &b, const Carrier &carrier)
      : origin{Rational::from_double(b, carrier.frame[0]),
               Rational::from_double(b, carrier.frame[1]),
               Rational::from_double(b, carrier.frame[2])} {
    if (carrier.kind != 1 && carrier.kind != 2)
      return;
    const auto x = frame_axis(b, carrier, 0), y = frame_axis(b, carrier, 1);
    if (carrier.kind == 1) {
      const auto xx = dot(x, x), xy = dot(x, y), yy = dot(y, y);
      const auto det = xx.multiply(yy).subtract(xy.multiply(xy));
      require(det.sign() > 0, "singular original plane basis");
      Point u{Rational(b), Rational(b), Rational(b)}, v{Rational(b), Rational(b), Rational(b)};
      for (uint32_t axis = 0; axis < 3; ++axis) {
        u[axis] = yy.multiply(x[axis]).subtract(xy.multiply(y[axis])).divide(det);
        v[axis] = xx.multiply(y[axis]).subtract(xy.multiply(x[axis])).divide(det);
      }
      plane_dual = std::array<Point, 2>{std::move(u), std::move(v)};
    } else {
      auto xy = cross(x, y);
      auto det = dot(xy, frame_axis(b, carrier, 2));
      if (det.sign() != 0) {
        for (auto &coordinate : xy)
          coordinate = coordinate.divide(det);
        cylinder_height = std::move(xy);
      }
    }
  }
  UV plane(const Point &target) const {
    if (!plane_dual)
      throw Error(ErrorCode::internal_error, "original plane locator not prepared");
    Point d{target[0].subtract(origin[0]), target[1].subtract(origin[1]),
            target[2].subtract(origin[2])};
    return {chosen(dot((*plane_dual)[0], d)), chosen(dot((*plane_dual)[1], d))};
  }
  Rational height(const Point &target) const {
    if (!cylinder_height)
      throw Error(ErrorCode::unsupported, "cylinder height locator has singular original frame");
    Point d{target[0].subtract(origin[0]), target[1].subtract(origin[1]),
            target[2].subtract(origin[2])};
    return chosen(dot(*cylinder_height, d));
  }
};
Interval period_lift(const Carrier &carrier, const Coedge &use, uint32_t axis) {
  if (use.periodic_shift[axis] == 0)
    return {};
  if (!trace_axis_is_periodic(carrier, axis))
    throw Error(ErrorCode::unsupported, "original use has no supported periodic axis");
  return numeric::scale(numeric::half_pi(), static_cast<double>(use.periodic_shift[axis]) * 4.);
}
UV guide_point(const Carrier &carrier, const Coedge &use, const CurveSampleResult &sample,
               std::span<const double, 4> admitted_box, bgfd::Budget &b) {
  UV result{Rational(b), Rational(b)};
  for (uint32_t a = 0; a < 2; ++a) {
    const auto interval = numeric::add(sample.original_xyz[a], period_lift(carrier, use, a));
    require(numeric::valid(interval), "original lifted pcurve is not finite");
    result[a] = chosen(Rational::from_double(b, interval.lower)
                           .add(Rational::from_double(b, interval.upper))
                           .divide(Rational(b, 2)));
    // A padded enclosure midpoint can round outside an exact chart boundary
    // even when the original guide is its endpoint. Choose that boundary only
    // when it lies IN the same original enclosure; all source guards and the
    // authoritative displacement bound still apply to the chosen point.
    for (uint32_t side = 0; side < 2; ++side) {
      const auto edge = admitted_box[2 * a + side];
      const auto relation = result[a].compare(Rational::from_double(b, edge));
      if ((side == 0 ? relation < 0 : relation > 0) && interval.lower <= edge &&
          interval.upper >= edge)
        result[a] = Rational::from_double(b, edge);
    }
  }
  return result;
}
std::array<double, 4> original_rational_rectangle(const SourceData &source,
                                                   const Carrier &carrier, bgfd::Budget &budget) {
  std::array<double, 4> box{};
  for (uint32_t axis = 0; axis < 2; ++axis) {
    const auto knots = source.view(axis == 0 ? carrier.knot_u : carrier.knot_v);
    const auto mult = source.view(axis == 0 ? carrier.mult_u : carrier.mult_v);
    uint64_t expanded = 0;
    bool first = false, last = false;
    require(knots.size() == mult.size(), "original rational knot shape differs");
    for (size_t i = 0; i < knots.size(); ++i) {
      budget.charge(1);
      require(mult[i] <= UINT64_MAX - expanded, "original rational knot size overflow");
      const auto end = expanded + mult[i];
      if (expanded <= carrier.degrees[axis] && carrier.degrees[axis] < end) {
        box[2 * axis] = knots[i];
        first = true;
      }
      if (expanded <= carrier.control_shape[axis] && carrier.control_shape[axis] < end) {
        box[2 * axis + 1] = knots[i];
        last = true;
      }
      expanded = end;
    }
    require(first && last && box[2 * axis] < box[2 * axis + 1],
            "original rational natural rectangle is empty");
  }
  return box;
}
void authoritative_joint(const TraceFaceRequest &request, const SourceSite &a, const SourceSite &c,
                         const ExactTable &values, bgfd::Budget &b) {
  const auto &source = request.source.view();
  const auto &ua = source.coedges[a.use.value], &uc = source.coedges[c.use.value];
  if (ua.reconstructed || uc.reconstructed)
    return;
  const auto sa = import_exact(b, values.view(a.row.mapped_s)),
             sc = import_exact(b, values.view(c.row.mapped_s));
  if (ua.periodic_shift != uc.periodic_shift)
    throw Error(ErrorCode::unsupported, "authoritative joint has unproved different lift identity");
  if (ua.curve_id == uc.curve_id && sa.compare(sc) == 0)
    return;
  const auto pa = exact_curve_value(*request.curves, source.curve_index.find(ua.curve_id), sa, b),
             pc = exact_curve_value(*request.curves, source.curve_index.find(uc.curve_id), sc, b);
  if (!pa || !pc)
    throw Error(ErrorCode::unsupported, "authoritative analytic joint equality is unproved");
  for (uint32_t axis = 0; axis < 2; ++axis)
    if ((*pa)[axis].compare((*pc)[axis]) != 0)
      invalid("original authoritative pcurves disagree at a loop joint");
}
double residual_upper(const Interval *a, const Interval *b) {
  Interval difference[3];
  for (uint32_t axis = 0; axis < 3; ++axis)
    difference[axis] = numeric::sub(a[axis], b[axis]);
  return numeric::norm_upper(difference);
}
} // namespace
struct TraceSourceWorkData final {
  TraceFaceRequest request;
  std::optional<ExactTable> values_;
  const ExactTable &values() const {
    if (!values_)
      throw Error(ErrorCode::internal_error, "unpublished trace exact table");
    return *values_;
  }
  std::pmr::vector<TraceCurveSampleTask> tasks;
  std::pmr::vector<SourceSite> sites;
  std::pmr::vector<UseCurveTrace> uses;
  std::pmr::vector<SourceInterval> intervals;
  std::pmr::vector<Joint> joints;
  TraceSourceWorkData(TraceFaceRequest r)
      : request(std::move(r)), tasks(request.resource.get()), sites(request.resource.get()),
        uses(request.resource.get()), intervals(request.resource.get()),
        joints(request.resource.get()) {}
};
struct TraceSiteWorkData final {
  TraceSourceWork source;
  std::optional<ExactTable> values_;
  const ExactTable &values() const {
    if (!values_)
      throw Error(ErrorCode::internal_error, "unpublished trace exact table");
    return *values_;
  }
  std::pmr::vector<SourceSite> sites;
  std::pmr::vector<TraceCarrierSampleTask> tasks;
  std::pmr::vector<CurveSampleResult> original_samples;
  std::pmr::vector<SeamWedge> wedges;
  TraceSiteWorkData(TraceSourceWork original)
      : source(std::move(original)), sites(source->request.resource.get()),
        tasks(source->request.resource.get()), original_samples(source->request.resource.get()),
        wedges(source->request.resource.get()) {}
};
struct TraceSitesData final {
  TraceSiteWork work;
  std::optional<ExactTable> values_;
  const ExactTable &values() const {
    if (!values_)
      throw Error(ErrorCode::internal_error, "unpublished trace exact table");
    return *values_;
  }
  std::pmr::vector<SourceSite> sites;
  std::pmr::vector<SampleResult> carrier_samples;
  TraceSitesData(TraceSiteWork previous)
      : work(std::move(previous)), sites(work->source->request.resource.get()),
        carrier_samples(work->source->request.resource.get()) {}
};
TraceSourceWork prepare_use_trace(const TraceFaceRequest &request, JobContext &context) {
  admit_numeric_environment();
  auto &b = context.budget;
  validate_request(request, b);
  auto work = retained<TraceSourceWorkData>(request.resource, request);
  // Build the immutable table last after original use/parameter inventory.
  ExactBuilder values(request.resource, stage_key(request, 0));
  const auto &s = request.source.view();
  const auto &face = s.faces[request.face.value];
  std::pmr::vector<std::pair<uint32_t, uint32_t>> edge_rows(&b);
  edge_rows.reserve(request.boundary->edges.size());
  for (uint32_t i = 0; i < request.boundary->edges.size(); ++i) {
    b.charge(1);
    edge_rows.emplace_back(request.boundary->edges[i].edge.value, i);
  }
  std::sort(edge_rows.begin(), edge_rows.end(), [&](auto a, auto c) {
    b.charge(1);
    return a.first < c.first;
  });
  for (uint64_t li = 0; li < face.loops.count; ++li) {
    b.charge(1);
    const auto loop_slot = s.loop_index.find(s.loop_ids[face.loops.first + li]);
    const auto &loop = s.loops[loop_slot.value];
    const auto first_use = static_cast<uint32_t>(work->uses.size());
    for (uint64_t ui = 0; ui < loop.coedges.count; ++ui) {
      b.charge(1);
      const auto use_slot = s.coedge_index.find(s.coedge_ids[loop.coedges.first + ui]);
      const auto &use = s.coedges[use_slot.value];
      const auto edge_slot = s.edge_index.find(use.edge_id);
      const auto &edge = s.edges[edge_slot.value];
      if (edge.degenerate)
        throw Error(ErrorCode::unsupported, "declared quotient needs finite cap trace profile");
      if (!edge.curve_id)
        throw Error(ErrorCode::unsupported, "ordinary trace edge has no original curve");
      const auto row = std::lower_bound(edge_rows.begin(), edge_rows.end(), edge_slot.value,
                                        [&](auto item, uint32_t key) {
                                          b.charge(1);
                                          return item.first < key;
                                        });
      require(row != edge_rows.end() && row->first == edge_slot.value,
              "missing original edge in trace physical snapshot");
      const auto &chain = request.boundary->edges[row->second];
      require(chain.nodes.first <= request.boundary->nodes.size() &&
                  chain.nodes.count <= request.boundary->nodes.size() - chain.nodes.first &&
                  chain.intervals.first <= request.boundary->intervals.size() &&
                  chain.intervals.count <=
                      request.boundary->intervals.size() - chain.intervals.first &&
                  chain.nodes.count >= 2 && chain.nodes.count == chain.intervals.count + 1,
              "invalid original physical trace chain");
      require(work->sites.size() + chain.nodes.count <= UINT32_MAX &&
                  work->intervals.size() + chain.intervals.count <= UINT32_MAX,
              "trace site/interval index overflow");
      UseCurveTrace trace;
      trace.use = use_slot;
      trace.face = request.face;
      trace.face_generation = request.identity.face_generation;
      trace.boundary_generation = request.identity.boundary_generation;
      trace.authority = use.reconstructed ? CurveTraceAuthority::reconstructed_selected
                                          : CurveTraceAuthority::authoritative_original;
      trace.sites = {work->sites.size(), chain.nodes.count};
      trace.intervals = {work->intervals.size(), chain.intervals.count};
      const auto trace_index = static_cast<uint32_t>(work->uses.size());
      for (uint64_t j = 0; j < chain.nodes.count; ++j) {
        b.charge(1);
        const auto k = use.sense > 0 ? j : chain.nodes.count - 1 - j;
        const auto node_index = static_cast<uint32_t>(chain.nodes.first + k);
        const auto &node = request.boundary->nodes[node_index];
        require(node.original_parameter.has_value(), "physical trace node lacks original t");
        auto t = import_exact(b, request.boundary->parameters.view(*node.original_parameter));
        auto mapped = map_trace_parameter(request.source, use_slot, t);
        SourceSite site;
        site.use = use_slot;
        site.row.boundary_node = node_index;
        site.row.loop = loop_slot;
        site.row.original_t = values.append(t);
        site.row.mapped_s = values.append(mapped);
        site.row.original_periodic_shift = use.periodic_shift;
        if (k == 0)
          site.endpoint = 0;
        else if (k + 1 == chain.nodes.count)
          site.endpoint = 1;
        if (j == 0)
          site.row.junction_occurrence = static_cast<uint32_t>(ui);
        else if (j + 1 == chain.nodes.count)
          site.row.junction_occurrence = static_cast<uint32_t>((ui + 1) % loop.coedges.count);
        require(work->tasks.size() <= UINT32_MAX - 2, "trace task index overflow");
        site.spatial = static_cast<uint32_t>(work->tasks.size());
        work->tasks.push_back({s.curve_index.find(edge.curve_id), site.row.original_t});
        site.guide = static_cast<uint32_t>(work->tasks.size());
        work->tasks.push_back({s.curve_index.find(use.curve_id), site.row.mapped_s});
        work->sites.push_back(site);
      }
      for (uint64_t j = 0; j < chain.intervals.count; ++j) {
        const auto k = use.sense > 0 ? j : chain.intervals.count - 1 - j;
        work->intervals.push_back({request.boundary->intervals[chain.intervals.first + k],
                                   {static_cast<uint32_t>(trace.sites.first + j),
                                    static_cast<uint32_t>(trace.sites.first + j + 1)},
                                   trace_index});
      }
      work->uses.push_back(trace);
    }
    for (uint64_t ui = 0; ui < loop.coedges.count; ++ui) {
      b.charge(1);
      const auto &out = work->uses[first_use + ui];
      const auto &in = work->uses[first_use + (ui + loop.coedges.count - 1) % loop.coedges.count];
      const auto a = static_cast<uint32_t>(out.sites.first),
                 c = static_cast<uint32_t>(in.sites.first + in.sites.count - 1);
      const auto &pa = request.boundary->nodes[work->sites[a].row.boundary_node],
                 &pc = request.boundary->nodes[work->sites[c].row.boundary_node];
      require(pa.key == pc.key, "original loop adjacency has different physical endpoints");
      for (uint32_t axis = 0; axis < 3; ++axis)
        require(std::bit_cast<uint64_t>(pa.xyz[axis]) == std::bit_cast<uint64_t>(pc.xyz[axis]),
                "shared original loop endpoint has different actual XYZ");
      work->joints.push_back({{a, c}});
    }
  }
  work->values_ = values.seal();
  return work;
}
const ExactTable &trace_values(const TraceSourceWork &work) { return work->values(); }
std::span<const TraceCurveSampleTask> trace_curve_samples(const TraceSourceWork &work) {
  return work->tasks;
}
TraceCompletionKey trace_completion_key(const TraceSourceWork &work) {
  return {work->request.identity, work->values().key()};
}
TraceSiteWork prepare_trace_sites(const TraceSourceWork &source, TraceCompletionKey completion,
                                  std::span<const CurveSampleResult> samples, JobContext &context) {
  require(source && completion == trace_completion_key(source),
          "stale original trace sample completion");
  auto &b = context.budget;
  const auto &request = source->request;
  require(samples.size() == source->tasks.size(), "original trace sample result count mismatch");
  for (uint32_t i = 0; i < samples.size(); ++i) {
    b.charge(1);
    check_ready(samples[i].status);
    check_intervals(samples[i].original_xyz,
                    request.source.view().curves[source->tasks[i].curve.value].dimension);
  }
  const auto carrier_slot = request.source.view().carrier_index.find(
      request.source.view().faces[request.face.value].carrier_id);
  const auto &carrier = request.source.view().carriers[carrier_slot.value];
  const FaceLocator locator(b, carrier);
  std::array<double, 4> admitted_box{};
  bool found_root = false;
  for (const auto &root : request.carriers.original_roots())
    if (root.source_slot == carrier_slot.value) {
      b.charge(1);
      if (!found_root) {
        for (uint32_t axis = 0; axis < 4; ++axis)
          admitted_box[axis] = root.original_box[axis].lower;
        found_root = true;
      } else
        for (uint32_t axis = 0; axis < 2; ++axis) {
          admitted_box[2 * axis] =
              std::min(admitted_box[2 * axis], root.original_box[2 * axis].lower);
          admitted_box[2 * axis + 1] =
              std::max(admitted_box[2 * axis + 1], root.original_box[2 * axis + 1].upper);
        }
    }
  require(found_root, "trace carrier has no admitted original source root");
  // The ORIGINAL natural rectangle is distinct from a requested cache window.
  // Projection proposes a reconstructed locator; the later original source
  // guard and complete selected trace bound still decide acceptance.
  const auto natural_box = carrier.kind == 6
      ? std::optional(original_rational_rectangle(request.source.view(), carrier, b))
      : std::nullopt;
  std::pmr::vector<UV> uv(&b);
  uv.reserve(source->sites.size());
  for (const auto &site : source->sites) {
    b.charge(1);
    const auto &use = request.source.view().coedges[site.use.value];
    auto point = guide_point(carrier, use, samples[site.guide], admitted_box, b);
    if (use.reconstructed && carrier.kind == 1)
      point = locator.plane(target_point(request, site, source->values(), samples, b));
    else if (use.reconstructed && carrier.kind == 2) {
      const auto &guide =
          request.source.view().curves[request.source.view().curve_index.find(use.curve_id).value];
      if (guide.kind == 1 && guide.frame[3] == 0.)
        point[1] = locator.height(target_point(request, site, source->values(), samples, b));
    }
    if (use.reconstructed && natural_box)
      for (uint32_t axis = 0; axis < 2; ++axis) {
        const auto lower = Rational::from_double(b, (*natural_box)[2 * axis]);
        const auto upper = Rational::from_double(b, (*natural_box)[2 * axis + 1]);
        if (point[axis].compare(lower) < 0)
          point[axis] = lower;
        else if (point[axis].compare(upper) > 0)
          point[axis] = upper;
      }
    uv.push_back(std::move(point));
  }
  std::pmr::vector<uint32_t> representatives(&b);
  representatives.reserve(source->sites.size());
  for (uint32_t i = 0; i < source->sites.size(); ++i)
    representatives.push_back(i);
  std::optional<SeamWedge> wedge;
  if (request.carriers.purpose() == SourceCachePurpose::quotient_trim &&
      (carrier.kind == 3 || carrier.kind == 4) && source->uses.size() == 3 &&
      request.source.view().faces[request.face.value].loops.count == 1) {
    const auto &original = request.source.view();
    for (const auto &joint : source->joints) {
      const auto out = joint.sites[0], in = joint.sites[1];
      const auto &out_site = source->sites[out], &in_site = source->sites[in];
      const auto &out_use = original.coedges[out_site.use.value];
      const auto &in_use = original.coedges[in_site.use.value];
      if (out_use.edge_id != in_use.edge_id || out_use.sense != -in_use.sense)
        continue;
      const auto &edge = original.edges[original.edge_index.find(out_use.edge_id).value];
      if (edge.degenerate || edge.vertex_ids[0] == edge.vertex_ids[1])
        continue;
      if (!out_use.reconstructed || !in_use.reconstructed)
        throw Error(ErrorCode::unsupported, "authoritative curved seam cap needs its original domain map");
      require(out_site.endpoint < 2 && out_site.endpoint == in_site.endpoint,
              "seam wedge endpoint incidence differs");
      const auto loop_slot = out_site.row.loop;
      require(loop_slot == in_site.row.loop && original.loops[loop_slot.value].outer,
              "seam wedge is not one original outer loop");
      AnalyticCapKind kind = AnalyticCapKind::cone_apex;
      if (carrier.kind == 4) {
        // This chooses an auxiliary comparison pole, not a claim that the pole
        // lies in R or that either finite source endpoint equals that pole.
        // Actual sheet/map admission and full finite-band error follow later.
        const auto sign = uv[in][1].add(uv[out][1]).sign();
        if (!sign)
          throw Error(ErrorCode::unsupported, "sphere wedge comparison-pole branch is ambiguous");
        kind = sign > 0 ? AnalyticCapKind::sphere_north : AnalyticCapKind::sphere_south;
      }
      require(!wedge, "multiple seam wedge joints are outside the finite profile");
      wedge = SeamWedge{original.vertex_index.find(edge.vertex_ids[out_site.endpoint]), loop_slot,
                         {in_site.use, out_site.use}, {in, out}, kind, {}};
    }
  }
  for (const auto &joint : source->joints) {
    if (wedge && joint.sites[0] == wedge->shared_endpoint_sites[1] &&
        joint.sites[1] == wedge->shared_endpoint_sites[0])
      continue;
    authoritative_joint(request, source->sites[joint.sites[0]], source->sites[joint.sites[1]],
                        source->values(), b);
    const auto a = joint.sites[0], c = joint.sites[1];
    const bool c_authoritative =
        !request.source.view().coedges[source->sites[c].use.value].reconstructed;
    const auto chosen_site = c_authoritative ? c : a, other = c_authoritative ? a : c;
    uv[other] = uv[chosen_site];
    representatives[other] = chosen_site;
  }
  if (wedge)
    for (uint32_t sector = 0; sector < 2; ++sector) {
      const auto use_slot = wedge->meridian_uses[sector];
      const auto hit = std::find_if(source->uses.begin(), source->uses.end(),
                                     [&](const auto &row) { return row.use == use_slot; });
      require(hit != source->uses.end(), "wedge meridian trace is missing");
      const auto first = static_cast<uint32_t>(hit->sites.first);
      const auto last = static_cast<uint32_t>(hit->sites.first + hit->sites.count - 1);
      const auto regular = wedge->shared_endpoint_sites[sector] == first ? last : first;
      const auto branch = uv[regular][0];
      int direction = 0;
      for (uint64_t j = 0; j < hit->sites.count; ++j) {
        const auto at = static_cast<uint32_t>(hit->sites.first + j);
        uv[at][0] = branch;
        if (j) {
          const auto sign = uv[at][1].compare(uv[at - 1][1]);
          if (!sign || (direction && direction != sign))
            throw Error(ErrorCode::unresolved, "chosen seam meridian V is not strictly monotone");
          direction = sign;
        }
      }
    }
  auto work = retained<TraceSiteWorkData>(request.resource, source);
  ExactBuilder builder(request.resource, stage_key(request, 1));
  work->sites.reserve(source->sites.size());
  work->original_samples.assign(samples.begin(), samples.end());
  std::pmr::vector<uint32_t> sample_ids(source->sites.size(), UINT32_MAX, &b);
  for (uint32_t i = 0; i < source->sites.size(); ++i) {
    auto site = copy_site(source->sites[i], source->values(), builder, b, false);
    for (uint32_t axis = 0; axis < 2; ++axis) {
      site.row.chosen_original_uv[axis] = builder.append(uv[i][axis]);
      site.row.chart_xy[axis] = site.row.chosen_original_uv[axis];
    }
    work->sites.push_back(site);
  }
  if (wedge) {
    for (uint32_t sector = 0; sector < 2; ++sector)
      for (uint32_t axis = 0; axis < 2; ++axis)
        wedge->finite_connector_uv[2 * sector + axis] =
            work->sites[wedge->shared_endpoint_sites[sector]].row.chosen_original_uv[axis];
    work->wedges.push_back(*wedge);
  }
  for (uint32_t i = 0; i < source->sites.size(); ++i) {
    const auto representative = representatives[i];
    if (sample_ids[representative] == UINT32_MAX) {
      sample_ids[representative] = static_cast<uint32_t>(work->tasks.size());
      work->tasks.push_back({carrier_slot, work->sites[representative].row.chosen_original_uv});
    }
    work->sites[i].carrier = sample_ids[representative];
  }
  work->values_ = builder.seal();
  return work;
}
const ExactTable &trace_values(const TraceSiteWork &work) { return work->values(); }
std::span<const TraceCarrierSampleTask> trace_carrier_samples(const TraceSiteWork &work) {
  return work->tasks;
}
TraceCompletionKey trace_completion_key(const TraceSiteWork &work) {
  return {work->source->request.identity, work->values().key()};
}
TraceSites complete_trace_sites(const TraceSiteWork &work, TraceCompletionKey completion,
                                std::span<const SampleResult> samples, JobContext &context) {
  require(work && completion == trace_completion_key(work),
          "stale carrier trace sample completion");
  require(samples.size() == work->tasks.size(), "carrier trace sample result count mismatch");
  auto &b = context.budget;
  const auto &request = work->source->request;
  for (const auto &sample : samples) {
    b.charge(1);
    check_ready(sample.status);
    check_intervals(sample.original_xyz, 3);
  }
  auto cap = import_exact(b, request.allowances.view(request.node_source));
  auto result = retained<TraceSitesData>(request.resource, work);
  ExactBuilder builder(request.resource, stage_key(request, 2));
  result->carrier_samples.assign(samples.begin(), samples.end());
  result->sites.reserve(work->sites.size());
  for (const auto &site : work->sites) {
    b.charge(1);
    const auto &source = request.source.view();
    const auto &use = source.coedges[site.use.value];
    const auto &edge = source.edges[source.edge_index.find(use.edge_id).value];
    const auto &ce = work->original_samples[site.spatial];
    const auto &surface = samples[site.carrier];
    double upper = residual_upper(ce.original_xyz, surface.original_xyz);
    if (site.endpoint != UINT32_MAX) {
      const auto vertex_slot = source.vertex_index.find(edge.vertex_ids[site.endpoint]);
      const auto &vertex = source.vertices[vertex_slot.value];
      Interval v[3];
      for (uint32_t axis = 0; axis < 3; ++axis)
        v[axis] = numeric::point(vertex.xyz[axis]);
      upper = std::max(
          {upper, residual_upper(v, ce.original_xyz), residual_upper(v, surface.original_xyz)});
      const auto &aliases = request.boundary->vertex_aliases;
      if (!aliases.empty()) {
        require(request.boundary->vertex_alias_owner &&
                    aliases.data() == request.boundary->vertex_alias_owner->rows.data() &&
                    aliases.size() == request.boundary->vertex_alias_owner->rows.size(),
                "trace alias span lacks immutable owner");
        const auto alias = std::lower_bound(aliases.begin(), aliases.end(), vertex_slot,
                                            [&](const auto &item, auto key) {
                                              b.charge(1);
                                              return item.original < key;
                                            });
        if (alias != aliases.end() && alias->original == vertex_slot) {
          require(alias->representative.value < source.vertices.size(),
                  "trace alias representative invalid");
          const auto &canonical = source.vertices[alias->representative.value];
          Interval q[3];
          for (uint32_t axis = 0; axis < 3; ++axis)
            q[axis] = numeric::point(canonical.xyz[axis]);
          upper = std::max({upper, residual_upper(q, v), residual_upper(q, ce.original_xyz),
                            residual_upper(q, surface.original_xyz)});
        }
      }
    }
    if (!numeric::finite(upper) || Rational::from_double(b, upper).compare(cap) > 0) {
      auto number = [](double value) {
        std::array<char, 64> text{};
        const auto converted = std::to_chars(text.data(), text.data() + text.size(), value,
                                             std::chars_format::scientific, 17);
        return std::string(text.data(), converted.ptr);
      };
      throw Error(ErrorCode::unresolved, "source-node enclosure does not establish cap: upper=" +
                                             number(upper) + " cap=" + number(cap.to_double()) +
                                             " original_use=" + std::to_string(use.id.value));
    }
    auto output = copy_site(site, work->values(), builder, b, true);
    output.row.node_source_upper = builder.append(Rational::from_double(b, upper));
    result->sites.push_back(output);
  }
  result->values_ = builder.seal();
  return result;
}
namespace {
struct IntervalState final {
  UseCurveInterval row;
  std::array<Interval, 2> raw_hull{};
  bool has_raw_hull = false, cylinder_identity = false, whole_line_image = false;
  uint32_t physical_index = 0;
  double direct_raw_upper = 0;
};
Rational scalar_in_plane(const Carrier &carrier, std::span<const Rational, 3> covector,
                         std::span<const Rational, 2> uv) {
  // This is the finite scalar map p.S(u,v), not another point evaluator.
  auto &b = uv[0].budget();
  Rational result(b);
  for (uint32_t axis = 0; axis < 3; ++axis) {
    auto value = Rational::from_double(b, carrier.frame[axis])
                     .add(uv[0].multiply(Rational::from_double(b, carrier.frame[3 + axis])))
                     .add(uv[1].multiply(Rational::from_double(b, carrier.frame[6 + axis])));
    result = result.add(covector[axis].multiply(value));
  }
  return result;
}
void map_piece_covers(const TraceFaceRequest &request, Slot<CoedgeTag> slot,
                      std::span<const Rational, 2> t) {
  const auto &s = request.source.view();
  const auto &use = s.coedges[slot.value];
  if (use.parameter_map_kind != 2)
    return;
  const auto &edge = s.edges[s.edge_index.find(use.edge_id).value];
  auto &b = t[0].budget();
  const auto direction = edge.parameter_range[0] < edge.parameter_range[1] ? 1 : -1;
  auto middle = t[0].add(t[1]).divide(Rational(b, 2));
  auto map = std::span<const Vec2>(s.parameter_maps)
                 .subspan(static_cast<size_t>(use.parameter_map.first),
                          static_cast<size_t>(use.parameter_map.count));
  size_t low = 1, high = map.size();
  while (low < high) {
    b.charge(1);
    const auto m = low + (high - low) / 2;
    if (Rational::from_double(b, map[m][0]).compare(middle) * direction < 0)
      low = m + 1;
    else
      high = m;
  }
  require(low < map.size(), "trace interval outside original parameter map");
  auto a = Rational::from_double(b, map[low - 1][0]), c = Rational::from_double(b, map[low][0]);
  for (const auto &point : t)
    require(point.compare(a) * direction >= 0 && point.compare(c) * direction <= 0,
            "physical interval crosses mandatory original map breakpoint");
}
bool checked_cylinder_identity(const TraceFaceRequest &request, Slot<CoedgeTag> slot,
                               std::span<const Rational, 2> t, std::span<const Rational, 4> uv) {
  const auto &s = request.source.view();
  const auto &use = s.coedges[slot.value];
  const auto &edge = s.edges[s.edge_index.find(use.edge_id).value];
  const auto &curve = s.curves[s.curve_index.find(edge.curve_id).value];
  const auto &carrier =
      s.carriers[s.carrier_index.find(s.faces[request.face.value].carrier_id).value];
  if (carrier.kind != 2 || (curve.kind != 2 && curve.kind != 3) || uv[0].compare(t[0]) != 0 ||
      uv[2].compare(t[1]) != 0 || uv[1].compare(uv[3]) != 0)
    return false;
  auto &b = t[0].budget();
  for (uint32_t axis = 0; axis < 3; ++axis) {
    auto center = Rational::from_double(b, carrier.frame[axis])
                      .add(uv[1].multiply(Rational::from_double(b, carrier.frame[9 + axis])));
    if (center.compare(Rational::from_double(b, curve.frame[axis])) != 0)
      return false;
    for (uint32_t a = 0; a < 2; ++a) {
      auto x = Rational::from_double(b, curve.radii[a])
                   .multiply(Rational::from_double(b, curve.frame[3 + 3 * a + axis]));
      auto y = Rational::from_double(b, carrier.radii[0])
                   .multiply(Rational::from_double(b, carrier.frame[3 + 3 * a + axis]));
      if (x.compare(y) != 0)
        return false;
    }
  }
  return true;
}
void hull_include(Interval &target, Interval value, bool initial) {
  require(numeric::valid(value), "unbounded original pcurve hull");
  if (initial)
    target = value;
  else {
    target.lower = std::min(target.lower, value.lower);
    target.upper = std::max(target.upper, value.upper);
  }
}
std::array<Interval, 2> curve_piece_hull(const CurveChordPiece &piece,
                                         std::span<const Interval> controls) {
  const auto &root = piece.bound.rational_curve;
  std::array<Interval, 2> hull{};
  if (root.family == CurveFamily::rational) {
    const auto stride = root.dimension + 1;
    require(piece.bound.input.controls.first <= controls.size() &&
                piece.bound.input.controls.count <=
                    controls.size() - piece.bound.input.controls.first,
            "pcurve hull control range invalid");
    auto net = controls.subspan(piece.bound.input.controls.first, piece.bound.input.controls.count);
    require(net.size() == (root.degree + 1) * stride, "pcurve hull control count invalid");
    for (uint32_t i = 0; i <= root.degree; ++i) {
      auto weight = net[i * stride + root.dimension];
      require(weight.lower > 0., "pcurve hull lacks positive rational weights");
      for (uint32_t axis = 0; axis < 2; ++axis)
        hull_include(hull[axis], numeric::div(net[i * stride + axis], weight), i == 0);
    }
  } else if (root.family == CurveFamily::line) {
    throw Error(ErrorCode::internal_error,
                "line pcurve support uses its original endpoint enclosures");
  } else {
    for (uint32_t axis = 0; axis < 2; ++axis) {
      const auto rx = numeric::scale(numeric::point(root.frame[3 + axis]), root.radii[0]),
                 ry = numeric::scale(numeric::point(root.frame[6 + axis]), root.radii[1]);
      auto magnitude = numeric::add(numeric::point(numeric::magnitude(rx)),
                                    numeric::point(numeric::magnitude(ry)));
      hull[axis] =
          numeric::add(numeric::point(root.frame[axis]), {-magnitude.upper, magnitude.upper});
    }
  }
  return hull;
}
struct DomainRootIndex final {
  struct Node final {
    std::array<double, 4> box;
    uint32_t first, count, left = UINT32_MAX, right = UINT32_MAX;
  };
  std::pmr::vector<OriginalRootSlot> roots;
  std::pmr::vector<Node> nodes;
  explicit DomainRootIndex(std::pmr::memory_resource *resource)
      : roots(resource), nodes(resource) {}
  void build(const SourceCache &cache, Slot<CarrierTag> carrier, bgfd::Budget &b) {
    const auto source = cache.original_roots();
    const auto begin = std::lower_bound(source.begin(), source.end(), carrier.value,
                                        [&](const auto &r, uint32_t slot) {
                                          b.charge(1);
                                          return r.source_slot < slot;
                                        });
    const auto end =
        std::upper_bound(begin, source.end(), carrier.value, [&](uint32_t slot, const auto &r) {
          b.charge(1);
          return slot < r.source_slot;
        });
    for (auto it = begin; it != end; ++it) {
      b.charge(1);
      roots.push_back({static_cast<uint32_t>(it - source.begin())});
    }
    if (!roots.empty())
      split(cache, 0, static_cast<uint32_t>(roots.size()), b);
  }
  uint32_t split(const SourceCache &cache, uint32_t first, uint32_t count, bgfd::Budget &b) {
    b.charge(1);
    const auto index = static_cast<uint32_t>(nodes.size());
    std::array<double, 4> box{};
    for (uint32_t i = 0; i < count; ++i) {
      b.charge(1);
      const auto &r = cache.original_roots()[roots[first + i].value];
      if (i == 0)
        for (uint32_t j = 0; j < 4; ++j)
          box[j] = j % 2 ? r.original_box[j].upper : r.original_box[j].lower;
      else
        for (uint32_t axis = 0; axis < 2; ++axis) {
          box[2 * axis] = std::min(box[2 * axis], r.original_box[2 * axis].lower);
          box[2 * axis + 1] = std::max(box[2 * axis + 1], r.original_box[2 * axis + 1].upper);
        }
    }
    nodes.push_back({box, first, count});
    if (count <= 8)
      return index;
    const auto axis = (box[3] - box[2] > box[1] - box[0]) ? 1U : 0U;
    const auto middle = first + count / 2;
    std::nth_element(
        roots.begin() + first, roots.begin() + middle, roots.begin() + first + count,
        [&](auto a, auto c) {
          b.charge(1);
          const auto &ra = cache.original_roots()[a.value], &rc = cache.original_roots()[c.value];
          if (ra.original_box[2 * axis].lower != rc.original_box[2 * axis].lower)
            return ra.original_box[2 * axis].lower < rc.original_box[2 * axis].lower;
          if (ra.original_box[2 * (1 - axis)].lower != rc.original_box[2 * (1 - axis)].lower)
            return ra.original_box[2 * (1 - axis)].lower < rc.original_box[2 * (1 - axis)].lower;
          return a.value < c.value;
        });
    const auto left = split(cache, first, middle - first, b),
               right = split(cache, middle, first + count - middle, b);
    nodes[index].left = left;
    nodes[index].right = right;
    return index;
  }
};
std::optional<Rational> transported_domain(const TraceFaceRequest &request,
                                           const DomainRootIndex &index, const IntervalState &state,
                                           const Rational &delta, bgfd::Budget &b) {
  if (delta.sign() == 0)
    return Rational(b);
  const auto carrier_slot = request.source.view().carrier_index.find(
      request.source.view().faces[request.face.value].carrier_id);
  const auto &carrier = request.source.view().carriers[carrier_slot.value];
  double du = 0, dv = 0;
  Rational area(b), covered(b);
  std::pmr::vector<std::pair<double, double>> linear_coverage(&b);
  bool point_covered = false, global_plane = false;
  if (!state.has_raw_hull)
    throw Error(ErrorCode::unresolved, "authoritative trace lacks original domain support");
  auto lo_u = Rational::from_double(b, state.raw_hull[0].lower),
       hi_u = Rational::from_double(b, state.raw_hull[0].upper),
       lo_v = Rational::from_double(b, state.raw_hull[1].lower),
       hi_v = Rational::from_double(b, state.raw_hull[1].upper);
  const auto width_u = hi_u.subtract(lo_u), width_v = hi_v.subtract(lo_v);
  area = width_u.multiply(width_v);
  std::pmr::vector<uint32_t> stack(&b);
  if (!index.nodes.empty())
    stack.push_back(0);
  while (!stack.empty()) {
    b.charge(1);
    const auto node = index.nodes[stack.back()];
    stack.pop_back();
    if (carrier.kind != 1 &&
        (node.box[0] > state.raw_hull[0].upper || node.box[1] < state.raw_hull[0].lower ||
         node.box[2] > state.raw_hull[1].upper || node.box[3] < state.raw_hull[1].lower))
      continue;
    if (node.left != UINT32_MAX) {
      stack.push_back(node.right);
      stack.push_back(node.left);
      continue;
    }
    for (uint32_t ri = 0; ri < node.count; ++ri) {
      b.charge(1);
      const auto root_slot = index.roots[node.first + ri];
      const auto &root = request.carriers.original_roots()[root_slot.value];
      std::array<Interval, 4> box;
      if (carrier.kind == 1) {
        for (uint32_t j = 0; j < 4; ++j)
          box[j] = root.original_box[j];
      } else {
        const auto a = std::max(lo_u.bounds().lower, root.original_box[0].lower),
                   c = std::min(hi_u.bounds().upper, root.original_box[1].upper),
                   d = std::max(lo_v.bounds().lower, root.original_box[2].lower),
                   e = std::min(hi_v.bounds().upper, root.original_box[3].upper);
        if (a > c || d > e)
          continue;
        box = {numeric::point(a), numeric::point(c), numeric::point(d), numeric::point(e)};
        if (width_u.sign() != 0 && width_v.sign() != 0)
          covered = covered.add(
              Rational::from_double(b, c)
                  .subtract(Rational::from_double(b, a))
                  .multiply(Rational::from_double(b, e).subtract(Rational::from_double(b, d))));
        else if (width_u.sign() != 0)
          linear_coverage.emplace_back(a, c);
        else if (width_v.sign() != 0)
          linear_coverage.emplace_back(d, e);
        else
          point_covered = true;
      }
      const auto bound = bound_original_carrier_derivatives(request.carriers, root_slot, box, b);
      require(numeric::finite(bound.du) && numeric::finite(bound.dv),
              "nonfinite authoritative carrier derivative bound");
      du = std::max(du, bound.du);
      dv = std::max(dv, bound.dv);
      if (carrier.kind == 1) {
        covered = area;
        global_plane = true;
        stack.clear();
        break;
      }
    }
  }
  bool complete = global_plane;
  if (!complete && width_u.sign() != 0 && width_v.sign() != 0)
    complete = covered.compare(area) == 0;
  else if (!complete && width_u.sign() == 0 && width_v.sign() == 0)
    complete = point_covered;
  else if (!complete) {
    std::sort(linear_coverage.begin(), linear_coverage.end(), [&](auto a, auto c) {
      b.charge(1);
      return a < c;
    });
    Rational length(b);
    if (!linear_coverage.empty()) {
      auto span = linear_coverage.front();
      for (size_t i = 1; i < linear_coverage.size(); ++i) {
        b.charge(1);
        if (linear_coverage[i].first <= span.second)
          span.second = std::max(span.second, linear_coverage[i].second);
        else {
          length = length.add(
              Rational::from_double(b, span.second).subtract(Rational::from_double(b, span.first)));
          span = linear_coverage[i];
        }
      }
      length = length.add(
          Rational::from_double(b, span.second).subtract(Rational::from_double(b, span.first)));
    }
    complete = length.compare(width_u.sign() != 0 ? width_u : width_v) == 0;
  }
  if (!complete)
    return std::nullopt;
  return delta.multiply(Rational::from_double(b, du).add(Rational::from_double(b, dv)));
}
} // namespace
struct TraceIntervalWorkData final {
  TraceSites sites;
  std::optional<ExactTable> values_;
  const ExactTable &values() const {
    if (!values_)
      throw Error(ErrorCode::internal_error, "unpublished trace exact table");
    return *values_;
  }
  std::pmr::vector<UseCurveSite> rows;
  std::pmr::vector<IntervalState> intervals;
  std::pmr::vector<TraceBoundTask> tasks;
  std::pmr::vector<Interval> controls;
  std::pmr::vector<std::optional<UseCurveTrace::WholeLineImage>> whole_images;
  DomainRootIndex domain_index;
  explicit TraceIntervalWorkData(TraceSites accepted)
      : sites(std::move(accepted)), rows(sites->work->source->request.resource.get()),
        intervals(sites->work->source->request.resource.get()),
        tasks(sites->work->source->request.resource.get()),
        controls(sites->work->source->request.resource.get()),
        whole_images(sites->work->source->request.resource.get()),
        domain_index(sites->work->source->request.resource.get()) {}
};
struct UseTraceResultData final {
  TraceIntervalWork work;
  ExactTable values;
  std::pmr::vector<UseCurveTrace> uses;
  std::pmr::vector<UseCurveSite> sites;
  std::pmr::vector<UseCurveInterval> intervals;
  std::pmr::vector<BoundaryKey> requests;
  std::pmr::vector<SeamWedge> wedges;
  uint64_t exact_fallbacks = 0;
  UseTraceResultData(TraceIntervalWork previous, ExactTable table)
      : work(std::move(previous)), values(std::move(table)),
        uses(work->sites->work->source->request.resource.get()),
        sites(work->sites->work->source->request.resource.get()),
        intervals(work->sites->work->source->request.resource.get()),
        requests(work->sites->work->source->request.resource.get()),
        wedges(work->sites->work->source->request.resource.get()) {}
};
TraceIntervalWork prepare_trace_intervals(const TraceSites &sites, JobContext &context) {
  require(static_cast<bool>(sites), "missing settled trace sites");
  auto &b = context.budget;
  const auto &source = sites->work->source;
  const auto &request = source->request;
  const auto &s = request.source.view();
  require(request.physical && request.physical->boundary == request.boundary,
          "trace intervals lack accepted physical chord pool");
  auto work = retained<TraceIntervalWorkData>(request.resource, sites);
  ExactBuilder builder(request.resource, stage_key(request, 3));
  for (const auto &site : sites->sites) {
    auto copied = copy_site(site, sites->values(), builder, b, true);
    static_cast<void>(copy_ref(builder, sites->values(), site.row.node_source_upper, b,
                               copied.row.node_source_upper));
    work->rows.push_back(copied.row);
  }
  std::pmr::vector<std::pair<BoundaryKey, uint32_t>> physical_index(&b);
  for (uint32_t i = 0; i < request.physical->intervals.size(); ++i) {
    b.charge(1);
    physical_index.emplace_back(request.physical->intervals[i].boundary_key, i);
  }
  std::sort(physical_index.begin(), physical_index.end(), [&](const auto &a, const auto &c) {
    b.charge(1);
    return a.first < c.first;
  });
  const auto carrier_slot = s.carrier_index.find(s.faces[request.face.value].carrier_id);
  const auto &carrier = s.carriers[carrier_slot.value];
  bool needs_domain_index = false;
  std::pmr::vector<double> whole_diameters(&b);
  for (const auto &use : source->uses) {
    std::pmr::vector<UseCurveSite> chosen(&b);
    for (uint64_t i = 0; i < use.sites.count; ++i)
      chosen.push_back(sites->sites[use.sites.first + i].row);
    auto image = match_authoritative_line_image(request.source, *request.curves, use.use,
                                                sites->values(), chosen, b);
    work->whole_images.emplace_back();
    whole_diameters.push_back(0);
    if (!image) {
      needs_domain_index =
          needs_domain_index || use.authority == CurveTraceAuthority::authoritative_original;
      continue;
    }
    UseCurveTrace::WholeLineImage record;
    record.raw_curve = s.curve_index.find(s.coedges[use.use.value].curve_id);
    record.raw_traversal =
        import_exact(b, sites->values().view(chosen.back().mapped_s))
            .compare(import_exact(b, sites->values().view(chosen.front().mapped_s)));
    for (uint32_t i = 0; i < 4; ++i)
      record.endpoints[i] = builder.append(image->endpoints[i]);
    for (uint32_t axis = 0; axis < 2; ++axis) {
      record.covector[axis] = builder.append(image->covector[axis]);
      record.scalar_endpoints[axis] = builder.append(image->scalars[axis]);
      auto width = image->endpoints[2 + axis].subtract(image->endpoints[axis]);
      if (width.sign() < 0)
        width = width.negative();
      whole_diameters.back() = std::max(whole_diameters.back(), width.bounds().upper);
    }
    work->whole_images.back() = record;
  }
  if (needs_domain_index)
    work->domain_index.build(request.carriers, carrier_slot, b);
  auto append_task = [&](uint32_t interval, TraceBoundPurpose purpose, AffineTraceBoundInput input,
                         std::span<const Interval> controls) {
    require(work->controls.size() + controls.size() <= UINT32_MAX, "trace control offset overflow");
    require(input.input.controls.first <= controls.size() &&
                input.input.controls.count <= controls.size() - input.input.controls.first,
            "trace input control range invalid");
    controls = controls.subspan(input.input.controls.first, input.input.controls.count);
    const auto base = static_cast<uint32_t>(work->controls.size());
    input.input.controls.first = base;
    input.rational_curve.controls = input.input.controls;
    work->controls.insert(work->controls.end(), controls.begin(), controls.end());
    work->tasks.push_back({interval, purpose, input});
  };
  for (uint32_t index = 0; index < source->intervals.size(); ++index) {
    b.charge(1);
    const auto &original = source->intervals[index];
    const auto &a = sites->sites[original.sites[0]], &c = sites->sites[original.sites[1]];
    const auto &use = s.coedges[a.use.value];
    std::array<Rational, 2> t{import_exact(b, sites->values().view(a.row.original_t)),
                              import_exact(b, sites->values().view(c.row.original_t))};
    map_piece_covers(request, a.use, t);
    std::array<Rational, 4> uv{import_exact(b, sites->values().view(a.row.chosen_original_uv[0])),
                               import_exact(b, sites->values().view(a.row.chosen_original_uv[1])),
                               import_exact(b, sites->values().view(c.row.chosen_original_uv[0])),
                               import_exact(b, sites->values().view(c.row.chosen_original_uv[1]))};
    std::array<Rational, 6> xyz{Rational(b), Rational(b), Rational(b),
                                Rational(b), Rational(b), Rational(b)};
    for (uint32_t side = 0; side < 2; ++side) {
      const auto &point =
          request.boundary->nodes[sites->sites[original.sites[side]].row.boundary_node];
      for (uint32_t axis = 0; axis < 3; ++axis)
        xyz[3 * side + axis] = Rational::from_double(b, point.xyz[axis]);
    }
    const auto entry = std::lower_bound(physical_index.begin(), physical_index.end(), original.key,
                                        [&](const auto &item, auto key) {
                                          b.charge(1);
                                          return item.first < key;
                                        });
    require(entry != physical_index.end() && entry->first == original.key,
            "missing accepted spatial chord bound");
    const auto &physical = request.physical->intervals[entry->second];
    const auto na = a.row.boundary_node, nc = c.row.boundary_node;
    require((physical.boundary_nodes[0] == na && physical.boundary_nodes[1] == nc) ||
                (physical.boundary_nodes[0] == nc && physical.boundary_nodes[1] == na),
            "accepted spatial bound uses different actual endpoints");
    IntervalState state;
    state.whole_line_image = work->whole_images[original.use].has_value();
    state.physical_index = entry->second;
    state.row.boundary_key = original.key;
    state.row.sites = original.sites;
    state.row.authority = use.reconstructed ? CurveTraceAuthority::reconstructed_selected
                                            : CurveTraceAuthority::authoritative_original;
    state.row.correspondence = physical.selected_correspondence;
    if (physical.selected_correspondence == CurveCorrespondence::monotone_line_image) {
      if (carrier.kind != 1 || !physical.image_map)
        throw Error(ErrorCode::unsupported, "physical image profile lacks compatible implemented "
                                            "carrier trace");
      auto map = *physical.image_map;
      std::array<Rational, 3> covector{
          import_exact(b, request.physical->values.view(map.covector[0])),
          import_exact(b, request.physical->values.view(map.covector[1])),
          import_exact(b, request.physical->values.view(map.covector[2]))};
      for (uint32_t axis = 0; axis < 3; ++axis)
        map.covector[axis] = builder.append(covector[axis]);
      for (uint32_t side = 0; side < 2; ++side) {
        static_cast<void>(copy_ref(builder, request.physical->values,
                                   physical.image_map->curve_scalar[side], b,
                                   map.curve_scalar[side]));
        const auto use_side = t[0].compare(t[1]) < 0 ? side : 1 - side;
        const auto physical_t =
            import_exact(b, request.physical->values.view(physical.image_map->original_t[side]));
        require(physical_t.compare(t[use_side]) == 0,
                "physical image map scalar/t pairing invalid");
        map.trace_scalar[side] = builder.append(scalar_in_plane(
            carrier, covector, std::span<const Rational, 2>(uv.data() + 2 * use_side, 2)));
        map.original_t[side] = builder.append(t[use_side]);
      }
      map.traversal = t[0].compare(t[1]) < 0 ? 1 : -1;
      state.row.image_map = map;
    }
    if (checked_cylinder_identity(request, a.use, t, uv)) {
      state.cylinder_identity = true;
      state.row.correspondence = CurveCorrespondence::cylinder_affine_identity;
      CylinderAffineIdentityRecord identity;
      identity.spatial = s.curve_index.find(s.edges[s.edge_index.find(use.edge_id).value].curve_id);
      identity.carrier = carrier_slot;
      identity.orientation = t[0].compare(t[1]) < 0 ? 1 : -1;
      for (uint32_t side = 0; side < 2; ++side)
        identity.original_t[side] = builder.append(t[side]);
      for (uint32_t k = 0; k < 4; ++k)
        identity.chosen_uv[k] = builder.append(uv[k]);
      identity.constant_v = builder.append(uv[1]);
      state.row.cylinder_identity = identity;
    } else {
      auto plan = prepare_affine_trace(request.carriers, {carrier_slot, uv, xyz}, b);
      for (const auto &piece : plan.pieces)
        append_task(index, TraceBoundPurpose::carrier_trace, piece.bound, plan.controls);
      require(!plan.pieces.empty(), "carrier affine trace produced no source pieces");
    }
    if (state.whole_line_image)
      state.direct_raw_upper = whole_diameters[original.use];
    if (!use.reconstructed && !state.whole_line_image) {
      const auto pcurve = s.curve_index.find(use.curve_id);
      std::array<Rational, 2> mapped{import_exact(b, sites->values().view(a.row.mapped_s)),
                                     import_exact(b, sites->values().view(c.row.mapped_s))};
      std::array<Rational, 6> raw_chord{uv[0], uv[1], Rational(b), uv[2], uv[3], Rational(b)};
      std::array<Interval, 3> translation{period_lift(carrier, use, 0),
                                          period_lift(carrier, use, 1), Interval{}};
      for (uint32_t side = 0; side < 2; ++side) {
        for (uint32_t axis = 0; axis < 2; ++axis) {
          auto original_point =
              numeric::add(sites->work->original_samples[sites->sites[original.sites[side]].guide]
                               .original_xyz[axis],
                           translation[axis]);
          const auto chosen_uv = uv[2 * side + axis].bounds();
          hull_include(state.raw_hull[axis], original_point, !state.has_raw_hull);
          hull_include(state.raw_hull[axis], {chosen_uv.lower, chosen_uv.upper}, false);
        }
        state.has_raw_hull = true;
      }
      if (mapped[0].compare(mapped[1]) == 0) {
        Interval error[3]{};
        for (uint32_t side = 0; side < 2; ++side) {
          for (uint32_t axis = 0; axis < 2; ++axis) {
            auto target =
                numeric::add(sites->work->original_samples[sites->sites[original.sites[side]].guide]
                                 .original_xyz[axis],
                             period_lift(carrier, use, axis));
            auto u = uv[2 * side + axis].bounds();
            error[axis] = numeric::sub(target, {u.lower, u.upper});
          }
          state.direct_raw_upper = std::max(state.direct_raw_upper, numeric::norm_upper(error));
        }
      } else {
        auto plan =
            prepare_curve_chord(*request.curves, {pcurve, mapped, raw_chord, translation}, b);
        require(!plan.pieces.empty(), "authoritative pcurve produced no original pieces");
        for (const auto &piece : plan.pieces) {
          // Source lines are already enclosed by the ORIGINAL endpoint
          // samples above. Bound-packet t is normalized and is not source s.
          if (piece.bound.rational_curve.family != CurveFamily::line) {
            auto hull = curve_piece_hull(piece, plan.controls);
            for (uint32_t axis = 0; axis < 2; ++axis)
              hull_include(state.raw_hull[axis],
                           numeric::add(hull[axis], period_lift(carrier, use, axis)), false);
          }
          append_task(index, TraceBoundPurpose::authoritative_uv, piece.bound, plan.controls);
        }
      }
    }
    work->intervals.push_back(state);
  }
  work->values_ = builder.seal();
  return work;
}
const ExactTable &trace_values(const TraceIntervalWork &work) { return work->values(); }
TraceCompletionKey trace_completion_key(const TraceIntervalWork &work) {
  return {work->sites->work->source->request.identity, work->values().key()};
}
std::span<const TraceBoundTask> trace_bound_tasks(const TraceIntervalWork &work) {
  return work->tasks;
}
std::span<const Interval> trace_bound_controls(const TraceIntervalWork &work) {
  return work->controls;
}
UseTraceResult complete_trace_intervals(const TraceIntervalWork &work,
                                        TraceCompletionKey completion,
                                        std::span<const CurveNodeBounds> bounds,
                                        JobContext &context) {
  require(work && completion == trace_completion_key(work), "stale trace interval completion");
  require(bounds.size() == work->tasks.size(), "trace interval bound count mismatch");
  auto &b = context.budget;
  const auto &request = work->sites->work->source->request;
  auto allowance = import_exact(b, request.allowances.view(request.positional));
  std::pmr::vector<double> carrier(work->intervals.size(), 0., &b),
      raw(work->intervals.size(), 0., &b);
  for (uint32_t i = 0; i < bounds.size(); ++i) {
    b.charge(1);
    check_ready(bounds[i].status);
    require(numeric::finite(bounds[i].parameter_upper) && bounds[i].parameter_upper >= 0.,
            "invalid trace numerical upper");
    const auto &task = work->tasks[i];
    auto &output = task.purpose == TraceBoundPurpose::carrier_trace ? carrier[task.interval]
                                                                    : raw[task.interval];
    output = std::max(output, bounds[i].parameter_upper);
  }
  ExactBuilder builder(request.resource, stage_key(request, 4));
  std::pmr::vector<UseCurveSite> rows(request.resource.get());
  for (const auto &site : work->rows) {
    auto row = site;
    static_cast<void>(copy_ref(builder, work->values(), site.original_t, b, row.original_t));
    static_cast<void>(copy_ref(builder, work->values(), site.mapped_s, b, row.mapped_s));
    static_cast<void>(
        copy_ref(builder, work->values(), site.node_source_upper, b, row.node_source_upper));
    for (uint32_t axis = 0; axis < 2; ++axis) {
      static_cast<void>(copy_ref(builder, work->values(), site.chosen_original_uv[axis], b,
                                 row.chosen_original_uv[axis]));
      row.chart_xy[axis] = row.chosen_original_uv[axis];
    }
    rows.push_back(row);
  }
  std::pmr::vector<UseCurveInterval> intervals(request.resource.get());
  std::pmr::vector<BoundaryKey> requests(request.resource.get());
  uint64_t exact_fallbacks = 0;
  for (uint32_t i = 0; i < work->intervals.size(); ++i) {
    b.charge(1);
    const auto &state = work->intervals[i];
    auto row = state.row;
    const auto *physical = &request.physical->intervals[state.physical_index];
    require(physical->boundary_key == row.boundary_key, "missing retained physical trace bound");
    auto spatial = import_exact(
        b, request.physical->values.view(physical->selected_correspondence ==
                                                 CurveCorrespondence::monotone_line_image
                                             ? physical->image_chord_upper.value()
                                             : physical->parameter_chord_upper));
    auto trace =
        state.cylinder_identity
            ? import_exact(b, request.physical->values.view(physical->parameter_chord_upper))
            : Rational::from_double(b, carrier[i]);
    auto source_to_trace = state.cylinder_identity ? Rational(b) : spatial.add(trace);
    row.carrier_trace_to_actual_chord_upper = builder.append(trace);
    row.source_to_selected_trace_upper = builder.append(source_to_trace);
    auto delta = Rational::from_double(b, std::max(raw[i], state.direct_raw_upper));
    for (uint32_t axis = 0; axis < 2; ++axis)
      row.uv_chord_component_upper[axis] = builder.append(delta);
    Rational domain(b);
    if (row.authority == CurveTraceAuthority::authoritative_original && !state.whole_line_image) {
      auto transported = transported_domain(request, work->domain_index, state, delta, b);
      if (!transported) {
        ++exact_fallbacks;
        auto precise = state;
        precise.has_raw_hull = false;
        const auto &source_interval = work->sites->work->source->intervals[i];
        const auto &original_site = work->sites->sites[source_interval.sites[0]];
        const auto &other_site = work->sites->sites[source_interval.sites[1]];
        const auto &use = request.source.view().coedges[original_site.use.value];
        std::array<Rational, 2> mapped{
            import_exact(b, work->sites->values().view(original_site.row.mapped_s)),
            import_exact(b, work->sites->values().view(other_site.row.mapped_s))};
        const auto hull = enclose_curve_hull_precise(
            *request.curves, request.source.view().curve_index.find(use.curve_id), mapped, b);
        const auto &carrier_source =
            request.source.view()
                .carriers[request.source.view()
                              .carrier_index
                              .find(request.source.view().faces[request.face.value].carrier_id)
                              .value];
        for (uint32_t axis = 0; axis < 2; ++axis) {
          precise.raw_hull[axis] = numeric::add(hull[axis], period_lift(carrier_source, use, axis));
          for (const auto *site : {&original_site, &other_site}) {
            const auto point =
                import_exact(b, work->sites->values().view(site->row.chosen_original_uv[axis]))
                    .bounds();
            hull_include(precise.raw_hull[axis], {point.lower, point.upper}, false);
          }
        }
        precise.has_raw_hull = true;
        transported = transported_domain(request, work->domain_index, precise, delta, b);
        if (!transported)
          throw Error(ErrorCode::unsupported,
                      "original authoritative pcurve support leaves prepared "
                      "carrier closure");
      }
      domain = std::move(*transported);
      row.authoritative_domain_upper = builder.append(domain);
    }
    if (row.image_map) {
      auto &map = *row.image_map;
      for (auto &ref : map.covector) {
        const auto old = ref;
        static_cast<void>(copy_ref(builder, work->values(), old, b, ref));
      }
      for (auto &ref : map.curve_scalar) {
        const auto old = ref;
        static_cast<void>(copy_ref(builder, work->values(), old, b, ref));
      }
      for (auto &ref : map.trace_scalar) {
        const auto old = ref;
        static_cast<void>(copy_ref(builder, work->values(), old, b, ref));
      }
      for (auto &ref : map.original_t) {
        const auto old = ref;
        static_cast<void>(copy_ref(builder, work->values(), old, b, ref));
      }
    }
    if (row.cylinder_identity) {
      auto &identity = *row.cylinder_identity;
      for (auto &ref : identity.original_t) {
        const auto old = ref;
        static_cast<void>(copy_ref(builder, work->values(), old, b, ref));
      }
      for (auto &ref : identity.chosen_uv) {
        const auto old = ref;
        static_cast<void>(copy_ref(builder, work->values(), old, b, ref));
      }
      const auto old = identity.constant_v;
      static_cast<void>(copy_ref(builder, work->values(), old, b, identity.constant_v));
    }
    if (spatial.compare(allowance) > 0 || trace.add(domain).compare(allowance) > 0)
      requests.push_back(row.boundary_key);
    intervals.push_back(row);
  }
  std::sort(requests.begin(), requests.end(), [&](auto a, auto c) {
    b.charge(1);
    return a < c;
  });
  requests.erase(std::unique(requests.begin(), requests.end()), requests.end());
  std::pmr::vector<UseCurveTrace> use_rows(work->sites->work->source->uses, request.resource.get());
  for (uint32_t i = 0; i < use_rows.size(); ++i)
    if (work->whole_images[i]) {
      auto record = *work->whole_images[i];
      for (auto &ref : record.endpoints) {
        const auto old = ref;
        static_cast<void>(copy_ref(builder, work->values(), old, b, ref));
      }
      for (auto &ref : record.covector) {
        const auto old = ref;
        static_cast<void>(copy_ref(builder, work->values(), old, b, ref));
      }
      for (auto &ref : record.scalar_endpoints) {
        const auto old = ref;
        static_cast<void>(copy_ref(builder, work->values(), old, b, ref));
      }
      use_rows[i].whole_line_image = record;
    }
  auto result = retained<UseTraceResultData>(request.resource, work, builder.seal());
  result->uses = std::move(use_rows);
  result->sites = std::move(rows);
  for (auto wedge : work->sites->work->wedges) {
    for (uint32_t sector = 0; sector < 2; ++sector)
      for (uint32_t axis = 0; axis < 2; ++axis)
        wedge.finite_connector_uv[2 * sector + axis] =
            result->sites[wedge.shared_endpoint_sites[sector]].chosen_original_uv[axis];
    result->wedges.push_back(wedge);
  }
  result->intervals = std::move(intervals);
  result->requests = std::move(requests);
  result->exact_fallbacks = exact_fallbacks;
  return result;
}
const ExactTable &trace_values(const UseTraceResult &result) { return result->values; }
TraceIdentity trace_identity(const UseTraceResult &result) {
  return result->work->sites->work->source->request.identity;
}
std::span<const UseCurveTrace> trace_uses(const UseTraceResult &result) { return result->uses; }
std::span<const UseCurveSite> trace_sites(const UseTraceResult &result) { return result->sites; }
std::span<const UseCurveInterval> trace_intervals(const UseTraceResult &result) {
  return result->intervals;
}
std::span<const BoundaryKey> trace_requests(const UseTraceResult &result) {
  return result->requests;
}
std::span<const SeamWedge> trace_wedges(const UseTraceResult &result) { return result->wedges; }
uint64_t trace_exact_fallbacks(const UseTraceResult &result) { return result->exact_fallbacks; }
} // namespace bgmesh::adaptive
