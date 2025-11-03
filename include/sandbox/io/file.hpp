#pragma once

#include <sandbox/io/fd.hpp>

#include <filesystem>

namespace sandbox::io {

class file final : public fd {
 public:
  enum open_flag : std::uint8_t {
    kTrunc = 1 << 0,    ///< Truncate file when created
    kCloexec = 1 << 1,  ///< Child processes do not have access to this file
  };
  using open_flags_t = std::underlying_type_t<open_flag>;

  enum flag : std::uint8_t {
    kAppend = 1 << 1,  ///< Open in append mode
    kSync = 1 << 2,    ///< Set synchronous mode
    kDsync = 1 << 3,   ///< Set synchronous mode for data
  };
  using flags_t = std::underlying_type_t<flag>;

  enum class access_mode : std::uint8_t {
    kRdonly,  ///< Set read-only mode
    kWronly,  ///< Set write-only mode
    kRdwr,    ///< Set read-write mode
  };

 public:
  static file open(const std::filesystem::path& path, access_mode mode,
                   open_flags_t open_flags = {},
                   std::common_type_t<fd::flags_t, file::flags_t> flags =
                       {}) noexcept(false);
  static file create(const std::filesystem::path& path, access_mode mode,
                     open_flags_t open_flags = {},
                     std::common_type_t<fd::flags_t, file::flags_t> flags = {},
                     std::filesystem::perms perms =
                         std::filesystem::perms::owner_all |
                         std::filesystem::perms::group_read |
                         std::filesystem::perms::others_read) noexcept(false);

 public:
  static_assert(std::is_same_v<fd::flags_t, file::flags_t>);

  void set_flags(std::common_type_t<fd::flags_t, file::flags_t> flags) noexcept(
      false);
  std::common_type_t<fd::flags_t, file::flags_t> get_flags() const
      noexcept(false);

 public:
  access_mode get_access_mode() const noexcept(false);

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
  void set_offset(std::int64_t relative_bytes,
                  seek_whence whence = seek_whence::kBegin) noexcept(false);
  std::size_t get_offset() noexcept(false);

 public:
  void truncate(std::size_t bytes) noexcept(false);

 public:
  std::size_t get_size_bytes() noexcept(false);

 private:
  file(io::fd&& fd) noexcept;
};

}  // namespace sandbox::io
