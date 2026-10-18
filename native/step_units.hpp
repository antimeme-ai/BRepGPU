#ifndef BREPGPU_STEP_UNITS_HPP
#define BREPGPU_STEP_UNITS_HPP
// Length units and uncertainty of STEP representation contexts, resolved from
// the unit entities and never from unit names (papers/2026-10-17-units-
// parameter-semantics.md UR-1, UR-2, UR-4, UR-6, UR-7), plus the XDE transfer
// fixed to millimetres by construction. Shared by the STEP source adapter and
// the reference source exporter.
//
// - A context's length unit: exactly one length unit in its
//   global_unit_assigned_context. An si_unit METRE gives 10^(prefix+3) mm; a
//   conversion_based_unit gives its positive measure value times its target's
//   mm, resolved recursively to depth 8. Anything else refuses.
// - Uncertainty is read per context, each measure in its own unit. A context
//   with one length measure is governed by it; with several, by the largest of
//   those named 'closure', 'distance_accuracy_value' (also written with spaces)
//   or 'point coincidence tolerance' (any case), else by the largest. The value
//   bounds every gap the sender anticipated, so the largest governs; the
//   'largest of several unnamed' fallback is our rule (G6 section 3 [own]), not
//   a source's. Absence is recorded, never defaulted.
// - The representations whose contexts govern written values are walked from
//   the shape representations OCCT transfers: one used by a
//   SHAPE_DEFINITION_REPRESENTATION, or reached from one by a representation
//   relationship or mapped item (as STEPControl_ActorRead transfers them).
#include <Interface_EntityIterator.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <Interface_Graph.hxx>
#include <StepBasic_ConversionBasedUnit.hxx>
#include <StepBasic_ConversionBasedUnitAndLengthUnit.hxx>
#include <StepBasic_MeasureWithUnit.hxx>
#include <StepBasic_NamedUnit.hxx>
#include <StepBasic_SiUnit.hxx>
#include <StepBasic_SiUnitAndLengthUnit.hxx>
#include <StepBasic_UncertaintyMeasureWithUnit.hxx>
#include <StepData_StepModel.hxx>
#include <StepGeom_GeomRepContextAndGlobUnitAssCtxAndGlobUncertaintyAssCtx.hxx>
#include <StepGeom_GeometricRepresentationContextAndGlobalUnitAssignedContext.hxx>
#include <StepRepr_GlobalUncertaintyAssignedContext.hxx>
#include <StepRepr_GlobalUnitAssignedContext.hxx>
#include <StepRepr_MappedItem.hxx>
#include <StepRepr_ProductDefinitionShape.hxx>
#include <StepRepr_ReprItemAndMeasureWithUnit.hxx>
#include <StepRepr_Representation.hxx>
#include <StepRepr_RepresentationContext.hxx>
#include <StepRepr_RepresentationMap.hxx>
#include <StepRepr_RepresentationRelationship.hxx>
#include <StepShape_ShapeDefinitionRepresentation.hxx>
#include <StepShape_ShapeRepresentation.hxx>
#include <TCollection_HAsciiString.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <charconv>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace brepgpu::step_units {
using ID = std::uint64_t;

inline ID label(const Handle(StepData_StepModel) &model, const Handle(Standard_Transient) &e) {
  if (e.IsNull())
    return 0;
  const int n = model->IdentLabel(e);
  if (n <= 0)
    throw std::runtime_error("missing original STEP entity label");
  return ID(n);
}

// Shortest decimal text that reads back to the same double.
inline std::string shortest(double value) {
  char text[64];
  const auto end = std::to_chars(text, text + sizeof text, value).ptr;
  return std::string(text, end);
}

// 10^e as the correctly rounded double of the decimal literal.
inline double decimal(int e) {
  static const double positive[] = {1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,
                                    1e8,  1e9,  1e10, 1e11, 1e12, 1e13, 1e14, 1e15,
                                    1e16, 1e17, 1e18, 1e19, 1e20, 1e21};
  static const double negative[] = {1e0,   1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6,  1e-7,
                                    1e-8,  1e-9,  1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15,
                                    1e-16, 1e-17, 1e-18, 1e-19, 1e-20, 1e-21};
  if (e < -21 || e > 21)
    throw std::runtime_error("SI prefix outside the supported decimal range");
  return e >= 0 ? positive[e] : negative[-e];
}

