#include <benchmark/benchmark.h>

#include <latch>
#include <thread>

#include "queues/locked_mpmc_queue.hpp"

namespace benchmarks::queues {

template <typename Value>
void BM_queues_locked_mpmc_queue_nonblocking_throughput(
    benchmark::State& state) {
  const std::size_t capacity = state.range(0);
  const std::size_t items = state.range(1);
  const std::size_t producers = state.range(2);
  const std::size_t consumers = state.range(3);
  const Value value{};

  class entry_t final {
   private:
    Value value_;

   public:
    constexpr Value& value() { return value_; };
  };
  using queue_t = core::queues::locked_mpmc_queue<
      Value, std::size_t, core::utils::kRuntimeDefinedCapacity<std::size_t>,
      std::allocator<entry_t>>;

  std::atomic<std::size_t> try_push_times = 0;
  std::atomic<std::size_t> try_pop_times = 0;

  for (const auto _ : state) {
    state.PauseTiming();

    queue_t queue(capacity);

    std::latch latch(producers + consumers + 1);
    std::vector<std::thread> threads;
    threads.reserve(producers + consumers);

    std::atomic<std::size_t> pushed_items_count = 0;
    std::atomic<std::size_t> popped_items_count = 0;

    const auto producer = [&latch, &queue, items, producers,
                           &pushed_items_count, &try_push_times,
                           value = value] {
      if (producers == 1) {
        latch.arrive_and_wait();
        for (std::size_t pushed_items_count = 0; pushed_items_count < items;) {
          try_push_times.fetch_add(1, std::memory_order::relaxed);
          pushed_items_count += queue.try_push(value);
        }
      } else {
        latch.arrive_and_wait();
        while (pushed_items_count.fetch_add(1, std::memory_order::relaxed) <
               items) {
          do {
            try_push_times.fetch_add(1, std::memory_order::relaxed);
          } while (!queue.try_push(value));
        }
      }
    };

    const auto consumer = [&latch, &queue, items, consumers,
                           &popped_items_count, &try_pop_times] {
      if (consumers == 1) {
        latch.arrive_and_wait();
        for (std::size_t popped_items_count = 0; popped_items_count < items;) {
          Value value;
          try_pop_times.fetch_add(1, std::memory_order::relaxed);
          popped_items_count += queue.try_pop(value);
        }
      } else {
        latch.arrive_and_wait();
        while (popped_items_count.fetch_add(1, std::memory_order::relaxed) <
               items) {
          Value value{};
          do {
            try_pop_times.fetch_add(1, std::memory_order::relaxed);
          } while (!queue.try_pop(value));
        }
      }
    };

    for (std::size_t i = 0; i < producers; ++i) {
      threads.emplace_back(producer);
    }

    for (std::size_t i = 0; i < consumers; ++i) {
      threads.emplace_back(consumer);
    }

    state.ResumeTiming();

    latch.arrive_and_wait();
    for (auto& thread : threads) {
      thread.join();
    }
  }

  state.counters["successful_push_ratio"] = benchmark::Counter(
      items / (try_push_times.load(std::memory_order::relaxed) /
               static_cast<double>(state.iterations())));
  state.counters["successful_pop_ratio"] = benchmark::Counter(
      items / (try_pop_times.load(std::memory_order::relaxed) /
               static_cast<double>(state.iterations())));
}
BENCHMARK_TEMPLATE(BM_queues_locked_mpmc_queue_nonblocking_throughput, int)
    ->Args({1023 /* capacity */, 1048576 /* items*/, 1 /* producers*/,
            1 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 2 /* producers*/,
            2 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 1 /* producers*/,
            4 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 4 /* producers*/,
            1 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 4 /* producers*/,
            4 /* consumers*/})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_queues_locked_mpmc_queue_nonblocking_throughput,
                   std::shared_ptr<int>)
    ->Args({1023 /* capacity */, 1048576 /* items*/, 1 /* producers*/,
            1 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 2 /* producers*/,
            2 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 1 /* producers*/,
            4 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 4 /* producers*/,
            1 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 4 /* producers*/,
            4 /* consumers*/})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

template <typename Value>
void BM_queues_locked_mpmc_queue_blocking_throughput(benchmark::State& state) {
  const std::size_t capacity = state.range(0);
  const std::size_t items = state.range(1);
  const std::size_t producers = state.range(2);
  const std::size_t consumers = state.range(3);
  const Value value{};

  class entry_t final {
   private:
    Value value_;

   public:
    constexpr Value& value() { return value_; };
  };
  using queue_t = core::queues::locked_mpmc_queue<
      Value, std::size_t, core::utils::kRuntimeDefinedCapacity<std::size_t>,
      std::allocator<entry_t>>;

  for (const auto _ : state) {
    state.PauseTiming();

    queue_t queue(capacity);

    std::latch latch(producers + consumers + 1);
    std::vector<std::thread> threads;
    threads.reserve(producers + consumers);

    std::atomic<std::size_t> pushed_items_count = 0;
    std::atomic<std::size_t> popped_items_count = 0;

    const auto producer = [&latch, &queue, items, producers,
                           &pushed_items_count, value = value] {
      if (producers == 1) {
        latch.arrive_and_wait();
        for (std::size_t pushed_items_count = 0; pushed_items_count < items;
             ++pushed_items_count) {
          queue.push(value);
        }
      } else {
        latch.arrive_and_wait();
        while (pushed_items_count.fetch_add(1, std::memory_order::relaxed) <
               items) {
          queue.push(value);
        }
      }
    };

    const auto consumer = [&latch, &queue, items, consumers,
                           &popped_items_count] {
      if (consumers == 1) {
        latch.arrive_and_wait();
        for (std::size_t popped_items_count = 0; popped_items_count < items;
             ++popped_items_count) {
          queue.pop();
        }
      } else {
        latch.arrive_and_wait();
        while (popped_items_count.fetch_add(1, std::memory_order::relaxed) <
               items) {
          queue.pop();
        }
      }
    };

    for (std::size_t i = 0; i < producers; ++i) {
      threads.emplace_back(producer);
    }

    for (std::size_t i = 0; i < consumers; ++i) {
      threads.emplace_back(consumer);
    }

    state.ResumeTiming();

    latch.arrive_and_wait();
    for (auto& thread : threads) {
      thread.join();
    }
  }
}
BENCHMARK_TEMPLATE(BM_queues_locked_mpmc_queue_blocking_throughput, int)
    ->Args({1023 /* capacity */, 1048576 /* items*/, 1 /* producers*/,
            1 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 2 /* producers*/,
            2 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 1 /* producers*/,
            4 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 4 /* producers*/,
            1 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 4 /* producers*/,
            4 /* consumers*/})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_queues_locked_mpmc_queue_blocking_throughput,
                   std::shared_ptr<int>)
    ->Args({1023 /* capacity */, 1048576 /* items*/, 1 /* producers*/,
            1 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 2 /* producers*/,
            2 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 1 /* producers*/,
            4 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 4 /* producers*/,
            1 /* consumers*/})
    ->Args({1023 /* capacity */, 1048576 /* items*/, 4 /* producers*/,
            4 /* consumers*/})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

}  // namespace benchmarks::queues
