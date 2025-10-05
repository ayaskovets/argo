#include "net/inet/ip.hpp"

#include <arpa/inet.h>

#include <cerrno>
#include <format>

namespace core::net::inet {

namespace {

constexpr int kInetPtonSuccess = 1;
constexpr auto kInetNtopError = nullptr;

}  // namespace

const ip& ip::kLoopback() noexcept {
  static const ip ip(INADDR_LOOPBACK);
  return ip;
}

const ip& ip::kAny() noexcept {
  static const ip ip(INADDR_ANY);
  return ip;
}

const ip& ip::kBroadcast() noexcept {
  static const ip ip(INADDR_BROADCAST);
  return ip;
}

const ip& ip::kNonRoutable() noexcept {
  static const ip ip("192.0.2.0");
  return ip;
}

ip::ip(std::uint32_t bytes) noexcept : data_(htonl(bytes)) {}

ip::ip(std::uint32_t bytes, network_byte_order_t) noexcept : data_(bytes) {}

ip::ip(std::string_view string) noexcept(false) {
  if (string.size() > INET_ADDRSTRLEN) {
    throw std::invalid_argument(std::format("invalid ip: {}", string));
  }
  if (::inet_pton(AF_INET, string.data(), &data_) != kInetPtonSuccess) {
    throw std::invalid_argument(
        std::format("invalid ip {}: {}", string, std::strerror(errno)));
  }
}

std::uint32_t ip::get_bytes() const noexcept { return ntohl(data_); }

std::uint32_t ip::get_bytes(network_byte_order_t) const noexcept {
  return data_;
}

std::string ip::to_string() const noexcept(false) {
  std::string string;
  string.resize(INET_ADDRSTRLEN);

  if (::inet_ntop(AF_INET, &data_, string.data(), string.size()) !=
      kInetNtopError) [[likely]] {
    string.resize(string.find('\0'));
    return string;
  }

  throw std::runtime_error(
      std::format("could not serialize ip: {}", std::strerror(errno)));
}

}  // namespace core::net::inet
