#include "assembly.hpp"
#include <algorithm>
#include <limits>

namespace bgmesh {
namespace {
using Matrix = std::array<Rational, 12>;
Rational magnitude(const Rational &value) { return value.sign() < 0 ? value.negative() : value; }
Matrix identity(bgfd::Budget &budget) {
  const Rational z(budget, 0), o(budget, 1);
  return {o, z, z, z, z, o, z, z, z, z, o, z};
}
Matrix compose(const Matrix &a, const Frame &b) {
  auto &budget = a[0].budget();
  auto out = identity(budget);
  for (std::size_t row = 0; row < 3; ++row)
    for (std::size_t col = 0; col < 4; ++col) {
      Rational value(budget, 0);
      for (std::size_t k = 0; k < 3; ++k)
        value = value.add(a[row * 4 + k].multiply(Rational::from_double(budget, b[k * 4 + col])));
      if (col == 3)
        value = value.add(a[row * 4 + 3]);
      out[row * 4 + col] = std::move(value);
    }
  return out;
}
Rational floor_grid(const Rational &value, uint32_t bits) {
  if (value.sign() <= 0)
    throw Error(ErrorCode::unresolved, "nonpositive effective definition allowance");
  const auto numerator = value.numerator().shift_left(bits);
  auto integer = bgfd::Integer::divmod_nonnegative(numerator, value.denominator()).first;
  if (integer.sign() == 0)
    throw Error(ErrorCode::unresolved, "definition allowance falls below requested precision grid");
  return Rational::from_ratio(std::move(integer),
                              Rational(value.budget(), 1).numerator().shift_left(bits));
}
struct Transport final {
  Rational norm, storage_error;
};
Transport transport(const Matrix &a, const Frame &stored, const DefinitionEnvelopeView &box,
                    const Rational &epsilon, uint32_t bits) {
  auto &budget = epsilon.budget();
  Rational maximum_row(budget, 1);
  for (std::size_t i = 0; i < 3; ++i) {
    Rational row_sum(budget, 0);
    for (std::size_t j = 0; j < 3; ++j) {
      Rational g(budget, 0);
      for (std::size_t k = 0; k < 3; ++k)
        g = g.add(a[k * 4 + i].multiply(a[k * 4 + j]));
      row_sum = row_sum.add(magnitude(g));
    }
    if (row_sum.compare(maximum_row) > 0)
      maximum_row = std::move(row_sum);
  }
  Rational error_squared(budget, 0);
  for (std::size_t i = 0; i < 3; ++i) {
    auto axis_error =
        magnitude(a[i * 4 + 3].subtract(Rational::from_double(budget, stored[i * 4 + 3])));
    for (std::size_t j = 0; j < 3; ++j) {
      const auto lo = magnitude(box.xyz[2 * j].subtract(epsilon));
      const auto hi = magnitude(box.xyz[2 * j + 1].add(epsilon));
      const auto extent = lo.compare(hi) > 0 ? lo : hi;
      const auto difference =
          magnitude(a[i * 4 + j].subtract(Rational::from_double(budget, stored[i * 4 + j])));
      axis_error = axis_error.add(difference.multiply(extent));
    }
    error_squared = error_squared.add(axis_error.multiply(axis_error));
  }
  return {maximum_row.square_root_bounds(bits).upper, error_squared.square_root_bounds(bits).upper};
}
struct Quality final {
  Rational positional, node;
};
struct PathFrame final {
  Slot<ProductTag> product;
  Matrix transform;
  StyleId style;
  std::size_t next = 0;
  bool emitted = false;
};
} // namespace
AssemblyData::AssemblyData(std::shared_ptr<RunResource> owner, SourceStore original,
                           ExactTable exact)
    : resource(std::move(owner)), source(std::move(original)), values(std::move(exact)),
      definitions(resource.get()), instances(resource.get()), paths(resource.get()) {}

AssemblyStore prepare_assembly(bgfd::Budget &budget, const SourceStore &source,
                               const std::shared_ptr<RunResource> &resource,
                               std::span<const DefinitionEnvelopeView> envelopes,
                               const RequestedQuality &quality, ExactTableKey key,
                               AssemblyPrecision precision, AssemblyLimits limits) {
  if (!resource || &quality.positional.budget() != &budget ||
      &quality.node_source.budget() != &budget)
    throw Error(ErrorCode::internal_error, "assembly preparation resource/owner mismatch");
  if (quality.positional.sign() <= 0 || quality.node_source.sign() <= 0 ||
      precision.norm_fractional_bits < 32 || precision.allowance_fractional_bits < 32 ||
      precision.norm_fractional_bits > 4096 || precision.allowance_fractional_bits > 4096)
    throw Error(ErrorCode::invalid_source, "invalid assembly quality/precision profile");
  const auto &original = source.view();
  if (envelopes.size() != original.definitions.size())
    throw Error(ErrorCode::invalid_source, "assembly needs every original definition envelope");
  std::pmr::vector<const DefinitionEnvelopeView *> boxes(original.definitions.size(), nullptr,
                                                         &budget);
  for (const auto &box : envelopes) {
    budget.charge(1);
    if (&box.source.view() != &original || box.definition.value >= boxes.size() ||
        boxes[box.definition.value] != nullptr)
      throw Error(ErrorCode::invalid_source, "foreign or duplicate definition envelope");
    for (std::size_t axis = 0; axis < 3; ++axis) {
      if (&box.xyz[axis * 2].budget() != &budget || &box.xyz[axis * 2 + 1].budget() != &budget)
        throw Error(ErrorCode::internal_error, "definition envelope arithmetic owner mismatch");
      if (box.xyz[axis * 2].compare(box.xyz[axis * 2 + 1]) > 0)
        throw Error(ErrorCode::invalid_source, "reversed original definition envelope");
    }
    boxes[box.definition.value] = &box;
  }
  // One parent/usage index, ordered by original IDs. Traversal never rescans all
  // source usages at each expanded path.
  std::pmr::vector<uint32_t> children(&budget);
  children.reserve(original.usages.size());
  for (uint32_t i = 0; i < original.usages.size(); ++i)
    children.push_back(i);
  std::sort(children.begin(), children.end(), [&](uint32_t a, uint32_t b) {
    budget.charge(1);
    const auto &x = original.usages[a], &y = original.usages[b];
    return x.parent_product_id < y.parent_product_id ||
           (x.parent_product_id == y.parent_product_id && x.id < y.id);
  });
  std::pmr::vector<Range<uint32_t>> adjacency(original.products.size(), &budget);
  for (std::size_t i = 0; i < children.size();) {
    const auto first = i;
    const auto parent = original.usages[children[i]].parent_product_id;
    while (i < children.size() && original.usages[children[i]].parent_product_id == parent) {
      budget.charge(1);
      ++i;
    }
    adjacency[original.product_index.find(parent).value] = {first, i - first};
  }
  std::pmr::vector<Quality> allowances(&budget);
  allowances.reserve(boxes.size());
  for (std::size_t i = 0; i < boxes.size(); ++i)
    allowances.push_back({quality.positional, quality.node_source});
  std::pmr::vector<NativeInstance> instances(resource.get());
  std::pmr::vector<UsageId> paths(resource.get()), path(&budget);
  std::pmr::vector<PathFrame> stack(&budget);
  const auto root = original.product_index.find(original.root_product_id);
  stack.push_back({root, identity(budget), original.products[root.value].style_id});
  while (!stack.empty()) {
    budget.charge(1);
    auto &frame = stack.back();
    const auto &product = original.products[frame.product.value];
    if (!frame.emitted) {
      frame.emitted = true;
      if (product.definition_id) {
        if (instances.size() >= limits.instances)
          throw Error(ErrorCode::resource_exhausted, "expanded source instance count exhausted");
        const auto definition = original.definition_index.find(product.definition_id);
        Frame stored{};
        for (std::size_t i = 0; i < stored.size(); ++i)
          stored[i] = frame.transform[i].to_double();
        const auto bound = transport(frame.transform, stored, *boxes[definition.value],
                                     quality.positional, precision.norm_fractional_bits);
        auto local = floor_grid(quality.positional.subtract(bound.storage_error).divide(bound.norm),
                                precision.allowance_fractional_bits);
        auto node =
            floor_grid(quality.node_source.divide(bound.norm), precision.allowance_fractional_bits);
        auto &selected = allowances[definition.value];
        if (local.compare(selected.positional) < 0)
          selected.positional = std::move(local);
        if (node.compare(selected.node) < 0)
          selected.node = std::move(node);
        const Range<UsageId> stored_path{paths.size(), path.size()};
        paths.insert(paths.end(), path.begin(), path.end());
        instances.push_back({product.id, definition, stored_path, stored, frame.style});
      }
    }
    const auto children_range = adjacency[frame.product.value];
    if (frame.next == children_range.count) {
      stack.pop_back();
      if (!stack.empty())
        path.pop_back();
      continue;
    }
    const auto usage_slot = children[static_cast<std::size_t>(children_range.first) + frame.next++];
    const auto &usage = original.usages[usage_slot];
    const auto child = original.product_index.find(usage.child_product_id);
    auto transform = compose(frame.transform, usage.transform);
    const auto style = usage.style_id ? usage.style_id
                       : original.products[child.value].style_id
                           ? original.products[child.value].style_id
                           : frame.style;
    path.push_back(usage.id);
    if (path.size() > original.products.size())
      throw Error(ErrorCode::internal_error, "source usage path exceeds admitted acyclic graph");
    stack.push_back({child, std::move(transform), style});
  }
  ExactBuilder values(resource, key);
  std::pmr::vector<DefinitionAllowance> definitions(resource.get());
  definitions.reserve(allowances.size());
  for (std::size_t i = 0; i < allowances.size(); ++i) {
    budget.charge(1);
    definitions.push_back(
        {Slot<DefinitionTag>{static_cast<uint32_t>(i)},
         values.append(floor_grid(allowances[i].positional, precision.allowance_fractional_bits)),
         values.append(floor_grid(allowances[i].node, precision.allowance_fractional_bits))});
  }
  auto data = std::allocate_shared<AssemblyData>(RunAllocator<AssemblyData>(resource), resource,
                                                 source, values.seal());
  data->definitions = std::move(definitions);
  data->instances = std::move(instances);
  data->paths = std::move(paths);
  return AssemblyStore(std::move(data));
}
} // namespace bgmesh
