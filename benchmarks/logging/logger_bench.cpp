#include <wip/logging/logging.hpp>

#include <benchmark/benchmark.h>

namespace benchmarks::logging {

namespace logging = wip::logging;

constexpr void BM_logging_default(benchmark::State& state) noexcept {
  (void)std::format("{}", 42);
  for (const auto _ : state) {
    benchmark::DoNotOptimize((logging::info("{}\n", 42), 42));
  }
}
BENCHMARK(BM_logging_default);

constexpr void BM_logging_transparent(benchmark::State& state) noexcept {
  const logging::logger logger(logging::level::kInfo,
                               logging::renderer::kNoop(),
                               logging::writer::kStderr());
  for (const auto _ : state) {
    benchmark::DoNotOptimize((logger(logging::level::kInfo, "{}\n", 42), 42));
  }
}
BENCHMARK(BM_logging_transparent);

constexpr void BM_logging_fprintf(benchmark::State& state) noexcept {
  for (const auto _ : state) {
    benchmark::DoNotOptimize(fprintf(stderr, "%d\n", 42));
  }
}
BENCHMARK(BM_logging_fprintf);

constexpr void BM_logging_fputs(benchmark::State& state) noexcept {
  for (const auto _ : state) {
    benchmark::DoNotOptimize(fputs("42\n", stderr));
  }
}
BENCHMARK(BM_logging_fputs);

}  // namespace benchmarks::logging
