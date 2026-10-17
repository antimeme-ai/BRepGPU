#include "mesh_io.hpp"
#include "mesh_walk.hpp"
#include "snapshot_digest.hpp"
#include <atomic>
#include <bit>
#include <fstream>
#include <limits>
#include <unordered_map>

namespace bgmesh {
namespace {
using mesh_detail::invalid;
constexpr uint32_t section_count = 29;
struct Section final {
  std::string_view tag;
  uint32_t width;
  uint64_t count = 0, offset = 0, bytes = 0;
};
struct Patch final {
  uint64_t offset, value;
};
struct SectionShape final {
  uint64_t count;
  uint32_t width;
};
struct WireHeader final {
  SourceDigest original{}, snapshot{};
  uint64_t length = 0;
};
std::array<Section, section_count> schema() {
  return {{{"SOURCE__", 0},
           {"EXACT___", 32},
           {"LIMBS___", 4},
#define BGNMESH_SECTION(tag, type, member, width) {tag, width},
           BGNMESH_BEFORE_QUALITY(BGNMESH_SECTION){"QUALITY_", 24},
           BGNMESH_AFTER_QUALITY(BGNMESH_SECTION)
#undef BGNMESH_SECTION
  }};
}
uint64_t sum(uint64_t a, uint64_t b) {
  if (b > UINT64_MAX - a)
    invalid("mesh wire count overflow");
  return a + b;
}
uint64_t product(uint64_t a, uint64_t b) {
  if (a && b > UINT64_MAX / a)
    invalid("mesh wire product overflow");
  return a * b;
}
std::size_t size(uint64_t n) {
  if (n > std::numeric_limits<std::size_t>::max())
    invalid("mesh count exceeds host size");
  return static_cast<std::size_t>(n);
}
struct File final {
  std::fstream stream;
  bgfd::Budget &budget;
  uint64_t position = 0, end = 0;
  bool writing;
  File(const std::filesystem::path &path, bgfd::Budget &work, uint64_t cap, bool output)
      : stream(path, std::ios::binary | (output ? std::ios::out | std::ios::trunc : std::ios::in)),
        budget(work), end(cap), writing(output) {
    if (!stream)
      invalid("cannot open native mesh file");
  }
  void seek(uint64_t n) {
    if (n > end || n > static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max()))
      invalid("mesh seek outside file");
    if (writing)
      stream.seekp(static_cast<std::streamoff>(n));
    else
      stream.seekg(static_cast<std::streamoff>(n));
    if (!stream)
      invalid("native mesh seek failed");
    position = n;
  }
  void raw(std::span<std::byte> bytes) {
    if (bytes.size() > end - position) {
      if (writing)
        throw Error(ErrorCode::resource_exhausted, "mesh output byte limit exceeded");
      invalid("truncated mesh payload");
    }
    budget.charge(bytes.size());
    for (std::size_t offset = 0; offset < bytes.size();) {
      const auto count = std::min<std::size_t>(65536, bytes.size() - offset);
      auto *pointer = reinterpret_cast<char *>(bytes.data() + offset);
      if (writing)
        stream.write(pointer, static_cast<std::streamsize>(count));
      else
        stream.read(pointer, static_cast<std::streamsize>(count));
      if (!stream)
        invalid("native mesh read/write failed");
      offset += count;
    }
    position += bytes.size();
  }
  void raw_const(std::span<const std::byte> bytes) {
    if (!writing)
      invalid("mesh input cannot write bytes");
    // The output API never writes through this span.
    if (bytes.size() > end - position)
      throw Error(ErrorCode::resource_exhausted, "mesh output byte limit exceeded");
    budget.charge(bytes.size());
    for (std::size_t offset = 0; offset < bytes.size();) {
      const auto count = std::min<std::size_t>(65536, bytes.size() - offset);
      stream.write(reinterpret_cast<const char *>(bytes.data() + offset),
                   static_cast<std::streamsize>(count));
      if (!stream)
        invalid("native mesh write failed");
      offset += count;
    }
    position += bytes.size();
  }
  void u64(uint64_t &v) {
    std::array<std::byte, 8> bytes{};
    if (writing)
      for (unsigned i = 0; i < 8; ++i)
        bytes[i] = static_cast<std::byte>((v >> (8 * i)) & 255);
    raw(bytes);
    if (!writing) {
      v = 0;
      for (unsigned i = 0; i < 8; ++i)
        v |= uint64_t(std::to_integer<uint8_t>(bytes[i])) << (8 * i);
    }
  }
  void u32(uint32_t &v) {
    std::array<std::byte, 4> bytes{};
    if (writing)
      for (unsigned i = 0; i < 4; ++i)
        bytes[i] = static_cast<std::byte>((v >> (8 * i)) & 255);
    raw(bytes);
    if (!writing) {
      v = 0;
      for (unsigned i = 0; i < 4; ++i)
        v |= uint32_t(std::to_integer<uint8_t>(bytes[i])) << (8 * i);
    }
  }
  void zero() {
    uint32_t v = 0;
    u32(v);
    if (v)
      invalid("nonzero mesh reserved field");
  }
  void patch(Patch change) {
    const auto saved = position;
    seek(change.offset);
    u64(change.value);
    seek(saved);
  }
};
bool same(ExactView a, ExactView b) {
  return a.sign == b.sign && std::ranges::equal(a.numerator, b.numerator) &&
         std::ranges::equal(a.denominator, b.denominator);
}
struct ExactIndex final {
  const NativeMeshData &data;
  bgfd::Budget &budget;
  uint64_t limit;
  std::pmr::vector<const ExactTable *> tables;
  std::pmr::vector<ExactView> values;
  std::pmr::unordered_multimap<uint64_t, uint64_t> hashes;
  ExactIndex(const NativeMeshData &mesh, bgfd::Budget &work, uint64_t rows)
      : data(mesh), budget(work), limit(rows), tables(mesh.resource.get()),
        values(mesh.resource.get()), hashes(mesh.resource.get()) {
    tables.reserve(mesh.exact_tables.size());
    for (const auto &t : mesh.exact_tables)
      tables.push_back(&t);
    std::sort(tables.begin(), tables.end(), [](auto a, auto b) { return a->key() < b->key(); });
    for (std::size_t i = 1; i < tables.size(); ++i)
      if (tables[i - 1]->key() == tables[i]->key())
        invalid("duplicate mesh exact table key");
  }
  ExactView view(ExactRef ref) {
    budget.charge(1);
    const auto it = std::lower_bound(tables.begin(), tables.end(), ref.table,
                                     [](auto t, auto key) { return t->key() < key; });
    if (it == tables.end() || (*it)->key() != ref.table)
      invalid("mesh exact reference has no retained table");
    return (*it)->view(ref);
  }
  uint64_t intern(ExactRef ref) {
    const auto v = view(ref);
    uint64_t hash = 1469598103934665603ULL;
    const auto mix = [&](uint64_t word) {
      budget.charge(1);
      hash = (hash ^ word) * 1099511628211ULL;
    };
    mix(static_cast<uint64_t>(v.sign + 1));
    mix(v.numerator.size());
    mix(v.denominator.size());
    for (auto word : v.numerator)
      mix(word);
    for (auto word : v.denominator)
      mix(word);
    const auto range = hashes.equal_range(hash);
    for (auto it = range.first; it != range.second; ++it) {
      budget.charge(v.numerator.size() + v.denominator.size());
      if (same(values[size(it->second - 1)], v))
        return it->second;
    }
    if (values.size() >= limit)
      throw Error(ErrorCode::resource_exhausted, "mesh exact row limit exceeded");
    values.push_back(v);
    const auto id = static_cast<uint64_t>(values.size());
    hashes.emplace(hash, id);
    return id;
  }
};
struct Collect final : mesh_detail::MeshObserver<Collect> {
  ExactIndex &index;
  uint64_t list_items = 0;
  Collect(const NativeMeshData &d, bgfd::Budget &b, ExactIndex &i) : MeshObserver{d, b}, index(i) {}
  void u64(uint64_t &) { budget.charge(1); }
  void u32(uint32_t &) { budget.charge(1); }
  void i32(int32_t &v) {
    auto word = std::bit_cast<uint32_t>(v);
    u32(word);
  }
  void f64(double &v) {
    require(std::isfinite(v), "nonfinite mesh scalar");
    budget.charge(1);
  }
  void exact(ExactRef &v) { static_cast<void>(index.intern(v)); }
  void account_items(uint64_t n) {
    list_items = sum(list_items, n);
    if (list_items > index.limit)
      throw Error(ErrorCode::resource_exhausted, "mesh aggregate list limit exceeded");
  }
};
struct Encode final : mesh_detail::MeshObserver<Encode> {
  ExactIndex &index;
  File &file;
  Encode(const NativeMeshData &d, bgfd::Budget &b, ExactIndex &i, File &f)
      : MeshObserver{d, b}, index(i), file(f) {}
  void u64(uint64_t &v) { file.u64(v); }
  void u32(uint32_t &v) { file.u32(v); }
  void i32(int32_t &v) {
    auto word = std::bit_cast<uint32_t>(v);
    u32(word);
  }
  void f64(double &v) {
    require(std::isfinite(v), "nonfinite mesh scalar");
    auto word = std::bit_cast<uint64_t>(v);
    u64(word);
  }
  void exact(ExactRef &v) {
    auto id = index.intern(v);
    u64(id);
  }
};
template <class Visitor> void visit_mesh(Visitor &visitor, const NativeMeshData &data) {
#define BGNMESH_VISIT(tag, type, member, width)                                                    \
  for (const auto &original : data.member) {                                                       \
    auto row = original;                                                                           \
    visitor.fields(row);                                                                           \
  }
  BGNMESH_BEFORE_QUALITY(BGNMESH_VISIT)
  auto quality = data.quality;
  visitor.fields(quality);
  BGNMESH_AFTER_QUALITY(BGNMESH_VISIT)
#undef BGNMESH_VISIT
}
uint64_t section_start(File &file, std::string_view tag, SectionShape shape) {
  file.raw_const(std::as_bytes(std::span(tag.data(), tag.size())));
  file.u64(shape.count);
  uint64_t bytes = 0;
  file.u64(bytes);
  file.u32(shape.width);
  file.zero();
  return file.position;
}
void section_end(File &file, uint64_t start) { file.patch({start - 16, file.position - start}); }
void header(File &file, WireHeader &meta) {
  std::array<std::byte, 8> magic{};
  constexpr std::string_view name = "BGNMESH1";
  if (file.writing)
    std::copy(std::as_bytes(std::span(name.data(), name.size())).begin(),
              std::as_bytes(std::span(name.data(), name.size())).end(), magic.begin());
  file.raw(magic);
  if (!file.writing &&
      !std::ranges::equal(magic, std::as_bytes(std::span(name.data(), name.size()))))
    invalid("unknown native mesh magic");
  uint32_t version = 1;
  file.u32(version);
  if (version != 1)
    invalid("unknown native mesh version");
  file.zero();
  file.u64(meta.length);
  uint32_t count = section_count;
  file.u32(count);
  if (count != section_count)
    invalid("unknown native mesh section inventory");
  file.zero();
  file.raw(meta.original);
  file.raw(meta.snapshot);
}
std::atomic<uint64_t> decoded_owner{0};
ExactTableKey decode_key() {
  auto id = decoded_owner.load(std::memory_order_relaxed);
  for (;;) {
    if (id == UINT64_MAX)
      throw Error(ErrorCode::resource_exhausted, "decoded mesh identities exhausted");
    if (decoded_owner.compare_exchange_weak(id, id + 1, std::memory_order_relaxed))
      return {{OwnerKind::run, UINT32_MAX, id}, 0, 0};
  }
}
struct Decode final {
  NativeMeshData &data;
  bgfd::Budget &budget;
  File &file;
  const ExactTable &table;
  uint64_t limit, items = 0;
  void require(bool value, const char *message) {
    if (!value)
      invalid(message);
  }
  template <class... T> void fields(T &...v) { (mesh_detail::transfer(*this, v), ...); }
  void u64(uint64_t &v) { file.u64(v); }
  void u32(uint32_t &v) { file.u32(v); }
  void zero32() { file.zero(); }
  void i32(int32_t &v) {
    uint32_t word = 0;
    u32(word);
    v = std::bit_cast<int32_t>(word);
  }
  void f64(double &v) {
    uint64_t word = 0;
    u64(word);
    v = std::bit_cast<double>(word);
    require(std::isfinite(v), "nonfinite mesh scalar");
  }
  template <class T> void source(SourceId<T> &v) {
    u64(v.value);
    require(mesh_detail::source_exists(data, v), "mesh source ID absent from original source");
  }
  template <class T> void reference(MeshRef<T> &v) {
    u64(v.value);
    require(v.value && v.value <= mesh_detail::rows<T>(data).size(),
            "mesh reference outside row table");
  }
  template <class T> void optional_reference(MeshRef<T> &v) {
    u64(v.value);
    require(v.value <= mesh_detail::rows<T>(data).size(), "optional mesh reference outside table");
  }
  void exact(ExactRef &v) {
    uint64_t id = 0;
    u64(id);
    require(id && id <= table.size(), "mesh exact reference outside table");
    v = {table.key(), id - 1};
  }
  void optional_exact(std::optional<ExactRef> &v) {
    uint64_t id = 0;
    u64(id);
    require(id <= table.size(), "optional mesh exact reference outside table");
    if (id)
      v = ExactRef{table.key(), id - 1};
    else
      v.reset();
  }
  template <class T> void list(Range<T> &v) {
    uint64_t count = 0;
    u64(count);
    items = sum(items, count);
    if (items > limit)
      throw Error(ErrorCode::resource_exhausted, "mesh aggregate list count limit exceeded");
    budget.charge(count);
    auto &p = mesh_detail::pool<T>(data);
    const auto first = static_cast<uint64_t>(p.size());
    require(count <= p.max_size() - p.size(), "mesh list allocation overflow");
    p.resize(size(sum(first, count)));
    v = {first, count};
    for (uint64_t i = 0; i < count; ++i)
      fields(p[size(first + i)]);
  }
  template <std::size_t I = 0, class... T> void alternative(std::variant<T...> &v, uint32_t index) {
    if constexpr (I < sizeof...(T)) {
      if (index == I) {
        v.template emplace<I>();
        fields(std::get<I>(v));
      } else
        alternative<I + 1>(v, index);
    } else
      invalid("unknown mesh variant");
  }
  template <class... T> void variant(std::variant<T...> &v) {
    uint32_t tag = 0;
    u32(tag);
    zero32();
    constexpr bool none =
        std::is_same_v<std::variant_alternative_t<0, std::variant<T...>>, std::monostate>;
    require(none ? tag < sizeof...(T) : tag >= 1 && tag <= sizeof...(T),
            "unknown mesh variant tag");
    alternative(v, tag - (none ? 0u : 1u));
  }
};
struct Descriptor final {
  int32_t sign;
  uint32_t n, d;
  uint64_t no, doff;
};
ExactTable load_exacts(File &file, const std::array<Section, section_count> &sections,
                       const std::shared_ptr<RunResource> &resource, bgfd::Budget &budget) {
  const auto &es = sections[1], &ls = sections[2];
  std::pmr::vector<Descriptor> descriptors(resource.get());
  descriptors.resize(size(es.count));
  file.seek(es.offset);
  uint64_t limb_count = 0;
  for (auto &d : descriptors) {
    uint32_t sign = 0;
    file.u32(sign);
    d.sign = std::bit_cast<int32_t>(sign);
    file.u32(d.n);
    file.u64(d.no);
    file.u32(d.d);
    file.zero();
    file.u64(d.doff);
    if (d.sign < -1 || d.sign > 1 || !d.d || d.no != limb_count || d.doff != sum(d.no, d.n))
      invalid("noncanonical exact descriptor");
    limb_count = sum(d.doff, d.d);
  }
  if (limb_count != ls.count)
    invalid("unreachable or missing exact limbs");
  std::pmr::vector<uint32_t> limbs(resource.get());
  limbs.resize(size(ls.count));
  file.seek(ls.offset);
  for (auto &word : limbs)
    file.u32(word);
  ExactBuilder builder(resource, decode_key());
  for (const auto &d : descriptors) {
    const auto all = std::span<const uint32_t>(limbs);
    const ExactView view{d.sign, all.subspan(size(d.no), d.n), all.subspan(size(d.doff), d.d)};
    if ((d.n && view.numerator.back() == 0) || view.denominator.back() == 0 ||
        ((d.sign == 0) != (d.n == 0)))
      invalid("noncanonical exact magnitude/sign");
    const auto value = import_exact(budget, view);
    if (value.sign() != view.sign ||
        !std::ranges::equal(value.numerator().magnitude(), view.numerator) ||
        !std::ranges::equal(value.denominator().magnitude(), view.denominator))
      invalid("unreduced exact wire fraction");
    static_cast<void>(builder.append(value));
  }
  return builder.seal();
}
void translated(const bgfd::Failure &failure) {
  throw Error(failure.status == BGFD_RESOURCE_EXHAUSTED ? ErrorCode::resource_exhausted
                                                        : ErrorCode::invalid_source,
              failure.what());
}
} // namespace
void write_native_mesh(const std::filesystem::path &path, const NativeMesh &mesh,
                       MeshIOLimits limits) {
  try {
    const auto &d = mesh.view();
    bgfd::Budget budget({limits.bytes, limits.work}, d.resource.get());
    ExactIndex index(d, budget, limits.rows);
    Collect collect(d, budget, index);
    visit_mesh(collect, d);
    uint64_t total_rows = sum(sum(2, index.values.size()), collect.list_items), limb_count = 0;
    for (const auto v : index.values) {
      if (v.numerator.size() > UINT32_MAX || v.denominator.size() > UINT32_MAX)
        invalid("exact wire magnitude count overflow");
      limb_count = sum(limb_count, sum(v.numerator.size(), v.denominator.size()));
    }
    total_rows = sum(total_rows, limb_count);
#define BGNMESH_COUNT(tag, type, member, width) total_rows = sum(total_rows, d.member.size());
    BGNMESH_TABLES(BGNMESH_COUNT)
#undef BGNMESH_COUNT
    if (total_rows > limits.rows)
      throw Error(ErrorCode::resource_exhausted, "mesh aggregate row limit exceeded");
    budget.charge(d.source.view().original_bytes.size());
    WireHeader meta{d.source.view().source_digest, snapshot_digest(d.source.view().original_bytes),
                    0};
    File file(path, budget, limits.bytes, true);
    header(file, meta);
    auto start = section_start(file, "SOURCE__", {1, 0});
    uint64_t row_bytes = sum(8, d.source.view().original_bytes.size());
    file.u64(row_bytes);
    auto byte_count = static_cast<uint64_t>(d.source.view().original_bytes.size());
    file.u64(byte_count);
    file.raw_const(d.source.view().original_bytes);
    section_end(file, start);
    start = section_start(file, "EXACT___", {index.values.size(), 32});
    uint64_t offset = 0;
    for (auto v : index.values) {
      auto sign = std::bit_cast<uint32_t>(static_cast<int32_t>(v.sign));
      auto n = static_cast<uint32_t>(v.numerator.size()),
           den = static_cast<uint32_t>(v.denominator.size());
      file.u32(sign);
      file.u32(n);
      file.u64(offset);
      file.u32(den);
      file.zero();
      offset += n;
      file.u64(offset);
      offset += den;
    }
    section_end(file, start);
    start = section_start(file, "LIMBS___", {limb_count, 4});
    for (auto v : index.values) {
      for (auto word : v.numerator)
        file.u32(word);
      for (auto word : v.denominator)
        file.u32(word);
    }
    section_end(file, start);
    Encode encode(d, budget, index, file);
#define BGNMESH_WRITE(tag, type, member, width)                                                    \
  {                                                                                                \
    const auto section = section_start(file, tag, {d.member.size(), width});                       \
    for (const auto &original_row : d.member) {                                                    \
      auto row = original_row;                                                                     \
      uint64_t row_start = 0;                                                                      \
      if constexpr (width == 0) {                                                                  \
        uint64_t bytes = 0;                                                                        \
        file.u64(bytes);                                                                           \
        row_start = file.position;                                                                 \
      }                                                                                            \
      encode.fields(row);                                                                          \
      if constexpr (width == 0)                                                                    \
        file.patch({row_start - 8, file.position - row_start});                                    \
    }                                                                                              \
    section_end(file, section);                                                                    \
  }
    BGNMESH_BEFORE_QUALITY(BGNMESH_WRITE)
    start = section_start(file, "QUALITY_", {1, 24});
    auto quality = d.quality;
    encode.fields(quality);
    section_end(file, start);
    BGNMESH_AFTER_QUALITY(BGNMESH_WRITE)
#undef BGNMESH_WRITE
    file.patch({16, file.position});
    file.stream.flush();
    if (!file.stream)
      invalid("native mesh flush failed");
  } catch (const bgfd::Failure &failure) {
    translated(failure);
  }
}
NativeMesh load_native_mesh(const std::filesystem::path &path,
                            const std::shared_ptr<RunResource> &resource, MeshIOLimits limits) {
  if (!resource)
    throw Error(ErrorCode::internal_error, "null native mesh resource");
  try {
    std::error_code error;
    const auto length = std::filesystem::file_size(path, error);
    if (error || length < 96)
      invalid("native mesh file length outside limit");
    if (length > limits.bytes)
      throw Error(ErrorCode::resource_exhausted, "native mesh input byte limit exceeded");
    bgfd::Budget budget({limits.bytes, limits.work}, resource.get());
    File file(path, budget, length, false);
    WireHeader meta;
    header(file, meta);
    if (meta.length != length)
      invalid("native mesh header length mismatch");
    auto sections = schema();
    uint64_t rows = 0;
    for (auto &section : sections) {
      std::array<std::byte, 8> tag{};
      file.raw(tag);
      if (!std::ranges::equal(tag,
                              std::as_bytes(std::span(section.tag.data(), section.tag.size()))))
        invalid("unknown/reordered mesh section");
      file.u64(section.count);
      file.u64(section.bytes);
      uint32_t width = 0;
      file.u32(width);
      file.zero();
      if (width != section.width)
        invalid("native mesh row width mismatch");
      rows = sum(rows, section.count);
      if (rows > limits.rows)
        throw Error(ErrorCode::resource_exhausted, "mesh row limit exceeded");
      if (section.bytes > length - file.position ||
          (width ? product(section.count, width) != section.bytes
                 : product(section.count, 8) > section.bytes))
        invalid("mesh section size mismatch");
      section.offset = file.position;
      file.seek(sum(section.offset, section.bytes));
    }
    if (file.position != length || sections[0].count != 1 || sections[26].count != 1)
      invalid("trailing mesh bytes or singleton count mismatch");
    file.seek(sections[0].offset);
    uint64_t source_row = 0, source_size = 0;
    file.u64(source_row);
    file.u64(source_size);
    if (source_row != sum(source_size, 8) || sections[0].bytes != sum(source_row, 8))
      invalid("mesh source section length mismatch");
    const auto source = [&] {
      std::pmr::vector<std::byte> source_bytes(resource.get());
      source_bytes.resize(size(source_size));
      file.raw(source_bytes);
      budget.charge(source_size);
      if (snapshot_digest(source_bytes) != meta.snapshot)
        invalid("native mesh source snapshot SHA mismatch");
      return read_source(source_bytes, resource, {limits.bytes, limits.rows, limits.work});
    }();
    if (source.view().source_digest != meta.original)
      invalid("native mesh original source SHA identity mismatch");
    NativeMeshBuilder builder(resource, source);
    auto &d = builder.data();
    const auto exact = load_exacts(file, sections, resource, budget);
    d.exact_tables.push_back(exact);
    uint32_t section_index = 3;
#define BGNMESH_ALLOC(tag, type, member, width)                                                    \
  d.member.resize(size(sections[section_index++].count));
    BGNMESH_BEFORE_QUALITY(BGNMESH_ALLOC)
    ++section_index;
    BGNMESH_AFTER_QUALITY(BGNMESH_ALLOC)
#undef BGNMESH_ALLOC
    Decode decode{d, budget, file, exact, limits.rows, rows};
    section_index = 3;
#define BGNMESH_READ(tag, type, member, width)                                                     \
  {                                                                                                \
    const auto &section = sections[section_index++];                                               \
    file.seek(section.offset);                                                                     \
    const auto section_end_position = sum(section.offset, section.bytes);                          \
    for (auto &row : d.member) {                                                                   \
      uint64_t row_end = section_end_position;                                                     \
      if constexpr (width == 0) {                                                                  \
        uint64_t bytes = 0;                                                                        \
        file.u64(bytes);                                                                           \
        row_end = sum(file.position, bytes);                                                       \
        if (row_end > section_end_position)                                                        \
          invalid("mesh variable row exceeds section");                                            \
      }                                                                                            \
      const auto previous_end = file.end;                                                          \
      file.end = row_end;                                                                          \
      decode.fields(row);                                                                          \
      file.end = previous_end;                                                                     \
      if constexpr (width == 0) {                                                                  \
        if (file.position != row_end)                                                              \
          invalid("unconsumed variable mesh row");                                                 \
      }                                                                                            \
    }                                                                                              \
    if (file.position != section_end_position)                                                     \
      invalid("unconsumed mesh section");                                                          \
  }
    BGNMESH_BEFORE_QUALITY(BGNMESH_READ)
    file.seek(sections[section_index++].offset);
    decode.fields(d.quality);
    BGNMESH_AFTER_QUALITY(BGNMESH_READ)
#undef BGNMESH_READ
    if (file.position != length || file.stream.peek() != std::char_traits<char>::eof())
      invalid("trailing bytes in opened native mesh stream");
    ExactIndex index(d, budget, limits.rows);
    Collect collect(d, budget, index);
    visit_mesh(collect, d);
    if (index.values.size() != exact.size())
      invalid("unreachable/duplicate exact mesh values");
    for (uint64_t i = 0; i < exact.size(); ++i)
      if (!same(index.values[size(i)], exact.view({exact.key(), i})))
        invalid("mesh exact rows not in canonical semantic order");
    return builder.seal(budget);
  } catch (const bgfd::Failure &failure) {
    translated(failure);
  }
  throw Error(ErrorCode::internal_error, "unreachable mesh decoder");
}
} // namespace bgmesh
