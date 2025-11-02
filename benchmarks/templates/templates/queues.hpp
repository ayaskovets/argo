#pragma once

#include <atomic>
#include <latch>
#include <thread>

#include <benchmark/benchmark.h>

namespace benchmarks::queues::templates {

template <typename Queue>
constexpr void nonblocking_throughput(benchmark::State& state) noexcept {
  const std::size_t capacity = state.range(0);
  const std::size_t items = state.range(1);
  const std::size_t producers = state.range(2);
  const std::size_t consumers = state.range(3);
  const typename Queue::value_type value{};

  std::atomic<std::size_t> try_push_times = 0;
  std::atomic<std::size_t> try_pop_times = 0;

  for (const auto _ : state) {
    state.PauseTiming();

    Queue queue(capacity);
    std::latch latch(producers + consumers + 1);

    std::atomic<std::size_t> pushed_items = 0;
    const auto producer = [&] {
      latch.arrive_and_wait();

      if (producers == 1) {
        for (std::size_t i = 0; i < items;) {
          try_push_times.fetch_add(1, std::memory_order::relaxed);
          i += queue.try_push(value);
        }
      } else {
        while (pushed_items.fetch_add(1, std::memory_order::relaxed) < items) {
          do {
            try_push_times.fetch_add(1, std::memory_order::relaxed);
          } while (!queue.try_push(value));
        }
      }
    };

    std::atomic<std::size_t> popped_items = 0;
    const auto consumer = [&] {
      latch.arrive_and_wait();

      if (consumers == 1) {
        for (std::size_t i = 0; i < items;) {
          typename Queue::value_type out_value;
          try_pop_times.fetch_add(1, std::memory_order::relaxed);
          i += queue.try_pop(out_value);
        }
      } else {
        while (popped_items.fetch_add(1, std::memory_order::relaxed) < items) {
          typename Queue::value_type out_value{};
          do {
            try_pop_times.fetch_add(1, std::memory_order::relaxed);
          } while (!queue.try_pop(out_value));
        }
      }
    };

    std::vector<std::jthread> threads;
    threads.reserve(producers + consumers);
    for (std::size_t i = 0; i < producers; ++i) {
      threads.emplace_back(producer);
    }
    for (std::size_t i = 0; i < consumers; ++i) {
      threads.emplace_back(consumer);
    }

    state.ResumeTiming();
    latch.arrive_and_wait();
  }

  state.counters["good_push_prc"] =
      benchmark::Counter(100.0 * items /
                         (try_push_times.load(std::memory_order::relaxed) /
                          static_cast<double>(state.iterations())));
  state.counters["good_pop_prc"] =
      benchmark::Counter(100.0 * items /
                         (try_pop_times.load(std::memory_order::relaxed) /
                          static_cast<double>(state.iterations())));
}

template <typename Queue>
constexpr void blocking_throughput(benchmark::State& state) {
  const std::size_t capacity = state.range(0);
  const std::size_t items = state.range(1);
  const std::size_t producers = state.range(2);
  const std::size_t consumers = state.range(3);
  const typename Queue::value_type value{};

  for (const auto _ : state) {
    state.PauseTiming();

    Queue queue(capacity);
    std::latch latch(producers + consumers + 1);

    std::atomic<std::size_t> pushed_items = 0;
    const auto producer = [&] {
      latch.arrive_and_wait();

      if (producers == 1) {
        for (std::size_t i = 0; i < items; ++i) {
          queue.push(value);
        }
      } else {
        while (pushed_items.fetch_add(1, std::memory_order::relaxed) < items) {
          queue.push(value);
        }
      }
    };

    std::atomic<std::size_t> popped_items = 0;
    const auto consumer = [&] {
      latch.arrive_and_wait();

      if (consumers == 1) {
        for (std::size_t i = 0; i < items; ++i) {
          queue.pop();
        }
      } else {
        while (popped_items.fetch_add(1, std::memory_order::relaxed) < items) {
          queue.pop();
        }
      }
    };

    std::vector<std::jthread> threads;
    threads.reserve(producers + consumers);
    for (std::size_t i = 0; i < producers; ++i) {
      threads.emplace_back(producer);
    }
    for (std::size_t i = 0; i < consumers; ++i) {
      threads.emplace_back(consumer);
    }

    state.ResumeTiming();
    latch.arrive_and_wait();
  }
}

template <typename Queue>
constexpr void single_threaded_throughput(benchmark::State& state) noexcept {
  const std::size_t capacity = state.range(0);
  const std::size_t items = state.range(1);
  const typename Queue::value_type value{};

  for (const auto _ : state) {
    state.PauseTiming();

    Queue queue(capacity);

    state.ResumeTiming();

    std::size_t pushed_items = 0;
    while (pushed_items < items) {
      const std::size_t available_slots_in_queue =
          std::min(capacity, items - pushed_items);
      for (std::size_t i = 0; i < available_slots_in_queue; ++i) {
        queue.push(value);
      }
      for (std::size_t i = 0; i < capacity; ++i) {
        queue.pop();
      }
      pushed_items += available_slots_in_queue;
    }
  }
}

}  // namespace benchmarks::queues::templates
