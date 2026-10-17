#include "exact_store.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
void check() {
  auto resource = std::make_shared<bgmesh::RunResource>(uint64_t{1} << 20);
  const bgmesh::ExactTableKey key{{bgmesh::OwnerKind::geometry, 2, 9}, 7, 0};
  bgmesh::ExactRef ref;
  auto table = [&] {
    bgfd::Budget source({uint64_t{1} << 20, uint64_t{1} << 24}, resource.get());
    bgmesh::ExactBuilder builder(resource, key);
    const auto rational = bgmesh::Rational(source, 3).divide(bgmesh::Rational(source, 7));
    ref = builder.append(rational);
    auto result = builder.seal();
    bool rejected = false;
    try {
      static_cast<void>(builder.append(rational));
    } catch (const bgmesh::Error &) {
      rejected = true;
    }
    require(rejected, "sealed table builder still mutable");
    return result;
  }();
  require(table.key() == key && ref.index == 0 && table.size() == 1,
          "wrong qualified exact reference");
  const auto words = table.view(ref);
  require(words.sign == 1 && words.numerator.size() == 1 && words.numerator[0] == 3 &&
              words.denominator.size() == 1 && words.denominator[0] == 7,
          "shared original exact words lost");
  bgfd::Budget destination({uint64_t{1} << 20, uint64_t{1} << 24}, resource.get());
  const auto imported = bgmesh::import_exact(destination, words);
  require(imported.compare(
              bgmesh::Rational(destination, 3).divide(bgmesh::Rational(destination, 7))) == 0,
          "explicit destination import changes rational");
  auto foreign = ref;
  foreign.table.generation++;
  bool rejected = false;
  try {
    static_cast<void>(table.view(foreign));
  } catch (const bgmesh::Error &) {
    rejected = true;
  }
  require(rejected, "unqualified/foreign exact table reference accepted");
  std::cout << "native immutable exact-table checks PASS\n";
}
} // namespace
int main() {
  try {
    check();
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  } catch (const bgfd::Failure &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
