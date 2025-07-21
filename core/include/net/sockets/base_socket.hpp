#pragma once

#include <span>

#include "io/fd.hpp"
#include "net/sockets/base_sockaddr.hpp"
#include "net/sockets/family.hpp"
#include "net/sockets/protocol.hpp"
#include "net/sockets/type.hpp"

namespace core::net::sockets {

class base_socket : public io::fd {
 protected:
  base_socket(io::fd::uninitialized_t) noexcept;
  base_socket(net::sockets::family family, net::sockets::type type,
              net::sockets::protocol protocol);

 public:
  void set_nonblock(bool value);
  void set_reuseaddr(bool value);
  void set_reuseport(bool value);
  void set_keepalive(bool value);

 public:
  bool get_nonblock() const;
  bool get_reuseaddr() const;
  bool get_reuseport() const;
  bool get_keepalive() const;
  net::sockets::family get_family() const;
  net::sockets::type get_type() const;

 public:
  enum class bind_status : std::uint8_t {
    kSuccess,
    kInUse,
  };
  bind_status bind(const net::sockets::base_sockaddr& sockaddr);

 public:
  enum class connection_status : std::uint8_t {
    kSuccess,
    kPending,
    kRefused,
  };
  connection_status connect(const net::sockets::base_sockaddr& sockaddr);

 public:
  void get_bind_sockaddr(net::sockets::base_sockaddr& sockaddr) const;
  void get_connect_sockaddr(net::sockets::base_sockaddr& sockaddr) const;

 public:
  void listen(std::size_t backlog);

  enum class accept_status : std::uint8_t {
    kSuccess,
    kEmptyQueue,
  };
  accept_status accept(base_socket& socket) const;

 public:
  std::size_t send(std::span<const std::byte> bytes) const;
  std::size_t send_to(std::span<const std::byte> bytes,
                      const net::sockets::base_sockaddr& sockaddr) const;
  std::size_t receive(std::span<std::byte> bytes) const;
  std::size_t receive_from(std::span<std::byte> bytes,
                           net::sockets::base_sockaddr& sockaddr) const;
};

}  // namespace core::net::sockets
