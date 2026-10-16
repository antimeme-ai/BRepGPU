#include "core.hpp"
#include "core_detail.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <limits>
#include <utility>

namespace bgcdt {
namespace {
using Clock = std::chrono::steady_clock;
[[nodiscard]] uint64_t elapsed(Clock::time_point start) {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count());
}
constexpr Slot none = std::numeric_limits<Slot>::max();
[[noreturn]] void invariant(const char *message) {
  throw bgfd::Failure(BGFD_INTERNAL_ERROR, message);
}
[[nodiscard]] Slot narrow(std::size_t value) {
  if (value >= none)
    throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "topology slot capacity exceeded");
  return static_cast<Slot>(value);
}
enum class Frame { xy, y_minus_x };
struct TriangleHandle final {
  Slot index;
};
struct CapParameters final {
  Slot apex;
  uint64_t seed;
};
class Geometry final {
  Predicates &p_;

public:
  explicit Geometry(Predicates &p) : p_(p) {}
  [[nodiscard]] int orient(Slot a, Slot b, Slot c) { return p_.orient(a, b, c); }
  [[nodiscard]] int circle(Slot a, Slot b, Slot c, Slot d) {
    if (a == b || a == c || a == d || b == c || b == d || c == d)
      return 0;
    const int exact = p_.incircle(a, b, c, d);
    if (exact != 0)
      return exact;
    const std::array<Slot, 4> sites{a, b, c, d};
    std::array<unsigned, 4> order{0, 1, 2, 3};
    std::sort(order.begin(), order.end(),
              [&](unsigned i, unsigned j) { return sites[i] < sites[j]; });
    for (unsigned selected : order) {
      std::array<Slot, 3> other{};
      unsigned out = 0;
      for (unsigned i = 0; i < 4; ++i)
        if (i != selected)
          other[out++] = sites[i];
      const int cofactor = orient(other[0], other[1], other[2]);
      if (cofactor != 0)
        return selected % 2 == 0 ? cofactor : -cofactor;
    }
    return 0;
  }
  [[nodiscard]] bool less(Slot a, Slot b, Frame frame) {
    const Axis primary = frame == Frame::xy ? Axis::x : Axis::y;
    const Axis secondary = frame == Frame::xy ? Axis::y : Axis::x;
    const int first = p_.compare_axis(a, b, primary);
    if (first != 0)
      return first < 0;
    const int second = p_.compare_axis(a, b, secondary);
    if (second != 0)
      return frame == Frame::xy ? second < 0 : second > 0;
    return a < b;
  }
};
class Random final {
  uint64_t state_;
  bgfd::Budget &budget_;

public:
  Random(uint64_t seed, bgfd::Budget &budget) : state_(seed), budget_(budget) {}
  [[nodiscard]] uint64_t next() {
    budget_.charge(1);
    state_ += UINT64_C(0x9e3779b97f4a7c15);
    uint64_t value = state_;
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
  }
  [[nodiscard]] std::size_t below(std::size_t bound) {
    if (bound == 0)
      invariant("empty random choice");
    const uint64_t count = static_cast<uint64_t>(bound);
    const uint64_t threshold = (uint64_t{0} - count) % count;
    uint64_t value = 0;
    do {
      value = next();
    } while (value < threshold);
    return static_cast<std::size_t>(value % count);
  }
  template <class T> void shuffle(std::pmr::vector<T> &values) {
    for (std::size_t i = values.size(); i > 1; --i)
      std::swap(values[i - 1], values[below(i)]);
  }
};
struct Quad final {
  std::array<Slot, 4> next{};
  Slot a = none, b = none;
  bool live = false;
};
class DivideConquer final {
  Geometry &g_;
  bgfd::Budget &budget_;
  CoreStats &stats_;
  std::pmr::vector<Quad> edges_;
  std::pmr::vector<Slot> free_, order_;
  uint64_t active_ = 0;
  [[nodiscard]] static Slot rot(Slot e) { return (e & ~Slot{3}) | ((e + 1) & Slot{3}); }
  [[nodiscard]] static Slot invrot(Slot e) { return (e & ~Slot{3}) | ((e + 3) & Slot{3}); }
  [[nodiscard]] static Slot sym(Slot e) {
    return e ^ Slot { 2 };
  }
  [[nodiscard]] Slot next(Slot e) const { return edges_[e / 4].next[e % 4]; }
  Slot &next_ref(Slot e) { return edges_[e / 4].next[e % 4]; }
  [[nodiscard]] Slot origin(Slot e) const {
    return (e & Slot{2}) == 0 ? edges_[e / 4].a : edges_[e / 4].b;
  }
  [[nodiscard]] Slot dest(Slot e) const { return origin(sym(e)); }
  [[nodiscard]] Slot left_next(Slot e) const { return rot(next(invrot(e))); }
  [[nodiscard]] Slot origin_prev(Slot e) const { return rot(next(rot(e))); }
  [[nodiscard]] bool left_of(Slot p, Slot e) { return g_.orient(origin(e), dest(e), p) > 0; }
  [[nodiscard]] bool right_of(Slot p, Slot e) { return g_.orient(origin(e), dest(e), p) < 0; }
  void splice(Slot a, Slot b) {
    budget_.charge(1);
    const Slot alpha = rot(next(a)), beta = rot(next(b));
    std::swap(next_ref(a), next_ref(b));
    std::swap(next_ref(alpha), next_ref(beta));
  }
  [[nodiscard]] Slot edge(Slot a, Slot b) {
    budget_.charge(1);
    Slot slot = 0;
    if (free_.empty()) {
      if (edges_.size() >= static_cast<std::size_t>(none / 4))
        throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "quad-edge handle capacity exceeded");
      slot = narrow(edges_.size());
      edges_.push_back({});
    } else {
      slot = free_.back();
      free_.pop_back();
    }
    const Slot base = slot * 4;
    edges_[slot] = Quad{{base, base + 3, base + 2, base + 1}, a, b, true};
    ++active_;
    stats_.quadedge_peak = std::max(stats_.quadedge_peak, active_);
    return base;
  }
  [[nodiscard]] Slot connect(Slot a, Slot b) {
    const Slot result = edge(dest(a), origin(b));
    splice(result, left_next(a));
    splice(sym(result), b);
    return result;
  }
  void erase(Slot e) {
    if (e / 4 >= edges_.size() || !edges_[e / 4].live)
      invariant("deleting dead quad-edge");
    splice(e, origin_prev(e));
    splice(sym(e), origin_prev(sym(e)));
    edges_[e / 4].live = false;
    free_.push_back(e / 4);
    --active_;
  }
  [[nodiscard]] std::pair<Slot, Slot> extrema(Slot hull, Frame frame) {
    Slot left = hull, right = sym(hull), current = hull;
    uint64_t walked = 0;
    do {
      if (++walked > 2 * active_ + 2)
        invariant("quad-edge exterior hull walk cycled");
      budget_.charge(1);
      if (g_.less(origin(current), origin(left), frame))
        left = current;
      if (g_.less(origin(right), dest(current), frame))
        right = sym(current);
      current = sym(left_next(sym(current)));
    } while (current != hull);
    return {left, right};
  }
  [[nodiscard]] std::pair<Slot, Slot> build(std::span<Slot> sites, Frame frame) {
    budget_.charge(1);
    if (sites.size() == 2) {
      if (g_.less(sites[1], sites[0], frame))
        std::swap(sites[0], sites[1]);
      const Slot e = edge(sites[0], sites[1]);
      return {e, sym(e)};
    }
    if (sites.size() == 3) {
      const auto ordering_start = Clock::now();
      std::sort(sites.begin(), sites.end(), [&](Slot a, Slot b) { return g_.less(a, b, frame); });
      stats_.order_ns += elapsed(ordering_start);
      const Slot a = edge(sites[0], sites[1]), b = edge(sites[1], sites[2]);
      splice(sym(a), b);
      const int turn = g_.orient(sites[0], sites[1], sites[2]);
      if (turn == 0)
        return {a, sym(b)};
      const Slot c = connect(b, a);
      return turn > 0 ? std::pair{a, sym(b)} : std::pair{sym(c), c};
    }
    const auto middle = sites.size() / 2;
    const auto ordering_start = Clock::now();
    std::nth_element(sites.begin(), sites.begin() + static_cast<std::ptrdiff_t>(middle),
                     sites.end(), [&](Slot a, Slot b) { return g_.less(a, b, frame); });
    stats_.order_ns += elapsed(ordering_start);
    const auto other_frame = frame == Frame::xy ? Frame::y_minus_x : Frame::xy;
    const auto left = build(sites.first(middle), other_frame);
    const auto right = build(sites.subspan(middle), other_frame);
    auto [ldo, ldi] = extrema(left.first, frame);
    auto [rdi, rdo] = extrema(right.first, frame);
    uint64_t tangent_steps = 0;
    const uint64_t tangent_limit = 2 * active_ + 2;
    for (;;) {
      if (++tangent_steps > tangent_limit)
        invariant("lower common tangent walk cycled");
      budget_.charge(1);
      ++stats_.merge_steps;
      if (left_of(origin(rdi), ldi))
        ldi = left_next(ldi);
      else if (right_of(origin(ldi), rdi))
        rdi = next(sym(rdi));
      else
        break;
    }
    Slot base = connect(sym(rdi), ldi);
    if (origin(ldi) == origin(ldo))
      ldo = sym(base);
    if (origin(rdi) == origin(rdo))
      rdo = base;
    uint64_t knitting_steps = 0;
    const uint64_t knitting_limit = 4 * (active_ + static_cast<uint64_t>(sites.size())) + 4;
    for (;;) {
      if (++knitting_steps > knitting_limit)
        invariant("Delaunay dividing chain walk cycled");
      budget_.charge(1);
      ++stats_.merge_steps;
      Slot lc = next(sym(base));
      if (right_of(dest(lc), base)) {
        for (;;) {
          const Slot upcoming = next(lc);
          if (g_.circle(dest(base), origin(base), dest(lc), dest(upcoming)) <= 0)
            break;
          erase(lc);
          lc = upcoming;
        }
      }
      Slot rc = origin_prev(base);
      if (right_of(dest(rc), base)) {
        for (;;) {
          const Slot upcoming = origin_prev(rc);
          if (g_.circle(dest(base), origin(base), dest(rc), dest(upcoming)) <= 0)
            break;
          erase(rc);
          rc = upcoming;
        }
      }
      const bool lv = right_of(dest(lc), base), rv = right_of(dest(rc), base);
      if (!lv && !rv)
        break;
      if (!lv || (rv && g_.circle(dest(lc), origin(lc), origin(rc), dest(rc)) > 0))
        base = connect(rc, sym(base));
      else
        base = connect(sym(base), sym(lc));
    }
    return {ldo, rdo};
  }

