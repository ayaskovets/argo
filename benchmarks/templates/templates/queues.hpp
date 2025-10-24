#pragma once

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
  const typename Queue::value_t value{};

  std::atomic<std::size_t> try_push_times = 0;
  std::atomic<std::size_t> try_pop_times = 0;

  for (const auto _ : state) {
    state.PauseTiming();

    Queue queue(capacity);
    std::latch latch(producers + consumers + 1);

    std::atomic<std::size_t> pushed_items_count = 0;
    const auto producer = [&] {
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

    std::atomic<std::size_t> popped_items_count = 0;
    const auto consumer = [&] {
      if (consumers == 1) {
        latch.arrive_and_wait();
        for (std::size_t popped_items_count = 0; popped_items_count < items;) {
          typename Queue::value_t value;
          try_pop_times.fetch_add(1, std::memory_order::relaxed);
          popped_items_count += queue.try_pop(value);
        }
      } else {
        latch.arrive_and_wait();
        while (popped_items_count.fetch_add(1, std::memory_order::relaxed) <
               items) {
          typename Queue::value_t value{};
          do {
            try_pop_times.fetch_add(1, std::memory_order::relaxed);
          } while (!queue.try_pop(value));
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
  const typename Queue::value_t value{};

  for (const auto _ : state) {
    state.PauseTiming();

    Queue queue(capacity);
    std::latch latch(producers + consumers + 1);

    std::atomic<std::size_t> pushed_items_count = 0;
    const auto producer = [&] {
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

    std::atomic<std::size_t> popped_items_count = 0;
    const auto consumer = [&] {
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
constexpr void single_threaded_nonblocking_throughput(
    benchmark::State& state) noexcept {
  const std::size_t capacity = state.range(0);
  const std::size_t items = state.range(1);
  const typename Queue::value_t value{};

  for (const auto _ : state) {
    state.PauseTiming();

    Queue queue(capacity);

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

}  // namespace benchmarks::queues::templates
