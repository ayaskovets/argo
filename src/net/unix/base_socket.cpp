#include <sandbox/net/unix/base_socket.hpp>
#include <sandbox/utils/syscall.hpp>

#include <unistd.h>

namespace sandbox::net::unix {

namespace {

constexpr int kSyscallError = -1;

}  // namespace

base_socket::base_socket(io::uninitialized_t) noexcept
    : net::base_socket(sandbox::io::uninitialized_t{}) {}

base_socket::base_socket(net::socktype type) noexcept(false)
    : net::base_socket(net::family::kUnix, type, net::protocol::kUnspecified) {}

base_socket::~base_socket() noexcept {
  static const base_socket kUninitialized(io::uninitialized_t{});
  if (*this == kUninitialized) {
    return;
  }

  net::unix::sockaddr sockaddr(net::unix::sockaddr::kEmpty());
  try {
    get_bind_sockaddr(sockaddr);
    [[maybe_unused]] const auto retval =
        SYSCALL(::unlink(sockaddr.get_path().data()));
  } catch (...) {
  }
}

void base_socket::unlink_close() noexcept(false) {
  static const base_socket kUninitialized(io::uninitialized_t{});
  if (*this == kUninitialized) {
    return;
  }

  net::unix::sockaddr sockaddr(net::unix::sockaddr::kEmpty());
  get_bind_sockaddr(sockaddr);

  const auto retval = SYSCALL(::unlink(sockaddr.get_path().data()));
  if (retval == kSyscallError) {
    if (errno != ENOENT) {
      throw std::runtime_error(
          std::format("unlink failed: {}", std::strerror(errno)));
    }
  }
  net::base_socket::close();
}

net::bind_status base_socket::unlink_bind(
    const net::unix::sockaddr& sockaddr) noexcept(false) {
  const auto retval = SYSCALL(::unlink(sockaddr.get_path().data()));
  if (retval == kSyscallError) {
    if (errno != ENOENT) {
      throw std::runtime_error(
          std::format("unlink failed: {}", std::strerror(errno)));
    }
  }
  return net::base_socket::bind(sockaddr);
}

}  // namespace sandbox::net::unix
