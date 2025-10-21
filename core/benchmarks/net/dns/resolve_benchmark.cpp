#include <benchmark/benchmark.h>

#include "net/dns.hpp"

namespace benchmarks::net::dns {

void BM_net_dns_resolve_localhost(benchmark::State& state) {
  for (const auto _ : state) {
    benchmark::DoNotOptimize(core::net::resolve("localhost",
                                                core::net::family::kUnspecified,
                                                core::net::protocol::kTcp));
  }
}
BENCHMARK(BM_net_dns_resolve_localhost)->Unit(benchmark::kMicrosecond);

void BM_net_dns_resolve_google(benchmark::State& state) {
  try {
    core::net::resolve("google.com", core::net::family::kUnspecified,
                       core::net::protocol::kTcp);
  } catch (const std::exception& e) {
    state.SkipWithMessage(e.what());
  }

  for (const auto _ : state) {
    benchmark::DoNotOptimize(core::net::resolve("google.com",
                                                core::net::family::kUnspecified,
                                                core::net::protocol::kTcp));
  }
}
BENCHMARK(BM_net_dns_resolve_google)->Unit(benchmark::kMicrosecond);

}  // namespace benchmarks::net::dns
