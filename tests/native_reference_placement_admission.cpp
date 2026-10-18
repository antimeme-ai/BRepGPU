// One placement admission rule for the raw reference display: every matrix of
// tests/fixtures/rigid-placements.tsv is placed once on one triangle in an
// authored BGREFM01 file, and RawScene must admit it exactly when the corpus
// says "rigid". A refusal counts only with the placement message; any other
// decode failure is reported as such.
#include "../native/om10_reference/raw.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}
struct Writer {
  std::vector<char> out;
  void word(uint64_t value, unsigned width) {
    for (unsigned i = 0; i < width; ++i)
      out.push_back(static_cast<char>((value >> (8 * i)) & 255U));
  }
  void number(double value) {
    uint64_t bits;
    std::memcpy(&bits, &value, 8);
    word(bits, 8);
  }
};
std::string reference_status(const double *matrix, const std::string &path) {
  Writer w;
  for (char c : std::string("BGREFM01"))
    w.out.push_back(c);
  w.word(1, 4);
  w.word(0, 4);
  for (uint64_t count : {1, 1, 3, 1, 1})
    w.word(count, 8);
  w.word(0, 8);
  for (uint64_t value : {7, 0, 3, 0, 1}) // definition id, vertex/triangle ranges
    w.word(value, 8);
  w.word(9, 8); // occurrence id
  w.word(0, 4);
  w.word(0, 4);
  for (int i = 0; i < 12; ++i)
    w.number(matrix[i]);
  for (int i = 0; i < 3; ++i)
    w.number(0);
  const double xy[3][2] = {{-1, -1}, {1, -1}, {-1, 1}};
  for (unsigned v = 0; v < 3; ++v) {
    for (double x : {xy[v][0], xy[v][1], 0.0, 0.0, 0.0, 1.0})
      w.number(x);
    w.word(v + 1, 8);
  }
  for (uint64_t index : {0, 1, 2, 0})
    w.word(index, 4);
  w.word(1, 8);
  for (double x : {.8, .8, .8, .5})
    w.number(x);
  {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    check(file.write(w.out.data(), static_cast<std::streamsize>(w.out.size())).good(),
          "cannot write probe " + path);
  }
  try {
    bgref::RawScene scene(path);
  } catch (const bgom10::Failure &error) {
    const std::string message(error.what());
    return message.find("raw_decode: nonproper/nonrigid matrix") != std::string::npos
               ? "refused"
               : "other:" + message;
  }
  return "admitted";
}
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 3, "usage: brepgpu-reference-placement-admission CORPUS.tsv PROBE.bgrefmesh");
    std::ifstream input(argv[1]);
    check(static_cast<bool>(input), "cannot read placement corpus");
    std::string line;
    unsigned rows = 0, mismatches = 0;
    while (std::getline(input, line)) {
      if (line.empty() || line[0] == '#')
        continue;
      std::istringstream fields(line);
      std::string name, expected, item;
      check(static_cast<bool>(std::getline(fields, name, '\t')) &&
                static_cast<bool>(std::getline(fields, expected, '\t')),
            "placement corpus row name/status");
      double matrix[12];
      for (double &x : matrix) {
        check(static_cast<bool>(std::getline(fields, item, '\t')), "placement corpus width");
        char *end = nullptr;
        x = std::strtod(item.c_str(), &end);
        check(end && *end == '\0', "placement corpus scalar " + item);
      }
      check(!std::getline(fields, item, '\t'), "placement corpus trailing field");
      const auto status = reference_status(matrix, argv[2]);
      const bool agree = status == (expected == "rigid" ? "admitted" : "refused");
      std::cout << name << '\t' << expected << "\treference=" << status << '\t'
                << (agree ? "AGREE" : "DISAGREE") << '\n';
      ++rows;
      mismatches += agree ? 0 : 1;
    }
    check(rows > 0, "empty placement corpus");
    check(mismatches == 0, std::to_string(mismatches) + " placement admission disagreements");
    std::cout << "PASS\traw reference display follows the one rigid placement rule over " << rows
              << " corpus matrices\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
