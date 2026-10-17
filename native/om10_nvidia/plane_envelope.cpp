// Conservative reference-domain preparation: dump untouched imported
// homogeneous curve controls and the final SDK plane frames. No geometry is
// changed.
#include <SmApiGeneral.h>
#include <SmApiUsd.h>
#include <SmAxis2Placement.h>
#include <SmBSplineCurve.h>
#include <SmBrep.h>
#include <SmEdge.h>
#include <SmPlane.h>
#include <SmVertex.h>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
void require(bool b, const char *m) {
  if (!b)
    throw std::runtime_error(m);
}
void finite_word(double x) {
  require(std::isfinite(x), "nonfinite source word");
}
struct Imported {
  std::vector<SmBrep *> values;
  ~Imported() {
    for (auto *p : values)
      delete p;
  }
};
int main(int argc, char **argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--help") {
      std::cout << "Dump complete imported positive-weight B-spline boundary "
                   "hull and canonical plane frames.\nINPUT.usdc OUTPUT_PREFIX "
                   "PLANE_FRAMES.tsv\nReference preparation only; no source "
                   "geometry or triangles are changed.\n";
      return 0;
    }
    require(argc == 4, "usage: INPUT.usdc OUTPUT_PREFIX PLANE_FRAMES.tsv");
    const auto begin = Clock::now();
    SmApiCreateContext();
    require(SmApiUsdEnsurePluginRegistered() == SM_SUCCESS,
            "plugin unavailable");
    const auto ready = Clock::now();
    Imported b;
    std::vector<SmApiUsdBrepImportResult> results;
    const auto status =
        SmApiUsdImportBreps(argv[1], b.values, FALSE, FALSE, &results);
    require(status == SM_SUCCESS && b.values.size() == 1 && b.values[0] &&
                results.size() == 1 && results[0].m_sStatus == SM_SUCCESS &&
                results[0].m_pBrep == b.values[0],
            "strict complete import failed");
    const auto imported = Clock::now();
    SmTArray<SmFace *> faces;
    SmTArray<SmEdge *> edges;
    SmTArray<SmVertex *> vertices;
    b.values[0]->GetFaces(faces);
    b.values[0]->GetEdges(edges);
    b.values[0]->GetVertices(vertices);
    const std::string prefix = argv[2];
    for (const auto *suffix :
         {".controls.tsv", ".inventory.tsv", ".canonical.tsv", ".stages.tsv"})
      require(!fs::exists(prefix + suffix), "capture output exists");
    std::ofstream out(prefix + ".controls.tsv"),
        meta(prefix + ".inventory.tsv"), frames(prefix + ".canonical.tsv");
    require(out && meta && frames, "capture open failed");
    out << std::setprecision(17) << "kind\towner\tindex\thx\thy\thz\tw\n";
    meta << "faces\tedges\tvertices\tcontrols\n";
    uint64_t total = 0;
    for (ULONG i = 0; i < edges.GetSize(); ++i) {
      auto *c = SM_CAST_PTR(SmBSplineCurve, edges[i]->GetCurve());
      require(c && c->GetDim() == 3, "unsupported boundary curve");
      const auto natural = c->GetNaturalInterval(),
                 edge = edges[i]->GetInterval();
      for (double x :
           {natural.GetMin(), natural.GetMax(), edge.GetMin(), edge.GetMax()})
        finite_word(x);
      require(natural.GetMin() < natural.GetMax() &&
                  edge.GetMin() >= natural.GetMin() &&
                  edge.GetMax() <= natural.GetMax(),
              "curve interval not within natural support");
      const ULONG count = c->GetNumberControlPoints(), degree = c->GetDegree();
      SmTArray<double> knots;
      require(count > degree && degree > 0 &&
                  c->GetKnotsAll(knots) == SM_SUCCESS &&
                  knots.GetSize() == count + degree + 1,
              "unsupported curve knot word");
      for (ULONG k = 0; k < knots.GetSize(); ++k) {
        finite_word(knots[k]);
        require(k == 0 || knots[k] >= knots[k - 1], "unordered knots");
      }
      for (ULONG k = 0; k < count; ++k) {
        SmPoint3d h;
        double w = 0;
        require(c->GetControlPoint(SM_CP_HOMOGENEOUS_RATIONAL, k, h, w) ==
                    SM_SUCCESS,
                "homogeneous control capture failed");
        finite_word(w);
        require(w > 0, "nonpositive curve weight");
        for (unsigned j = 0; j < 3; ++j)
          finite_word(h[j]);
        out << "curve\t" << i << '\t' << k << '\t' << h[0] << '\t' << h[1]
            << '\t' << h[2] << '\t' << w << '\n';
        ++total;
      }
    }
    for (ULONG i = 0; i < vertices.GetSize(); ++i) {
      const auto &q = vertices[i]->GetPoint();
      for (unsigned j = 0; j < 3; ++j)
        finite_word(q[j]);
      out << "vertex\t" << i << "\t0\t" << q[0] << '\t' << q[1] << '\t' << q[2]
          << "\t1\n";
    }
    meta << faces.GetSize() << '\t' << edges.GetSize() << '\t'
         << vertices.GetSize() << '\t' << total << '\n';
    std::ifstream in(argv[3]);
    require(bool(in), "frame input missing");
    std::string header;
    std::getline(in, header);
    frames << std::setprecision(17)
           << "plane\tox\toy\toz\tux\tuy\tuz\tvx\tvy\tvz\n";
    ULONG index = 0, expected = 0;
    double x[9];
    while (in >> index) {
      require(index == expected++, "frame inventory noncanonical");
      for (double &v : x) {
        require(bool(in >> v), "incomplete frame");
        finite_word(v);
      }
      SmAxis2Placement placement;
      const SmPoint3d origin(x[0], x[1], x[2]);
      const SmVector3d z(x[3], x[4], x[5]), u(x[6], x[7], x[8]);
      require(placement.SetCanonical(origin, u, z * u) == SM_SUCCESS,
              "invalid plane frame");
      SmPlane *raw_plane = nullptr;
      require(SmPlane::CreateCanonical(*b.values[0]->GetContext(), placement,
                                       raw_plane) == SM_SUCCESS &&
                  raw_plane,
              "canonical plane creation failed");
      std::unique_ptr<SmPlane> represented(raw_plane);
      const auto scale = represented->GetUVScale();
      require(scale[0] == 1 && scale[1] == 1, "unexpected plane UVScale");
      SmPoint3d o;
      SmVector3d a, d;
      represented->GetPosition().GetCanonical(o, a, d);
      frames << index;
      for (const auto &q : {o, a, d})
        for (unsigned j = 0; j < 3; ++j) {
          finite_word(q[j]);
          frames << '\t' << q[j];
        }
      frames << '\n';
    }
    require(in.eof(), "malformed frame input");
    for (auto *f : {&out, &meta, &frames}) {
      f->close();
      require(!f->fail(), "capture close failed");
    }
    std::cout << "CAPTURE faces=" << faces.GetSize()
              << " edges=" << edges.GetSize() << " controls=" << total
              << " planes=" << expected << '\n';

    std::ofstream timings(prefix + ".stages.tsv");
    require(bool(timings), "timing output unavailable");
    auto seconds = [](auto a, auto z) {
      return std::chrono::duration<double>(z - a).count();
    };
    timings << std::setprecision(17) << "stage\tseconds\ncontext_plugin_setup\t"
            << seconds(begin, ready) << "\nstrict_source_import\t"
            << seconds(ready, imported) << "\ncontrols_vertices_final_frames\t"
            << seconds(imported, Clock::now()) << '\n';
    timings.close();
    require(!timings.fail(), "timing close failed");
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
