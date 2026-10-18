// CUDA display tile bins in canonical order, and display bytes across runs and
// GPUs. Fleet only: native_om10_cuda_bins <device> [<device> ...].
//
// The bin oracle is a sequential model: tile t's list holds, in ascending
// primitive order, every primitive whose tile rectangle contains t. The display
// oracle is O-bytes (determinism paper section 3.2): every run on every listed
// device emits the same rgba, depth, primitive, instance and sample bytes.
#include "../native/om10/cuda.cu"
#include "om10_display_fixtures.hpp"
#include <iostream>

namespace {
using namespace bgom10;
using namespace om10_fixtures;

// Returns the long-tile grid build_bins actually launched.
uint32_t device_bins(int device, const BinFamily &f, std::vector<uint64_t> &offsets,
                     std::vector<uint32_t> &bins) {
  const uint64_t n = f.triangles.size(), ntiles = uint64_t(f.tiles_x) * f.tiles_y;
  Session s;
  s.device = device;
  Error e{}, closing{};
  bool ok = selected(s, &e, [&] {
    open_stream(s);
    allocate(s.triangles, n);
    copy_in(s.triangles, f.triangles.data(), n, s);
    const uint64_t references = build_bins(s, n, f.tiles_x, ntiles);
    offsets.resize(ntiles + 1);
    bins.resize(references);
    copy_out(offsets.data(), s.offsets, ntiles + 1, s);
    copy_out(bins.data(), s.bins, references, s);
    finish(s);
  });
  bool clean = true;
  bool released = !s.stream || selected(s, &closing, [&] { cleanup(s, &closing, clean); });
  require_test(ok, std::string("bin build failed: ") + e.message);
  require_test(released && clean, std::string("bin session close failed: ") + closing.message);
  return s.long_grid;
}

// Returns false (and prints the run summary) when any run differs from the model.
bool check_bins(const std::vector<int> &devices, int runs) {
  bool pass = true;
  for (const BinFamily &f : bin_families()) {
    std::vector<uint64_t> want_offsets;
    std::vector<uint32_t> want_bins;
    model_bins(f, want_offsets, want_bins);
    for (int device : devices) {
      int equal = 0;
      uint32_t long_grid = 0;
      std::vector<uint64_t> digests;
      for (int run = 0; run < runs; ++run) {
        std::vector<uint64_t> offsets;
        std::vector<uint32_t> bins;
        long_grid = device_bins(device, f, offsets, bins);
        require_test(offsets == want_offsets,
                     f.name + ": tile offsets differ from the sequential count model");
        equal += bins == want_bins;
        uint64_t d = fnv(bins.data(), bins.size() * sizeof(uint32_t));
        if (std::find(digests.begin(), digests.end(), d) == digests.end())
          digests.push_back(d);
      }
      // Launch shape actually used (section 3.3, N4): tiles per sort path,
      // which follow from the offsets checked above, and the long-tile grid.
      uint64_t paths[3] = {};
      for (uint64_t t = 0; t + 1 < want_offsets.size(); ++t) {
        const uint64_t k = want_offsets[t + 1] - want_offsets[t];
        if (k >= 2)
          ++paths[k <= SHORT_BIN ? 0 : k <= MEDIUM_BIN ? 1 : 2];
      }
      // The ordering stage runs, and launches a long-tile grid, exactly when
      // there is more than one reference.
      const bool shape = (want_bins.size() > 1) == (long_grid > 0);
      std::cout << "bins\t" << f.name << "\tdevice " << device << "\treferences "
                << want_bins.size() << "\twarp1/warp8/block_tiles " << paths[0] << "/"
                << paths[1] << "/" << paths[2] << "\tlong_grid " << long_grid << "\tmodel_equal "
                << equal << "/" << runs << "\tdistinct_digests " << digests.size() << '\n';
      pass = pass && equal == runs && shape;
    }
  }
  return pass;
}

bool check_display(const std::vector<int> &devices, int runs) {
  const DisplayScene d = display_scene();
  const Request r = display_request();
  const Api *api = brepgpu_om10_cuda_v2();
  const Frame first = render_frame(api, d, r, 1, devices.front());
  uint64_t covered = 0;
  for (uint64_t p : first.primitive)
    covered += p != UINT64_MAX;
  require_test(covered > first.primitive.size() / 2, "display fixture covers too few pixels");
  bool pass = true;
  for (int device : devices) {
    int equal = 0;
    for (int run = 0; run < runs; ++run)
      equal += render_frame(api, d, r, 1, device) == first;
    std::cout << "display\tdevice " << device << "\tprimitives " << d.primitive_refs.size()
              << "\tcovered_pixels " << covered << "\tequal_to_first " << equal << "/" << runs
              << "\tfirst_digest " << std::hex << first.digest() << std::dec << '\n';
    pass = pass && equal == runs;
  }
  for (int device : devices)
    check_empty_frame(render_frame(api, EmptyScene{}, r, 1, device), r);
  std::cout << "display\tempty scene background-only on every device\n";
  return pass;
}
} // namespace

int main(int argc, char **argv) {
  try {
    std::vector<int> devices;
    for (int i = 1; i < argc; ++i)
      devices.push_back(std::stoi(argv[i]));
    require_test(!devices.empty(), "usage: native_om10_cuda_bins <device> [<device> ...]");
    const bool display = check_display(devices, 6);
    const bool bins = check_bins(devices, 8);
    std::cout << (display ? "PASS" : "FAIL") << "\tdisplay bytes identical across runs and devices\n"
              << (bins ? "PASS" : "FAIL") << "\ttile bins in canonical primitive order\n";
    return display && bins ? 0 : 1;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
