#pragma once

#include <wip/net/base_socket.hpp>

namespace wip::net::inet::tcp {

/**
 * @brief IPv4 TCP socket
 */
class socket final : public net::base_socket {
 public:
  socket(io::uninitialized_t) noexcept;
  socket() noexcept(false);
};

}  // namespace wip::net::inet::tcp
