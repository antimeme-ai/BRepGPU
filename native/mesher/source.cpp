#include "source.hpp"
#include "../rigid_placement.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <limits>
#include <numeric>
#include <string>
#include <type_traits>

namespace bgmesh {
SourceData::SourceData(std::shared_ptr<RunResource> owner)
    : resource(std::move(owner)), original_bytes(resource.get()), products(resource.get()),
      definitions(resource.get()), usages(resource.get()), vertices(resource.get()),
      edges(resource.get()), coedges(resource.get()), loops(resource.get()), faces(resource.get()),
      carriers(resource.get()), curves(resource.get()), styles(resource.get()),
      diagnostics(resource.get()), strings(resource.get()), scalars(resource.get()),
      multiplicities(resource.get()), source_ids(resource.get()), parameter_maps(resource.get()),
      face_ids(resource.get()), edge_ids(resource.get()), vertex_ids(resource.get()),
      coedge_ids(resource.get()), loop_ids(resource.get()), surface_spans(resource.get()),
      curve_spans(resource.get()), product_index(resource.get()), definition_index(resource.get()),
      usage_index(resource.get()), vertex_index(resource.get()), edge_index(resource.get()),
      coedge_index(resource.get()), loop_index(resource.get()), face_index(resource.get()),
      carrier_index(resource.get()), curve_index(resource.get()), style_index(resource.get()),
      definition_faces(resource.get()), definition_edges(resource.get()),
      definition_vertices(resource.get()), edge_coedges(resource.get()),
      edge_faces(resource.get()) {}

namespace {
[[noreturn]] void invalid(const std::string &message) {
  throw Error(ErrorCode::invalid_source, message);
}
void require(bool condition, const char *message) {
  if (!condition)
    invalid(message);
}
template <class T> std::span<const T> slice(const std::pmr::vector<T> &pool, Range<T> range) {
  if (range.first > pool.size() || range.count > pool.size() - range.first)
    throw Error(ErrorCode::internal_error, "source range outside sealed pool");
  return std::span<const T>(pool).subspan(static_cast<std::size_t>(range.first),
                                          static_cast<std::size_t>(range.count));
}
struct Work final {
  SourceLimits limits;
  uint64_t remaining;
  explicit Work(SourceLimits value) : limits(value), remaining(value.work) {}
  void charge(uint64_t count) {
    if (count > remaining)
      throw Error(ErrorCode::resource_exhausted, "source work limit exceeded");
    remaining -= count;
  }
  void records(uint64_t count) {
    if (count > limits.records || count > std::numeric_limits<uint32_t>::max())
      throw Error(ErrorCode::resource_exhausted, "source record count limit exceeded");
    charge(count);
  }
};
class Reader final {
  std::span<const std::byte> bytes_;
  std::size_t cursor_ = 0;
  Work &work_;

public:
  Reader(std::span<const std::byte> bytes, Work &work) : bytes_(bytes), work_(work) {}
  [[nodiscard]] uint64_t remaining() const { return bytes_.size() - cursor_; }
  [[nodiscard]] std::span<const std::byte> take(uint64_t count) {
    if (count > remaining())
      invalid("truncated source binary");
    const auto result = bytes_.subspan(cursor_, static_cast<std::size_t>(count));
    cursor_ += static_cast<std::size_t>(count);
    return result;
  }
  [[nodiscard]] uint64_t integer(uint32_t width = 8) {
    const auto bytes = take(width);
    uint64_t value = 0;
    for (uint32_t i = 0; i < width; ++i)
      value |= uint64_t{std::to_integer<uint8_t>(bytes[i])} << (8 * i);
    work_.charge(1);
    return value;
  }
  [[nodiscard]] uint32_t u32() { return static_cast<uint32_t>(integer(4)); }
  [[nodiscard]] int32_t i32() { return std::bit_cast<int32_t>(u32()); }
  [[nodiscard]] double f64() {
    const auto value = std::bit_cast<double>(integer());
    if (!std::isfinite(value))
      invalid("nonfinite source scalar");
    return value;
  }
  void zero() { require(u32() == 0, "nonzero reserved source field"); }
  [[nodiscard]] bool boolean() {
    const auto value = u32();
    require(value <= 1, "noncanonical source boolean");
    return value != 0;
  }
  template <class T> [[nodiscard]] SourceId<T> id() { return {integer()}; }
  template <std::size_t N> [[nodiscard]] std::array<double, N> numbers() {
    std::array<double, N> result{};
    for (auto &value : result)
      value = f64();
    return result;
  }
  template <std::size_t N> [[nodiscard]] std::array<uint32_t, N> integers() {
    std::array<uint32_t, N> result{};
    for (auto &value : result)
      value = u32();
    return result;
  }
  [[nodiscard]] Reader row() {
    const auto size = integer();
    return Reader(take(size), work_);
  }
  void end() const { require(remaining() == 0, "trailing source record bytes"); }
  [[nodiscard]] uint64_t count(uint64_t width) {
    const auto value = integer();
    require(value <= remaining() / width, "source count exceeds record length");
    work_.records(value);
    return value;
  }
  template <class T, class F>
  [[nodiscard]] Range<T> vector(std::pmr::vector<T> &pool, uint64_t width, F read) {
    const auto size = count(width);
    require(size <= pool.max_size() - pool.size(), "source vector size overflow");
    Range<T> range{pool.size(), size};
    // Preserve amortized growth across source rows. Reserving exactly this
    // row's end would copy the complete shared scalar pool once per curve.
    for (uint64_t i = 0; i < size; ++i)
      pool.push_back(read());
    return range;
  }
  [[nodiscard]] Range<double> doubles(std::pmr::vector<double> &pool) {
    return vector(pool, 8, [this] { return f64(); });
  }
  [[nodiscard]] Range<uint32_t> u32s(std::pmr::vector<uint32_t> &pool) {
    return vector(pool, 4, [this] { return u32(); });
  }
  template <class T> [[nodiscard]] Range<SourceId<T>> ids(std::pmr::vector<SourceId<T>> &pool) {
    return vector(pool, 8, [this] { return id<T>(); });
  }
  [[nodiscard]] Range<char> string(std::pmr::vector<char> &pool) {
    const auto size = u32();
    const auto raw = take(size);
    work_.charge(size);
    // Reject overlong encodings, surrogate codepoints and values beyond
    // Unicode.
    for (std::size_t i = 0; i < raw.size();) {
      const auto first = std::to_integer<uint8_t>(raw[i++]);
      if (first < 0x80)
        continue;
      unsigned tail = 0;
      uint32_t code = 0, minimum = 0;
      if (first >= 0xc2 && first <= 0xdf) {
        tail = 1;
        code = first & 31U;
        minimum = 0x80;
      } else if (first >= 0xe0 && first <= 0xef) {
        tail = 2;
        code = first & 15U;
        minimum = 0x800;
      } else if (first >= 0xf0 && first <= 0xf4) {
        tail = 3;
        code = first & 7U;
        minimum = 0x10000;
      } else
        invalid("invalid UTF-8 source string");
      require(tail <= raw.size() - i, "truncated UTF-8 source string");
      for (unsigned j = 0; j < tail; ++j) {
        const auto byte = std::to_integer<uint8_t>(raw[i++]);
        require((byte & 0xc0U) == 0x80U, "invalid UTF-8 continuation");
        code = (code << 6) | (byte & 63U);
      }
      require(code >= minimum && code <= 0x10ffff && !(code >= 0xd800 && code <= 0xdfff),
              "invalid UTF-8 codepoint");
    }
    Range<char> result{pool.size(), raw.size()};
    for (auto byte : raw)
      pool.push_back(static_cast<char>(std::to_integer<unsigned char>(byte)));
    return result;
  }
};
constexpr std::array<std::string_view, 13> tags{
    "META____", "PRODUCTS", "SOLIDS__", "USAGES__", "VERTICES", "EDGES___", "COEDGES_",
    "LOOPS___", "FACES___", "CARRIERS", "CURVES__", "STYLES__", "DIAGS___"};
bool equal_bytes(std::span<const std::byte> bytes, std::string_view expected) {
  return bytes.size() == expected.size() &&
         std::equal(bytes.begin(), bytes.end(), expected.begin(), [](std::byte a, char b) {
           return std::to_integer<unsigned char>(a) == static_cast<unsigned char>(b);
         });
}
void decode_row(std::size_t section, Reader &r, SourceData &d) {
  switch (section) {
  case 0:
    d.adapter_revision = r.string(d.strings);
    break;
  case 1: {
    Product v{};
    v.id = r.id<ProductTag>();
    v.definition_id = r.id<DefinitionTag>();
    v.style_id = r.id<StyleTag>();
    v.name = r.string(d.strings);
    v.reference = r.string(d.strings);
    d.products.push_back(v);
    break;
  }
  case 2: {
    Definition v{};
    v.id = r.id<DefinitionTag>();
    v.product_id = r.id<ProductTag>();
    v.faces = r.ids(d.face_ids);
    v.edges = r.ids(d.edge_ids);
    v.vertices = r.ids(d.vertex_ids);
    d.definitions.push_back(v);
    break;
  }
  case 3: {
    Usage v{};
    v.id = r.id<UsageTag>();
    v.parent_product_id = r.id<ProductTag>();
    v.child_product_id = r.id<ProductTag>();
    v.style_id = r.id<StyleTag>();
    v.name = r.string(d.strings);
    v.transform = r.numbers<12>();
    d.usages.push_back(v);
    break;
  }
  case 4: {
    Vertex v{};
    v.id = r.id<VertexTag>();
    v.xyz = r.numbers<3>();
    v.tolerance = r.f64();
    d.vertices.push_back(v);
    break;
  }
  case 5: {
    Edge v{};
    v.id = r.id<EdgeTag>();
    v.vertex_ids = {r.id<VertexTag>(), r.id<VertexTag>()};
    v.curve_id = r.id<CurveTag>();
    v.parameter_range = r.numbers<2>();
    v.curve_same_sense = r.boolean();
    v.same_parameter = r.boolean();
    v.degenerate = r.boolean();
    r.zero();
    v.tolerance = r.f64();
    d.edges.push_back(v);
    break;
  }
  case 6: {
    Coedge v{};
    v.id = r.id<CoedgeTag>();
    v.edge_id = r.id<EdgeTag>();
    v.loop_id = r.id<LoopTag>();
    v.curve_id = r.id<CurveTag>();
    v.sense = r.i32();
    v.reconstructed = r.boolean();
    v.parameter_range = r.numbers<2>();
    v.periodic_shift = {r.i32(), r.i32()};
    v.parameter_map_kind = r.u32();
    r.zero();
    const auto size = r.count(8);
    // Kind 4 rows are (t, s, sampled residual mm); every other kind is (t, s).
    const uint64_t width = v.parameter_map_kind == 4 ? 3 : 2;
    require(size % width == 0, "source parameter map vector is not whole rows");
    v.parameter_map = {d.parameter_maps.size(), size / width};
    v.parameter_residuals = {d.scalars.size(), width == 3 ? size / 3 : 0};
    for (uint64_t i = 0; i < size / width; ++i) {
      d.parameter_maps.push_back(r.numbers<2>());
      if (width == 3)
        d.scalars.push_back(r.f64());
    }
    d.coedges.push_back(v);
    break;
  }
  case 7: {
    Loop v{};
    v.id = r.id<LoopTag>();
    v.face_id = r.id<FaceTag>();
    v.outer = r.boolean();
    r.zero();
    v.coedges = r.ids(d.coedge_ids);
    d.loops.push_back(v);
    break;
  }
  case 8: {
    Face v{};
    v.id = r.id<FaceTag>();
    v.definition_id = r.id<DefinitionTag>();
    v.carrier_id = r.id<CarrierTag>();
    v.style_id = r.id<StyleTag>();
    v.orientation = r.i32();
    v.shell = r.u32();
    v.tolerance = r.f64();
    v.loops = r.ids(d.loop_ids);
    d.faces.push_back(v);
    break;
  }
  case 9: {
    Carrier v{};
    v.id = r.id<CarrierTag>();
    v.kind = r.u32();
    r.zero();
    v.frame = r.numbers<12>();
    v.radii = r.numbers<2>();
    v.uv_bounds = r.numbers<4>();
    v.degrees = r.integers<2>();
    v.control_shape = r.integers<2>();
    v.knot_u = r.doubles(d.scalars);
    v.mult_u = r.u32s(d.multiplicities);
    v.knot_v = r.doubles(d.scalars);
    v.mult_v = r.u32s(d.multiplicities);
    v.controls = r.doubles(d.scalars);
    const auto size = r.count(8);
    v.spans = {d.surface_spans.size(), size};
    for (uint64_t i = 0; i < size; ++i) {
      auto rr = r.row();
      SurfaceSpan s{};
      s.bounds = rr.numbers<4>();
      s.degrees = rr.integers<2>();
      s.controls = rr.doubles(d.scalars);
      rr.end();
      d.surface_spans.push_back(s);
    }
    d.carriers.push_back(v);
    break;
  }
  case 10: {
    Curve v{};
    v.id = r.id<CurveTag>();
    v.dimension = r.u32();
    v.kind = r.u32();
    v.frame = r.numbers<12>();
    v.radii = r.numbers<2>();
    v.parameter_range = r.numbers<2>();
    v.degree = r.u32();
    r.zero();
    v.knots = r.doubles(d.scalars);
    v.multiplicities = r.u32s(d.multiplicities);
    v.controls = r.doubles(d.scalars);
    const auto size = r.count(8);
    v.spans = {d.curve_spans.size(), size};
    for (uint64_t i = 0; i < size; ++i) {
      auto rr = r.row();
      CurveSpan s{};
      s.parameter_range = rr.numbers<2>();
      s.degree = rr.u32();
      rr.zero();
      s.controls = rr.doubles(d.scalars);
      rr.end();
      d.curve_spans.push_back(s);
    }
    d.curves.push_back(v);
    break;
  }
  case 11: {
    Style v{};
    v.id = r.id<StyleTag>();
    v.source_ids = r.vector(d.source_ids, 8, [&r] { return r.integer(); });
    v.rgba = r.numbers<4>();
    v.has_color = r.boolean();
    v.has_density = r.boolean();
    v.material_name = r.string(d.strings);
    v.density_unit = r.string(d.strings);
    v.density = r.f64();
    d.styles.push_back(v);
    break;
  }
  case 12: {
    Diagnostic v{};
    v.severity = r.u32();
    r.zero();
    v.stage = r.string(d.strings);
    v.source_ids = r.vector(d.source_ids, 8, [&r] { return r.integer(); });
    v.message = r.string(d.strings);
    d.diagnostics.push_back(v);
    break;
  }
  default:
    throw Error(ErrorCode::internal_error, "unknown source decoder section");
  }
}
template <class T, class Tag>
void index_rows(const std::pmr::vector<T> &rows, IdIndex<Tag> &index, Work &work) {
  work.records(rows.size());
  index.entries.reserve(rows.size());
  for (std::size_t i = 0; i < rows.size(); ++i) {
    require(rows[i].id.value != 0, "zero source ID");
    index.entries.emplace_back(rows[i].id, Slot<Tag>{static_cast<uint32_t>(i)});
  }
  std::sort(index.entries.begin(), index.entries.end(), [&work](const auto &a, const auto &b) {
    work.charge(1);
    return a.first < b.first;
  });
  for (std::size_t i = 1; i < index.entries.size(); ++i)
    require(index.entries[i - 1].first != index.entries[i].first, "duplicate source ID");
}
void controls(std::span<const double> values, uint32_t dimension, uint64_t count) {
  require(count <= std::numeric_limits<uint64_t>::max() / (uint64_t{dimension} + 1),
          "source control count overflow");
  require(values.size() == count * (uint64_t{dimension} + 1),
          "source homogeneous control count mismatch");
  for (std::size_t i = dimension; i < values.size(); i += static_cast<std::size_t>(dimension) + 1)
    require(values[i] > 0, "nonpositive source homogeneous weight");
}
void knots(std::span<const double> values, std::span<const uint32_t> mults, uint32_t degree,
           uint64_t count) {
  require(values.size() >= 2 && values.size() == mults.size(), "source knot vector mismatch");
  uint64_t total = 0;
  for (std::size_t i = 0; i < values.size(); ++i) {
    require(mults[i] > 0 && mults[i] <= degree + 1, "source knot multiplicity out of range");
    require(i == 0 || values[i] > values[i - 1], "source knots not strictly increasing");
    total += mults[i];
  }
  require(total == count + degree + 1, "source knot/control relation mismatch");
  // U[p] < U[n+1] is needed even for unclamped B-splines.
  uint64_t position = 0;
  double left = 0, right = 0;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (position <= degree && degree < position + mults[i])
      left = values[i];
    if (position <= count && count < position + mults[i])
      right = values[i];
    position += mults[i];
  }
  require(left < right, "empty natural source knot domain");
}
void style_reference(const SourceData &d, StyleId id) {
  if (id)
    static_cast<void>(d.style_index.find(id));
}
void make_csr(Csr &out, std::span<const std::pair<uint32_t, uint32_t>> pairs, std::size_t rows) {
  out.offsets.assign(rows + 1, 0);
  for (const auto &pair : pairs)
    ++out.offsets[static_cast<std::size_t>(pair.first) + 1];
  std::partial_sum(out.offsets.begin(), out.offsets.end(), out.offsets.begin());
  out.values.resize(pairs.size());
  std::pmr::vector<uint64_t> cursors(out.offsets, out.offsets.get_allocator().resource());
  for (const auto &pair : pairs)
    out.values[static_cast<std::size_t>(cursors[pair.first]++)] = pair.second;
}
template <class Id, class Tag>
void inventory(Csr &out, const std::pmr::vector<Id> &ids, std::span<const Range<Id>> ranges,
               const IdIndex<Tag> &index, Work &work) {
  out.offsets.push_back(0);
  std::pmr::vector<uint32_t> seen(index.entries.size(), 0, out.values.get_allocator().resource());
  uint32_t stamp = 0;
  for (auto range : ranges) {
    ++stamp;
    for (auto id : slice(ids, range)) {
      work.charge(1);
      const auto slot = index.find(id).value;
      require(seen[slot] != stamp, "duplicate source inventory ID");
      seen[slot] = stamp;
      out.values.push_back(slot);
    }
    out.offsets.push_back(out.values.size());
  }
}
void validate(SourceData &d, Work &work) {
  index_rows(d.products, d.product_index, work);
  index_rows(d.definitions, d.definition_index, work);
  index_rows(d.usages, d.usage_index, work);
  index_rows(d.vertices, d.vertex_index, work);
  index_rows(d.edges, d.edge_index, work);
  index_rows(d.coedges, d.coedge_index, work);
  index_rows(d.loops, d.loop_index, work);
  index_rows(d.faces, d.face_index, work);
  index_rows(d.carriers, d.carrier_index, work);
  index_rows(d.curves, d.curve_index, work);
  index_rows(d.styles, d.style_index, work);
  static_cast<void>(d.product_index.find(d.root_product_id));
  for (const auto &v : d.products) {
    if (v.definition_id)
      require(d.definitions[d.definition_index.find(v.definition_id).value].product_id == v.id,
              "product definition owner mismatch");
    style_reference(d, v.style_id);
  }
  for (const auto &v : d.vertices)
    require(v.tolerance >= 0, "negative source vertex tolerance");
  for (const auto &v : d.carriers) {
    require(v.kind >= 1 && v.kind <= 6, "unsupported source carrier kind");
    require(v.uv_bounds[0] < v.uv_bounds[1] && v.uv_bounds[2] < v.uv_bounds[3],
            "invalid source UV hint");
    if (v.kind == 6) {
      const auto p = v.degrees[0], q = v.degrees[1], nu = v.control_shape[0],
                 nv = v.control_shape[1];
      require(p <= 3 && q <= 3 && nu > p && nv > q, "unsupported source surface degree/shape");
      controls(d.view(v.controls), 3, uint64_t{nu} * nv);
      knots(d.view(v.knot_u), d.view(v.mult_u), p, nu);
      knots(d.view(v.knot_v), d.view(v.mult_v), q, nv);
      require(v.spans.count != 0, "missing saved source surface spans");
      for (const auto &s : slice(d.surface_spans, v.spans)) {
        require(s.degrees == v.degrees && s.bounds[0] < s.bounds[1] && s.bounds[2] < s.bounds[3],
                "invalid saved source surface span");
        controls(d.view(s.controls), 3, uint64_t{p + 1} * (q + 1));
      }
    }
  }
  for (const auto &v : d.curves) {
    require((v.dimension == 2 || v.dimension == 3) && v.kind >= 1 && v.kind <= 5,
            "unsupported source curve kind/dimension");
    if (v.kind == 4 || v.kind == 5) {
      require(v.degree <= 32, "unsupported source curve degree");
      const auto count = v.controls.count / (uint64_t{v.dimension} + 1);
      controls(d.view(v.controls), v.dimension, count);
      require(count > v.degree, "invalid source curve control shape");
      if (v.kind == 4) {
        require(count == uint64_t{v.degree} + 1 && v.parameter_range == Vec2{0, 1},
                "invalid natural Bezier source domain");
      } else
        knots(d.view(v.knots), d.view(v.multiplicities), v.degree, count);
      for (const auto &s : slice(d.curve_spans, v.spans)) {
        require(s.degree == v.degree && s.parameter_range[0] < s.parameter_range[1],
                "invalid saved source curve span");
        controls(d.view(s.controls), v.dimension, uint64_t{s.degree} + 1);
      }
    }
  }
  for (const auto &v : d.edges) {
    for (auto id : v.vertex_ids)
      static_cast<void>(d.vertex_index.find(id));
    require(v.tolerance >= 0, "negative source edge tolerance");
    if (v.curve_id) {
      const auto &curve = d.curves[d.curve_index.find(v.curve_id).value];
      require(curve.dimension == 3, "source spatial curve dimension mismatch");
      if (curve.kind == 4)
        for (auto t : v.parameter_range)
          require(t >= 0 && t <= 1, "edge outside natural Bezier domain");
    } else
      require(v.degenerate, "nondegenerate source edge without curve");
  }
  std::pmr::vector<uint32_t> use_owners(d.coedges.size(), 0, d.resource.get());
  std::pmr::vector<uint32_t> loop_owners(d.loops.size(), 0, d.resource.get());
  std::pmr::vector<uint32_t> face_owners(d.faces.size(), 0, d.resource.get());
  std::pmr::vector<Range<FaceId>> face_ranges(d.resource.get());
  std::pmr::vector<Range<EdgeId>> edge_ranges(d.resource.get());
  std::pmr::vector<Range<VertexId>> vertex_ranges(d.resource.get());
  for (const auto &v : d.definitions) {
    static_cast<void>(d.product_index.find(v.product_id));
    require(d.products[d.product_index.find(v.product_id).value].definition_id == v.id,
            "definition product reciprocal mismatch");
    face_ranges.push_back(v.faces);
    edge_ranges.push_back(v.edges);
    vertex_ranges.push_back(v.vertices);
    for (auto id : slice(d.face_ids, v.faces)) {
      const auto slot = d.face_index.find(id).value;
      require(d.faces[slot].definition_id == v.id, "definition face owner mismatch");
      ++face_owners[slot];
    }
  }
  inventory(d.definition_faces, d.face_ids, std::span<const Range<FaceId>>(face_ranges),
            d.face_index, work);
  // Shell ordinals of each definition run 0 (outer) .. n (voids) without gaps.
  for (const auto &v : d.definitions) {
    std::pmr::vector<uint8_t> shells(v.faces.count, 0, d.resource.get());
    for (auto id : slice(d.face_ids, v.faces)) {
      work.charge(1);
      const auto shell = d.faces[d.face_index.find(id).value].shell;
      require(shell < shells.size(), "source face shell ordinals are not contiguous");
      shells[shell] = 1;
    }
    std::size_t used = 0;
    while (used < shells.size() && shells[used])
      ++used;
    for (std::size_t i = used; i < shells.size(); ++i)
      require(!shells[i], "source face shell ordinals are not contiguous");
    require(v.faces.count == 0 || used != 0, "source face shell ordinals are not contiguous");
  }
  inventory(d.definition_edges, d.edge_ids, std::span<const Range<EdgeId>>(edge_ranges),
            d.edge_index, work);
  inventory(d.definition_vertices, d.vertex_ids, std::span<const Range<VertexId>>(vertex_ranges),
            d.vertex_index, work);
  for (std::size_t i = 0; i < d.faces.size(); ++i) {
    const auto &v = d.faces[i];
    require(face_owners[i] == 1, "face missing/duplicate definition owner");
    static_cast<void>(d.carrier_index.find(v.carrier_id));
    require((v.orientation == -1 || v.orientation == 1) && v.tolerance >= 0 && v.loops.count != 0,
            "invalid source face");
    style_reference(d, v.style_id);
    for (auto id : slice(d.loop_ids, v.loops)) {
      const auto slot = d.loop_index.find(id).value;
      require(d.loops[slot].face_id == v.id, "face loop owner mismatch");
      ++loop_owners[slot];
    }
  }
  for (std::size_t i = 0; i < d.loops.size(); ++i) {
    const auto &v = d.loops[i];
    require(loop_owners[i] == 1 && v.coedges.count != 0, "invalid source loop ownership");
    VertexId previous{}, first{};
    for (auto id : slice(d.coedge_ids, v.coedges)) {
      const auto slot = d.coedge_index.find(id).value;
      const auto &use = d.coedges[slot];
      require(use.loop_id == v.id, "loop coedge owner mismatch");
      ++use_owners[slot];
      const auto &edge = d.edges[d.edge_index.find(use.edge_id).value];
      require(use.sense == -1 || use.sense == 1, "invalid source coedge sense");
      const auto a = edge.vertex_ids[use.sense == 1 ? 0U : 1U];
      const auto b = edge.vertex_ids[use.sense == 1 ? 1U : 0U];
      if (first)
        require(previous == a, "source loop endpoint occurrence mismatch");
      else
        first = a;
      previous = b;
    }
    require(previous == first, "source loop is not closed by original vertex IDs");
  }
  std::pmr::vector<std::pair<uint32_t, uint32_t>> edge_uses(d.resource.get()),
      edge_faces(d.resource.get());
  for (std::size_t i = 0; i < d.coedges.size(); ++i) {
    const auto &v = d.coedges[i];
    require(use_owners[i] == 1, "coedge missing/duplicate loop owner");
    const auto e = d.edge_index.find(v.edge_id).value;
    const auto &edge = d.edges[e];
    const auto &curve = d.curves[d.curve_index.find(v.curve_id).value];
    require(curve.dimension == 2, "source pcurve dimension mismatch");
    require(v.parameter_map_kind >= 1 && v.parameter_map_kind <= 4,
            "unknown source parameter map kind");
    if (v.parameter_map_kind == 4)
      for (const auto residual : slice(d.scalars, v.parameter_residuals))
        require(std::isfinite(residual) && residual >= 0,
                "nonfinite or negative measured parameter map residual");
    require(edge.same_parameter || v.parameter_map_kind != 1,
            "SameParameter false without explicit map");
    if (curve.kind == 4)
      for (auto t : v.parameter_range)
        require(t >= 0 && t <= 1, "pcurve outside natural Bezier domain");
    if (v.parameter_map_kind != 1) {
      const auto map = slice(d.parameter_maps, v.parameter_map);
      require(map.size() >= 2, "missing source parameter map endpoints");
      const double direction = edge.parameter_range[1] >= edge.parameter_range[0] ? 1 : -1;
      for (std::size_t j = 1; j < map.size(); ++j)
        require(direction * (map[j][0] - map[j - 1][0]) > 0, "nonmonotone source edge map");
    }
    edge_uses.emplace_back(e, static_cast<uint32_t>(i));
    const auto face = d.face_index.find(d.loops[d.loop_index.find(v.loop_id).value].face_id).value;
    edge_faces.emplace_back(e, face);
  }
  std::sort(edge_faces.begin(), edge_faces.end());
  edge_faces.erase(std::unique(edge_faces.begin(), edge_faces.end()), edge_faces.end());
  make_csr(d.edge_coedges, edge_uses, d.edges.size());
  make_csr(d.edge_faces, edge_faces, d.edges.size());
  // Inventory checks use generation stamps, not a scan through every definition
  // for each use.
  std::pmr::vector<uint32_t> marked_edges(d.edges.size(), 0, d.resource.get());
  std::pmr::vector<uint32_t> marked_vertices(d.vertices.size(), 0, d.resource.get());
  for (std::size_t i = 0; i < d.definitions.size(); ++i) {
    const auto stamp = static_cast<uint32_t>(i + 1);
    for (auto e : d.definition_edges.at(static_cast<uint32_t>(i)))
      marked_edges[e] = stamp;
    for (auto v : d.definition_vertices.at(static_cast<uint32_t>(i)))
      marked_vertices[v] = stamp;
    for (auto e : d.definition_edges.at(static_cast<uint32_t>(i)))
      for (auto id : d.edges[e].vertex_ids)
        require(marked_vertices[d.vertex_index.find(id).value] == stamp,
                "definition edge endpoint missing from vertex inventory");
    for (auto f : d.definition_faces.at(static_cast<uint32_t>(i)))
      for (auto l : slice(d.loop_ids, d.faces[f].loops))
        for (auto c : slice(d.coedge_ids, d.loops[d.loop_index.find(l).value].coedges)) {
          work.charge(1);
          require(marked_edges[d.edge_index.find(d.coedges[d.coedge_index.find(c).value].edge_id)
                                   .value] == stamp,
                  "definition face edge missing from edge inventory");
        }
  }
  for (const auto &v : d.styles) {
    for (auto c : v.rgba)
      require(c >= 0 && c <= 1, "invalid source RGBA");
    require(!v.has_density || v.density >= 0, "negative source material density");
  }
  for (const auto &v : d.diagnostics)
    require(v.severity < 2, "source adapter error diagnostic");
  std::pmr::vector<std::pair<uint32_t, uint32_t>> graph_pairs(d.resource.get());
  std::pmr::vector<uint32_t> indegree(d.products.size(), 0, d.resource.get());
  for (std::size_t i = 0; i < d.usages.size(); ++i) {
    const auto &v = d.usages[i];
    const auto a = d.product_index.find(v.parent_product_id).value;
    const auto b = d.product_index.find(v.child_product_id).value;
    style_reference(d, v.style_id);
    const auto placement = brepgpu::rigid_placement_status(v.transform.data());
    require(placement != brepgpu::PlacementStatus::nonfinite &&
                placement != brepgpu::PlacementStatus::nonorthonormal,
            "nonrigid original source placement");
    require(placement == brepgpu::PlacementStatus::rigid, "reflected/singular source placement");
    graph_pairs.emplace_back(a, static_cast<uint32_t>(i));
    require(indegree[b] != std::numeric_limits<uint32_t>::max(), "source usage degree overflow");
    ++indegree[b];
  }
  Csr graph(d.resource.get());
  make_csr(graph, graph_pairs, d.products.size());
  std::pmr::vector<uint32_t> queue(d.resource.get());
  std::pmr::vector<uint64_t> paths(d.products.size(), 0, d.resource.get());
  paths[d.product_index.find(d.root_product_id).value] = 1;
  for (std::size_t i = 0; i < indegree.size(); ++i)
    if (indegree[i] == 0)
      queue.push_back(static_cast<uint32_t>(i));
  uint64_t placements = 1;
  for (std::size_t pos = 0; pos < queue.size(); ++pos) {
    const auto a = queue[pos];
    for (auto usage : graph.at(a)) {
      work.charge(1);
      const auto b = d.product_index.find(d.usages[usage].child_product_id).value;
      if (paths[a] > work.limits.records - placements || paths[a] > work.limits.records - paths[b])
        throw Error(ErrorCode::resource_exhausted, "source assembly placement count exceeds limit");
      placements += paths[a];
      paths[b] += paths[a];
      if (--indegree[b] == 0)
        queue.push_back(b);
    }
  }
  require(queue.size() == d.products.size(), "cyclic source product usage graph");
}
} // namespace

