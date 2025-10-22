#include <sandbox/net/inet6/sockaddr.hpp>

#include <benchmark/benchmark.h>

namespace benchmarks::net::inet6 {

void BM_net_inet6_from_bytes(benchmark::State& state) {
  const std::array<std::byte, 16> bytes{
      std::byte(255), std::byte(255), std::byte(255), std::byte(255),
      std::byte(255), std::byte(255), std::byte(255), std::byte(255),
      std::byte(255), std::byte(255), std::byte(255), std::byte(255),
      std::byte(255), std::byte(255), std::byte(255), std::byte(255)};
  for (const auto _ : state) {
    benchmark::DoNotOptimize(sandbox::net::inet6::sockaddr(
        sandbox::net::inet6::ip(std::span(bytes),
                                sandbox::net::network_byte_order_t{}),
        sandbox::net::inet::port(0)));
  }
}
BENCHMARK(BM_net_inet6_from_bytes);

void BM_net_inet6_from_string(benchmark::State& state) {
  const std::string string("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
  for (const auto _ : state) {
    benchmark::DoNotOptimize(sandbox::net::inet6::sockaddr(
        sandbox::net::inet6::ip(string), sandbox::net::inet::port(0)));
  }
}
BENCHMARK(BM_net_inet6_from_string);

void BM_net_inet6_to_string(benchmark::State& state) {
  const sandbox::net::inet6::sockaddr sockaddr(
      sandbox::net::inet6::ip("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"),
      sandbox::net::inet::port(0));
  for (const auto _ : state) {
    benchmark::DoNotOptimize(sockaddr.to_string());
  }
}
BENCHMARK(BM_net_inet6_to_string);

}  // namespace benchmarks::net::inet6
