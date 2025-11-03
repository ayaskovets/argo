#include <sandbox/io/fd_impl.hpp>
#include <sandbox/io/file.hpp>
#include <sandbox/utils/syscall.hpp>

#include <format>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace sandbox::io {

namespace {

constexpr int kSyscallError = -1;

constexpr int to_native_seek_whence(file::seek_whence whence) noexcept {
  switch (whence) {
    case file::seek_whence::kCurrent:
      return SEEK_CUR;
    case file::seek_whence::kBegin:
      return SEEK_SET;
    case file::seek_whence::kEnd:
      return SEEK_END;
    case file::seek_whence::kData:
      return SEEK_DATA;
    case file::seek_whence::kHole:
      return SEEK_HOLE;
  }
}

constexpr int to_native_open_flags(file::open_flags_t flags) noexcept {
  int native_flags{};
  native_flags |= (flags & file::open_flag::kTrunc) ? O_TRUNC : 0;
  native_flags |= (flags & file::open_flag::kCloexec) ? O_CLOEXEC : 0;
  return native_flags;
}

constexpr int to_native_flags(
    std::common_type_t<fd::flags_t, file::flags_t> flags) noexcept {
  int native_flags{};
  native_flags |= (flags & file::flag::kAppend) ? O_APPEND : 0;
  native_flags |= (flags & fd::flag::kNonblock) ? O_NONBLOCK : 0;
  native_flags |= (flags & file::flag::kSync) ? O_SYNC : 0;
  native_flags |= (flags & file::flag::kDsync) ? O_DSYNC : 0;
  return native_flags;
}

constexpr std::common_type_t<fd::flags_t, file::flags_t> from_native_flags(
    int native_flags) noexcept {
  std::common_type_t<fd::flags_t, file::flags_t> flags{};
  flags |= (native_flags & O_NONBLOCK) ? fd::flag::kNonblock : 0;
  flags |= (native_flags & O_APPEND) ? file::flag::kAppend : 0;
  flags |= (native_flags & O_SYNC) ? file::flag::kSync : 0;
  flags |= (native_flags & O_DSYNC) ? file::flag::kDsync : 0;
  return flags;
}

constexpr int to_native_mode(file::access_mode mode) noexcept {
  switch (mode) {
    case file::access_mode::kRdonly:
      return O_RDONLY;
    case file::access_mode::kWronly:
      return O_WRONLY;
    case file::access_mode::kRdwr:
      return O_RDWR;
  }
}

constexpr int native_open(const std::filesystem::path& path, int native_flags,
                          std::filesystem::perms perms) noexcept(false) {
  const int retval = SYSCALL(::open(
      path.c_str(), native_flags,
      static_cast<std::underlying_type_t<std::filesystem::perms>>(perms)));
  if (retval == kSyscallError) {
    throw std::runtime_error(std::format("failed to open file at {}: {}",
                                         path.c_str(), std::strerror(errno)));
  }
  return retval;
}

}  // namespace

file file::open(
    const std::filesystem::path& path, access_mode mode,
    open_flags_t open_flags,
    std::common_type_t<fd::flags_t, file::flags_t> flags) noexcept(false) {
  const int native_flags = to_native_open_flags(open_flags) |
                           to_native_flags(flags) | to_native_mode(mode);
  return file(io::fd::adopt(io::fd::impl(
      native_open(path, native_flags, std::filesystem::perms::none))));
}

file file::create(const std::filesystem::path& path, access_mode mode,
                  open_flags_t open_flags,
                  std::common_type_t<fd::flags_t, file::flags_t> flags,
                  std::filesystem::perms perms) noexcept(false) {
  const int native_flags = to_native_open_flags(open_flags) |
                           to_native_flags(flags) | to_native_mode(mode) |
                           O_CREAT | O_EXCL;
  return file(
      io::fd::adopt(io::fd::impl(native_open(path, native_flags, perms))));
}

void file::set_flags(
    std::common_type_t<fd::flags_t, file::flags_t> flags) noexcept(false) {
  if (SYSCALL(::fcntl(*native_handle(), F_SETFL, to_native_flags(flags)) ==
              kSyscallError)) [[unlikely]] {
    throw std::runtime_error(
        std::format("failed to set flags on file: {}", std::strerror(errno)));
  }
}

std::common_type_t<fd::flags_t, file::flags_t> file::get_flags() const
    noexcept(false) {
  const auto retval = SYSCALL(::fcntl(*native_handle(), F_GETFL));
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(
        std::format("failed to get flags on file: {}", std::strerror(errno)));
  }
  return from_native_flags(retval);
}

void file::seek(std::size_t bytes, seek_whence whence) noexcept(false) {
  if (SYSCALL(::lseek(*native_handle(), static_cast<::off_t>(bytes),
                      to_native_seek_whence(whence))) ==
      static_cast<::off_t>(kSyscallError)) {
    throw std::runtime_error(
        std::format("failed to seek fd: {}", std::strerror(errno)));
  }
}

std::size_t file::tell_bytes() noexcept(false) {
  const ::off_t retval = SYSCALL(::lseek(*native_handle(), 0, SEEK_CUR));
  if (retval == static_cast<::off_t>(kSyscallError)) {
    throw std::runtime_error(
        std::format("failed to seek fd: {}", std::strerror(errno)));
  }
  return retval;
}

std::size_t file::size_bytes() noexcept(false) {
  struct stat stat;
  if (SYSCALL(::fstat(*native_handle(), &stat)) == kSyscallError) {
    throw std::runtime_error(
        std::format("failed to call fstat: {}", std::strerror(errno)));
  }
  return stat.st_size;
}

void file::truncate(std::size_t bytes) noexcept(false) {
  if (SYSCALL(::ftruncate(*native_handle(), static_cast<::off_t>(bytes))) ==
      kSyscallError) {
    throw std::runtime_error(
        std::format("failed to truncate fd: {}", std::strerror(errno)));
  }
}

file::file(io::fd&& fd) noexcept : io::fd(std::move(fd)) {}

}  // namespace sandbox::io
