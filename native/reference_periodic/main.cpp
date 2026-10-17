#include "../face_domain/exact.hpp"
#include "prepare.hpp"
#include <BinTools_ShapeSet.hxx>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
namespace {
void require(bool yes, const char *text) {
  if (!yes)
    throw std::runtime_error(text);
}
double elapsed(Clock::time_point start) {
  return std::chrono::duration<double>(Clock::now() - start).count();
}
void failure(const fs::path &output, bool owned, const char *message) {
  if (owned) {
    std::error_code ignored;
    fs::remove(output / "prepared.brep", ignored);
    fs::remove(output / "prepared.brep.tmp", ignored);
    std::ofstream status(output / "status.tsv");
    std::string detail(message);
    for (auto &c : detail)
      if (c == '\t' || c == '\n' || c == '\r')
        c = ' ';
    status << "key\tvalue\nstatus\tFAIL\nstage\tprepare\ndetail\t" << detail << '\n';
  }
  std::cerr << message << '\n';
}
} // namespace
int main(int argc, char **argv) {
  fs::path output;
  bool owned = false;
  const auto begin = Clock::now();
  try {
    if (argc == 2 && std::string(argv[1]) == "--help") {
      std::cout
          << "Guarded RAW OCCT periodic chart preparation; no tessellation\n"
             "INPUT.brep NEW_OUTPUT_DIR [--projection-precision-mm VALUE "
             "--source-residual-mm VALUE --positional-mm VALUE]\n"
             "Defaults 1e-7/.001/.005 mm; angle split pi; source-derived closing precision.\n";
      return 0;
    }
    require(argc >= 3, "expected INPUT.brep NEW_OUTPUT_DIR");
    brepgpu::SourceUVOptions options;
    for (int i = 3; i < argc; i += 2) {
      require(i + 1 < argc, "missing preparation option value");
      const std::string arg = argv[i], text = argv[i + 1];
      size_t used = 0;
      const double value = std::stod(text, &used);
      require(used == text.size() && std::isfinite(value) && value > 0,
              "invalid preparation option value");
      if (arg == "--projection-precision-mm")
        options.projection_precision_mm = value;
      else if (arg == "--source-residual-mm")
        options.source_residual_mm = value;
      else if (arg == "--positional-mm")
        options.tolerance_mm = value;
      else
        throw std::runtime_error("unknown preparation option");
    }
    output = argv[2];
    require(!fs::exists(output) && fs::create_directory(output), "output directory must be new");
    owned = true;
    auto time = Clock::now();
    std::ifstream input(argv[1], std::ios::binary);
    require(bool(input), "cannot open original BRep");
    BinTools_ShapeSet source;
    source.Read(input);
    TopoDS_Shape original;
    source.ReadSubs(original, input, source.NbShapes());
    const double read_seconds = elapsed(time);
    auto result = brepgpu::prepare_periodic(original, options, &source);
    BinTools_ShapeSet writer;
    writer.SetFormatNb(BinTools_FormatVersion_VERSION_4);
    writer.SetWithTriangles(false);
    writer.SetWithNormals(false);
    writer.Add(result.shape);
    require(static_cast<uint64_t>(writer.NbShapes()) == result.prepared_shapes,
            "serialized shape inventory changed");
    std::set<uint64_t> face_rows, edge_rows, source_faces;
    for (const auto &row : result.history) {
      require(row.prepared_index > 0 && row.prepared_index <= result.prepared_shapes &&
                  row.source_index <= result.original_shapes,
              "history index outside serialized inventory");
      if (row.role == "face") {
        face_rows.insert(row.prepared_index);
        source_faces.insert(row.source_index);
      }
      if (row.role == "edge" || row.role == "generated_edge")
        edge_rows.insert(row.prepared_index);
      const auto kind = row.role == "face"                                   ? TopAbs_FACE
                        : row.role == "edge" || row.role == "generated_edge" ? TopAbs_EDGE
                                                                             : TopAbs_VERTEX;
      require(writer.Shape(static_cast<int>(row.prepared_index)).ShapeType() == kind,
              "serialized history role mismatch");
      if (row.source_index)
        require(source.Shape(static_cast<int>(row.source_index)).ShapeType() == kind,
                "source history role mismatch");
    }
    uint64_t written_faces = 0, written_edges = 0;
    for (int i = 1; i <= writer.NbShapes(); ++i) {
      written_faces += writer.Shape(i).ShapeType() == TopAbs_FACE ? 1U : 0U;
      written_edges += writer.Shape(i).ShapeType() == TopAbs_EDGE ? 1U : 0U;
    }
    require(written_faces == result.prepared_faces && face_rows.size() == written_faces &&
                edge_rows.size() == written_edges && source_faces.size() == result.original_faces,
            "serialized face/edge history inventory differs");
    std::ofstream restorations(output / "range-restorations.tsv");
    std::ofstream stages(output / "stages.tsv"), history(output / "source-history.tsv"),
        tolerances(output / "tolerances.tsv"), metadata(output / "metadata.tsv");
    require(stages && history && tolerances && metadata && restorations,
            "cannot write preparation reports");
    stages << std::setprecision(17) << "stage\tseconds\nread\t" << read_seconds << '\n';
    for (const auto &row : result.stages)
      stages << row.name << '\t' << row.seconds << '\n';
    history
        << std::setprecision(17)
        << "role\tsource_index\tprepared_index\tsource_serialized_index\tprepared_serialized_index"
           "\tsource_first\tsource_last\tfirst\tlast\tperiod\tdirection\torder\tfirst_vertex\tlast_"
           "vertex\tsource_first_vertex\tsource_last_vertex\n";
    for (const auto &row : result.history)
      history << row.role << '\t' << row.source_index << '\t' << row.prepared_index << '\t'
              << (row.source_index ? result.original_shapes - row.source_index + 1 : 0) << '\t'
              << result.prepared_shapes - row.prepared_index + 1 << '\t' << row.source_first << '\t'
              << row.source_last << '\t' << row.first << '\t' << row.last << '\t' << row.period
              << '\t' << row.direction << '\t' << row.order << '\t' << row.first_vertex << '\t'
              << row.last_vertex << '\t' << row.source_first_vertex << '\t'
              << row.source_last_vertex << '\n';
    tolerances << std::setprecision(17) << "role\tprepared_index\toriginal_mm\tprepared_mm\n";
    for (const auto &row : result.tolerances)
      tolerances << row.role << '\t' << row.index << '\t' << row.original << '\t' << row.prepared
                 << '\n';
    restorations << std::setprecision(17)
                 << "source_index\tprepared_index\told_first\told_last\tfirst\tlast\told_same_"
                    "range\told_same_parameter\tsame_range\tsame_parameter\n";
    for (const auto &row : result.range_restorations)
      restorations << row.source_index << '\t' << row.prepared_index << '\t' << row.old_first
                   << '\t' << row.old_last << '\t' << row.first << '\t' << row.last << '\t'
                   << row.old_same_range << '\t' << row.old_same_parameter << "\t0\t0\n";
    metadata << std::setprecision(17) << "key\tvalue\npolicy\traw-source-uv+angle-pi-v1\n"
             << "projection_precision_mm\t" << options.projection_precision_mm
             << "\nsource_residual_mm\t" << options.source_residual_mm << "\npositional_mm\t"
             << options.tolerance_mm << "\nprojected_edges\t" << result.raw_stats.projected_edges
             << "\nseam_pairs\t" << result.raw_stats.seam_pairs << "\nchart_approximations\t"
             << result.chart_approximations << "\nfinite_probe_max_residual_mm\t"
             << result.raw_stats.max_residual_mm << '\n';
    time = Clock::now();
    std::ofstream file(output / "prepared.brep.tmp", std::ios::binary);
    writer.Write(file);
    writer.Write(result.shape, file);
    file.close();
    require(!file.fail(), "prepared BRep write failed");
    stages << "serialize\t" << elapsed(time) << '\n';
    restorations.close();
    stages.close();
    history.close();
    tolerances.close();
    metadata.close();
    require(!stages.fail() && !history.fail() && !tolerances.fail() && !metadata.fail() &&
                !restorations.fail(),
            "preparation report close failed");
    std::ofstream status(output / "status.tsv");
    status << std::setprecision(17) << "key\tvalue\nstatus\tPASS\noriginal_faces\t"
           << result.original_faces << "\nprepared_faces\t" << result.prepared_faces
           << "\noriginal_face_coverage\tCOMPLETE\nall_prepared_faces_have_ancestry\t1\nseconds\t"
           << elapsed(begin) << '\n';
    status.close();
    require(!status.fail(), "status close failed");
    fs::rename(output / "prepared.brep.tmp", output / "prepared.brep");
    std::cout << "prepared_faces\t" << result.prepared_faces << "\nseconds\t" << elapsed(begin)
              << '\n';
    return 0;
  } catch (const Standard_Failure &error) {
    failure(output, owned, error.what());
  } catch (const bgfd::Failure &error) {
    failure(output, owned, error.what());
  } catch (const std::exception &error) {
    failure(output, owned, error.what());
  }
  return 1;
}
