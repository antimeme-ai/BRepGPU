#ifndef BREPGPU_MESHER_SNAPSHOT_DIGEST_HPP
#define BREPGPU_MESHER_SNAPSHOT_DIGEST_HPP
#include "values.hpp"
#include <span>
namespace bgmesh {
[[nodiscard]] SourceDigest snapshot_digest(std::span<const std::byte>);
}
#endif
