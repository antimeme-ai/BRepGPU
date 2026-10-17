#include "exact_store.hpp"
#include <algorithm>
#include <limits>

namespace bgmesh {
namespace {
template <class T> void reserve_append(std::pmr::vector<T> &pool, std::size_t extra) {
  if (extra > pool.max_size() - pool.size())
    throw Error(ErrorCode::resource_exhausted, "immutable exact table size overflow");
  const auto required = pool.size() + extra;
  if (required > pool.capacity()) {
    const auto capacity = pool.capacity();
    const auto grown = capacity > pool.max_size() / 2 ? pool.max_size() : 2 * capacity;
    pool.reserve(std::max(required, grown));
  }
}
std::span<const uint32_t> words(const ExactTableData &data, Range<uint32_t> range) {
  if (range.first > data.limbs.size() || range.count > data.limbs.size() - range.first)
    throw Error(ErrorCode::internal_error, "immutable exact limb range out of bounds");
  return std::span<const uint32_t>(data.limbs)
      .subspan(static_cast<std::size_t>(range.first), static_cast<std::size_t>(range.count));
}
} // namespace
ExactBuilder::ExactBuilder(const std::shared_ptr<RunResource> &resource, ExactTableKey key) {
  if (!resource)
    throw Error(ErrorCode::internal_error, "exact table requires an owned run resource");
  data_ =
      std::allocate_shared<ExactTableData>(RunAllocator<ExactTableData>(resource), resource, key);
}
ExactRef ExactBuilder::append(const Rational &value) {
  if (!data_)
    throw Error(ErrorCode::internal_error, "cannot append to sealed exact table");
  const auto n = value.numerator().magnitude(), d = value.denominator().magnitude();
  if (n.size() > std::numeric_limits<std::size_t>::max() - d.size())
    throw Error(ErrorCode::resource_exhausted, "exact entry size overflow");
  value.budget().charge(static_cast<uint64_t>(n.size() + d.size()));
  // Both reserves precede mutation. Trivial inserts below cannot allocate or
  // throw, so a failed reservation preserves every previous readable entry.
  reserve_append(data_->entries, 1);
  reserve_append(data_->limbs, n.size() + d.size());
  const ExactEntry entry{
      value.sign(), {data_->limbs.size(), n.size()}, {data_->limbs.size() + n.size(), d.size()}};
  const ExactRef result{data_->key, data_->entries.size()};
  data_->limbs.insert(data_->limbs.end(), n.begin(), n.end());
  data_->limbs.insert(data_->limbs.end(), d.begin(), d.end());
  data_->entries.push_back(entry);
  return result;
}
ExactTable ExactBuilder::seal() {
  if (!data_)
    throw Error(ErrorCode::internal_error, "exact table builder already sealed");
  return ExactTable(std::exchange(data_, {}));
}
ExactView ExactTable::view(ExactRef ref) const {
  if (ref.table != data_->key || ref.index >= data_->entries.size())
    throw Error(ErrorCode::internal_error, "foreign or missing exact table reference");
  const auto &entry = data_->entries[static_cast<std::size_t>(ref.index)];
  return {entry.sign, words(*data_, entry.numerator), words(*data_, entry.denominator)};
}
Rational import_exact(bgfd::Budget &budget, ExactView value) {
  return Rational::from_ratio(bgfd::Integer::from_words(budget, value.sign, value.numerator),
                              bgfd::Integer::from_words(budget, 1, value.denominator));
}
} // namespace bgmesh
