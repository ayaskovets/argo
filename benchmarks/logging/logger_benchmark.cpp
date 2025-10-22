#include <sandbox/logging/logging.hpp>

#include <benchmark/benchmark.h>

namespace benchmarks::logging {

void BM_logging_default(benchmark::State& state) {
  (void)std::format("{}", 42);
  for (const auto _ : state) {
    benchmark::DoNotOptimize((sandbox::logging::info("{}\n", 42), 42));
  }
}
BENCHMARK(BM_logging_default);

void BM_logging_transparent(benchmark::State& state) {
  const sandbox::logging::logger logger(sandbox::logging::level::kInfo,
                                        sandbox::logging::renderer::kNoop(),
                                        sandbox::logging::writer::kStderr());
  for (const auto _ : state) {
    benchmark::DoNotOptimize(
        (logger(sandbox::logging::level::kInfo, "{}\n", 42), 42));
  }
}
BENCHMARK(BM_logging_transparent);

void BM_logging_fprintf(benchmark::State& state) {
  for (const auto _ : state) {
    benchmark::DoNotOptimize(fprintf(stderr, "%d\n", 42));
  }
}
BENCHMARK(BM_logging_fprintf);

void BM_logging_fputs(benchmark::State& state) {
  for (const auto _ : state) {
    benchmark::DoNotOptimize(fputs("42\n", stderr));
  }
}
BENCHMARK(BM_logging_fputs);

}  // namespace benchmarks::logging
