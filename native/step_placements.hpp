#ifndef BREPGPU_STEP_PLACEMENTS_HPP
#define BREPGPU_STEP_PLACEMENTS_HPP
// Usage placements read from the raw STEP entities, and the mapped-item
// structures the source tables cannot carry, refused by name. G1's authority
// rule (papers/2026-10-17-occt-ingress-translation-edits.md section 4): the
// raw entity is authoritative, OCCT's translated location only an evaluator.
// Written for the STEP source adapter; the reference exporter may share it.
//
// Placements. Each NEXT_ASSEMBLY_USAGE_OCCURRENCE is placed by exactly one
// CONTEXT_DEPENDENT_SHAPE_REPRESENTATION whose representation_relation is a
// representation_relationship_with_transformation. The reading transcribes the
// independent oracle (python/brepgpu/imported_oracles.py
// _original_usage_transforms, original_frame, original_cartesian_operator), so
// the two are separate transcriptions of one standard:
// - cartesian_transformation_operator_3d (ISO 10303-42, AP242 MIM long form):
//   scl = NVL(scale, 1), WR1 scl > 0; u = base_axis(3, axis1, axis2, axis3)
//   with axis3 defaulting to (0, 0, 1) and u[2] keeping axis2's sense, so a
//   mirror has determinant -1; P -> local_origin + scl u P from rep_1 into
//   rep_2. OCCT 8.0.1 (StepToGeom.cxx:2153-2213) inverts the scale, reads a
//   mirror as a rotation and defaults axis3 to axis1 x axis2 (G1 P1).
// - item_defined_transformation: the child-side frame A maps onto the
//   parent-side frame B, C = B A^-1 (PDM Schema Usage Guide r4.3 section 3.4.3,
//   Example 14). OCCT 8.0.1 composes A^-1 B (STEPControl_ActorRead.cxx:2488,
//   gp_Trsf.cxx:172-194); the two agree only when A is the identity.
// - WR2: transform_item_1 belongs to rep_1, transform_item_2 to rep_2; a pair
//   in the swapped order is swapped (as OCCT's ComputeTransformation does); a
//   pair in neither order refuses (OCCT uses it as given).
// - Orientation: the transformation maps rep_1 into rep_2 and the NAUO names
//   parent and child; the representations' own products decide which way the
//   usage reads (OCCT's CheckSRRReversesNAUO uses direct SDRs only). A
//   representation's products are those of its shape definitions, extended
//   over plain shape_representation_relationships that place nothing. Exactly
//   one reading must hold; a reversed relation reads through its exact inverse.
// - Refused by name: a usage with no placing relation or several, a placing
//   relation without a transformation (OCCT reads the identity, which no source
//   states), any other operator, and the operator's own invalid cases.
// Translations are read without the context length factor, which admission
// makes 1 (mm only; M8 bead brepgpu-12s.25.6). Admission of the result stays
// the one rigid predicate in the readers (native/rigid_placement.hpp), so a
// raw mirror or scale is written as read and refused there by name.
//
// Mapped items. OCCT 8.0.1 transfers a MAPPED_ITEM by transferring its mapped
// representation and then every plain shape_representation_relationship and
// mechanical_design_and_draughting_relationship sharing it
// (STEPControl_ActorRead.cxx:2070 TransferRelatedSRR, :2679-2720), whose other
// representations it transfers in turn (:1337-1430, :1507-1563). Nothing is
// bound until each transfer returns, so a cycle through those links recurses
// without bound (SIGSEGV). refuse_unbounded_transfer finds every such cycle on
// the raw graph before the transfer. refuse_unpublished_mapped_items refuses a
// mapped item that a transferred shape representation holds and whose mapped
// representation reaches a solid: OCCT translates it as geometry of the
// holder's product, and the source tables publish occurrences only as usages,
// so the occurrence would otherwise be lost without a record.
#include "step_units.hpp"
#include <Interface_EntityIterator.hxx>
#include <Interface_Graph.hxx>
#include <StepBasic_ProductDefinition.hxx>
#include <StepData_StepModel.hxx>
#include <StepGeom_Axis2Placement3d.hxx>
#include <StepGeom_CartesianPoint.hxx>
#include <StepGeom_CartesianTransformationOperator3d.hxx>
#include <StepGeom_Direction.hxx>
#include <StepRepr_ItemDefinedTransformation.hxx>
#include <StepRepr_MappedItem.hxx>
#include <StepRepr_MechanicalDesignAndDraughtingRelationship.hxx>
#include <StepRepr_NextAssemblyUsageOccurrence.hxx>
#include <StepRepr_ProductDefinitionShape.hxx>
#include <StepRepr_Representation.hxx>
#include <StepRepr_RepresentationItem.hxx>
#include <StepRepr_RepresentationMap.hxx>
#include <StepRepr_RepresentationRelationshipWithTransformation.hxx>
#include <StepRepr_ShapeRepresentationRelationship.hxx>
#include <StepRepr_Transformation.hxx>
#include <StepShape_ContextDependentShapeRepresentation.hxx>
#include <StepShape_ManifoldSolidBrep.hxx>
#include <StepShape_ShapeDefinitionRepresentation.hxx>
#include <StepShape_ShapeRepresentation.hxx>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace brepgpu::step_placements {
using ID = std::uint64_t;
using Vec = std::array<double, 3>;
// Row-major 3x4 child-to-parent map (r00 r01 r02 tx r10 ... tz).
using Matrix = std::array<double, 12>;

