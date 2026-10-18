// CPU display tile bins in canonical order across worker counts, and display
// bytes across worker counts: native_om10_cpu_bins [<threads> ...] (default
// 1 2 3 12 24 48 96).
//
// The bin oracle is the sequential model in om10_display_fixtures.hpp: tile t's
// list holds, in ascending primitive order, every primitive whose tile
// rectangle contains t. The display oracle is O-bytes (determinism paper
// section 3.2): every worker count emits the same rgba, depth, primitive,
// instance and sample bytes as a separate one-worker baseline session. Each
// row prints the chunk count the bin builder actually used (section 3.3, N4).
#include "../native/om10/cpu.cpp"
#include "om10_display_fixtures.hpp"
#include <iostream>

namespace {
using namespace bgom10;
using namespace om10_fixtures;

bool check_bins(const std::vector<uint32_t> &threads, int runs) {
  bool pass = true;
  for (const BinFamily &f : bin_families()) {
    std::vector<uint64_t> want_offsets;
    std::vector<uint32_t> want_bins;
    model_bins(f, want_offsets, want_bins);
    for (uint32_t n : threads) {
      Session s(SceneView{}, n);
      int equal = 0;
      std::vector<uint64_t> digests;
      for (int run = 0; run < runs; ++run) {
        s.triangles = f.triangles;
        build_bins(s, f.tiles_x, uint64_t(f.tiles_x) * f.tiles_y);
        require_test(s.offsets == want_offsets,
                     f.name + ": tile offsets differ from the sequential count model");
        equal += s.bins == want_bins;
        const uint64_t d = fnv(s.bins.data(), s.bins.size() * sizeof(uint32_t));
        if (std::find(digests.begin(), digests.end(), d) == digests.end())
          digests.push_back(d);
      }
      std::cout << "bins\t" << f.name << "\tthreads " << n << "\tchunks " << s.bin_chunks
                << "\treferences " << want_bins.size() << "\tmodel_equal " << equal << "/" << runs
                << "\tdistinct_digests " << digests.size() << '\n';
      pass = pass && equal == runs;
    }
  }
  return pass;
}

bool check_display(const std::vector<uint32_t> &threads) {
  const DisplayScene d = display_scene();
  const Request r = display_request();
  // Baseline: its own one-worker session, whose bins are filled serially.
  const Frame baseline = render_frame(cpu_api_v2(), d, r, 1, 0);
  uint64_t covered = 0;
  for (uint64_t p : baseline.primitive)
    covered += p != UINT64_MAX;
  require_test(covered > baseline.primitive.size() / 2, "display fixture covers too few pixels");
  bool pass = true;
  for (uint32_t n : threads) {
    const bool equal = render_frame(cpu_api_v2(), d, r, n, 0) == baseline;
    std::cout << "display\tthreads " << n << "\tprimitives " << d.primitive_refs.size()
              << "\tcovered_pixels " << covered << "\tequal_to_baseline " << equal
              << "\tbaseline_digest " << std::hex << baseline.digest() << std::dec << '\n';
    pass = pass && equal;
  }
  for (uint32_t n : threads)
    check_empty_frame(render_frame(cpu_api_v2(), EmptyScene{}, r, n, 0), r);
  std::cout << "display\tempty scene background-only at every worker count\n";
  return pass;
}
} // namespace

int main(int argc, char **argv) {
  try {
    std::vector<uint32_t> threads;
    for (int i = 1; i < argc; ++i)
      threads.push_back(uint32_t(std::stoul(argv[i])));
    if (threads.empty())
      threads = {1, 2, 3, 12, 24, 48, 96};
    const bool display = check_display(threads);
    const bool bins = check_bins(threads, 3);
    std::cout << (display ? "PASS" : "FAIL") << "\tdisplay bytes identical across worker counts\n"
              << (bins ? "PASS" : "FAIL") << "\ttile bins in canonical primitive order\n";
    return display && bins ? 0 : 1;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
