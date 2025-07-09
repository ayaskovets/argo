#pragma once

#include "net/sockets/base_socket.hpp"

namespace core::net::inet::tcp {

class socket final : public net::sockets::base_socket {
 public:
  socket(io::fd::uninitialized_t) noexcept;
  socket();
};

}  // namespace core::net::inet::tcp