inline int si_exponent(const Handle(StepBasic_SiUnit) &unit) {
  if (!unit->HasPrefix())
    return 0;
  switch (unit->Prefix()) {
  case StepBasic_spExa: return 18;
  case StepBasic_spPeta: return 15;
  case StepBasic_spTera: return 12;
  case StepBasic_spGiga: return 9;
  case StepBasic_spMega: return 6;
  case StepBasic_spKilo: return 3;
  case StepBasic_spHecto: return 2;
  case StepBasic_spDeca: return 1;
  case StepBasic_spDeci: return -1;
  case StepBasic_spCenti: return -2;
  case StepBasic_spMilli: return -3;
  case StepBasic_spMicro: return -6;
  case StepBasic_spNano: return -9;
  case StepBasic_spPico: return -12;
  case StepBasic_spFemto: return -15;
  case StepBasic_spAtto: return -18;
  }
  throw std::runtime_error("unknown SI prefix");
}

inline bool is_length_unit(const Handle(StepBasic_NamedUnit) &unit) {
  return !unit.IsNull() && (unit->IsKind(STANDARD_TYPE(StepBasic_SiUnitAndLengthUnit)) ||
                            unit->IsKind(STANDARD_TYPE(StepBasic_ConversionBasedUnitAndLengthUnit)));
}

// Millimetres per unit of one length unit (UR-2). Names never set a factor.
inline double length_mm(const Handle(StepBasic_NamedUnit) &unit, const std::string &where,
                        std::vector<const Standard_Transient *> chain = {}) {
  if (std::find(chain.begin(), chain.end(), unit.get()) != chain.end())
    throw std::runtime_error(where + " has a conversion-based length unit that refers to itself "
                                     "(a cycle)");
  if (chain.size() >= 8)
    throw std::runtime_error(where + " has a conversion-based length unit nested deeper than 8");
  chain.push_back(unit.get());
  if (auto si = Handle(StepBasic_SiUnit)::DownCast(unit); !si.IsNull()) {
    if (si->Name() != StepBasic_sunMetre)
      throw std::runtime_error(where + " has an SI length unit that is not the metre");
    return decimal(si_exponent(si) + 3);
  }
  auto converted = Handle(StepBasic_ConversionBasedUnit)::DownCast(unit);
  if (converted.IsNull() || !is_length_unit(unit))
    throw std::runtime_error(where + " has an uninterpretable length unit");
  auto value = converted->ConversionFactor();
  auto measure = Handle(StepBasic_MeasureWithUnit)::DownCast(value);
  if (auto item = Handle(StepRepr_ReprItemAndMeasureWithUnit)::DownCast(value);
      measure.IsNull() && !item.IsNull())
    measure = item->GetMeasureWithUnit();
  if (measure.IsNull())
    throw std::runtime_error(where + " has a conversion-based length unit without a measure");
  auto target = measure->UnitComponent().NamedUnit();
  // The target is a length unit, or an SI unit named metre (written without
  // LENGTH_UNIT); the Python oracle accepts the same two forms.
  if (target.IsNull() ||
      !(is_length_unit(target) || !Handle(StepBasic_SiUnit)::DownCast(target).IsNull()))
    throw std::runtime_error(where + " has a length conversion whose target is not a length unit");
  const double factor = measure->ValueComponent();
  if (!std::isfinite(factor) || factor <= 0)
    throw std::runtime_error(where + " has a length conversion that is not a positive measure");
  return factor * length_mm(target, where, chain);
}

// UR-2: a name never sets a factor, but a name that says another factor is a
// finding. The factor a common length name states, if it is one.
inline std::optional<double> named_length_mm(std::string name) {
  std::transform(name.begin(), name.end(), name.begin(),
                 [](unsigned char c) { return char(std::tolower(c)); });
  static const std::map<std::string, double> names{
      {"mm", 1.},        {"millimetre", 1.}, {"millimeter", 1.}, {"cm", 10.},
      {"centimetre", 10.}, {"centimeter", 10.}, {"m", 1000.},     {"metre", 1000.},
      {"meter", 1000.},  {"in", 25.4},       {"inch", 25.4},     {"inches", 25.4},
      {"ft", 304.8},     {"foot", 304.8},    {"feet", 304.8},    {"mil", .0254},
      {"micron", .001},  {"micrometre", .001}, {"micrometer", .001}, {"km", 1e6},
      {"kilometre", 1e6}, {"kilometer", 1e6}};
  const auto found = names.find(name);
  if (found == names.end())
    return std::nullopt;
  return found->second;
}

