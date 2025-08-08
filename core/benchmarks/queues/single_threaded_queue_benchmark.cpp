#include <benchmark/benchmark.h>

#include "queues/single_threaded_queue.hpp"

namespace benchmarks::queues {

template <typename Value>
void BM_queues_single_threaded_queue_nonblocking_throughput(
    benchmark::State& state) {
  const std::size_t capacity = state.range(0);
  const std::size_t items = state.range(1);
  const Value value{};

  using entry_t =
      core::queues::detail::single_threaded_queue_entry_impl<Value,
                                                             alignof(Value)>;
  using queue_t = core::queues::single_threaded_queue<
      Value, std::size_t, core::utils::kRuntimeDefinedCapacity<std::size_t>,
      std::allocator<entry_t>>;

  for (const auto _ : state) {
    state.PauseTiming();

    queue_t queue(capacity);

    state.ResumeTiming();

    for (std::size_t pushed_items_count = 0; pushed_items_count < items;) {
      for (std::size_t i = 0; i < capacity && pushed_items_count < items;
           ++i, ++pushed_items_count) {
        queue.push(value);
      }
      for (std::size_t i = 0; i < capacity; ++i) {
        queue.pop();
      }
    }
  }
}
BENCHMARK_TEMPLATE(BM_queues_single_threaded_queue_nonblocking_throughput, int)
    ->Args({1024 /* capacity */, 1048576 /* items*/})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_queues_single_threaded_queue_nonblocking_throughput,
                   std::shared_ptr<int>)
    ->Args({1024 /* capacity */, 1048576 /* items*/})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

}  // namespace benchmarks::queues
