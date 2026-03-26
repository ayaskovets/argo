#include <wip/net/inet/ip.hpp>
#include <wip/utils/syscall.hpp>

#include <arpa/inet.h>
#include <cerrno>
#include <format>

namespace wip::net::inet {

ip const & ip::kLoopback() noexcept {
  static ip const ip(INADDR_LOOPBACK);
  return ip;
}

ip const & ip::kAny() noexcept {
  static ip const ip(INADDR_ANY);
  return ip;
}

ip const & ip::kBroadcast() noexcept {
  static ip const ip(INADDR_BROADCAST);
  return ip;
}

ip const & ip::kNonRoutable() noexcept {
  static ip const ip("192.0.2.0");  ///< TEST-NET-1
  return ip;
}

ip::ip(std::uint32_t bytes) noexcept : data_(htonl(bytes)) {}

ip::ip(std::uint32_t bytes, network_byte_order_t) noexcept : data_(bytes) {}

ip::ip(std::string_view string) noexcept(false) {
  constexpr int kInetPtonSuccess = 1;

  if (string.size() > INET_ADDRSTRLEN) {
    throw std::invalid_argument(std::format("invalid ip: {}", string));
  }
  if (SYSCALL(::inet_pton(AF_INET, string.data(), &data_)) != kInetPtonSuccess)
      [[unlikely]] {
    throw std::invalid_argument(
        std::format("invalid ip {}: {}", string, std::strerror(errno)));
  }
}

std::uint32_t ip::get_bytes() const noexcept { return ntohl(data_); }

std::uint32_t ip::get_bytes(network_byte_order_t) const noexcept {
  return data_;
}

std::string ip::to_string() const noexcept(false) {
  constexpr auto kInetNtopError = nullptr;

  std::string string;
  string.resize(INET_ADDRSTRLEN);

  if (SYSCALL(::inet_ntop(AF_INET, &data_, string.data(), string.size())) !=
      kInetNtopError) [[likely]] {
    string.resize(string.find('\0'));
    return string;
  }

  throw std::runtime_error(
      std::format("could not serialize ip: {}", std::strerror(errno)));
}

}  // namespace wip::net::inet
