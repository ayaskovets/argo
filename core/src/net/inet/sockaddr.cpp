#include "net/inet/sockaddr.hpp"

#include <arpa/inet.h>

namespace core::net::inet {

sockaddr::sockaddr(net::inet::ip ip, net::inet::port port) noexcept(false)
    : net::base_sockaddr(net::family::kInet) {
  ::sockaddr_in* storage = reinterpret_cast<::sockaddr_in*>(get_storage());
  storage->sin_addr.s_addr = ip.get_bytes(net::network_byte_order_t{});
  storage->sin_port = port.get_bytes(net::network_byte_order_t{});
}

net::inet::ip sockaddr::get_ip() const noexcept {
  const ::sockaddr_in* storage =
      reinterpret_cast<const ::sockaddr_in*>(get_storage());
  return net::inet::ip(storage->sin_addr.s_addr, net::network_byte_order_t{});
}

net::inet::port sockaddr::get_port() const noexcept {
  const ::sockaddr_in* storage =
      reinterpret_cast<const ::sockaddr_in*>(get_storage());
  return net::inet::port(storage->sin_port, net::network_byte_order_t{});
}

}  // namespace core::net::inet
