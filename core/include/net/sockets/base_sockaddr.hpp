#pragma once

#include <format>
#include <memory>
#include <string>

#include "net/sockets/family.hpp"

namespace core::net::sockets {

class base_socket;
class base_sockaddr {
 private:
  friend class base_socket;

 protected:
  // NOTE: base_sockaddr always allocates enough memory to hold a sockaddr of
  // the provided family. It is the responsibility of the derived class to fill
  // the memory as needed
  explicit base_sockaddr(net::sockets::family family);

 protected:
  // NOTE: get_storage() methods are expected to be used only in the subclasses
  // to wrap access to the allocated storage or for two-step creation of a
  // base_sockaddr object
  class storage;
  storage* get_storage() noexcept;
  const storage* get_storage() const noexcept;

 public:
  bool operator==(const base_sockaddr& that) const;
  bool operator!=(const base_sockaddr& that) const;

 public:
  std::size_t get_length() const;
  net::sockets::family get_family() const;

 public:
  std::string to_string() const;

 private:
  std::shared_ptr<void> storage_;
};

}  // namespace core::net::sockets

template <>
struct std::formatter<core::net::sockets::base_sockaddr> {
  template <class FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <class FormatContext>
  constexpr auto format(const core::net::sockets::base_sockaddr& sockaddr,
                        FormatContext& ctx) const {
    return std::format_to(ctx.out(), "{}", sockaddr.to_string());
  }
};