inline double dot(const Vec &a, const Vec &b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
inline Vec cross(const Vec &a, const Vec &b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
inline Vec unit(const Vec &v, const std::string &where) {
  const double length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if (!(std::isfinite(length) && length > 0))
    throw std::runtime_error(where + ": original source direction degenerates");
  return {v[0] / length, v[1] / length, v[2] / length};
}
// DIRECTION ratios (WR1: not all zero), finite, three of them.
inline Vec ratios(const Handle(StepGeom_Direction) &d, const std::string &where) {
  if (d.IsNull() || d->NbDirectionRatios() != 3)
    throw std::runtime_error(where + ": direction is missing or not 3D");
  const Vec v{d->DirectionRatiosValue(1), d->DirectionRatiosValue(2), d->DirectionRatiosValue(3)};
  if (!(std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2])) ||
      (v[0] == 0 && v[1] == 0 && v[2] == 0))
    throw std::runtime_error(where + ": original source direction degenerates");
  return v;
}
inline Vec point(const Handle(StepGeom_CartesianPoint) &p, const std::string &where) {
  if (p.IsNull() || p->NbCoordinates() != 3)
    throw std::runtime_error(where + ": point is missing or not 3D");
  const Vec v{p->CoordinatesValue(1), p->CoordinatesValue(2), p->CoordinatesValue(3)};
  if (!(std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2])))
    throw std::runtime_error(where + ": point is not finite");
  return v;
}

struct Frame {
  Vec origin, x, y, z;
};
// axis2_placement_3d as the oracle reads it: z = NVL(axis, (0,0,1)), x the
// ref_direction (default (1,0,0)) projected off z, y = z x x.
inline Frame frame(const Handle(StepGeom_Axis2Placement3d) &a, const std::string &where) {
  if (a.IsNull())
    throw std::runtime_error(where + ": frame is not an axis2_placement_3d");
  Frame f;
  f.origin = point(a->Location(), where);
  f.z = a->HasAxis() ? unit(ratios(a->Axis(), where), where) : Vec{0., 0., 1.};
  // first_proj_axis(z_axis, arg): without arg, (1,0,0) unless z_axis is
  // +-(1,0,0), then (0,1,0) (ISO 10303-42 via the AP242 MIM long form).
  const Vec x = a->HasRefDirection() ? unit(ratios(a->RefDirection(), where), where)
                : (f.z != Vec{1., 0., 0.} && f.z != Vec{-1., 0., 0.}) ? Vec{1., 0., 0.}
                                                                     : Vec{0., 1., 0.};
  const double along = dot(x, f.z);
  f.x = unit({x[0] - along * f.z[0], x[1] - along * f.z[1], x[2] - along * f.z[2]}, where);
  f.y = cross(f.z, f.x);
  return f;
}

