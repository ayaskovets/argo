#pragma once

#include "net/base_socket.hpp"

namespace core::net::inet6::tcp {

/**
 * @brief IPv6 TCP socket
 */
class socket final : public net::base_socket {
 public:
  socket(io::uninitialized_t) noexcept;
  socket() noexcept(false);
};

}  // namespace core::net::inet6::tcp
