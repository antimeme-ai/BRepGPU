#include "snapshot_digest.hpp"
#include <bit>

namespace bgmesh {
// Rehome the existing owned step_source SHA-256 definition, streaming blocks
// rather than copying the complete source to append padding.
SourceDigest snapshot_digest(std::span<const std::byte> input) {
  constexpr std::array<uint32_t, 64> constants{
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
      0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
      0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
      0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
      0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
      0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
      0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
      0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
      0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
      0xc67178f2};
  if (input.size() > UINT64_MAX / 8)
    throw Error(ErrorCode::resource_exhausted, "source digest bit count overflow");
  std::array<uint32_t, 8> state{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
  const uint64_t blocks =
      static_cast<uint64_t>(input.size()) / 64 + 1 + static_cast<uint64_t>(input.size() % 64 >= 56);
  const auto bits = static_cast<uint64_t>(input.size()) * 8;
  for (uint64_t block = 0; block < blocks; ++block) {
    std::array<uint32_t, 64> w{};
    for (uint64_t i = 0; i < 64; ++i) {
      const auto offset = block * 64 + i;
      uint32_t value = 0;
      if (offset < input.size())
        value = std::to_integer<uint32_t>(input[static_cast<std::size_t>(offset)]);
      else if (offset == input.size())
        value = 128;
      else if (block == blocks - 1 && i >= 56)
        value = static_cast<uint32_t>((bits >> (8 * (63 - i))) & 255);
      w[static_cast<std::size_t>(i / 4)] |= value << (24 - 8 * (i % 4));
    }
    for (std::size_t i = 16; i < 64; ++i) {
      const auto a = w[i - 15], b = w[i - 2];
      w[i] = w[i - 16] + (std::rotr(a, 7) ^ std::rotr(a, 18) ^ (a >> 3)) + w[i - 7] +
             (std::rotr(b, 17) ^ std::rotr(b, 19) ^ (b >> 10));
    }
    auto a = state[0], b = state[1], c = state[2], d = state[3], e = state[4], f = state[5],
         g = state[6], h = state[7];
    for (std::size_t i = 0; i < 64; ++i) {
      const auto t = h + (std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25)) +
                     ((e & f) ^ ((~e) & g)) + constants[i] + w[i];
      const auto t2 =
          (std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
      h = g;
      g = f;
      f = e;
      e = d + t;
      d = c;
      c = b;
      b = a;
      a = t + t2;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
  }
  SourceDigest result{};
  for (std::size_t i = 0; i < 8; ++i)
    for (std::size_t j = 0; j < 4; ++j)
      result[4 * i + j] = static_cast<std::byte>((state[i] >> (24 - 8 * j)) & 255);
  return result;
}
} // namespace bgmesh
