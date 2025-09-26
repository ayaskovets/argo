#pragma once

#include "net/base_sockaddr.hpp"

namespace core::net::unix {

/**
 * @brief UNIX address
 */
class sockaddr final : public net::base_sockaddr {
 public:
  static const sockaddr& kInvalid() noexcept;
  static const sockaddr& kEmpty() noexcept;

 public:
  explicit sockaddr(std::string_view path);

 public:
  std::string_view get_path() const noexcept;
};

}  // namespace core::net::unix

template <>
class std::formatter<core::net::unix::sockaddr> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const core::net::unix::sockaddr& sockaddr,
                        FormatContext& ctx) const {
    return std::format_to(ctx.out(), "{}", sockaddr.to_string());
  }
};
