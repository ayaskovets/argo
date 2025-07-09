#include "io/fd.hpp"

#include <unistd.h>

#include <cerrno>
#include <format>

namespace core::io {

namespace {

constexpr int kSyscallError = -1;
constexpr int kInvalidFd = -1;

}  // namespace

struct fd::impl final {
  int native_handle;
};

const fd& fd::kStdin() noexcept {
  static const fd fd = fd::adopt(STDIN_FILENO);
  return fd;
}

const fd& fd::kStdout() noexcept {
  static const fd fd = fd::adopt(STDOUT_FILENO);
  return fd;
}

const fd& fd::kStderr() noexcept {
  static const fd fd = fd::adopt(STDERR_FILENO);
  return fd;
}

fd::fd(uninitialized_t) noexcept : pimpl_(impl{.native_handle = kInvalidFd}) {}

fd fd::adopt(int fd) {
  if (fd < 0) [[unlikely]] {
    throw std::invalid_argument(std::format("invalid file descriptor: {}", fd));
  }

  class fd adopter(uninitialized_t{});
  *adopter.pimpl_ = impl{.native_handle = fd};
  return adopter;
}

fd::fd(const fd& that) {
  if (that.pimpl_->native_handle == kInvalidFd) [[unlikely]] {
    pimpl_->native_handle = kInvalidFd;
  } else if ((pimpl_->native_handle = ::dup(that.pimpl_->native_handle)) ==
             kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format("failed to clone file descriptor: {}",
                                         std::strerror(errno)));
  }
}

fd& fd::operator=(const fd& that) {
  if (pimpl_->native_handle == that.pimpl_->native_handle) {
    return *this;
  }

  if (pimpl_->native_handle != kInvalidFd &&
      that.pimpl_->native_handle != kInvalidFd &&
      ::dup2(that.pimpl_->native_handle, pimpl_->native_handle) ==
          kSyscallError) {
    throw std::runtime_error(std::format("failed to clone file descriptor: {}",
                                         std::strerror(errno)));
  } else if (pimpl_->native_handle != kInvalidFd &&
             ::close(std::exchange(pimpl_->native_handle, kInvalidFd)) ==
                 kSyscallError) {
    throw std::runtime_error(std::format("failed to close file descriptor: {}",
                                         std::strerror(errno)));
  } else if ((pimpl_->native_handle = ::dup(that.pimpl_->native_handle)) ==
             kSyscallError) {
    throw std::runtime_error(std::format("failed to clone file descriptor: {}",
                                         std::strerror(errno)));
  }
  return *this;
}

fd::fd(fd&& that) noexcept
    : pimpl_(std::exchange(*that.pimpl_, impl{.native_handle = kInvalidFd})) {}

fd& fd::operator=(fd&& that) {
  if (pimpl_->native_handle == that.pimpl_->native_handle) {
    return *this;
  }

  if (pimpl_->native_handle != kInvalidFd &&
      ::close(std::exchange(pimpl_->native_handle, kInvalidFd)) ==
          kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format("failed to close file descriptor: {}",
                                         std::strerror(errno)));
  }
  pimpl_->native_handle = std::exchange(that.pimpl_->native_handle, kInvalidFd);
  return *this;
}

fd::~fd() noexcept {
  if (pimpl_->native_handle != kInvalidFd) [[unlikely]] {
    [[maybe_unused]] const auto _ =
        ::close(std::exchange(pimpl_->native_handle, kInvalidFd));
  }
}

bool fd::operator==(const fd& that) const noexcept {
  return pimpl_->native_handle == that.pimpl_->native_handle;
}

bool fd::operator!=(const fd& that) const noexcept { return !operator==(that); }

void fd::close() {
  if (pimpl_->native_handle == kInvalidFd) [[unlikely]] {
    throw std::runtime_error(
        std::format("close() called on already closed file descriptor: {}",
                    std::strerror(errno)));
  }
  if (::close(std::exchange(pimpl_->native_handle, kInvalidFd)) ==
      kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format("failed to close file descriptor: {}",
                                         std::strerror(errno)));
  }
}

int fd::get_native_handle() const noexcept { return pimpl_->native_handle; }

}  // namespace core::io
