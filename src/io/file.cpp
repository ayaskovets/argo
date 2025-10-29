#include <sandbox/io/fd_impl.hpp>
#include <sandbox/io/file.hpp>
#include <sandbox/utils/syscall.hpp>

#include <format>

#include <fcntl.h>
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

constexpr int to_native_flags(
    std::common_type_t<fd::flags_t, file::creation_flags_t> flags) noexcept {
  int native_flags{};
  native_flags |= (flags & file::creation_flag::kCreat) ? O_CREAT : 0;
  native_flags |= (flags & file::creation_flag::kExcl) ? O_EXCL : 0;
  native_flags |= (flags & file::creation_flag::kTrunc) ? O_TRUNC : 0;
  native_flags |= (flags & fd::flag::kAppend) ? O_APPEND : 0;
  native_flags |= (flags & file::creation_flag::kCloexec) ? O_CLOEXEC : 0;
  native_flags |= (flags & fd::flag::kNonblock) ? O_NONBLOCK : 0;
  native_flags |= (flags & file::creation_flag::kSync) ? O_SYNC : 0;
  native_flags |= (flags & file::creation_flag::kDsync) ? O_DSYNC : 0;
  native_flags |= (flags & file::creation_flag::kRdonly) ? O_RDONLY : 0;
  native_flags |= (flags & file::creation_flag::kWronly) ? O_WRONLY : 0;
  native_flags |= (flags & file::creation_flag::kRdwr) ? O_RDWR : 0;
  return native_flags;
}

constexpr std::common_type_t<fd::flags_t, file::creation_flags_t>
from_native_flags(int native_flags) noexcept {
  std::common_type_t<fd::flags_t, file::creation_flags_t> flags{};
  flags |= (native_flags & O_CREAT) ? file::creation_flag::kCreat : 0;
  flags |= (native_flags & O_EXCL) ? file::creation_flag::kExcl : 0;
  flags |= (native_flags & O_TRUNC) ? file::creation_flag::kTrunc : 0;
  flags |= (native_flags & O_APPEND) ? fd::flag::kAppend : 0;
  flags |= (native_flags & O_CLOEXEC) ? file::creation_flag::kCloexec : 0;
  flags |= (native_flags & O_NONBLOCK) ? fd::flag::kNonblock : 0;
  flags |= (native_flags & O_SYNC) ? file::creation_flag::kSync : 0;
  flags |= (native_flags & O_DSYNC) ? file::creation_flag::kDsync : 0;
  flags |= (native_flags & O_RDONLY) ? file::creation_flag::kRdonly : 0;
  flags |= (native_flags & O_WRONLY) ? file::creation_flag::kWronly : 0;
  flags |= (native_flags & O_RDWR) ? file::creation_flag::kRdwr : 0;
  return flags;
}

}  // namespace

file::file(const std::filesystem::path& path, fd::flags_t flags) noexcept(false)
    : io::fd(io::fd::adopt(SYSCALL(
          io::fd::impl(::open(path.c_str(), to_native_flags(flags)))))) {}

std::common_type_t<fd::flags_t, file::creation_flags_t> file::get_flags() const
    noexcept(false) {
  const auto retval = SYSCALL(::fcntl(*native_handle(), F_GETFL));
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to get O_NONBLOCK on socket: {}", std::strerror(errno)));
  }
  return from_native_flags(retval);
}

void file::seek(std::size_t offset, seek_whence whence) noexcept(false) {
  if (SYSCALL(::lseek(*native_handle(), static_cast<::off_t>(offset),
                      to_native_seek_whence(whence))) ==
      static_cast<::off_t>(kSyscallError)) {
    throw std::runtime_error(
        std::format("failed to seek fd: {}", std::strerror(errno)));
  }
}

void file::truncate(std::size_t bytes) noexcept(false) {
  if (SYSCALL(::ftruncate(*native_handle(), static_cast<::off_t>(bytes))) ==
      kSyscallError) {
    throw std::runtime_error(
        std::format("failed to truncate fd: {}", std::strerror(errno)));
  }
}

}  // namespace sandbox::io
