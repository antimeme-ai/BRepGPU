#include "exact.hpp"
#include <array>
#include <chrono>
#include <cstring>
#include <limits>
#include <memory>
#include <numeric>
#include <tuple>

namespace bgfd {
using Clock = std::chrono::steady_clock;
[[nodiscard]] uint64_t elapsed(Clock::time_point start) {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count());
}
[[nodiscard]] uint32_t slot(std::size_t value) {
  if (value >= std::numeric_limits<uint32_t>::max())
    throw Failure(BGFD_RESOURCE_EXHAUSTED, "dense slot count overflow");
  return static_cast<uint32_t>(value);
}
struct NodeSlot final {
  uint32_t value;
  explicit NodeSlot(uint32_t index) noexcept : value(index) {}
  [[nodiscard]] operator std::size_t() const noexcept { return value; }
  [[nodiscard]] bool operator==(const NodeSlot &) const noexcept = default;
};
struct Point final {
  uint64_t id;
  Integer x, y;
  std::array<uint32_t, 2> rank{};
  Point(uint64_t identifier, Integer u, Integer v)
      : id(identifier), x(std::move(u)), y(std::move(v)) {}
};
struct Box final {
  uint32_t x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  [[nodiscard]] bool overlaps(const Box &other) const noexcept {
    return x0 <= other.x1 && other.x0 <= x1 && y0 <= other.y1 && other.y0 <= y1;
  }
  void include(const Box &other) noexcept {
    x0 = std::min(x0, other.x0);
    y0 = std::min(y0, other.y0);
    x1 = std::max(x1, other.x1);
    y1 = std::max(y1, other.y1);
  }
};
struct Segment final {
  uint32_t loop;
  NodeSlot a, b;
  Box box;
};
struct TreeNode final {
  Box box;
  uint32_t begin = 0, end = 0, left = 0, right = 0;
};
class Tree final {
  Budget *budget_;
  std::pmr::vector<TreeNode> nodes_;
  std::pmr::vector<uint32_t> order_;
  std::pmr::vector<Box> boxes_;
  [[nodiscard]] uint32_t build(uint32_t begin, uint32_t end) {
    budget_->charge(1);
    const uint32_t index = slot(nodes_.size());
    nodes_.push_back({boxes_[order_[begin]], begin, end, 0, 0});
    for (uint32_t i = begin + 1; i < end; ++i)
      nodes_[index].box.include(boxes_[order_[i]]);
    if (end - begin > 8) {
      const Box bounds = nodes_[index].box;
      const bool xaxis = bounds.x1 - bounds.x0 >= bounds.y1 - bounds.y0;
      const uint32_t middle = begin + (end - begin) / 2;
      std::nth_element(order_.begin() + begin, order_.begin() + middle, order_.begin() + end,
                       [&](uint32_t a, uint32_t b) {
                         budget_->charge(1);
                         const Box first = boxes_[a], second = boxes_[b];
                         const uint64_t x = xaxis ? static_cast<uint64_t>(first.x0) + first.x1
                                                  : static_cast<uint64_t>(first.y0) + first.y1;
                         const uint64_t y = xaxis ? static_cast<uint64_t>(second.x0) + second.x1
                                                  : static_cast<uint64_t>(second.y0) + second.y1;
                         return std::tie(x, a) < std::tie(y, b);
                       });
      const uint32_t left = build(begin, middle), right = build(middle, end);
      nodes_[index].left = left;
      nodes_[index].right = right;
    }
    return index;
  }

public:
  explicit Tree(Budget &budget)
      : budget_(&budget), nodes_(&budget), order_(&budget), boxes_(&budget) {}
  void reset(const std::pmr::vector<Box> &boxes) {
    nodes_.clear();
    order_.clear();
    boxes_ = boxes;
    if (boxes.empty())
      return;
    if (boxes.size() > (static_cast<std::size_t>(std::numeric_limits<uint32_t>::max()) - 1) / 2)
      throw Failure(BGFD_RESOURCE_EXHAUSTED, "tree node count overflow");
    order_.resize(boxes.size());
    std::iota(order_.begin(), order_.end(), uint32_t{0});
    nodes_.reserve(2 * boxes.size() - 1);
    static_cast<void>(build(0, slot(boxes.size())));
  }
  template <class Visitor> bool query(const Box &bounds, bgfd_stats &stats, Visitor visitor) const {
    if (nodes_.empty())
      return false;
    std::array<uint32_t, 64> pending{};
    std::size_t count = 1;
    pending[0] = 0;
    while (count != 0) {
      const TreeNode &node = nodes_[pending[--count]];
      budget_->charge(1);
      ++stats.bvh_visits;
      if (!node.box.overlaps(bounds))
        continue;
      if (node.left == 0) {
        for (uint32_t i = node.begin; i < node.end; ++i) {
          const uint32_t object = order_[i];
          budget_->charge(1);
          ++stats.leaf_box_tests;
          if (boxes_[object].overlaps(bounds) && visitor(object))
            return true;
        }
      } else {
        if (count + 2 > pending.size())
          throw Failure(BGFD_RESOURCE_EXHAUSTED, "BVH traversal depth exhausted");
        pending[count++] = node.right;
        pending[count++] = node.left;
      }
    }
    return false;
  }
};

