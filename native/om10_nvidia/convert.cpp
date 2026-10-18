// Entry to the unmodified official reference converter.
//
// INPUT.brep NEW_OUTPUT.usdc converts one file in one process. --serve FD
// keeps one process per preparation worker, as an NVIDIA user converting
// in-process would: one context and plugin setup, then one job per stdin line
// "INPUT<TAB>NEW_OUTPUT", each answered, flushed, on the dedicated descriptor
// FD (never stdout, which OCCT and USD may write to) with
// "job<TAB>0|1<TAB>conversion_seconds<TAB>message". A job line without exactly
// one tab is refused. The first conversion of a
// process carries one-time setup (about 10 ms on the authored fixtures, against
// 2.7 ms for later conversions); serving pays it once per worker, as in-process
// use does. Timing lines on stdout (single mode at exit, serve mode at start
// and end): main start and end on CLOCK_MONOTONIC (steady_clock), which the
// launching preparation worker also uses, and each ConvertOcctFileToUsd span.
#include <SmApiGeneral.h>
#include <SmApiUsd.h>
#include <unistd.h>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace occt {
bool ConvertOcctFileToUsd(const std::string &, const std::string &, std::string &);
}
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
double monotonic(Clock::time_point t) { return std::chrono::duration<double>(t.time_since_epoch()).count(); }
std::string number(double value) {
  std::ostringstream out;
  out << std::setprecision(17) << value;
  return out.str();
}
std::string clean(std::string value) {
  for (char &c : value)
    if (c == '\t' || c == '\n' || c == '\r') c = ' ';
  return value;
}
// One conversion into a pending file, renamed only on success. Returns the
// ConvertOcctFileToUsd span when the bridge was called.
std::optional<double> convert(const fs::path &input, const fs::path &output) {
  require(!fs::exists(output), "output already exists");
  require(fs::is_regular_file(input), "input is not a regular file");
  const fs::path temporary = output.string() + ".pending.usdc";
  require(!fs::exists(temporary), "temporary output already exists");
  std::string message;
  const auto begin = Clock::now();
  bool converted = false;
  try {
    converted = occt::ConvertOcctFileToUsd(input.string(), temporary.string(), message);
  } catch (...) {
    std::error_code ignored;
    fs::remove(temporary, ignored);
    throw;
  }
  const double span = std::chrono::duration<double>(Clock::now() - begin).count();
  if (!converted || !fs::is_regular_file(temporary)) {
    std::error_code ignored;
    fs::remove(temporary, ignored);
    throw std::runtime_error(std::string("official OCCT-to-USD bridge: ") +
                             (converted ? "converter did not emit a USD file" : message) + "\tspan=" +
                             number(span));
  }
  fs::rename(temporary, output);
  return span;
}
// Protocol replies on their own descriptor, one flushed line each.
struct Replies {
  FILE *stream;
  explicit Replies(int fd) : stream(fdopen(fd, "w")) { require(stream != nullptr, "reply descriptor is not open"); }
  void line(const std::string &text) {
    std::fputs((text + "\n").c_str(), stream);
    std::fflush(stream);
  }
};
int serve(Clock::time_point main_start, int fd) {
  Replies replies(fd);
  replies.line("main_start_monotonic=" + number(monotonic(main_start)));
  SmApiCreateContext();
  require(SmApiUsdEnsurePluginRegistered() == SM_SUCCESS, "reference plugin unavailable");
  replies.line("ready");
  std::string line;
  while (std::getline(std::cin, line)) {
    const auto tab = line.find('\t');
    std::string status = "1", span, message;
    try {
      require(tab != std::string::npos && line.find('\t', tab + 1) == std::string::npos,
              "job needs exactly INPUT<TAB>OUTPUT");
      span = number(*convert(line.substr(0, tab), line.substr(tab + 1)));
      status = "0";
    } catch (const std::exception &error) {
      message = error.what();
      const auto mark = message.rfind("\tspan=");
      if (mark != std::string::npos) {
        span = message.substr(mark + 6);
        message.resize(mark);
      }
    }
    replies.line("job\t" + status + "\t" + span + "\t" + clean(message));
  }
  replies.line("main_end_monotonic=" + number(monotonic(Clock::now())));
  return 0;
}
int main(int argc, char **argv) {
  const auto main_start = Clock::now();
  std::optional<double> conversion;
  try {
    if (argc == 2 && std::string(argv[1]) == "--help") {
      std::cout << "Official NVIDIA CPU OCCT-to-USD conversion only; no tessellation.\n"
                   "INPUT.brep NEW_OUTPUT.usdc | --serve REPLY_FD (jobs INPUT<TAB>NEW_OUTPUT on stdin)\n";
      return 0;
    }
    if (argc == 3 && std::string(argv[1]) == "--serve") return serve(main_start, std::stoi(argv[2]));
    require(argc == 3, "usage: INPUT.brep NEW_OUTPUT.usdc | --serve");
    SmApiCreateContext();
    require(SmApiUsdEnsurePluginRegistered() == SM_SUCCESS, "reference plugin unavailable");
    conversion = convert(argv[1], argv[2]);
    std::cout << std::setprecision(17) << "converted_seconds=" << *conversion << '\n'
              << "main_start_monotonic=" << monotonic(main_start) << '\n'
              << "conversion_seconds=" << *conversion << '\n'
              << "main_end_monotonic=" << monotonic(Clock::now()) << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::string message = error.what();
    const auto mark = message.rfind("\tspan=");
    std::cout << std::setprecision(17) << "main_start_monotonic=" << monotonic(main_start) << '\n';
    if (mark != std::string::npos) {
      std::cout << "conversion_seconds=" << message.substr(mark + 6) << '\n';
      message.resize(mark);
    }
    std::cout << "main_end_monotonic=" << monotonic(Clock::now()) << '\n';
    std::cerr << message << '\n';
    return 1;
  }
}
