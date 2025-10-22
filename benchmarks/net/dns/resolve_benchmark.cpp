#include <sandbox/net/dns.hpp>

#include <benchmark/benchmark.h>

namespace benchmarks::net::dns {

void BM_net_dns_resolve_localhost(benchmark::State& state) {
  for (const auto _ : state) {
    benchmark::DoNotOptimize(
        sandbox::net::resolve("localhost", sandbox::net::family::kUnspecified,
                              sandbox::net::protocol::kTcp));
  }
}
BENCHMARK(BM_net_dns_resolve_localhost)->Unit(benchmark::kMicrosecond);

void BM_net_dns_resolve_google(benchmark::State& state) {
  try {
    sandbox::net::resolve("google.com", sandbox::net::family::kUnspecified,
                          sandbox::net::protocol::kTcp);
  } catch (const std::exception& e) {
    state.SkipWithMessage(e.what());
  }

  for (const auto _ : state) {
    benchmark::DoNotOptimize(
        sandbox::net::resolve("google.com", sandbox::net::family::kUnspecified,
                              sandbox::net::protocol::kTcp));
  }
}
BENCHMARK(BM_net_dns_resolve_google)->Unit(benchmark::kMicrosecond);

}  // namespace benchmarks::net::dns
