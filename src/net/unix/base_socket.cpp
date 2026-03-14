#include <wip/net/unix/base_socket.hpp>
#include <wip/utils/syscall.hpp>

#include <unistd.h>

namespace wip::net::unix {

namespace {

constexpr int kSyscallError = -1;

}  // namespace

base_socket::base_socket(io::uninitialized_t) noexcept
    : net::base_socket(wip::io::uninitialized_t{}) {}

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
    if (!sockaddr.get_path().empty()) {
      [[maybe_unused]] const auto retval =
          SYSCALL(::unlink(sockaddr.get_path().c_str()));
    }
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
  if (!sockaddr.get_path().empty()) {
    const auto retval = SYSCALL(::unlink(sockaddr.get_path().c_str()));
    if (retval == kSyscallError) {
      if (errno != ENOENT) {
        throw std::runtime_error(
            std::format("unlink failed: {}", std::strerror(errno)));
      }
    }
  }

  net::base_socket::close();
}

net::bind_status base_socket::bind(
    const net::unix::sockaddr& sockaddr) noexcept(false) {
  switch (net::base_socket::bind(sockaddr)) {
    case net::bind_status::kInUse:
      if (get_reuseaddr()) {
        return unlink_bind(sockaddr);
      }
      return net::bind_status::kInUse;
    case net::bind_status::kSuccess:
      return net::bind_status::kSuccess;
  }
}

net::bind_status base_socket::unlink_bind(
    const net::unix::sockaddr& sockaddr) noexcept(false) {
  const auto retval = SYSCALL(::unlink(sockaddr.get_path().c_str()));
  if (retval == kSyscallError) {
    if (errno != ENOENT) {
      throw std::runtime_error(
          std::format("unlink failed: {}", std::strerror(errno)));
    }
  }
  return net::base_socket::bind(sockaddr);
}

}  // namespace wip::net::unix