public:
  DivideConquer(Geometry &geometry, bgfd::Budget &budget, CoreStats &stats, std::size_t count)
      : g_(geometry), budget_(budget), stats_(stats), edges_(&budget), free_(&budget),
        order_(&budget) {
    order_.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
      order_.push_back(narrow(i));
  }
  [[nodiscard]] std::pmr::vector<TriangleWord> run() {
    if (order_.size() < 3)
      invariant("triangulation needs three distinct sites");
    const auto initial_start = Clock::now();
    static_cast<void>(build(order_, Frame::xy));
    stats_.initial_ns += elapsed(initial_start);
    const auto conversion_start = Clock::now();
    std::pmr::vector<TriangleWord> output(&budget_);
    budget_.charge(static_cast<uint64_t>(edges_.size()) * 4);
    std::pmr::vector<uint8_t> visited(edges_.size() * 4, 0, &budget_);
    for (std::size_t i = 0; i < edges_.size(); ++i) {
      if (!edges_[i].live)
        continue;
      for (unsigned direction : {0U, 2U}) {
        const Slot start = narrow(i * 4 + direction);
        if (visited[start] != 0)
          continue;
        Slot current = start;
        TriangleWord triangle{};
        std::size_t length = 0;
        do {
          budget_.charge(1);
          if (visited[current] != 0)
            invariant("quad-edge face cycle is inconsistent");
          visited[current] = 1;
          if (length < 3)
            triangle[length] = origin(current);
          ++length;
          current = left_next(current);
        } while (current != start);
        if (length == 3 && g_.orient(triangle[0], triangle[1], triangle[2]) > 0)
          output.push_back(triangle);
      }
    }
    stats_.conversion_ns += elapsed(conversion_start);
    stats_.initial_triangles = static_cast<uint64_t>(output.size());
    return output;
  }
};
struct IndexEntry final {
  uint64_t key = 0;
  Slot value = none;
};
class EdgeIndex final {
  std::pmr::vector<IndexEntry> entries_;
  bgfd::Budget &budget_;
  CoreStats &stats_;
  [[nodiscard]] std::size_t home(uint64_t key) const {
    key ^= key >> 30;
    key *= UINT64_C(0xbf58476d1ce4e5b9);
    key ^= key >> 27;
    key *= UINT64_C(0x94d049bb133111eb);
    key ^= key >> 31;
    return static_cast<std::size_t>(key) & (entries_.size() - 1);
  }
  [[nodiscard]] std::size_t following(std::size_t i) const {
    return (i + 1) & (entries_.size() - 1);
  }
  void probe() {
    budget_.charge(1);
    ++stats_.hash_probes;
  }

public:
  static uint64_t key(Slot a, Slot b) { return (static_cast<uint64_t>(a) << 32) | b; }
  EdgeIndex(bgfd::Budget &budget, CoreStats &stats, std::size_t count)
      : entries_(&budget), budget_(budget), stats_(stats) {
    if (count > std::numeric_limits<std::size_t>::max() / 24)
      throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "edge index capacity overflow");
    const auto requested = std::max(std::size_t{16}, count * 24);
    if (requested > (std::size_t{1} << (std::numeric_limits<std::size_t>::digits - 1)))
      throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "edge index capacity overflow");
    const auto capacity = std::bit_ceil(requested);
    budget_.charge(static_cast<uint64_t>(capacity));
    entries_.resize(capacity);
  }
  [[nodiscard]] Slot get(Slot a, Slot b) {
    const uint64_t wanted = key(a, b);
    std::size_t slot = home(wanted);
    for (;;) {
      probe();
      const auto &item = entries_[slot];
      if (item.value == none)
        return none;
      if (item.key == wanted)
        return item.value;
      slot = following(slot);
    }
  }
  void set(Slot a, Slot b, TriangleHandle triangle) {
    const uint64_t wanted = key(a, b);
    std::size_t slot = home(wanted);
    for (;;) {
      probe();
      auto &item = entries_[slot];
      if (item.value == none) {
        item = {wanted, triangle.index};
        return;
      }
      if (item.key == wanted)
        invariant("duplicate directed topology edge");
      slot = following(slot);
    }
  }
  void erase(Slot a, Slot b) {
    const uint64_t wanted = key(a, b);
    std::size_t hole = home(wanted);
    for (;;) {
      probe();
      if (entries_[hole].value == none)
        invariant("missing directed edge during deletion");
      if (entries_[hole].key == wanted)
        break;
      hole = following(hole);
    }
    std::size_t current = following(hole);
    for (;;) {
      probe();
      if (entries_[current].value == none)
        break;
      const auto origin = home(entries_[current].key), mask = entries_.size() - 1;
      if (((current - origin) & mask) >= ((hole - origin) & mask)) {
        entries_[hole] = entries_[current];
        hole = current;
      }
      current = following(current);
    }
    entries_[hole] = {};
  }
};
struct Face final {
  TriangleWord sites{};
  uint64_t seen = 0;
  bool live = false, selected = false;
};
class Mesh final {
  bgfd::Budget &budget_;
  CoreStats &stats_;
  EdgeIndex index_;
  std::pmr::vector<Slot> free_, hint_, degree_;
  uint64_t live_ = 0, epoch_ = 0;
  std::size_t face_limit_;

public:
  std::pmr::vector<Face> faces;
  Mesh(bgfd::Budget &budget, CoreStats &stats, std::size_t count)
      : budget_(budget), stats_(stats), index_(budget, stats, count), free_(&budget),
        hint_(count, none, &budget), degree_(count, 0, &budget), face_limit_(count * 2),
        faces(&budget) {
    if (count > static_cast<std::size_t>(none / 2))
      throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "triangle capacity overflow");
    budget_.charge(static_cast<uint64_t>(count) * 2);
    faces.reserve(count * 2);
  }
  [[nodiscard]] Slot directed(Slot a, Slot b) { return index_.get(a, b); }
  [[nodiscard]] Slot opposite(TriangleHandle triangle, Slot a, Slot b) const {
    for (Slot p : faces[triangle.index].sites)
      if (p != a && p != b)
        return p;
    invariant("triangle opposite vertex absent");
  }
  [[nodiscard]] Slot hint(Slot site) const { return hint_[site]; }
  [[nodiscard]] Slot degree(Slot site) const { return degree_[site]; }
  [[nodiscard]] Slot add(TriangleWord sites) {
    budget_.charge(1);
    if (sites[0] == sites[1] || sites[1] == sites[2] || sites[2] == sites[0])
      invariant("collapsed occurrence triangle");
    for (unsigned i = 0; i < 3; ++i)
      if (index_.get(sites[i], sites[(i + 1) % 3]) != none)
        invariant("directed topology edge already exists");
    Slot result = none;
    if (free_.empty()) {
      if (faces.size() >= face_limit_)
        invariant("planar topology live-face bound exceeded");
      result = narrow(faces.size());
      faces.push_back({sites, 0, true});
    } else {
      result = free_.back();
      free_.pop_back();
      faces[result] = {sites, 0, true};
    }
    for (unsigned i = 0; i < 3; ++i) {
      index_.set(sites[i], sites[(i + 1) % 3], TriangleHandle{result});
      hint_[sites[i]] = result;
      ++degree_[sites[i]];
    }
    ++live_;
    stats_.triangle_peak = std::max(stats_.triangle_peak, live_);
    return result;
  }
  void erase(Slot triangle) {
    budget_.charge(1);
    if (triangle >= faces.size() || !faces[triangle].live)
      invariant("deleting dead triangle");
    const auto sites = faces[triangle].sites;
    for (unsigned i = 0; i < 3; ++i) {
      index_.erase(sites[i], sites[(i + 1) % 3]);
      if (hint_[sites[i]] == triangle)
        hint_[sites[i]] = none;
      --degree_[sites[i]];
    }
    faces[triangle].live = false;
    free_.push_back(triangle);
    --live_;
  }
  template <class F> void star(Slot site, F &&visit) {
    if (hint_[site] == none)
      invariant("live site has no incident triangle");
    ++epoch_;
    if (epoch_ == 0)
      invariant("topology traversal epoch exhausted");
    std::pmr::vector<Slot> work(&budget_);
    work.push_back(hint_[site]);
    while (!work.empty()) {
      budget_.charge(1);
      const Slot tid = work.back();
      work.pop_back();
      if (faces[tid].seen == epoch_)
        continue;
      faces[tid].seen = epoch_;
      if (!visit(tid))
        return;
      const auto triangle = faces[tid].sites;
      for (unsigned i = 0; i < 3; ++i) {
        const Slot a = triangle[i], b = triangle[(i + 1) % 3];
        if (a != site && b != site)
          continue;
        const Slot adjacent = directed(b, a);
        if (adjacent != none && faces[adjacent].seen != epoch_)
          work.push_back(adjacent);
      }
    }
  }
};
class SignedCap final {
  Predicates &p_;
  Geometry g_;
  bgfd::Budget &budget_;
  CoreStats &stats_;
  Random random_;
  std::span<const Slot> boundary_;
  Slot apex_;
  Mesh mesh_;
  struct Insert final {
    Slot u, v, w;
  };
  [[nodiscard]] Slot site(Slot occurrence) const { return boundary_[occurrence]; }
  void insert(Slot u, Slot v, Slot w) {
    std::pmr::vector<Insert> work(&budget_);
    work.push_back({u, v, w});
    while (!work.empty()) {
      budget_.charge(1);
      const auto item = work.back();
      work.pop_back();
      const Slot opposite = mesh_.directed(item.w, item.v);
      if (opposite == none) {
        static_cast<void>(mesh_.add({item.u, item.v, item.w}));
        continue;
      }
      const Slot x = mesh_.opposite(TriangleHandle{opposite}, item.w, item.v);
      if (g_.circle(site(item.w), site(item.v), site(x), site(item.u)) <= 0) {
        static_cast<void>(mesh_.add({item.u, item.v, item.w}));
        continue;
      }
      mesh_.erase(opposite);
      ++stats_.secondary_deletions;
      work.push_back({item.u, x, item.w});
      work.push_back({item.u, item.v, x});
    }
  }

public:
  SignedCap(Predicates &predicates, bgfd::Budget &budget, std::span<const Slot> boundary,
            CapParameters parameters, CoreStats &stats)
      : p_(predicates), g_(predicates), budget_(budget), stats_(stats),
        random_(parameters.seed, budget), boundary_(boundary), apex_(parameters.apex),
        mesh_(budget, stats, boundary.size()) {}
  [[nodiscard]] std::pmr::vector<TriangleWord> run() {
    ++stats_.secondary_calls;
    const auto count = boundary_.size();
    if (count < 3 || apex_ >= p_.size())
      invariant("invalid secondary cap size or apex");
    for (Slot point : boundary_) {
      budget_.charge(1);
      if (point >= p_.size() || point == apex_)
        invariant("deleted fan apex remains in secondary boundary");
    }
    std::pmr::vector<Slot> previous(count, 0, &budget_), next(count, 0, &budget_);
    std::pmr::vector<Slot> order(&budget_);
    for (std::size_t i = 0; i < count; ++i) {
      budget_.charge(1);
      previous[i] = narrow((i + count - 1) % count);
      next[i] = narrow((i + 1) % count);
      order.push_back(narrow(i));
    }
    random_.shuffle(order);
    for (std::size_t i = count; i > 3; --i) {
      budget_.charge(1);
      const Slot u = order[i - 1];
      next[previous[u]] = next[u];
      previous[next[u]] = previous[u];
    }
    const Slot a = order[0], b = next[a], c = next[b];
    const int xy = g_.orient(site(a), site(b), site(c));
    stats_.cap_clockwise_seeds += xy < 0;
    stats_.cap_vertical_seeds += xy == 0;
    // This is a signed lifted cap facet, not an XY-positive triangle seed.
    if (g_.circle(site(a), site(b), site(c), apex_) < 0)
      invariant("secondary cap seed faces away from deleted apex");
    static_cast<void>(mesh_.add({a, b, c}));
    for (std::size_t i = 3; i < count; ++i) {
      budget_.charge(1);
      const Slot u = order[i];
      insert(u, next[u], previous[u]);
    }
    std::pmr::vector<TriangleWord> output(&budget_);
    for (const auto &face : mesh_.faces) {
      budget_.charge(1);
      if (face.live)
        output.push_back(face.sites);
    }
    if (output.size() != count - 2)
      invariant("completed secondary cap count differs");
    return output;
  }
};

