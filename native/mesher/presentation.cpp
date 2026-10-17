#include "presentation.hpp"
#include "../om10/validation.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <numeric>

namespace bgmesh {
namespace {
[[noreturn]] void invalid(const char *s) { throw Error(ErrorCode::invalid_source, s); }
using V = Vec3;
V add(const V &a, const V &b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
V sub(const V &a, const V &b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
V mul(const V &a, double b) { return {a[0] * b, a[1] * b, a[2] * b}; }
double dot(const V &a, const V &b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
V cross(const V &a, const V &b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
V unit(const V &a) {
  const double n = std::hypot(a[0], a[1], a[2]);
  if (!std::isfinite(n) || n == 0)
    invalid("presentation basis is not representable");
  return mul(a, 1 / n);
}
V transform(const bgom10::Instance &i, const V &p) {
  V out{};
  for (std::size_t a = 0; a < 3; ++a)
    out[a] = i.matrix[4 * a] * p[0] + i.matrix[4 * a + 1] * p[1] + i.matrix[4 * a + 2] * p[2] +
             i.matrix[4 * a + 3];
  return out;
}
struct Box final {
  V lo{INFINITY, INFINITY, INFINITY}, hi{-INFINITY, -INFINITY, -INFINITY};
  void include(const V &p) {
    for (std::size_t a = 0; a < 3; ++a) {
      if (!std::isfinite(p[a]))
        invalid("presentation point is nonfinite");
      lo[a] = std::min(lo[a], p[a]);
      hi[a] = std::max(hi[a], p[a]);
    }
  }
  V center() const { return add(mul(lo, .5), mul(hi, .5)); }
  V corner(unsigned k) const {
    return {(k & 1U) ? hi[0] : lo[0], (k & 2U) ? hi[1] : lo[1], (k & 4U) ? hi[2] : lo[2]};
  }
};
using Color = std::array<uint8_t, 3>;
struct Rectangle final {
  int x, y, width, height;
};
class Canvas final {
  std::span<uint8_t> pixels;
  int width, height;

public:
  Canvas(std::span<uint8_t> p, const bgom10::Request &r)
      : pixels(p), width(static_cast<int>(r.width)), height(static_cast<int>(r.height)) {}
  void pixel(int x, int y, Color c) {
    if (x < 0 || y < 0 || x >= width || y >= height)
      return;
    const auto at = 4 * (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                         static_cast<std::size_t>(x));
    for (std::size_t a = 0; a < 3; ++a)
      pixels[at + a] = c[a];
    pixels[at + 3] = 255;
  }
  void rectangle(Rectangle r, Color c) {
    for (int j = 0; j < r.height; ++j)
      for (int i = 0; i < r.width; ++i)
        pixel(r.x + i, r.y + j, c);
  }
  void line(int x, int y, int xx, int yy, Color c) {
    const int dx = std::abs(xx - x), sx = x < xx ? 1 : -1, dy = -std::abs(yy - y),
              sy = y < yy ? 1 : -1;
    int error = dx + dy;
    for (;;) {
      pixel(x, y, c);
      if (x == xx && y == yy)
        break;
      const int twice = 2 * error;
      if (twice >= dy) {
        error += dy;
        x += sx;
      }
      if (twice <= dx) {
        error += dx;
        y += sy;
      }
    }
  }
  void text(int x, int y, std::string_view s, int scale, Color c) {
    // Owned five-column uppercase/digit strokes; cosmetic labels only.
    static constexpr std::array<std::array<uint8_t, 7>, 36> glyph{
        {{{14, 17, 17, 31, 17, 17, 17}}, {{30, 17, 17, 30, 17, 17, 30}},
         {{14, 17, 16, 16, 16, 17, 14}}, {{30, 17, 17, 17, 17, 17, 30}},
         {{31, 16, 16, 30, 16, 16, 31}}, {{31, 16, 16, 30, 16, 16, 16}},
         {{14, 17, 16, 23, 17, 17, 14}}, {{17, 17, 17, 31, 17, 17, 17}},
         {{14, 4, 4, 4, 4, 4, 14}},      {{7, 2, 2, 2, 2, 18, 12}},
         {{17, 18, 20, 24, 20, 18, 17}}, {{16, 16, 16, 16, 16, 16, 31}},
         {{17, 27, 21, 21, 17, 17, 17}}, {{17, 25, 25, 21, 19, 19, 17}},
         {{14, 17, 17, 17, 17, 17, 14}}, {{30, 17, 17, 30, 16, 16, 16}},
         {{14, 17, 17, 17, 21, 18, 13}}, {{30, 17, 17, 30, 20, 18, 17}},
         {{15, 16, 16, 14, 1, 1, 30}},   {{31, 4, 4, 4, 4, 4, 4}},
         {{17, 17, 17, 17, 17, 17, 14}}, {{17, 17, 17, 17, 17, 10, 4}},
         {{17, 17, 17, 21, 21, 21, 10}}, {{17, 17, 10, 4, 10, 17, 17}},
         {{17, 17, 10, 4, 4, 4, 4}},     {{31, 1, 2, 4, 8, 16, 31}},
         {{14, 17, 19, 21, 25, 17, 14}}, {{4, 12, 4, 4, 4, 4, 14}},
         {{14, 17, 1, 2, 4, 8, 31}},     {{30, 1, 1, 14, 1, 1, 30}},
         {{2, 6, 10, 18, 31, 2, 2}},     {{31, 16, 16, 30, 1, 1, 30}},
         {{14, 16, 16, 30, 17, 17, 14}}, {{31, 1, 2, 4, 8, 8, 8}},
         {{14, 17, 17, 14, 17, 17, 14}}, {{14, 17, 17, 15, 1, 1, 14}}}};
    for (char ch : s) {
      if (x >= width)
        break;
      if (ch >= 'a' && ch <= 'z')
        ch = static_cast<char>(ch - 'a' + 'A');
      int index = -1;
      if (ch >= 'A' && ch <= 'Z')
        index = ch - 'A';
      else if (ch >= '0' && ch <= '9')
        index = 26 + ch - '0';
      if (index >= 0)
        for (int row = 0; row < 7; ++row)
          for (int col = 0; col < 5; ++col)
            if ((glyph[static_cast<std::size_t>(index)][static_cast<std::size_t>(row)] &
                 (1U << static_cast<unsigned>(4 - col))) != 0)
              rectangle({x + col * scale, y + row * scale, scale, scale}, c);
      if (ch == '.')
        rectangle({x + 2 * scale, y + 6 * scale, scale, scale}, c);
      if (ch == '-')
        rectangle({x, y + 3 * scale, 5 * scale, scale}, c);
      if (ch == ':') {
        rectangle({x + 2 * scale, y + 2 * scale, scale, scale}, c);
        rectangle({x + 2 * scale, y + 5 * scale, scale, scale}, c);
      }
      if (ch == '/')
        line(x + 4 * scale, y, x, y + 6 * scale, c);
      x += 6 * scale;
    }
  }
};
uint32_t crc_byte(uint32_t crc, uint8_t b) {
  crc ^= b;
  for (unsigned j = 0; j < 8; ++j)
    crc = (crc >> 1) ^ ((0U - (crc & 1U)) & 0xedb88320U);
  return crc;
}
class Png final {
  std::ofstream file;
  uint32_t crc = 0;
  void byte(uint8_t b) { file.put(static_cast<char>(b)); }

public:
  explicit Png(const std::filesystem::path &p) : file(p, std::ios::binary) {
    if (!file)
      throw Error(ErrorCode::io_failure, "cannot open PNG output");
    constexpr std::array<uint8_t, 8> signature{137, 80, 78, 71, 13, 10, 26, 10};
    for (auto b : signature)
      byte(b);
  }
  void word(uint32_t v) {
    for (int shift = 24; shift >= 0; shift -= 8)
      byte(static_cast<uint8_t>(v >> static_cast<unsigned>(shift)));
  }
  void begin(uint32_t n, const char *kind) {
    word(n);
    crc = UINT32_MAX;
    for (unsigned i = 0; i < 4; ++i)
      data(static_cast<uint8_t>(kind[i]));
  }
  void data(uint8_t b) {
    byte(b);
    crc = crc_byte(crc, b);
  }
  void number(uint32_t v) {
    for (int shift = 24; shift >= 0; shift -= 8)
      data(static_cast<uint8_t>(v >> static_cast<unsigned>(shift)));
  }
  void end() { word(crc ^ UINT32_MAX); }
  void close() {
    file.close();
    if (!file)
      throw Error(ErrorCode::io_failure, "PNG output failed");
  }
};
} // namespace
NativePresentation make_presentation(bgfd::Budget &budget, const NativeScene &scene, uint32_t width,
                                     uint32_t height, double strength) {
  if (!scene)
    invalid("native presentation scene is absent");
  return make_presentation(budget, scene->view(), width, height, strength);
}
NativePresentation make_presentation(bgfd::Budget &budget, const bgom10::SceneView &view,
                                     uint32_t width, uint32_t height, double strength) {
  if (!width || !height || width > 8192 || height > 8192 || !std::isfinite(strength) ||
      strength < 0 || strength > 100)
    invalid("invalid native presentation dimensions/explosion");
  NativePresentation out(&budget);
  bgom10::validate_scene_view(&view, true, {&budget, &budget, [](void *state, uint64_t count) {
                                              static_cast<bgfd::Budget *>(state)->charge(count);
                                            }});
  const auto definitions_view = bgom10::records(view.definitions, view.ndefinitions);
  const auto instances_view = bgom10::records(view.instances, view.ninstances);
  const auto vertices_view = bgom10::records(view.vertices, view.nvertices);
  if (definitions_view.size() == 0 || instances_view.size() == 0)
    invalid("presentation needs a placed assembly");
  std::pmr::vector<Box> definitions(&budget), placed(&budget);
  definitions.resize(definitions_view.size());
  for (std::size_t i = 0; i < definitions.size(); ++i) {
    const auto &d = definitions_view[i];
    if (d.vertex_count == 0)
      invalid("presentation definition has no stored vertices");
    for (uint64_t j = 0; j < d.vertex_count; ++j) {
      budget.charge(1);
      const auto &v = vertices_view[static_cast<std::size_t>(d.first_vertex + j)];
      definitions[i].include({v.xyz[0], v.xyz[1], v.xyz[2]});
    }
  }
  Box assembled;
  for (const auto &i : instances_view) {
    Box b;
    for (unsigned k = 0; k < 8; ++k) {
      budget.charge(1);
      b.include(transform(i, definitions[i.definition].corner(k)));
    }
    assembled.include(b.lo);
    assembled.include(b.hi);
    placed.push_back(b);
    out.centers.push_back(b.center());
  }
  const auto extent = sub(assembled.hi, assembled.lo), center = assembled.center();
  out.axial_direction =
      static_cast<uint32_t>(std::min_element(extent.begin(), extent.end()) - extent.begin());
  out.strength = strength;
  const std::size_t axis = out.axial_direction;
  V up{};
  up[axis] = 1;
  V tangent{};
  tangent[(axis + 1) % 3] = 1;
  V side = unit(cross(up, tangent));
  const V direction = unit(add(mul(up, 1.7), add(mul(tangent, .9), mul(side, .65))));
  const V right = unit(cross(tangent, direction)), vertical = unit(cross(direction, right));
  for (const auto &c : out.centers) {
    auto delta = mul(sub(c, center), strength * .2);
    delta[axis] = strength * (c[axis] - center[axis]);
    out.explosion.push_back(delta);
  }
  for (std::size_t frame = 0; frame < 2; ++frame) {
    auto &r = out.views[frame];
    r.width = width;
    r.height = height;
    r.supersample = 1;
    r.alpha = static_cast<double>(frame);
    for (std::size_t k = 0; k < 3; ++k) {
      r.basis[k] = right[k];
      r.basis[3 + k] = vertical[k];
      r.basis[6 + k] = direction[k];
      r.center[k] = center[k];
    }
    double xmin = INFINITY, xmax = -INFINITY, ymin = INFINITY, ymax = -INFINITY;
    for (std::size_t i = 0; i < placed.size(); ++i)
      for (unsigned k = 0; k < 8; ++k) {
        budget.charge(1);
        auto point = placed[i].corner(k);
        if (frame)
          point = add(point, out.explosion[i]);
        point = sub(point, center);
        const auto x = dot(right, point), y = dot(vertical, point);
        xmin = std::min(xmin, x);
        xmax = std::max(xmax, x);
        ymin = std::min(ymin, y);
        ymax = std::max(ymax, y);
      }
    const double available_x = static_cast<double>(width) * (frame ? .68 : .90),
                 available_y = static_cast<double>(height) * .80;
    const double dx = xmax - xmin, dy = ymax - ymin;
    if (!std::isfinite(dx) || !std::isfinite(dy) || dx <= 0 || dy <= 0)
      invalid("presentation assembly extent is not representable");
    r.scale = std::min(available_x / dx, available_y / dy);
    for (std::size_t k = 0; k < 3; ++k)
      r.center[k] += right[k] * (xmin * .5 + xmax * .5) + vertical[k] * (ymin * .5 + ymax * .5);
    r.pixel_center[0] = static_cast<double>(width) * .5;
    r.pixel_center[1] = static_cast<double>(height) * .54;
    r.background[0] = .008;
    r.background[1] = .014;
    r.background[2] = .026;
    if (!std::isfinite(r.scale) || r.scale <= 0)
      invalid("presentation scale is not representable");
  }
  return out;
}
void annotate_frame(bgfd::Budget &budget, std::span<uint8_t> rgba, const bgom10::Request &r,
                    const NativePresentation &p, bool exploded, FrameCaption caption) {
  if (r.width == 0 || r.height == 0 || r.width > 8192 || r.height > 8192 ||
      rgba.size() != uint64_t{r.width} * r.height * 4)
    invalid("annotation frame size mismatch");
  budget.charge(rgba.size());
  Canvas canvas(rgba, r);
  const Color white{222, 233, 247}, cyan{70, 203, 229}, dark{20, 36, 53};
  const int scale = r.width >= 1000 ? 2 : 1;
  canvas.rectangle({0, 0, static_cast<int>(r.width), 65 * scale}, dark);
  canvas.text(18 * scale, 12 * scale, caption.heading, scale + 1, white);
  canvas.text(18 * scale, 34 * scale, caption.timing, scale, cyan);
  if (!exploded)
    return;
  struct Label final {
    std::size_t instance;
    int x, y;
  };
  std::pmr::vector<Label> left(&budget), right(&budget);
  const V xaxis{r.basis[0], r.basis[1], r.basis[2]}, yaxis{r.basis[3], r.basis[4], r.basis[5]},
      center{r.center[0], r.center[1], r.center[2]};
  for (std::size_t i = 0; i < p.centers.size(); ++i) {
    const auto q = sub(add(p.centers[i], p.explosion[i]), center);
    const double x = r.pixel_center[0] + r.scale * dot(xaxis, q),
                 y = r.pixel_center[1] - r.scale * dot(yaxis, q);
    if (!std::isfinite(x) || !std::isfinite(y) || std::abs(x) > 1e7 || std::abs(y) > 1e7)
      invalid("label projection is not representable");
    Label label{i, static_cast<int>(std::round(x)), static_cast<int>(std::round(y))};
    (x < r.pixel_center[0] ? left : right).push_back(label);
  }
  // Balance dense columns without moving geometry or changing occurrence IDs.
  if (left.size() > right.size() + 1 || right.size() > left.size() + 1) {
    std::pmr::vector<Label> all(&budget);
    all.insert(all.end(), left.begin(), left.end());
    all.insert(all.end(), right.begin(), right.end());
    std::sort(all.begin(), all.end(),
              [](auto a, auto b) { return a.x != b.x ? a.x < b.x : a.instance < b.instance; });
    const auto mid = (all.size() + 1) / 2;
    left.assign(all.begin(), all.begin() + static_cast<std::ptrdiff_t>(mid));
    right.assign(all.begin() + static_cast<std::ptrdiff_t>(mid), all.end());
  }
  for (unsigned side = 0; side < 2; ++side) {
    auto &labels = side ? right : left;
    std::sort(labels.begin(), labels.end(),
              [](auto a, auto b) { return a.y != b.y ? a.y < b.y : a.instance < b.instance; });
    const int top = 140 * scale / 2, bottom = static_cast<int>(r.height) - 30 * scale;
    if (!labels.empty() &&
        (bottom <= top || static_cast<std::size_t>((bottom - top) / (8 * scale)) < labels.size()))
      throw Error(ErrorCode::resource_exhausted,
                  "image cannot hold all occurrence labels; increase height");
    for (std::size_t j = 0; j < labels.size(); ++j) {
      const auto &v = labels[j];
      const int yy = top + static_cast<int>((j + 1) * static_cast<std::size_t>(bottom - top) /
                                            (labels.size() + 1));
      const int xx = side ? static_cast<int>(r.width) - 104 * scale : 16 * scale,
                elbow = side ? static_cast<int>(r.width) - 116 * scale : 104 * scale;
      canvas.line(v.x, v.y, elbow, yy, dark);
      canvas.line(elbow, yy, side ? xx - 4 : xx + 72 * scale, yy, cyan);
      canvas.rectangle({v.x - 2, v.y - 2, 5, 5}, cyan);
      canvas.text(xx, yy - 3 * scale, "PART " + std::to_string(v.instance + 1), scale, white);
    }
  }
}
void write_png(const std::filesystem::path &path, uint32_t width, uint32_t height,
               std::span<const uint8_t> rgba, bgfd::Budget &budget) {
  if (!width || !height || width > 8192 || height > 8192 ||
      rgba.size() != uint64_t{width} * height * 4)
    invalid("PNG dimensions/data mismatch");
  const uint64_t row = uint64_t{width} * 3 + 1, raw = row * height, blocks = (raw + 65534) / 65535;
  if (raw + blocks * 5 + 6 > UINT32_MAX)
    throw Error(ErrorCode::resource_exhausted, "PNG size overflow");
  budget.charge(raw);
  Png file(path);
  file.begin(13, "IHDR");
  file.number(width);
  file.number(height);
  for (uint8_t b : std::array<uint8_t, 5>{8, 2, 0, 0, 0})
    file.data(b);
  file.end();
  file.begin(static_cast<uint32_t>(raw + blocks * 5 + 6), "IDAT");
  file.data(0x78);
  file.data(0x01);
  uint32_t adler_a = 1, adler_b = 0;
  for (uint64_t start = 0; start < raw;) {
    const auto n = static_cast<uint16_t>(std::min(uint64_t{65535}, raw - start));
    file.data(start + n == raw ? 1 : 0);
    file.data(static_cast<uint8_t>(n));
    file.data(static_cast<uint8_t>(n >> 8));
    const auto inverse = static_cast<uint16_t>(~n);
    file.data(static_cast<uint8_t>(inverse));
    file.data(static_cast<uint8_t>(inverse >> 8));
    for (uint64_t i = 0; i < n; ++i) {
      const auto offset = start + i, y = offset / row, x = offset % row;
      const uint8_t b =
          x == 0 ? uint8_t{0}
                 : rgba[static_cast<std::size_t>((y * width + (x - 1) / 3) * 4 + (x - 1) % 3)];
      file.data(b);
      adler_a = (adler_a + b) % 65521;
      adler_b = (adler_b + adler_a) % 65521;
    }
    start += n;
  }
  file.number((adler_b << 16) | adler_a);
  file.end();
  file.begin(0, "IEND");
  file.end();
  file.close();
}
} // namespace bgmesh
