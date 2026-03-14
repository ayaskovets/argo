#pragma once

#include <wip/net/base_socket.hpp>
#include <wip/net/unix/sockaddr.hpp>

namespace wip::net::unix {

/**
 * @brief UNIX socket base class
 */
class base_socket : public net::base_socket {
 protected:
  explicit base_socket(io::uninitialized_t) noexcept;
  explicit base_socket(wip::net::socktype type) noexcept(false);
  ~base_socket() noexcept;

 public:
  void unlink_close() noexcept(false);

 public:
  /**
   * @warning Bind for unix socket with SO_REUSEADDR enabled does smart unlink
   * in case address is already in use
   */
  net::bind_status bind(const net::unix::sockaddr& sockaddr) noexcept(false);
  net::bind_status unlink_bind(const net::unix::sockaddr& sockaddr) noexcept(
      false);
};

}  // namespace wip::net::unix
