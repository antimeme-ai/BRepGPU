#include "../native/mesher/mesh_io.hpp"
#include "../native/mesher/snapshot_digest.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace bgmesh;
void require(bool value, const char *text) {
  if (!value)
    throw std::runtime_error(text);
}
struct FixtureOptions final {
  bool alternative_storage = false, source_maps = false;
  bgfd::Budget *validation = nullptr;
};
NativeMesh fixture(const std::shared_ptr<RunResource> &run, const SourceStore &source,
                   FixtureOptions options = {}) {
  const auto unused = options.alternative_storage;
  bgfd::Budget b({1ULL << 24, 1ULL << 32}, run.get());
  NativeMeshBuilder builder(run, source);
  auto &d = builder.data();
  ExactBuilder exact(run, unused ? ExactTableKey{{OwnerKind::face, 99, 876}, 123, 87}
                                 : ExactTableKey{{OwnerKind::run, 0, 0}, 0, 0});
  if (unused)
    static_cast<void>(exact.append(Rational(b, 1234567).divide(Rational(b, 89))));
  const auto first = exact.append(Rational(b, unused ? 1 : 0)),
             second = exact.append(Rational(b, unused ? 0 : 1));
  const auto z = unused ? second : first, o = unused ? first : second;
  auto two = exact.append(Rational(b, 2));
  d.exact_tables.push_back(exact.seal());
  if (unused) {
    ExactBuilder duplicate(run, {{OwnerKind::run, 0, 1234}, 19, 73});
    two = duplicate.append(Rational(b, 2));
    d.exact_tables.insert(d.exact_tables.begin(), duplicate.seal());
  }
  d.quality = {2., 2., 1};
  const auto &s = source.view();
  d.definitions.push_back({s.definitions[0].id, {0, 1}, {0, 4}, {0, 4}, {0, 2}});
  d.face_refs.push_back({1});
  d.loop_refs.push_back({1});
  d.cell_refs = {{1}, {2}};
  d.faces.push_back({s.faces[0].id, {1}, {1}, {}, 0, {0, 1}, {0, 2}, {0, 2}});
  d.charts.push_back({s.carriers[0].id, OriginalUVChart{{z, o, z, o}}});
  const std::array<std::array<ExactRef, 2>, 4> uv{{{z, z}, {o, z}, {o, o}, {z, o}}};
  for (uint64_t i = 0; i < 4; ++i) {
    d.nodes.push_back({{1}, s.vertices[i].xyz, OriginalVertexGenerator{s.vertices[i].id}, {}});
    d.sites.push_back({{1}, {1}, uv[i], {i + 1}, {}, {i == 3 ? 2u : 1u}});
    d.node_refs.push_back({i + 1});
    d.site_refs.push_back({i + 1});
    d.edges.push_back({s.edges[i].id, {1}, 0, {i * 2, 2}, {}});
    d.edge_samples.push_back({{i + 1}, z, {i + 1}, {}});
    d.edge_samples.push_back({{i + 1}, o, {(i + 1) % 4 + 1}, {}});
    d.edge_sample_refs.push_back({i * 2 + 1});
    d.edge_sample_refs.push_back({i * 2 + 2});
    d.uses.push_back({s.coedges[i].id,
                      s.loops[0].id,
                      {1},
                      {i + 1},
                      s.coedges[i].sense,
                      s.coedges[i].reconstructed,
                      s.coedges[i].parameter_map_kind,
                      s.coedges[i].periodic_shift,
                      {i * 2, 2},
                      {},
                      {}});
    d.use_refs.push_back({i + 1});
    for (uint64_t endpoint = 0; endpoint < 2; ++endpoint) {
      const auto site = (i + endpoint) % 4;
      const Vec2 chosen{static_cast<double>(site == 1 || site == 2),
                        static_cast<double>(site == 2 || site == 3)};
      const MeshLocator locator{MeshLocatorKind::direct_original_uv,
                                chosen,
                                {chosen[0], chosen[0], chosen[1], chosen[1]},
                                {},
                                {},
                                {},
                                0};
      d.use_samples.push_back(
          {{i + 1}, {i * 2 + endpoint + 1}, {site + 1}, endpoint ? o : z, locator, locator});
      d.use_sample_refs.push_back({i * 2 + endpoint + 1});
    }
  }
  d.loops.push_back({s.loops[0].id, {1}, true, 1, {0, 4}, {0, 4}, {}});
  const bool all_reconstructed = std::all_of(s.coedges.begin(), s.coedges.end(),
                                             [](const auto &use) { return use.reconstructed; });
  if (!all_reconstructed) {
    d.faces[0].domain_map = {1};
    d.domain_maps.push_back({{1}, MeshReferencePolicy::authoritative_original, {}, {}});
  }
  const std::array<std::array<uint64_t, 3>, 2> triangles{{{1, 2, 3}, {1, 3, 4}}};
  for (uint64_t i = 0; i < 2; ++i) {
    const auto &t = triangles[i];
    const std::array<MSite, 3> sites{{{t[0]}, {t[1]}, {t[2]}}};
    d.lineage.push_back({{1}, 0, MeshLineageOperation::initial, {}, {4 + i * 3, 3}});
    for (const auto site : sites)
      d.site_refs.push_back(site);
    d.cells.push_back({{1}, {1}, {i + 1}, {i + 1}, {i, 1}, {}});
    d.cell_maps.push_back({{i + 1}, UVTriangleMap{sites}});
    d.cell_facets.push_back({{i + 1}, {i + 1}, MeshInterpolant::barycentric_triangle});
    d.cell_facet_refs.push_back({i + 1});
    for (const auto site : sites) {
      const auto point = d.use_samples[(site.value - 1) * 2].recovery;
      d.corners.push_back({{1}, site, s.carriers[0].id, 1, point});
    }
    d.facets.push_back(
        {{1}, {{{t[0]}, {t[1]}, {t[2]}}}, {{{i * 3 + 1}, {i * 3 + 2}, {i * 3 + 3}}}, {1}});
    d.facet_refs.push_back({i + 1});
  }
  d.components.push_back(
      {MDef{1}, MeshComponentKind::definition_positional, MeshValueRole::allowance, two, {}});
  d.components.push_back(
      {MDef{1}, MeshComponentKind::definition_node_source, MeshValueRole::allowance, two, {}});
  d.usage_paths.push_back(s.usages[0].id);
  d.instances.push_back({{0, 1}, s.products[1].id, {1}, s.usages[0].transform, {0, 0, 0}, {1}});
  d.appearances.push_back({{0}, {.2, .4, .8}, .5, s.products[1].id});
  if (options.source_maps) {
    require(!all_reconstructed, "extended fixture requires authoritative original uses");
    // The warped parametrization is still the physical unit XY plane. This
    // projection chart and geometric-line profile describe the actual source.
    d.charts[0].chart =
        PhysicalPlaneChart{{z, o, z, o}, {o, z, z, z, o, z}, {o, z, z, o},
                           {z, z, z},    {o, z, z, z, o, z}, {z, z, o, z, o, o, z, o}};
    for (auto &map : d.cell_maps) {
      const auto sites = std::get<UVTriangleMap>(map.map).sites;
      map.map = PlaneTriangleMap{sites, {1}};
    }
    for (auto &sample : d.use_samples) {
      for (auto *locator : {&sample.raw, &sample.recovery}) {
        locator->kind = MeshLocatorKind::projection_inverse;
        locator->chart = {1};
      }
    }
    for (auto &corner : d.corners) {
      corner.uv.kind = MeshLocatorKind::projection_inverse;
      corner.uv.chart = {1};
    }
    ExactBuilder support_values(run, {{OwnerKind::run, 0, 777}, 1, 0});
    const auto eighth = support_values.append(Rational(b, 1).divide(Rational(b, 8))),
               negative = support_values.append(Rational(b, -1).divide(Rational(b, 8)));
    d.exact_tables.push_back(support_values.seal());
    d.points = {MeshPoint2{negative, negative}, MeshPoint2{eighth, negative},
                MeshPoint2{eighth, eighth}, MeshPoint2{negative, eighth}};
    // A=Q=(0,0), hence both retained maps are actual identity maps. Their
    // support records still exercise source-point/projected/map composition.
    d.supports.push_back({{1}, NodalFanSupport{ProjectedPcurvePoint{{1}, {1}, z}, {1}, {0, 4}}});
    d.supports.push_back(
        {{1},
         GraphStripSupport{
             {1}, {z, o}, {2, 1}, {{{1}, {2}}}, {z, z}, {o, z}, {z, o}, o, eighth, eighth}});
    d.support_refs = {{1}, {2}, {1}};
    d.domain_maps[0].fans = {0, 1};
    d.domain_maps[0].strips = {1, 1};
    d.profiles.push_back({{1},
                          {1},
                          s.edges[0].curve_id,
                          s.carriers[0].id,
                          {z, o},
                          GeometricLineProfile{{o, z, z}, {z, o}, {{{1}, {2}}}, 1},
                          {0, 1}});
    d.profile_refs.push_back({1});
    d.uses[0].profiles = {0, 1};
    d.uses[0].components = {0, 1};
    d.components.push_back({MUse{1},
                            MeshComponentKind::trace_position_residual,
                            MeshValueRole::canonical_upper,
                            z,
                            {1}});
    d.component_refs.push_back({3});
    d.use_refs.push_back({1});
    d.use_refs.push_back({4});
    d.joints.push_back(
        {{1}, s.vertices[0].id, {1}, 0, {1}, {4, 2}, d.use_samples[0].recovery, {1}});
    d.sites[0].branch = MJoint{1};
    d.sites[1].branch = MUse{1};
    d.source_vertex_refs.push_back(s.vertices[0].id);
    d.quotients.push_back({{1},
                           s.carriers[0].id,
                           MeshQuotientKind::joint,
                           {0, 1},
                           {4, 2},
                           {0, 1},
                           {1},
                           {0, 2},
                           MeshPole::none});
    d.nodes[0].generator = QuotientGenerator{{1}};
    d.nodes[0].quotient = {1};
    d.closures.push_back({{1},
                          s.loops[0].id,
                          s.vertices[0].id,
                          {4},
                          {1},
                          {{{1}, {1}}},
                          MeshClosureKind::joint,
                          {1},
                          {}});
    d.closure_refs.push_back({1});
    d.loops[0].closures = {0, 1};
  }
  return options.validation ? builder.seal(*options.validation) : builder.seal();
}
std::vector<char> bytes(const std::filesystem::path &path) {
  std::ifstream f(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(f), {}};
}
uint64_t word(const std::vector<char> &data, std::size_t offset) {
  require(offset <= data.size() && 8 <= data.size() - offset, "wire word outside fixture");
  uint64_t value = 0;
  for (unsigned i = 0; i < 8; ++i)
    value |= uint64_t(static_cast<unsigned char>(data[offset + i])) << (8 * i);
  return value;
}
struct WireMutation final {
  std::size_t offset;
  uint64_t value;
  unsigned count = 8;
};
void put(std::vector<char> &data, WireMutation change) {
  require(change.offset <= data.size() && change.count <= data.size() - change.offset,
          "wire mutation outside fixture");
  for (unsigned i = 0; i < change.count; ++i)
    data[change.offset + i] =
        std::bit_cast<char>(static_cast<unsigned char>((change.value >> (8 * i)) & 255));
}
void save(const std::filesystem::path &path, const std::vector<char> &data) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out.write(data.data(), static_cast<std::streamsize>(data.size()));
  require(bool(out), "fixture write failed");
}
void malformed(const std::shared_ptr<RunResource> &run, const std::filesystem::path &directory,
               const std::vector<char> &original) {
  std::array<std::size_t, 29> headers{}, payloads{};
  std::size_t offset = 96;
  for (std::size_t i = 0; i < 29; ++i) {
    headers[i] = offset;
    payloads[i] = offset + 32;
    offset += 32 + static_cast<std::size_t>(word(original, offset + 16));
  }
  require(offset == original.size(), "literal29section extent");
  const auto path = directory / "malformed.bgnmesh";
  const auto refuse = [&](const std::vector<char> &data) {
    save(path, data);
    const auto before = run->live();
    bool failed = false;
    try {
      static_cast<void>(load_native_mesh(path, run));
    } catch (const Error &) {
      failed = true;
    }
    require(failed, "malformed mesh was accepted");
    require(run->live() == before, "rejected wire retained native owners");
  };
  for (auto item : std::array<std::pair<std::size_t, uint64_t>, 8>{{{8, 2},
                                                                    {12, 1},
                                                                    {16, 0},
                                                                    {24, 28},
                                                                    {28, 1},
                                                                    {96, 0},
                                                                    {headers[3] + 8, UINT64_MAX},
                                                                    {headers[18] + 24, 1}}}) {
    auto data = original;
    put(data, {item.first, item.second,
               item.first == 16 || item.first == 96 || item.first == headers[3] + 8 ? 8u : 4u});
    refuse(data);
  }
  for (auto position : std::array<std::size_t, 3>{32, 64, payloads[0] + 16}) {
    auto data = original;
    data[position] ^= 1;
    refuse(data);
  }
  {
    auto data = original;
    put(data, {payloads[1] + 16, 0, 4});
    refuse(data);
  }
  {
    auto data = original;
    put(data, {payloads[1] + 24, UINT64_MAX});
    refuse(data);
  }
  {
    auto data = original;
    put(data, {payloads[2], 0, 4});
    refuse(data);
  }
  {
    auto data = original;
    put(data, {payloads[3] + 24, 0});
    refuse(data);
  }
  {
    auto data = original;
    put(data, {payloads[6] + 48, UINT32_MAX, 4});
    refuse(data);
  }
  {
    auto data = original;
    put(data, {payloads[7] + 24, 0, 4});
    refuse(data);
  }
  {
    auto data = original;
    put(data, {payloads[19] + 16, 99, 4});
    refuse(data);
  }
  {
    auto data = original;
    put(data, {payloads[26] + 16, 2, 4});
    refuse(data);
  }
  {
    auto data = original;
    put(data,
        {payloads[5] + 8 + 8, std::bit_cast<uint64_t>(std::numeric_limits<double>::infinity())});
    refuse(data);
  }
  for (auto count : std::array<std::size_t, 4>{0, 95, 127, original.size() - 1}) {
    auto data = original;
    data.resize(count);
    refuse(data);
  }
  {
    auto data = original;
    data.push_back(0);
    put(data, {16, data.size()});
    refuse(data);
  }
  {
    auto data = original;
    put(data, {payloads[3], word(data, payloads[3]) + 1});
    refuse(data);
  }
  std::filesystem::remove(path);
}
void digest_vectors() {
  const std::array<std::pair<std::string_view, std::string_view>, 2> cases{
      {{"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
       {"abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"}}};
  constexpr std::string_view digits = "0123456789abcdef";
  for (const auto &[input, expected] : cases) {
    const auto digest = snapshot_digest(std::as_bytes(std::span(input.data(), input.size())));
    std::string actual;
    for (auto value : digest) {
      const auto n = std::to_integer<unsigned>(value);
      actual.push_back(digits[n >> 4]);
      actual.push_back(digits[n & 15]);
    }
    require(actual == expected, "independent literal SHA256 vector");
  }
}
void resource_failures(const std::shared_ptr<RunResource> &run, const NativeMesh &mesh,
                       const std::filesystem::path &path) {
  const auto baseline = run->live();
  const auto wire = bytes(path);
  uint64_t wire_rows = 0;
  std::size_t offset = 96;
  for (unsigned i = 0; i < 29; ++i) {
    wire_rows += word(wire, offset + 8);
    offset += 32 + static_cast<std::size_t>(word(wire, offset + 16));
  }
  // The independently authored base graph has49 inline list items. This cap
  // admits rows and lists separately but refuses their required aggregate.
  for (const auto limits :
       std::array<MeshIOLimits, 4>{{{96, 100000000, 1ULL << 40},
                                    {1ULL << 30, 1, 1ULL << 40},
                                    {1ULL << 30, 100000000, 0},
                                    {1ULL << 30, wire_rows + 48, 1ULL << 40}}}) {
    bool failed = false;
    try {
      static_cast<void>(load_native_mesh(path, run, limits));
    } catch (const Error &e) {
      failed = e.code == ErrorCode::resource_exhausted;
    }
    require(failed, "reader did not enforce typed byte/row/work limits");
    failed = false;
    const auto output = path.parent_path() / "limited.bgnmesh";
    try {
      write_native_mesh(output, mesh, limits);
    } catch (const Error &e) {
      failed = e.code == ErrorCode::resource_exhausted;
    }
    require(failed, "writer did not enforce typed byte/row/work limits");
    std::filesystem::remove(output);
    require(run->live() == baseline, "resource refusal retained codec storage");
  }
  bool failed = false;
  try {
    static_cast<void>(load_native_mesh("nonexistent-codec-input", {}));
  } catch (const Error &e) {
    failed = e.code == ErrorCode::internal_error;
  }
  require(failed, "null codec resource reached filesystem/allocator");
  bgfd::Budget tiny({1ULL << 24, 0}, run.get());
  failed = false;
  try {
    static_cast<void>(fixture(run, mesh.view().source, {.validation = &tiny}));
  } catch (const Error &e) {
    failed = e.code == ErrorCode::resource_exhausted;
  }
  require(failed, "mesh constructor escaped supplied validation work limit");
  require(run->live() == baseline, "failed bounded constructor retained ownership");
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 3, "expected original source and output directory");
    const auto run = std::make_shared<RunResource>(1ULL << 28);
    const auto source = load_source(argv[1], run);
    const std::filesystem::path directory(argv[2]);
    std::filesystem::create_directories(directory);
    const auto a = directory / "a.bgnmesh", c = directory / "c.bgnmesh";
    write_native_mesh(a, fixture(run, source));
    write_native_mesh(c, fixture(run, source, {.alternative_storage = true}));
    const auto original = bytes(a);
    require(original == bytes(c), "unused exact cache perturbed semantic bytes");
    require(original.size() > 96 && std::string_view(original.data(), 8) == "BGNMESH1",
            "literal mesh wire header");
    const auto loaded = load_native_mesh(a, run);
    require(loaded.view().source.view().original_bytes == source.view().original_bytes,
            "original source bytes changed");
    require(loaded.view().faces.size() == 1 && loaded.view().facets.size() == 2 &&
                loaded.view().uses.size() == 4,
            "full source/cell/use inventory lost in roundtrip");
    write_native_mesh(c, loaded);
    require(original == bytes(c), "wire readback changed semantic bytes");
    resource_failures(run, loaded, a);
    malformed(run, directory, original);
    digest_vectors();
    const auto authoritative = load_source(
        std::filesystem::path(argv[1]).parent_path() / "authoritative-joint.bgsource", run);
    const auto maps = directory / "maps.bgnmesh";
    write_native_mesh(maps, fixture(run, authoritative, {.source_maps = true}));
    const auto map_bytes = bytes(maps);
    const auto restored = load_native_mesh(maps, run);
    require(restored.view().supports.size() == 2 && restored.view().profiles.size() == 1 &&
                restored.view().joints.size() == 1 && restored.view().quotients.size() == 1 &&
                restored.view().closures.size() == 1,
            "nonempty source map/profile/quotient rows were lost");
    write_native_mesh(c, restored);
    require(bytes(c) == map_bytes, "finite domain/source maps changed in roundtrip");
    std::cout << "PASS\tfull semantic native mesh wire roundtrip\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