// cartesian_transformation_operator_3d, rep_1 to rep_2 (or its exact inverse).
inline Matrix cartesian_operator(const Handle(StepGeom_CartesianTransformationOperator3d) &op,
                                 bool inverse, const std::string &where) {
  const std::optional<Vec> axis1 =
      op->HasAxis1() ? std::optional<Vec>(ratios(op->Axis1(), where)) : std::nullopt;
  const std::optional<Vec> axis2 =
      op->HasAxis2() ? std::optional<Vec>(ratios(op->Axis2(), where)) : std::nullopt;
  const std::optional<Vec> axis3 =
      op->HasAxis3() ? std::optional<Vec>(ratios(op->Axis3(), where)) : std::nullopt;
  const double scl = op->HasScale() ? op->Scale() : 1.;
  if (!(std::isfinite(scl) && scl > 0.))
    throw std::runtime_error(where + ": transformation operator scale violates WR1");
  const Vec origin = point(op->LocalOrigin(), where);
  // base_axis(3, ...): d1 := NVL(normalise(axis3), (0,0,1)).
  const Vec z = axis3 ? unit(*axis3, where) : Vec{0., 0., 1.};
  // first_proj_axis(d1, axis1); an axis1 parallel to d1, or within rounding
  // of it, has no projection. Projected twice so x is orthogonal to rounding.
  Vec v;
  constexpr double eps = 2.220446049250313e-16;
  if (!axis1)
    v = (z != Vec{1., 0., 0.} && z != Vec{-1., 0., 0.}) ? Vec{1., 0., 0.} : Vec{0., 1., 0.};
  else {
    v = unit(*axis1, where);
    const Vec sine = cross(v, z);
    if (!(std::sqrt(dot(sine, sine)) > 8 * eps))
      throw std::runtime_error(where + ": transformation operator axis1 is parallel to axis3");
  }
  for (int pass = 0; pass < 2; ++pass) {
    const double along = dot(v, z);
    v = {v[0] - along * z[0], v[1] - along * z[1], v[2] - along * z[2]};
  }
  const Vec x = unit(v, where);
  // second_proj_axis(d1, d2, axis2): sign(arg . (z x x)) (z x x), equal to the
  // standard's projection in exact arithmetic and orthonormal to rounding.
  const Vec arg = axis2 ? *axis2 : Vec{0., 1., 0.};
  const Vec w = unit(cross(z, x), where);
  const double along = dot(arg, w);
  if (!(std::abs(along) > 8 * eps * std::sqrt(dot(arg, arg))))
    throw std::runtime_error(where + ": transformation operator axis2 lies in the axis1-axis3 plane");
  const Vec y = along > 0 ? w : Vec{-w[0], -w[1], -w[2]};
  const std::array<Vec, 3> u{x, y, z};
  Matrix m;
  for (int i = 0; i < 3; ++i) {
    if (inverse) {
      m[4 * i + 0] = u[i][0] / scl;
      m[4 * i + 1] = u[i][1] / scl;
      m[4 * i + 2] = u[i][2] / scl;
      m[4 * i + 3] = -dot(u[i], origin) / scl;
    } else {
      m[4 * i + 0] = scl * u[0][i];
      m[4 * i + 1] = scl * u[1][i];
      m[4 * i + 2] = scl * u[2][i];
      m[4 * i + 3] = origin[i];
    }
  }
  return m;
}

// The child-side frame a mapped onto the parent-side frame b: B A^-1.
inline Matrix frames(const Frame &a, const Frame &b) {
  const std::array<Vec, 3> ar{a.x, a.y, a.z}, br{b.x, b.y, b.z};
  Matrix m;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j)
      m[4 * i + j] = br[0][i] * ar[0][j] + br[1][i] * ar[1][j] + br[2][i] * ar[2][j];
    m[4 * i + 3] =
        b.origin[i] - (m[4 * i] * a.origin[0] + m[4 * i + 1] * a.origin[1] + m[4 * i + 2] * a.origin[2]);
  }
  return m;
}

