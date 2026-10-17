#ifndef BREPGPU_MESHER_ADAPTIVE_CALLER_IO_HPP
#define BREPGPU_MESHER_ADAPTIVE_CALLER_IO_HPP
#include "adaptive_curves.hpp"
#include "curve_source_cache.hpp"
#include <fstream>

namespace bgmesh::adaptive::caller {
class Wire final {
  std::ofstream out_;
  bgfd::Budget &budget_;

public:
  Wire(const std::filesystem::path &path, bgfd::Budget &budget)
      : out_(path, std::ios::binary), budget_(budget) {
    if (!out_)
      throw Error(ErrorCode::io_failure,
                  "cannot open shared-boundary artifact");
  }
  void bytes(std::span<const std::byte> bytes) {
    budget_.charge(bytes.size());
    out_.write(reinterpret_cast<const char *>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    if (!out_)
      throw Error(ErrorCode::io_failure,
                  "shared-boundary artifact write failed");
  }
  template <unsigned count> void word(uint64_t value) {
    static_assert(count == 4 || count == 8);
    std::array<std::byte, 8> bytes{};
    for (unsigned i = 0; i < count; ++i)
      bytes[i] = static_cast<std::byte>((value >> (8 * i)) & 255);
    this->bytes(std::span(bytes).first(count));
  }
  void rational(ExactView value) {
    if (value.numerator.size() > UINT32_MAX ||
        value.denominator.size() > UINT32_MAX)
      throw Error(ErrorCode::resource_exhausted,
                  "shared-boundary rational exceeds wire count range");
    word<4>(static_cast<uint32_t>(value.sign));
    word<4>(value.numerator.size());
    word<4>(value.denominator.size());
    for (auto w : value.numerator)
      word<4>(w);
    for (auto w : value.denominator)
      word<4>(w);
  }
  void finish() {
    out_.close();
    if (!out_)
      throw Error(ErrorCode::io_failure, "artifact write/close failed");
  }
};
void write_curves(const std::filesystem::path &,
                  const SharedCurveSnapshotData &, bgfd::Budget &);
void digest(std::ostream &, const SourceDigest &);
std::pmr::vector<CurveRequest>
definition_curve_requests(const SourceStore &, Slot<DefinitionTag>,
                          const std::shared_ptr<RunResource> &);
} // namespace bgmesh::adaptive::caller
#endif