class Cavity final {
  Predicates &p_;
  Geometry &g_;
  bgfd::Budget &budget_;
  CoreStats &stats_;
  Random &random_;
  std::span<const Slot> word_;
  Mesh mesh_;
  std::pmr::vector<uint8_t> marked_;
  std::pmr::vector<Slot> touched_, boundary_next_;
  [[nodiscard]] Slot site(Slot occurrence) const { return word_[occurrence]; }
  [[nodiscard]] int circle(Slot a, Slot b, Slot c, Slot d) {
    return g_.circle(site(a), site(b), site(c), site(d));
  }
  void mark(Slot occurrence) {
    if (marked_[occurrence] == 0) {
      marked_[occurrence] = 1;
      touched_.push_back(occurrence);
    }
  }
  struct Insert final {
    Slot u, v, w;
  };
  void chew(std::span<const Slot> boundary, Slot old_apex) {
    std::pmr::vector<Slot> original(&budget_);
    original.reserve(boundary.size());
    for (Slot occurrence : boundary) {
      budget_.charge(1);
      original.push_back(site(occurrence));
    }
    const auto triangles =
        detail::signed_cap(p_, budget_, original, site(old_apex), random_.next(), stats_);
    for (const auto &t : triangles)
      static_cast<void>(mesh_.add({boundary[t[0]], boundary[t[1]], boundary[t[2]]}));
  }
  // Ordered new-vertex/old-apex occurrence roles in the published fan algorithm.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  void fix_fan(Slot u, Slot old_apex) {
    if (old_apex == none)
      invariant("crossed fan has no farther original neighbor apex");
    std::pmr::vector<Slot> selected(&budget_);
    mesh_.star(u, [&](Slot tid) {
      const auto t = mesh_.faces[tid].sites;
      if (marked_[t[0]] != 0 && marked_[t[1]] != 0 && marked_[t[2]] != 0) {
        selected.push_back(tid);
        mesh_.faces[tid].selected = true;
      }
      return true;
    });
    if (selected.empty())
      invariant("marked secondary fan has no triangles");
    Slot start = none;
    for (Slot tid : selected) {
      const auto triangle = mesh_.faces[tid].sites;
      for (unsigned i = 0; i < 3; ++i) {
        const Slot a = triangle[i], b = triangle[(i + 1) % 3];
        const Slot other = mesh_.directed(b, a);
        if (other != none && mesh_.faces[other].selected)
          continue;
        if (boundary_next_[a] != none)
          invariant("secondary fan boundary branches");
        boundary_next_[a] = b;
        start = a;
      }
    }
    if (start == none)
      invariant("secondary fan has no boundary");
    std::pmr::vector<Slot> boundary(&budget_);
    Slot current = start;
    do {
      budget_.charge(1);
      boundary.push_back(current);
      const Slot following = boundary_next_[current];
      boundary_next_[current] = none;
      if (following == none)
        invariant("secondary fan boundary is open");
      current = following;
    } while (current != start);
    for (Slot tid : selected) {
      mesh_.faces[tid].selected = false;
      mesh_.erase(tid);
    }
    chew(boundary, old_apex);
  }
  // The ordered u,v,w triangle and separate old-apex occurrence are mathematical roles.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  void insert(Slot u, Slot v, Slot w, Slot old_apex) {
    ++stats_.cavity_insertions;
    std::pmr::vector<Insert> work(&budget_);
    work.push_back({u, v, w});
    while (!work.empty()) {
      budget_.charge(1);
      const auto item = work.back();
      work.pop_back();
      const Slot adjacent = mesh_.directed(item.w, item.v);
      if (adjacent == none) {
        static_cast<void>(mesh_.add({item.u, item.v, item.w}));
        continue;
      }
      const Slot x = mesh_.opposite(TriangleHandle{adjacent}, item.w, item.v);
      const int inside = circle(item.w, item.v, x, item.u);
      if (inside <= 0 && g_.orient(site(item.u), site(item.v), site(item.w)) > 0) {
        static_cast<void>(mesh_.add({item.u, item.v, item.w}));
        continue;
      }
      if (inside <= 0) {
        if (old_apex == none || (item.w != old_apex && item.v != old_apex && x != old_apex))
          invariant("crossed secondary fan does not share expected apex");
        mark(item.u);
        mark(item.v);
        mark(item.w);
        mark(x);
      }
      mesh_.erase(adjacent);
      work.push_back({item.u, x, item.w});
      work.push_back({item.u, item.v, x});
    }
    if (marked_[u] != 0)
      fix_fan(u, old_apex);
    for (Slot occurrence : touched_) {
      budget_.charge(1);
      marked_[occurrence] = 0;
    }
    touched_.clear();
  }

