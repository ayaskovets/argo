#include <wip/net/dns.hpp>
#include <wip/utils/syscall.hpp>

#include <cerrno>
#include <format>
#include <netdb.h>
#include <span>

namespace wip::net {

namespace {

constexpr int to_native_family(net::family family) noexcept(false) {
  switch (family) {
    case net::family::kUnspecified:
      return AF_UNSPEC;
    case net::family::kInet:
      return AF_INET;
    case net::family::kInet6:
      return AF_INET6;
    case net::family::kUnix:
      throw std::invalid_argument(
          std::format("unsupported family: {}", family));
  }
}

constexpr int to_native_type(net::protocol protocol) noexcept(false) {
  switch (protocol) {
    case net::protocol::kUnspecified:
      return 0;
    case net::protocol::kTcp:
      return SOCK_STREAM;
    case net::protocol::kUdp:
      return SOCK_DGRAM;
  }
}

std::variant<net::inet::sockaddr, net::inet6::sockaddr> to_sockaddr(
    ::addrinfo const & addrinfo) noexcept(false) {
  switch (addrinfo.ai_family) {
    case AF_INET: {
      ::sockaddr_in const * sockaddr =
          reinterpret_cast<::sockaddr_in const *>(addrinfo.ai_addr);

      return net::inet::sockaddr(
          net::inet::ip(static_cast<std::uint32_t>(sockaddr->sin_addr.s_addr),
                        net::network_byte_order_t{}),
          net::inet::port(sockaddr->sin_port, net::network_byte_order_t{}));
    }
    case AF_INET6: {
      ::sockaddr_in6 const * sockaddr =
          reinterpret_cast<::sockaddr_in6 const *>(addrinfo.ai_addr);

      return net::inet6::sockaddr(
          net::inet6::ip(std::span(reinterpret_cast<std::byte const(&)[16]>(
                             sockaddr->sin6_addr)),
                         net::network_byte_order_t{}),
          net::inet6::port(sockaddr->sin6_port, net::network_byte_order_t{}));
    }
    default:
      throw std::runtime_error(
          std::format("unexpected address family {}", addrinfo.ai_family));
  }
}

}  // namespace

std::vector<std::variant<net::inet::sockaddr, net::inet6::sockaddr>> resolve(
    std::string_view hostname, net::family family,
    net::protocol protocol) noexcept(false) {
  std::unique_ptr<::addrinfo, decltype(&SYSCALL(::freeaddrinfo))> results_raii(
      nullptr, SYSCALL(::freeaddrinfo));

  ::addrinfo const hints{
      .ai_family = to_native_family(family),
      .ai_socktype = to_native_type(protocol),
  };

  ::addrinfo * results = nullptr;
  auto const retval =
      SYSCALL(::getaddrinfo(hostname.data(), nullptr, &hints, &results));
  switch (retval) {
    case 0:
      results_raii.reset(results);
      break;
    [[unlikely]] case EAI_SYSTEM:
      throw std::runtime_error(
          std::format("failed to resolve any address at {}: {}", hostname,
                      std::strerror(errno)));
    default:
      throw std::runtime_error(
          std::format("failed to resolve any address at {}: {}", hostname,
                      SYSCALL(::gai_strerror(retval))));
  }

  std::vector<std::variant<net::inet::sockaddr, net::inet6::sockaddr>>
      sockaddrs;
  for (::addrinfo const * ptr = results; ptr; ptr = ptr->ai_next) {
    sockaddrs.emplace_back(to_sockaddr(*ptr));
  }
  return sockaddrs;
}

}  // namespace wip::net
