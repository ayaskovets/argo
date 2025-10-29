#include <sandbox/io/fd.hpp>
#include <sandbox/io/fd_impl.hpp>
#include <sandbox/utils/syscall.hpp>

#include <cerrno>
#include <format>

#include <fcntl.h>
#include <unistd.h>

namespace sandbox::io {

namespace {

constexpr int kSyscallError = -1;
constexpr int kUninitializedNativeHandle = -1;

constexpr bool is_valid(int native_handle) noexcept {
  return native_handle >= 0;
}

constexpr int to_native_flags(fd::flags_t flags) noexcept {
  int native_flags{};
  native_flags |= (flags & fd::flag::kAppend) ? O_APPEND : 0;
  native_flags |= (flags & fd::flag::kNonblock) ? O_NONBLOCK : 0;
  return native_flags;
}

constexpr fd::flags_t from_native_flags(int native_flags) noexcept {
  int flags{};
  flags |= (native_flags & O_APPEND) ? fd::flag::kAppend : 0;
  flags |= (native_flags & O_NONBLOCK) ? fd::flag::kNonblock : 0;
  return flags;
}

}  // namespace

fd& fd::kStdin() noexcept {
  static fd fd = fd::adopt(fd::impl(STDIN_FILENO));
  return fd;
}

fd& fd::kStdout() noexcept {
  static fd fd = fd::adopt(fd::impl(STDOUT_FILENO));
  return fd;
}

fd& fd::kStderr() noexcept {
  static fd fd = fd::adopt(fd::impl(STDERR_FILENO));
  return fd;
}

fd::fd(uninitialized_t) noexcept : pimpl_(kUninitializedNativeHandle) {}

fd fd::adopt(const impl& native_handle) noexcept(false) {
  if (!is_valid(*native_handle)) [[unlikely]] {
    throw std::invalid_argument(
        std::format("invalid fd descriptor: {}", *native_handle));
  }

  class fd adopter(uninitialized_t{});
  adopter.pimpl_.emplace(native_handle);
  return adopter;
}

fd::fd(const fd& that) noexcept(false) : pimpl_(kUninitializedNativeHandle) {
  if (!is_valid(**that.pimpl_)) [[unlikely]] {
    return;
  }

  const auto retval = SYSCALL(::dup(**that.pimpl_));
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(
        std::format("failed to clone fd descriptor: {}", std::strerror(errno)));
  }

  pimpl_.emplace(retval);
}

fd& fd::operator=(const fd& that) noexcept(false) {
  if (*pimpl_ == *that.pimpl_) [[unlikely]] {
    return *this;
  }

  if (is_valid(**pimpl_) && is_valid(**that.pimpl_)) {
    const auto retval = SYSCALL(::dup2(**that.pimpl_, (**pimpl_)));
    if (retval == kSyscallError) [[unlikely]] {
      throw std::runtime_error(std::format("failed to clone fd descriptor: {} ",
                                           std::strerror(errno)));
    }
    return *this;
  }

  if (is_valid(**pimpl_)) {
    const auto retval = SYSCALL(::close(**pimpl_));
    pimpl_.emplace(kUninitializedNativeHandle);
    if (retval == kSyscallError) [[unlikely]] {
      throw std::runtime_error(std::format("failed to close fd descriptor: {} ",
                                           std::strerror(errno)));
    }
    return *this;
  }

  const auto retval = SYSCALL(::dup(**that.pimpl_));
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format("failed to clone fd descriptor: {} ",
                                         std::strerror(errno)));
  }

  pimpl_.emplace(retval);
  return *this;
}

fd::fd(fd&& that) noexcept
    : pimpl_(std::exchange(*that.pimpl_, impl(kUninitializedNativeHandle))) {}

fd& fd::operator=(fd&& that) noexcept(false) {
  if (*pimpl_ == *that.pimpl_) [[unlikely]] {
    return *this;
  }

  if (is_valid(**pimpl_)) {
    const auto retval = SYSCALL(::close(**pimpl_));
    pimpl_.emplace(kUninitializedNativeHandle);
    if (retval == kSyscallError) [[unlikely]] {
      throw std::runtime_error(std::format("failed to close fd descriptor: {} ",
                                           std::strerror(errno)));
    }
  }

  pimpl_ = std::exchange(that.pimpl_, impl(kUninitializedNativeHandle));
  return *this;
}

fd::~fd() noexcept {
  if (!is_valid(**pimpl_)) [[unlikely]] {
    return;
  }

  [[maybe_unused]] const auto retval = SYSCALL(::close(**pimpl_));
  pimpl_.emplace(kUninitializedNativeHandle);
}

void fd::set_flags(flags_t flags) noexcept(false) {
  if (SYSCALL(::fcntl(**pimpl_, F_SETFL, to_native_flags(flags)) ==
              kSyscallError)) [[unlikely]] {
    throw std::runtime_error(std::format("failed to set O_NONBLOCK on fd: {}",
                                         std::strerror(errno)));
  }
}

fd::flags_t fd::get_flags() noexcept(false) {
  const auto retval = SYSCALL(::fcntl(**pimpl_, F_GETFL));
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to get O_NONBLOCK on socket: {}", std::strerror(errno)));
  }
  return from_native_flags(retval);
}

std::size_t fd::write(std::span<const std::byte> bytes) noexcept(false) {
  const auto retval = SYSCALL(::write(**pimpl_, bytes.data(), bytes.size()));
  if (retval == kSyscallError) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS) {
      return 0;
    }
    throw std::runtime_error(
        std::format("failed to write to fd: {}", std::strerror(errno)));
  }
  return retval;
}

std::size_t fd::read(std::span<std::byte> bytes) noexcept(false) {
  const auto retval = SYSCALL(::read(**pimpl_, bytes.data(), bytes.size()));
  if (retval == kSyscallError) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS) {
      return 0;
    }
    throw std::runtime_error(
        std::format("failed to read from fd: {}", std::strerror(errno)));
  }
  return retval;
}

bool fd::operator==(const fd& that) const noexcept {
  return *pimpl_ == *that.pimpl_;
}

bool fd::operator!=(const fd& that) const noexcept { return !operator==(that); }

void fd::close() noexcept(false) {
  if (!is_valid(**pimpl_)) [[unlikely]] {
    throw std::runtime_error(
        std::format("close() called on already closed fd descriptor: {}",
                    std::strerror(errno)));
  }

  const auto retval = SYSCALL(::close(**pimpl_));
  pimpl_.emplace(kUninitializedNativeHandle);
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format("failed to close fd descriptor: {} ",
                                         std::strerror(errno)));
  }
}

const fd::impl& fd::native_handle() const noexcept { return *pimpl_; }

}  // namespace sandbox::io
