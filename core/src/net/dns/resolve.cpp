#include "net/dns/resolve.hpp"

#include <netdb.h>

#include <cerrno>
#include <format>
#include <span>

#include "utils/scope.hpp"

namespace core::net::dns {

namespace {

constexpr int to_native_family(net::sockets::family family) {
  switch (family) {
    case net::sockets::family::kUnspecified:
      return AF_UNSPEC;
    case net::sockets::family::kInet:
      return AF_INET;
    case net::sockets::family::kInet6:
      return AF_INET6;
    default:
      throw std::invalid_argument(
          std::format("unsupported family: {}", family));
  }
}

constexpr int to_native_type(net::sockets::protocol protocol) {
  switch (protocol) {
    case net::sockets::protocol::kUnspecified:
      return 0;
    case net::sockets::protocol::kTcp:
      return SOCK_STREAM;
    case net::sockets::protocol::kUdp:
      return SOCK_DGRAM;
    default:
      throw std::invalid_argument(
          std::format("unsupported protocol: {}", protocol));
  }
}

std::variant<net::inet::sockaddr, net::inet6::sockaddr> to_sockaddr(
    const ::addrinfo &addrinfo) {
  switch (addrinfo.ai_family) {
    case AF_INET: {
      const ::sockaddr_in *sockaddr =
          reinterpret_cast<const ::sockaddr_in *>(addrinfo.ai_addr);

      return net::inet::sockaddr(
          net::inet::ip(static_cast<std::uint32_t>(sockaddr->sin_addr.s_addr),
                        net::inet::ip::network_byte_order_t{}),
          net::inet::port(sockaddr->sin_port,
                          net::inet::port::network_byte_order_t{}));
    }
    case AF_INET6: {
      const ::sockaddr_in6 *sockaddr =
          reinterpret_cast<const ::sockaddr_in6 *>(addrinfo.ai_addr);

      return net::inet6::sockaddr(
          net::inet6::ip(std::span(reinterpret_cast<const std::byte(&)[16]>(
                             sockaddr->sin6_addr)),
                         net::inet6::ip::network_byte_order_t{}),
          net::inet6::port(sockaddr->sin6_port,
                           net::inet6::port::network_byte_order_t{}));
    }
    default:
      throw std::runtime_error(
          std::format("unexpected address family {}", addrinfo.ai_family));
  }
}

}  // namespace

std::vector<std::variant<net::inet::sockaddr, net::inet6::sockaddr>> resolve(
    std::string_view hostname, net::sockets::family family,
    net::sockets::protocol protocol) {
  std::unique_ptr<::addrinfo, decltype(&::freeaddrinfo)> results_raii(
      nullptr, ::freeaddrinfo);

  const ::addrinfo hints{
      .ai_family = to_native_family(family),
      .ai_socktype = to_native_type(protocol),
  };

  ::addrinfo *results = nullptr;
  switch (const int error =
              ::getaddrinfo(hostname.data(), nullptr, &hints, &results)) {
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
                      ::gai_strerror(error)));
  }

  std::vector<std::variant<net::inet::sockaddr, net::inet6::sockaddr>>
      sockaddrs;
  for (const ::addrinfo *ptr = results; ptr; ptr = ptr->ai_next) {
    sockaddrs.emplace_back(to_sockaddr(*ptr));
  }
  return sockaddrs;
}

}  // namespace core::net::dns
