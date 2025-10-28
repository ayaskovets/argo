#include <sandbox/io/fd_impl.hpp>
#include <sandbox/net/base_socket.hpp>
#include <sandbox/utils/syscall.hpp>

#include <cerrno>
#include <format>

#include <arpa/inet.h>
#include <sys/socket.h>

namespace sandbox::net {

namespace {

constexpr int kSyscallError = -1;

constexpr int to_native_family(net::family family) noexcept(false) {
  switch (family) {
    case net::family::kUnspecified:
      throw std::invalid_argument(
          std::format("invalid socket family: {}", family));
    case net::family::kInet:
      return AF_INET;
    case net::family::kInet6:
      return AF_INET6;
    case net::family::kUnix:
      return AF_UNIX;
  }
}

constexpr net::family from_native_family(int family) noexcept(false) {
  switch (family) {
    case AF_INET:
      return net::family::kInet;
    case AF_INET6:
      return net::family::kInet6;
    case AF_UNIX:
      return net::family::kUnix;
    [[unlikely]] default:
      throw std::runtime_error("unimplemented");
  }
}

constexpr int to_native_type(net::socktype type) noexcept {
  switch (type) {
    case net::socktype::kStream:
      return SOCK_STREAM;
    case net::socktype::kDgram:
      return SOCK_DGRAM;
    case net::socktype::kRaw:
      return SOCK_RAW;
    case net::socktype::kRdm:
      return SOCK_RDM;
    case net::socktype::kSeqpacket:
      return SOCK_SEQPACKET;
  }
}

constexpr net::socktype from_native_type(int type) noexcept(false) {
  switch (type) {
    case SOCK_STREAM:
      return net::socktype::kStream;
    case SOCK_DGRAM:
      return net::socktype::kDgram;
    case SOCK_RAW:
      return net::socktype::kRaw;
    case SOCK_RDM:
      return net::socktype::kRdm;
    case SOCK_SEQPACKET:
      return net::socktype::kSeqpacket;
    [[unlikely]] default:
      throw std::runtime_error("unimplemented");
  }
}

constexpr int to_native_protocol(net::protocol protocol) noexcept {
  switch (protocol) {
    case net::protocol::kUnspecified:
      return 0;
    case net::protocol::kTcp:
      return IPPROTO_TCP;
    case net::protocol::kUdp:
      return IPPROTO_UDP;
  }
}

}  // namespace

base_socket::base_socket(io::uninitialized_t) noexcept
    : io::fd(io::uninitialized_t{}) {}

base_socket::base_socket(net::family family, net::socktype type,
                         net::protocol protocol) noexcept(false)
    : io::fd(io::fd::adopt(fd::impl([family, type, protocol] {
        const auto retval =
            SYSCALL(::socket(to_native_family(family), to_native_type(type),
                             to_native_protocol(protocol)));
        if (retval == kSyscallError) [[unlikely]] {
          throw std::runtime_error(
              std::format("socket() failed: {}", std::strerror(errno)));
        }
        return retval;
      }()))) {}

void base_socket::set_reuseaddr(bool value) noexcept(false) {
  const int optval = (value ? 1 : 0);
  const auto retval = SYSCALL(::setsockopt(
      *native_handle(), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)));
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to set SO_REUSEADDR on socket: {}", std::strerror(errno)));
  }
}

void base_socket::set_reuseport(bool value) noexcept(false) {
  const int optval = (value ? 1 : 0);
  const auto retval = SYSCALL(::setsockopt(
      *native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)));
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to set SO_REUSEPORT on socket: {}", std::strerror(errno)));
  }
}

void base_socket::set_keepalive(bool value) noexcept(false) {
  const int optval = (value ? 1 : 0);
  const auto retval = SYSCALL(::setsockopt(
      *native_handle(), SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)));
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to set SO_KEEPALIVE on socket: {}", std::strerror(errno)));
  }
}

net::family base_socket::get_family() const noexcept(false) {
#ifdef SO_DOMAIN
  int optval;
  ::socklen_t optlen = sizeof(int);
  const auto retval = SYSCALL(
      ::getsockopt(*native_handle(), SOL_SOCKET, SO_DOMAIN, &optval, &optlen));
  if (retval == kSyscallError || optlen != sizeof(optval)) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to get SO_DOMAIN on socket: {}", std::strerror(errno)));
  }
  return from_native_family(optval);
#else
  ::sockaddr_storage storage;
  ::socklen_t socklen = sizeof(storage);
  const auto retval = SYSCALL(::getsockname(
      *native_handle(), reinterpret_cast<::sockaddr *>(&storage), &socklen));
  if (retval == kSyscallError) {
    throw std::runtime_error(std::format(
        "failed to get SO_DOMAIN on socket: {}", std::strerror(errno)));
  }
  return from_native_family(storage.ss_family);
#endif
}

net::socktype base_socket::get_type() const noexcept(false) {
  int optval;
  ::socklen_t optlen = sizeof(optval);
  const auto retval = SYSCALL(
      ::getsockopt(*native_handle(), SOL_SOCKET, SO_TYPE, &optval, &optlen));
  if (retval == kSyscallError || optlen != sizeof(optval)) [[unlikely]] {
    throw std::runtime_error(std::format("failed to get SO_TYPE on socket: {}",
                                         std::strerror(errno)));
  }
  return from_native_type(optval);
}

bool base_socket::get_reuseaddr() const noexcept(false) {
  int optval;
  ::socklen_t optlen = sizeof(optval);
  const auto retval = SYSCALL(::getsockopt(*native_handle(), SOL_SOCKET,
                                           SO_REUSEADDR, &optval, &optlen));
  if (retval == kSyscallError || optlen != sizeof(optval)) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to get SO_REUSEADDR on socket: {}", std::strerror(errno)));
  }
  return (optval ? true : false);
}