struct Placement {
  ID usage = 0, cdsr = 0, relation = 0, operator_id = 0;
  Handle(StepRepr_Representation) rep1, rep2;
  bool cartesian = false, reversed = false, swapped = false;
  Matrix matrix{};
};

class Reader {
public:
  explicit Reader(const Handle(StepData_StepModel) &model) : model_(model) {
    std::set<const Standard_Transient *> placing;
    for (int i = 1; i <= model_->NbEntities(); ++i)
      if (auto c = Handle(StepShape_ContextDependentShapeRepresentation)::DownCast(model_->Value(i));
          !c.IsNull() && !c->RepresentationRelation().IsNull())
        placing.insert(c->RepresentationRelation().get());
    for (int i = 1; i <= model_->NbEntities(); ++i) {
      const auto &e = model_->Value(i);
      if (auto sdr = Handle(StepShape_ShapeDefinitionRepresentation)::DownCast(e); !sdr.IsNull()) {
        auto shape = Handle(StepRepr_ProductDefinitionShape)::DownCast(
            sdr->Definition().PropertyDefinition());
        if (!shape.IsNull() && !shape->Definition().ProductDefinition().IsNull() &&
            !sdr->UsedRepresentation().IsNull())
          direct_[id(sdr->UsedRepresentation())].insert(id(shape->Definition().ProductDefinition()));
      } else if (e->DynamicType() == STANDARD_TYPE(StepRepr_ShapeRepresentationRelationship) &&
                 !placing.count(e.get())) {
        auto r = Handle(StepRepr_ShapeRepresentationRelationship)::DownCast(e);
        if (!r->Rep1().IsNull() && !r->Rep2().IsNull()) {
          links_[id(r->Rep1())].insert(id(r->Rep2()));
          links_[id(r->Rep2())].insert(id(r->Rep1()));
        }
      }
    }
    for (int i = 1; i <= model_->NbEntities(); ++i)
      if (auto c = Handle(StepShape_ContextDependentShapeRepresentation)::DownCast(model_->Value(i));
          !c.IsNull())
        read(c);
    for (int i = 1; i <= model_->NbEntities(); ++i)
      if (auto u = Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(model_->Value(i));
          !u.IsNull() && !placements_.count(id(u)))
        throw std::runtime_error("usage " + std::to_string(id(u)) +
                                 " has no context_dependent_shape_representation placing it; no "
                                 "placement is declared");
  }
  const Placement &at(ID usage) const { return placements_.at(usage); }
  const std::map<ID, Placement> &placements() const { return placements_; }

private:
  ID id(const Handle(Standard_Transient) &e) const { return step_units::label(model_, e); }
  std::set<ID> products(ID rep) const {
    std::set<ID> seen{rep}, found;
    std::vector<ID> stack{rep};
    while (!stack.empty()) {
      const ID current = stack.back();
      stack.pop_back();
      if (auto d = direct_.find(current); d != direct_.end())
        found.insert(d->second.begin(), d->second.end());
      if (auto l = links_.find(current); l != links_.end())
        for (ID next : l->second)
          if (seen.insert(next).second)
            stack.push_back(next);
    }
    return found;
  }
  static bool holds(const Handle(StepRepr_Representation) &rep,
                    const Handle(StepRepr_RepresentationItem) &item) {
    for (int i = 1; i <= rep->NbItems(); ++i)
      if (rep->ItemsValue(i) == item)
        return true;
    return false;
  }
  void read(const Handle(StepShape_ContextDependentShapeRepresentation) &cdsr) {
    const auto relation = cdsr->RepresentationRelation();
    const auto shape = cdsr->RepresentedProductRelation();
    Handle(StepRepr_NextAssemblyUsageOccurrence) usage;
    if (!shape.IsNull())
      usage = Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(
          shape->Definition().ProductDefinitionRelationship());
    // Only usages are placed; a CDSR of another relationship places nothing
    // the tables publish (a NAUO left without a placement refuses below).
    if (usage.IsNull())
      return;
    const ID ui = id(usage);
    const std::string where = "usage " + std::to_string(ui);
    if (relation.IsNull() || relation->Rep1().IsNull() || relation->Rep2().IsNull())
      throw std::runtime_error(where + ": its placing representation relationship is missing");
    auto transformed = Handle(StepRepr_RepresentationRelationshipWithTransformation)::DownCast(relation);
    if (transformed.IsNull())
      throw std::runtime_error(where + ": placing relation " + std::to_string(id(relation)) +
                               " carries no transformation; no source states its placement");
    const auto value = transformed->TransformationOperator().Value();
    auto idt = Handle(StepRepr_ItemDefinedTransformation)::DownCast(value);
    auto cto = Handle(StepGeom_CartesianTransformationOperator3d)::DownCast(value);
    if (idt.IsNull() && cto.IsNull())
      throw std::runtime_error(where + ": unsupported representation transformation operator");
    if (placements_.count(ui))
      throw std::runtime_error(where + ": more than one context_dependent_shape_representation "
                                       "places it (ambiguous placement)");
    Placement p;
    p.usage = ui;
    p.cdsr = id(cdsr);
    p.relation = id(relation);
    p.rep1 = relation->Rep1();
    p.rep2 = relation->Rep2();
    p.operator_id = id(value);
    p.cartesian = !cto.IsNull();
    const auto rep1 = relation->Rep1(), rep2 = relation->Rep2();
    const ID parent = id(usage->RelatingProductDefinition()), child = id(usage->RelatedProductDefinition());
    const auto first = products(id(rep1)), second = products(id(rep2));
    auto reads = [&](ID on_first, ID on_second) {
      return (!first.empty() || !second.empty()) && (first.empty() || first.count(on_first)) &&
             (second.empty() || second.count(on_second));
    };
    const bool forward = reads(child, parent), backward = reads(parent, child);
    if (forward == backward)
      throw std::runtime_error(where + ": representation relation " + std::to_string(p.relation) +
                               " does not orient its usage (the representations' products read "
                               "both ways or neither)");
    p.reversed = backward;
    const std::string operator_where = where + " operator " + std::to_string(p.operator_id);
    if (p.cartesian)
      p.matrix = cartesian_operator(cto, p.reversed, operator_where);
    else {
      auto a = idt->TransformItem1(), b = idt->TransformItem2();
      if (a.IsNull() || b.IsNull())
        throw std::runtime_error(operator_where + ": item-defined transformation frames missing");
      if (!(holds(rep1, a) && holds(rep2, b))) {
        if (!(holds(rep2, a) && holds(rep1, b)))
          throw std::runtime_error(operator_where + ": item-defined transformation frames do not "
                                                    "belong to its representations (WR2)");
        std::swap(a, b);
        p.swapped = true;
      }
      if (p.reversed)
        std::swap(a, b);
      p.matrix = frames(frame(Handle(StepGeom_Axis2Placement3d)::DownCast(a), operator_where),
                        frame(Handle(StepGeom_Axis2Placement3d)::DownCast(b), operator_where));
    }
    placements_.emplace(ui, p);
  }
  Handle(StepData_StepModel) model_;
  std::map<ID, std::set<ID>> direct_, links_;
  std::map<ID, Placement> placements_;
};