inline Handle(StepRepr_GlobalUnitAssignedContext)
unit_context(const Handle(StepRepr_RepresentationContext) &context) {
  if (auto c = Handle(StepGeom_GeometricRepresentationContextAndGlobalUnitAssignedContext)::DownCast(
          context);
      !c.IsNull())
    return c->GlobalUnitAssignedContext();
  if (auto c = Handle(StepGeom_GeomRepContextAndGlobUnitAssCtxAndGlobUncertaintyAssCtx)::DownCast(
          context);
      !c.IsNull())
    return c->GlobalUnitAssignedContext();
  return {};
}

struct ContextLength {
  ID context = 0, unit = 0;
  bool declared = false;
  double mm = 0;
  // A conversion-based unit's own name and the factor that name states, when
  // it is a common length name (UR-2 finding when it disagrees with mm).
  std::string name;
  std::optional<double> named_mm;
};

// The one length unit of a representation context (UR-4.1-4.3).
inline ContextLength context_length(const Handle(StepData_StepModel) &model,
                                    const Handle(StepRepr_Representation) &rep) {
  ContextLength result;
  auto context = rep->ContextOfItems();
  if (context.IsNull())
    return result;
  result.context = label(model, context);
  const std::string where = "representation " + std::to_string(label(model, rep)) +
                            " context " + std::to_string(result.context);
  auto units = unit_context(context);
  if (units.IsNull())
    return result;
  for (int i = 1; i <= units->NbUnits(); ++i) {
    auto unit = units->UnitsValue(i);
    if (!is_length_unit(unit))
      continue;
    if (result.declared)
      throw std::runtime_error(where + " declares more than one length unit");
    result.declared = true;
    result.unit = label(model, unit);
    result.mm = length_mm(unit, where + " length unit " + std::to_string(result.unit));
    if (auto converted = Handle(StepBasic_ConversionBasedUnit)::DownCast(unit);
        !converted.IsNull() && !converted->Name().IsNull()) {
      result.name = converted->Name()->ToCString();
      result.named_mm = named_length_mm(result.name);
    }
  }
  return result;
}

struct Measure {
  ID id = 0, unit = 0;
  double value = 0, mm = 0;
  bool length = false, named = false;
};
struct ContextUncertainty {
  ID context = 0;
  std::vector<Measure> measures;
  bool governed = false;
  double mm = 0;
  std::string rule = "absent";
};

inline bool governing_name(const Handle(TCollection_HAsciiString) &name) {
  if (name.IsNull())
    return false;
  std::string text = name->ToCString();
  std::transform(text.begin(), text.end(), text.begin(),
                 [](unsigned char c) { return char(std::tolower(c)); });
  return text == "closure" || text == "distance_accuracy_value" ||
         text == "distance accuracy value" ||
         text == "point coincidence tolerance";
}

// UR-7: every uncertainty of one context, each in its own unit, and the
// value that governs the context.
inline ContextUncertainty context_uncertainty(const Handle(StepData_StepModel) &model,
                                              const Handle(StepRepr_Representation) &rep) {
  ContextUncertainty result;
  auto context = rep->ContextOfItems();
  if (context.IsNull())
    return result;
  result.context = label(model, context);
  auto complex =
      Handle(StepGeom_GeomRepContextAndGlobUnitAssCtxAndGlobUncertaintyAssCtx)::DownCast(context);
  if (complex.IsNull() || complex->GlobalUncertaintyAssignedContext().IsNull())
    return result;
  auto uncertainties = complex->GlobalUncertaintyAssignedContext();
  for (int i = 1; i <= uncertainties->NbUncertainty(); ++i) {
    auto u = uncertainties->UncertaintyValue(i);
    if (u.IsNull())
      continue;
    Measure m;
    m.id = label(model, u);
    m.value = u->ValueComponent();
    const std::string where = "uncertainty " + std::to_string(m.id);
    if (!std::isfinite(m.value) || m.value < 0)
      throw std::runtime_error(where + " is not a finite nonnegative value");
    auto unit = u->UnitComponent().NamedUnit();
    m.unit = label(model, unit);
    m.length = is_length_unit(unit);
    if (m.length)
      m.mm = m.value * length_mm(unit, where + " unit " + std::to_string(m.unit));
    m.named = governing_name(u->Name());
    result.measures.push_back(m);
  }
  const bool any_named = std::any_of(result.measures.begin(), result.measures.end(),
                                     [](const Measure &m) { return m.length && m.named; });
  for (const auto &m : result.measures)
    if (m.length && (m.named || !any_named) && (!result.governed || m.mm > result.mm)) {
      result.governed = true;
      result.mm = m.mm;
    }
  const auto lengths = std::count_if(result.measures.begin(), result.measures.end(),
                                     [](const Measure &m) { return m.length; });
  result.rule = !result.governed ? "absent"
                : lengths == 1   ? "single length measure"
                : any_named      ? "largest named length measure"
                                 : "largest length measure";
  return result;
}

