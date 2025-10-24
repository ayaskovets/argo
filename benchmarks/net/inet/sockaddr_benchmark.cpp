#include <sandbox/net/inet/sockaddr.hpp>

#include <benchmark/benchmark.h>

namespace benchmarks::net::inet {

namespace inet = sandbox::net::inet;

constexpr void BM_net_inet_sockaddr_from_bytes(
    benchmark::State& state) noexcept {
  const std::uint32_t bytes(0x7f000001);
  for (const auto _ : state) {
    benchmark::DoNotOptimize(inet::sockaddr(inet::ip(bytes), inet::port(0)));
  }
}
BENCHMARK(BM_net_inet_sockaddr_from_bytes);

constexpr void BM_net_inet_sockaddr_from_string(
    benchmark::State& state) noexcept {
  const std::string string("127.0.0.1");
  for (const auto _ : state) {
    benchmark::DoNotOptimize(inet::sockaddr(inet::ip(string), inet::port(0)));
  }
}
BENCHMARK(BM_net_inet_sockaddr_from_string);

constexpr void BM_net_inet_sockaddr_to_string(
    benchmark::State& state) noexcept {
  const inet::sockaddr sockaddr(inet::ip("127.0.0.1"), inet::port(0));
  for (const auto _ : state) {
    benchmark::DoNotOptimize(sockaddr.to_string());
  }
}
BENCHMARK(BM_net_inet_sockaddr_to_string);

}  // namespace benchmarks::net::inet