// Whether OCCT's located placement agrees with the raw one: every entry within
// 2^-32 of the larger magnitude (at least 1), the oracle's coordinate grade.
// The largest relative entry difference. Translations must also agree within
// the source's resolution (placement_translation_difference below).
inline double placement_difference(const Matrix &raw, const std::array<double, 12> &occt) {
  double worst = 0;
  for (int i = 0; i < 12; ++i)
    worst = std::max(worst, std::abs(raw[i] - occt[i]) /
                                std::max({1., std::abs(raw[i]), std::abs(occt[i])}));
  return worst;
}
inline constexpr double placement_agreement = 1. / 4294967296.;
// The largest absolute translation difference in mm (B2 .37.20 review 9a). The
// relative rule above alone admits up to 2^-32 |t|, which exceeds a 1e-7 mm
// uncertainty beyond about 430 mm. OCCT's value is written only when every
// translation entry also differs from the raw one by no more than the
// resolution: the smallest usable declared uncertainty of the placing
// relation's two representation contexts (each in mm, G6 UR-7), or 2^-32 mm
// when neither declares a usable one.
inline double placement_translation_difference(const Matrix &raw, const std::array<double, 12> &occt) {
  double worst = 0;
  for (int i = 3; i < 12; i += 4)
    worst = std::max(worst, std::abs(raw[i] - occt[i]));
  return worst;
}