std::span<const double> SourceData::view(Range<double> range) const {
  return slice(scalars, range);
}
std::span<const uint32_t> SourceData::view(Range<uint32_t> range) const {
  return slice(multiplicities, range);
}
std::string_view SourceData::text(Range<char> range) const {
  const auto data = slice(strings, range);
  return {data.data(), data.size()};
}
std::span<const uint32_t> Csr::at(uint32_t slot) const {
  if (static_cast<std::size_t>(slot) + 1 >= offsets.size())
    throw Error(ErrorCode::internal_error, "source CSR slot out of range");
  return std::span<const uint32_t>(values).subspan(
      static_cast<std::size_t>(offsets[slot]),
      static_cast<std::size_t>(offsets[static_cast<std::size_t>(slot) + 1] - offsets[slot]));
}
SourceStore read_source(std::span<const std::byte> bytes,
                        const std::shared_ptr<RunResource> &resource, SourceLimits limits) {
  if (!resource)
    throw Error(ErrorCode::internal_error, "source requires an owned run resource");
  if (bytes.size() > limits.wire_bytes || bytes.size() > (uint64_t{1} << 30))
    throw Error(ErrorCode::resource_exhausted, "source binary exceeds wire limit");
  Work work(limits);
  Reader r(bytes, work);
  require(equal_bytes(r.take(8), "BGSRC001") && r.u32() == 1, "unsupported source magic/version");
  r.zero();
  auto data = std::allocate_shared<SourceData>(RunAllocator<SourceData>(resource), resource);
  const auto digest = r.take(32);
  std::copy(digest.begin(), digest.end(), data->source_digest.begin());
  data->units_mm = r.f64();
  data->source_uncertainty_mm = r.f64();
  require(data->units_mm > 0 && data->source_uncertainty_mm >= 0,
          "invalid source units/uncertainty");
  data->root_product_id = r.id<ProductTag>();
  const auto sections = r.u32();
  r.zero();
  require(sections == tags.size() && r.integer() == bytes.size(),
          "source section count/file length mismatch");
  data->original_bytes.assign(bytes.begin(), bytes.end());
  for (std::size_t section = 0; section < tags.size(); ++section) {
    require(equal_bytes(r.take(8), tags[section]), "source section order/type mismatch");
    const auto count = r.integer();
    const auto size = r.integer();
    require(count <= size / 8, "source section count exceeds payload");
    work.records(count);
    if (section == 0)
      require(count == 1, "source requires one metadata row");
    Reader rows(r.take(size), work);
    for (uint64_t i = 0; i < count; ++i) {
      try {
        auto rr = rows.row();
        decode_row(section, rr, *data);
        rr.end();
      } catch (const Error &error) {
        throw Error(error.code,
                    std::string(tags[section]) + " row " + std::to_string(i) + ": " + error.what());
      }
    }
    rows.end();
  }
  r.end();
  validate(*data, work);
  return SourceStore(std::move(data));
}
SourceStore load_source(const std::filesystem::path &path,
                        const std::shared_ptr<RunResource> &resource, SourceLimits limits) {
  if (!resource)
    throw Error(ErrorCode::internal_error, "source requires an owned run resource");
  std::error_code error;
  const auto size = std::filesystem::file_size(path, error);
  if (error)
    throw Error(ErrorCode::io_failure, "cannot stat source input");
  if (size > limits.wire_bytes || size > (uint64_t{1} << 30))
    throw Error(ErrorCode::resource_exhausted, "source file exceeds wire limit");
  std::pmr::vector<std::byte> bytes(resource.get());
  bytes.resize(static_cast<std::size_t>(size));
  std::ifstream input(path, std::ios::binary);
  if (!input.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(size)) ||
      input.peek() != std::char_traits<char>::eof())
    throw Error(ErrorCode::io_failure, "source input changed or could not be read");
  // Keep this owner alive until the temporary byte vector is destroyed, even
  // when read_source throws and destroys its partially built SourceData.
  return read_source(bytes, resource, limits);
}
} // namespace bgmesh
