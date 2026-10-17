#include "prepare.hpp"
#include "../mesher/exact.hpp"
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BinTools_ShapeSet.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <NCollection_IndexedMap.hxx>
#include <ShapeBuild_ReShape.hxx>
#include <ShapeExtend_WireData.hxx>
#include <ShapeUpgrade_ShapeDivideAngle.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <TopoDS.hxx>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace brepgpu {
namespace {
using Map = NCollection_IndexedMap<TopoDS_Shape, TopTools_ShapeMapHasher>;
using Rational = bgmesh::Rational;
using Clock = std::chrono::steady_clock;
void require(bool yes, const char *what) {
  if (!yes)
    throw std::runtime_error(what);
}
Handle(Geom_Surface) basis(Handle(Geom_Surface) s) {
  while (auto t = Handle(Geom_RectangularTrimmedSurface)::DownCast(s)) {
    s = t->BasisSurface();
  }
  return s;
}
std::vector<double> coefficients(const Handle(Geom_Surface) & surface) {
  auto s = basis(surface);
  GeomAdaptor_Surface a(s);
  std::vector<double> out{static_cast<double>(a.GetType())};
  auto ax = [&](const gp_Ax3 &p) {
    auto o = p.Location();
    auto x = p.XDirection(), y = p.YDirection(), z = p.Direction();
    out.insert(out.end(), {o.X(), o.Y(), o.Z(), x.X(), x.Y(), x.Z(), y.X(), y.Y(), y.Z(), z.X(),
                           z.Y(), z.Z()});
  };
  switch (a.GetType()) {
  case GeomAbs_Plane:
    ax(a.Plane().Position());
    break;
  case GeomAbs_Cylinder:
    ax(a.Cylinder().Position());
    out.push_back(a.Cylinder().Radius());
    break;
  case GeomAbs_Cone:
    ax(a.Cone().Position());
    out.push_back(a.Cone().RefRadius());
    out.push_back(a.Cone().SemiAngle());
    break;
  case GeomAbs_Sphere:
    ax(a.Sphere().Position());
    out.push_back(a.Sphere().Radius());
    break;
  case GeomAbs_Torus:
    ax(a.Torus().Position());
    out.push_back(a.Torus().MajorRadius());
    out.push_back(a.Torus().MinorRadius());
    break;
  case GeomAbs_BSplineSurface: {
    auto n = Handle(Geom_BSplineSurface)::DownCast(s);
    out.insert(out.end(),
               {static_cast<double>(n->UDegree()), static_cast<double>(n->VDegree()),
                static_cast<double>(n->NbUPoles()), static_cast<double>(n->NbVPoles()),
                static_cast<double>(n->IsUPeriodic()), static_cast<double>(n->IsVPeriodic())});
    for (int i = 1; i <= n->NbUKnots(); ++i) {
      out.push_back(n->UKnot(i));
      out.push_back(n->UMultiplicity(i));
    }
    for (int i = 1; i <= n->NbVKnots(); ++i) {
      out.push_back(n->VKnot(i));
      out.push_back(n->VMultiplicity(i));
    }
    for (int i = 1; i <= n->NbUPoles(); ++i)
      for (int j = 1; j <= n->NbVPoles(); ++j) {
        auto p = n->Pole(i, j);
        out.insert(out.end(), {p.X(), p.Y(), p.Z(), n->Weight(i, j)});
      }
    break;
  }
  default:
    throw std::runtime_error("unsupported coefficient comparison carrier");
  }
  return out;
}
std::vector<double> curve_coefficients(Handle(Geom_Curve) c) {
  if (c.IsNull())
    return {};
  while (auto trimmed = Handle(Geom_TrimmedCurve)::DownCast(c))
    c = trimmed->BasisCurve();
  GeomAdaptor_Curve a(c);
  std::vector<double> result{static_cast<double>(a.GetType())};
  auto p = [&](const gp_Pnt &v) { result.insert(result.end(), {v.X(), v.Y(), v.Z()}); };
  auto d = [&](const gp_Dir &v) { result.insert(result.end(), {v.X(), v.Y(), v.Z()}); };
  auto ax = [&](const gp_Ax2 &v) {
    p(v.Location());
    d(v.XDirection());
    d(v.YDirection());
    d(v.Direction());
  };
  switch (a.GetType()) {
  case GeomAbs_Line:
    p(a.Line().Location());
    d(a.Line().Direction());
    break;
  case GeomAbs_Circle:
    ax(a.Circle().Position());
    result.push_back(a.Circle().Radius());
    break;
  case GeomAbs_Ellipse:
    ax(a.Ellipse().Position());
    result.push_back(a.Ellipse().MajorRadius());
    result.push_back(a.Ellipse().MinorRadius());
    break;
  case GeomAbs_BSplineCurve: {
    auto b = Handle(Geom_BSplineCurve)::DownCast(c);
    result.insert(result.end(),
                  {static_cast<double>(b->Degree()), static_cast<double>(b->IsPeriodic()),
                   static_cast<double>(b->NbPoles()), static_cast<double>(b->NbKnots())});
    for (int i = 1; i <= b->NbKnots(); ++i) {
      result.push_back(b->Knot(i));
      result.push_back(b->Multiplicity(i));
    }
    for (int i = 1; i <= b->NbPoles(); ++i) {
      p(b->Pole(i));
      result.push_back(b->Weight(i));
    }
    break;
  }
  default:
    throw std::runtime_error("unsupported source curve coefficient comparison");
  }
  return result;
}

Rational remainder(const Rational &x, const Rational &period) {
  const auto q = x.divide(period);
  auto n = q.numerator().sign() < 0 ? q.numerator().negative() : bgfd::Integer(q.numerator());
  auto divided = bgfd::Integer::divmod_nonnegative(n, q.denominator());
  auto fraction = Rational::from_ratio(std::move(divided.second), bgfd::Integer(q.denominator()));
  if (q.sign() < 0 && fraction.sign() != 0)
    fraction = Rational(x.budget(), 1).subtract(fraction);
  return fraction.multiply(period);
}
bool image_equal(bgfd::Budget &budget, double first, double last, double period,
                 std::span<const std::array<double, 2>> pieces,
                 std::span<const int> directions = {}) {
  if ((!directions.empty() && directions.size() != pieces.size()) ||
      std::any_of(directions.begin(), directions.end(),
                  [](int direction) { return direction != 1; }))
    return false;
  if (!std::isfinite(first) || !std::isfinite(last) || !std::isfinite(period) || last <= first ||
      period < 0 || pieces.empty())
    return false;
  const auto a = Rational::from_double(budget, first), b = Rational::from_double(budget, last);
  const auto width = b.subtract(a), p = Rational::from_double(budget, period);
  if (period > 0 && width.compare(p) > 0)
    return false;
  std::pmr::vector<std::array<Rational, 2>> spans(&budget);
  for (const auto &row : pieces) {
    if (!std::isfinite(row[0]) || !std::isfinite(row[1]) || row[1] <= row[0])
      return false;
    const auto l = Rational::from_double(budget, row[0]), r = Rational::from_double(budget, row[1]);
    const auto length = r.subtract(l);
    if (period == 0) {
      spans.push_back({l.subtract(a), r.subtract(a)});
      continue;
    }
    if (length.compare(p) > 0)
      return false;
    auto start = remainder(l.subtract(a), p);
    auto end = start.add(length);
    if (end.compare(p) <= 0)
      spans.push_back({std::move(start), std::move(end)});
    else {
      spans.push_back({std::move(start), p});
      spans.push_back({Rational(budget), end.subtract(p)});
    }
  }
  std::sort(spans.begin(), spans.end(), [&](const auto &x, const auto &y) {
    budget.charge(1);
    const int c = x[0].compare(y[0]);
    return c ? c < 0 : x[1].compare(y[1]) < 0;
  });
  Rational cursor(budget);
  for (const auto &row : spans) {
    if (row[0].compare(cursor) != 0 || row[1].compare(width) > 0)
      return false;
    cursor = row[1];
  }
  return cursor.compare(width) == 0;
}
uint64_t index(const BinTools_ShapeSet &set, const TopoDS_Shape &s) {
  const int i = set.Index(s.Located(TopLoc_Location()));
  require(i > 0, "source/prepared shape identity missing");
  return static_cast<uint64_t>(i);
}
std::array<double, 3> point(const TopoDS_Vertex &v) {
  const auto p = BRep_Tool::Pnt(v);
  return {p.X(), p.Y(), p.Z()};
}
struct Vertex {
  TopoDS_Vertex shape;
  std::array<double, 3> xyz;
  double tolerance;
  uint64_t source_index;
};
struct Edge {
  TopoDS_Edge shape;
  std::vector<double> coefficients;
  double first, last, period, tolerance;
  uint64_t source_index;
  TopoDS_Vertex first_vertex, last_vertex;
};
struct Face {
  TopoDS_Face shape;
  std::vector<double> coefficients;
  double tolerance;
};
Handle(Geom_Curve) curve_basis(Handle(Geom_Curve) c) {
  while (auto t = Handle(Geom_TrimmedCurve)::DownCast(c))
    c = t->BasisCurve();
  return c;
}
std::vector<brepgpu::SourceUVFace> bindings(const TopoDS_Shape &shape,
                                            const BinTools_ShapeSet &source) {
  std::vector<brepgpu::SourceUVFace> result;
  uint64_t use_id = 0;
  for (TopExp_Explorer fi(shape, TopAbs_FACE); fi.More(); fi.Next()) {
    brepgpu::SourceUVFace f;
    f.face = TopoDS::Face(fi.Current());
    f.id = index(source, f.face);
    for (TopExp_Explorer wi(f.face, TopAbs_WIRE); wi.More(); wi.Next()) {
      brepgpu::SourceUVLoop loop;
      loop.wire = TopoDS::Wire(wi.Current());
      loop.id = index(source, loop.wire);
      Handle(ShapeExtend_WireData) storage = new ShapeExtend_WireData(loop.wire, true);
      Handle(ShapeExtend_WireData) data = new ShapeExtend_WireData(loop.wire, false);
      std::vector<std::pair<uint64_t, int>> original_word, connected_word;
      for (int j = 1; j <= storage->NbEdges(); ++j)
        original_word.emplace_back(index(source, storage->Edge(j)),
                                   static_cast<int>(storage->Edge(j).Orientation()));
      for (int j = 1; j <= data->NbEdges(); ++j)
        connected_word.emplace_back(index(source, data->Edge(j)),
                                    static_cast<int>(data->Edge(j).Orientation()));
      std::sort(original_word.begin(), original_word.end());
      std::sort(connected_word.begin(), connected_word.end());
      require(original_word == connected_word,
              "connected wire traversal lost original occurrences");
      for (int j = 1; j <= data->NbEdges(); ++j) {
        brepgpu::SourceUVUse use;
        use.id = ++use_id;
        use.edge = data->Edge(j);
        use.edge_id = index(source, use.edge);
        TopExp::Vertices(use.edge, use.start, use.end, true);
        require(!use.start.IsNull() && !use.end.IsNull(), "source edge has no original endpoint");
        use.start_id = index(source, use.start);
        use.end_id = index(source, use.end);
        loop.uses.push_back(use);
      }
      f.loops.push_back(std::move(loop));
    }
    result.push_back(std::move(f));
  }
  return result;
}
} // namespace
bool periodic_image_equal(double first, double last, double period,
                          std::span<const std::array<double, 2>> pieces,
                          std::span<const int> directions) {
  bgfd::Budget budget({uint64_t{1} << 26, uint64_t{1} << 32});
  return image_equal(budget, first, last, period, pieces, directions);
}
PreparedPeriodic prepare_periodic(const TopoDS_Shape &original, const SourceUVOptions &options,
                                  const BinTools_ShapeSet *source_indices) {
  require(!options.prepare_query, "periodic reference preparation requires RAW policy");
  require(!original.IsNull() && original.ShapeType() == TopAbs_SOLID &&
              original.Location().IsIdentity(),
          "periodic preparation requires one identity-located solid");
  PreparedPeriodic out;
  auto start = Clock::now();
  auto record = [&](const char *name) {
    const auto now = Clock::now();
    out.stages.push_back({name, std::chrono::duration<double>(now - start).count()});
    start = now;
  };
  BinTools_ShapeSet owned_source;
  if (!source_indices) {
    owned_source.Add(original);
    source_indices = &owned_source;
  }
  const auto &source = *source_indices;
  out.original_shapes = static_cast<uint64_t>(source.NbShapes());
  const auto source_faces = bindings(original, source);
  out.original_faces = source_faces.size();
  require(!source_faces.empty(), "source solid has no faces");
  auto raw = prepare_source_uv(original, source_faces, options);
  out.raw_stats = raw.stats;
  out.chart_approximations = raw.chart_approximations.size();
  record("raw_pcurve");
  std::vector<Face> faces;
  std::vector<Edge> edges;
  std::vector<Vertex> vertices;
  Map edge_map, vertex_map;
  bool periodic = false;
  double tolerance = 0;
  for (const auto &original_face : source_faces) {
    auto f = TopoDS::Face(raw.mapped(original_face.face));
    auto s = BRep_Tool::Surface(f);
    auto basis_surface = basis(s);
    GeomAdaptor_Surface ad(basis_surface);
    const auto type = ad.GetType();
    periodic |= basis_surface->IsUPeriodic() && (type == GeomAbs_Cylinder || type == GeomAbs_Cone ||
                                                 type == GeomAbs_Sphere || type == GeomAbs_Torus);
    const auto fs = coefficients(s);
    require(BRep_Tool::Tolerance(f) == BRep_Tool::Tolerance(original_face.face),
            "RAW source face tolerance changed");
    require(fs == coefficients(BRep_Tool::Surface(original_face.face)),
            "RAW original surface changed");
    faces.push_back({f, fs, BRep_Tool::Tolerance(f)});
    tolerance = std::max(tolerance, BRep_Tool::Tolerance(f));
  }
  require(periodic, "source has no supported analytic periodic chart");
  NCollection_DataMap<TopoDS_Shape, uint64_t, TopTools_ShapeMapHasher> source_ids;
  for (const auto kind : {TopAbs_VERTEX, TopAbs_EDGE}) {
    Map originals;
    TopExp::MapShapes(original, kind, originals);
    for (int i = 1; i <= originals.Extent(); ++i) {
      const auto copied = raw.mapped(originals(i));
      source_ids.Bind(copied.Located(TopLoc_Location()), index(source, originals(i)));
      if (kind == TopAbs_VERTEX) {
        const auto a = TopoDS::Vertex(originals(i)), b = TopoDS::Vertex(copied);
        require(point(a) == point(b) && BRep_Tool::Tolerance(a) == BRep_Tool::Tolerance(b),
                "RAW source vertex geometry/tolerance changed");
      } else {
        const auto a = TopoDS::Edge(originals(i)), b = TopoDS::Edge(copied);
        double af = 0, al = 0, bf = 0, bl = 0;
        const auto ac = BRep_Tool::Curve(a, af, al), bc = BRep_Tool::Curve(b, bf, bl);
        require(curve_coefficients(ac) == curve_coefficients(bc) && af == bf && al == bl &&
                    BRep_Tool::Tolerance(a) == BRep_Tool::Tolerance(b),
                "RAW source edge geometry/range/tolerance changed");
      }
    }
  }
  TopExp::MapShapes(raw.raw_shape, TopAbs_EDGE, edge_map);
  TopExp::MapShapes(raw.raw_shape, TopAbs_VERTEX, vertex_map);
  for (int i = 1; i <= vertex_map.Extent(); ++i) {
    auto v = TopoDS::Vertex(vertex_map(i));
    vertices.push_back(
        {v, point(v), BRep_Tool::Tolerance(v), source_ids.Find(v.Located(TopLoc_Location()))});
    tolerance = std::max(tolerance, BRep_Tool::Tolerance(v));
  }
  for (int i = 1; i <= edge_map.Extent(); ++i) {
    auto e = TopoDS::Edge(edge_map(i).Oriented(TopAbs_FORWARD));
    double first, last;
    auto c = BRep_Tool::Curve(e, first, last);
    auto cb = curve_basis(c);
    TopoDS_Vertex a, b;
    TopExp::Vertices(e, a, b, true);
    require(!a.IsNull() && !b.IsNull(), "source edge endpoints absent");
    if (!c.IsNull()) {
      require(c->Value(first).Distance(BRep_Tool::Pnt(a)) <= options.source_residual_mm &&
                  c->Value(last).Distance(BRep_Tool::Pnt(b)) <= options.source_residual_mm,
              "source vertex/curve endpoint residual exceeds declared grade");
    }
    edges.push_back({e, curve_coefficients(c), first, last,
                     cb.IsNull() || !cb->IsPeriodic() ? 0 : cb->Period(), BRep_Tool::Tolerance(e),
                     source_ids.Find(e.Located(TopLoc_Location())), a, b});
    tolerance = std::max(tolerance, BRep_Tool::Tolerance(e));
  }
  require(std::isfinite(tolerance) && tolerance > 0, "invalid input topology tolerance");
  record("source_snapshot");
  ShapeUpgrade_ShapeDivideAngle divide(std::acos(-1.), raw.raw_shape);
  divide.SetPrecision(tolerance);
  divide.SetMaxTolerance(tolerance);
  divide.SetMinTolerance(std::min(tolerance, 1.e-7));
  divide.Perform();
  require(!divide.Status(ShapeExtend_FAIL), "angle subdivision failed");
  out.shape = divide.Result();
  require(!out.shape.IsNull() && out.shape.ShapeType() == TopAbs_SOLID,
          "angle subdivision lost original solid");
  record("angle_divide");
  const auto context = divide.GetContext();
  require(!context.IsNull(), "subdivision history absent");
  BinTools_ShapeSet prepared;
  prepared.Add(out.shape);
  out.prepared_shapes = static_cast<uint64_t>(prepared.NbShapes());
  Map covered_faces, covered_edges, covered_vertices;
  for (size_t i = 0; i < faces.size(); ++i) {
    const auto &f = faces[i];
    auto descendants = context->Apply(f.shape);
    require(!descendants.IsNull(), "original face history lost");
    size_t count = 0;
    for (TopExp_Explorer it(descendants, TopAbs_FACE); it.More(); it.Next()) {
      auto part = TopoDS::Face(it.Current());
      require(part.Orientation() == f.shape.Orientation(), "source face orientation changed");
      require(!covered_faces.Contains(part), "multiple original faces share one descendant");
      require(coefficients(BRep_Tool::Surface(part)) == f.coefficients,
              "original surface basis coefficients changed");
      require(BRep_Tool::Tolerance(part) == f.tolerance, "source face tolerance changed");
      out.tolerances.push_back(
          {"face", index(prepared, part), f.tolerance, BRep_Tool::Tolerance(part)});
      covered_faces.Add(part);
      ++count;
      out.history.push_back({"face", index(source, source_faces[i].face), index(prepared, part), 0,
                             0, 0, 0, 0, part.Orientation() == TopAbs_FORWARD ? 1 : -1});
    }
    require(count > 0, "original face has no descendant");
  }
  Map final_faces;
  TopExp::MapShapes(out.shape, TopAbs_FACE, final_faces);
  require(covered_faces.Extent() == final_faces.Extent(), "unowned output face");
  out.prepared_faces = static_cast<uint64_t>(final_faces.Extent());
  for (const auto &v : vertices) {
    auto descendant = context->Apply(v.shape);
    require(!descendant.IsNull() && descendant.ShapeType() == TopAbs_VERTEX,
            "source vertex history lost");
    auto part = TopoDS::Vertex(descendant);
    require(point(part) == v.xyz && BRep_Tool::Tolerance(part) == v.tolerance,
            "source vertex geometry/tolerance changed");
    out.tolerances.push_back(
        {"vertex", index(prepared, part), v.tolerance, BRep_Tool::Tolerance(part)});
    require(!covered_vertices.Contains(part), "multiple original vertices share one descendant");
    covered_vertices.Add(part);
    out.history.push_back({"vertex", v.source_index, index(prepared, part)});
  }
  bgfd::Budget budget({uint64_t{1} << 26, uint64_t{1} << 32});
  for (const auto &e : edges) {
    auto descendant = context->Apply(e.shape);
    require(!descendant.IsNull(), "source edge history lost");
    std::vector<TopoDS_Edge> parts;
    Map unique;
    std::vector<std::array<double, 2>> spans;
    std::vector<int> directions;
    for (TopExp_Explorer it(descendant, TopAbs_EDGE); it.More(); it.Next()) {
      auto part = TopoDS::Edge(it.Current());
      require(!unique.Contains(part), "duplicate edge descendant");
      unique.Add(part);
      double first, last;
      auto c = BRep_Tool::Curve(part, first, last);
      require(curve_coefficients(c) == e.coefficients, "source curve basis coefficients changed");
      require(BRep_Tool::Tolerance(part) == e.tolerance,
              "source edge descendant tolerance changed");
      if (e.period == 0 && !c.IsNull() && !e.first_vertex.IsSame(e.last_vertex)) {
        TopoDS_Vertex a, b;
        TopExp::Vertices(TopoDS::Edge(part.Oriented(TopAbs_FORWARD)), a, b, true);
        const double restored_first = a.IsSame(context->Apply(e.first_vertex)) ? e.first : first;
        const double restored_last = b.IsSame(context->Apply(e.last_vertex)) ? e.last : last;
        require(restored_last > restored_first, "restored source range is not positive");
        if (restored_first != first || restored_last != last) {
          out.range_restorations.push_back(
              {e.source_index, index(prepared, part), first, last, restored_first, restored_last,
               BRep_Tool::SameRange(part), BRep_Tool::SameParameter(part)});
          BRep_Builder builder;
          builder.Range(part, restored_first, restored_last, true);
          builder.SameRange(part, false);
          builder.SameParameter(part, false);
          first = restored_first;
          last = restored_last;
        }
      }
      require(part.Orientation() == TopAbs_FORWARD, "source parameter traversal reversed");
      if (!c.IsNull()) {
        spans.push_back({first, last});
        directions.push_back(1);
      }
      out.tolerances.push_back(
          {"edge", index(prepared, part), e.tolerance, BRep_Tool::Tolerance(part)});
      parts.push_back(part);
      require(!covered_edges.Contains(part), "multiple original edges share one descendant");
      covered_edges.Add(part);
    }
    require(!parts.empty(), "source edge has no descendant");
    if (!e.coefficients.empty() &&
        !image_equal(budget, e.first, e.last, e.period, spans, directions)) {
      std::ostringstream detail;
      detail << std::setprecision(17)
             << "original curve image coverage changed: source_index=" << e.source_index
             << " range=[" << e.first << ',' << e.last << "] period=" << e.period;
      for (const auto &span : spans)
        detail << " child=[" << span[0] << ',' << span[1] << ']';
      throw std::runtime_error(detail.str());
    }
    auto cursor = TopoDS::Vertex(context->Apply(e.first_vertex));
    const auto end = TopoDS::Vertex(context->Apply(e.last_vertex));
    std::vector<bool> used(parts.size(), false);
    std::vector<size_t> chain;
    for (size_t step = 0; step < parts.size(); ++step) {
      size_t chosen = parts.size();
      TopoDS_Vertex next;
      for (size_t j = 0; j < parts.size(); ++j)
        if (!used[j]) {
          TopoDS_Vertex a, b;
          TopExp::Vertices(parts[j], a, b, true);
          if (a.IsSame(cursor)) {
            require(chosen == parts.size(), "branched original edge traversal");
            chosen = j;
            next = b;
          }
        }
      require(chosen != parts.size(), "disconnected/reversed original edge traversal");
      used[chosen] = true;
      chain.push_back(chosen);
      cursor = next;
    }
    require(cursor.IsSame(end), "original edge endpoint incidence changed");
    for (size_t order = 0; order < chain.size(); ++order) {
      const size_t j = chain[order];
      TopoDS_Vertex a, b;
      TopExp::Vertices(parts[j], a, b, true);
      double first, last;
      BRep_Tool::Curve(parts[j], first, last);
      out.history.push_back({"edge", e.source_index, index(prepared, parts[j]), e.first, e.last,
                             first, last, e.period,
                             parts[j].Orientation() == TopAbs_FORWARD ? 1 : -1,
                             static_cast<uint64_t>(order), index(prepared, a), index(prepared, b),
                             index(prepared, context->Apply(e.first_vertex)),
                             index(prepared, context->Apply(e.last_vertex))});
    }
  }
  Map all_edges, all_vertices;
  TopExp::MapShapes(out.shape, TopAbs_EDGE, all_edges);
  TopExp::MapShapes(out.shape, TopAbs_VERTEX, all_vertices);
  for (int i = 1; i <= all_edges.Extent(); ++i)
    if (!covered_edges.Contains(all_edges(i))) {
      auto e = TopoDS::Edge(all_edges(i));
      out.history.push_back({"generated_edge", 0, index(prepared, e)});
      out.tolerances.push_back({"generated_edge", index(prepared, e), 0, BRep_Tool::Tolerance(e)});
    }
  for (int i = 1; i <= all_vertices.Extent(); ++i)
    if (!covered_vertices.Contains(all_vertices(i))) {
      auto v = TopoDS::Vertex(all_vertices(i));
      out.history.push_back({"generated_vertex", 0, index(prepared, v)});
      out.tolerances.push_back(
          {"generated_vertex", index(prepared, v), 0, BRep_Tool::Tolerance(v)});
    }
  record("history_validation");
  return out;
}
} // namespace brepgpu
