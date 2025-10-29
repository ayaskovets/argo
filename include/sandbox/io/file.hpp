#pragma once

#include <sandbox/io/fd.hpp>

#include <cstdint>
#include <filesystem>
#include <type_traits>

namespace sandbox::io {

class file final : public fd {
 public:
  enum creation_flag : std::uint16_t {
    kCreat = 1 << 2,    ///< Create file if it does not exist
    kExcl = 1 << 3,     ///< Ensure file is created
    kTrunc = 1 << 4,    ///< Truncate file when created
    kCloexec = 1 << 5,  ///< Close fd in child processes
    kSync = 1 << 6,     ///< Set synchronous mode
    kDsync = 1 << 7,    ///< Set synchronous mode for data
    kRdonly = 1 << 8,   ///< Set read-only mode
    kWronly = 1 << 9,   ///< Set write-only mode
    kRdwr = 1 << 10,    ///< Set read-write mode
  };
  using creation_flags_t = std::underlying_type_t<creation_flag>;

 public:
  file(const std::filesystem::path& path,
       creation_flags_t flags) noexcept(false);

 public:
  static_assert(std::is_same_v<creation_flags_t, flags_t>);
  std::common_type_t<fd::flags_t, creation_flags_t> get_flags() const
      noexcept(false);

 public:
  /**
   * @brief Offset base for seek()
   *
   * @warning Options kData and kHole are not supported for all filesystems
   */
  enum class seek_whence : std::uint8_t {
    kCurrent,  ///< Set offset to current offset + bytes
    kBegin,    ///< Set offset to bytes
    kEnd,      ///< Set offset to file size + bytes
    kData,     ///< Set offset to the nearest data starting from >= offset
    kHole,     ///< Set offset to the nearest hole starting from >= offset
  };
  void seek(std::size_t bytes, seek_whence whence) noexcept(false);

 public:
  void truncate(std::size_t bytes) noexcept(false);
};

}  // namespace sandbox::io
