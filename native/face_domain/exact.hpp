#ifndef BREPGPU_FACE_DOMAIN_EXACT_HPP
#define BREPGPU_FACE_DOMAIN_EXACT_HPP
#include "face_domain.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory_resource>
#include <span>
#include <stdexcept>
#include <stop_token>
#include <utility>
#include <vector>

namespace bgfd {
struct Failure final {
  bgfd_status status;
  const char *message;
  Failure(bgfd_status code, const char *text) noexcept : status(code), message(text) {}
  [[nodiscard]] const char *what() const noexcept { return message; }
};
struct Limits final {
  uint64_t bytes, work;
};
class Cancelled final : public std::exception {
public:
  [[nodiscard]] const char *what() const noexcept override { return "native exact work cancelled"; }
};
class Budget final : public std::pmr::memory_resource {
  uint64_t limit_, live_ = 0, peak_ = 0, work_limit_, work_ = 0, max_limbs_ = 0;
  uint64_t divisions_ = 0, quotient_corrections_ = 0, division_addbacks_ = 0, lehmer_batches_ = 0;
  std::pmr::memory_resource *upstream_;
  std::stop_token stop_;
  void *do_allocate(std::size_t bytes, std::size_t alignment) override {
    const auto size = static_cast<uint64_t>(bytes);
    if (size > limit_ - live_)
      throw Failure(BGFD_RESOURCE_EXHAUSTED, "native memory budget exhausted");
    void *value = upstream_->allocate(bytes, alignment);
    live_ += size;
    peak_ = std::max(peak_, live_);
    return value;
  }
  void do_deallocate(void *value, std::size_t bytes, std::size_t alignment) override {
    live_ -= static_cast<uint64_t>(bytes);
    upstream_->deallocate(value, bytes, alignment);
  }
  bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override {
    return this == &other;
  }

public:
  // Owner-affine nested work cap; no new allocator or arithmetic ownership.
  // Keep scopes in stack order and do not reset_work() inside a live scope.
  class WorkScope final {
    Budget &budget_;
    uint64_t previous_;
    friend class Budget;
    WorkScope(Budget &owner, uint64_t amount) : budget_(owner), previous_(owner.work_limit_) {
      owner.charge(0);
      if (amount < owner.work_limit_ - owner.work_)
        owner.work_limit_ = owner.work_ + amount;
    }