struct Engine final {
  Budget budget;
  bgfd_stats stats{};
  std::pmr::vector<Point> points;
  std::pmr::vector<std::pmr::vector<NodeSlot>> loops;
  Integer x_weight, y_weight;
  std::pmr::vector<Segment> segments;
  Tree segment_tree;
  std::pmr::vector<std::array<uint64_t, 3>> triangles;
  bool ready = false;
  explicit Engine(const bgfd_input &input)
      : budget({.bytes = input.memory_limit, .work = input.work_limit}), points(&budget),
        loops(&budget), x_weight(budget), y_weight(budget), segments(&budget), segment_tree(budget),
        triangles(&budget) {
    const auto start = Clock::now();
    stats.abi = BGFD_ABI;
    stats.size = sizeof(bgfd_stats);
    stats.token = input.token;
    if (input.node_count == 0 || input.node_count >= std::numeric_limits<uint32_t>::max() ||
        input.loop_count == 0 || input.loop_count >= std::numeric_limits<uint32_t>::max() ||
        input.slot_count >= std::numeric_limits<uint32_t>::max() || input.nodes == nullptr ||
        input.loops == nullptr || input.slots == nullptr ||
        (input.limb_count != 0 && input.limbs == nullptr))
      throw Failure(BGFD_INVALID_INPUT, "invalid node/loop buffers");
    if (input.node_count > std::numeric_limits<std::size_t>::max() / sizeof(Point) ||
        input.limb_count > std::numeric_limits<std::size_t>::max() / sizeof(uint32_t))
      throw Failure(BGFD_RESOURCE_EXHAUSTED, "input count exceeds address space");
    points.reserve(static_cast<std::size_t>(input.node_count));
    std::pmr::vector<uint64_t> ids(&budget);
    ids.reserve(static_cast<std::size_t>(input.node_count));
    for (uint64_t i = 0; i < input.node_count; ++i) {
      const auto &node = input.nodes[i];
      ids.push_back(node.id);
      auto x = Integer::decode(budget, node.x, input.limbs, input.limb_count);
      auto y = Integer::decode(budget, node.y, input.limbs, input.limb_count);
      stats.max_limbs = std::max(
          {stats.max_limbs, static_cast<uint64_t>(x.size()), static_cast<uint64_t>(y.size())});
      points.emplace_back(node.id, std::move(x), std::move(y));
    }
    std::sort(ids.begin(), ids.end(), [&](uint64_t a, uint64_t b) {
      budget.charge(1);
      return a < b;
    });
    if (std::adjacent_find(ids.begin(), ids.end()) != ids.end())
      throw Failure(BGFD_INVALID_INPUT, "duplicate original node ID");
    x_weight = Integer::decode(budget, input.x_weight, input.limbs, input.limb_count);
    y_weight = Integer::decode(budget, input.y_weight, input.limbs, input.limb_count);
    if (x_weight.sign() <= 0 || y_weight.sign() <= 0)
      throw Failure(BGFD_INVALID_INPUT, "distance weights must be positive");
    uint64_t next = 0;
    for (uint64_t i = 0; i < input.loop_count; ++i) {
      const auto word = input.loops[i];
      if (word.offset != next || word.count < 3 || word.offset > input.slot_count ||
          word.count > input.slot_count - word.offset)
        throw Failure(BGFD_INVALID_INPUT, "invalid or overlapping loop word");
      loops.emplace_back();
      auto &loop = loops.back();
      loop.reserve(static_cast<std::size_t>(word.count));
      for (uint64_t j = 0; j < word.count; ++j) {
        const uint32_t index = input.slots[word.offset + j];
        if (index >= points.size())
          throw Failure(BGFD_INVALID_INPUT, "loop references missing node");
        loop.push_back(NodeSlot(index));
        budget.charge(1);
      }
      next += word.count;
    }
    if (next != input.slot_count)
      throw Failure(BGFD_INVALID_INPUT, "uncovered loop slots");
    std::pmr::vector<uint32_t> order(&budget);
    order.resize(points.size());
    for (std::size_t axis = 0; axis < 2; ++axis) {
      std::iota(order.begin(), order.end(), uint32_t{0});
      auto coord = [&](uint32_t i) -> const Integer & {
        return axis == 0 ? points[i].x : points[i].y;
      };
      std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) {
        const int c = coord(a).compare(coord(b));
        return c < 0 || (c == 0 && a < b);
      });
      uint32_t rank = 0;
      for (std::size_t i = 0; i < order.size(); ++i) {
        if (i != 0 && coord(order[i]).compare(coord(order[i - 1])) != 0)
          ++rank;
        points[order[i]].rank[axis] = rank;
      }
    }
    build_segments(loops);
    stats.node_count = input.node_count;
    stats.segment_count = segments.size();
    stats.decode_ns = elapsed(start);
  }
  [[nodiscard]] Box point_box(NodeSlot node) const {
    const auto rank = points[node].rank;
    return {rank[0], rank[1], rank[0], rank[1]};
  }
  // Endpoint order is immaterial to the closed bounding box.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  [[nodiscard]] Box edge_box(NodeSlot a, NodeSlot b) const {
    Box result = point_box(a);
    result.include(point_box(b));
    return result;
  }
  void build_segments(const std::pmr::vector<std::pmr::vector<NodeSlot>> &boundaries) {
    segments.clear();
    std::pmr::vector<Box> boxes(&budget);
    for (std::size_t li = 0; li < boundaries.size(); ++li) {
      const auto &word = boundaries[li];
      for (std::size_t i = 0; i < word.size(); ++i) {
        const auto a = word[i], b = word[(i + 1) % word.size()];
        segments.push_back({slot(li), a, b, edge_box(a, b)});
        boxes.push_back(segments.back().box);
      }
    }
    segment_tree.reset(boxes);
  }
  [[nodiscard]] bool equal(NodeSlot a, NodeSlot b) const noexcept {
    return points[a].rank == points[b].rank;
  }
  [[nodiscard]] int orient(NodeSlot a, NodeSlot b, NodeSlot c) {
    budget.charge(1);
    ++stats.orientations;
    return points[b]
        .x.subtract(points[a].x)
        .multiply(points[c].y.subtract(points[a].y))
        .subtract(points[b].y.subtract(points[a].y).multiply(points[c].x.subtract(points[a].x)))
        .sign();
  }
  [[nodiscard]] bool on_segment(NodeSlot a, NodeSlot b, NodeSlot p) {
    return edge_box(a, b).overlaps(point_box(p)) && orient(a, b, p) == 0;
  }
  [[nodiscard]] bool touches(NodeSlot a, NodeSlot b, NodeSlot c, NodeSlot d) {
    ++stats.segment_predicates;
    budget.charge(1);
    if (!edge_box(a, b).overlaps(edge_box(c, d)))
      return false;
    const int x = orient(a, b, c), y = orient(a, b, d), z = orient(c, d, a), w = orient(c, d, b);
    return (x * y < 0 && z * w < 0) || (x == 0 && on_segment(a, b, c)) ||
           (y == 0 && on_segment(a, b, d)) || (z == 0 && on_segment(c, d, a)) ||
           (w == 0 && on_segment(c, d, b));
  }
  [[nodiscard]] bool crossing(bgfd_crossing &output) {
    const auto start = Clock::now();
    output.found = 0;
    const uint64_t before_nodes = stats.bvh_visits, before_boxes = stats.leaf_box_tests;
    auto finish = [&] {
      stats.crossing_ns += elapsed(start);
      stats.crossing_bvh_visits += stats.bvh_visits - before_nodes;
      stats.crossing_box_tests += stats.leaf_box_tests - before_boxes;
    };
    for (std::size_t i = 0; i < segments.size(); ++i) {
      const auto first = segments[i];
      uint32_t best = std::numeric_limits<uint32_t>::max();
      static_cast<void>(segment_tree.query(first.box, stats, [&](uint32_t j) {
        if (j <= i || j >= best)
          return false;
        const auto other = segments[j];
        if (first.loop == other.loop &&
            (first.a == other.a || first.a == other.b || first.b == other.a || first.b == other.b))
          return false;
        ++stats.segment_candidates;
        if (touches(first.a, first.b, other.a, other.b))
          best = j;
        return false;
      }));
      if (best != std::numeric_limits<uint32_t>::max()) {
        const auto other = segments[best];
        output.found = 1;
        output.segment_i = i;
        output.segment_j = best;
        output.ids[0] = points[first.a].id;
        output.ids[1] = points[first.b].id;
        output.ids[2] = points[other.a].id;
        output.ids[3] = points[other.b].id;
        finish();
        return true;
      }
    }
    finish();
    return false;
  }
  [[nodiscard]] int winding(const std::pmr::vector<NodeSlot> &word, const Integer &x,
                            const Integer &y, bool doubled) {
    int64_t winding_number = 0;
    for (std::size_t i = 0; i < word.size(); ++i) {
      const auto &a = points[word[i]], &b = points[word[(i + 1) % word.size()]];
      Integer ax = doubled ? a.x.add(a.x) : Integer(a.x),
              ay = doubled ? a.y.add(a.y) : Integer(a.y);
      Integer bx = doubled ? b.x.add(b.x) : Integer(b.x),
              by = doubled ? b.y.add(b.y) : Integer(b.y);
      const int turn = bx.subtract(ax)
                           .multiply(y.subtract(ay))
                           .subtract(by.subtract(ay).multiply(x.subtract(ax)))
                           .sign();
      ++stats.orientations;
      budget.charge(1);
      const int axp = ax.compare(x), bxp = bx.compare(x), ayp = ay.compare(y), byp = by.compare(y);
      if (turn == 0 && ((axp <= 0 && bxp >= 0) || (bxp <= 0 && axp >= 0)) &&
          ((ayp <= 0 && byp >= 0) || (byp <= 0 && ayp >= 0)))
        return 2;
      if (ayp <= 0 && byp > 0 && turn > 0)
        ++winding_number;
      if (byp <= 0 && ayp > 0 && turn < 0)
        --winding_number;
    }
    if (winding_number < -1 || winding_number > 1)
      throw Failure(BGFD_INVALID_DOMAIN, "non-simple winding");
    return static_cast<int>(winding_number);
  }
  void validate_domain() {
    std::pmr::vector<uint8_t> seen(points.size(), uint8_t{0}, &budget);
    for (std::size_t li = 0; li < loops.size(); ++li) {
      Integer area(budget);
      const auto &word = loops[li];
      for (std::size_t i = 0; i < word.size(); ++i) {
        const auto a = word[i], b = word[(i + 1) % word.size()];
        if (seen[a] != 0)
          throw Failure(BGFD_INVALID_DOMAIN, "original node slot repeated in loops");
        seen[a] = 1;
        if (equal(a, b))
          throw Failure(BGFD_INVALID_DOMAIN, "collapsed original boundary edge");
        area =
            area.add(points[a].x.multiply(points[b].y).subtract(points[a].y.multiply(points[b].x)));
      }
      if ((li == 0 && area.sign() <= 0) || (li != 0 && area.sign() >= 0))
        throw Failure(BGFD_INVALID_DOMAIN, "source loop winding differs");
    }
    bgfd_crossing found{};
    if (crossing(found))
      throw Failure(BGFD_CROSSING, "intersecting sampled boundary");
    for (std::size_t h = 1; h < loops.size(); ++h) {
      const auto &probe = points[loops[h][0]];
      if (winding(loops[0], probe.x, probe.y, false) != 1)
        throw Failure(BGFD_INVALID_DOMAIN, "hole outside original outer loop");
      for (std::size_t k = 1; k < loops.size(); ++k)
        if (h != k && winding(loops[k], probe.x, probe.y, false) != 0)
          throw Failure(BGFD_INVALID_DOMAIN, "nested or touching holes");
    }
  }
  [[nodiscard]] Integer distance(NodeSlot a, NodeSlot b) {
    const auto x = points[a].x.subtract(points[b].x), y = points[a].y.subtract(points[b].y);
    return x.multiply(x).multiply(x_weight).add(y.multiply(y).multiply(y_weight));
  }
  [[nodiscard]] bool visible(NodeSlot h, NodeSlot o) {
    const auto bounds = edge_box(h, o);
    const bool blocked = segment_tree.query(bounds, stats, [&](uint32_t index) {
      const auto edge = segments[index];
      if (equal(edge.a, h) || equal(edge.a, o) || equal(edge.b, h) || equal(edge.b, o)) {
        const NodeSlot other = (equal(edge.a, h) || equal(edge.a, o)) ? edge.b : edge.a;
        return !equal(other, h) && !equal(other, o) && on_segment(h, o, other);
      }
      ++stats.segment_candidates;
      return touches(h, o, edge.a, edge.b);
    });
    if (blocked)
      return false;
    const auto x = points[h].x.add(points[o].x), y = points[h].y.add(points[o].y);
    if (winding(loops[0], x, y, true) != 1)
      return false;
    for (std::size_t i = 1; i < loops.size(); ++i)
      if (winding(loops[i], x, y, true) != 0)
        return false;
    return true;
  }
  [[nodiscard]] std::pmr::vector<NodeSlot> bridge() {
    const auto start = Clock::now();
    std::pmr::vector<NodeSlot> outer(loops[0], &budget);
    std::pmr::vector<uint32_t> holes(&budget);
    for (std::size_t i = 1; i < loops.size(); ++i)
      holes.push_back(slot(i));
    auto rightmost = [&](const std::pmr::vector<NodeSlot> &word) {
      std::size_t best = 0;
      for (std::size_t i = 1; i < word.size(); ++i) {
        const auto a = points[word[i]].rank, b = points[word[best]].rank;
        if (a[0] > b[0] || (a[0] == b[0] && a[1] < b[1]))
          best = i;
      }
      return best;
    };
    while (!holes.empty()) {
      std::size_t selected = 0;
      for (std::size_t i = 1; i < holes.size(); ++i) {
        const auto &a = loops[holes[i]], &b = loops[holes[selected]];
        const auto ar = points[a[rightmost(a)]].rank, br = points[b[rightmost(b)]].rank;
        if (ar[0] > br[0] || (ar[0] == br[0] && ar[1] < br[1]))
          selected = i;
      }
      const auto &hole = loops[holes[selected]];
      const auto hi = rightmost(hole);
      const NodeSlot h = hole[hi];
      std::pmr::vector<std::pmr::vector<NodeSlot>> boundaries(&budget);
      boundaries.push_back(outer);
      for (uint32_t i : holes)
        boundaries.push_back(loops[i]);
      build_segments(boundaries);
      struct Candidate final {
        Integer distance;
        uint32_t position;
        Candidate(Integer value, uint32_t index) : distance(std::move(value)), position(index) {}
      };
      std::pmr::vector<Candidate> candidates(&budget);
      for (std::size_t i = 0; i < outer.size(); ++i)
        if (!equal(h, outer[i]))
          candidates.emplace_back(distance(h, outer[i]), slot(i));
      std::sort(candidates.begin(), candidates.end(), [&](const Candidate &a, const Candidate &b) {
        const int cmp = a.distance.compare(b.distance);
        return cmp < 0 || (cmp == 0 && a.position < b.position);
      });
      uint32_t chosen = std::numeric_limits<uint32_t>::max();
      for (const auto &candidate : candidates) {
        ++stats.bridge_candidates;
        budget.charge(1);
        if (visible(h, outer[candidate.position])) {
          chosen = candidate.position;
          break;
        }
      }
      if (chosen == std::numeric_limits<uint32_t>::max())
        throw Failure(BGFD_NO_BRIDGE, "no visible bridge for source hole");
      if (hole.size() + 2 > std::numeric_limits<uint32_t>::max() - outer.size())
        throw Failure(BGFD_RESOURCE_EXHAUSTED, "bridge occurrence count overflow");
      std::pmr::vector<NodeSlot> joined(&budget);
      joined.reserve(outer.size() + hole.size() + 2);
      joined.insert(joined.end(), outer.begin(), outer.begin() + chosen + 1);
      for (std::size_t i = 0; i < hole.size(); ++i)
        joined.push_back(hole[(hi + i) % hole.size()]);
      joined.push_back(h);
      joined.push_back(outer[chosen]);
      joined.insert(joined.end(), outer.begin() + chosen + 1, outer.end());
      outer = std::move(joined);
      holes.erase(holes.begin() + static_cast<std::ptrdiff_t>(selected));
    }
    stats.bridge_ns += elapsed(start);
    return outer;
  }
  void begin_operation() {
    const auto decode = stats.decode_ns;
    stats = {};
    stats.abi = BGFD_ABI;
    stats.size = sizeof(bgfd_stats);
    stats.decode_ns = decode;
    stats.node_count = points.size();
    budget.reset_work();
    ready = false;
    triangles.clear();
  }
  void solve() {
    begin_operation();
    // The segment tree may have been rebuilt for a prior bridge operation.
    build_segments(loops);
    stats.segment_count = segments.size();
    validate_domain();
    bool convex = loops.size() == 1;
    if (convex)
      for (std::size_t i = 0; i < loops[0].size(); ++i)
        if (orient(loops[0][(i + loops[0].size() - 1) % loops[0].size()], loops[0][i],
                   loops[0][(i + 1) % loops[0].size()]) <= 0) {
          convex = false;
          break;
        }
    auto word = bridge();
    const auto start = Clock::now();
    const uint32_t n = slot(word.size());
    std::pmr::vector<uint32_t> previous(&budget), next(&budget);
    std::pmr::vector<uint8_t> alive(n, uint8_t{1}, &budget);
    previous.resize(n);
    next.resize(n);
    Tree point_tree(budget);
    std::pmr::vector<Box> boxes(&budget);
    for (uint32_t i = 0; i < n; ++i) {
      previous[i] = i == 0 ? n - 1 : i - 1;
      next[i] = i + 1 == n ? 0 : i + 1;
      if (!convex)
        boxes.push_back(point_box(word[i]));
    }
    if (!convex)
      point_tree.reset(boxes);
    stats.strict_convex = convex ? 1 : 0;
    uint32_t remaining = n, cursor = 0, stalled = 0;
    while (remaining > 3) {
      budget.charge(1);
      if (alive[cursor] == 0) {
        cursor = next[cursor];
        continue;
      }
      ++stats.ear_attempts;
      const uint32_t ai = previous[cursor], bi = cursor, ci = next[cursor];
      const auto a = word[ai], b = word[bi], c = word[ci];
      bool ear = convex || orient(a, b, c) > 0;
      if (ear && !convex) {
        ++stats.ear_blocker_queries;
        Box bounds = edge_box(a, b);
        bounds.include(point_box(c));
        ear = !point_tree.query(bounds, stats, [&](uint32_t j) {
          ++stats.ear_candidates;
          if (alive[j] == 0) {
            ++stats.dead_skips;
            return false;
          }
          if (j == ai || j == bi || j == ci || equal(word[j], a) || equal(word[j], b) ||
              equal(word[j], c))
            return false;
          return orient(a, b, word[j]) >= 0 && orient(b, c, word[j]) >= 0 &&
                 orient(c, a, word[j]) >= 0;
        });
      }
      if (ear) {
        triangles.push_back({points[a].id, points[b].id, points[c].id});
        alive[bi] = 0;
        next[ai] = ci;
        previous[ci] = ai;
        --remaining;
        cursor = ci;
        stalled = 0;
      } else {
        cursor = next[cursor];
        ++stalled;
        if (stalled > remaining)
          throw Failure(BGFD_NO_POSITIVE_EAR, "source trim polygon has no positive ear");
      }
    }
    uint32_t a = 0;
    while (a < n && alive[a] == 0)
      ++a;
    const uint32_t b = next[a], c = next[b];
    if (orient(word[a], word[b], word[c]) <= 0)
      throw Failure(BGFD_NO_POSITIVE_EAR, "degenerate final source trim ear");
    triangles.push_back({points[word[a]].id, points[word[b]].id, points[word[c]].id});
    stats.triangles = triangles.size();
    stats.ear_ns += elapsed(start);
    ready = true;
  }
};
} // namespace bgfd