// UR-7: a declared value below 1e-15 of the definition's diagonal (a declared
// 0 among them) makes no usable claim; absence is a separate state.
inline bool uncertainty_usable(double mm, double diagonal) {
  return mm > 0 && mm >= 1e-15 * diagonal;
}
// A solid's governing uncertainty: of the contexts of the representations that
// hold it, the largest governed value (each sender claims its own gaps).
struct HeldUncertainty {
  ContextUncertainty governing;
  ID holder = 0;
};
inline HeldUncertainty
held_uncertainty(const Handle(StepData_StepModel) &model,
                 const std::vector<Handle(StepRepr_Representation)> &holders) {
  HeldUncertainty out;
  for (const auto &rep : holders) {
    auto uncertainty = context_uncertainty(model, rep);
    if (!out.holder ||
        (uncertainty.governed && (!out.governing.governed || uncertainty.mm > out.governing.mm))) {
      out.governing = uncertainty;
      out.holder = label(model, rep);
    }
  }
  return out;
}

// Shape representations the transfer reaches, as STEPControl_ActorRead does.
class RepresentationGraph {
public:
  explicit RepresentationGraph(const Interface_Graph &graph) : graph_(graph) {}
  bool sdr_used(const Handle(StepRepr_Representation) &rep) const {
    auto users = graph_.Sharings(rep);
    for (users.Start(); users.More(); users.Next())
      if (auto sdr = Handle(StepShape_ShapeDefinitionRepresentation)::DownCast(users.Value());
          !sdr.IsNull() && sdr->UsedRepresentation() == rep)
        return true;
    return false;
  }
  std::vector<Handle(StepRepr_Representation)>
  callers(const Handle(StepRepr_Representation) &rep) const {
    std::vector<Handle(StepRepr_Representation)> out;
    auto users = graph_.Sharings(rep);
    for (users.Start(); users.More(); users.Next()) {
      if (auto relation = Handle(StepRepr_RepresentationRelationship)::DownCast(users.Value());
          !relation.IsNull()) {
        auto other = relation->Rep1() == rep ? relation->Rep2() : relation->Rep1();
        if (!Handle(StepShape_ShapeRepresentation)::DownCast(other).IsNull())
          out.push_back(other);
      }
      if (auto map = Handle(StepRepr_RepresentationMap)::DownCast(users.Value());
          !map.IsNull() && map->MappedRepresentation() == rep) {
        auto items = graph_.Sharings(map);
        for (items.Start(); items.More(); items.Next())
          if (auto item = Handle(StepRepr_MappedItem)::DownCast(items.Value()); !item.IsNull()) {
            auto holders = graph_.Sharings(item);
            for (holders.Start(); holders.More(); holders.Next())
              if (auto holder = Handle(StepShape_ShapeRepresentation)::DownCast(holders.Value());
                  !holder.IsNull())
                out.push_back(holder);
          }
      }
    }
    return out;
  }
  // Breadth-first over relationship/mapped-item links, each representation
  // once, depth <= 8. Returns the representations on the path from start to
  // the first SDR-used one (start included), or nothing when none is reached.
  std::vector<Handle(StepRepr_Representation)>
  transfer_path(const Handle(StepRepr_Representation) &start) const {
    std::map<const Standard_Transient *, std::size_t> seen{{start.get(), 0}};
    struct Node {
      Handle(StepRepr_Representation) rep;
      int depth;
      std::size_t parent;
    };
    std::vector<Node> queue{{start, 0, 0}};
    for (std::size_t i = 0; i < queue.size(); ++i) {
      const auto rep = queue[i].rep;
      if (sdr_used(rep)) {
        std::vector<Handle(StepRepr_Representation)> path;
        for (std::size_t k = i;; k = queue[k].parent) {
          path.push_back(queue[k].rep);
          if (k == 0)
            break;
        }
        return path;
      }
      if (queue[i].depth >= 8)
        continue;
      for (const auto &caller : callers(rep))
        if (seen.emplace(caller.get(), queue.size()).second)
          queue.push_back({caller, queue[i].depth + 1, i});
    }
    return {};
  }
  bool transferred(const Handle(StepRepr_Representation) &start) const {
    return !transfer_path(start).empty();
  }
  // Shape representations directly holding an item that OCCT transfers.
  std::vector<Handle(StepRepr_Representation)>
  holders(const Handle(Standard_Transient) &item) const {
    std::vector<Handle(StepRepr_Representation)> out;
    auto users = graph_.Sharings(item);
    for (users.Start(); users.More(); users.Next())
      if (auto rep = Handle(StepShape_ShapeRepresentation)::DownCast(users.Value());
          !rep.IsNull() && transferred(rep))
        out.push_back(rep);
    return out;
  }

private:
  const Interface_Graph &graph_;
};

