#include "net/inet6/sockaddr.hpp"

#include <arpa/inet.h>

namespace core::net::inet6 {

sockaddr::sockaddr(net::inet6::ip ip, net::inet6::port port)
    : net::base_sockaddr(net::sockets::family::kInet6) {
  ::sockaddr_in6* storage = reinterpret_cast<::sockaddr_in6*>(get_storage());
  std::memcpy(&storage->sin6_addr,
              ip.get_bytes(net::network_byte_order_t{}).data(),
              sizeof(storage->sin6_addr));
  storage->sin6_port = port.get_bytes(net::network_byte_order_t{});
}

net::inet6::ip sockaddr::get_ip() const noexcept {
  const ::sockaddr_in6* storage =
      reinterpret_cast<const ::sockaddr_in6*>(get_storage());
  return net::inet6::ip(
      std::span(reinterpret_cast<const std::byte(&)[16]>(storage->sin6_addr)),
      net::network_byte_order_t{});
}

net::inet6::port sockaddr::get_port() const noexcept {
  const ::sockaddr_in6* storage =
      reinterpret_cast<const ::sockaddr_in6*>(get_storage());
  return net::inet6::port(storage->sin6_port, net::network_byte_order_t{});
}

}  // namespace core::net::inet6
