#include "io/base_fd.hpp"

#include <unistd.h>

#include <cerrno>
#include <format>

namespace core::io {

namespace {

constexpr int kSyscallError = -1;

}  // namespace

class base_fd::impl final {
 private:
  constexpr static inline int kUninitializedFd = -1;

 public:
  constexpr explicit impl(int native_handle) noexcept
      : native_handle_(native_handle) {}
  constexpr explicit impl(uninitialized_t) noexcept
      : native_handle_(kUninitializedFd) {}

 public:
  constexpr bool is_valid() const noexcept { return native_handle_ >= 0; }
  constexpr int native_handle() const noexcept { return native_handle_; }

 private:
  int native_handle_;
};

const base_fd& base_fd::kStdin() noexcept {
  static const base_fd base_fd = base_fd::adopt(STDIN_FILENO);
  return base_fd;
}

const base_fd& base_fd::kStdout() noexcept {
  static const base_fd base_fd = base_fd::adopt(STDOUT_FILENO);
  return base_fd;
}

const base_fd& base_fd::kStderr() noexcept {
  static const base_fd base_fd = base_fd::adopt(STDERR_FILENO);
  return base_fd;
}

base_fd::base_fd(uninitialized_t) noexcept : pimpl_(uninitialized_t{}) {}

base_fd base_fd::adopt(int native_handle) noexcept(false) {
  impl impl(native_handle);
  if (!impl.is_valid()) [[unlikely]] {
    throw std::invalid_argument(
        std::format("invalid file descriptor: {}", native_handle));
  }

  class base_fd adopter(uninitialized_t{});
  adopter.pimpl_.emplace(std::move(impl));
  return adopter;
}

base_fd::base_fd(const base_fd& that) noexcept(false)
    : pimpl_(uninitialized_t{}) {
  if (that.pimpl_->is_valid()) [[likely]] {
    if (const int native_handle = ::dup(that.pimpl_->native_handle());
        native_handle != kSyscallError) [[unlikely]] {
      pimpl_.emplace(native_handle);
    } else {
      throw std::runtime_error(std::format(
          "failed to clone file descriptor: {}", std::strerror(errno)));
    }
  }
}

base_fd& base_fd::operator=(const base_fd& that) noexcept(false) {
  if (pimpl_->native_handle() == that.pimpl_->native_handle()) {
    return *this;
  }

  if (pimpl_->is_valid() && that.pimpl_->is_valid()) {
    if (::dup2(that.pimpl_->native_handle(), pimpl_->native_handle()) ==
        kSyscallError) [[unlikely]] {
      throw std::runtime_error(std::format(
          "failed to clone file descriptor: {} ", std::strerror(errno)));
    }
  } else if (pimpl_->is_valid()) {
    if (::close(
            std::exchange(pimpl_, impl(uninitialized_t{}))->native_handle()) ==
        kSyscallError) [[unlikely]] {
      throw std::runtime_error(std::format(
          "failed to close file descriptor: {} ", std::strerror(errno)));
    }
  } else if (const int native_handle = ::dup(that.pimpl_->native_handle());
             native_handle != kSyscallError) [[likely]] {
    pimpl_.emplace(native_handle);
  } else [[unlikely]] {
    throw std::runtime_error(std::format("failed to clone file descriptor: {} ",
                                         std::strerror(errno)));
  }
  return *this;
}

base_fd::base_fd(base_fd&& that) noexcept
    : pimpl_(std::exchange(*that.pimpl_, impl(uninitialized_t{}))) {}

base_fd& base_fd::operator=(base_fd&& that) noexcept(false) {
  if (pimpl_->native_handle() == that.pimpl_->native_handle()) {
    return *this;
  }

  if (pimpl_->is_valid() &&
      ::close(
          std::exchange(pimpl_, impl(uninitialized_t{}))->native_handle()) ==
          kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format("failed to close file descriptor: {} ",
                                         std::strerror(errno)));
  }

  pimpl_ = std::exchange(that.pimpl_, impl(uninitialized_t{}));
  return *this;
}

base_fd::~base_fd() noexcept {
  if (pimpl_->is_valid()) [[unlikely]] {
    [[maybe_unused]] const int _ = ::close(
        std::exchange(pimpl_, impl(uninitialized_t{}))->native_handle());
  }
}

bool base_fd::operator==(const base_fd& that) const noexcept {
  return pimpl_->native_handle() == that.pimpl_->native_handle();
}

bool base_fd::operator!=(const base_fd& that) const noexcept {
  return !operator==(that);
}

void base_fd::close() noexcept(false) {
  if (!pimpl_->is_valid()) [[unlikely]] {
    throw std::runtime_error(
        std::format("close() called on already closed file descriptor: {}",
                    std::strerror(errno)));
  }

  if (::close(
          std::exchange(pimpl_, impl(uninitialized_t{}))->native_handle()) ==
      kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format("failed to close file descriptor: {} ",
                                         std::strerror(errno)));
  }
}

int base_fd::native_handle() const noexcept { return pimpl_->native_handle(); }

}  // namespace core::io
