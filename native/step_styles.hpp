#ifndef BREPGPU_STEP_STYLES_HPP
#define BREPGPU_STEP_STYLES_HPP
// STEP surface colours read for the source tables, shared by the STEP source
// adapter and the reference source exporter so both write the same styles
// (CAx-IF Recommended Practices for Model Styling and Organization v1.10,
// sections 4.2.1, 4.3 and 5; papers/2026-10-17-instancing-semantics.md 5.2).
//
// - Every styled item of the model is read, including the root-list ones whose
//   item is a shape representation, which STEPConstruct_Styles files apart from
//   Style(i) (STEPConstruct_Styles.cxx:529-547). A styled item without a colour
//   this reader decodes (a null style, an invisibility override, a curve-only
//   style) is recorded as unsupported.
// - A styled item's item is a face, a solid, or a product's shape
//   representation, directly or through a mapped item. A mapped item names no
//   usage: a plain style on one styles a single occurrence only when exactly
//   one usage is placed by a plain shape_representation_relationship (no
//   transformation of its own) between the mapped item's holder and the mapped
//   representation, and that usage's parent has one occurrence. Otherwise it
//   styles the mapped product, recorded as a collapse. Anything else is
//   recorded as unplaced.
// - Usage paths are counted once, over the whole usage graph in topological
//   order, saturating at two (one unit of work per product and per usage).
// - Over-riding styles beat every style they override, directly or through a
//   chain of over_ridden_style links (the chain may pass through styles of
//   other items or colourless ones). Two styles left on one item are a
//   recorded conflict; the lower entity is kept.
// - A product's style is its shape representation's (assembly node or
//   component part), else for a part its solid's.
// - A CONTEXT_DEPENDENT_OVER_RIDING_STYLED_ITEM is a style for the occurrence
//   its style_context names (Model Styling 5.1). Of several on one occurrence
//   and item, those overridden by another drop out; two left are a conflict.
//   Between nested occurrences the deeper (more specific) one wins in the
//   consumers (our rule); a shallower one that overrides a deeper one
//   supersedes it, and unrelated nested ones that disagree are a conflict.
#include <Interface_EntityIterator.hxx>
#include <Interface_Graph.hxx>
#include <Quantity_Color.hxx>
#include <STEPConstruct_RenderingProperties.hxx>
#include <STEPConstruct_Styles.hxx>
#include <StepBasic_ProductDefinition.hxx>
#include <StepData_StepModel.hxx>
#include <StepRepr_MappedItem.hxx>
#include <StepRepr_NextAssemblyUsageOccurrence.hxx>
#include <StepRepr_ProductDefinitionShape.hxx>
#include <StepRepr_RepresentationMap.hxx>
#include <StepRepr_RepresentationRelationship.hxx>
#include <StepRepr_RepresentationRelationshipWithTransformation.hxx>
#include <StepRepr_ShapeRepresentationRelationship.hxx>
#include <StepShape_AdvancedFace.hxx>
#include <StepShape_ContextDependentShapeRepresentation.hxx>
#include <StepShape_ShapeDefinitionRepresentation.hxx>
#include <StepShape_ShapeRepresentation.hxx>
#include <StepVisual_Colour.hxx>
#include <StepVisual_ContextDependentOverRidingStyledItem.hxx>
#include <StepVisual_OverRidingStyledItem.hxx>
#include <StepVisual_StyledItem.hxx>
#include <XSControl_WorkSession.hxx>
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace brepgpu::step_styles {
using ID = std::uint64_t;
struct Colour {
  ID id = 0;
  std::vector<ID> sources;
  std::array<double, 4> rgba{0, 0, 0, 1};
  bool color = false;
};
struct Note {
  std::uint32_t severity = 1;
  std::string stage;
  std::vector<ID> ids;
  std::string message;
};
struct OccurrenceRow {
  ID style = 0, face = 0;
  std::vector<ID> path;
};

class Resolver {
public:
  Resolver(const Handle(StepData_StepModel) &model, const Handle(XSControl_WorkSession) &session)
      : model_(model), graph_(session->Graph()), reader_(session) {
    reader_.LoadStyles();
    std::set<ID> children;
    for (int i = 1; i <= model_->NbEntities(); ++i) {
      const auto &e = model_->Value(i);
      if (auto u = Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(e); !u.IsNull()) {
        link_[id(u)] = {id(u->RelatingProductDefinition()), id(u->RelatedProductDefinition())};
        children.insert(id(u->RelatedProductDefinition()));
      }
      if (auto p = Handle(StepBasic_ProductDefinition)::DownCast(e); !p.IsNull())
        products_.push_back(id(p));
      if (auto sdr = Handle(StepShape_ShapeDefinitionRepresentation)::DownCast(e); !sdr.IsNull()) {
        auto shape = Handle(StepRepr_ProductDefinitionShape)::DownCast(
            sdr->Definition().PropertyDefinition());
        if (!shape.IsNull() && !shape->Definition().ProductDefinition().IsNull() &&
            !sdr->UsedRepresentation().IsNull()) {
          const ID product = id(shape->Definition().ProductDefinition());
          product_reps_[product].insert(id(sdr->UsedRepresentation()));
          rep_products_[id(sdr->UsedRepresentation())].insert(product);
        }
      }
    }
    for (ID p : products_)
      if (!children.count(p))
        roots_.push_back(p);
    // Usages placed by a plain relationship (no transformation of its own),
    // keyed by the two representations it relates.
    for (int i = 1; i <= model_->NbEntities(); ++i) {
      auto cdsr = Handle(StepShape_ContextDependentShapeRepresentation)::DownCast(model_->Value(i));
      if (cdsr.IsNull() || cdsr->RepresentationRelation().IsNull() ||
          cdsr->RepresentedProductRelation().IsNull())
        continue;
      const auto relation = cdsr->RepresentationRelation();
      auto u = Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(
          cdsr->RepresentedProductRelation()->Definition().ProductDefinitionRelationship());
      if (u.IsNull() || relation->IsKind(STANDARD_TYPE(StepRepr_RepresentationRelationshipWithTransformation)))
        continue;
      const ID a = id(relation->Rep1()), b = id(relation->Rep2());
      plain_usages_[{std::min(a, b), std::max(a, b)}].push_back(id(u));
    }
    count_paths();
    for (int i = 1; i <= model_->NbEntities(); ++i) {
      auto s = Handle(StepVisual_StyledItem)::DownCast(model_->Value(i));
      if (s.IsNull())
        continue;
      Handle(StepVisual_Colour) sc, bc, cc;
      STEPConstruct_RenderingProperties rendering;
      bool component = false;
      Colour c;
      c.id = id(s);
      c.sources = {c.id};
      if (reader_.GetColors(s, sc, bc, cc, rendering, component)) {
        Quantity_Color color;
        auto chosen = !sc.IsNull() ? sc : (!bc.IsNull() ? bc : cc);
        if (!chosen.IsNull() && STEPConstruct_Styles::DecodeColor(chosen, color)) {
          c.rgba = {color.Red(), color.Green(), color.Blue(), 1 - rendering.Transparency()};
          c.color = true;
          c.sources.push_back(id(chosen));
        }
      }
      colours_.push_back(c);
      colour_of_[c.id] = c;
      Handle(Standard_Transient) item = s->ItemAP242().Value();
      if (!c.color) {
        notes.push_back({0, "source_style_unsupported", {c.id, id(item)},
                         "styled item carries no surface colour this reader decodes (a null "
                         "style, an invisibility or a curve-only style); not shown"});
        continue;
      }
      if (auto o = Handle(StepVisual_ContextDependentOverRidingStyledItem)::DownCast(s);
          !o.IsNull()) {
        occurrence_items_.push_back(o);
        continue;
      }
      if (item.IsNull()) {
        unplaced(c.id, 0, "styled item has no item");
        continue;
      }
      if (auto mapped = Handle(StepRepr_MappedItem)::DownCast(item); !mapped.IsNull()) {
        place_mapped_style(s, mapped);
        continue;
      }
      item_styles_[id(item)].push_back(s);
    }
  }
  // Every styled item, in file order.
  const std::vector<Colour> &colours() const { return colours_; }
  // The one style an item keeps: not overridden by another of its styles.
  ID item_style(ID item) {
    auto found = item_styles_.find(item);
    if (found == item_styles_.end())
      return 0;
    placed_.insert(item);
    const auto kept = surviving(found->second);
    if (kept.size() > 1 && noted_.insert(item).second) {
      std::vector<ID> ids{item};
      ids.insert(ids.end(), kept.begin(), kept.end());
      notes.push_back({1, "source_style_conflict", ids,
                       "styled items on one item, none overriding the other; the lowest entity "
                       "is kept"});
    }
    return kept.empty() ? 0 : kept[0];
  }
  // A product's style: its shape representations' (assembly node or component
  // part; extra holds a part's representations from its solid's holder up),
  // else its solid's (0 for an assembly).
  ID product_style(ID product, const std::set<ID> &extra, ID solid) {
    auto &reps = product_reps_[product];
    reps.insert(extra.begin(), extra.end());
    std::vector<ID> styles;
    for (ID rep : reps)
      if (ID s = item_style(rep))
        styles.push_back(s);
    std::sort(styles.begin(), styles.end());
    styles.erase(std::unique(styles.begin(), styles.end()), styles.end());
    if (styles.size() > 1 && noted_.insert(product).second) {
      std::vector<ID> ids{product};
      ids.insert(ids.end(), styles.begin(), styles.end());
      notes.push_back({1, "source_style_conflict", ids,
                       "shape representations of one product carry different styles; the lowest "
                       "entity is kept"});
    }
    const ID solid_style = solid ? item_style(solid) : 0;
    return !styles.empty() ? styles[0] : solid_style;
  }
  // Per-occurrence styles. A CDORSI's style_context lists the representation
  // relationships from the root to the styled instance. OCCT does not keep
  // them in the typed field (StepVisual_StyleContextSelect has no relationship
  // case, so the read fails with "Entity has illegal type") but the entity
  // graph still shares them. Each relationship is one usage, through the
  // CONTEXT_DEPENDENT_SHAPE_REPRESENTATION that places it; the usages are
  // ordered by chaining from the root, so their written order is not needed.
  // face_on(product, face) says whether a face belongs to that leaf product.
  std::vector<OccurrenceRow> occurrences(ID root, const std::function<bool(ID, ID)> &face_on) {
    std::map<ID, ID> relation_usage;
    for (int i = 1; i <= model_->NbEntities(); ++i) {
      auto cdsr = Handle(StepShape_ContextDependentShapeRepresentation)::DownCast(model_->Value(i));
      if (cdsr.IsNull() || cdsr->RepresentationRelation().IsNull() ||
          cdsr->RepresentedProductRelation().IsNull())
        continue;
      if (auto u = Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(
              cdsr->RepresentedProductRelation()->Definition().ProductDefinitionRelationship());
          !u.IsNull())
        relation_usage[id(cdsr->RepresentationRelation())] = id(u);
    }
    // Candidates per (path, face): CDORSIs and per-instance mapped-item styles.
    std::map<std::pair<std::vector<ID>, ID>, std::vector<Handle(StepVisual_StyledItem)>> keys;
    for (const auto &[style, path] : mapped_rows_)
      keys[{path, 0}].push_back(style);
    for (const auto &o : occurrence_items_) {
      const ID oi = id(o);
      std::set<ID> path_usages;
      bool foreign = false;
      auto shared = graph_.Shareds(o);
      for (shared.Start(); shared.More(); shared.Next())
        if (auto relation = Handle(StepRepr_RepresentationRelationship)::DownCast(shared.Value());
            !relation.IsNull()) {
          auto found = relation_usage.find(id(relation));
          if (found == relation_usage.end())
            foreign = true;
          else
            path_usages.insert(found->second);
        }
      Handle(Standard_Transient) item = o->ItemAP242().Value();
      if (auto mapped = Handle(StepRepr_MappedItem)::DownCast(item);
          !mapped.IsNull() && !mapped->MappingSource().IsNull())
        item = mapped->MappingSource()->MappedRepresentation();
      const ID item_id = id(item);
      if (foreign || path_usages.empty()) {
        unplaced(oi, item_id, "style_context is not a list of assembly usage relationships");
        continue;
      }
      std::vector<ID> path;
      ID parent = root;
      while (!path_usages.empty()) {
        auto next = std::find_if(path_usages.begin(), path_usages.end(), [&](ID u) {
          return link_.count(u) && link_.at(u).first == parent;
        });
        if (next == path_usages.end())
          break;
        path.push_back(*next);
        parent = link_.at(*next).second;
        path_usages.erase(next);
      }
      if (!path_usages.empty()) {
        unplaced(oi, item_id, "style_context is not a path from the root");
        continue;
      }
      ID face = 0;
      if (!Handle(StepShape_AdvancedFace)::DownCast(item).IsNull()) {
        if (!face_on(parent, item_id)) {
          unplaced(oi, item_id, "styled face is not on the occurrence its style_context reaches");
          continue;
        }
        face = item_id;
      } else if (!product_reps_[parent].count(item_id)) {
        unplaced(oi, item_id,
                 "styled item is not the shape representation of the occurrence its "
                 "style_context reaches");
        continue;
      }
      keys[{path, face}].push_back(o);
    }
    // One row per (path, face): drop the overridden; two left are a conflict.
    std::map<std::pair<std::vector<ID>, ID>, Handle(StepVisual_StyledItem)> rows;
    for (const auto &[key, styles] : keys) {
      const auto kept = surviving(styles);
      if (kept.size() > 1) {
        std::vector<ID> ids{key.second};
        ids.insert(ids.end(), kept.begin(), kept.end());
        notes.push_back({1, "source_style_conflict", ids,
                         "occurrence styles on one occurrence path and item, none overriding "
                         "another; the lowest entity is kept"});
      }
      for (const auto &s : styles)
        if (id(s) == kept[0])
          rows[key] = s;
    }
    // Nested whole-occurrence rows: the deeper one wins in the consumers.
    std::set<std::pair<std::vector<ID>, ID>> superseded;
    for (const auto &[outer_key, outer] : rows)
      for (const auto &[inner_key, inner] : rows) {
        const auto &a = outer_key.first, &b = inner_key.first;
        if (outer_key.second || inner_key.second || a.size() >= b.size() ||
            !std::equal(a.begin(), a.end(), b.begin()))
          continue;
        if (overrides(inner, outer))
          continue;
        if (overrides(outer, inner)) {
          superseded.insert(inner_key);
          notes.push_back({0, "source_style_precedence", {id(outer), id(inner)},
                           "an outer occurrence style overrides the nested one; the nested one "
                           "is dropped"});
        } else if (colour_of_.at(id(outer)).rgba != colour_of_.at(id(inner)).rgba)
          notes.push_back({1, "source_style_conflict", {id(outer), id(inner)},
                           "nested occurrence styles disagree and neither overrides the other; "
                           "consumers keep the deeper one (our rule, not the practice's)"});
      }
    std::vector<OccurrenceRow> out;
    for (const auto &[key, style] : rows)
      if (!superseded.count(key))
        out.push_back({id(style), key.second, key.first});
    return out;
  }
  // A coloured style on anything else (edges, curves, points, shells, sets,
  // presentation items) is not shown by the source tables: say so per style.
  void finish() {
    for (const auto &entry : item_styles_)
      if (!placed_.count(entry.first))
        for (const auto &s : entry.second)
          unplaced(id(s), entry.first,
                   "styled item is not a face, solid or product shape representation of the "
                   "source");
  }
  // Units of work the usage-path count has charged (products taken, usages
  // relaxed, path steps rebuilt).
  std::uint64_t path_work() const { return path_work_; }
  std::vector<Note> notes;

private:
  ID id(const Handle(Standard_Transient) &e) const {
    if (e.IsNull())
      return 0;
    const int n = model_->IdentLabel(e);
    if (n <= 0)
      throw std::runtime_error("missing original STEP entity label");
    return ID(n);
  }
  void unplaced(ID style, ID item, const std::string &why) {
    notes.push_back({1, "source_style_unplaced", {style, item}, why});
  }
  // a overrides b directly or through a chain of over_ridden_style links,
  // whatever item or colour the styles between them have; a cycle ends it.
  static bool overrides(const Handle(StepVisual_StyledItem) &a,
                        const Handle(StepVisual_StyledItem) &b) {
    std::set<const Standard_Transient *> seen{a.get()};
    for (auto o = Handle(StepVisual_OverRidingStyledItem)::DownCast(a); !o.IsNull();) {
      const auto next = o->OverRiddenStyle();
      if (next.IsNull())
        return false;
      if (next == b)
        return true;
      if (!seen.insert(next.get()).second)
        return false;
      o = Handle(StepVisual_OverRidingStyledItem)::DownCast(next);
    }
    return false;
  }
  // The styles of one group that no other style of the group overrides,
  // lowest entity first; independent of the order the file lists them in.
  std::vector<ID> surviving(const std::vector<Handle(StepVisual_StyledItem)> &group) const {
    std::vector<ID> kept;
    for (const auto &s : group)
      if (std::none_of(group.begin(), group.end(),
                       [&](const Handle(StepVisual_StyledItem) &t) { return overrides(t, s); }))
        kept.push_back(id(s));
    std::sort(kept.begin(), kept.end());
    kept.erase(std::unique(kept.begin(), kept.end()), kept.end());
    if (kept.empty() && !group.empty()) // an override cycle: no style beats all
      for (const auto &s : group)
        kept.push_back(id(s));
    std::sort(kept.begin(), kept.end());
    return kept;
  }
  // Kahn's order over the usage graph: a root has one path, a product the
  // sum of its parents' counts, saturating at two; one unit of work per
  // product taken and per usage relaxed. A product on or below a cycle is never
  // taken and has unboundedly many paths (two). via_ keeps the one usage into a
  // product whose count is one.
  void count_paths() {
    std::map<ID, std::vector<ID>> out;
    std::map<ID, int> pending;
    for (ID p : products_)
      pending[p];
    for (const auto &[usage, ends] : link_) {
      out[ends.first].push_back(usage);
      ++pending[ends.second];
    }
    std::vector<ID> ready;
    for (ID p : products_)
      if (!pending[p]) {
        ready.push_back(p);
        paths_[p] = 1;
      }
    while (!ready.empty()) {
      const ID p = ready.back();
      ready.pop_back();
      ++path_work_;
      for (ID usage : out[p]) {
        ++path_work_;
        const ID child = link_.at(usage).second;
        auto &count = paths_[child];
        count = std::min(2, count + paths_[p]);
        if (count == 1)
          via_[child] = usage;
        if (--pending[child] == 0)
          ready.push_back(child);
      }
    }
    for (const auto &[p, left] : pending)
      if (left)
        paths_[p] = 2;
  }
  // The one usage path from the root to a product whose count is one.
  std::vector<ID> path_to(ID product) {
    std::vector<ID> path;
    for (auto at = via_.find(product); at != via_.end(); at = via_.find(link_.at(path.back()).first)) {
      ++path_work_;
      path.push_back(at->second);
    }
    std::reverse(path.begin(), path.end());
    return path;
  }
  // A plain style on a mapped item: per instance when one usage is placed by
  // a plain relationship between the item's holder and the mapped
  // representation (the mapped item is then that usage's placement) and the
  // usage's parent has one occurrence; else on the mapped product, recorded.
  void place_mapped_style(const Handle(StepVisual_StyledItem) &s,
                          const Handle(StepRepr_MappedItem) &mapped) {
    if (mapped->MappingSource().IsNull() || mapped->MappingSource()->MappedRepresentation().IsNull()) {
      unplaced(id(s), id(mapped), "styled mapped item maps no representation");
      return;
    }
    const auto mapped_rep = mapped->MappingSource()->MappedRepresentation();
    const ID rep = id(mapped_rep);
    std::vector<Handle(StepShape_ShapeRepresentation)> holders;
    auto sharing = graph_.Sharings(mapped);
    for (sharing.Start(); sharing.More(); sharing.Next())
      if (auto holder = Handle(StepShape_ShapeRepresentation)::DownCast(sharing.Value());
          !holder.IsNull() && rep_products_.count(id(holder)))
        holders.push_back(holder);
    if (holders.empty() || !rep_products_.count(rep)) {
      // Not held by a product's shape: a presentation of the mapped product.
      item_styles_[rep].push_back(s);
      return;
    }
    std::vector<ID> placed;
    bool twin = false;
    for (const auto &holder : holders) {
      const ID h = id(holder);
      int maps = 0; // mapped items of this holder that map the same representation
      for (int i = 1; i <= holder->NbItems(); ++i)
        if (auto other = Handle(StepRepr_MappedItem)::DownCast(holder->ItemsValue(i));
            !other.IsNull() && !other->MappingSource().IsNull() &&
            other->MappingSource()->MappedRepresentation() == mapped_rep)
          ++maps;
      twin = twin || maps > 1;
      auto found = plain_usages_.find({std::min(h, rep), std::max(h, rep)});
      if (found != plain_usages_.end())
        for (ID usage : found->second)
          if (rep_products_.at(h).count(link_.at(usage).first) &&
              rep_products_.at(rep).count(link_.at(usage).second))
            placed.push_back(usage);
    }
    std::string why;
    if (placed.empty())
      why = "no usage is placed by a plain relationship between its holder and the mapped "
            "representation (a usage with its own transformation is not the mapped item's)";
    else if (placed.size() > 1 || twin)
      why = "several usages or mapped items match the one relationship";
    else if (paths_[link_.at(placed[0]).first] != 1)
      why = "the usage's parent has several occurrences";
    if (why.empty()) {
      auto path = path_to(link_.at(placed[0]).first);
      path.push_back(placed[0]);
      mapped_rows_.push_back({s, path});
      return;
    }
    notes.push_back({1, "source_style_collapsed", {id(s), id(mapped), rep},
                     "styled mapped item does not place one occurrence: " + why +
                         "; its style is applied to the mapped product"});
    item_styles_[rep].push_back(s);
  }
  Handle(StepData_StepModel) model_;
  const Interface_Graph &graph_;
  STEPConstruct_Styles reader_;
  std::vector<Colour> colours_;
  std::map<ID, Colour> colour_of_;
  std::map<ID, std::vector<Handle(StepVisual_StyledItem)>> item_styles_;
  std::vector<Handle(StepVisual_ContextDependentOverRidingStyledItem)> occurrence_items_;
  std::vector<std::pair<Handle(StepVisual_StyledItem), std::vector<ID>>> mapped_rows_;
  std::map<ID, std::set<ID>> product_reps_, rep_products_;
  std::map<ID, std::pair<ID, ID>> link_;
  std::map<std::pair<ID, ID>, std::vector<ID>> plain_usages_;
  std::map<ID, int> paths_;
  std::map<ID, ID> via_;
  std::uint64_t path_work_ = 0;
  std::vector<ID> products_, roots_;
  std::set<ID> placed_, noted_;
};
} // namespace brepgpu::step_styles
#endif
