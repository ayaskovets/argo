#include "net/base_socket.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>

#include <cerrno>
#include <format>

namespace core::net {

namespace {

constexpr int kSyscallError = -1;

constexpr int to_native_family(net::sockets::family family) noexcept(false) {
  switch (family) {
    case net::sockets::family::kUnspecified:
      throw std::invalid_argument(
          std::format("invalid socket family: {}", family));
    case net::sockets::family::kInet:
      return AF_INET;
    case net::sockets::family::kInet6:
      return AF_INET6;
    case net::sockets::family::kUnix:
      return AF_UNIX;
  }
}

constexpr net::sockets::family from_native_family(int family) noexcept(false) {
  switch (family) {
    case AF_INET:
      return net::sockets::family::kInet;
    case AF_INET6:
      return net::sockets::family::kInet6;
    case AF_UNIX:
      return net::sockets::family::kUnix;
    [[unlikely]] default:
      throw std::runtime_error("unimplemented");
  }
}

constexpr int to_native_type(net::sockets::type type) noexcept {
  switch (type) {
    case net::sockets::type::kStream:
      return SOCK_STREAM;
    case net::sockets::type::kDgram:
      return SOCK_DGRAM;
    case net::sockets::type::kRaw:
      return SOCK_RAW;
    case net::sockets::type::kRdm:
      return SOCK_RDM;
    case net::sockets::type::kSeqpacket:
      return SOCK_SEQPACKET;
  }
}

constexpr net::sockets::type from_native_type(int type) noexcept(false) {
  switch (type) {
    case SOCK_STREAM:
      return net::sockets::type::kStream;
    case SOCK_DGRAM:
      return net::sockets::type::kDgram;
    case SOCK_RAW:
      return net::sockets::type::kRaw;
    case SOCK_RDM:
      return net::sockets::type::kRdm;
    case SOCK_SEQPACKET:
      return net::sockets::type::kSeqpacket;
    [[unlikely]] default:
      throw std::runtime_error("unimplemented");
  }
}

constexpr int to_native_protocol(net::sockets::protocol protocol) noexcept {
  switch (protocol) {
    case net::sockets::protocol::kUnspecified:
      return 0;
    case net::sockets::protocol::kTcp:
      return IPPROTO_TCP;
    case net::sockets::protocol::kUdp:
      return IPPROTO_UDP;
  }
}

}  // namespace

base_socket::base_socket(io::fd::uninitialized_t) noexcept
    : io::fd(io::fd::uninitialized_t{}) {}

base_socket::base_socket(net::sockets::family family, net::sockets::type type,
                         net::sockets::protocol protocol) noexcept(false)
    : io::fd(io::fd::adopt([family, type, protocol] {
        const int fd = ::socket(to_native_family(family), to_native_type(type),
                                to_native_protocol(protocol));
        if (fd == kSyscallError) [[unlikely]] {
          throw std::runtime_error(
              std::format("socket() failed: {}", std::strerror(errno)));
        }
        return fd;
      }())) {}

void base_socket::set_nonblock(bool value) noexcept(false) {
  int flags = ::fcntl(native_handle(), F_GETFL);
  if (flags == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format("failed to get F_GETFL on socket: {}",
                                         std::strerror(errno)));
  }

  flags = value ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
  if (::fcntl(native_handle(), F_SETFL, flags) == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to set O_NONBLOCK on socket: {}", std::strerror(errno)));
  }
}

void base_socket::set_reuseaddr(bool value) noexcept(false) {
  const int optval = (value ? 1 : 0);
  if (::setsockopt(native_handle(), SOL_SOCKET, SO_REUSEADDR, &optval,
                   sizeof(optval)) == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to set SO_REUSEADDR on socket: {}", std::strerror(errno)));
  }
}

void base_socket::set_reuseport(bool value) noexcept(false) {
  const int optval = (value ? 1 : 0);
  if (::setsockopt(native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval,
                   sizeof(optval)) == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to set SO_REUSEPORT on socket: {}", std::strerror(errno)));
  }
}

