#include <wip/datetime/iso8601.hpp>

#include <benchmark/benchmark.h>

namespace benchmarks::datetime {

namespace datetime = wip::datetime;

constexpr void BM_datetime_to_iso8601(benchmark::State& state) noexcept {
  const auto now = std::chrono::system_clock::now();
  for (const auto _ : state) {
    benchmark::DoNotOptimize(datetime::to_iso8601(now));
  }
}
BENCHMARK(BM_datetime_to_iso8601);

constexpr void BM_datetime_serialize_chrono_with_std_format(
    benchmark::State& state) noexcept {
  const auto now = std::chrono::system_clock::now();
  for (const auto _ : state) {
    benchmark::DoNotOptimize(std::format("{}", now));
  }
}
BENCHMARK(BM_datetime_serialize_chrono_with_std_format);

}  // namespace benchmarks::datetime
