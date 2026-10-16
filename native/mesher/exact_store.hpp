#ifndef BREPGPU_MESHER_EXACT_STORE_HPP
#define BREPGPU_MESHER_EXACT_STORE_HPP
#include "exact.hpp"
#include "resources.hpp"
#include <memory>
#include <span>
#include <vector>

namespace bgmesh {
enum class OwnerKind : uint32_t { source, geometry, boundary, face, contact, run };
struct OwnerKey final {
  OwnerKind kind = OwnerKind::run;
  uint32_t definition = 0;
  uint64_t id = 0;
  auto operator<=>(const OwnerKey &) const = default;
};
struct ExactTableKey final {
  OwnerKey owner;
  uint64_t generation = 0;
  uint32_t ordinal = 0;
  auto operator<=>(const ExactTableKey &) const = default;
};
struct ExactRef final {
  ExactTableKey table;
  uint64_t index = 0;
  auto operator<=>(const ExactRef &) const = default;
};
// Borrowed spans: retain the ExactTable (or an owning task lease) through use.
struct ExactView final {
  int sign;
  std::span<const uint32_t> numerator, denominator;
};
struct ExactEntry final {
  int sign;
  Range<uint32_t> numerator, denominator;
};
struct ExactTableData final {
  std::shared_ptr<RunResource> resource;
  ExactTableKey key;
  std::pmr::vector<ExactEntry> entries;
  std::pmr::vector<uint32_t> limbs;
  ExactTableData(std::shared_ptr<RunResource> owner, ExactTableKey identity)
      : resource(std::move(owner)), key(identity), entries(resource.get()), limbs(resource.get()) {}
  ExactTableData(const ExactTableData &) = delete;
  ExactTableData &operator=(const ExactTableData &) = delete;
};
class ExactBuilder;
class ExactTable final {
  std::shared_ptr<const ExactTableData> data_;
  explicit ExactTable(std::shared_ptr<const ExactTableData> data) : data_(std::move(data)) {}
  friend class ExactBuilder;

public:
  [[nodiscard]] ExactTableKey key() const noexcept { return data_->key; }
  [[nodiscard]] const std::shared_ptr<RunResource> &resource() const noexcept {
    return data_->resource;
  }
  [[nodiscard]] uint64_t size() const noexcept { return data_->entries.size(); }
  [[nodiscard]] ExactView view(ExactRef ref) const;
};
class ExactBuilder final {
  std::shared_ptr<ExactTableData> data_;

public:
  ExactBuilder(const std::shared_ptr<RunResource> &, ExactTableKey);
  ExactBuilder(const ExactBuilder &) = delete;
  ExactBuilder &operator=(const ExactBuilder &) = delete;
  [[nodiscard]] ExactRef append(const Rational &);
  [[nodiscard]] ExactTable seal();
};
[[nodiscard]] Rational import_exact(bgfd::Budget &, ExactView);
} // namespace bgmesh
#endif