bool base_socket::get_reuseport() const noexcept(false) {
  int optval;
  ::socklen_t optlen = sizeof(optval);
  const auto retval = SYSCALL(::getsockopt(*native_handle(), SOL_SOCKET,
                                           SO_REUSEPORT, &optval, &optlen));
  if (retval == kSyscallError || optlen != sizeof(optval)) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to get SO_REUSEPORT on socket: {}", std::strerror(errno)));
  }
  return (optval ? true : false);
}

bool base_socket::get_keepalive() const noexcept(false) {
  int optval;
  ::socklen_t optlen = sizeof(optval);
  const auto retval = SYSCALL(::getsockopt(*native_handle(), SOL_SOCKET,
                                           SO_KEEPALIVE, &optval, &optlen));
  if (retval == kSyscallError || optlen != sizeof(optval)) [[unlikely]] {
    throw std::runtime_error(std::format(
        "failed to get SO_KEEPALIVE on socket: {}", std::strerror(errno)));
  }
  return (optval ? true : false);
}

bind_status base_socket::bind(const net::base_sockaddr &sockaddr) noexcept(
    false) {
  const auto retval = SYSCALL(
      ::bind(*native_handle(),
             reinterpret_cast<const ::sockaddr *>(sockaddr.get_storage()),
             sockaddr.get_length()));
  if (retval == kSyscallError) [[unlikely]] {
    if (errno == EADDRINUSE) {
      return bind_status::kInUse;
    }
    throw std::runtime_error(
        std::format("failed to bind socket: {}", std::strerror(errno)));
  }
  return bind_status::kSuccess;
}

connect_status base_socket::connect(
    const net::base_sockaddr &sockaddr) noexcept(false) {
  const auto retval = SYSCALL(
      ::connect(*native_handle(),
                reinterpret_cast<const ::sockaddr *>(sockaddr.get_storage()),
                sockaddr.get_length()));
  if (retval == kSyscallError) {
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
  const auto retval = SYSCALL(::getsockname(
      *native_handle(), reinterpret_cast<::sockaddr *>(sockaddr.get_storage()),
      &socklen));
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(
        std::format("failed to get bind address: {}", std::strerror(errno)));
  }
}

void base_socket::get_connect_sockaddr(net::base_sockaddr &sockaddr) const {
  ::socklen_t socklen = static_cast<::socklen_t>(sockaddr.get_length());
  const auto retval = SYSCALL(::getpeername(
      *native_handle(), reinterpret_cast<::sockaddr *>(sockaddr.get_storage()),
      &socklen));
  if (retval == kSyscallError) [[unlikely]] {
    throw std::runtime_error(
        std::format("failed to get peer address: {}", std::strerror(errno)));
  }
}

void base_socket::listen(std::size_t backlog) noexcept(false) {
  const auto retval =
      SYSCALL(::listen(*native_handle(), static_cast<int>(backlog)));
  if (retval == kSyscallError) {
    throw std::runtime_error(
        std::format("listen failed: {}", std::strerror(errno)));
  }
}

accept_status base_socket::accept(base_socket &socket) const noexcept(false) {
  const auto retval = SYSCALL(::accept(*native_handle(), nullptr, nullptr));
  if (retval == kSyscallError) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return accept_status::kEmptyQueue;
    }
    throw std::runtime_error(
        std::format("accept failed: {}", std::strerror(errno)));
  }
  static_cast<io::fd &>(socket) = io::fd::adopt(fd::impl(retval));
  return accept_status::kSuccess;
}

std::size_t base_socket::send(std::span<const std::byte> bytes) const
    noexcept(false) {
  const auto retval =
      SYSCALL(::send(*native_handle(), bytes.data(), bytes.size(), 0));
  if (retval == kSyscallError) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS) {
      return 0;
    }
    throw std::runtime_error(
        std::format("send failed: {}", std::strerror(errno)));
  }
  return static_cast<std::size_t>(retval);
}

std::size_t base_socket::send_to(std::span<const std::byte> bytes,
                                 const net::base_sockaddr &sockaddr) const
    noexcept(false) {
  const auto retval = SYSCALL(
      ::sendto(*native_handle(), bytes.data(), bytes.size(), 0,
               reinterpret_cast<const ::sockaddr *>(sockaddr.get_storage()),
               sockaddr.get_length()));
  if (retval == kSyscallError) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS) {
      return 0;
    }
    throw std::runtime_error(
        std::format("sendto failed: {}", std::strerror(errno)));
  }
  return static_cast<std::size_t>(retval);
}

std::size_t base_socket::receive(std::span<std::byte> bytes) const
    noexcept(false) {
  const auto retval =
      SYSCALL(::recv(*native_handle(), bytes.data(), bytes.size(), 0));
  if (retval == kSyscallError) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    }
    throw std::runtime_error(
        std::format("recv failed: {}", std::strerror(errno)));
  }
  return static_cast<std::size_t>(retval);
}

std::size_t base_socket::receive_from(std::span<std::byte> bytes,
                                      net::base_sockaddr &sockaddr) const
    noexcept(false) {
  ::socklen_t socklen = static_cast<::socklen_t>(sockaddr.get_length());
  const auto retval = SYSCALL(::recvfrom(
      *native_handle(), bytes.data(), bytes.size(), 0,
      reinterpret_cast<::sockaddr *>(sockaddr.get_storage()), &socklen));
  if (retval == kSyscallError) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    }
    throw std::runtime_error(
        std::format("recvfrom failed: {}", std::strerror(errno)));
  }
  return static_cast<std::size_t>(retval);
}

}  // namespace sandbox::net
