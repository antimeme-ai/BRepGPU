// The shared style resolver (native/step_styles.hpp) on a STEP model without
// a transfer. OCCT 8.0.1 recurses without bound when a plain
// shape_representation_relationship joins a mapped item's holder and its
// mapped representation (STEPControl_ActorRead::TransferRelatedSRR follows the
// relationship back into the holder), so the resolver's handling of that
// structure is tested on the model alone. Prints one line per product's style
// (as the adapter asks for it: the product's representations from each solid's
// holder up to the transferred one, and its solid), per occurrence-style row and
// per note, then the path-count work.
#include "step_styles.hpp"
#include "step_units.hpp"
#include <StepShape_ManifoldSolidBrep.hxx>
#include <StepShape_ShapeDefinitionRepresentation.hxx>
#include <StepRepr_ProductDefinitionShape.hxx>
#include <map>
#include <STEPControl_Reader.hxx>
#include <XSControl_WorkSession.hxx>
#include <iostream>
#include <set>
#include <vector>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: brepgpu-step-styles-fixture INPUT.stp\n";
    return 2;
  }
  try {
    STEPControl_Reader reader;
    if (reader.ReadFile(argv[1]) != IFSelect_RetDone)
      throw std::runtime_error("STEP read failed");
    auto model = Handle(StepData_StepModel)::DownCast(reader.WS()->Model());
    brepgpu::step_styles::Resolver resolver(model, reader.WS());
    std::set<std::uint64_t> products, children;
    for (int i = 1; i <= model->NbEntities(); ++i) {
      if (auto p = Handle(StepBasic_ProductDefinition)::DownCast(model->Value(i)); !p.IsNull())
        products.insert(model->IdentLabel(p));
      if (auto u = Handle(StepRepr_NextAssemblyUsageOccurrence)::DownCast(model->Value(i)); !u.IsNull())
        children.insert(model->IdentLabel(u->RelatedProductDefinition()));
    }
    // Each leaf product's solid and part representations, as main.cpp finds them.
    const brepgpu::step_units::RepresentationGraph representations(reader.WS()->Graph());
    auto label=[&](const Handle(Standard_Transient)& e){return std::uint64_t(model->IdentLabel(e));};
    std::map<std::uint64_t,std::uint64_t> solid_of;std::map<std::uint64_t,std::set<std::uint64_t>> reps_of;
    for(int i=1;i<=model->NbEntities();++i)if(auto s=Handle(StepShape_ManifoldSolidBrep)::DownCast(model->Value(i));!s.IsNull())
      for(const auto& holder:representations.holders(s)){const auto path=representations.transfer_path(holder);if(path.empty())continue;
        auto users=reader.WS()->Graph().Sharings(path.back());for(users.Start();users.More();users.Next())if(auto sdr=Handle(StepShape_ShapeDefinitionRepresentation)::DownCast(users.Value());!sdr.IsNull()){
          auto shape=Handle(StepRepr_ProductDefinitionShape)::DownCast(sdr->Definition().PropertyDefinition());if(shape.IsNull()||shape->Definition().ProductDefinition().IsNull())continue;
          const auto pd=label(shape->Definition().ProductDefinition());solid_of[pd]=label(s);for(const auto& r:path)reps_of[pd].insert(label(r));}}
    for(auto p:products)std::cout<<"product\t"<<p<<'\t'<<resolver.product_style(p,reps_of[p],solid_of.count(p)?solid_of[p]:0)<<'\n';
    std::vector<std::uint64_t> roots;
    for (auto p : products)
      if (!children.count(p))
        roots.push_back(p);
    if (roots.size() != 1)
      throw std::runtime_error("ambiguous root");
    for (const auto &row : resolver.occurrences(roots[0], [](auto, auto) { return false; })) {
      std::cout << "occurrence\t" << row.style << '\t' << row.face;
      for (auto u : row.path)
        std::cout << '\t' << u;
      std::cout << '\n';
    }
    resolver.finish();
    for (const auto &note : resolver.notes) {
      std::cout << "note\t" << note.stage;
      for (auto i : note.ids)
        std::cout << '\t' << i;
      std::cout << '\n';
    }
    std::cout << "style_path_work\t" << resolver.path_work() << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "step styles fixture failed: " << e.what() << "\n";
    return 1;
  }
}