  public:
    WorkScope(const WorkScope &) = delete;
    WorkScope &operator=(const WorkScope &) = delete;
    ~WorkScope() { budget_.work_limit_ = previous_; }
  };
  [[nodiscard]] WorkScope scoped_work(uint64_t amount) { return WorkScope(*this, amount); }
  explicit Budget(Limits limits,
                  std::pmr::memory_resource *upstream = std::pmr::new_delete_resource(),
                  std::stop_token stop = {})
      : limit_(limits.bytes), work_limit_(limits.work), upstream_(upstream),
        stop_(std::move(stop)) {
    if (upstream == nullptr || upstream == this)
      throw Failure(BGFD_INVALID_INPUT, "invalid budget upstream");
  }
  Budget(const Budget &) = delete;
  Budget &operator=(const Budget &) = delete;
  void charge(uint64_t amount) {
    if (stop_.stop_requested())
      throw Cancelled();
    if (amount > work_limit_ - work_)
      throw Failure(BGFD_RESOURCE_EXHAUSTED, "exact work budget exhausted");
    work_ += amount;
  }
  void reset_work() noexcept { work_ = 0; }
  [[nodiscard]] uint64_t work() const noexcept { return work_; }
  [[nodiscard]] uint64_t peak() const noexcept { return peak_; }
  [[nodiscard]] uint64_t live() const noexcept { return live_; }
  void observe(std::size_t count) noexcept {
    max_limbs_ = std::max(max_limbs_, static_cast<uint64_t>(count));
  }
  [[nodiscard]] uint64_t max_limbs() const noexcept { return max_limbs_; }
  void observe_division() {
    charge(1);
    ++divisions_;
  }
  void observe_quotient_correction() {
    charge(1);
    ++quotient_corrections_;
  }
  void observe_division_addback() {
    charge(1);
    ++division_addbacks_;
  }
  void observe_lehmer_batch() {
    charge(1);
    ++lehmer_batches_;
  }
  [[nodiscard]] uint64_t divisions() const noexcept { return divisions_; }
  [[nodiscard]] uint64_t quotient_corrections() const noexcept { return quotient_corrections_; }
  [[nodiscard]] uint64_t division_addbacks() const noexcept { return division_addbacks_; }
  [[nodiscard]] uint64_t lehmer_batches() const noexcept { return lehmer_batches_; }
};

class Integer final {
  Budget *budget_;
  std::pmr::vector<uint32_t> limbs_;
  int sign_ = 0;
  void require_same_owner(const Integer &other) const {
    if (budget_ != other.budget_)
      throw Failure(BGFD_INTERNAL_ERROR, "integer arithmetic allocator mismatch");
  }
  void trim() noexcept {
    while (!limbs_.empty() && limbs_.back() == 0)
      limbs_.pop_back();
    if (limbs_.empty())
      sign_ = 0;
    budget_->observe(limbs_.size());
  }
  [[nodiscard]] static int magnitude_compare(const Integer &left, const Integer &right) {
    left.budget_->charge(1);
    if (left.limbs_.size() != right.limbs_.size())
      return left.limbs_.size() > right.limbs_.size() ? 1 : -1;
    for (std::size_t i = left.limbs_.size(); i > 0; --i) {
      left.budget_->charge(1);
      if (left.limbs_[i - 1] != right.limbs_[i - 1])
        return left.limbs_[i - 1] > right.limbs_[i - 1] ? 1 : -1;
    }
    return 0;
  }
  [[nodiscard]] static Integer magnitude_add(const Integer &left, const Integer &right, int sign) {
    Integer result(*left.budget_);
    const auto count = std::max(left.limbs_.size(), right.limbs_.size());
    left.budget_->charge(static_cast<uint64_t>(count));
    result.limbs_.resize(count + 1);
    uint64_t carry = 0;
    for (std::size_t i = 0; i < count; ++i) {
      const uint64_t a = i < left.limbs_.size() ? left.limbs_[i] : 0;
      const uint64_t b = i < right.limbs_.size() ? right.limbs_[i] : 0;
      const uint64_t sum = a + b + carry;
      result.limbs_[i] = static_cast<uint32_t>(sum);
      carry = sum >> 32;
    }
    result.limbs_[count] = static_cast<uint32_t>(carry);
    result.sign_ = sign;
    result.trim();
    return result;
  }
  [[nodiscard]] static Integer magnitude_sub(const Integer &left, const Integer &right, int sign) {
    Integer result(*left.budget_);
    left.budget_->charge(static_cast<uint64_t>(left.limbs_.size()));
    result.limbs_.resize(left.limbs_.size());
    uint64_t borrow = 0;
    for (std::size_t i = 0; i < left.limbs_.size(); ++i) {
      const uint64_t a = left.limbs_[i];
      const uint64_t b = (i < right.limbs_.size() ? right.limbs_[i] : 0) + borrow;
      result.limbs_[i] = static_cast<uint32_t>(a - b);
      borrow = a < b ? 1 : 0;
    }
    result.sign_ = sign;
    result.trim();
    return result;
  }

public:
  explicit Integer(Budget &budget) : budget_(&budget), limbs_(&budget) {}
  Integer(const Integer &other)
      : budget_(other.budget_), limbs_(other.budget_), sign_(other.sign_) {
    budget_->charge(static_cast<uint64_t>(other.limbs_.size()));
    limbs_ = other.limbs_;
  }
  Integer(Integer &&other) noexcept
      : budget_(other.budget_), limbs_(std::move(other.limbs_), other.budget_), sign_(other.sign_) {
    other.sign_ = 0;
  }
  Integer &operator=(const Integer &other) {
    if (this != &other) {
      if (budget_ != other.budget_)
        throw Failure(BGFD_INTERNAL_ERROR, "integer allocator mismatch");
      budget_->charge(static_cast<uint64_t>(other.limbs_.size()));
      limbs_ = other.limbs_;
      sign_ = other.sign_;
    }
    return *this;
  }
  Integer &operator=(Integer &&other) {
    if (this != &other) {
      if (budget_ != other.budget_)
        throw Failure(BGFD_INTERNAL_ERROR, "integer move allocator mismatch");
      limbs_ = std::move(other.limbs_);
      sign_ = other.sign_;
      other.sign_ = 0;
    }
    return *this;
  }
  [[nodiscard]] static Integer decode(Budget &budget, bgfd_integer descriptor,
                                      const uint32_t *limbs, uint64_t count) {
    if (descriptor.offset > count || descriptor.count > count - descriptor.offset ||
        descriptor.sign < -1 || descriptor.sign > 1 ||
        ((descriptor.count == 0) != (descriptor.sign == 0)))
      throw Failure(BGFD_INVALID_INPUT, "invalid exact integer descriptor");
    Integer result(budget);
    if (descriptor.count != 0) {
      if (limbs == nullptr || limbs[descriptor.offset + descriptor.count - 1] == 0)
        throw Failure(BGFD_INVALID_INPUT, "noncanonical exact integer");
      result.limbs_.assign(limbs + descriptor.offset, limbs + descriptor.offset + descriptor.count);
      budget.charge(descriptor.count);
    }
    result.sign_ = descriptor.sign;
    budget.observe(result.limbs_.size());
    return result;
  }
  [[nodiscard]] int sign() const noexcept { return sign_; }
  [[nodiscard]] Budget &budget() const noexcept { return *budget_; }
  [[nodiscard]] std::size_t size() const noexcept { return limbs_.size(); }
  [[nodiscard]] std::span<const uint32_t> magnitude() const noexcept { return limbs_; }
  [[nodiscard]] int compare(const Integer &other) const {
    require_same_owner(other);
    budget_->charge(1);
    if (sign_ != other.sign_)
      return sign_ > other.sign_ ? 1 : -1;
    return sign_ == 0 ? 0 : sign_ * magnitude_compare(*this, other);
  }
  [[nodiscard]] Integer negative() const {
    Integer result(*this);
    result.sign_ = -result.sign_;
    return result;
  }
  [[nodiscard]] Integer add(const Integer &other) const {
    require_same_owner(other);
    if (sign_ == 0)
      return Integer(other);
    if (other.sign_ == 0)
      return Integer(*this);
    if (sign_ == other.sign_)
      return magnitude_add(*this, other, sign_);
    const int comparison = magnitude_compare(*this, other);
    if (comparison == 0)
      return Integer(*budget_);
    return comparison > 0 ? magnitude_sub(*this, other, sign_)
                          : magnitude_sub(other, *this, other.sign_);
  }
  [[nodiscard]] Integer subtract(const Integer &other) const {
    require_same_owner(other);
    return add(other.negative());
  }
  [[nodiscard]] Integer multiply(const Integer &other) const {
    require_same_owner(other);
    Integer result(*budget_);
    if (sign_ == 0 || other.sign_ == 0)
      return result;
    const uint64_t a_count = static_cast<uint64_t>(limbs_.size());
    const uint64_t b_count = static_cast<uint64_t>(other.limbs_.size());
    if (b_count != 0 && a_count > std::numeric_limits<uint64_t>::max() / b_count)
      throw Failure(BGFD_RESOURCE_EXHAUSTED, "integer work size overflow");
    budget_->charge(a_count * b_count);
    if (other.limbs_.size() > std::numeric_limits<std::size_t>::max() - limbs_.size())
      throw Failure(BGFD_RESOURCE_EXHAUSTED, "integer storage size overflow");
    result.limbs_.resize(limbs_.size() + other.limbs_.size());
    for (std::size_t i = 0; i < limbs_.size(); ++i) {
      uint64_t carry = 0;
      for (std::size_t j = 0; j < other.limbs_.size(); ++j) {
        const uint64_t product =
            static_cast<uint64_t>(limbs_[i]) * other.limbs_[j] + result.limbs_[i + j] + carry;
        result.limbs_[i + j] = static_cast<uint32_t>(product);
        carry = product >> 32;
      }
      result.limbs_[i + other.limbs_.size()] = static_cast<uint32_t>(carry);
    }
    result.sign_ = sign_ * other.sign_;
    result.trim();
    return result;
  }
  [[nodiscard]] static Integer from_words(Budget &, int sign, std::span<const uint32_t>);
  [[nodiscard]] Integer clone_into(Budget &) const;
  [[nodiscard]] uint64_t bit_length() const;
  [[nodiscard]] Integer shift_left(uint64_t) const;
  [[nodiscard]] Integer shift_right_exact(uint64_t) const;
  [[nodiscard]] static std::pair<Integer, Integer> divmod_nonnegative(const Integer &,
                                                                      const Integer &);
  [[nodiscard]] static Integer gcd_nonnegative(const Integer &, const Integer &);
  [[nodiscard]] Integer divide_exact(const Integer &positive_divisor) const;
};
} // namespace bgfd
#endif