// The resolution both translation and rotation bounds use: the smallest usable
// declared uncertainty (mm) of the placing relation's two representation
// contexts, or 2^-32 mm when neither declares a usable one.
inline double placement_resolution(const Handle(StepData_StepModel) &model, const Placement &p) {
  double resolution = INFINITY;
  for (const auto &rep : {p.rep1, p.rep2}) {
    const auto u = step_units::context_uncertainty(model, rep);
    if (u.governed && u.mm > 0)
      resolution = std::min(resolution, u.mm);
  }
  return std::isfinite(resolution) ? resolution : 1. / 4294967296.;
}

// The child's extent in its own frame (the frame the placement maps from): an
// axis-aligned box that holds every point of the child's geometry, in mm. A
// child with no geometry passes an empty extent. The caller must bound the
// points: OCCT's box of a translated shape whose edges have no pcurves spans
// the whole unbounded carrier, which here can never agree (an infinite or NaN
// corner move is a disagreement), so the raw placement is written.
struct Extent {
  bool empty = true;
  std::array<double, 3> lo{}, hi{};
};

// The one placement-agreement predicate (B2-reader-export review round 1
// decisions: both arms call it). OCCT's located placement may be written in
// place of the raw one only when all three hold:
// - every entry differs by at most 2^-32 relative (placement_difference);
// - every translation entry differs by at most the resolution (review 9a);
// - a rotation difference moves no point of the child's extent by more than
//   the resolution (review round 2, finding 3): the move (R_raw - R_occt) q +
//   (t_raw - t_occt) is affine in q, so its largest norm over the box is at a
//   corner; the translation difference is the move of the frame origin.
struct Agreement {
  double relative = 0, translation_mm = 0, moved_mm = 0, resolution_mm = 0;
  bool agrees = false;
};
inline Agreement agree(const Matrix &raw, const Matrix &occt, double resolution_mm, const Extent &extent) {
  Agreement a;
  a.resolution_mm = resolution_mm;
  a.relative = placement_difference(raw, occt);
  a.translation_mm = placement_translation_difference(raw, occt);
  a.moved_mm = a.translation_mm;
  if (!extent.empty)
    for (int corner = 0; corner < 8; ++corner) {
      const double q[3] = {(corner & 1) ? extent.hi[0] : extent.lo[0], (corner & 2) ? extent.hi[1] : extent.lo[1],
                           (corner & 4) ? extent.hi[2] : extent.lo[2]};
      double d2 = 0;
      for (int r = 0; r < 3; ++r) {
        double e = raw[4 * r + 3] - occt[4 * r + 3];
        for (int k = 0; k < 3; ++k)
          if (const double delta = raw[4 * r + k] - occt[4 * r + k]; delta != 0)
            e += delta * q[k]; // an equal entry moves nothing, however far the point
        d2 += e * e;
      }
      const double d = std::sqrt(d2);
      if (!std::isfinite(d)) {
        a.moved_mm = INFINITY; // an unbounded extent under a rotation difference never agrees
        break;
      }
      a.moved_mm = std::max(a.moved_mm, d);
    }
  a.agrees = a.relative <= placement_agreement && a.translation_mm <= resolution_mm && a.moved_mm <= resolution_mm;
  return a;
}

