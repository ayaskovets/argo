#pragma once

#include <wip/net/base_sockaddr.hpp>

#include <filesystem>

namespace wip::net::unix {

/**
 * @brief UNIX address
 */
class sockaddr final : public net::base_sockaddr {
 public:
  static const sockaddr& kInvalid() noexcept;
  static const sockaddr& kEmpty() noexcept;

 public:
  explicit sockaddr(const std::filesystem::path& path) noexcept(false);

 public:
  std::filesystem::path get_path() const noexcept;
};

}  // namespace wip::net::unix

template <>
class std::formatter<wip::net::unix::sockaddr> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const wip::net::unix::sockaddr& sockaddr,
                        FormatContext& ctx) const noexcept(false) {
    return std::format_to(ctx.out(), "{}", sockaddr.to_string());
  }
};
