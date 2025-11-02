#pragma once

#include <sandbox/utils/leak_detector_allocator.hpp>

#include <algorithm>
#include <latch>
#include <numeric>
#include <thread>

#include <gtest/gtest.h>

namespace tests::queues::templates {

namespace utils = sandbox::utils;

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, int>)
constexpr void minimal_capacity(std::size_t capacity) noexcept {
  for (std::size_t i = 0; i < capacity; ++i) {
    EXPECT_ANY_THROW((Queue(i)));
  }
  EXPECT_NO_THROW((Queue(capacity)));
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, int>)
constexpr void nonblocking_smoke() noexcept {
  int value;
  Queue queue(2);

  EXPECT_FALSE(queue.try_pop(value));
  EXPECT_TRUE(queue.try_push(1));
  EXPECT_TRUE(queue.try_push(2));
  EXPECT_FALSE(queue.try_push(3));
  EXPECT_TRUE(queue.try_pop(value));
  EXPECT_EQ(value, 1);
  EXPECT_TRUE(queue.try_pop(value));
  EXPECT_EQ(value, 2);
  EXPECT_FALSE(queue.try_pop(value));
  EXPECT_TRUE(queue.try_push(4));
  EXPECT_TRUE(queue.try_pop(value));
  EXPECT_EQ(value, 4);
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, int>)
constexpr void blocking_smoke() noexcept {
  Queue queue(2);

  queue.push(1);
  queue.push(2);
  EXPECT_EQ(queue.pop(), 1);
  EXPECT_EQ(queue.pop(), 2);
  queue.push(3);
  EXPECT_EQ(queue.pop(), 3);
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, int>)
constexpr void mixed_smoke() noexcept {
  int value;
  Queue queue(2);

  EXPECT_EQ(queue.size(), 0);
  EXPECT_FALSE(queue.try_pop(value));
  EXPECT_EQ(queue.size(), 0);
  queue.push(1);
  EXPECT_EQ(queue.size(), 1);
  queue.push(2);
  EXPECT_EQ(queue.size(), 2);
  EXPECT_FALSE(queue.try_emplace(3));
  EXPECT_EQ(queue.size(), 2);
  EXPECT_EQ(queue.pop(), 1);
  EXPECT_EQ(queue.size(), 1);
  queue.emplace(4);
  EXPECT_EQ(queue.size(), 2);
  EXPECT_FALSE(queue.try_push(5));
  EXPECT_EQ(queue.size(), 2);
  EXPECT_EQ(queue.pop(), 2);
  EXPECT_EQ(queue.size(), 1);
  EXPECT_EQ(queue.pop(), 4);
  EXPECT_EQ(queue.size(), 0);
  EXPECT_FALSE(queue.try_pop(value));
  EXPECT_EQ(queue.size(), 0);
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, int>)
constexpr void blocking_push() noexcept {
  Queue queue(2);

  queue.push(1);
  queue.push(2);

  const std::jthread consumer([&queue] {
    int value;
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 1);
  });

  queue.push(3);
  EXPECT_EQ(queue.pop(), 2);
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, int>)
constexpr void blocking_pop() noexcept {
  Queue queue(2);

  const std::jthread producer([&queue] { EXPECT_TRUE(queue.try_push(42)); });

  EXPECT_EQ(queue.pop(), 42);
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, int>)
constexpr void capacity_one() noexcept {
  int value;
  Queue queue(1);

  EXPECT_FALSE(queue.try_pop(value));
  queue.push(1);
  EXPECT_FALSE(queue.try_push(3));
  EXPECT_EQ(queue.pop(), 1);
  queue.push(4);
  EXPECT_FALSE(queue.try_push(5));
  EXPECT_EQ(queue.pop(), 4);
  EXPECT_FALSE(queue.try_pop(value));
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, std::shared_ptr<int>>)
constexpr void queue_destructor() noexcept {
  std::shared_ptr<int> value = std::make_shared<int>();
  {
    Queue queue(2);

    EXPECT_EQ(value.use_count(), 1);
    queue.push(value);
    EXPECT_EQ(value.use_count(), 2);
    queue.push(value);
    EXPECT_EQ(value.use_count(), 3);
  }
  EXPECT_EQ(value.use_count(), 1);
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, std::shared_ptr<int>>)
constexpr void item_destructor() noexcept {
  std::shared_ptr<int> value = std::make_shared<int>();
  Queue queue(2);

  EXPECT_EQ(value.use_count(), 1);
  queue.push(value);
  EXPECT_EQ(value.use_count(), 2);
  queue.push(value);
  EXPECT_EQ(value.use_count(), 3);
  queue.pop();
  EXPECT_EQ(value.use_count(), 2);
  queue.push(value);
  EXPECT_EQ(value.use_count(), 3);
  queue.pop();
  EXPECT_EQ(value.use_count(), 2);
  queue.pop();
  EXPECT_EQ(value.use_count(), 1);
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, std::unique_ptr<int>>)
constexpr void non_copyable_item_type() noexcept {
  std::unique_ptr<int> value;
  Queue queue(2);

  queue.push(std::move(value));
  queue.pop();
  queue.try_push(std::move(value));
  queue.try_pop(value);
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::allocator_type,
                          utils::leak_detector_allocator<
                              typename Queue::allocator_value_type, true>>)
