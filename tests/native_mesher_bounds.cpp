#include "../native/mesher/bound_tasks.hpp"
#include <bit>
#include <cfenv>
#include <chrono>
#include <cstdlib>
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
void require(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    std::exit(1);
  }
}
uint32_t word(std::istream &in) {
  uint32_t result = 0;
  for (unsigned i = 0; i < 4; ++i) {
    const auto byte = in.get();
    require(byte != std::char_traits<char>::eof(), "truncated bound vector");
    result |= static_cast<uint32_t>(byte) << (8 * i);
  }
  return result;
}
bgmesh::Rational rational(std::istream &in, bgfd::Budget &budget) {
  bgmesh::ExactWords words;
  words.sign = std::bit_cast<int32_t>(word(in));
  const auto n = word(in), d = word(in);
  require(n < 10000 && d > 0 && d < 10000, "invalid bound vector size");
  words.numerator.resize(n);
  words.denominator.resize(d);
  for (auto &limb : words.numerator)
    limb = word(in);
  for (auto &limb : words.denominator)
    limb = word(in);
  return bgmesh::Rational::from_words(budget, words);
}
struct Corpus {
  std::vector<bgmb_root> roots;
  std::vector<bgmb_interval> controls;
  std::vector<bgmb_task> tasks;
  std::vector<bgmesh::BoundDecision> expected;
};
Corpus batch(const char *path) {
  Corpus corpus;
  std::ifstream in(path, std::ios::binary);
  std::string magic(8, '\0');
  in.read(magic.data(), 8);
  require(magic == "BGBNDP01", "wrong bound vector magic");
  const auto count = word(in);
  require(count > 0 && count < 1000, "invalid bound vector count");
  uint64_t total_work = 0, peak = 0;
  uint32_t filtered[3]{};
  for (uint32_t row = 0; row < count; ++row) {
    bgfd::Budget budget({1ULL << 27, 1ULL << 34});
    const auto kind = word(in), p = word(in), q = word(in), controls = word(in);
    std::array<bgmesh::Rational, 4> bounds{rational(in, budget), rational(in, budget),
                                           rational(in, budget), rational(in, budget)};
    std::pmr::vector<bgmesh::Rational> points(&budget), uv(&budget), xyz(&budget);
    for (uint32_t i = 0; i < controls; ++i)
      points.push_back(rational(in, budget));
    for (unsigned i = 0; i < 6; ++i)
      uv.push_back(rational(in, budget));
    for (unsigned i = 0; i < 9; ++i)
      xyz.push_back(rational(in, budget));
    auto tolerance = rational(in, budget);
    const auto expected =
        word(in) != 0 ? bgmesh::BoundDecision::accept : bgmesh::BoundDecision::bound_fail;
    const bgmesh::BoundTaskKey key{7, row, 11, 3};
    bgmesh::BoundAnswer result{};
    const auto offset = corpus.controls.size();
    corpus.controls.resize(offset + controls);
    auto packed = std::span<bgmb_interval>(corpus.controls).subspan(offset, controls);
    if (kind == 0) {
      bgmesh::BezierSurface source(budget, p, q, bounds);
      source.controls = std::move(points);
      corpus.roots.push_back(bgmesh::pack_bound_root(source, packed, offset));
      corpus.tasks.push_back(bgmesh::pack_bound_task(
          key, row, source, std::span<const bgmesh::Rational, 6>(uv.data(), 6),
          std::span<const bgmesh::Rational, 9>(xyz.data(), 9), tolerance));
      result = bgmesh::surface_bound_exact(
          key, source, std::span<const bgmesh::Rational, 6>(uv.data(), 6),
          std::span<const bgmesh::Rational, 9>(xyz.data(), 9), tolerance);
    } else {
      require(kind == 1, "invalid bound kind");
      bgmesh::BezierCurve source(budget, {.dimension = 3, .degree = p}, {bounds[0], bounds[1]});
      source.controls = std::move(points);
      corpus.roots.push_back(bgmesh::pack_bound_root(source, packed, offset));
      corpus.tasks.push_back(
          bgmesh::pack_bound_task(key, row, source, uv[0], uv[1],
                                  std::span<const bgmesh::Rational, 6>(xyz.data(), 6), tolerance));
      result =
          bgmesh::curve_bound_exact(key, source, uv[0], uv[1],
                                    std::span<const bgmesh::Rational, 6>(xyz.data(), 6), tolerance);
    }
    if (result.decision != expected) {
      std::cerr << "bound row=" << row << " kind=" << kind << " p=" << p << " q=" << q << '\n';
      require(false, "independent power-basis actual-cell bound disagrees");
    }
    require(result.key == key, "bound task identity retained");
    const auto filtered_answer =
        bgmesh::filter_bound_cpu(budget, corpus.roots.back(), corpus.controls, corpus.tasks.back());
    require(filtered_answer.key == key, "CPU filter task identity retained");
    require(filtered_answer.decision == bgmesh::BoundDecision::unresolved ||
                filtered_answer.decision == expected,
            "CPU interval filter made unproved decision");
    ++filtered[static_cast<uint32_t>(filtered_answer.decision)];
    if (row == 5)
      require(filtered_answer.decision == bgmesh::BoundDecision::accept,
              "exact root anchoring lost translated plane filter proof");
    corpus.expected.push_back(expected);
    total_work += budget.work();
    peak = std::max(peak, budget.peak());
  }
  require(in.get() == std::char_traits<char>::eof(), "extra bound vector bytes");
  std::cout << "bound_vectors=" << count << " charged_work=" << total_work << " peak_bytes=" << peak
            << " cpu_accept=" << filtered[0] << " cpu_fail=" << filtered[1]
            << " cpu_unresolved=" << filtered[2] << '\n';
  require(filtered[0] > 0 && filtered[1] > 0 && filtered[2] > 0,
          "CPU all filter outcomes exercised");
  const auto original = std::fegetround();
  require(std::fesetround(FE_DOWNWARD) == 0, "change bound worker rounding");
  bgfd::Budget filter_budget({1ULL << 20, 1ULL << 30});
  const auto disabled =
      bgmesh::filter_bound_cpu(filter_budget, corpus.roots[0], corpus.controls, corpus.tasks[0]);
  require(std::fesetround(original) == 0, "restore bound worker rounding");
  require(disabled.decision == bgmesh::BoundDecision::unresolved,
          "CPU bad environment requests fallback");
  bgfd::Budget exhausted({0, 0});
  bool depleted = false;
  try {
    static_cast<void>(
        bgmesh::filter_bound_cpu(exhausted, corpus.roots[0], corpus.controls, corpus.tasks[0]));
  } catch (const bgfd::Failure &error) {
    depleted = error.status == BGFD_RESOURCE_EXHAUSTED;
  }
  require(depleted, "CPU packed work bypassed owner budget");
  return corpus;
}
void device(const char *path, const Corpus &corpus, int32_t ordinal) {
  void *library = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (library == nullptr) {
    std::cerr << dlerror() << '\n';
    require(false, "CUDA bound module unavailable");
  }
  const auto factory =
      reinterpret_cast<const bgmb_api *(*)()>(dlsym(library, "brepgpu_mesher_bound_cuda_v1"));
  require(factory != nullptr, "CUDA bound factory missing");
  const auto *api = factory();
  require(api != nullptr && api->abi == BGMESH_BOUND_ABI && api->size == sizeof(bgmb_api),
          "CUDA bound ABI mismatch");
  const auto get_device = reinterpret_cast<int (*)(int *)>(dlsym(library, "cudaGetDevice"));
  require(get_device != nullptr, "CUDA runtime query unavailable");
  int before = -1, after = -1;
  require(get_device(&before) == 0, "read previous CUDA device");
  auto restored = [&] {
    require(get_device(&after) == 0 && after == before, "bound API changed caller CUDA device");
  };
  bgmb_config config{
      BGMESH_BOUND_ABI, sizeof(bgmb_config), ordinal,    static_cast<uint32_t>(corpus.tasks.size()),
      1ULL << 26,       1ULL << 26,          1ULL << 40, 0};
  bgmb_error error{};
  bgmb_session *session = nullptr;
  auto tiny = config;
  tiny.device_bytes = 1;
  require(api->create(&tiny, corpus.roots.data(), static_cast<uint32_t>(corpus.roots.size()),
                      corpus.controls.data(), corpus.controls.size(), &session, &error) != 0 &&
              session == nullptr,
          "CUDA bound allocation budget ignored");
  restored();
  require(api->create(&config, corpus.roots.data(), static_cast<uint32_t>(corpus.roots.size()),
                      corpus.controls.data(), corpus.controls.size(), &session, &error) == 0,
          "CUDA bound create failed");
  restored();
  std::vector<bgmb_answer> answers(corpus.tasks.size());
  auto tasks = corpus.tasks;
  uint64_t ticket = 0;
  require(
      api->submit(session, tasks.data(), static_cast<uint32_t>(tasks.size()), &ticket, &error) == 0,
      "CUDA bound submit failed");
  restored();
  // Caller buffers can change as soon as submit returns; staged input is owned.
  tasks[0].key.owner = 999;
  uint64_t duplicate = 0;
  require(api->submit(session, tasks.data(), static_cast<uint32_t>(tasks.size()), &duplicate,
                      &error) != 0,
          "CUDA bound accepted concurrent mutable batch");
  uint32_t ready = 0;
  require(api->poll(session, ticket + 1, answers.data(), static_cast<uint32_t>(answers.size()),
                    &ready, &error) != 0,
          "CUDA bound accepted stale ticket");
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
  do {
    const auto status = api->poll(session, ticket, answers.data(),
                                  static_cast<uint32_t>(answers.size()), &ready, &error);
    if (status != 0)
      std::cerr << error.message << '\n';
    require(status == 0, "CUDA bound poll failed");
    restored();
    require(std::chrono::steady_clock::now() < deadline, "CUDA bound completion timeout");
    if (!ready)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
  } while (!ready);
  uint32_t counts[3]{};
  for (std::size_t i = 0; i < answers.size(); ++i) {
    const auto &a = answers[i];
    const auto &k = corpus.tasks[i].key;
    require(a.key.owner == k.owner && a.key.cell == k.cell && a.key.generation == k.generation &&
                a.key.ordinal == k.ordinal,
            "CUDA bound identity/order changed");
    require(a.reserved == 0 && a.decision <= 2, "invalid CUDA bound answer");
    require(a.decision == 2 || a.decision == static_cast<uint32_t>(corpus.expected[i]),
            "CUDA filter made unproved decision");
    ++counts[a.decision];
    if (i == 5)
      require(a.decision == 0, "CUDA translated plane anchor was ineffective");
  }
  require(counts[0] > 0 && counts[1] > 0 && counts[2] > 0, "CUDA all outcomes exercised");
  // Destroy also drains a live second batch, without borrowing caller staging.
  require(api->submit(session, corpus.tasks.data(), static_cast<uint32_t>(corpus.tasks.size()),
                      &ticket, &error) == 0,
          "CUDA session second batch failed");
  require(api->destroy(session, &error) == 0, "CUDA bound drain/destroy failed");
  restored();
  config.work_limit = 0;
  session = nullptr;
  require(api->create(&config, corpus.roots.data(), static_cast<uint32_t>(corpus.roots.size()),
                      corpus.controls.data(), corpus.controls.size(), &session, &error) == 0,
          "zero-work root session create failed");
  require(api->submit(session, corpus.tasks.data(), static_cast<uint32_t>(corpus.tasks.size()),
                      &ticket, &error) == BGMB_RESOURCE,
          "CUDA packed work bypassed reservation");
  restored();
  require(api->destroy(session, &error) == 0, "zero-work session destroy failed");
  restored();
  require(dlclose(library) == 0, "CUDA module close failed");
  std::cout << "device=" << ordinal << " accept=" << counts[0] << " fail=" << counts[1]
            << " unresolved=" << counts[2] << '\n';
}
} // namespace
int main(int argc, char **argv) {
  require(argc == 2 || argc == 3, "usage: native_mesher_bounds vector-file [CUDA-module]");
  const auto corpus = batch(argv[1]);
  if (argc == 3)
    for (int32_t ordinal = 0; ordinal < 2; ++ordinal)
      device(argv[2], corpus, ordinal);
}