public:
  Cavity(Predicates &predicates, Geometry &geometry, bgfd::Budget &budget, CoreStats &stats,
         Random &random, std::span<const Slot> word)
      : p_(predicates), g_(geometry), budget_(budget), stats_(stats), random_(random), word_(word),
        mesh_(budget, stats, word.size()), marked_(word.size(), 0, &budget), touched_(&budget),
        boundary_next_(word.size(), none, &budget) {}
  [[nodiscard]] std::pmr::vector<TriangleWord> run() {
    const auto count = word_.size();
    if (count < 3)
      invariant("evacuated cavity has too few occurrences");
    stats_.cavity_occurrences += static_cast<uint64_t>(count);
    stats_.max_cavity = std::max(stats_.max_cavity, static_cast<uint64_t>(count));
    std::pmr::vector<Slot> previous(count, 0, &budget_), next(count, 0, &budget_);
    std::pmr::vector<Slot> order(&budget_);
    for (std::size_t i = 0; i < count; ++i) {
      previous[i] = i == 0 ? 0 : narrow(i - 1);
      next[i] = narrow(std::min(i + 1, count - 1));
      if (i > 0 && i + 1 < count)
        order.push_back(narrow(i));
    }
    for (std::size_t live = order.size(); live > 1; --live) {
      Slot selected = none;
      std::size_t position = 0;
      do {
        position = random_.below(live);
        selected = order[position];
      } while (p_.compare_line_distance(word_.front(), word_.back(), site(selected),
                                        site(previous[selected])) < 0 &&
               p_.compare_line_distance(word_.front(), word_.back(), site(selected),
                                        site(next[selected])) < 0);
      next[previous[selected]] = next[selected];
      previous[next[selected]] = previous[selected];
      std::swap(order[live - 1], order[position]);
    }
    static_cast<void>(mesh_.add({0, order[0], narrow(count - 1)}));
    for (std::size_t i = 1; i < order.size(); ++i) {
      const Slot u = order[i], v = next[u], w = previous[u];
      Slot apex = none;
      if (p_.compare_line_distance(word_.front(), word_.back(), site(v), site(u)) > 0)
        apex = v;
      if (p_.compare_line_distance(word_.front(), word_.back(), site(w), site(u)) > 0) {
        if (apex != none)
          invariant("cavity insertion violates distance eligibility");
        apex = w;
      }
      insert(u, v, w, apex);
    }
    std::pmr::vector<TriangleWord> output(&budget_);
    for (const auto &face : mesh_.faces) {
      budget_.charge(1);
      if (!face.live)
        continue;
      const auto t = face.sites;
      const TriangleWord triangle{site(t[0]), site(t[1]), site(t[2])};
      if (triangle[0] == triangle[1] || triangle[1] == triangle[2] || triangle[2] == triangle[0] ||
          g_.orient(triangle[0], triangle[1], triangle[2]) <= 0)
        invariant("completed cavity has a nonpositive source triangle");
      output.push_back(triangle);
    }
    if (output.size() != count - 2)
      invariant("completed cavity triangle count differs");
    return output;
  }
};