constexpr void allocator() noexcept {
  typename Queue::allocator_type allocator;
  Queue::allocator_type::clear();

  EXPECT_TRUE(allocator.is_clean());
  {
    Queue queue(allocator);
    EXPECT_EQ(queue.get_allocator(), allocator);

    queue.push(1.1);
    EXPECT_FALSE(allocator.is_clean());
    queue.push(2.2);
    EXPECT_FALSE(allocator.is_clean());
    EXPECT_EQ(queue.pop(), 1.1);
    EXPECT_FALSE(allocator.is_clean());
    EXPECT_EQ(queue.pop(), 2.2);
    EXPECT_FALSE(allocator.is_clean());
  }
  EXPECT_TRUE(allocator.is_clean());
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, int>)
constexpr void nonblocking(const auto& param) noexcept {
  const auto& [items_size, queue_size, producers, consumers] = param;

  Queue queue(queue_size);
  std::latch latch(producers + consumers);

  std::atomic<std::size_t> pushed_items_count = 0;
  std::vector<int> pushed_items(items_size);
  std::iota(pushed_items.begin(), pushed_items.end(), 0);
  const auto producer = [&] {
    latch.arrive_and_wait();
    while (true) {
      const std::size_t index = pushed_items_count.fetch_add(1);
      if (index >= items_size) {
        return;
      }
      while (!queue.try_push(pushed_items[index]));
    }
  };

  std::atomic<std::size_t> popped_items_count = 0;
  std::vector<int> popped_items(items_size);
  const auto consumer = [&] {
    latch.arrive_and_wait();
    while (true) {
      const std::size_t index = popped_items_count.fetch_add(1);
      if (index >= items_size) {
        return;
      }
      while (!queue.try_pop(popped_items[index]));
    }
  };

  std::vector<std::thread> threads;
  threads.reserve(producers + consumers);
  for (std::size_t i = 0; i < producers; ++i) {
    threads.emplace_back(producer);
  }
  for (std::size_t i = 0; i < consumers; ++i) {
    threads.emplace_back(consumer);
  }
  for (auto& thread : threads) {
    thread.join();
  }

  std::sort(popped_items.begin(), popped_items.end());
  EXPECT_EQ(pushed_items, popped_items);
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, int>)
constexpr void blocking(const auto& param) noexcept {
  const auto& [items_size, queue_size, producers, consumers] = param;

  Queue queue(queue_size);
  std::latch latch(producers + consumers);

  std::atomic<std::size_t> pushed_items_count = 0;
  std::vector<int> pushed_items(items_size);
  std::iota(pushed_items.begin(), pushed_items.end(), 0);
  const auto producer = [&] {
    latch.arrive_and_wait();
    while (true) {
      const std::size_t index = pushed_items_count.fetch_add(1);
      if (index >= items_size) {
        return;
      }
      queue.push(pushed_items[index]);
    }
  };

  std::atomic<std::size_t> popped_items_count = 0;
  std::vector<int> popped_items(items_size);
  const auto consumer = [&] {
    latch.arrive_and_wait();
    while (true) {
      const std::size_t index = popped_items_count.fetch_add(1);
      if (index >= items_size) {
        return;
      }
      popped_items[index] = queue.pop();
    }
  };

  std::vector<std::thread> threads;
  threads.reserve(producers + consumers);
  for (std::size_t i = 0; i < producers; ++i) {
    threads.emplace_back(producer);
  }
  for (std::size_t i = 0; i < consumers; ++i) {
    threads.emplace_back(consumer);
  }
  for (auto& thread : threads) {
    thread.join();
  }

  std::sort(popped_items.begin(), popped_items.end());
  EXPECT_EQ(pushed_items, popped_items);
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, int>)
constexpr void queued_producers_nonblocking() noexcept {
  Queue queue(2);
  queue.push(0);
  queue.push(1);

  std::vector<std::jthread> producers(16);
  std::latch latch(producers.size() + 1);
  for (std::size_t i = 0; i < producers.size(); ++i) {
    producers[i] = std::jthread([&, i] {
      latch.count_down();
      queue.push(i + queue.capacity());
    });
  }

  latch.arrive_and_wait();
  for (std::size_t i = 0; i < producers.size();) {
    int value;
    i += queue.try_pop(value);
  }
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, int>)
constexpr void queued_producers_blocking() noexcept {
  Queue queue(2);
  queue.push(0);
  queue.push(1);

  std::vector<std::jthread> producers(16);
  std::latch latch(producers.size() + 1);
  for (std::size_t i = 0; i < producers.size(); ++i) {
    producers[i] = std::jthread([&, i] {
      latch.count_down();
      queue.push(i + queue.capacity());
    });
  }

  latch.arrive_and_wait();
  for (std::size_t i = 0; i < producers.size(); ++i) {
    queue.pop();
  }
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, int>)
constexpr void queued_consumers_nonblocking() noexcept {
  Queue queue(2);

  std::vector<std::jthread> consumers(16);
  std::latch latch(consumers.size() + 1);
  for (std::size_t i = 0; i < consumers.size(); ++i) {
    consumers[i] = std::jthread([&] {
      latch.count_down();
      queue.pop();
    });
  }

  latch.arrive_and_wait();
  for (std::size_t i = 0; i < consumers.size();) {
    i += queue.try_push(i);
  }
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, int>)
constexpr void queued_consumers_blocking() noexcept {
  Queue queue(2);

  std::vector<std::jthread> consumers(16);
  std::latch latch(consumers.size() + 1);
  for (std::size_t i = 0; i < consumers.size(); ++i) {
    consumers[i] = std::jthread([&] {
      latch.count_down();
      queue.pop();
    });
  }

  latch.arrive_and_wait();
  for (std::size_t i = 0; i < consumers.size(); ++i) {
    queue.push(i);
  }
}

template <typename Queue>
  requires(std::is_same_v<typename Queue::value_type, int>)
constexpr void rollover() noexcept {
  Queue queue(8);

  for (std::size_t i = 0; i < 100; ++i) {
    EXPECT_TRUE(queue.try_push(i));
    EXPECT_EQ(queue.pop(), i);
  }
}

}  // namespace tests::queues::templates