void base_socket::set_keepalive(bool value) noexcept(false) {
  const int optval = (value ? 1 : 0);
  if (::setsockopt(native_handle(), SOL_SOCKET, SO_KEEPALIVE, &optval,
                   sizeof(optval)) == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to set SO_KEEPALIVE on socket: {}", std::strerror(errno)));
  }
}

net::sockets::family base_socket::get_family() const noexcept(false) {
#ifdef SO_DOMAIN
  int optval;
  ::socklen_t optlen = sizeof(int);
  if (::getsockopt(native_handle(), SOL_SOCKET, SO_DOMAIN, &optval, &optlen) ==
          kSyscallError ||
      optlen != sizeof(optval)) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to get SO_DOMAIN on socket: {}", std::strerror(errno)));
  }
  return from_native_family(optval);
#else
  ::sockaddr_storage storage;
  ::socklen_t socklen = sizeof(storage);
  if (::getsockname(native_handle(), reinterpret_cast<::sockaddr *>(&storage),
                    &socklen) == kSyscallError) {
    throw std::runtime_error(std::format(
        "failed to get SO_DOMAIN on socket: {}", std::strerror(errno)));
  }
  return from_native_family(storage.ss_family);
#endif
}

net::sockets::type base_socket::get_type() const noexcept(false) {
  int optval;
  ::socklen_t optlen = sizeof(optval);
  if (::getsockopt(native_handle(), SOL_SOCKET, SO_TYPE, &optval, &optlen) ==
          kSyscallError ||
      optlen != sizeof(optval)) [[unlikely]] {
    throw std::runtime_error(std::format("failed to get SO_TYPE on socket: {}",
                                         std::strerror(errno)));
  }
  return from_native_type(optval);
}

bool base_socket::get_nonblock() const noexcept(false) {
  const int flags = ::fcntl(native_handle(), F_GETFL);
  if (flags == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to get O_NONBLOCK on socket: {}", std::strerror(errno)));
  }
  return (flags & O_NONBLOCK);
}

bool base_socket::get_reuseaddr() const noexcept(false) {
  int optval;
  ::socklen_t optlen = sizeof(optval);
  if (::getsockopt(native_handle(), SOL_SOCKET, SO_REUSEADDR, &optval,
                   &optlen) == kSyscallError ||
      optlen != sizeof(optval)) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to get SO_REUSEADDR on socket: {}", std::strerror(errno)));
  }
  return (optval ? true : false);
}

bool base_socket::get_reuseport() const noexcept(false) {
  int optval;
  ::socklen_t optlen = sizeof(optval);
  if (::getsockopt(native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval,
                   &optlen) == kSyscallError ||
      optlen != sizeof(optval)) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to get SO_REUSEPORT on socket: {}", std::strerror(errno)));
  }
  return (optval ? true : false);
}

bool base_socket::get_keepalive() const noexcept(false) {
  int optval;
  ::socklen_t optlen = sizeof(optval);
  if (::getsockopt(native_handle(), SOL_SOCKET, SO_KEEPALIVE, &optval,
                   &optlen) == kSyscallError ||
      optlen != sizeof(optval)) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to get SO_KEEPALIVE on socket: {}", std::strerror(errno)));
  }
  return (optval ? true : false);
}

bind_status base_socket::bind(const net::base_sockaddr &sockaddr) noexcept(
    false) {
  if (::bind(native_handle(),
             reinterpret_cast<const ::sockaddr *>(sockaddr.get_storage()),
             sockaddr.get_length()) == kSyscallError) [[unlikely]] {
    switch (errno) {
      case EADDRINUSE:
        return bind_status::kInUse;
      default:
        throw std::runtime_error(
            std::format("failed to bind socket: {}", std::strerror(errno)));
    }
  }
  return bind_status::kSuccess;
}

connect_status base_socket::connect(
    const net::base_sockaddr &sockaddr) noexcept(false) {
  if (::connect(native_handle(),
                reinterpret_cast<const ::sockaddr *>(sockaddr.get_storage()),
                sockaddr.get_length()) == kSyscallError) {
    switch (errno) {
      case EISCONN:
        break;
      case EINPROGRESS:
      case EAGAIN:
      case EALREADY:
        return connect_status::kPending;
      case ECONNREFUSED:
      case ENOENT:
        return connect_status::kRefused;
      [[unlikely]] default:
        throw std::runtime_error(std::format(
            "failed to connect base_socket: {}", std::strerror(errno)));
    }
  }
  return connect_status::kSuccess;
}

