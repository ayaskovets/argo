#include <sandbox/queues/single_threaded_queue.hpp>

#include <benchmark/benchmark.h>

namespace benchmarks::queues {

template <typename Value>
void BM_queues_single_threaded_queue_nonblocking_throughput(
    benchmark::State& state) {
  const std::size_t capacity = state.range(0);
  const std::size_t items = state.range(1);
  const Value value{};

  using allocator_value_t =
      sandbox::queues::detail::single_threaded_queue_entry<Value,
                                                           alignof(Value)>;
  using queue_t = sandbox::queues::single_threaded_queue<
      Value, std::size_t, sandbox::utils::kDynamicExtent<std::size_t>,
      std::allocator<allocator_value_t>>;

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
