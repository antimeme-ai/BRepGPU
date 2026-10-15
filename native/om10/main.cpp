#include "scene.hpp"
#include <array>
#include <chrono>
#include <dlfcn.h>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
namespace fs = std::filesystem;
namespace bgom10 {
using Clock = std::chrono::steady_clock;
double ms(Clock::time_point t) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}
struct Writer {
  std::ofstream file;
  std::array<char, 65536> buffer{};
  size_t used = 0;
  explicit Writer(const fs::path &p) : file(p, std::ios::binary) {
    require(bool(file), "output: cannot open file");
  }
  void flush() {
    if (used) {
      file.write(buffer.data(), used);
      used = 0;
      require(bool(file), "output: write failed");
    }
  }
  void raw(const void *p, size_t n) {
    flush();
    if (n)
      file.write(static_cast<const char *>(p), n);
    require(bool(file), "output: write failed");
  }
  void u64(uint64_t v) {
    for (int i = 0; i < 8; ++i) {
      if (used == buffer.size())
        flush();
      buffer[used++] = static_cast<char>(v >> (8 * i));
    }
  }
  void u32(uint32_t v) {
    for (int i = 0; i < 4; ++i) {
      if (used == buffer.size())
        flush();
      buffer[used++] = static_cast<char>(v >> (8 * i));
    }
  }
  void f64(double v) {
    uint64_t u;
    std::memcpy(&u, &v, 8);
    u64(u);
  }
  void finish() {
    flush();
    file.close();
    require(!file.fail(), "output: close failed");
  }
};
void write_jets(const fs::path &p, const std::vector<Jet> &jets) {
  Writer w(p);
  w.raw("BGJETS01", 8);
  w.u64(jets.size());
  for (const auto &j : jets) {
    for (double d : j.position)
      w.f64(d);
    for (double d : j.du)
      w.f64(d);
    for (double d : j.dv)
      w.f64(d);
    for (double d : j.normal)
      w.f64(d);
    for (double d : j.used)
      w.f64(d);
    w.u32(j.status);
    w.u32(j.flags);
  }
  w.finish();
}
void write_projected(const fs::path &p, const std::vector<Projected> &points) {
  Writer w(p);
  w.raw("BGPROJ01", 8);
  w.u64(points.size());
  for (const auto &v : points) {
    w.u64(v.instance);
    w.u64(v.definition);
    w.u64(v.source_vertex);
    w.u64(static_cast<uint64_t>(v.x));
    w.u64(static_cast<uint64_t>(v.y));
    w.f64(v.depth);
    for (double d : v.world)
      w.f64(d);
    for (double d : v.normal)
      w.f64(d);
  }
  w.finish();
}
struct Frame {
  std::vector<uint8_t> rgba;
  std::vector<double> depth;
  std::vector<uint64_t> primitive, instance;
  std::vector<Projected> projected;
  std::vector<Sample> samples;
  Frame(const Request &r, uint64_t nplaced) {
    uint64_t n = uint64_t(r.width) * r.height;
    if (!(r.flags & PROJECT_ONLY)) {
      rgba.resize(n * 4);
      depth.resize(n);
      primitive.resize(n);
      instance.resize(n);
      if (r.flags & DUMP_SAMPLES)
        samples.resize(n * r.supersample * r.supersample);
    }
    if (r.flags & DUMP_PROJECTED)
      projected.resize(nplaced);
  }
  FrameOutput output() {
    return {rgba.data(),
            depth.data(),
            primitive.data(),
            instance.data(),
            projected.empty() ? nullptr : projected.data(),
            samples.empty() ? nullptr : samples.data()};
  }
};
void write_frame(const fs::path &dir, uint32_t index, const Request &r,
                 const Frame &f) {
  std::ostringstream s;
  s << "frame-" << std::setfill('0') << std::setw(6) << index;
  fs::path prefix = dir / s.str();
  if (!(r.flags & PROJECT_ONLY)) {
    Writer rgba(prefix.string() + ".rgba");
    rgba.raw(f.rgba.data(), f.rgba.size());
    rgba.finish();
    Writer d(prefix.string() + ".depth"), p(prefix.string() + ".primitive"),
        i(prefix.string() + ".instance");
    for (double v : f.depth)
      d.f64(v);
    for (auto v : f.primitive)
      p.u64(v);
    for (auto v : f.instance)
      i.u64(v);
    d.finish();
    p.finish();
    i.finish();
  }
  if (r.flags & DUMP_PROJECTED)
    write_projected(prefix.string() + ".projected", f.projected);
  if ((r.flags & DUMP_SAMPLES) && !(r.flags & PROJECT_ONLY)) {
    Writer w(prefix.string() + ".samples");
    w.raw("BGSAMP01", 8);
    w.u64(r.width * r.supersample);
    w.u64(r.height * r.supersample);
    for (const auto &v : f.samples) {
      w.f64(v.depth);
      w.u64(v.primitive);
      w.u64(v.instance);
    }
    w.finish();
  }
}
struct TimingRow {
  int64_t frame;
  std::string stage;
  double value;
};
void append(std::vector<TimingRow> &rows, int64_t frame, const Times &t) {
  if (frame < 0) {
    rows.push_back({-1, "session_create", t.create});
    rows.push_back({-1, "scene_allocate_upload", t.upload});
    rows.push_back({-1, "source_evaluate", t.evaluate});
    if (t.download)
      rows.push_back({-1, "source_jet_download", t.download});
  } else {
    rows.push_back({frame, "instance_transform_project", t.project});
    rows.push_back({frame, "triangle_setup", t.setup});
    rows.push_back({frame, "tile_count_scan_allocate_fill", t.bins});
    rows.push_back({frame, "visibility_shading_resolve", t.display});
    rows.push_back({frame, "result_download", t.download});
  }
}
void api_check(bool ok, const Error &e) {
  if (!ok) {
    std::ostringstream s;
    s << e.stage << ": " << e.message << " (row=" << e.row
      << ", native=" << e.native_code << ")";
    throw Failure(s.str());
  }
}
struct SessionOwner {
  const Api *api = nullptr;
  void *token = nullptr;
  void *plugin = nullptr;
  bool close(Error &e, Times &t) {
    if (!token)
      return true;
    void *value = token;
    token = nullptr;
    return api->close(value, &e, &t);
  }
  ~SessionOwner() {
    if (token) {
      Error e{};
      Times t;
      api->close(token, &e, &t);
    }
    if (plugin)
      dlclose(plugin);
  }
};
int run(int argc, char **argv) {
  auto full = Clock::now();
  std::map<std::string, std::string> args;
  bool dump = false;
  for (int i = 1; i < argc; ++i) {
    std::string k = argv[i];
    if (k == "--dump-source-jets") {
      dump = true;
      continue;
    }
    require(k == "--scene" || k == "--views" || k == "--output-directory" ||
                k == "--backend" || k == "--threads" || k == "--device" ||
                k == "--plugin",
            "argument: unknown option");
    require(i + 1 < argc, "argument: missing option value");
    require(args.emplace(k, argv[++i]).second, "argument: repeated option");
  }
  for (const auto *k :
       {"--scene", "--views", "--output-directory", "--backend"})
    require(args.count(k), "argument: missing required option");
  std::string backend = args["--backend"];
  require(backend == "cpu" || backend == "cuda", "argument: invalid backend");
  auto integer = [&](const char *key, int fallback) {
    if (!args.count(key))
      return fallback;
    std::string s = args[key];
    size_t used = 0;
    long long n = std::stoll(s, &used);
    require(used == s.size() && n >= 0 && n <= INT32_MAX,
            "argument: invalid integer");
    return static_cast<int>(n);
  };
  uint32_t threads = integer("--threads", 1);
  int32_t device = integer("--device", 0);
  require(threads >= 1 && threads <= 256, "argument: thread count range");
  std::vector<TimingRow> rows;
  auto t = Clock::now();
  Scene scene(args["--scene"]);
  auto requests = load_requests(args["--views"]);
  rows.push_back({-1, "file_read_decode_validate", ms(t)});
  SceneView view = scene.view();
  fs::path directory = args["--output-directory"];
  if (fs::exists(directory))
    require(fs::is_directory(directory) && fs::is_empty(directory),
            "output: directory must be empty");
  else
    fs::create_directories(directory);
  SessionOwner owner;
  Error error{};
  Times init;
  if (backend == "cpu")
    owner.api = cpu_api();
  else {
    t = Clock::now();
    fs::path plugin =
        args.count("--plugin")
            ? fs::path(args["--plugin"])
            : fs::absolute(argv[0]).parent_path() / "libbrepgpu_om10_cuda.so";
    owner.plugin = dlopen(plugin.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!owner.plugin)
      throw Failure(std::string("session_create: CUDA plugin unavailable: ") +
                    dlerror());
    auto factory = reinterpret_cast<const Api *(*)()>(
        dlsym(owner.plugin, "brepgpu_om10_cuda_v1"));
    require(factory, "session_create: CUDA factory missing");
    owner.api = factory();
    require(owner.api && owner.api->version == 1 &&
                owner.api->size == sizeof(Api),
            "session_create: CUDA plugin API mismatch");
    init.create += ms(t);
  }
  Times setup;
  api_check(
      owner.api->create(&view, threads, device, &owner.token, &error, &setup),
      error);
  init.create += setup.create;
  init.upload += setup.upload;
  std::vector<Jet> jets;
  if (dump)
    jets.resize(view.nvertices);
  Times eval;
  api_check(owner.api->evaluate(owner.token, dump ? jets.data() : nullptr,
                                &error, &eval),
            error);
  init.evaluate = eval.evaluate;
  init.download = eval.download;
  append(rows, -1, init);
  if (dump) {
    t = Clock::now();
    write_jets(directory / "source-jets.bin", jets);
    rows.push_back({-1, "source_jet_write", ms(t)});
  }
  for (uint32_t index = 0; index < requests.size(); ++index) {
    auto frame_start = Clock::now();
    {
      const Request &r = requests[index];
      t = Clock::now();
      Frame frame(r, view.nplaced_vertices);
      double preparation = ms(t);
      auto output = frame.output();
      Times timing;
      api_check(owner.api->render(owner.token, &r, &output, &error, &timing),
                error);
      append(rows, index, timing);
      rows.push_back({index, "host_output_allocate", preparation});
      t = Clock::now();
      write_frame(directory, index, r, frame);
      rows.push_back({index, "output_write", ms(t)});
    }
    rows.push_back({index, "reused_frame_wall", ms(frame_start)});
  }
  Times closing;
  api_check(owner.close(error, closing), error);
  rows.push_back({-1, "session_close", closing.close});
  if (owner.plugin) {
    t = Clock::now();
    require(dlclose(owner.plugin) == 0, "session_close: plugin close failed");
    owner.plugin = nullptr;
    rows.push_back({-1, "plugin_close", ms(t)});
  }
  double complete = ms(full);
  rows.push_back({-1, "complete_native_wall", complete});
  std::ofstream timing(directory / "timings.tsv");
  timing << "scope\tframe\tstage\tmilliseconds\tcount\tnotes\n"
         << std::setprecision(17);
  for (const auto &row : rows)
    timing << (row.frame < 0 ? "session" : "frame") << '\t' << row.frame << '\t'
           << row.stage << '\t' << row.value
           << "\t0\tcompleted host wall; nested stages are not additive\n";
  timing.close();
  require(!timing.fail(), "output: timings write failed");
  std::ofstream metadata(directory / "metadata.tsv");
  metadata
      << "key\tvalue\nbackend\t" << backend << "\ndevice\t"
      << (backend == "cuda" ? device : -1) << "\nthreads\t" << threads
      << "\ndefinitions\t" << view.ndefinitions << "\ninstances\t"
      << view.ninstances << "\nsource_vertices\t" << view.nvertices
      << "\nunique_triangles\t" << view.ntriangles << "\nplaced_vertices\t"
      << view.nplaced_vertices << "\nplaced_triangles\t" << view.nprimitives
      << "\nview_count\t" << requests.size() << "\nsource_jet_dump\t" << dump
      << "\nraster\torthographic; 1/256 sample pixel; integer top-left; maxZ; "
         "lowest primitive exact ties\nnormal\tsource first jet; face-local "
         "interpolation; normalized after rigid placement\nlighting\tlinear "
         "Lambert .20ambient+.70key+.16fill; .55Blinn; roughness "
         "exponent16/32/64/128\nresolve\tlinear average then sRGB; depth/ID "
         "upper-left subpixel\ncomplete_native_wall_scope\tinput decode "
         "through explicit session/plugin close; excludes final "
         "metadata/timing file writes and process shutdown\n";
  metadata.close();
  require(!metadata.fail(), "output: metadata write failed");
  std::cout << "completed\t" << backend << "\t" << requests.size() << "\t"
            << std::setprecision(17) << complete << " ms\n";
  return 0;
}
} // namespace bgom10
int main(int argc, char **argv) {
  try {
    return bgom10::run(argc, argv);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "internal: unknown exception\n";
    return 1;
  }
}
