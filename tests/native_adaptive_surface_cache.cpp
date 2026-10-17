#include "adaptive_surface_cache.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace bgmesh;
using namespace bgmesh::adaptive;
namespace {
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2 || argc == 4, "usage: surface-cache source [cuda-module device]");
    auto resource = std::make_shared<RunResource>(uint64_t{1} << 30);
    bgfd::Budget budget({uint64_t{1} << 27, uint64_t{1} << 40}, resource.get());
    const auto source = load_source(argv[1], resource);
    const auto &face = source.view().faces.front();
    const auto carrier = source.view().carrier_index.find(face.carrier_id);
    const CarrierRequest request{carrier, {0, 1, 0, 1}};
    const auto cache =
        prepare_source_cache(source, std::span(&request, 1), resource, {}, {}, budget);
    CacheLimits limits;
    limits.cpu_threads = 4;
    SurfaceCacheSession session(resource, cache, limits);
    if (argc == 4)
      session.open_device({static_cast<int32_t>(std::stoi(argv[3])), argv[2]});
    ExactBuilder builder(resource, {{OwnerKind::run, 0, 998}, 1, 1});
    const auto tau = builder.append(Rational::from_double(budget, .02));
    const auto values = builder.seal();
    std::vector<RootAllowance> allowances;
    for (uint32_t i = 0; i < cache.roots().size(); ++i)
      allowances.push_back({{i}, tau});
    const SurfaceFaceIdentity identity{
        source.view().definition_index.find(face.definition_id), {0}, 1, 1, 1, 1};
    session.begin({1, identity, cache, values, allowances});
    JobContext context({uint64_t{1} << 26, uint64_t{1} << 40}, resource.get(), {});
    uint32_t classifications = 0;
    bool selected = false;
    for (uint32_t step = 0; step < 1000000; ++step) {
      const auto progress = session.step(context);
      if (progress == SurfaceProgress::needs_classification) {
        auto candidates = session.candidates();
        CandidateClassification classification(candidates);
        classification.labels.assign(candidates->boxes.size(), SurfaceClass::interior);
        session.classify(std::move(classification));
        ++classifications;
      } else if (progress == SurfaceProgress::selected) {
        selected = true;
        break;
      }
    }
    require(selected && classifications > 0,
            "surface owner did not settle classified source cover");
    const auto view = session.selection();
    require(view && !view.selection()->leaves.empty(), "surface owner omitted selected leaves");
    require(view.key().face == identity, "surface owner lost captured face identity");
    const auto stats = session.stats();
    Rational selected_area(budget);
    for (const auto &leaf : view.selection()->leaves) {
      const auto &table = view.selection()->values;
      const auto dx = import_exact(budget, table.view(leaf.box[1]))
                          .subtract(import_exact(budget, table.view(leaf.box[0])));
      const auto dy = import_exact(budget, table.view(leaf.box[3]))
                          .subtract(import_exact(budget, table.view(leaf.box[2])));
      require(dx.sign() > 0 && dy.sign() > 0, "selected leaf has nonpositive chart area");
      selected_area = selected_area.add(dx.multiply(dy));
    }
    require(selected_area.compare(Rational(budget, 1)) == 0,
            "source selection did not cover the requested unit rectangle");
    require(stats.packet_rounds == 0 && stats.actual_facet_tests == 0,
            "leaf exposure constructed an unnecessary full rectangle mesh");
    if (argc == 4)
      require(stats.device_points > 0 && stats.device_bounds > 0,
              "surface owner omitted real device geometry");
    {
      // An exact dyadic anchor beyond the compact grid-depth profile. Its
      // original POWER source, not a production restricted net, is the oracle.
      const auto half = Rational(budget, 1).divide(Rational(budget, 2));
      const auto u = half.add(Rational(budget, 1).divide(Rational::from_double(budget, 0x1p80)));
      const auto v = half.add(Rational(budget, 1).divide(Rational::from_double(budget, 0x1p81)));
      ExactBuilder anchors(resource, {{OwnerKind::run, 0, 997}, 1, 1});
      const std::array<ExactRef, 2> uv{anchors.append(u), anchors.append(v)};
      const auto anchor_values = anchors.seal();
      const std::array<SurfacePointInput, 2> requests{{{uv}, {uv}}};
      const auto before = session.stats().point_evaluations;
      session.sample({view, anchor_values, 1, requests});
      bool complete = false;
      for (uint32_t step = 0; step < 1000000; ++step) {
        const auto progress = session.step(context);
        if (progress == SurfaceProgress::points_complete) {
          complete = true;
          break;
        }
        if (progress == SurfaceProgress::needs_classification) {
          CandidateClassification classification(session.candidates());
          classification.labels.assign(classification.candidates->boxes.size(),
                                       SurfaceClass::interior);
          session.classify(std::move(classification));
        }
      }
      require(complete, "exact anchor request did not complete");
      const auto result = session.point_result();
      require(result->points.size() == 2 && session.stats().point_evaluations == before + 1,
              "duplicate exact anchors did not share the canonical point pool");
      const std::array<Rational, 3> original{
          Rational(budget, 3).multiply(u).subtract(u.multiply(u)).divide(Rational(budget, 2)),
          v.clone_into(budget),
          u.multiply(Rational(budget, 1).subtract(u))
              .multiply(v)
              .multiply(Rational(budget, 1).subtract(v))};
      for (const auto &point : result->points) {
        require(import_exact(budget, result->values.view(point.original_uv[0])).compare(u) == 0 &&
                    import_exact(budget, result->values.view(point.original_uv[1])).compare(v) == 0,
                "point pool rounded an exact anchor coordinate");
        Rational square(budget);
        for (uint32_t axis = 0; axis < 3; ++axis) {
          const auto residual =
              original[axis].subtract(Rational::from_double(budget, point.actual_xyz[axis]));
          square = square.add(residual.multiply(residual));
        }
        const auto departure = Rational::from_double(budget, point.departure_upper);
        require(square.compare(departure.multiply(departure)) <= 0,
                "anchor departure failed the independent original POWER source");
      }
      session.release_operation();
    }
    {
      // Raising an existing grid authority must invalidate the cover before
      // the point result is exposed. The retained old capability stays intact.
      const auto before_view = session.selection();
      const auto &point = before_view.selection()->points.front();
      const SurfacePointInput promote{point.original_uv, ArithmeticStage::p128};
      session.sample({before_view, before_view.selection()->values, 2, std::span(&promote, 1)});
      bool complete = false;
      for (uint32_t i = 0; i < 1000000; ++i) {
        const auto state = session.step(context);
        if (state == SurfaceProgress::needs_classification) {
          CandidateClassification mask(session.candidates());
          mask.labels.assign(mask.candidates->boxes.size(), SurfaceClass::interior);
          session.classify(std::move(mask));
        } else if (state == SurfaceProgress::points_complete) {
          complete = true;
          break;
        }
      }
      require(complete && session.selection().key().selection.cover_revision >
                              before_view.key().selection.cover_revision,
              "point promotion did not invalidate and reselect the cover");
      require(session.point_result()->points.front().key.stage == ArithmeticStage::p128,
              "point promotion lost demanded stage");
      session.release_operation();
      bool stale = false;
      try {
        session.sample({before_view, before_view.selection()->values, 3, std::span(&promote, 1)});
      } catch (const Error &error) {
        stale = error.code == ErrorCode::invalid_source;
      }
      require(stale, "stale point capability was admitted");
      const auto current = session.selection();
      auto invalid = promote;
      invalid.demanded = static_cast<ArithmeticStage>(99);
      bool refused = false;
      try {
        session.sample({current, before_view.selection()->values, 4, std::span(&invalid, 1)});
      } catch (const Error &error) {
        refused = error.code == ErrorCode::invalid_source;
      }
      require(refused && session.selection().key() == current.key(),
              "invalid complete point horizon mutated the cover");
      session.cancel();
      bool cancelled = false;
      try {
        (void)session.step(context);
      } catch (const Error &error) {
        cancelled = error.code == ErrorCode::cancelled;
      }
      require(cancelled, "surface operation ignored cancellation");
    }
    session.close();
    require(!view.selection()->points.empty(), "owner close invalidated immutable host view");
    {
      SurfaceCacheSession outside(resource, cache, limits);
      outside.begin({1, identity, cache, values, allowances});
      bool complete = false;
      bool attacked_partial = false;
      for (uint32_t step = 0; step < 1000000; ++step) {
        const auto progress = outside.step(context);
        if (progress == SurfaceProgress::needs_classification) {
          const auto candidates = outside.candidates();
          if (!attacked_partial) {
            CandidateClassification partial(candidates);
            bool refused = false;
            try {
              outside.classify(std::move(partial));
            } catch (const Error &error) {
              refused = error.code == ErrorCode::invalid_source;
            }
            require(refused, "partial classification mutated a live source horizon");
            attacked_partial = true;
          }
          CandidateClassification excluded(candidates);
          excluded.labels.assign(candidates->boxes.size(), SurfaceClass::outside);
          outside.classify(std::move(excluded));
        } else if (progress == SurfaceProgress::selected) {
          complete = true;
          break;
        }
      }
      require(complete, "outside mask failed to settle");
      require(outside.selection().selection()->leaves.empty(), "outside mask emitted material");
      const auto skipped = outside.stats();
      require(skipped.point_evaluations == 0 && skipped.control_restrictions == 0,
              "outside mask performed avoidable source geometry");
      outside.close();
    }
    {
      SurfaceCacheSession pruned(resource, cache, limits);
      pruned.begin({1, identity, cache, values, allowances});
      bool saw_children = false, complete = false;
      for (uint32_t step = 0; step < 1000000; ++step) {
        const auto progress = pruned.step(context);
        if (progress == SurfaceProgress::needs_classification) {
          const auto candidates = pruned.candidates();
          CandidateClassification masked(candidates);
          bool children = false;
          for (const auto &box : candidates->boxes)
            children = children || box.key.depth != 0;
          saw_children = saw_children || children;
          masked.labels.assign(candidates->boxes.size(),
                               children ? SurfaceClass::outside : SurfaceClass::interior);
          pruned.classify(std::move(masked));
        } else if (progress == SurfaceProgress::selected) {
          complete = true;
          break;
        }
      }
      require(complete && saw_children && pruned.selection().selection()->leaves.empty(),
              "fixture failed to exercise newly outside children");
      require(pruned.stats().control_restrictions == 0 && pruned.stats().point_evaluations == 4,
              "child geometry ran before its complete outside classification");
      pruned.close();
    }
    std::cout << "surface-cache source selection PASS\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
