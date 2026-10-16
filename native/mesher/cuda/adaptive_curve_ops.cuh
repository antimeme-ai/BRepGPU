// Included inside adaptive_cuda.cu's owned implementation namespace. This unit
// shares Buffer, DeviceSession, reservations, scan, tickets and drain
// functions.
__global__ void curve_analytic_samples(const CurveRootDescriptor *source, const Interval *nets,
                                       const CurveDeviceSample *tasks, uint32_t count,
                                       CurveSampleResult *pool, CurveSampleResult *output) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= count)
    return;
  const auto &task = tasks[i];
  const auto &root = source[task.input.original.value];
  if (!task.supplied && root.family == CurveFamily::rational)
    return;
  const auto value = task.supplied
                         ? task.supplied_result
                         : numeric::evaluate_curve_sample_rn64(root, nets, task.input, nullptr);
  pool[task.destination] = output[i] = value;
}
__global__ void curve_rational_samples(const CurveRootDescriptor *source, const Interval *nets,
                                       const CurveDeviceSample *tasks, uint32_t count,
                                       CurveSampleResult *pool, CurveSampleResult *output) {
  __shared__ Interval workspace[warps][2][132];
  const auto warp = threadIdx.x / 32, lane = threadIdx.x % 32;
  const auto i = blockIdx.x * warps + warp;
  if (i >= count)
    return;
  const auto &task = tasks[i];
  const auto &root = source[task.input.original.value];
  if (task.supplied || root.family != CurveFamily::rational)
    return;
  const auto t = numeric::curve_normalized_parameter_rn64(root, task.input.t);
  if (!numeric::valid(t)) {
    if (!lane)
      pool[task.destination] = output[i] = CurveSampleResult{};
    return;
  }
  const auto stride = root.dimension + 1;
  for (uint32_t j = lane; j < root.controls.count; j += 32)
    workspace[warp][0][j] = nets[root.controls.first + j];
  __syncwarp();
  uint32_t current = 0;
  for (uint32_t level = 0; level < root.degree; ++level) {
    for (uint32_t j = lane; j < (root.degree - level) * stride; j += 32)
      workspace[warp][1 - current][j] =
          numeric::curve_lerp(workspace[warp][current][j], workspace[warp][current][j + stride], t);
    __syncwarp();
    current = 1 - current;
  }
  if (!lane) {
    const auto value =
        numeric::curve_sample_from_homogeneous(root, workspace[warp][current], task.input);
    pool[task.destination] = output[i] = value;
  }
}
__global__ void curve_pair_kernel(const CurveRootDescriptor *roots, Interval *pool,
                                  const CurveDeviceRestriction *tasks, uint32_t count) {
  __shared__ Interval workspace[warps][2][132];
  const auto warp = threadIdx.x / 32, lane = threadIdx.x % 32;
  const auto i = blockIdx.x * warps + warp;
  if (i >= count)
    return;
  const auto &task = tasks[i];
  const auto &root = roots[task.root.value];
  const auto stride = root.dimension + 1;
  for (uint32_t j = lane; j < task.parent.count; j += 32)
    workspace[warp][0][j] = pool[task.parent.first + j];
  __syncwarp();
  if (lane < stride) {
    pool[task.left_destination + lane] = workspace[warp][0][lane];
    pool[task.right_destination + root.degree * stride + lane] =
        workspace[warp][0][root.degree * stride + lane];
  }
  uint32_t current = 0;
  for (uint32_t level = 1; level <= root.degree; ++level) {
    for (uint32_t j = lane; j < (root.degree - level + 1) * stride; j += 32)
      workspace[warp][1 - current][j] = numeric::curve_midpoint(
          workspace[warp][current][j], workspace[warp][current][j + stride]);
    __syncwarp();
    current = 1 - current;
    if (lane < stride) {
      pool[task.left_destination + level * stride + lane] = workspace[warp][current][lane];
      pool[task.right_destination + (root.degree - level) * stride + lane] =
          workspace[warp][current][(root.degree - level) * stride + lane];
    }
    __syncwarp();
  }
}
__device__ CurveDeviceNodeAnswer curve_decide(const CurveDeviceNode &node,
                                              const CurveNodeBounds &bound) {
  CurveDeviceNodeAnswer answer{};
  answer.bounds = bound;
  answer.selected = CurveCorrespondence::affine_parameter;
  if (bound.status != NumericStatus::ready) {
    answer.action = 2;
    return answer;
  }
  auto upper = bound.parameter_upper;
  if (node.allow_image && bound.image_available && bound.image_upper <= upper) {
    upper = bound.image_upper;
    answer.selected = bound.correspondence;
  }
  answer.action = upper <= node.tolerance_down ? 0U : 1U;
  return answer;
}
__device__ CurveNodeInput curve_actual_node(const CurveDeviceNode &node,
                                            const CurveSampleResult *points, bool &ready) {
  auto input = node.input;
  ready = true;
  for (uint32_t end = 0; end < 2; ++end) {
    const auto &sample = points[node.points[end]];
    ready = ready && sample.status == NumericStatus::ready;
    input.endpoint_departures[end] = sample.departure_upper;
    for (uint32_t a = 0; a < 3; ++a)
      input.actual_endpoints[end * 3 + a] = sample.actual_xyz[a];
  }
  return input;
}
__global__ void curve_install_nodes(const CurveDeviceNode *tasks, uint32_t count,
                                    CurveDeviceNode *pool, uint32_t *frontier) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < count) {
    pool[tasks[i].slot] = tasks[i];
    frontier[i] = tasks[i].slot;
  }
}
__global__ void curve_analytic_nodes(const CurveRootDescriptor *roots, const CurveDeviceNode *pool,
                                     const uint32_t *frontier, uint32_t count,
                                     const CurveSampleResult *points,
                                     CurveDeviceNodeAnswer *output) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= count)
    return;
  const auto &node = pool[frontier[i]];
  const auto &root = roots[node.input.root.value];
  if (!node.cached && root.family == CurveFamily::rational)
    return;
  auto bound = node.cached_bounds;
  if (!node.cached) {
    bool ready = false;
    const auto input = curve_actual_node(node, points, ready);
    bound = ready ? numeric::bound_curve_node_rn64(root, nullptr, input) : CurveNodeBounds{};
  }
  output[i] = curve_decide(node, bound);
}
__global__ void curve_rational_nodes(const CurveRootDescriptor *roots, const Interval *nets,
                                     const CurveDeviceNode *pool, const uint32_t *frontier,
                                     uint32_t count, const CurveSampleResult *points,
                                     CurveDeviceNodeAnswer *output) {
  __shared__ Interval controls[warps][132];
  __shared__ CurveNodeInput predicates[warps];
  __shared__ double anchors[warps][3];
  __shared__ uint32_t ready[warps];
  const auto warp = threadIdx.x / 32, lane = threadIdx.x % 32;
  const auto i = blockIdx.x * warps + warp;
  if (i >= count)
    return;
  const auto &node = pool[frontier[i]];
  const auto &root = roots[node.input.root.value];
  if (node.cached || root.family != CurveFamily::rational)
    return;
  for (uint32_t j = lane; j < node.input.controls.count; j += 32)
    controls[warp][j] = nets[node.input.controls.first + j];
  if (!lane) {
    bool point_ready = false;
    predicates[warp] = curve_actual_node(node, points, point_ready);
    ready[warp] = point_ready;
  }
  __syncwarp();
  if (!lane) {
    for (uint32_t a = 0; a < 3; ++a) {
      anchors[warp][a] = 0.;
      if (a < root.dimension) {
        const auto x = numeric::div(controls[warp][a], controls[warp][root.dimension]);
        anchors[warp][a] =
            numeric::rn_add(numeric::rn_mul(x.lower, .5), numeric::rn_mul(x.upper, .5));
      }
    }
  }
  __syncwarp();
  double maximum = 0.;
  if (ready[warp])
    for (uint32_t k = lane; k <= root.degree + 1; k += 32) {
      Interval xyz[3]{};
      for (uint32_t a = 0; a < root.dimension; ++a)
        xyz[a] = numeric::curve_residual_coefficient_rn64(root, controls[warp], predicates[warp],
                                                          anchors[warp], {k, a});
      maximum = numeric::maximum(maximum, numeric::norm_upper(xyz));
    }
  for (unsigned offset = 16; offset; offset /= 2)
    maximum = numeric::maximum(maximum, __shfl_down_sync(0xffffffffU, maximum, offset));
  if (!lane) {
    const auto bound = ready[warp] ? numeric::bound_curve_node_rn64(root, controls[warp],
                                                                    predicates[warp], maximum)
                                   : CurveNodeBounds{};
    output[i] = curve_decide(node, bound);
  }
}
__global__ void curve_mark(const CurveDeviceNodeAnswer *answers, uint32_t count, uint32_t *marks) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < count)
    marks[i] = answers[i].action == 1;
}
__global__ void curve_compact(CurveDeviceNodeAnswer *answers, const uint32_t *marks,
                              const uint32_t *prefix, uint32_t count, uint32_t *next) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < count) {
    answers[i].split_prefix = prefix[i];
    if (marks[i]) {
      next[2 * prefix[i]] = i;
      next[2 * prefix[i] + 1] = i;
    }
  }
}
__global__ void curve_trace_kernel(const AffineTraceBoundInput *tasks, uint32_t count,
                                   const Interval *controls, CurveNodeBounds *output) {
  const auto i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < count) {
    const auto &task = tasks[i];
    output[i] = numeric::bound_affine_trace_rn64(
        task, task.input.controls.count ? controls + task.input.controls.first : nullptr);
  }
}
void curve_mode(DeviceSession &s) { require(s.curve_mode, "curve operation on surface session"); }
void curve_shape(const CurveRootDescriptor &root, uint32_t nc) {
  require(static_cast<uint32_t>(root.family) <= static_cast<uint32_t>(CurveFamily::rational) &&
              root.dimension >= 2 && root.dimension <= 3 && root.degree <= 32 &&
              root.line_profile <= 1 && root.line_axis < root.dimension,
          "invalid curve family/dimension/degree/profile");
  require(root.controls.first <= nc && root.controls.count <= nc - root.controls.first,
          "curve source control range");
  require(root.controls.count ==
              (root.family == CurveFamily::rational ? (root.degree + 1) * (root.dimension + 1) : 0),
          "curve source control shape");
  for (uint32_t i = 0; i < 2; ++i)
    require(numeric::valid(root.original_t[i]) && numeric::valid(root.prepared_t[i]),
            "curve source parameter range");
  require(root.original_t[0].lower == root.original_t[0].upper &&
              root.original_t[1].lower == root.original_t[1].upper &&
              root.original_t[0].lower < root.original_t[1].upper &&
              root.prepared_t[0].lower <= root.prepared_t[1].upper &&
              root.prepared_t[0].lower >= root.original_t[0].lower &&
              root.prepared_t[1].upper <= root.original_t[1].upper,
          "curve prepared range outside original source");
  for (auto x : root.frame)
    require(numeric::finite(x), "nonfinite curve frame");
  for (auto x : root.radii)
    require(numeric::finite(x), "nonfinite curve radius");
  require(numeric::valid(root.second_derivative_upper) && root.second_derivative_upper.lower >= 0,
          "invalid curve derivative bound");
}
int32_t create_curve(const DeviceConfig *config, const CurveRootDescriptor *prepared, uint32_t nr,
                     const CurveRootDescriptor *original, uint32_t no, const Interval *coefficients,
                     uint32_t nc, DeviceSession **output, DeviceError *error) {
  if (output)
    *output = nullptr;
  return call(nullptr, error, [&] {
    require(config && output && prepared && original && nr && no, "invalid curve create inputs");
    require(nc <= config->controls && (!nc || coefficients), "curve control capacity");
    for (uint32_t i = 0; i < nr; ++i) {
      curve_shape(prepared[i], nc);
      require(prepared[i].original.value < no, "unknown original curve root");
    }
    for (uint32_t i = 0; i < no; ++i)
      curve_shape(original[i], nc);
    for (uint32_t i = 0; i < nc; ++i)
      require(numeric::valid(coefficients[i]), "invalid source curve coefficient");
    auto &s = create_owner(config, output, true);
    s.root_count = nr;
    s.original_count = no;
    const auto prepared_bytes = product(nr, sizeof(CurveRootDescriptor));
    const auto original_bytes = product(no, sizeof(CurveRootDescriptor));
    grow(s, host_roots, prepared_bytes, true);
    grow(s, host_originals, original_bytes, true);
    std::memcpy(s.buffers[host_roots].pointer, prepared, static_cast<size_t>(prepared_bytes));
    std::memcpy(s.buffers[host_originals].pointer, original, static_cast<size_t>(original_bytes));
    grow(s, roots, prepared_bytes, false);
    grow(s, originals, original_bytes, false);
    checked(cudaMemcpy(s.buffers[roots].pointer, prepared, static_cast<size_t>(prepared_bytes),
                       cudaMemcpyHostToDevice));
    checked(cudaMemcpy(s.buffers[originals].pointer, original, static_cast<size_t>(original_bytes),
                       cudaMemcpyHostToDevice));
    s.counters.uploaded_bytes += prepared_bytes + original_bytes;
    if (nc) {
      grow(s, controls, product(nc, sizeof(Interval)), false);
      grow(s, host_control_valid, nc, true);
      std::memset(s.buffers[host_control_valid].pointer, 1, nc);
      checked(cudaMemcpy(s.buffers[controls].pointer, coefficients,
                         static_cast<size_t>(product(nc, sizeof(Interval))),
                         cudaMemcpyHostToDevice));
      s.counters.uploaded_bytes += product(nc, sizeof(Interval));
    }
    s.broken = false;
  });
}
int32_t curve_samples(DeviceSession *s, const CurveDeviceSample *tasks, uint32_t count,
                      uint64_t *ticket, DeviceError *error) {
  return call(s, error, [&] {
    require(s && tasks && ticket && count && count <= s->config.frontier,
            "invalid curve sample operation");
    curve_mode(*s);
    idle(*s);
    charge(*s, product(count, 8192));
    grow(*s, validation, product(count, sizeof(Span)), true);
    uint32_t end = 0;
    for (uint32_t i = 0; i < count; ++i) {
      const auto &task = tasks[i];
      require(task.destination < s->config.points &&
                  task.input.original.value < s->original_count && task.supplied <= 1 &&
                  task.input.fixed_actual <= 1,
              "invalid curve sample destination/override");
      const auto &root =
          data<CurveRootDescriptor>(s->buffers[host_originals])[task.input.original.value];
      require(numeric::valid(task.input.t) && task.input.t.lower >= root.original_t[0].lower &&
                  task.input.t.upper <= root.original_t[1].upper,
              "curve sample outside original root");
      if (!task.supplied)
        require(task.input.stage == ArithmeticStage::rn64, "unavailable device sample tier");
      for (auto x : task.input.actual_xyz)
        require(numeric::finite(x), "nonfinite fixed curve point");
      if (task.supplied) {
        require(task.supplied_result.status == NumericStatus::ready &&
                    numeric::finite(task.supplied_result.departure_upper) &&
                    task.supplied_result.departure_upper >= 0,
                "invalid demanded-tier curve sample");
        for (uint32_t a = 0; a < 3; ++a)
          require(numeric::valid(task.supplied_result.original_xyz[a]) &&
                      numeric::finite(task.supplied_result.actual_xyz[a]),
                  "invalid demanded-tier curve coordinate");
      }
      data<Span>(s->buffers[validation])[i] = {task.destination, task.destination + 1};
      end = std::max(end, task.destination + 1);
    }
    unique_slots(*s, count);
    grow(*s, points, product(end, sizeof(CurveSampleResult)), false, true);
    grow(*s, host_point_valid, end, true, true);
    output_capacity(*s, count, sizeof(CurveSampleResult));
    upload(*s, tasks, product(count, sizeof(CurveDeviceSample)));
    curve_analytic_samples<<<blocks(count), tile, 0, s->stream>>>(
        data<CurveRootDescriptor>(s->buffers[originals]), data<Interval>(s->buffers[controls]),
        data<CurveDeviceSample>(s->buffers[input]), count,
        data<CurveSampleResult>(s->buffers[points]), data<CurveSampleResult>(s->buffers[result]));
    curve_rational_samples<<<blocks(count, warps), warp_threads, 0, s->stream>>>(
        data<CurveRootDescriptor>(s->buffers[originals]), data<Interval>(s->buffers[controls]),
        data<CurveDeviceSample>(s->buffers[input]), count,
        data<CurveSampleResult>(s->buffers[points]), data<CurveSampleResult>(s->buffers[result]));
    for (uint32_t i = 0; i < count; ++i)
      data<uint8_t>(s->buffers[host_point_valid])[tasks[i].destination] = 1;
    finish(*s, Operation::curve_samples, count, sizeof(CurveSampleResult), ticket);
  });
}
int32_t curve_install_controls(DeviceSession *s, uint32_t destination, const Interval *source,
                               uint32_t count, uint64_t *ticket, DeviceError *error) {
  return call(s, error, [&] {
    require(s && source && ticket && count && destination <= s->config.controls &&
                count <= s->config.controls - destination,
            "invalid curve control installation");
    curve_mode(*s);
    idle(*s);
    charge(*s, count);
    for (uint32_t i = 0; i < count; ++i) {
      require(numeric::valid(source[i]), "invalid installed curve interval");
      require(uint64_t{destination} + i >= s->buffers[host_control_valid].bytes ||
                  !data<uint8_t>(s->buffers[host_control_valid])[destination + i],
              "curve control installation overwrites retained net");
    }
    const auto end = destination + count;
    grow(*s, controls, product(end, sizeof(Interval)), false, true);
    grow(*s, host_control_valid, end, true, true);
    grow(*s, host_input, product(count, sizeof(Interval)), true);
    std::memcpy(s->buffers[host_input].pointer, source,
                static_cast<size_t>(product(count, sizeof(Interval))));
    checked(cudaMemcpyAsync(
        data<Interval>(s->buffers[controls]) + destination, s->buffers[host_input].pointer,
        static_cast<size_t>(product(count, sizeof(Interval))), cudaMemcpyHostToDevice, s->stream));
    s->counters.uploaded_bytes += product(count, sizeof(Interval));
    std::memset(data<uint8_t>(s->buffers[host_control_valid]) + destination, 1, count);
    finish(*s, Operation::curve_restrictions, count, 0, ticket);
  });
}
int32_t curve_pairs(DeviceSession *s, const CurveDeviceRestriction *tasks, uint32_t count,
                    uint64_t *ticket, DeviceError *error) {
  return call(s, error, [&] {
    require(s && tasks && ticket && count && count <= s->config.frontier && count <= UINT32_MAX / 2,
            "invalid paired curve restriction");
    curve_mode(*s);
    idle(*s);
    charge(*s, product(count, 8192));
    grow(*s, validation, product(uint64_t{count} * 2, sizeof(Span)), true);
    uint32_t end = 0;
    for (uint32_t i = 0; i < count; ++i) {
      const auto &task = tasks[i];
      require(task.root.value < s->root_count, "unknown paired curve root");
      const auto &root = data<CurveRootDescriptor>(s->buffers[host_roots])[task.root.value];
      require(root.family == CurveFamily::rational && task.parent.count == root.controls.count,
              "paired curve net shape");
      control_range(*s, task.parent);
      for (uint32_t child = 0; child < 2; ++child) {
        const auto first = child ? task.right_destination : task.left_destination;
        require(first <= s->config.controls && task.parent.count <= s->config.controls - first,
                "paired curve destination capacity");
        data<Span>(s->buffers[validation])[2 * i + child] = {first, first + task.parent.count};
        end = std::max(end, first + task.parent.count);
      }
    }
    unique_slots(*s, count * 2);
    const auto *destinations = data<Span>(s->buffers[validation]);
    for (uint32_t i = 0; i < count; ++i) {
      const auto parent = tasks[i].parent;
      const auto *next =
          std::lower_bound(destinations, destinations + count * 2, parent.first,
                           [](const Span &range, uint32_t at) { return range.end <= at; });
      require(next == destinations + count * 2 || next->begin >= parent.first + parent.count,
              "paired curve destination overlaps batch parent read");
    }
    // Child payload is immutable after installation, including source nets.
    for (uint32_t i = 0; i < count * 2; ++i)
      for (uint32_t j = destinations[i].begin; j < destinations[i].end; ++j)
        require(j >= s->buffers[host_control_valid].bytes ||
                    !data<uint8_t>(s->buffers[host_control_valid])[j],
                "paired curve destination overwrites retained net");
    grow(*s, controls, product(end, sizeof(Interval)), false, true);
    grow(*s, host_control_valid, end, true, true);
    upload(*s, tasks, product(count, sizeof(CurveDeviceRestriction)));
    curve_pair_kernel<<<blocks(count, warps), warp_threads, 0, s->stream>>>(
        data<CurveRootDescriptor>(s->buffers[roots]), data<Interval>(s->buffers[controls]),
        data<CurveDeviceRestriction>(s->buffers[input]), count);
    for (uint32_t i = 0; i < count; ++i)
      for (auto at : {tasks[i].left_destination, tasks[i].right_destination})
        std::memset(data<uint8_t>(s->buffers[host_control_valid]) + at, 1, tasks[i].parent.count);
    finish(*s, Operation::curve_restrictions, count, 0, ticket);
  });
}
int32_t curve_nodes(DeviceSession *s, const CurveDeviceNode *tasks, uint32_t count,
                    uint64_t *ticket, DeviceError *error) {
  return call(s, error, [&] {
    require(s && tasks && ticket && count && count <= s->config.frontier,
            "invalid curve node operation");
    curve_mode(*s);
    idle(*s);
    charge(*s, product(count, 8192));
    grow(*s, validation, product(count, sizeof(Span)), true);
    uint32_t end = 0;
    for (uint32_t i = 0; i < count; ++i) {
      const auto &task = tasks[i];
      require(task.slot < s->config.nodes && task.input.root.value < s->root_count &&
                  task.cached <= 1 && task.allow_image <= 1 &&
                  numeric::finite(task.tolerance_down) && task.tolerance_down >= 0,
              "invalid curve node slot/tolerance/policy");
      const auto &root = data<CurveRootDescriptor>(s->buffers[host_roots])[task.input.root.value];
      require(numeric::valid(task.input.t[0]) && numeric::valid(task.input.t[1]) &&
                  task.input.t[0].lower >= root.prepared_t[0].lower &&
                  task.input.t[1].upper <= root.prepared_t[1].upper &&
                  task.input.t[0].lower <= task.input.t[1].upper,
              "curve node outside prepared interval");
      if (task.cached) {
        const auto &bound = task.cached_bounds;
        require(bound.status == NumericStatus::ready && numeric::finite(bound.parameter_upper) &&
                    bound.parameter_upper >= 0 && numeric::finite(bound.numerical_upper) &&
                    bound.numerical_upper >= 0 && bound.image_available <= 1 &&
                    (!bound.image_available ||
                     (root.line_profile &&
                      bound.correspondence == CurveCorrespondence::monotone_line_image &&
                      numeric::finite(bound.image_upper) && bound.image_upper >= 0)),
                "invalid cached curve bound");
      } else {
        require(task.input.stage == ArithmeticStage::rn64, "unavailable curve node tier");
        for (uint32_t j = 0; j < 2; ++j)
          require(task.points[j] < s->buffers[host_point_valid].bytes &&
                      data<uint8_t>(s->buffers[host_point_valid])[task.points[j]],
                  "curve node point not resident");
        if (root.family == CurveFamily::rational) {
          require(task.input.controls.count == root.controls.count, "curve node net shape");
          control_range(*s, task.input.controls);
        }
      }
      data<Span>(s->buffers[validation])[i] = {task.slot, task.slot + 1};
      end = std::max(end, task.slot + 1);
    }
    unique_slots(*s, count);
    grow(*s, nodes, product(end, sizeof(CurveDeviceNode)), false, true);
    grow(*s, frontier, product(count, sizeof(uint32_t)), false);
    grow(*s, next_frontier, product(uint64_t{count} * 2, sizeof(uint32_t)), false);
    grow(*s, marks, product(count, sizeof(uint32_t)), false);
    grow(*s, prefix, product(count, sizeof(uint32_t)), false);
    const auto scratch = product(uint64_t{count} + 16, sizeof(uint32_t));
    grow(*s, sums, scratch, false);
    grow(*s, sum_prefix, scratch, false);
    output_capacity(*s, count, sizeof(CurveDeviceNodeAnswer));
    upload(*s, tasks, product(count, sizeof(CurveDeviceNode)));
    curve_install_nodes<<<blocks(count), tile, 0, s->stream>>>(
        data<CurveDeviceNode>(s->buffers[input]), count, data<CurveDeviceNode>(s->buffers[nodes]),
        data<uint32_t>(s->buffers[frontier]));
    curve_analytic_nodes<<<blocks(count), tile, 0, s->stream>>>(
        data<CurveRootDescriptor>(s->buffers[roots]), data<CurveDeviceNode>(s->buffers[nodes]),
        data<uint32_t>(s->buffers[frontier]), count, data<CurveSampleResult>(s->buffers[points]),
        data<CurveDeviceNodeAnswer>(s->buffers[result]));
    curve_rational_nodes<<<blocks(count, warps), warp_threads, 0, s->stream>>>(
        data<CurveRootDescriptor>(s->buffers[roots]), data<Interval>(s->buffers[controls]),
        data<CurveDeviceNode>(s->buffers[nodes]), data<uint32_t>(s->buffers[frontier]), count,
        data<CurveSampleResult>(s->buffers[points]),
        data<CurveDeviceNodeAnswer>(s->buffers[result]));
    curve_mark<<<blocks(count), tile, 0, s->stream>>>(
        data<CurveDeviceNodeAnswer>(s->buffers[result]), count, data<uint32_t>(s->buffers[marks]));
    scan(*s, data<uint32_t>(s->buffers[marks]), data<uint32_t>(s->buffers[prefix]), count);
    curve_compact<<<blocks(count), tile, 0, s->stream>>>(
        data<CurveDeviceNodeAnswer>(s->buffers[result]), data<uint32_t>(s->buffers[marks]),
        data<uint32_t>(s->buffers[prefix]), count, data<uint32_t>(s->buffers[next_frontier]));
    finish(*s, Operation::curve_nodes, count, sizeof(CurveDeviceNodeAnswer), ticket);
  });
}
int32_t curve_trace_bounds(DeviceSession *s, const AffineTraceBoundInput *tasks, uint32_t count,
                           const Interval *net, uint32_t nc, uint64_t *ticket, DeviceError *error) {
  return call(s, error, [&] {
    require(s && tasks && ticket && count && count <= s->config.frontier &&
                nc <= s->config.controls && (!nc || net),
            "invalid curve trace operation");
    curve_mode(*s);
    idle(*s);
    charge(*s, product(count, 8192));
    for (uint32_t i = 0; i < nc; ++i)
      require(numeric::valid(net[i]), "invalid trace control interval");
    for (uint32_t i = 0; i < count; ++i) {
      const auto &task = tasks[i];
      require(task.kind == AffineTraceBoundKind::rational ||
                  task.kind == AffineTraceBoundKind::direct_derivative,
              "invalid trace bound kind");
      for (auto x : task.input.t)
        require(numeric::valid(x), "invalid trace interval");
      require(task.input.t[0].lower <= task.input.t[1].upper, "reversed trace interval");
      for (auto x : task.input.endpoint_departures)
        require(numeric::finite(x) && x >= 0, "invalid trace endpoint departure");
      for (auto x : task.affine_endpoints)
        require(numeric::valid(x), "invalid SAME affine trace endpoint");
      if (task.kind == AffineTraceBoundKind::rational) {
        require(task.rational_curve.family == CurveFamily::rational &&
                    task.rational_curve.degree <= 32 && task.rational_curve.dimension >= 2 &&
                    task.rational_curve.dimension <= 3 &&
                    task.input.controls.count ==
                        (task.rational_curve.degree + 1) * (task.rational_curve.dimension + 1) &&
                    task.input.controls.first <= nc &&
                    task.input.controls.count <= nc - task.input.controls.first,
                "invalid rational trace net shape/range");
      } else
        require(numeric::finite(task.second_derivative_upper) && task.second_derivative_upper >= 0,
                "invalid original trace curvature");
    }
    const auto task_bytes = product(count, sizeof(AffineTraceBoundInput));
    const auto offset =
        (task_bytes + alignof(Interval) - 1) / alignof(Interval) * alignof(Interval);
    const auto net_bytes = product(nc, sizeof(Interval));
    require(offset <= UINT64_MAX - net_bytes, "trace staging size overflow");
    grow(*s, host_input, offset + net_bytes, true);
    grow(*s, input, offset + net_bytes, false);
    std::memcpy(s->buffers[host_input].pointer, tasks, static_cast<size_t>(task_bytes));
    auto *host_bytes = data<unsigned char>(s->buffers[host_input]);
    if (nc)
      std::memcpy(host_bytes + offset, net, static_cast<size_t>(net_bytes));
    output_capacity(*s, count, sizeof(CurveNodeBounds));
    checked(cudaMemcpyAsync(s->buffers[input].pointer, host_bytes,
                            static_cast<size_t>(offset + net_bytes), cudaMemcpyHostToDevice,
                            s->stream));
    s->counters.uploaded_bytes += offset + net_bytes;
    auto *device_bytes = data<unsigned char>(s->buffers[input]);
    curve_trace_kernel<<<blocks(count), tile, 0, s->stream>>>(
        reinterpret_cast<const AffineTraceBoundInput *>(device_bytes), count,
        reinterpret_cast<const Interval *>(device_bytes + offset),
        data<CurveNodeBounds>(s->buffers[result]));
    finish(*s, Operation::curve_traces, count, sizeof(CurveNodeBounds), ticket);
  });
}
const CurveCudaAPI curve_api{
    curve_cuda_abi, sizeof(CurveCudaAPI), create_curve,       curve_samples, curve_install_controls,
    curve_pairs,    curve_nodes,          curve_trace_bounds, poll,          stats,
    destroy};