// Every shape representation used by a SHAPE_DEFINITION_REPRESENTATION of a
// product definition: it holds the product's shape and its placements.
inline std::vector<Handle(StepRepr_Representation)>
product_representations(const Handle(StepData_StepModel) &model) {
  std::vector<Handle(StepRepr_Representation)> out;
  for (int i = 1; i <= model->NbEntities(); ++i) {
    auto sdr = Handle(StepShape_ShapeDefinitionRepresentation)::DownCast(model->Value(i));
    if (sdr.IsNull())
      continue;
    auto shape = Handle(StepRepr_ProductDefinitionShape)::DownCast(
        sdr->Definition().PropertyDefinition());
    if (shape.IsNull() || shape->Definition().ProductDefinition().IsNull())
      continue;
    if (auto rep = Handle(StepShape_ShapeRepresentation)::DownCast(sdr->UsedRepresentation());
        !rep.IsNull())
      out.push_back(rep);
  }
  return out;
}

// Admission today is millimetres only (lifting it is M8, bead
// brepgpu-12s.25.6). The refusal comes from the resolved factor, never a name.
inline void require_millimetre(const Handle(StepData_StepModel) &model,
                               const Handle(StepRepr_Representation) &rep,
                               const ContextLength &length) {
  const std::string where = "representation " + std::to_string(label(model, rep)) +
                            " context " + std::to_string(length.context);
  if (!length.context)
    throw std::runtime_error("representation " + std::to_string(label(model, rep)) +
                             " has no readable representation context, so no length unit");
  if (!length.declared)
    throw std::runtime_error(where + " declares no length unit");
  if (length.mm != 1.0)
    throw std::runtime_error(where + " length unit " + std::to_string(length.unit) +
                             " resolves to " + shortest(length.mm) +
                             " mm per unit; non-millimetre sources are refused");
}

// STEPCAFControl_Reader::Transfer replaces the reader's length unit with the
// document's, or with the global xstep.cascade.unit for a document without
// one (STEPCAFControl_Reader::prepareUnits). Fix the document unit before the
// transfer and verify both after it, so the output is in millimetres by
// construction.
inline void prepare_millimetre_transfer(const Handle(TDocStd_Document) &doc) {
  XCAFDoc_DocumentTool::SetLengthUnit(doc, 1., UnitsMethods_LengthUnit_Millimeter);
}
inline void require_transferred_millimetre(const Handle(TDocStd_Document) &doc,
                                           const Handle(StepData_StepModel) &model) {
  double unit = 0;
  if (!XCAFDoc_DocumentTool::GetLengthUnit(doc, unit, UnitsMethods_LengthUnit_Millimeter) ||
      unit != 1. || model.IsNull() || model->LocalLengthUnit() != 1.)
    throw std::runtime_error("STEP transfer length unit is not the millimetre");
}
} // namespace brepgpu::step_units
#endif