void base_socket::get_bind_sockaddr(net::base_sockaddr &sockaddr) const
    noexcept(false) {
  ::socklen_t socklen = static_cast<::socklen_t>(sockaddr.get_length());
  if (::getsockname(native_handle(),
                    reinterpret_cast<::sockaddr *>(sockaddr.get_storage()),
                    &socklen) == kSyscallError) [[unlikely]] {
    throw std::runtime_error(
        std::format("failed to get bind address: {}", std::strerror(errno)));
  }
}

void base_socket::get_connect_sockaddr(net::base_sockaddr &sockaddr) const {
  ::socklen_t socklen = static_cast<::socklen_t>(sockaddr.get_length());
  if (::getpeername(native_handle(),
                    reinterpret_cast<::sockaddr *>(sockaddr.get_storage()),
                    &socklen) == kSyscallError) [[unlikely]] {
    throw std::runtime_error(
        std::format("failed to get peer address: {}", std::strerror(errno)));
  }
}

void base_socket::listen(std::size_t backlog) noexcept(false) {
  if (::listen(native_handle(), static_cast<int>(backlog)) == kSyscallError) {
    throw std::runtime_error(
        std::format("listen failed: {}", std::strerror(errno)));
  }
}

accept_status base_socket::accept(base_socket &socket) const noexcept(false) {
  const int accepted = ::accept(native_handle(), nullptr, nullptr);
  if (accepted == kSyscallError) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return accept_status::kEmptyQueue;
    }
    throw std::runtime_error(
        std::format("accept failed: {}", std::strerror(errno)));
  }
  static_cast<io::fd &>(socket) = io::fd::adopt(accepted);
  return accept_status::kSuccess;
}

std::size_t base_socket::send(std::span<const std::byte> bytes) const
    noexcept(false) {
  const ::ssize_t sent = ::send(native_handle(), bytes.data(), bytes.size(), 0);
  if (sent == kSyscallError) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS) {
      return 0;
    }
    throw std::runtime_error(
        std::format("send failed: {}", std::strerror(errno)));
  }
  return static_cast<std::size_t>(sent);
}

std::size_t base_socket::send_to(std::span<const std::byte> bytes,
                                 const net::base_sockaddr &sockaddr) const
    noexcept(false) {
  const ::ssize_t sent =
      ::sendto(native_handle(), bytes.data(), bytes.size(), 0,
               reinterpret_cast<const ::sockaddr *>(sockaddr.get_storage()),
               sockaddr.get_length());
  if (sent == kSyscallError) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS) {
      return 0;
    }
    throw std::runtime_error(
        std::format("sendto failed: {}", std::strerror(errno)));
  }
  return static_cast<std::size_t>(sent);
}

std::size_t base_socket::receive(std::span<std::byte> bytes) const
    noexcept(false) {
  const ::ssize_t received =
      ::recv(native_handle(), bytes.data(), bytes.size(), 0);
  if (received == kSyscallError) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    }
    throw std::runtime_error(
        std::format("recv failed: {}", std::strerror(errno)));
  }
  return static_cast<std::size_t>(received);
}

std::size_t base_socket::receive_from(std::span<std::byte> bytes,
                                      net::base_sockaddr &sockaddr) const
    noexcept(false) {
  ::socklen_t socklen = static_cast<::socklen_t>(sockaddr.get_length());
  const ::ssize_t received = ::recvfrom(
      native_handle(), bytes.data(), bytes.size(), 0,
      reinterpret_cast<::sockaddr *>(sockaddr.get_storage()), &socklen);
  if (received == kSyscallError) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    }
    throw std::runtime_error(
        std::format("recvfrom failed: {}", std::strerror(errno)));
  }
  return static_cast<std::size_t>(received);
}

}  // namespace core::net
