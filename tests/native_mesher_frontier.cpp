#include "../native/mesher/face_frontier.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
void pending_order() {
  bgfd::Budget b({1u << 27, uint64_t{1} << 32});
  bgmesh::MutableFaceTopology mesh(b);
  const auto a = mesh.append_site({0, 0}), c = mesh.append_site({4, 0}),
             d = mesh.append_site({0, 4});
  const auto e = mesh.append_site({10, 0}), f = mesh.append_site({11, 0}),
             g = mesh.append_site({10, 1});
  const auto big = mesh.add_initial_triangle({a, c, d}),
             small = mesh.add_initial_triangle({e, f, g});
  mesh.seal_initial();
  bgmesh::FaceCellFrontier frontier(b, mesh, {{bgmesh::OwnerKind::face, 0, 91}, 7, 3});
  frontier.initialize();
  const auto big_key = frontier.key(big), small_key = frontier.key(small);
  const auto first_work = frontier.take_pending(1), second_work = frontier.take_pending(1);
  require(first_work.size() == 1 && first_work[0] == big_key && second_work.size() == 1 &&
              second_work[0] == small_key && frontier.take_pending(1).empty(),
          "bounded local pending stream duplicated or omitted initial cells");
  require(frontier.complete(small_key, bgmesh::CellNumericalState::needs_refine),
          "current completion rejected");
  const auto first = frontier.next();
  if (!first)
    throw std::runtime_error("missing pending cell");
  require(first->key == big_key && first->state == bgmesh::CellNumericalState::pending,
          "lower ready failure bypassed larger pending cell");
  auto stale = big_key;
  --stale.face.face_generation;
  require(!frontier.complete(stale, bgmesh::CellNumericalState::accepted),
          "stale face result accepted");
  stale = big_key;
  --stale.face.boundary_generation;
  require(!frontier.complete(stale, bgmesh::CellNumericalState::accepted),
          "stale boundary result accepted");
  require(frontier.complete(big_key, bgmesh::CellNumericalState::unresolved),
          "unresolved bound result rejected");
  const auto unresolved = frontier.next();
  require(unresolved && unresolved->key == big_key &&
              unresolved->state == bgmesh::CellNumericalState::unresolved,
          "higher unresolved cell did not block lower ready failure");
  const auto big_retry = frontier.retry(big);
  const auto retried_work = frontier.take_pending(1);
  require(retried_work.size() == 1 && retried_work[0] == big_retry &&
              frontier.take_pending(1).empty(),
          "retry did not publish one fresh local work key");
  require(!frontier.complete(big_key, bgmesh::CellNumericalState::accepted) &&
              frontier.complete(big_retry, bgmesh::CellNumericalState::accepted),
          "largest retry did not reject old request");
  const auto next = frontier.next();
  if (!next)
    throw std::runtime_error("lost failed cell");
  require(next->key == small_key && next->state == bgmesh::CellNumericalState::needs_refine,
          "accepted prefix blocked next failure");
  require(frontier.take_settled().size() == 1, "accepted prefix missing local notification");
  const auto fresh = frontier.retry(small);
  require(fresh.request_revision == 1 &&
              !frontier.complete(small_key, bgmesh::CellNumericalState::accepted),
          "old bound revision accepted");
  require(frontier.complete(fresh, bgmesh::CellNumericalState::accepted),
          "fresh revision rejected");
  require(!frontier.next(), "fully accepted frontier not settled");
  require(frontier.mark_contact_failure(fresh), "frozen accepted contact not marked");
  const auto contact = frontier.next();
  if (!contact)
    throw std::runtime_error("contact mark lost");
  require(contact->key == fresh && contact->contact_failure,
          "contact did not requeue retired accepted cell");
}
void equal_diameter_order() {
  bgfd::Budget b({1u << 27, uint64_t{1} << 32});
  bgmesh::MutableFaceTopology mesh(b);
  const auto a = mesh.append_site({0, 0}), c = mesh.append_site({1, 0}),
             d = mesh.append_site({1, 1}), e = mesh.append_site({0, 1});
  const auto first = mesh.add_initial_triangle({a, c, d});
  const auto second = mesh.add_initial_triangle({a, d, e});
  mesh.seal_initial();
  for (const bool reverse : {false, true}) {
    bgmesh::FaceCellFrontier frontier(b, mesh, {{bgmesh::OwnerKind::face, 0, 93}, 2, 4});
    frontier.initialize();
    const auto first_key = frontier.key(first), second_key = frontier.key(second);
    const auto lead = reverse ? second_key : first_key;
    const auto follow = reverse ? first_key : second_key;
    require(frontier.complete(lead, bgmesh::CellNumericalState::needs_refine) &&
                frontier.complete(follow, bgmesh::CellNumericalState::needs_refine),
            "tie completions rejected");
    const auto next = frontier.next();
    require(next && next->key == first_key, "completion order changed stable-lineage diameter tie");
  }
}
void local_edit_fold() {
  bgfd::Budget b({1u << 27, uint64_t{1} << 33});
  bgmesh::MutableFaceTopology mesh(b);
  const auto a = mesh.append_site({0, 0}), c = mesh.append_site({2, 0}),
             d = mesh.append_site({0, 1}), e = mesh.append_site({0, -1});
  const auto first = mesh.add_initial_triangle({a, c, d});
  (void)mesh.add_initial_triangle({c, a, e});
  mesh.seal_initial();
  bgmesh::FaceCellFrontier frontier(b, mesh, {{bgmesh::OwnerKind::face, 0, 92}, 0, 0});
  frontier.initialize();
  const auto old = frontier.key(first);
  const auto edge = mesh.find_edge(a, c);
  if (!edge)
    throw std::runtime_error("missing shared edge");
  const auto midpoint = mesh.bisect_internal_edge(*edge);
  // A second edit before consuming changes removes/recycles intermediate cells.
  const auto spoke = mesh.find_edge(a, midpoint);
  if (!spoke)
    throw std::runtime_error("missing child edge");
  (void)mesh.bisect_internal_edge(*spoke);
  const auto edits = mesh.take_changes();
  frontier.apply_changes(edits);
  require(!frontier.complete(old, bgmesh::CellNumericalState::accepted),
          "removed generation accepted after local fold");
  const auto work = frontier.take_pending(100);
  require(work.size() == mesh.stats().live_triangles && frontier.take_pending(1).empty(),
          "local pending stream retained intermediate generations");
  for (const auto cell : mesh.triangles())
    require(frontier.complete(frontier.key(cell), bgmesh::CellNumericalState::accepted),
            "surviving inserted generation not queued");
  require(!frontier.next() && frontier.take_settled().size() == mesh.triangles().size(),
          "local fold omitted/retained intermediate generations");
  require(frontier.stats().tracked_cells == mesh.stats().live_triangles,
          "frontier invents a second connectivity inventory");
}
} // namespace
int main() {
  try {
    pending_order();
    equal_diameter_order();
    local_edit_fold();
    std::cout << "native deterministic face frontier PASS\n";
  } catch (const bgfd::Failure &e) {
    std::cerr << e.what() << '\n';
    return 1;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