// Representations OCCT's transfer of one shape representation enters from its
// mapped items: the mapped representation, the far side of every plain
// shape_representation_relationship that shares it (TransferRelatedSRR reads
// the exact type only, and transfers rep_2 when the mapped one is rep_1), and
// both sides of every mechanical_design_and_draughting_relationship sharing it
// (TransferRelatedSRR :2708, TransferEntity(MDADR) :1507-1563 transfers both
// representations). The constructive-geometry relationship is followed only
// under read.step.constructive.geometry, off by default
// (DESTEP_Parameters.hxx:172), so it is not modelled.
struct MappedLink {
  Handle(StepRepr_MappedItem) item;
  Handle(StepRepr_Representation) mapped, next;
  Handle(StepRepr_RepresentationRelationship) relation; // null: the mapped one itself
};
inline std::vector<MappedLink> mapped_links(const Interface_Graph &graph,
                                            const Handle(StepRepr_Representation) &rep) {
  std::vector<MappedLink> out;
  for (int i = 1; i <= rep->NbItems(); ++i) {
    auto item = Handle(StepRepr_MappedItem)::DownCast(rep->ItemsValue(i));
    if (item.IsNull() || item->MappingSource().IsNull() ||
        item->MappingSource()->MappedRepresentation().IsNull())
      continue;
    const auto mapped = item->MappingSource()->MappedRepresentation();
    out.push_back({item, mapped, mapped, {}});
    auto users = graph.Sharings(mapped);
    for (users.Start(); users.More(); users.Next())
      if (users.Value()->DynamicType() == STANDARD_TYPE(StepRepr_ShapeRepresentationRelationship)) {
        auto relation = Handle(StepRepr_ShapeRepresentationRelationship)::DownCast(users.Value());
        auto next = relation->Rep1() == mapped ? relation->Rep2() : relation->Rep1();
        if (!next.IsNull())
          out.push_back({item, mapped, next, relation});
      } else if (users.Value()->DynamicType() ==
                 STANDARD_TYPE(StepRepr_MechanicalDesignAndDraughtingRelationship)) {
        auto relation = Handle(StepRepr_RepresentationRelationship)::DownCast(users.Value());
        for (const auto &next : {relation->Rep1(), relation->Rep2()})
          if (!next.IsNull() && next != mapped)
            out.push_back({item, mapped, next, relation});
      }
  }
  return out;
}

// .37.27: a cycle of mapped-item links recurses without bound in OCCT 8.0.1;
// refuse it by name before the transfer, naming the representations around it.
inline void refuse_unbounded_transfer(const Handle(StepData_StepModel) &model,
                                      const Interface_Graph &graph) {
  auto id = [&](const Handle(Standard_Transient) &e) { return step_units::label(model, e); };
  std::map<const Standard_Transient *, int> state; // 1 on the stack, 2 done
  struct Step {
    Handle(StepRepr_Representation) rep;
    std::vector<MappedLink> links;
    std::size_t next = 0;
  };
  for (int i = 1; i <= model->NbEntities(); ++i) {
    auto start = Handle(StepRepr_Representation)::DownCast(model->Value(i));
    if (start.IsNull() || state.count(start.get()))
      continue;
    std::vector<Step> stack{{start, mapped_links(graph, start)}};
    state[start.get()] = 1;
    while (!stack.empty()) {
      auto &top = stack.back();
      if (top.next == top.links.size()) {
        state[top.rep.get()] = 2;
        stack.pop_back();
        continue;
      }
      const auto link = top.links[top.next++];
      const auto found = state.find(link.next.get());
      if (found == state.end()) {
        state[link.next.get()] = 1;
        stack.push_back({link.next, mapped_links(graph, link.next)});
        continue;
      }
      if (found->second != 1)
        continue;
      std::string cycle;
      bool on = false;
      for (const auto &s : stack) {
        on = on || s.rep == link.next;
        if (!on)
          continue;
        const auto &l = s.links[s.next - 1];
        cycle += " representation " + std::to_string(id(s.rep)) + " (mapped item " +
                 std::to_string(id(l.item)) + " of representation " + std::to_string(id(l.mapped)) +
                 (l.relation.IsNull()
                      ? std::string()
                      : (l.relation->DynamicType() ==
                                 STANDARD_TYPE(StepRepr_MechanicalDesignAndDraughtingRelationship)
                             ? ", mechanical_design_and_draughting_relationship "
                             : ", plain shape_representation_relationship ") +
                            std::to_string(id(l.relation))) +
                 ") ->";
      }
      throw std::runtime_error(
          "OCCT 8.0.1 would recurse without bound transferring this file: mapped-item links "
          "form a cycle:" +
          cycle + " representation " + std::to_string(id(link.next)) +
          " (STEPControl_ActorRead.cxx:2070 TransferRelatedSRR, :2679-2710); refused before "
          "transfer");
    }
  }
}

