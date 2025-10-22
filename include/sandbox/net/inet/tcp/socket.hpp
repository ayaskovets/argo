#pragma once

#include <sandbox/net/base_socket.hpp>

namespace sandbox::net::inet::tcp {

/**
 * @brief IPv4 TCP socket
 */
class socket final : public net::base_socket {
 public:
  socket(io::uninitialized_t) noexcept;
  socket() noexcept(false);
};

}  // namespace sandbox::net::inet::tcp
