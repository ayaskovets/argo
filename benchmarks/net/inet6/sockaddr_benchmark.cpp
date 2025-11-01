#include <sandbox/net/inet6/sockaddr.hpp>

#include <benchmark/benchmark.h>

namespace benchmarks::net::inet6 {

namespace inet6 = sandbox::net::inet6;
namespace net = sandbox::net;

constexpr void BM_net_inet6_from_bytes(benchmark::State& state) noexcept {
  const std::array<std::byte, 16> bytes{
      std::byte(255), std::byte(255), std::byte(255), std::byte(255),
      std::byte(255), std::byte(255), std::byte(255), std::byte(255),
      std::byte(255), std::byte(255), std::byte(255), std::byte(255),
      std::byte(255), std::byte(255), std::byte(255), std::byte(255)};
  for (const auto _ : state) {
    benchmark::DoNotOptimize(inet6::sockaddr(
        inet6::ip(std::span(bytes), net::network_byte_order_t{}),
        inet6::port(0)));
  }
}
BENCHMARK(BM_net_inet6_from_bytes);

constexpr void BM_net_inet6_from_string(benchmark::State& state) noexcept {
  const std::string string("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
  for (const auto _ : state) {
    benchmark::DoNotOptimize(
        inet6::sockaddr(inet6::ip(string), inet6::port(0)));
  }
}
BENCHMARK(BM_net_inet6_from_string);

constexpr void BM_net_inet6_to_string(benchmark::State& state) noexcept {
  const inet6::sockaddr sockaddr(
      inet6::ip("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"), inet6::port(0));
  for (const auto _ : state) {
    benchmark::DoNotOptimize(sockaddr.to_string());
  }
}
BENCHMARK(BM_net_inet6_to_string);

}  // namespace benchmarks::net::inet6
