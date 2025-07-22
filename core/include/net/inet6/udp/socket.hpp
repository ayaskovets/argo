#pragma once

#include "net/sockets/base_socket.hpp"

namespace core::net::inet6::udp {

/**
 * @brief IPv6 UDP socket
 */
class socket final : public net::sockets::base_socket {
 public:
  socket(io::fd::uninitialized_t) noexcept;
  socket();
};

}  // namespace core::net::inet6::udp