class Constructor final {
  Predicates &p_;
  Geometry g_;
  bgfd::Budget &budget_;
  CoreStats &stats_;
  Random random_;
  Mesh mesh_;
  std::pmr::vector<std::array<Slot, 2>> constraints_;
  std::pmr::vector<uint64_t> protected_;
  [[nodiscard]] bool protected_edge(Slot a, Slot b) const {
    const auto key = EdgeIndex::key(std::min(a, b), std::max(a, b));
    return std::binary_search(protected_.begin(), protected_.end(), key,
                              [&](uint64_t x, uint64_t y) {
                                budget_.charge(1);
                                return x < y;
                              });
  }
  [[nodiscard]] Slot locate(Slot a, Slot b) {
    Slot result = none;
    mesh_.star(a, [&](Slot tid) {
      ++stats_.location_steps;
      const auto triangle = mesh_.faces[tid].sites;
      for (unsigned i = 0; i < 3; ++i) {
        if (triangle[i] != a)
          continue;
        const Slot v = triangle[(i + 1) % 3], w = triangle[(i + 2) % 3];
        if (g_.orient(a, v, b) > 0 && g_.orient(a, b, w) > 0) {
          result = tid;
          return false;
        }
      }
      return true;
    });
    if (result == none)
      invariant("segment start wedge not found");
    return result;
  }
  void cavity(std::span<const Slot> word) {
    Cavity scratch(p_, g_, budget_, stats_, random_, word);
    const auto triangles = scratch.run();
    for (const auto &triangle : triangles)
      static_cast<void>(mesh_.add(triangle));
  }
  void insert_constraint(Slot a, Slot b) {
    ++stats_.constraints;
    if (mesh_.directed(a, b) != none || mesh_.directed(b, a) != none)
      return;
    if (mesh_.degree(b) < mesh_.degree(a))
      std::swap(a, b);
    Slot current = locate(a, b), previous = none;
    std::pmr::vector<Slot> strip(&budget_), left(&budget_), right(&budget_);
    left.push_back(a);
    right.push_back(a);
    for (;;) {
      if (strip.size() >= mesh_.faces.size())
        invariant("constraint strip walk cycled");
      budget_.charge(1);
      const auto triangle = mesh_.faces[current].sites;
      strip.push_back(current);
      if (std::find(triangle.begin(), triangle.end(), b) != triangle.end())
        break;
      bool found = false;
      for (unsigned i = 0; i < 3; ++i) {
        const Slot u = triangle[i], v = triangle[(i + 1) % 3];
        const Slot adjacent = mesh_.directed(v, u);
        if (adjacent == previous)
          continue;
        const int su = g_.orient(a, b, u), sv = g_.orient(a, b, v);
        if (su == 0 || sv == 0 || su == sv)
          continue;
        if (protected_edge(u, v))
          invariant("source constraints cross during strip walk");
        const Slot l = su > 0 ? u : v, r = su > 0 ? v : u;
        if (left.back() != l)
          left.push_back(l);
        if (right.back() != r)
          right.push_back(r);
        if (adjacent == none)
          invariant("constraint left the convex hull");
        previous = current;
        current = adjacent;
        found = true;
        ++stats_.crossed_edges;
        break;
      }
      if (!found)
        invariant("constraint strip has no exit");
    }
    left.push_back(b);
    right.push_back(b);
    std::reverse(left.begin(), left.end());
    for (Slot tid : strip)
      mesh_.erase(tid);
    cavity(left);
    cavity(right);
    if (mesh_.directed(a, b) == none && mesh_.directed(b, a) == none)
      invariant("recovered source constraint absent");
  }

public:
  Constructor(Predicates &predicates, bgfd::Budget &budget, CoreStats &stats, uint64_t seed,
              std::span<const uint32_t> slots, std::span<const bgfd_loop> loops)
      : p_(predicates), g_(predicates), budget_(budget), stats_(stats), random_(seed, budget),
        mesh_(budget, stats, predicates.size()), constraints_(&budget), protected_(&budget) {
    for (const auto &loop : loops) {
      budget_.charge(1);
      if (loop.count < 3 || loop.count >= none || loop.offset > slots.size() ||
          loop.count > slots.size() - loop.offset)
        throw bgfd::Failure(BGFD_INVALID_INPUT, "invalid direct source loop range");
      for (uint32_t i = 0; i < loop.count; ++i) {
        budget_.charge(1);
        const Slot a = slots[loop.offset + i], b = slots[loop.offset + (i + 1) % loop.count];
        if (a >= p_.size() || b >= p_.size())
          throw bgfd::Failure(BGFD_INVALID_INPUT, "invalid direct source point slot");
        constraints_.push_back({a, b});
        protected_.push_back(EdgeIndex::key(std::min(a, b), std::max(a, b)));
      }
    }
    std::sort(protected_.begin(), protected_.end(), [&](uint64_t a, uint64_t b) {
      budget_.charge(1);
      return a < b;
    });
  }
  [[nodiscard]] std::pmr::vector<TriangleWord> run(std::size_t holes) {
    {
      DivideConquer initial(g_, budget_, stats_, p_.size());
      const auto triangles = initial.run();
      const auto conversion_start = Clock::now();
      for (const auto &triangle : triangles)
        static_cast<void>(mesh_.add(triangle));
      stats_.conversion_ns += elapsed(conversion_start);
    }
    const auto recovery_start = Clock::now();
    random_.shuffle(constraints_);
    for (const auto &edge : constraints_)
      insert_constraint(edge[0], edge[1]);
    stats_.recovery_ns += elapsed(recovery_start);
    const auto selection_start = Clock::now();
    budget_.charge(static_cast<uint64_t>(mesh_.faces.size()));
    std::pmr::vector<uint8_t> removed(mesh_.faces.size(), 0, &budget_);
    std::pmr::vector<Slot> work(&budget_);
    for (const auto &edge : constraints_) {
      budget_.charge(1);
      const Slot right = mesh_.directed(edge[1], edge[0]);
      if (right != none && removed[right] == 0) {
        removed[right] = 1;
        work.push_back(right);
      }
    }
    while (!work.empty()) {
      budget_.charge(1);
      const Slot tid = work.back();
      work.pop_back();
      ++stats_.void_triangles;
      const auto triangle = mesh_.faces[tid].sites;
      for (unsigned i = 0; i < 3; ++i) {
        const Slot a = triangle[i], b = triangle[(i + 1) % 3];
        if (protected_edge(a, b))
          continue;
        const Slot adjacent = mesh_.directed(b, a);
        if (adjacent != none && removed[adjacent] == 0) {
          removed[adjacent] = 1;
          work.push_back(adjacent);
        }
      }
    }
    std::pmr::vector<TriangleWord> output(&budget_);
    std::pmr::vector<uint8_t> used(p_.size(), 0, &budget_);
    for (std::size_t i = 0; i < mesh_.faces.size(); ++i) {
      budget_.charge(1);
      if (!mesh_.faces[i].live || removed[i] != 0)
        continue;
      const auto tri = mesh_.faces[i].sites;
      if (g_.orient(tri[0], tri[1], tri[2]) <= 0)
        invariant("direct output triangle not positive");
      output.push_back(tri);
      for (Slot point : tri)
        used[point] = 1;
    }
    budget_.charge(static_cast<uint64_t>(used.size()));
    if (holes > (std::numeric_limits<std::size_t>::max() - p_.size()) / 2 ||
        output.size() != p_.size() + holes * 2 - 2 ||
        std::find(used.begin(), used.end(), uint8_t{0}) != used.end())
      invariant("direct output domain site/count inventory differs");
    stats_.selection_ns += elapsed(selection_start);
    return output;
  }
};
} // namespace
namespace detail {
std::pmr::vector<TriangleWord> signed_cap(Predicates &predicates, bgfd::Budget &budget,
                                          std::span<const Slot> boundary, Slot deleted_apex,
                                          uint64_t seed, CoreStats &stats) {
  SignedCap cap(predicates, budget, boundary, CapParameters{.apex = deleted_apex, .seed = seed},
                stats);
  return cap.run();
}
} // namespace detail
std::pmr::vector<TriangleWord> solve(Predicates &predicates, bgfd::Budget &budget,
                                     std::span<const uint32_t> slots,
                                     std::span<const bgfd_loop> loops, uint64_t seed,
                                     CoreStats &stats) {
  if (loops.empty() || predicates.size() < 3)
    throw bgfd::Failure(BGFD_INVALID_INPUT, "direct CDT requires original domain sites");
  Constructor constructor(predicates, budget, stats, seed, slots, loops);
  return constructor.run(loops.size() - 1);
}
} // namespace bgcdt
