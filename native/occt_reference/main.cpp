// External OCCT reference tessellation. No owned mesher or NVIDIA operation.
#include "../source_uv/source_uv.hpp"
#include <BRepAdaptor_Surface.hxx>
#include <BRepLib_ToolTriangulatedShape.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <BinTools_ShapeSet.hxx>
#include <Poly_Triangulation.hxx>
#include <ShapeExtend_WireData.hxx>
#include <Standard_Failure.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
void require(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}
double seconds(Clock::time_point start) {
  return std::chrono::duration<double>(Clock::now() - start).count();
}
uint64_t number(const std::string &text) {
  require(!text.empty() &&
              text.find_first_not_of("0123456789") == std::string::npos,
          "unsigned integer required");
  size_t used = 0;
  const auto result = std::stoull(text, &used);
  require(used == text.size() && result < UINT64_MAX, "invalid integer");
  return result;
}
struct FaceMap {
  uint64_t id;
  int storage, orientation;
};
struct Job {
  fs::path path;
  std::vector<FaceMap> faces;
};
struct Vertex {
  std::array<double, 3> point, normal;
  uint64_t key;
};
struct Triangle {
  std::array<uint32_t, 3> nodes;
  uint64_t face;
};
struct Progress {
  Clock::time_point started = Clock::now();
  size_t faces = 0;
  int flags = 0;
  double read = 0, preparation = 0, mesh = 0;
};
struct Writer {
  std::ofstream out;
  explicit Writer(const fs::path &path) : out(path, std::ios::binary) {
    require(bool(out), "cannot open mesh output");
  }
  void u32(uint32_t x) {
    for (int i = 0; i < 4; ++i)
      out.put(char(x >> (8 * i)));
  }
  void u64(uint64_t x) {
    for (int i = 0; i < 8; ++i)
      out.put(char(x >> (8 * i)));
  }
  void f64(double x) {
    uint64_t raw;
    std::memcpy(&raw, &x, 8);
    u64(raw);
  }
  void close() {
    out.close();
    require(!out.fail(), "mesh write/close failed");
  }
};
void mesh_job(uint64_t id, const Job &job, double deflection, double angle,
              const fs::path &directory, std::ofstream &faces,
              std::ofstream &definitions, std::ofstream &preparation,
              Progress &progress) {
  const auto begin = Clock::now();
  progress.started = begin;
  std::ifstream input(job.path, std::ios::binary);
  require(bool(input), "cannot read original definition BRep");
  BinTools_ShapeSet shapes;
  shapes.Read(input);
  TopoDS_Shape solid;
  shapes.ReadSubs(solid, input, shapes.NbShapes());
  require(!solid.IsNull() && solid.ShapeType() == TopAbs_SOLID &&
              solid.Location().IsIdentity() &&
              solid.Orientation() == TopAbs_FORWARD,
          "expected original identity-located forward solid");
  std::map<int, TopoDS_Face> original;
  for (TopExp_Explorer it(solid, TopAbs_FACE); it.More(); it.Next()) {
    auto face = TopoDS::Face(it.Current());
    const auto index = shapes.Index(face.Located(TopLoc_Location()));
    require(index > 0 && original.emplace(index, face).second,
            "duplicate original root face");
    TopLoc_Location location;
    require(BRep_Tool::Triangulation(face, location).IsNull() &&
                !BRep_Tool::Surface(face).IsNull(),
            "source has triangulation or no surface");
  }
  require(original.size() == job.faces.size() && !original.empty(),
          "original face inventory differs");
  std::set<int> used;
  for (const auto &row : job.faces) {
    require(row.storage > 0 && row.storage <= shapes.NbShapes(),
            "face storage index out of range");
    const int index = shapes.NbShapes() - row.storage + 1;
    const auto hit = original.find(index);
    require(hit != original.end() && used.insert(index).second &&
                shapes.Shape(index).ShapeType() == TopAbs_FACE,
            "original storage face binding differs");
    require(hit->second.Orientation() ==
                (row.orientation > 0 ? TopAbs_FORWARD : TopAbs_REVERSED),
            "original face orientation differs");
  }
  progress.faces = original.size();
  const double read_seconds = progress.read = seconds(begin);
  bool missing_uv = false;
  for (const auto &entry : original)
    for (TopExp_Explorer it(entry.second, TopAbs_EDGE); it.More(); it.Next()) {
      double first = 0, last = 0;
      if (BRep_Tool::CurveOnSurface(TopoDS::Edge(it.Current()), entry.second,
                                    first, last)
              .IsNull())
        missing_uv = true;
    }
  auto meshing_faces = original;
  brepgpu::SourceUVStats uv_stats;
  size_t chart_approximations = 0;
  {
    const auto prep_start = Clock::now();
    const auto reference_id = [&](const TopoDS_Shape &shape) {
      const auto index = shapes.Index(shape.Located(TopLoc_Location()));
      require(index > 0, "UV binding has no original BinTools shape identity");
      return uint64_t(shapes.NbShapes() - index + 1);
    };
    std::vector<brepgpu::SourceUVFace> bindings;
    uint64_t use_id = 0;
    for (const auto &row : job.faces) {
      brepgpu::SourceUVFace binding;
      binding.id = row.id;
      binding.face = original.at(shapes.NbShapes() - row.storage + 1);
      for (TopExp_Explorer wire_it(binding.face, TopAbs_WIRE); wire_it.More();
           wire_it.Next()) {
        brepgpu::SourceUVLoop loop;
        loop.wire = TopoDS::Wire(wire_it.Current());
        loop.id = reference_id(loop.wire);
        Handle(ShapeExtend_WireData) data =
            new ShapeExtend_WireData(loop.wire, true);
        for (int i = 1; i <= data->NbEdges(); ++i) {
          brepgpu::SourceUVUse use;
          use.id = ++use_id;
          use.edge = data->Edge(i);
          use.edge_id = reference_id(use.edge);
          TopExp::Vertices(use.edge, use.start, use.end, true);
          require(!use.start.IsNull() && !use.end.IsNull(),
                  "UV use lacks original endpoint");
          use.start_id = reference_id(use.start);
          use.end_id = reference_id(use.end);
          loop.uses.push_back(use);
        }
        binding.loops.push_back(std::move(loop));
      }
      bindings.push_back(std::move(binding));
    }
    brepgpu::SourceUVOptions options;
    options.prepare_query = true;
    options.tolerance_mm = deflection;
    const auto prepared = brepgpu::prepare_source_uv(solid, bindings, options);
    solid = prepared.query_shape;
    require(!solid.IsNull(), "source-linked query copy unavailable");
    for (const auto &entry : original)
      meshing_faces.at(entry.first) =
          TopoDS::Face(prepared.query_mapped(entry.second));
    uv_stats = prepared.stats;
    chart_approximations = prepared.chart_approximations.size();
    progress.preparation = seconds(prep_start);
  }
  preparation << id << '\t' << missing_uv << '\t' << uv_stats.projected_edges
              << '\t' << uv_stats.seam_pairs << '\t' << chart_approximations
              << '\t' << uv_stats.max_residual_mm << '\t'
              << uv_stats.query_column_edges << '\t'
              << uv_stats.query_joint_endpoints << '\t'
              << uv_stats.query_connectors << '\t'
              << uv_stats.max_query_residual_mm << '\t'
              << uv_stats.max_query_uv_metric_mm << '\t' << progress.preparation
              << '\n';
  const auto mesh_start = Clock::now();
  BRepMesh_IncrementalMesh mesher(solid, deflection, false, angle, false);
  require(mesher.IsDone(), "OCCT mesher did not complete");
  const int flags = progress.flags = mesher.GetStatusFlags();
  const double mesh_seconds = progress.mesh = seconds(mesh_start);
  size_t triangulated = 0;
  for (const auto &entry : meshing_faces) {
    TopLoc_Location location;
    const auto mesh = BRep_Tool::Triangulation(entry.second, location);
    if (!mesh.IsNull() && mesh->NbNodes() > 0 && mesh->NbTriangles() > 0)
      ++triangulated;
  }
  std::cerr << "OCCT definition=" << id << " flags=" << flags
            << " faces=" << original.size() << " triangulated=" << triangulated
            << " mesh_seconds=" << mesh_seconds << '\n';
  if (triangulated != original.size())
    for (const auto &row : job.faces) {
      const auto &face = meshing_faces.at(shapes.NbShapes() - row.storage + 1);
      TopLoc_Location location;
      const auto mesh = BRep_Tool::Triangulation(face, location);
      if (mesh.IsNull() || mesh->NbNodes() == 0 || mesh->NbTriangles() == 0)
        std::cerr << "untriangulated definition=" << id << " face=" << row.id
                  << " type=" << int(BRepAdaptor_Surface(face).GetType())
                  << '\n';
    }
  std::vector<Vertex> vertices;
  std::vector<Triangle> triangles;
  struct FaceCount {
    FaceMap source;
    int nodes, triangles;
    double actual_deflection;
  };
  std::vector<FaceCount> counts;
  for (const auto &row : job.faces) {
    const auto &face = meshing_faces.at(shapes.NbShapes() - row.storage + 1);
    TopLoc_Location location;
    const auto triangulation = BRep_Tool::Triangulation(face, location);
    if (triangulation.IsNull()) {
      unsigned edges = 0, missing = 0, nonsame = 0;
      for (TopExp_Explorer it(face, TopAbs_EDGE); it.More(); it.Next()) {
        ++edges;
        const auto edge = TopoDS::Edge(it.Current());
        double first = 0, last = 0;
        if (BRep_Tool::CurveOnSurface(edge, face, first, last).IsNull())
          ++missing;
        if (!BRep_Tool::SameParameter(edge))
          ++nonsame;
      }
      std::cerr << "missing face=" << row.id
                << " type=" << int(BRepAdaptor_Surface(face).GetType())
                << " edges=" << edges << " missing_pcurves=" << missing
                << " nonsame=" << nonsame << " free=" << face.Free()
                << " locked=" << face.Locked() << '\n';
    }
    require(!triangulation.IsNull() && triangulation->NbNodes() > 0 &&
                triangulation->NbTriangles() > 0,
            "original source face has no complete triangulation: definition=" +
                std::to_string(id) + " face=" + std::to_string(row.id) +
                " flags=" + std::to_string(flags));
    BRepLib_ToolTriangulatedShape::ComputeNormals(face, triangulation);
    require(triangulation->HasNormals(), "OCCT normals unavailable");
    const auto transform = location.Transformation();
    std::vector<uint32_t> slots(size_t(triangulation->NbNodes()) + 1,
                                UINT32_MAX);
    const auto corner = [&](int node) {
      require(node > 0 && node <= triangulation->NbNodes(),
              "OCCT triangle node outside face");
      auto &slot = slots[size_t(node)];
      if (slot != UINT32_MAX)
        return slot;
      const auto p = triangulation->Node(node).Transformed(transform);
      auto n = triangulation->Normal(node);
      n.Transform(transform);
      if (transform.IsNegative())
        n.Reverse();
      if (face.Orientation() == TopAbs_REVERSED)
        n.Reverse();
      require(std::isfinite(p.X()) && std::isfinite(p.Y()) &&
                  std::isfinite(p.Z()) && std::isfinite(n.X()) &&
                  std::isfinite(n.Y()) && std::isfinite(n.Z()),
              "nonfinite OCCT point/normal");
      require(vertices.size() < UINT32_MAX, "reference vertex capacity");
      slot = static_cast<uint32_t>(vertices.size());
      vertices.push_back(
          {{p.X(), p.Y(), p.Z()}, {n.X(), n.Y(), n.Z()}, uint64_t(slot) + 1});
      return slot;
    };
    for (int i = 1; i <= triangulation->NbTriangles(); ++i) {
      int a, b, c;
      triangulation->Triangle(i).Get(a, b, c);
      if (face.Orientation() == TopAbs_REVERSED)
        std::swap(b, c);
      Triangle triangle{{corner(a), corner(b), corner(c)}, row.id};
      require(a != b && b != c && c != a && triangles.size() < UINT32_MAX,
              "degenerate triangle indices or triangle capacity");
      const auto &p = vertices[triangle.nodes[0]].point;
      const auto &q = vertices[triangle.nodes[1]].point;
      const auto &r = vertices[triangle.nodes[2]].point;
      std::array<long double, 3> u{}, v{}, cross{};
      for (size_t k = 0; k < 3; ++k) {
        u[k] = static_cast<long double>(q[k]) - p[k];
        v[k] = static_cast<long double>(r[k]) - p[k];
      }
      cross = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2],
               u[0] * v[1] - u[1] * v[0]};
      if (cross[0] == 0 && cross[1] == 0 && cross[2] == 0) {
        std::cerr << std::setprecision(17) << "collapsed definition=" << id
                  << " face=" << row.id
                  << " type=" << int(BRepAdaptor_Surface(face).GetType())
                  << " triangle=" << i;
        for (const auto node : {a, b, c}) {
          const auto point = triangulation->Node(node).Transformed(transform);
          std::cerr << " node=" << node << " xyz=" << point.X() << ','
                    << point.Y() << ',' << point.Z();
          if (triangulation->HasUVNodes()) {
            const auto uv = triangulation->UVNode(node);
            std::cerr << " uv=" << uv.X() << ',' << uv.Y();
          }
        }
        std::cerr << '\n';
      }
      require(cross[0] != 0 || cross[1] != 0 || cross[2] != 0,
              "collapsed actual OCCT triangle");
      triangles.push_back(triangle);
    }
    counts.push_back({row, triangulation->NbNodes(),
                      triangulation->NbTriangles(),
                      triangulation->Deflection()});
  }
  const auto path =
      directory / ("definition-" + std::to_string(id) + ".bgrefmesh");
  const auto temporary = path.string() + ".tmp";
  Writer w(temporary);
  w.out.write("BGREFM01", 8);
  w.u32(1);
  w.u32(0);
  w.u64(1);
  w.u64(0);
  w.u64(vertices.size());
  w.u64(triangles.size());
  w.u64(1);
  w.u64(0);
  w.u64(id);
  w.u64(0);
  w.u64(vertices.size());
  w.u64(0);
  w.u64(triangles.size());
  for (const auto &vertex : vertices) {
    for (auto x : vertex.point)
      w.f64(x);
    for (auto x : vertex.normal)
      w.f64(x);
    w.u64(vertex.key);
  }
  for (const auto &triangle : triangles) {
    for (auto n : triangle.nodes)
      w.u32(n);
    w.u32(0);
    w.u64(triangle.face);
  }
  for (auto x : {.5, .5, .5, .5})
    w.f64(x);
  w.close();
  fs::rename(temporary, path);
  for (const auto &count : counts)
    faces << id << '\t' << count.source.id << '\t' << count.source.storage
          << '\t' << count.source.orientation << '\t' << count.nodes << '\t'
          << count.triangles << '\t' << count.actual_deflection << '\t' << flags
          << '\n';
  definitions << id << "\tPASS\t" << job.faces.size() << '\t' << vertices.size()
              << '\t' << triangles.size() << '\t' << flags << '\t'
              << read_seconds << '\t' << progress.preparation << '\t'
              << mesh_seconds << '\t' << seconds(begin)
              << "\tcomplete original face inventory\n";
}
int main(int argc, char **argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--help") {
      std::cout << "OCCT REFERENCE CPU tessellation: --jobs TSV --deflection "
                   "MM --output-directory "
                   "NEWDIR\n";
      return 0;
    }
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc; ++i) {
      std::string key = argv[i];
      require((key == "--jobs" || key == "--deflection" ||
               key == "--output-directory") &&
                  i + 1 < argc,
              "unknown or missing argument");
      require(args.emplace(key, argv[++i]).second, "duplicate argument");
    }
    require(args.size() == 3, "missing reference arguments");
    size_t used = 0;
    const double deflection = std::stod(args.at("--deflection"), &used),
                 angle = .5;
    require(used == args.at("--deflection").size() &&
                std::isfinite(deflection) && deflection > 0,
            "finite positive requested deflection required");
    std::ifstream input(args.at("--jobs"));
    std::string line;
    require(bool(std::getline(input, line)) &&
                line == "definition\tbrep\tface\tstorage\torientation",
            "source jobs header invalid");
    std::map<uint64_t, Job> jobs;
    std::set<uint64_t> face_ids;
    while (std::getline(input, line)) {
      std::istringstream stream(line);
      std::array<std::string, 5> fields;
      for (auto &field : fields)
        require(bool(std::getline(stream, field, '\t')),
                "truncated source face job");
      require(stream.peek() == std::char_traits<char>::eof(),
              "extra face job fields");
      auto &job = jobs[number(fields[0])];
      if (job.path.empty())
        job.path = fields[1];
      require(job.path == fields[1] &&
                  face_ids.insert(number(fields[2])).second,
              "duplicate/foreign face job");
      const auto storage = number(fields[3]);
      require(storage > 0 && storage <= INT32_MAX, "source storage capacity");
      require(fields[4] == "1" || fields[4] == "-1",
              "source face orientation invalid");
      job.faces.push_back(
          {number(fields[2]), int(storage), fields[4] == "1" ? 1 : -1});
    }
    require(!jobs.empty(), "no source definition jobs");
    const fs::path directory(args.at("--output-directory"));
    require(!fs::exists(directory), "output directory already exists");
    fs::create_directories(directory);
    std::ofstream faces(directory / "faces.tsv"),
        definitions(directory / "definitions.tsv"),
        preparation(directory / "preparation.tsv");
    require(bool(faces) && bool(definitions) && bool(preparation),
            "cannot open source reports");
    preparation << std::setprecision(17)
                << "definition\tmissing_uv\tprojected_edges\tseam_pairs\tchart_"
                   "approximations\tfinite_probe_max_residual_mm\tquery_"
                   "columns\tquery_joint_"
                   "endpoints\tquery_connectors\tquery_finite_probe_max_"
                   "residual_mm\tquery_max_uv_"
                   "metric_perturbation_mm\tseconds\n";
    faces << std::setprecision(17)
          << "definition\tface\tstorage\torientation\tnodes\ttriangles\tstored_"
             "deflection\tstatus_"
             "flags\n";
    definitions
        << std::setprecision(17)
        << "definition\tstatus\tfaces\tvertices\ttriangles\tstatus_flags\tread_"
           "seconds\tpreparation_seconds\tmesh_seconds\toperation_"
           "seconds\tmessage\n";
    bool good = true;
    for (const auto &entry : jobs) {
      Progress progress;
      try {
        mesh_job(entry.first, entry.second, deflection, angle, directory, faces,
                 definitions, preparation, progress);
      } catch (const Standard_Failure &e) {
        good = false;
        definitions << entry.first << "\tFAIL\t" << progress.faces << "\t0\t0\t"
                    << progress.flags << '\t' << progress.read << '\t'
                    << progress.preparation << '\t' << progress.mesh << '\t'
                    << seconds(progress.started) << "\tOCCT failure\n";
        std::cerr << "definition=" << entry.first << ' ' << e.what() << '\n';
      } catch (const std::exception &e) {
        good = false;
        definitions << entry.first << "\tFAIL\t" << progress.faces << "\t0\t0\t"
                    << progress.flags << '\t' << progress.read << '\t'
                    << progress.preparation << '\t' << progress.mesh << '\t'
                    << seconds(progress.started) << "\tdefinition failure\n";
        std::cerr << "definition=" << entry.first << ' ' << e.what() << '\n';
      }
    }
    faces.close();
    definitions.close();
    preparation.close();
    require(!faces.fail() && !definitions.fail() && !preparation.fail(),
            "report close failed");
    return good ? 0 : 1;
  } catch (const Standard_Failure &e) {
    std::cerr << e.what() << '\n';
    return 1;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
