#include "../native/mesher/contact_epoch.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace bgmesh;
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
ContactFaceDelta delta(const std::shared_ptr<RunResource> &resource, FaceFrontierIdentity face,
                       std::span<const ContactFacetInput> insert,
                       std::span<const FacetHandle> remove = {}) {
  auto result = std::allocate_shared<ContactFaceDeltaData>(
      RunAllocator<ContactFaceDeltaData>(resource), resource, face);
  result->inserted.assign(insert.begin(), insert.end());
  result->removed.assign(remove.begin(), remove.end());
  return result;
}
ContactFacetInput input(uint32_t slot, FaceFrontierIdentity face, uint32_t cell,
                        std::array<NodeSlot, 3> nodes, std::array<Vec3, 3> xyz) {
  const DeclaredContactEdge ordinary{ContactEdgeKind::internal_face, face.owner.id,
                                     face.face_generation, 0};
  ContactFacetInput result{
      {{slot}, 1}, {face, {cell, 1}, 0}, {0, nodes, xyz}, {ordinary, ordinary, ordinary}};
  for (std::size_t i = 0; i < 3; ++i)
    result.node_keys[i] = {0, NodeGenerator::face_site, uint64_t{1000} + nodes[i].value, 0, 0};
  return result;
}
void run(const SourceStore &source, const std::shared_ptr<RunResource> &resource) {
  bgfd::Budget budget({1ULL << 28, 1ULL << 35}, resource.get());
  const std::array faces{
      FaceFrontierIdentity{{OwnerKind::face, 0, source.view().faces[0].id.value}, 0, 0},
      FaceFrontierIdentity{{OwnerKind::face, 0, source.view().faces[1].id.value}, 0, 0}};
  ContactEpochOwner owner(budget, resource, source, {0}, {100});
  auto a = input(0, faces[0], 0, {NodeSlot{0}, NodeSlot{1}, NodeSlot{2}},
                 {{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}});
  auto b = input(1, faces[1], 0, {NodeSlot{1}, NodeSlot{0}, NodeSlot{3}},
                 {{{1, 0, 0}, {0, 0, 0}, {0, -1, 0}}});
  const auto cross = input(2, faces[1], 1, {NodeSlot{4}, NodeSlot{5}, NodeSlot{6}},
                           {{{.25, .25, -1}, {.25, .25, 1}, {.75, .25, 0}}});
  const DeclaredContactEdge shared{ContactEdgeKind::source_edge, 400, 0, 0};
  a.edges[0] = shared;
  b.edges[0] = shared;
  const auto epoch = owner.begin(faces);
  owner.arrive(epoch, delta(resource, faces[0], std::span(&a, 1)));
  bool early = false;
  try {
    owner.prepare(epoch);
  } catch (const Error &) {
    early = true;
  }
  require(early && owner.stats().bvh.live_facets == 0,
          "fast face changed frozen contact inventory");
  const std::array second{b, cross};
  owner.arrive(epoch, delta(resource, faces[1], second));
  owner.prepare(epoch);
  std::vector<ContactPairBatch> batches;
  while (const auto batch = owner.take_batch(epoch, 1))
    batches.push_back(*batch);
  require(batches.size() >= 2, "cross/shared-edge classifier cases not reached");
  std::vector<ContactPairCompleted> answers;
  for (auto it = batches.rbegin(); it != batches.rend(); ++it)
    answers.push_back(classify_contact_batch(budget, *it));
  for (const auto &answer : answers)
    require(owner.complete(answer), "current contact child rejected");
  const auto marks = owner.finish(epoch);
  require(marks.size() == 2 && marks[0] == a.cell && marks[1] == cross.cell,
          "whole frozen epoch did not mark BOTH forbidden cells only");
  const auto cached = owner.begin(faces);
  require(!owner.complete(answers.front()), "old epoch credited new horizon");
  owner.arrive(cached, delta(resource, faces[1], {}));
  owner.arrive(cached, delta(resource, faces[0], {}));
  owner.prepare(cached);
  require(!owner.take_batch(cached, 1), "unchanged contact pair classified again");
  require(owner.finish(cached) == marks, "unchanged forbidden contact lost its marks");
  const auto clean = owner.begin(faces);
  owner.arrive(clean, delta(resource, faces[0], {}));
  owner.arrive(clean, delta(resource, faces[1], {}, std::span(&cross.facet, 1)));
  owner.prepare(clean);
  require(!owner.take_batch(clean, 1) && owner.finish(clean).empty(),
          "removed facet retained old forbidden pair or rechecked unchanged pair");
  require(owner.stats().bvh.live_facets == 2 && (owner.stats().predicates.filtered_shared_edge +
                                                 owner.stats().predicates.filtered_projected) > 0,
          "declared shared edge did not use actual filtered classifier");
  std::cout << "native complete contact epochs: ordered/cache/invalidation PASS\n";
}
void replacement(const SourceStore &source, const std::shared_ptr<RunResource> &resource) {
  bgfd::Budget budget({1ULL << 28, 1ULL << 35}, resource.get());
  const std::array faces{
      FaceFrontierIdentity{{OwnerKind::face, 0, source.view().faces[0].id.value}, 0, 0},
      FaceFrontierIdentity{{OwnerKind::face, 0, source.view().faces[1].id.value}, 0, 0}};
  ContactEpochOwner owner(budget, resource, source, {0}, {100});
  auto declarations = [&](const ContactFacet &geometry, FaceFrontierIdentity face) {
    std::array<DeclaredContactEdge, 3> edges{};
    for (std::size_t i = 0; i < 3; ++i) {
      const auto a = std::min(geometry.nodes[i].value, geometry.nodes[(i + 1) % 3].value),
                 b = std::max(geometry.nodes[i].value, geometry.nodes[(i + 1) % 3].value);
      edges[i] = {ContactEdgeKind::internal_face, face.owner.id, face.face_generation,
                  (uint64_t{a} << 32) | b};
    }
    return edges;
  };
  auto a = input(0, faces[0], 0, {NodeSlot{0}, NodeSlot{1}, NodeSlot{2}},
                 {{{0, 0, 0}, {1, 1, 1}, {0, 1, 0}}});
  auto b = input(1, faces[0], 1, {NodeSlot{1}, NodeSlot{0}, NodeSlot{3}},
                 {{{1, 1, 1}, {0, 0, 0}, {1, 0, 0}}});
  auto blocker = input(2, faces[1], 0, {NodeSlot{4}, NodeSlot{5}, NodeSlot{6}},
                       {{{.1, .1, 0}, {.3, .1, 0}, {.1, .3, 0}}});
  a.edges = declarations(a.geometry, faces[0]);
  b.edges = declarations(b.geometry, faces[0]);
  blocker.edges = declarations(blocker.geometry, faces[1]);
  const auto epoch = owner.begin(faces);
  const std::array old{a, b};
  owner.arrive(epoch, delta(resource, faces[0], old));
  owner.arrive(epoch, delta(resource, faces[1], std::span(&blocker, 1)));
  owner.prepare(epoch);
  while (const auto batch = owner.take_batch(epoch, 8))
    require(owner.complete(classify_contact_batch(budget, *batch)),
            "replacement baseline child rejected");
  require(owner.finish(epoch).empty(), "old nonplanar quad should miss lower blocker");
  std::array<ContactProposedFacet, 2> proposed{
      ContactProposedFacet{
          {0, {NodeSlot{2}, NodeSlot{3}, NodeSlot{1}}, {{{0, 1, 0}, {1, 0, 0}, {1, 1, 1}}}}, {}},
      ContactProposedFacet{
          {0, {NodeSlot{3}, NodeSlot{2}, NodeSlot{0}}, {{{1, 0, 0}, {0, 1, 0}, {0, 0, 0}}}}, {}}};
  for (auto &facet : proposed) {
    facet.edges = declarations(facet.geometry, faces[0]);
    for (std::size_t i = 0; i < 3; ++i)
      facet.node_keys[i] = {0, NodeGenerator::face_site,
                            uint64_t{1000} + facet.geometry.nodes[i].value, 0, 0};
  }
  const std::array retired{a.facet, b.facet};
  require(!owner.approve_replacement(9, retired, proposed) && owner.stats().bvh.live_facets == 3,
          "proposed new diagonal crossed current facet or mutated rejection");
  const auto clear = owner.begin(faces);
  owner.arrive(clear, delta(resource, faces[0], {}));
  owner.arrive(clear, delta(resource, faces[1], {}, std::span(&blocker.facet, 1)));
  owner.prepare(clear);
  require(owner.finish(clear).empty(), "blocker retirement retained contacts");
  const auto approval = owner.approve_replacement(9, retired, proposed);
  if (!approval)
    throw std::runtime_error("empty-other-scene proposal rejected");
  require(owner.approval_current(*approval), "fresh proposal approval stale");
  ContactApprovalClaim claim{9, {a.cell, b.cell}, {}, {proposed[0].edges, proposed[1].edges}};
  for (std::size_t i = 0; i < 2; ++i)
    for (std::size_t c = 0; c < 3; ++c)
      claim.nodes[i][c] = {proposed[i].node_keys[c], proposed[i].geometry.xyz[c], {}};
  require(contact_approval_matches(*approval, claim), "same optimizer claim rejected");
  ++claim.proposal_ordinal;
  require(!contact_approval_matches(*approval, claim), "different proposal alias accepted");
  --claim.proposal_ordinal;
  ++claim.nodes[0][0].key.ordinal;
  require(!contact_approval_matches(*approval, claim),
          "same XYZ with changed physical identity accepted");
  --claim.nodes[0][0].key.ordinal;
  ++claim.old_cells[0].face.face_generation;
  require(!contact_approval_matches(*approval, claim), "foreign face generation claim accepted");
  const std::array updated{ContactFacetInput{{{0}, 2},
                                             {faces[0], {0, 2}, 0},
                                             proposed[0].geometry,
                                             proposed[0].edges,
                                             proposed[0].node_keys},
                           ContactFacetInput{{{1}, 2},
                                             {faces[0], {1, 2}, 0},
                                             proposed[1].geometry,
                                             proposed[1].edges,
                                             proposed[1].node_keys}};
  owner.commit_replacement(*approval, updated);
  require(!owner.approval_current(*approval) && owner.stats().bvh.live_facets == 2,
          "committed revision did not stale old proposal");
}

} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected source fixture");
    auto resource = std::make_shared<bgmesh::RunResource>(1ULL << 30);
    const auto source = bgmesh::load_source(argv[1], resource);
    run(source, resource);
    replacement(source, resource);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
