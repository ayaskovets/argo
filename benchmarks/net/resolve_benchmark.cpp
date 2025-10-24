#include <sandbox/net/dns.hpp>

#include <benchmark/benchmark.h>

namespace benchmarks::net {

namespace net = sandbox::net;

constexpr void BM_net_dns_resolve_localhost(benchmark::State& state) noexcept {
  for (const auto _ : state) {
    benchmark::DoNotOptimize(net::resolve(
        "localhost", net::family::kUnspecified, net::protocol::kTcp));
  }
}
BENCHMARK(BM_net_dns_resolve_localhost)->Unit(benchmark::kMicrosecond);

constexpr void BM_net_dns_resolve_google(benchmark::State& state) noexcept {
  try {
    net::resolve("google.com", net::family::kUnspecified, net::protocol::kTcp);
  } catch (const std::exception& e) {
    state.SkipWithMessage(e.what());
  }

  for (const auto _ : state) {
    benchmark::DoNotOptimize(net::resolve(
        "google.com", net::family::kUnspecified, net::protocol::kTcp));
  }
}
BENCHMARK(BM_net_dns_resolve_google)->Unit(benchmark::kMicrosecond);

}  // namespace benchmarks::net
