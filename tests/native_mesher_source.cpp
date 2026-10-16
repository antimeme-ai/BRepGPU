#include "source.hpp"
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using Bytes = std::vector<std::byte>;
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
enum class Width : uint32_t { u32 = 4, u64 = 8 };
void word(Bytes &out, uint64_t value, Width width = Width::u64) {
  for (uint32_t i = 0; i < static_cast<uint32_t>(width); ++i)
    out.push_back(static_cast<std::byte>((value >> (8 * i)) & 255));
}
void text(Bytes &out, std::string_view value) {
  word(out, value.size(), Width::u32);
  for (char c : value)
    out.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
}
void append(Bytes &out, std::span<const std::byte> data) {
  out.insert(out.end(), data.begin(), data.end());
}
void number(Bytes &out, double value) { word(out, std::bit_cast<uint64_t>(value)); }
void row(Bytes &out, const Bytes &value) {
  word(out, value.size());
  append(out, value);
}
Bytes fixture(bool curve = false, bool bad_weight = false, bool cycle = false) {
  std::array<Bytes, 13> payloads;
  std::array<uint64_t, 13> counts{};
  Bytes value;
  text(value, "native source fixture");
  row(payloads[0], value);
  counts[0] = 1;
  value.clear();
  word(value, 101);
  word(value, 0);
  word(value, 0);
  text(value, "root");
  text(value, "drawing");
  row(payloads[1], value);
  counts[1] = 1;
  if (cycle) {
    value.clear();
    word(value, 99);
    word(value, 101);
    word(value, 101);
    word(value, 0);
    text(value, "cycle");
    for (double x : std::array<double, 12>{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0})
      number(value, x);
    row(payloads[3], value);
    counts[3] = 1;
  }
  if (curve) {
    value.clear();
    word(value, 103);
    word(value, 3, Width::u32);
    word(value, 4, Width::u32);
    for (unsigned i = 0; i < 14; ++i)
      number(value, 0);
    number(value, 0);
    number(value, 1);
    word(value, 1, Width::u32);
    word(value, 0, Width::u32);
    word(value, 0);
    word(value, 0);
    word(value, 8);
    for (double x : std::array<double, 8>{-0.0, 0, 0, 1, 1, 2, 3, bad_weight ? 0.0 : 2.0})
      number(value, x);
    word(value, 0);
    row(payloads[10], value);
    counts[10] = 1;
  }
  constexpr std::array<std::string_view, 13> tags{
      "META____", "PRODUCTS", "SOLIDS__", "USAGES__", "VERTICES", "EDGES___", "COEDGES_",
      "LOOPS___", "FACES___", "CARRIERS", "CURVES__", "STYLES__", "DIAGS___"};
  Bytes out;
  for (char c : std::string_view("BGSRC001"))
    out.push_back(static_cast<std::byte>(c));
  word(out, 1, Width::u32);
  word(out, 0, Width::u32);
  for (unsigned i = 0; i < 32; ++i)
    out.push_back(static_cast<std::byte>(i));
  number(out, 25.4);
  number(out, .001);
  word(out, 101);
  word(out, 13, Width::u32);
  word(out, 0, Width::u32);
  uint64_t size = 88;
  for (const auto &p : payloads)
    size += 24 + p.size();
  word(out, size);
  for (std::size_t i = 0; i < tags.size(); ++i) {
    for (char c : tags[i])
      out.push_back(static_cast<std::byte>(c));
    word(out, counts[i]);
    word(out, payloads[i].size());
    append(out, payloads[i]);
  }
  return out;
}
void rejected(const Bytes &bytes, bgmesh::ErrorCode expected, bgmesh::SourceLimits limits = {}) {
  auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 24);
  try {
    static_cast<void>(bgmesh::read_source(bytes, resource, limits));
  } catch (const bgmesh::Error &error) {
    require(error.code == expected, "wrong source failure class");
    require(resource->live() == 0, "failed source retains reserved memory");
    return;
  }
  throw std::runtime_error("invalid source admitted");
}
void check() {
  bool null_rejected = false;
  try {
    static_cast<void>(bgmesh::load_source("unopened-source.bgsource", {}));
  } catch (const bgmesh::Error &error) {
    null_rejected = error.code == bgmesh::ErrorCode::internal_error;
  }
  require(null_rejected, "null source resource reached a PMR allocator");
  auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 24);
  const auto bytes = fixture(true);
  {
    const auto source = bgmesh::read_source(bytes, resource);
    const auto &data = source.view();
    require(data.products.size() == 1 && data.curves.size() == 1, "lost source inventory");
    require(data.units_mm == 25.4, "source metadata was rescaled/rejected");
    require(data.text(data.products[0].name) == "root", "wrong source string");
    require(data.root_product_id.value == 101 && data.product_index.find({101}).value == 0,
            "source ID/slot association changed");
    require(data.original_bytes.size() == bytes.size(), "source bytes lost");
    require(std::equal(bytes.begin(), bytes.end(), data.original_bytes.begin()),
            "source bits changed");
    const auto controls = data.view(data.curves[0].controls);
    require(controls.size() == 8 && std::bit_cast<uint64_t>(controls[0]) == (uint64_t{1} << 63),
            "original signed-zero/control words lost");
    require(data.edge_coedges.offsets.size() == 1, "empty CSR sentinel missing");
    require(resource->live() > 0, "source allocations bypass run resource");
  }
  require(resource->live() == 0, "source destruction leaks reserved memory");
  rejected(fixture(true, true), bgmesh::ErrorCode::invalid_source);
  rejected(fixture(false, false, true), bgmesh::ErrorCode::invalid_source);
  rejected(bytes, bgmesh::ErrorCode::resource_exhausted, {bytes.size() - 1, 10000, 10000});
  rejected(bytes, bgmesh::ErrorCode::resource_exhausted, {uint64_t{1} << 30, 0, 10000});
  for (std::size_t length = 0; length < bytes.size(); ++length) {
    Bytes cut(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(length));
    rejected(cut, bgmesh::ErrorCode::invalid_source);
  }
  for (std::size_t offset : std::array<std::size_t, 6>{0, 8, 12, 72, 76, 80}) {
    auto corrupt = bytes;
    corrupt[offset] ^= std::byte{0x80};
    rejected(corrupt, bgmesh::ErrorCode::invalid_source);
  }
  auto extra = bytes;
  extra.push_back(std::byte{0});
  rejected(extra, bgmesh::ErrorCode::invalid_source);
  auto small = std::make_shared<bgmesh::RunResource>(1);
  try {
    static_cast<void>(bgmesh::read_source(bytes, small));
  } catch (const bgmesh::Error &error) {
    require(error.code == bgmesh::ErrorCode::resource_exhausted && small->live() == 0,
            "allocation failure is not transactional");
    return;
  }
  throw std::runtime_error("source allocation limit ignored");
}
void all_sections(const char *path) {
  auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 24);
  {
    const auto source = bgmesh::load_source(path, resource);
    const auto &d = source.view();
    require(d.products.size() == 2 && d.definitions.size() == 1 && d.usages.size() == 1 &&
                d.vertices.size() == 4 && d.edges.size() == 4 && d.coedges.size() == 4 &&
                d.loops.size() == 1 && d.faces.size() == 1 && d.carriers.size() == 1 &&
                d.curves.size() == 8 && d.styles.size() == 1 && d.diagnostics.size() == 1,
            "all-section inventory mismatch");
    require(d.units_mm == 25.4 && d.source_uncertainty_mm == .0007 &&
                d.root_product_id.value == 100,
            "all-section metadata mismatch");
    require(d.text(d.adapter_revision) == "literal all-section λ fixture" &&
                d.text(d.products[1].reference) == "drawing",
            "UTF-8/source strings mismatch");
    require(d.usages[0].transform[3] == 13 && d.usages[0].transform[7] == -7 &&
                d.usages[0].transform[11] == .5 && d.usages[0].child_product_id.value == 101,
            "source usage changed");
    require(!d.edges[0].curve_same_sense && !d.edges[0].same_parameter &&
                d.edges[0].vertex_ids[0].value == 300 && d.edges[0].parameter_range[0] == 0,
            "source endpoint orientation changed");
    require(d.coedges[0].reconstructed && !d.coedges[1].reconstructed &&
                d.coedges[3].parameter_map_kind == 3 && d.coedges[3].periodic_shift[1] == -3 &&
                d.parameter_maps[d.coedges[0].parameter_map.first + 1] == bgmesh::Vec2{1, 1},
            "pcurve authority/map/lift lost");
    require(d.faces[0].orientation == -1 && d.faces[0].tolerance == .0003 && d.loops[0].outer,
            "source face/loop fields changed");
    const auto &c = d.carriers[0];
    require(c.kind == 6 && c.control_shape == std::array<uint32_t, 2>{2, 2} &&
                c.degrees == std::array<uint32_t, 2>{1, 1} && c.uv_bounds[0] == -2 &&
                d.view(c.knot_u)[1] == 1 && d.view(c.mult_u)[0] == 2 && c.spans.count == 1 &&
                std::bit_cast<uint64_t>(d.view(c.controls)[0]) == (uint64_t{1} << 63),
            "original surface/hint/spans changed");
    require(d.curves[0].kind == 5 && d.curves[0].dimension == 3 && d.curves[0].spans.count == 1 &&
                d.curves[1].kind == 4 && d.curves[1].dimension == 2,
            "source curve kinds/spans lost");
    const auto &s = d.styles[0];
    require(s.has_color && s.has_density && s.rgba[2] == .5 && s.density == .0085 &&
                d.text(s.material_name) == "brass λ" && d.text(s.density_unit) == "g/mm³" &&
                d.source_ids[s.source_ids.first] == 42,
            "source appearance/material lost");
    require(d.diagnostics[0].severity == 1 &&
                d.text(d.diagnostics[0].message) == "retained warning",
            "source diagnostic lost");
    require(d.definition_faces.at(0).size() == 1 && d.definition_edges.at(0).size() == 4 &&
                d.definition_vertices.at(0).size() == 4 && d.edge_coedges.at(0).size() == 1 &&
                d.edge_faces.at(0)[0] == 0,
            "source CSR inventory mismatch");
  }
  require(resource->live() == 0, "loaded source/resource lifetime leak");
}
} // namespace
int main(int argc, char **argv) {
  try {
    check();
    if (argc == 2)
      all_sections(argv[1]);
    std::cout << "native source specification checks PASS\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
