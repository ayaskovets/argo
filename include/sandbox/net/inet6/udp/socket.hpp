#pragma once

#include <sandbox/net/base_socket.hpp>

namespace sandbox::net::inet6::udp {

/**
 * @brief IPv6 UDP socket
 */
class socket final : public net::base_socket {
 public:
  socket(io::uninitialized_t) noexcept;
  socket() noexcept(false);
};

}  // namespace sandbox::net::inet6::udp
