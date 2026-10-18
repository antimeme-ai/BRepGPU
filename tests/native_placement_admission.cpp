// One placement admission rule: every corpus matrix must be admitted or
// refused identically by the native source reader and the CPU display's
// source-profile and legacy-profile scene factories.
// tests/test_step_ingress.py checks the Python reader,
// tests/test_placement_oracles.py the Python assembly, and
// tests/native_reference_placement_admission.cpp the raw reference display
// against the same expected column.
#include "../native/om10/types.hpp"
#include "source.hpp"
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using Bytes = std::vector<std::byte>;
using Matrix = std::array<double, 12>;
void require(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}
void word(Bytes &out, uint64_t value, uint32_t width = 8) {
  for (uint32_t i = 0; i < width; ++i)
    out.push_back(static_cast<std::byte>((value >> (8 * i)) & 255U));
}
void text(Bytes &out, std::string_view value) {
  word(out, value.size(), 4);
  for (char c : value)
    out.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
}
void number(Bytes &out, double value) { word(out, std::bit_cast<uint64_t>(value)); }
void row(Bytes &out, const Bytes &value) {
  word(out, value.size());
  out.insert(out.end(), value.begin(), value.end());
}
// Root product 100 places product 101 once through usage 99.
Bytes source_with_usage(const Matrix &matrix) {
  std::array<Bytes, 14> payloads;
  std::array<uint64_t, 14> counts{};
  Bytes value;
  text(value, "placement admission probe");
  row(payloads[0], value);
  counts[0] = 1;
  for (uint64_t id : {uint64_t{100}, uint64_t{101}}) {
    value.clear();
    word(value, id);
    word(value, 0);
    word(value, 0);
    text(value, id == 100 ? "root" : "child");
    text(value, "probe");
    row(payloads[1], value);
  }
  counts[1] = 2;
  value.clear();
  word(value, 99);
  word(value, 100);
  word(value, 101);
  word(value, 0);
  text(value, "probe usage");
  for (double x : matrix)
    number(value, x);
  row(payloads[3], value);
  counts[3] = 1;
  constexpr std::array<std::string_view, 14> tags{
      "META____", "PRODUCTS", "SOLIDS__", "USAGES__", "VERTICES", "EDGES___", "COEDGES_",
      "LOOPS___", "FACES___", "CARRIERS", "CURVES__", "STYLES__", "DIAGS___", "OCCSTYLE"};
  Bytes out;
  for (char c : std::string_view("BGSRC001"))
    out.push_back(static_cast<std::byte>(c));
  word(out, 2, 4);
  word(out, 0, 4);
  for (unsigned i = 0; i < 32; ++i)
    out.push_back(static_cast<std::byte>(i));
  number(out, 1);
  number(out, 0);
  word(out, 100);
  word(out, 14, 4);
  word(out, 0, 4);
  uint64_t size = 88;
  for (const auto &payload : payloads)
    size += 24 + payload.size();
  word(out, size);
  for (std::size_t i = 0; i < tags.size(); ++i) {
    for (char c : tags[i])
      out.push_back(static_cast<std::byte>(c));
    word(out, counts[i]);
    word(out, payloads[i].size());
    out.insert(out.end(), payloads[i].begin(), payloads[i].end());
  }
  return out;
}
std::string reader_status(const Matrix &matrix) {
  auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 24);
  try {
    static_cast<void>(bgmesh::read_source(source_with_usage(matrix), resource));
  } catch (const bgmesh::Error &error) {
    const std::string_view message(error.what());
    if (message.find("nonrigid original source placement") != std::string_view::npos)
      return "nonorthonormal";
    if (message.find("reflected/singular source placement") != std::string_view::npos)
      return "improper";
    return "other:" + std::string(message);
  }
  return "rigid";
}
// One planar triangle placed once; only the instance matrix varies. A refusal
// counts only when it is the display's placement rule ("nonrigid instance
// matrix"); any other failure is reported as such. The source profile stores
// XYZ with cofactor normals; the legacy profile has plain vertex flags.
std::string display_status(const Matrix &matrix, bool source_profile) {
  using namespace bgom10;
  Carrier carrier{};
  carrier.id = 1;
  carrier.tag = 1;
  carrier.x[0] = carrier.y[1] = carrier.z[2] = 1;
  carrier.uv[0] = carrier.uv[2] = -1;
  carrier.uv[1] = carrier.uv[3] = 1;
  Definition definition{1, 0, 3, 0, 1};
  Instance instance{};
  instance.id = 1;
  for (std::size_t i = 0; i < matrix.size(); ++i)
    instance.matrix[i] = matrix[i];
  Vertex vertices[3]{};
  const double xy[3][2] = {{-1., -1.}, {1., -1.}, {-1., 1.}};
  for (uint32_t i = 0; i < 3; ++i) {
    vertices[i].xyz[0] = vertices[i].uv[0] = xy[i][0];
    vertices[i].xyz[1] = vertices[i].uv[1] = xy[i][1];
    vertices[i].flags = source_profile ? STORED_XYZ | COFACTOR_NORMAL : 0U;
    vertices[i].physical = i + 1;
  }
  Triangle triangle{{0, 1, 2}, 0, 1};
  Material material{{.8, .8, .8}, .5};
  VertexRef refs[3]{{0, 0}, {1, 0}, {2, 0}};
  PrimitiveRef primitive{0, 0, 0, 0};
  SceneView scene{&definition, 1, &instance, 1, vertices, 3, &triangle,  1, &carrier, 1,
                  &material,   1, nullptr,   0, refs,     3, &primitive, 1};
  Error error{};
  Times times{};
  void *token = nullptr;
  const auto *api = source_profile ? cpu_api_v2() : cpu_api();
  require(api && api->version == (source_profile ? 2U : 1U), "display API version");
  const bool admitted = api->create(&scene, 1, 0, &token, &error, &times);
  require(admitted == (token != nullptr), "display admission and session token disagree");
  if (token)
    require(api->close(token, &error, &times), "display probe close");
  if (admitted)
    return "admitted";
  const std::string_view message(error.message);
  return message.find("nonrigid instance matrix") != std::string_view::npos
             ? "refused"
             : "other:" + std::string(message);
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "usage: brepgpu_native_placement_admission CORPUS.tsv");
    std::ifstream input(argv[1]);
    require(static_cast<bool>(input), "cannot read placement corpus");
    std::string line;
    uint64_t rows = 0, mismatches = 0;
    while (std::getline(input, line)) {
      if (line.empty() || line[0] == '#')
        continue;
      std::istringstream fields(line);
      std::string name, expected, item;
      require(static_cast<bool>(std::getline(fields, name, '\t')) &&
                  static_cast<bool>(std::getline(fields, expected, '\t')),
              "placement corpus row name/status");
      Matrix matrix{};
      for (auto &x : matrix) {
        require(static_cast<bool>(std::getline(fields, item, '\t')), "placement corpus width");
        char *end = nullptr;
        x = std::strtod(item.c_str(), &end);
        require(end && *end == '\0', "placement corpus scalar " + item);
      }
      require(!std::getline(fields, item, '\t'), "placement corpus trailing field");
      const auto reader = reader_status(matrix);
      const auto display = display_status(matrix, true);
      const auto legacy = display_status(matrix, false);
      const std::string wanted = expected == "rigid" ? "admitted" : "refused";
      const bool agree = reader == expected && display == wanted && legacy == wanted;
      std::cout << name << '\t' << expected << "\treader=" << reader << "\tdisplay=" << display
                << "\tlegacy=" << legacy << '\t' << (agree ? "AGREE" : "DISAGREE") << '\n';
      ++rows;
      mismatches += agree ? 0 : 1;
    }
    require(rows > 0, "empty placement corpus");
    require(mismatches == 0, std::to_string(mismatches) + " placement admission disagreements");
    std::cout << "PASS\tone rigid placement rule over " << rows << " corpus matrices\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
