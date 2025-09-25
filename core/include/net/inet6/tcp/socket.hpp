#pragma once

#include "net/base_socket.hpp"

namespace core::net::inet6::tcp {

/**
 * @brief IPv6 TCP socket
 */
class socket final : public net::base_socket {
 public:
  socket(io::fd::uninitialized_t) noexcept;
  socket();
};

}  // namespace core::net::inet6::tcp
