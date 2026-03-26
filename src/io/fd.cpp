#include <wip/io/fd.hpp>
#include <wip/io/fd_impl.hpp>
#include <wip/utils/syscall.hpp>

#include <cerrno>
#include <fcntl.h>
#include <format>
#include <unistd.h>

namespace wip::io {

namespace {

constexpr int kSyscallError = -1;
constexpr int kUninitializedNativeHandle = -1;

constexpr bool is_valid(int native_handle) noexcept {
  return native_handle >= 0;
}

constexpr int to_native_flags(fd::flags_t flags) noexcept {
  int native_flags{};
  native_flags |= (flags & fd::flag::kNonblock) ? O_NONBLOCK : 0;
  return native_flags;
}

constexpr fd::flags_t from_native_flags(int native_flags) noexcept {
  int flags{};
  flags |= (native_flags & O_NONBLOCK) ? fd::flag::kNonblock : 0;
  return flags;
}

}  // namespace

fd & fd::kStdin() noexcept {
  static fd fd = fd::adopt(fd::impl(STDIN_FILENO));
  return fd;
}

fd & fd::kStdout() noexcept {
  static fd fd = fd::adopt(fd::impl(STDOUT_FILENO));
  return fd;
}

fd & fd::kStderr() noexcept {
  static fd fd = fd::adopt(fd::impl(STDERR_FILENO));
  return fd;
}

fd::fd(uninitialized_t) noexcept : pimpl_(kUninitializedNativeHandle) {}

fd fd::adopt(impl const & native_handle) noexcept(false) {
  if (!is_valid(*native_handle)) [[unlikely]] {
    throw std::invalid_argument(
        std::format("invalid fd descriptor: {}", *native_handle));
  }

  class fd adopter(uninitialized_t{});
  adopter.pimpl_.emplace(native_handle);
  return adopter;
}

fd::fd(fd const & that) noexcept(false) : pimpl_(kUninitializedNativeHandle) {
  if (!is_valid(*that.native_handle())) [[unlikely]] {
    return;
  }

  auto const retval = SYSCALL(::dup(*that.native_handle()));
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(
        std::format("failed to clone fd descriptor: {}", std::strerror(errno)));
  }

  pimpl_.emplace(retval);
}

fd & fd::operator=(fd const & that) noexcept(false) {
  if (native_handle() == that.native_handle()) [[unlikely]] {
    return *this;
  }

  if (is_valid(*native_handle()) && is_valid(*that.native_handle())) {
    auto const retval =
        SYSCALL(::dup2(*that.native_handle(), (*native_handle())));
    if (retval == kSyscallError) [[unlikely]] {
      throw std::runtime_error(std::format("failed to clone fd descriptor: {} ",
                                           std::strerror(errno)));
    }
    return *this;
  }

  if (is_valid(*native_handle())) {
    auto const retval = SYSCALL(::close(*native_handle()));
    pimpl_.emplace(kUninitializedNativeHandle);
    if (retval == kSyscallError) [[unlikely]] {
      throw std::runtime_error(std::format("failed to close fd descriptor: {} ",
                                           std::strerror(errno)));
    }
    return *this;
  }

  auto const retval = SYSCALL(::dup(*that.native_handle()));
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format("failed to clone fd descriptor: {} ",
                                         std::strerror(errno)));
  }

  pimpl_.emplace(retval);
  return *this;
}

fd::fd(fd && that) noexcept
    : pimpl_(std::exchange(*that.pimpl_, impl(kUninitializedNativeHandle))) {}

fd & fd::operator=(fd && that) noexcept(false) {
  if (native_handle() == that.native_handle()) [[unlikely]] {
    return *this;
  }

  if (is_valid(*native_handle())) {
    auto const retval = SYSCALL(::close(*native_handle()));
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
  if (!is_valid(*native_handle())) [[unlikely]] {
    return;
  }

  [[maybe_unused]] auto const retval = SYSCALL(::close(*native_handle()));
  pimpl_.emplace(kUninitializedNativeHandle);
}

void fd::set_flags(flags_t flags) noexcept(false) {
  if (SYSCALL(::fcntl(*native_handle(), F_SETFL, to_native_flags(flags)) ==
              kSyscallError)) [[unlikely]] {
    throw std::runtime_error(
        std::format("failed to set flags on fd: {}", std::strerror(errno)));
  }
}

fd::flags_t fd::get_flags() const noexcept(false) {
  auto const retval = SYSCALL(::fcntl(*native_handle(), F_GETFL));
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(
        std::format("failed to get flags on fd: {}", std::strerror(errno)));
  }
  return from_native_flags(retval);
}

std::size_t fd::write(std::span<std::byte const> bytes) noexcept(false) {
  auto const retval =
      SYSCALL(::write(*native_handle(), bytes.data(), bytes.size()));
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
  auto const retval =
      SYSCALL(::read(*native_handle(), bytes.data(), bytes.size()));
  if (retval == kSyscallError) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS) {
      return 0;
    }
    throw std::runtime_error(
        std::format("failed to read from fd: {}", std::strerror(errno)));
  }
  return retval;
}

bool fd::operator==(fd const & that) const noexcept {
  return native_handle() == that.native_handle();
}

bool fd::operator!=(fd const & that) const noexcept {
  return !operator==(that);
}

void fd::close() noexcept(false) {
  if (!is_valid(*native_handle())) [[unlikely]] {
    throw std::runtime_error(
        std::format("close() called on already closed fd descriptor: {}",
                    std::strerror(errno)));
  }

  auto const retval = SYSCALL(::close(*native_handle()));
  pimpl_.emplace(kUninitializedNativeHandle);
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format("failed to close fd descriptor: {} ",
                                         std::strerror(errno)));
  }
}

fd::impl const & fd::native_handle() const noexcept { return *pimpl_; }

}  // namespace wip::io
