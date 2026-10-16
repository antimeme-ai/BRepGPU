#include "../native/mesher/boundary.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
std::vector<bgmesh::BoundaryEvaluationResult>
evaluate_literal(bgfd::Budget &budget, const bgmesh::BoundaryEvaluationBatch &batch) {
  std::vector<bgmesh::BoundaryEvaluationResult> results;
  for (const auto &task : batch.tasks()) {
    const auto t = bgmesh::import_exact(budget, batch.parameters().view(task.parameter));
    // Only edge400 has an interior source-t map knot in this fixture.
    const auto x = bgmesh::Rational(budget, 1).subtract(t).divide(bgmesh::Rational(budget, 2));
    results.push_back({task, 3, {x.to_double(), 0, 0}});
  }
  return results;
}
void epochs(const bgmesh::SourceStore &source,
            const std::shared_ptr<bgmesh::RunResource> &resource) {
  bgfd::Budget budget({1u << 27, uint64_t{1} << 32});
  bgmesh::DefinitionBoundaryForest forest(budget, source, bgmesh::Slot<bgmesh::DefinitionTag>{0},
                                          resource);
  const auto initial = forest.pending_evaluations();
  if (!initial)
    throw std::runtime_error("source map breakpoint omitted");
  require(initial->tasks().size() == 1, "source map breakpoint omitted");
  require(&initial->source().view() == &source.view() && initial->definition().value == 0,
          "evaluation batch lost source authority/definition lease");
  bool premature = false;
  try {
    (void)forest.freeze_face_boundary({0});
  } catch (const bgfd::Failure &) {
    premature = true;
  }
  require(premature, "unsettled original boundary exported");
  (void)forest.complete_evaluations(initial->epoch(), evaluate_literal(budget, *initial));
  const auto frozen = forest.freeze_face_boundary({0});
  require(&frozen->source.view() == &source.view() && frozen->definition.value == 0 &&
              frozen->face.value == 0 && frozen->boundary_generation == 0,
          "frozen boundary lost source/definition/face identity");
  require(frozen->edges.front().edge.value == 0 && frozen->edges.front().nodes.count == 3,
          "frozen boundary lost incident source chain");
  const auto whole = forest.freeze_definition_boundary();
  require(whole->edges.size() == source.view().definition_edges.at(0).size() &&
              whole->sample_paths.size() == whole->nodes.size() &&
              whole->edges.size() > frozen->edges.size(),
          "definition export omitted unused inventory or path rows");
  const auto &frozen_first = frozen->nodes[frozen->edges.front().nodes.first];
  require(
      frozen_first.original_parameter &&
          bgmesh::import_exact(budget, frozen->parameters.view(*frozen_first.original_parameter))
                  .to_double() == 1 &&
          frozen_first.key.source == 300,
      "frozen boundary reversed literal parameter/vertex association");
  const bgmesh::Slot<bgmesh::EdgeTag> edge{0};
  const auto chain = forest.chain(edge);
  require(chain.size() == 3, "descending edge did not retain source-t knot");
  require(forest.parameter(chain[0]).to_double() == 1 &&
              forest.parameter(chain[1]).to_double() == .5 &&
              forest.parameter(chain[2]).to_double() == -1,
          "range endpoint mapping was inverted");
  require(forest.sample(chain[0]).key.source == 300 && forest.sample(chain[2]).key.source == 301,
          "curve_same_sense incorrectly reversed source vertices");
  const auto intervals = forest.intervals(edge);
  require(intervals.size() == 2, "source knot did not produce two roots");
  const auto epoch = forest.begin_epoch();
  const bgmesh::Slot<bgmesh::FaceTag> first{0}, last{1};
  auto cut = intervals[0];
  cut.kind = bgmesh::BoundaryOwnerKind::face_cut;
  bool cut_rejected = false;
  try {
    (void)forest.interval(cut);
  } catch (const bgfd::Failure &) {
    cut_rejected = true;
  }
  require(cut_rejected, "face-owned cut aliased an original source edge interval");
  cut_rejected = false;
  try {
    forest.arrive(epoch, first, 0, std::span<const bgmesh::BoundaryKey>(&cut, 1));
  } catch (const bgfd::Failure &) {
    cut_rejected = true;
  }
  require(cut_rejected && !forest.horizon_complete(epoch),
          "cut request credited source-edge horizon");
  forest.arrive(epoch, first, 0, std::span<const bgmesh::BoundaryKey>(intervals.data(), 1));
  bool slow = false;
  try {
    (void)forest.prepare_splits(epoch);
  } catch (const bgfd::Failure &) {
    slow = true;
  }
  require(slow && forest.intervals(edge).size() == 2, "fast face split before slow horizon");
  const std::array last_requests{intervals[1], intervals[0]};
  forest.arrive(epoch, last, 0, last_requests);
  const auto batch = forest.prepare_splits(epoch);
  require(batch.tasks().size() == 2,
          "duplicate request not merged or slow face's distinct request lost");
  auto results = evaluate_literal(budget, batch);
  auto wrong = results;
  wrong[1].task = wrong[0].task;
  bool mismatched = false;
  try {
    (void)forest.complete_evaluations(epoch, wrong);
  } catch (const bgfd::Failure &) {
    mismatched = true;
  }
  require(mismatched && forest.chain(edge).size() == 3 && forest.face_generation(first) == 0,
          "mismatched result set partially published a boundary split");
  const auto update = forest.complete_evaluations(epoch, results);
  require(update.changed_faces.size() == 2 && update.new_samples.size() == 2 &&
              update.removed_intervals.size() == 2 && update.inserted_intervals.size() == 4,
          "shared split did not invalidate every incident generation once");
  require(forest.face_generation(first) == 1 && forest.face_generation(last) == 1 &&
              forest.chain(edge).size() == 5,
          "shared split state incorrect");
  const auto sample = forest.sample(update.new_samples[0]);
  require(sample.key.generator == bgmesh::NodeGenerator::original_edge && sample.key.ordinal == 2 &&
              forest.parameter(update.new_samples[0]).to_double() == .75,
          "sample identity/order or exact descending midpoint wrong");
  require(forest.sample_path(edge, update.new_samples[0]).empty() &&
              forest.sample_path(edge, chain[1]).empty(),
          "root midpoint/source seed path was not empty");
  const auto refrozen = forest.freeze_face_boundary(first);
  require(refrozen->boundary_generation == 1 && refrozen->edges.front().nodes.count == 5 &&
              frozen->boundary_generation == 0 && frozen->edges.front().nodes.count == 3,
          "new boundary mutation changed an old immutable snapshot");
  const auto second = forest.begin_epoch();
  forest.arrive(second, first, 1, {});
  forest.arrive(second, last, 1, {});
  const auto empty = forest.prepare_splits(second);
  require(empty.tasks().empty(), "settled epoch invented a split");
  (void)forest.complete_evaluations(second, {});
  const auto third = forest.begin_epoch();
  bool stale = false;
  try {
    forest.arrive(second, first, 1, {});
  } catch (const bgfd::Failure &) {
    stale = true;
  }
  require(stale && !forest.horizon_complete(third), "old epoch credited unchanged face generation");
  forest.arrive(third, first, 1, {});
  forest.arrive(third, last, 1, {});
  (void)forest.prepare_splits(third);
  (void)forest.complete_evaluations(third, {});
  const auto deeper = forest.begin_epoch();
  const auto split_children = forest.intervals(edge);
  const std::array deeper_requests{split_children[0], split_children[1]};
  forest.arrive(deeper, first, 1, deeper_requests);
  forest.arrive(deeper, last, 1, {});
  const auto deeper_batch = forest.prepare_splits(deeper);
  const auto deeper_update =
      forest.complete_evaluations(deeper, evaluate_literal(budget, deeper_batch));
  require(deeper_update.new_samples.size() == 2,
          "nested source interval did not split both children");
  const auto first_path = forest.sample_path(edge, deeper_update.new_samples[0]),
             second_path = forest.sample_path(edge, deeper_update.new_samples[1]);
  require(first_path.size() == 1 && second_path.size() == 1 && first_path[0] == 0 &&
              second_path[0] == 1,
          "literal descending root child paths lost first/second direction");
  const auto final = forest.freeze_definition_boundary();
  require(final->parameters.key() != deeper_batch.parameters().key(),
          "definition export aliases retained source evaluation table key");
  const auto final_chain = forest.chain(edge);
  const auto node_index = final->edges.front().nodes.first + 1;
  const auto path = final->sample_paths[static_cast<std::size_t>(node_index)];
  require(path.count == 1 && final->path_bits[static_cast<std::size_t>(path.first)] == 0 &&
              final_chain.size() == 7 && whole->edges.front().nodes.count == 3,
          "frozen definition paths changed an earlier immutable snapshot");
  bgmesh::DefinitionBoundaryForest other(budget, source, bgmesh::Slot<bgmesh::DefinitionTag>{1},
                                         resource);
  const auto other_pending = other.pending_evaluations();
  if (!other_pending)
    throw std::runtime_error("second definition omitted shared geometry");
  (void)other.complete_evaluations(other_pending->epoch(),
                                   evaluate_literal(budget, *other_pending));
  require(other.chain(edge).size() == 3 && other.sample(other.chain(edge)[0]).key.definition == 1 &&
              forest.sample(chain[0]).key.definition == 0,
          "physical identity leaked across definitions");
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected source fixture path");
    auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 28);
    const auto source = bgmesh::load_source(argv[1], resource);
    epochs(source, resource);
    std::cout << "native shared boundary forest: complete/stale epoch cases PASS\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