struct bgfd_context final {
  bgfd::Engine engine;
  uint64_t token;
  explicit bgfd_context(const bgfd_input &input) : engine(input), token(input.token) {}
};
namespace {
void set_error(bgfd_error *error, bgfd_status status, const char *message) noexcept {
  if (error == nullptr)
    return;
  error->abi = BGFD_ABI;
  error->size = sizeof(bgfd_error);
  error->status = status;
  error->reserved = 0;
  std::strncpy(error->message, message, sizeof(error->message) - 1);
  error->message[sizeof(error->message) - 1] = '\0';
}
template <class Operation>
bgfd_status guarded(bgfd_context *context, bgfd_error *error, Operation operation) noexcept {
  if (error != nullptr &&
      (error->abi != BGFD_ABI || error->size != sizeof(bgfd_error) || error->reserved != 0))
    return BGFD_INVALID_INPUT;
  try {
    operation();
    set_error(error, BGFD_OK, "");
    return BGFD_OK;
  } catch (const bgfd::Failure &failure) {
    if (context != nullptr) {
      context->engine.ready = false;
      context->engine.triangles.clear();
    }
    set_error(error, failure.status, failure.what());
    return failure.status;
  } catch (const std::bad_alloc &) {
    if (context != nullptr) {
      context->engine.ready = false;
      context->engine.triangles.clear();
    }
    set_error(error, BGFD_RESOURCE_EXHAUSTED, "native allocation failed");
    return BGFD_RESOURCE_EXHAUSTED;
  } catch (...) {
    if (context != nullptr) {
      context->engine.ready = false;
      context->engine.triangles.clear();
    }
    set_error(error, BGFD_INTERNAL_ERROR, "unexpected native exception");
    return BGFD_INTERNAL_ERROR;
  }
}
} // namespace
extern "C" bgfd_status bgfd_create(const bgfd_input *input, bgfd_context **output,
                                   bgfd_error *error) {
  if (output != nullptr)
    *output = nullptr;
  return guarded(nullptr, error, [&] {
    if (input == nullptr || output == nullptr || input->abi != BGFD_ABI ||
        input->size != sizeof(bgfd_input) || input->reserved != 0)
      throw bgfd::Failure(BGFD_INVALID_INPUT, "input ABI/version mismatch");
    if (input->memory_limit < sizeof(bgfd_context))
      throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "context exceeds memory budget");
    bgfd_input adjusted = *input;
    adjusted.memory_limit -= sizeof(bgfd_context);
    auto value = std::make_unique<bgfd_context>(adjusted);
    *output = value.release();
  });
}
extern "C" bgfd_status bgfd_first_crossing(bgfd_context *context, bgfd_crossing *output,
                                           bgfd_error *error) {
  return guarded(context, error, [&] {
    if (context == nullptr || output == nullptr || output->abi != BGFD_ABI ||
        output->size != sizeof(bgfd_crossing) || output->reserved != 0)
      throw bgfd::Failure(BGFD_INVALID_INPUT, "crossing ABI/version mismatch");
    context->engine.begin_operation();
    context->engine.build_segments(context->engine.loops);
    context->engine.stats.segment_count = context->engine.segments.size();
    static_cast<void>(context->engine.crossing(*output));
  });
}
extern "C" bgfd_status bgfd_triangulate(bgfd_context *context, bgfd_error *error) {
  return guarded(context, error, [&] {
    if (context == nullptr)
      throw bgfd::Failure(BGFD_INVALID_INPUT, "missing context");
    context->engine.solve();
  });
}
extern "C" bgfd_status bgfd_copy_triangles(bgfd_context *context, uint64_t *ids, uint64_t capacity,
                                           uint64_t *required, bgfd_error *error) {
  return guarded(context, error, [&] {
    if (context == nullptr || required == nullptr || !context->engine.ready)
      throw bgfd::Failure(BGFD_INVALID_INPUT, "no completed triangles");
    if (context->engine.triangles.size() > std::numeric_limits<uint64_t>::max() / 3)
      throw bgfd::Failure(BGFD_RESOURCE_EXHAUSTED, "triangle result size overflow");
    *required = 3 * static_cast<uint64_t>(context->engine.triangles.size());
    if (ids == nullptr && capacity == 0)
      return;
    if (ids == nullptr || capacity < *required)
      throw bgfd::Failure(BGFD_INVALID_INPUT, "triangle output capacity too small");
    uint64_t i = 0;
    for (const auto &triangle : context->engine.triangles)
      for (uint64_t id : triangle)
        ids[i++] = id;
  });
}
extern "C" bgfd_status bgfd_get_stats(const bgfd_context *context, bgfd_stats *output,
                                      bgfd_error *error) {
  return guarded(nullptr, error, [&] {
    if (context == nullptr || output == nullptr || output->abi != BGFD_ABI ||
        output->size != sizeof(bgfd_stats))
      throw bgfd::Failure(BGFD_INVALID_INPUT, "statistics ABI/version mismatch");
    *output = context->engine.stats;
    output->token = context->token;
    output->memory_peak = context->engine.budget.peak() + sizeof(bgfd_context);
    output->work = context->engine.budget.work();
    output->max_limbs = context->engine.budget.max_limbs();
  });
}
extern "C" void bgfd_destroy(bgfd_context *context) { delete context; }

extern "C" uint32_t bgfd_abi_size(uint32_t record) {
  switch (record) {
  case 0:
    return BGFD_ABI;
  case 1:
    return sizeof(bgfd_input);
  case 2:
    return sizeof(bgfd_error);
  case 3:
    return sizeof(bgfd_crossing);
  case 4:
    return sizeof(bgfd_stats);
  case 5:
    return sizeof(bgfd_node);
  case 6:
    return sizeof(bgfd_integer);
  case 7:
    return sizeof(bgfd_loop);
  default:
    return 0;
  }
}