// Whether one mapped item brings a solid with it in OCCT's transfer: its own
// links, then the items and mapped-item links of every representation they
// reach (TransferEntity(ShapeRepresentation) does not follow relationships).
inline bool reaches_solid(const Interface_Graph &graph, const std::vector<MappedLink> &links) {
  std::set<const Standard_Transient *> seen;
  std::vector<Handle(StepRepr_Representation)> stack;
  for (const auto &link : links)
    if (seen.insert(link.next.get()).second)
      stack.push_back(link.next);
  while (!stack.empty()) {
    const auto rep = stack.back();
    stack.pop_back();
    for (int i = 1; i <= rep->NbItems(); ++i)
      if (!Handle(StepShape_ManifoldSolidBrep)::DownCast(rep->ItemsValue(i)).IsNull())
        return true;
    for (const auto &link : mapped_links(graph, rep))
      if (seen.insert(link.next.get()).second)
        stack.push_back(link.next);
  }
  return false;
}

// .37.26: a solid-bearing mapped item held by a shape representation OCCT
// transfers places a solid the tables cannot publish (another product's
// occurrence outside every usage, or a solid moved within its own
// definition); refuse it by name.
inline void refuse_unpublished_mapped_items(const Handle(StepData_StepModel) &model,
                                            const Interface_Graph &graph,
                                            const step_units::RepresentationGraph &representations) {
  auto id = [&](const Handle(Standard_Transient) &e) { return step_units::label(model, e); };
  for (int i = 1; i <= model->NbEntities(); ++i) {
    auto item = Handle(StepRepr_MappedItem)::DownCast(model->Value(i));
    if (item.IsNull())
      continue;
    auto users = graph.Sharings(item);
    for (users.Start(); users.More(); users.Next()) {
      auto holder = Handle(StepShape_ShapeRepresentation)::DownCast(users.Value());
      if (holder.IsNull() || !representations.transferred(holder))
        continue;
      bool held = false;
      for (int k = 1; k <= holder->NbItems() && !held; ++k)
        held = holder->ItemsValue(k) == item;
      if (!held)
        continue;
      if (item->MappingSource().IsNull() || item->MappingSource()->MappedRepresentation().IsNull())
        throw std::runtime_error("mapped item " + std::to_string(id(item)) + " in representation " +
                                 std::to_string(id(holder)) + " maps no representation");
      const auto mapped = item->MappingSource()->MappedRepresentation();
      std::vector<MappedLink> own;
      for (const auto &link : mapped_links(graph, holder))
        if (link.item == item)
          own.push_back(link);
      if (reaches_solid(graph, own))
        throw std::runtime_error(
            "mapped item " + std::to_string(id(item)) + " in representation " +
            std::to_string(id(holder)) + " places representation " + std::to_string(id(mapped)) +
            ", which reaches a solid: OCCT translates it as geometry of the holder's product, "
            "and the source tables carry a solid only in its own representation's frame and a "
            "placed occurrence only as a usage; refused rather than dropped");
    }
  }
}
} // namespace brepgpu::step_placements
#endif
