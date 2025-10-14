#include "net/unix/base_socket.hpp"

#include <unistd.h>

namespace core::net::unix {

namespace {

constexpr int kSyscallError = -1;

}  // namespace

base_socket::base_socket(io::uninitialized_t) noexcept
    : net::base_socket(core::io::uninitialized_t{}) {}

base_socket::base_socket(net::sockets::type type) noexcept(false)
    : net::base_socket(net::sockets::family::kUnix, type,
                       net::sockets::protocol::kUnspecified) {}

base_socket::~base_socket() noexcept {
  static const base_socket kUninitialized(io::uninitialized_t{});
  if (*this == kUninitialized) {
    return;
  }

  net::unix::sockaddr sockaddr(net::unix::sockaddr::kEmpty());
  try {
    get_bind_sockaddr(sockaddr);
    static_cast<void>(::unlink(sockaddr.get_path().data()));
  } catch (...) {
  }
}

void base_socket::unlink_and_close() noexcept(false) {
  static const base_socket kUninitialized(io::uninitialized_t{});
  if (*this == kUninitialized) {
    return;
  }

  net::unix::sockaddr sockaddr(net::unix::sockaddr::kEmpty());
  get_bind_sockaddr(sockaddr);

  if (::unlink(sockaddr.get_path().data()) == kSyscallError) {
    if (errno != ENOENT) {
      throw std::runtime_error(
          std::format("unlink failed: {}", std::strerror(errno)));
    }
  }
  net::base_socket::close();
}

net::bind_status base_socket::unlink_bind(
    const net::unix::sockaddr& sockaddr) noexcept(false) {
  if (::unlink(sockaddr.get_path().data()) == kSyscallError) {
    if (errno != ENOENT) {
      throw std::runtime_error(
          std::format("unlink failed: {}", std::strerror(errno)));
    }
  }
  return net::base_socket::bind(sockaddr);
}

}  // namespace core::net::unix
