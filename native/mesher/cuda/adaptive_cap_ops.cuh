// Included inside adaptive_cuda.cu's private owner namespace. No second bank,
// stream, ticket, allocation owner or poll path is introduced.
__global__ void cap_facets_kernel(const RootDescriptor *original, const DeviceCapFacet *tasks,
                                  uint32_t count, CapFacetBounds *output) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < count) {
    const auto &task = tasks[i];
    output[i] =
        numeric::bound_cap_facet_rn64(original[task.input.original.value], task.source, task.input);
  }
}
int32_t cap_facets(DeviceSession *s, const DeviceCapFacet *tasks, uint32_t count, uint64_t *ticket,
                   DeviceError *error) {
  return call(s, error, [&] {
    require(s && tasks && ticket && count && count <= s->config.frontier,
            "invalid cap-facet operation");
    require(!s->curve_mode, "cap facet on curve session");
    idle(*s);
    const auto *original = data<RootDescriptor>(s->buffers[host_originals]);
    for (uint32_t i = 0; i < count; ++i) {
      const auto &source = tasks[i].source;
      const auto &input = tasks[i].input;
      require(input.original.value < s->original_count &&
                  source.original.value == input.original.value,
              "foreign cap-facet original root");
      const auto &root = original[input.original.value];
      const bool sphere = source.kind == AnalyticCapKind::sphere_north ||
                          source.kind == AnalyticCapKind::sphere_south;
      require(
          (sphere && root.family == GeometryFamily::sphere) ||
              (source.kind == AnalyticCapKind::cone_apex && root.family == GeometryFamily::cone),
          "cap kind differs from original source family");
      require(source.stage == ArithmeticStage::rn64 || source.stage == ArithmeticStage::p128 ||
                  source.stage == ArithmeticStage::p192 || source.stage == ArithmeticStage::p256 ||
                  source.stage == ArithmeticStage::precise,
              "unknown cap source arithmetic stage");
      require(numeric::valid(source.pole_v), "invalid cap auxiliary pole enclosure");
      for (auto coordinate : source.ideal_xyz)
        require(numeric::valid(coordinate), "invalid cap auxiliary ideal enclosure");
      for (uint32_t corner = 0; corner < 2; ++corner) {
        for (uint32_t axis = 0; axis < 2; ++axis) {
          const auto coordinate = input.cut_uv[2 * corner + axis];
          require(numeric::valid(coordinate) &&
                      coordinate.lower >= root.original_box[2 * axis].lower &&
                      coordinate.upper <= root.original_box[2 * axis + 1].upper,
                  "cap cut coordinate outside original profile");
        }
        const auto coordinate = input.finite_v[corner];
        require(numeric::valid(coordinate) && coordinate.lower >= root.original_box[2].lower &&
                    coordinate.upper <= root.original_box[3].upper,
                "cap finite connector outside original profile");
      }
      for (auto coordinate : input.actual_xyz)
        require(numeric::finite(coordinate), "invalid cap actual coordinate");
      require(numeric::finite(input.allowance_down) && input.allowance_down >= 0.,
              "invalid cap allowance");
    }
    // A fixed conservative charge bounds this finite analytic operation.
    // Refusal occurs before any buffer growth, upload, kernel launch or ticket
    // mutation.
    charge(*s, product(count, uint64_t{1} << 20));
    output_capacity(*s, count, sizeof(CapFacetBounds));
    upload(*s, tasks, product(count, sizeof(DeviceCapFacet)));
    cap_facets_kernel<<<blocks(count), tile, 0, s->stream>>>(
        data<RootDescriptor>(s->buffers[originals]), data<DeviceCapFacet>(s->buffers[input]), count,
        data<CapFacetBounds>(s->buffers[result]));
    finish(*s, Operation::cap_facets, count, sizeof(CapFacetBounds), ticket);
  });
}
const CapCudaAPI cap_api{adaptive_cap_cuda_abi, sizeof(CapCudaAPI), cap_facets};
