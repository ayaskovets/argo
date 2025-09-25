#include <benchmark/benchmark.h>

#include "net/inet6/sockaddr.hpp"

namespace benchmarks::net::inet6 {

void BM_net_inet6_from_bytes(benchmark::State& state) {
  const std::array<std::byte, 16> bytes{
      std::byte(255), std::byte(255), std::byte(255), std::byte(255),
      std::byte(255), std::byte(255), std::byte(255), std::byte(255),
      std::byte(255), std::byte(255), std::byte(255), std::byte(255),
      std::byte(255), std::byte(255), std::byte(255), std::byte(255)};
  for (const auto _ : state) {
    benchmark::DoNotOptimize(core::net::inet6::sockaddr(
        core::net::inet6::ip(std::span(bytes),
                             core::net::network_byte_order_t{}),
        core::net::inet::port(0)));
  }
}
BENCHMARK(BM_net_inet6_from_bytes);

void BM_net_inet6_from_string(benchmark::State& state) {
  const std::string string("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
  for (const auto _ : state) {
    benchmark::DoNotOptimize(core::net::inet6::sockaddr(
        core::net::inet6::ip(string), core::net::inet::port(0)));
  }
}
BENCHMARK(BM_net_inet6_from_string);

void BM_net_inet6_to_string(benchmark::State& state) {
  const core::net::inet6::sockaddr sockaddr(
      core::net::inet6::ip("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"),
      core::net::inet::port(0));
  for (const auto _ : state) {
    benchmark::DoNotOptimize(sockaddr.to_string());
  }
}
BENCHMARK(BM_net_inet6_to_string);

}  // namespace benchmarks::net::inet6
