#include "boundary.hpp"
#include <iostream>
#include <vector>
using namespace bgmesh;
namespace {
void require(bool ok, const char *what) {
  if (!ok)
    throw std::runtime_error(what);
}
std::vector<BoundaryEvaluationResult> points(bgfd::Budget &b,
                                             const BoundaryEvaluationBatch &batch) {
  std::vector<BoundaryEvaluationResult> r;
  for (const auto &task : batch.tasks()) {
    const auto t = import_exact(b, batch.parameters().view(task.parameter));
    const auto x = Rational(b, 1).subtract(t).divide(Rational(b, 2));
    r.push_back({task, 3, {x.to_double(), 0, 0}});
  }
  return r;
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected original shared-boundary source fixture");
    auto run = std::make_shared<RunResource>(uint64_t{1} << 28);
    bgfd::Budget b({uint64_t{1} << 26, uint64_t{1} << 34}, run.get());
    auto source = load_source(argv[1], run);
    DefinitionBoundaryForest f(b, source, {0}, run);
    if (const auto initial = f.pending_evaluations())
      (void)f.complete_evaluations(initial->epoch(), points(b, *initial));
    const auto sample = f.chain({0})[1];
    const auto old = f.sample(sample);
    const auto frozen = f.freeze_definition_boundary();
    const auto epoch = f.begin_epoch();
    auto forged = BoundaryPointRevision{sample, old.key, old.constructed_revision, {.25, 0, 0}};
    ++forged.expected_key.source;
    bool refused = false;
    try {
      f.stage_owner_promotions(epoch, std::span(&forged, 1));
    } catch (const bgfd::Failure &) {
      refused = true;
    }
    require(refused, "promotion admitted wrong logical key");
    const auto endpoint = f.chain({0})[0];
    const auto vertex = f.sample(endpoint);
    auto forbidden =
        BoundaryPointRevision{endpoint, vertex.key, vertex.constructed_revision, {0, 0, 0}};
    refused = false;
    try {
      f.stage_owner_promotions(epoch, std::span(&forbidden, 1));
    } catch (const bgfd::Failure &) {
      refused = true;
    }
    require(refused, "promotion replaced fixed original vertex");
    const auto revision = BoundaryPointRevision{
        sample, old.key, old.constructed_revision, {0x1.0000000000001p-2, 0, 0}};
    f.stage_owner_promotions(epoch, std::span(&revision, 1));
    const auto wire = source.view().edge_index.find(EdgeId{404});
    const auto keys = f.intervals(wire);
    f.request_owner_splits(epoch, keys);
    f.arrive(epoch, {0}, 0, {});
    f.arrive(epoch, {1}, 0, {});
    require(f.sample(sample).xyz == old.xyz, "staged promotion changed installed Q");
    const auto batch = f.prepare_splits(epoch);
    require(batch.tasks().size() == 1, "wire owner request omitted");
    auto wrong = points(b, batch);
    ++wrong[0].task.sample.value;
    refused = false;
    try {
      (void)f.complete_evaluations(epoch, wrong);
    } catch (const bgfd::Failure &) {
      refused = true;
    }
    require(refused && f.sample(sample).xyz == old.xyz, "failed split committed staged promotion");
    const auto update = f.complete_evaluations(epoch, points(b, batch));
    require(update.new_samples.size() == 1 && update.revised_samples.size() == 1 &&
                update.changed_faces.size() == 2,
            "combined wire split/promotion lost owner/incident updates");
    const auto now = f.sample(sample);
    require(now.key == old.key && now.constructed_revision == old.constructed_revision + 1 &&
                now.xyz == revision.xyz,
            "point revision changed logical identity or omitted value/revision");
    require(f.chain(wire).size() == 3, "wire edge split missing");
    const auto after = f.freeze_definition_boundary();
    require(after->constructed_revisions.size() == after->nodes.size() &&
                frozen->constructed_revisions.size() == frozen->nodes.size(),
            "snapshot omitted constructed revisions");
    require(frozen->constructed_revisions[1] == 0 && after->constructed_revisions[1] == 1 &&
                frozen->nodes[1].xyz == old.xyz,
            "revision changed prior immutable snapshot");
    const auto next = f.begin_epoch();
    refused = false;
    try {
      f.stage_owner_promotions(next, std::span(&revision, 1));
    } catch (const bgfd::Failure &) {
      refused = true;
    }
    require(refused, "stale constructed revision admitted");
    std::cout << "curve forest GREEN\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
