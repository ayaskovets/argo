#pragma once

#include "net/base_socket.hpp"

namespace core::net::inet::udp {

/**
 * @brief IPv4 UDP socket
 */
class socket final : public net::base_socket {
 public:
  socket(io::fd::uninitialized_t) noexcept;
  socket() noexcept(false);
};

}  // namespace core::net::inet::udp
