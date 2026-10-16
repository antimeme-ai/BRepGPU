#include "../native/mesher/boundary.hpp"
#include "../native/mesher/face_engine.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace bgmesh;
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
FaceBoundarySnapshot seed(bgfd::Budget &budget, GeometryContext &context,
                          const GeometryStore &geometry, DefinitionBoundaryForest &forest,
                          const std::shared_ptr<RunResource> &resource, uint64_t generation,
                          bool private_boundary_cut = false) {
  const auto &source = geometry.view().source.view();
  const auto original = forest.freeze_face_boundary({0});
  ExactBuilder builder(resource, {{OwnerKind::face, 0, 900}, generation, 1});
  const auto ref = [&](double value) {
    return builder.append(Rational::from_double(budget, value));
  };
  const auto &carrier = source.carriers[0];
  const GeometryBoundPrecision profile{80, 112};
  const uint32_t quotient = source.edges[2].degenerate ? 2u : 0u;
  const bool north = quotient == 2;
  const auto kind = carrier.kind == 3 ? AnalyticPoleKind::cone_apex
                    : north           ? AnalyticPoleKind::sphere_north
                                      : AnalyticPoleKind::sphere_south;
  const auto vlo = carrier.uv_bounds[2], vhi = carrier.uv_bounds[3];
  const auto vfar = north ? vlo : vhi, vpole = north ? vhi : vlo;
  const std::array<uint32_t, 2> cuts =
      north ? std::array<uint32_t, 2>{0, 1} : std::array<uint32_t, 2>{3, 2};
  const std::array<Vec2, 5> xy{{{0, vlo}, {1, vlo}, {1, vhi}, {0, vhi}, {.5, vpole}}};
  std::pmr::vector<FaceBoundarySite> sites(resource.get());
  for (uint32_t i = 0; i < 4; ++i) {
    const auto chain = forest.chain({i});
    const auto node = forest.sample(chain.front());
    const std::array point{ref(xy[i][0]), ref(xy[i][1])};
    sites.push_back(
        {point, point, {node.key, node.xyz, ref(source.edges[i].parameter_range[0])}, {}});
  }
  const std::array proxy{ref(.5), ref(vpole)};
  sites.push_back({proxy, proxy, {sites[quotient].node.key, sites[quotient].node.xyz, {}}, {}});
  std::array<BoundaryKey, 4> source_keys{};
  for (uint32_t i = 0; i < 4; ++i) {
    const auto keys = forest.intervals({i});
    source_keys[i] = keys.empty() ? BoundaryKey{source.edges[i].id.value, 0} : keys.front();
  }
  const std::array<Rational, 4> cut{Rational(budget, 0), Rational::from_double(budget, vfar),
                                    Rational(budget, 1), Rational::from_double(budget, vfar)};
  const std::array<double, 9> actual{
      sites[4].node.xyz[0],       sites[4].node.xyz[1],       sites[4].node.xyz[2],
      sites[cuts[0]].node.xyz[0], sites[cuts[0]].node.xyz[1], sites[cuts[0]].node.xyz[2],
      sites[cuts[1]].node.xyz[0], sites[cuts[1]].node.xyz[1], sites[cuts[1]].node.xyz[2]};
  const auto bound = bound_analytic_pole_fan(
      context, carrier,
      {kind,
       cut,
       actual,
       {Rational::from_double(budget, vpole), Rational::from_double(budget, vpole)},
       1,
       profile});
  const BoundaryKey cut_key = private_boundary_cut
                                  ? BoundaryKey{900, 0, BoundaryOwnerKind::face_cut}
                                  : source_keys[north ? 0u : 2u];
  const std::array cut_refs{ref(0), ref(vfar), ref(1), ref(vfar)};
  const auto zero = ref(0), one = ref(1), position = builder.append(bound.position_upper),
             band = builder.append(bound.band_upper);
  const auto angular = builder.append(bound.angular_upper),
             radial = builder.append(bound.radial_upper),
             corner = builder.append(bound.corner_upper);
  const auto pole_departure = builder.append(bound.pole_departure_upper);
  const auto pcurve = north ? std::array{one, zero} : std::array{zero, one};
  std::pmr::vector<FaceBoundarySegment> segments(resource.get());
  std::pmr::vector<FaceUseDisposition> uses(resource.get());
  for (uint32_t i = 0; i < 4; ++i) {
    const std::array parameters{ref(source.edges[i].parameter_range[0]),
                                ref(source.edges[i].parameter_range[1])};
    segments.push_back({i, source_keys[i], {i}, parameters, {0, 0}});
    uses.push_back({{i},
                    i == quotient ? FaceUseKind::declared_quotient : FaceUseKind::ordinary_chain,
                    {i, 1},
                    parameters,
                    i == quotient ? std::optional(pcurve) : std::nullopt});
  }
  auto result = std::allocate_shared<FaceBoundarySnapshotData>(
      RunAllocator<FaceBoundarySnapshotData>(resource), resource, geometry, Slot<DefinitionTag>{0},
      Slot<FaceTag>{0}, std::array<uint64_t, 3>{3, generation, 0},
      WorkingChartKind::analytic_unwrapped_uv, builder.seal());
  result->sites.assign(sites.begin(), sites.end());
  result->segments.assign(segments.begin(), segments.end());
  result->uses.assign(uses.begin(), uses.end());
  result->loops.push_back({{0}, true, {0, 4}, {0, 4}});
  result->source_boundary = original;
  result->construction_domain_present = true;
  FrozenPoleCap cap{};
  cap.kind = kind;
  cap.quotient_use = {quotient};
  cap.meridian_uses = {Slot<CoedgeTag>{3}, Slot<CoedgeTag>{1}};
  cap.consumed_meridians = {Range<FaceBoundarySegment>{3, 1}, Range<FaceBoundarySegment>{1, 1}};
  cap.pcurve_parameter = pcurve;
  cap.pole_site = 4;
  cap.pieces = {0, 1};
  cap.radial_features = {source_keys[3], source_keys[1]};
  cap.regular_sheet_sign = bound.regular_sheet_sign;
  cap.fixed_pole_upper = pole_departure;
  cap.fixed_band_upper = band;
  cap.precision = profile;
  result->pole_caps.push_back(cap);
  // All refs must precede sealing. The original band is the finite source v.
  result->pole_caps.back().original_pole_v = {sites[quotient].original_uv[1],
                                              sites[(quotient + 1) % 4].original_uv[1]};
  result->pole_pieces.push_back(
      {0, cuts, cut_key, cut_refs, bound.facet_roles, position, band, angular, radial, corner});
  return result;
}
void run(const std::filesystem::path &path) {
  auto resource = std::make_shared<RunResource>(uint64_t{1} << 30);
  const auto source = load_source(path, resource);
  bgfd::Budget budget({uint64_t{1} << 28, uint64_t{1} << 38}, resource.get());
  const auto geometry =
      compile_geometry(source, resource, {{OwnerKind::geometry, 0, 700}, 3, 0}, budget);
  GeometryContext context(budget);
  DefinitionBoundaryForest forest(budget, source, {0}, resource);
  require(!forest.pending_evaluations(),
          "authored analytic pole source unexpectedly has knot tasks");
  const std::array<Rational, 4> factor{Rational(budget, 1), Rational(budget), Rational(budget),
                                       Rational(budget, 1)};
  const Rational tau(budget, 1);
  const auto boundary = seed(budget, context, geometry, forest, resource, 7);
  FaceEngine engine(budget, context, boundary,
                    {factor, {Rational(budget, 4), Rational(budget, 1), Rational(budget, 1), tau}},
                    tau, 7);
  require(!engine.take_cell_work(32), "symbolic pole fan became an ordinary UV numerical packet");
  require(engine.advance().kind == FaceAdvanceKind::settled,
          "finite pole fan failed actual host bound");
  require(engine.stats().host_pole_cells == 1, "actual host fan execution was omitted from costs");
  const auto frozen = engine.freeze_result();
  const uint32_t quotient = source.view().edges[2].degenerate ? 2u : 0u;
  require(frozen->sites.size() == 3 && frozen->triangles.size() == 1 &&
              frozen->source_sites.size() == 5 && !frozen->source_sites[quotient] &&
              !frozen->source_sites[(quotient + 1) % 4] && frozen->source_sites[4] &&
              frozen->triangles[0].map_kind == CellMapKind::analytic_pole_fan_quad &&
              frozen->triangles[0].pole_piece == 0,
          "finite cap lost complete source inventory or actual symbolic cell association");
  const auto contact_seed = seed(budget, context, geometry, forest, resource, 8);
  FaceEngine contact(budget, context, contact_seed,
                     {factor, {Rational(budget, 4), Rational(budget, 1), Rational(budget, 1), tau}},
                     tau, 7);
  require(!contact.take_cell_work(32) && contact.advance().kind == FaceAdvanceKind::settled,
          "second cap failed canonical host settlement");
  const auto settled = contact.take_settled();
  require(settled.size() == 1 && contact.mark_contact_failure(settled[0]),
          "cap contact mark lost full cell identity");
  const auto request = contact.advance();
  require(request.kind == FaceAdvanceKind::boundary_request && request.boundary &&
              *request.boundary == contact_seed->pole_pieces[0].cut,
          "cap contact failure became ordinary UV heuristic/LEPP work");
  const auto tight_seed = seed(budget, context, geometry, forest, resource, 9);
  const auto tight_tau = Rational(budget, 1).divide(Rational(budget, 1000));
  const auto &cap = tight_seed->pole_caps[0];
  const auto &piece = tight_seed->pole_pieces[0];
  const long double v = (static_cast<long double>(source.view().carriers[0].uv_bounds[2]) +
                         static_cast<long double>(source.view().carriers[0].uv_bounds[3])) /
                        2;
  const long double alpha = source.view().carriers[0].radii[1];
  const bool sphere = source.view().carriers[0].kind == 4;
  const long double radius = sphere ? std::cos(v) : 1 + v * std::sin(alpha);
  const std::array<long double, 3> original_point{radius * std::cos(.5L), radius * std::sin(.5L),
                                                  sphere ? std::sin(v) : v * std::cos(alpha)};
  long double residual_squared = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto affine = .5L * tight_seed->sites[cap.pole_site].node.xyz[axis] +
                        .25L * tight_seed->sites[piece.cut_sites[0]].node.xyz[axis] +
                        .25L * tight_seed->sites[piece.cut_sites[1]].node.xyz[axis];
    const auto delta = original_point[axis] - affine;
    residual_squared += delta * delta;
  }
  require(residual_squared > 1e-6L,
          "independent original analytic point did not expose tight fan error");
  FaceEngine tight(
      budget, context, tight_seed,
      {factor, {Rational(budget, 4), Rational(budget, 1), Rational(budget, 1), tight_tau}},
      tight_tau, 7);
  require(!tight.take_cell_work(32), "failed symbolic fan became ordinary numerical work");
  const auto refinement = tight.advance();
  require(refinement.kind == FaceAdvanceKind::boundary_request && refinement.boundary,
          "independently excessive fan error became accepted mesh or ordinary candidate");
  bool private_rejected = false;
  try {
    const auto invalid_seed = seed(budget, context, geometry, forest, resource, 10, true);
    const FaceEngine invalid_engine(
        budget, context, invalid_seed,
        {factor, {Rational(budget, 4), Rational(budget, 1), Rational(budget, 1), tau}}, tau, 7);
  } catch (const Error &error) {
    private_rejected = error.code == ErrorCode::invalid_source;
  }
  require(private_rejected, "source boundary was silently relabeled as a private face cut");
  const auto floor_seed = seed(budget, context, geometry, forest, resource, 11);
  const auto floor_tau = Rational(budget, 1).divide(Rational(budget, 1'000'000'000'000'000'000LL));
  const auto &original_cap = floor_seed->pole_caps[0];
  const long double finite_pole = source.view().carriers[0].uv_bounds[quotient == 2 ? 3 : 2];
  const long double finite_radius =
      sphere ? std::cos(finite_pole) : 1 + finite_pole * std::sin(alpha);
  require(std::abs(finite_radius) > 1e-18L,
          "authored finite source pole did not expose the fixed band at this tolerance");
  require(original_cap.fixed_band_upper.has_value(), "authored source fixed band was not retained");
  FaceEngine floor(
      budget, context, floor_seed,
      {factor, {Rational(budget, 4), Rational(budget, 1), Rational(budget, 1), floor_tau}},
      floor_tau, 7);
  require(!floor.take_cell_work(32) && floor.advance().kind == FaceAdvanceKind::unresolved,
          "fixed source band exhausted tolerance but continued angular/radial subdivision");
}
} // namespace
int main(int argc, char **argv) {
  try {
    require(argc == 2, "expected north pole source fixture");
    const auto path = std::filesystem::path(argv[1]);
    run(path);
    run(path.parent_path() / (path.stem().string() + "-distinct.bgsource"));
    std::cout << "PASS same-engine symbolic pole construction/source mapping\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
