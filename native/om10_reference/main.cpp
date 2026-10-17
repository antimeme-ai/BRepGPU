#include "raw.hpp"
#include <array>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
namespace fs = std::filesystem;
namespace bgref {
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
void api_check(bool ok,const Error &e){
  if(!ok){
    std::ostringstream s;
    s<<e.stage<<": "<<e.message<<" (row="<<e.row<<", native="<<e.native_code<<")";
    throw Failure(s.str());
  }
}
struct Owner {
  const ReferenceApi *api=reference_cpu_api();
  void *token=nullptr;
  void close(){
    if(!token)return;
    void *value=token;token=nullptr;
    Error error{};Times times;
    api_check(api->close(value,&error,&times),error);
  }
  ~Owner(){
    if(token){
      Error error{};Times times;
      if(!api->close(token,&error,&times))
        std::fprintf(stderr,"cleanup: %s: %s\n",error.stage,error.message);
    }
  }
};
int run(int argc,char **argv){
  auto begin=Clock::now();
  if(argc==2 && std::string(argv[1])=="--help"){
    std::cout<<"brepgpu-om10-reference-display --mesh RAW_BGREFM01 --views BGVIEW01 "
               "--output-directory NEW_DIRECTORY [--threads 1..256]\n"
               "CPU-only raw external XYZ/normal display; no surface evaluation.\n";
    return 0;
  }
  std::map<std::string,std::string> args;
  for(int i=1;i<argc;++i){
    std::string key=argv[i];
    require(key=="--mesh" || key=="--views" || key=="--output-directory" || key=="--threads","argument: unknown option");
    require(i+1<argc,"argument: missing value");
    require(args.emplace(key,argv[++i]).second,"argument: repeated option");
  }
  for(const char *key:{"--mesh","--views","--output-directory"})
    require(args.count(key),"argument: missing required option");
  uint32_t threads=1;
  if(args.count("--threads")){
    size_t used=0;long long n=std::stoll(args["--threads"],&used);
    require(used==args["--threads"].size() && n>=1 && n<=256,"argument: thread count range");
    threads=static_cast<uint32_t>(n);
  }
  fs::path directory=args["--output-directory"];
  require(!fs::exists(directory),"output: destination must not exist");
  std::vector<TimingRow> rows;
  uint64_t definitions=0,occurrences=0,vertices=0,primitives=0;
  size_t frames=0;
  {
    auto start=Clock::now();
    const RawScene scene(args["--mesh"]);
    const auto requests=load_requests(args["--views"]);
    rows.push_back({-1,"raw_file_read_decode_validate",ms(start)});
    const SceneView view=scene.view();
    definitions=view.ndefinitions;occurrences=view.ninstances;
    vertices=view.nvertices;primitives=view.nprimitives;frames=requests.size();
    fs::create_directories(directory);
    Owner owner;Error error{};Times initial;
    api_check(owner.api->create(&scene,threads,&owner.token,&error,&initial),error);
    rows.push_back({-1,"raw_session_create",initial.create});
    for(uint32_t index=0;index<requests.size();++index){
      auto frame_start=Clock::now();
      {
        const auto &request=requests[index];
        start=Clock::now();Frame frame(request,view.nplaced_vertices);
        rows.push_back({index,"host_output_allocate",ms(start)});
        auto output=frame.output();Times times;
        api_check(owner.api->render(owner.token,&request,&output,&error,&times),error);
        rows.push_back({index,"raw_instance_transform_project",times.project});
        rows.push_back({index,"triangle_setup",times.setup});
        rows.push_back({index,"tile_count_scan_allocate_fill",times.bins});
        rows.push_back({index,"visibility_shading_resolve",times.display});
        rows.push_back({index,"diagnostic_host_copy",times.download});
        start=Clock::now();write_frame(directory,index,request,frame);
        rows.push_back({index,"output_write",ms(start)});
      }
      rows.push_back({index,"reused_frame_wall",ms(frame_start)});
    }
    start=Clock::now();owner.close();rows.push_back({-1,"raw_session_close",ms(start)});
  }
  rows.push_back({-1,"complete_native_through_host_teardown",ms(begin)});
  std::ofstream timing(directory/"timings.tsv");
  timing<<"scope\tframe\tstage\tmilliseconds\tcount\tnotes\n"<<std::setprecision(17);
  for(const auto &row:rows)
    timing<<(row.frame<0?"session":"frame")<<'\t'<<row.frame<<'\t'<<row.stage<<'\t'<<row.value
          <<"\t0\tcompleted host wall; nested not additive; final metadata/timings and process exit excluded\n";
  timing.close();require(!timing.fail(),"output: timing write failed");
  std::ofstream metadata(directory/"metadata.tsv");
  metadata<<"key\tvalue\nbackend\tshared_owned_cpu_display\nthreads\t"<<threads
    <<"\ngeometry\tsupplied external definition-local XYZ and per-corner oriented normals; no carrier evaluation"
    <<"\ndefinitions\t"<<definitions<<"\noccurrences\t"<<occurrences<<"\nraw_vertices\t"<<vertices
    <<"\nplaced_triangles\t"<<primitives<<"\nframes\t"<<frames
    <<"\ninput\tBGREFM01; external physical keys and face ordinals are producer identities"
    <<"\nnormal\tpositive unit normalization only; no sign reconstruction"
    <<"\ncomplete_native_scope\tthrough raw outputs and retained host/session teardown; excludes final metadata/timing writes and exit\n";
  metadata.close();require(!metadata.fail(),"output: metadata write failed");
  std::cout<<"raw reference CPU display complete; "<<frames<<" frames\n";
  return 0;
}
} // namespace bgref
int main(int argc,char **argv){
  try{return bgref::run(argc,argv);}
  catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
  catch(...){std::cerr<<"unknown reference display failure\n";return 1;}
}
