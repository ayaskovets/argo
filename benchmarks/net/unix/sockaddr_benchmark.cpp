#include <sandbox/net/unix/sockaddr.hpp>

#include <benchmark/benchmark.h>

namespace benchmarks::net::unix {

namespace unix = sandbox::net::unix;

void BM_net_unix_from_string(benchmark::State& state) {
  const std::string string("BM_net_unix_from_string_unix_sockaddr");
  for (const auto _ : state) {
    benchmark::DoNotOptimize(unix::sockaddr(string));
  }
}
BENCHMARK(BM_net_unix_from_string);

void BM_net_unix_to_string(benchmark::State& state) {
  const unix::sockaddr sockaddr("BM_net_unix_from_string_unix_sockaddr");
  for (const auto _ : state) {
    benchmark::DoNotOptimize(sockaddr.to_string());
  }
}
BENCHMARK(BM_net_unix_to_string);

}  // namespace benchmarks::net::unix
