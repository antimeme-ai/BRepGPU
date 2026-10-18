// The shared style resolver (native/step_styles.hpp) on a STEP model without
// a transfer. OCCT 8.0.1 recurses without bound when a plain
// shape_representation_relationship joins a mapped item's holder and its
// mapped representation (STEPControl_ActorRead::TransferRelatedSRR follows the
// relationship back into the holder), so the resolver's handling of that
// structure is tested on the model alone. Prints one line per occurrence-style
// row and per note, then the path-count work.
#include "step_styles.hpp"
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
