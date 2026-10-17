#ifndef BREPGPU_MESHER_OUTPUT_DIRECTORY_HPP
#define BREPGPU_MESHER_OUTPUT_DIRECTORY_HPP
#include "values.hpp"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <linux/fs.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace bgmesh {
// Stage a complete operation beside its destination. Only this owner's unique
// temporary tree is removed on failure; rename never replaces an existing tree.
class OutputDirectory final {
  std::filesystem::path final_, temporary_;
  bool published_ = false;

public:
  explicit OutputDirectory(const std::filesystem::path &target) : final_(target) {
    if (std::filesystem::exists(std::filesystem::symlink_status(final_)))
      throw Error(ErrorCode::io_failure, "output already exists; publication never replaces it");
    if (!std::filesystem::is_directory(final_.parent_path()))
      throw Error(ErrorCode::io_failure, "output parent must be an existing directory");
    auto pattern = (final_.parent_path() / (final_.filename().string() + ".tmp-XXXXXX")).string();
    if (!::mkdtemp(pattern.data()))
      throw Error(ErrorCode::io_failure,
                  "cannot create temporary output: " + std::string(std::strerror(errno)));
    temporary_ = pattern;
  }
  OutputDirectory(const OutputDirectory &) = delete;
  OutputDirectory &operator=(const OutputDirectory &) = delete;
  ~OutputDirectory() {
    if (published_ || temporary_.empty())
      return;
    std::error_code failure;
    std::filesystem::remove_all(temporary_, failure);
    if (failure)
      std::cerr << "cleanup\ttemporary_output\t" << failure.message() << '\n';
  }
  [[nodiscard]] const std::filesystem::path &path() const { return temporary_; }
  void publish() {
    if (::syscall(SYS_renameat2, AT_FDCWD, temporary_.c_str(), AT_FDCWD, final_.c_str(),
                  RENAME_NOREPLACE) != 0)
      throw Error(ErrorCode::io_failure,
                  "no-replace publication failed: " + std::string(std::strerror(errno)));
    published_ = true;
  }
};
} // namespace bgmesh
#endif
