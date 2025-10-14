#pragma once

#include "net/base_socket.hpp"

namespace core::net::inet::tcp {

/**
 * @brief IPv4 TCP socket
 */
class socket final : public net::base_socket {
 public:
  socket(io::uninitialized_t) noexcept;
  socket() noexcept(false);
};

}  // namespace core::net::inet::tcp
