// Included in adaptive_cuda.cu's existing private owner namespace. This operation
// uses its bank, reservation callbacks, staging, event, ticket, poll and drain.
__device__ TriangleCoefficient trim_coefficient(uint32_t degree, uint32_t ordinal) {
  uint32_t i = 0;
  while (ordinal > degree - i) {
    ordinal -= degree - i + 1;
    ++i;
  }
  return {degree, i, ordinal};
}
__global__ void analytic_source_triangles(const RootDescriptor *original,
    const SourceTriangleInput *tasks, uint32_t count, SourceTriangleBounds *output) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= count)
    return;
  const auto &root = original[tasks[i].original.value];
  if (root.family != GeometryFamily::rational_tensor)
    output[i] = numeric::bound_analytic_source_triangle_rn64(root, tasks[i]);
}
__global__ void rational_source_triangles(const RootDescriptor *original, const Interval *controls,
    const SourceTriangleInput *tasks, uint32_t count, SourceTriangleBounds *output) {
  __shared__ Interval source[warps][64], triangle[warps][112];
  __shared__ SourceTriangleInput inputs[warps];
  const auto warp = threadIdx.x / 32, lane = threadIdx.x % 32;
  const auto i = blockIdx.x * warps + warp;
  if (i >= count)
    return;
  const auto &root = original[tasks[i].original.value];
  if (root.family != GeometryFamily::rational_tensor)
    return;
  if (!lane)
    inputs[warp] = tasks[i];
  for (uint32_t j = lane; j < root.controls.count; j += 32)
    source[warp][j] = controls[root.controls.first + j];
  __syncwarp();
  const auto &input = inputs[warp];
  const auto degree = root.degree_u + root.degree_v;
  const auto homogeneous_count = (degree + 1) * (degree + 2) / 2;
  bool invalid = false;
  double weight = numeric::infinity();
  if (lane < homogeneous_count) {
    const auto coefficient = trim_coefficient(degree, lane);
    for (uint32_t channel = 0; channel < 4; ++channel) {
      const auto value = numeric::source_triangle_homogeneous_coefficient_rn64(
          root, source[warp], input, coefficient, channel);
      triangle[warp][lane * 4 + channel] = value;
      invalid = invalid || !numeric::valid(value);
    }
    weight = triangle[warp][lane * 4 + 3].lower;
  }
  __syncwarp();
  double maximum = 0., radius = 0.;
  const auto residual_count = (degree + 2) * (degree + 3) / 2;
  // Degree7 has36 residual coefficients; the final four use lanes0..3 again.
  for (uint32_t j = lane; j < residual_count; j += 32) {
    const auto coefficient = trim_coefficient(degree + 1, j);
    Interval residual[3]{};
    for (uint32_t axis = 0; axis < 3; ++axis)
      residual[axis] = numeric::source_triangle_residual_coefficient_rn64(
          root, triangle[warp], input, coefficient, axis);
    const auto upper = numeric::norm_upper(residual);
    invalid = invalid || !numeric::finite(upper);
    maximum = numeric::maximum(maximum, upper);
    radius = numeric::maximum(radius, numeric::source_triangle_interval_radius_rn64(residual));
  }
  const auto bad = __any_sync(0xffffffffU, invalid);
  for (unsigned offset = 16; offset; offset /= 2) {
    maximum = numeric::maximum(maximum, __shfl_down_sync(0xffffffffU, maximum, offset));
    weight = numeric::minimum(weight, __shfl_down_sync(0xffffffffU, weight, offset));
    radius = numeric::maximum(radius, __shfl_down_sync(0xffffffffU, radius, offset));
  }
  if (!lane)
    output[i] = bad ? SourceTriangleBounds{}
                    : numeric::finish_source_triangle_rn64(root, input, maximum, weight, radius);
}
int32_t source_triangles(DeviceSession *s, const SourceTriangleInput *tasks, uint32_t count,
                         uint64_t *ticket, DeviceError *error) {
  return call(s, error, [&] {
    require(s && tasks && ticket && count && count <= s->config.frontier,
            "invalid source-triangle operation");
    require(!s->curve_mode, "source triangle on curve session");
    idle(*s);
    // Conservative finite work charge for the admitted p,q<=3 coefficient work.
    charge(*s, product(count, uint64_t{1} << 20));
    bool rational = false, analytic = false;
    const auto *original = data<RootDescriptor>(s->buffers[host_originals]);
    for (uint32_t i = 0; i < count; ++i) {
      const auto &task = tasks[i];
      require(task.original.value < s->original_count, "foreign source-triangle original root");
      const auto &root = original[task.original.value];
      if (root.family == GeometryFamily::rational_tensor) {
        require(root.degree_u <= 3 && root.degree_v <= 3 && root.controls.count <= 64,
                "unsupported source-triangle tensor degree");
        control_range(*s, root.controls);
        rational = true;
      } else {
        analytic = true;
      }
      for (auto parameter : task.normalized_uv)
        require(numeric::valid(parameter) && parameter.lower >= 0. && parameter.upper <= 1.,
                "source-triangle parameter outside original root");
      for (auto coordinate : task.affine_xyz)
        require(numeric::valid(coordinate), "invalid SAME-L source-triangle coordinate");
      require(numeric::finite(task.allowance_down) && task.allowance_down >= 0.,
              "invalid source-triangle allowance");
    }
    output_capacity(*s, count, sizeof(SourceTriangleBounds));
    upload(*s, tasks, product(count, sizeof(SourceTriangleInput)));
    if (analytic)
      analytic_source_triangles<<<blocks(count), tile, 0, s->stream>>>(
          data<RootDescriptor>(s->buffers[originals]), data<SourceTriangleInput>(s->buffers[input]),
          count, data<SourceTriangleBounds>(s->buffers[result]));
    if (rational)
      rational_source_triangles<<<blocks(count, warps), warp_threads, 0, s->stream>>>(
          data<RootDescriptor>(s->buffers[originals]), data<Interval>(s->buffers[controls]),
          data<SourceTriangleInput>(s->buffers[input]), count,
          data<SourceTriangleBounds>(s->buffers[result]));
    finish(*s, Operation::trim_source_triangles, count, sizeof(SourceTriangleBounds), ticket);
  });
}
const TrimCudaAPI trim_api{adaptive_trim_cuda_abi, sizeof(TrimCudaAPI), source_triangles};
