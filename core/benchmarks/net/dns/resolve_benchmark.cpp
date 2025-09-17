#include <benchmark/benchmark.h>

#include "net/dns/resolve.hpp"

namespace benchmarks::net::dns {

void BM_net_dns_resolve_localhost(benchmark::State& state) {
  for (const auto _ : state) {
    benchmark::DoNotOptimize(core::net::dns::resolve(
        "localhost", core::net::sockets::family::kUnspecified,
        core::net::sockets::protocol::kTcp));
  }
}
BENCHMARK(BM_net_dns_resolve_localhost)->Unit(benchmark::kMicrosecond);

void BM_net_dns_resolve_google(benchmark::State& state) {
  try {
    core::net::dns::resolve("google.com",
                            core::net::sockets::family::kUnspecified,
                            core::net::sockets::protocol::kTcp);
  } catch (const std::exception& e) {
    state.SkipWithMessage(e.what());
  }

  for (const auto _ : state) {
    benchmark::DoNotOptimize(core::net::dns::resolve(
        "google.com", core::net::sockets::family::kUnspecified,
        core::net::sockets::protocol::kTcp));
  }
}
BENCHMARK(BM_net_dns_resolve_google)->Unit(benchmark::kMicrosecond);

}  // namespace benchmarks::net::dns
