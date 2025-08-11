#include "queues/lockless_mpmc_queue.hpp"

#include <gtest/gtest.h>

#include <latch>
#include <numeric>
#include <thread>

#include "utils/leak_detector_allocator.hpp"

namespace tests::queues {

TEST(queues_lockless_mpmc_queue, size) {
  static_assert(
      sizeof(core::queues::lockless_mpmc_queue<int, std::size_t, 10>) == 192);
  static_assert(
      alignof(core::queues::lockless_mpmc_queue<int, std::size_t, 10>) == 64);
  static_assert(sizeof(core::queues::lockless_mpmc_queue<int>) == 192);
  static_assert(alignof(core::queues::lockless_mpmc_queue<int>) == 64);
}

TEST(queues_lockless_mpmc_queue, capacity) {
  EXPECT_EQ((core::queues::lockless_mpmc_queue<std::string, std::size_t, 128>()
                 .capacity()),
            128);
  EXPECT_EQ((core::queues::lockless_mpmc_queue<std::string, std::size_t, 64>()
                 .capacity()),
            64);
  EXPECT_EQ(core::queues::lockless_mpmc_queue<std::string>(128).capacity(),
            128);
  EXPECT_EQ(core::queues::lockless_mpmc_queue<std::string>(64).capacity(), 64);
}

TEST(queues_lockless_mpmc_queue, minimal_capacity) {
  EXPECT_ANY_THROW(core::queues::lockless_mpmc_queue<int> queue(0));
  EXPECT_ANY_THROW(core::queues::lockless_mpmc_queue<int> queue(1));
  EXPECT_NO_THROW(core::queues::lockless_mpmc_queue<int> queue(2));
}

TEST(queues_lockless_mpmc_queue, nonblocking_smoke) {
  int value;
  core::queues::lockless_mpmc_queue<int> queue(2);

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

TEST(queues_lockless_mpmc_queue, blocking_smoke) {
  core::queues::lockless_mpmc_queue<int> queue(2);

  queue.push(1);
  queue.push(2);
  EXPECT_EQ(queue.pop(), 1);
  EXPECT_EQ(queue.pop(), 2);
  queue.push(3);
  EXPECT_EQ(queue.pop(), 3);
}

TEST(queues_lockless_mpmc_queue, smoke) {
  int value;
  core::queues::lockless_mpmc_queue<int> queue(2);

  EXPECT_FALSE(queue.try_pop(value));
  queue.push(1);
  queue.push(2);
  EXPECT_FALSE(queue.try_push(3));
  EXPECT_EQ(queue.pop(), 1);
  queue.push(4);
  EXPECT_FALSE(queue.try_push(5));
  EXPECT_EQ(queue.pop(), 2);
  EXPECT_EQ(queue.pop(), 4);
  EXPECT_FALSE(queue.try_pop(value));
}

TEST(queues_lockless_mpmc_queue, blocking_push) {
  core::queues::lockless_mpmc_queue<int> queue(2);

  queue.push(1);
  queue.push(2);

  std::thread consumer([&queue] {
    int value;
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 1);
  });

  queue.push(3);
  EXPECT_EQ(queue.pop(), 2);

  consumer.join();
}

TEST(queues_lockless_mpmc_queue, blocking_pop) {
  core::queues::lockless_mpmc_queue<int> queue(2);

  std::thread producer([&queue] { EXPECT_TRUE(queue.try_push(42)); });

  EXPECT_EQ(queue.pop(), 42);

  producer.join();
}

TEST(queues_lockless_mpmc_queue, capacity_one) {
  EXPECT_ANY_THROW(core::queues::lockless_mpmc_queue<int> queue(1));
}

TEST(queues_lockless_mpmc_queue, queue_destructor) {
  std::shared_ptr<int> value = std::make_shared<int>();
  {
    core::queues::lockless_mpmc_queue<std::shared_ptr<int>> queue(2);

    EXPECT_EQ(value.use_count(), 1);
    queue.push(value);
    EXPECT_EQ(value.use_count(), 2);
    queue.push(value);
    EXPECT_EQ(value.use_count(), 3);
  }
  EXPECT_EQ(value.use_count(), 1);
}

TEST(queues_lockless_mpmc_queue, item_destructor) {
  std::shared_ptr<int> value = std::make_shared<int>();
  core::queues::lockless_mpmc_queue<std::shared_ptr<int>> queue(2);

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

TEST(queues_lockless_mpmc_queue, non_copyable_item_type) {
  std::unique_ptr<int> value;
  core::queues::lockless_mpmc_queue<std::unique_ptr<int>> queue(2);

  queue.push(std::move(value));
  queue.pop();
  queue.try_push(std::move(value));
  queue.try_pop(value);
}

TEST(queues_lockless_mpmc_queue, allocator) {
  using value_t = double;
  using index_t = std::size_t;
  using entry_t = core::queues::detail::lockless_mpmc_queue_entry_impl<
      value_t, index_t, core::utils::kCacheLineSize>;
  core::utils::leak_detector_allocator<entry_t> allocator;

  EXPECT_TRUE(allocator.is_clean());
  {
    core::queues::lockless_mpmc_queue<value_t, index_t, 2, decltype(allocator)>
        queue(allocator);

    EXPECT_FALSE(allocator.is_clean());
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

class queues_lockless_mpmc_queue_workload
    : public ::testing::TestWithParam<std::tuple<
          std::size_t /* items_size */, std::size_t /* queue_size */,
          std::size_t /* producers */, std::size_t /* consumers */>> {};

TEST_P(queues_lockless_mpmc_queue_workload, nonblocking) {
  const auto& [items_size, queue_size, producers, consumers] = GetParam();

  core::queues::lockless_mpmc_queue<int> queue(queue_size);

  std::latch latch(producers + consumers);
  std::vector<std::thread> threads;
  threads.reserve(producers + consumers);

  std::atomic<std::size_t> pushed_items_count = 0;
  std::vector<int> pushed_items(items_size);
  std::iota(pushed_items.begin(), pushed_items.end(), 0);

  std::atomic<std::size_t> popped_items_count = 0;
  std::vector<int> popped_items(items_size);

  const auto producer = [items_size, &latch, &queue, &pushed_items,
                         &pushed_items_count] {
    latch.arrive_and_wait();
    while (true) {
      const std::size_t index = pushed_items_count.fetch_add(1);
      if (index >= items_size) {
        return;
      }
      while (!queue.try_push(pushed_items[index]));
    }
  };

  const auto consumer = [items_size, &latch, &queue, &popped_items,
                         &popped_items_count] {
    latch.arrive_and_wait();
    while (true) {
      const std::size_t index = popped_items_count.fetch_add(1);
      if (index >= items_size) {
        return;
      }
      while (!queue.try_pop(popped_items[index]));
    }
  };

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

TEST_P(queues_lockless_mpmc_queue_workload, blocking) {
  const auto& [items_size, queue_size, producers, consumers] = GetParam();

  core::queues::lockless_mpmc_queue<int> queue(queue_size);

  std::latch latch(producers + consumers);
  std::vector<std::thread> threads;
  threads.reserve(producers + consumers);

  std::atomic<std::size_t> pushed_items_count = 0;
  std::vector<int> pushed_items(items_size);
  std::iota(pushed_items.begin(), pushed_items.end(), 0);

  std::atomic<std::size_t> popped_items_count = 0;
  std::vector<int> popped_items(items_size);

  const auto producer = [items_size, &latch, &queue, &pushed_items,
                         &pushed_items_count] {
    latch.arrive_and_wait();
    while (true) {
      const std::size_t index = pushed_items_count.fetch_add(1);
      if (index >= items_size) {
        return;
      }
      queue.push(pushed_items[index]);
    }
  };

  const auto consumer = [items_size, &latch, &queue, &popped_items,
                         &popped_items_count] {
    latch.arrive_and_wait();
    while (true) {
      const std::size_t index = popped_items_count.fetch_add(1);
      if (index >= items_size) {
        return;
      }
      popped_items[index] = queue.pop();
    }
  };

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

INSTANTIATE_TEST_SUITE_P(
    queues_lockless_mpmc_queue_workload, queues_lockless_mpmc_queue_workload,
    ::testing::Values(std::make_tuple(5 /* items_size */, 4 /* queue_size */,
                                      1 /* producers */, 1 /* consumers */),
                      std::make_tuple(100 /* items_size */, 16 /* queue_size */,
                                      4 /* producers */, 1 /* consumers */),
                      std::make_tuple(100 /* items_size */, 16 /* queue_size */,
                                      1 /* producers */, 4 /* consumers */),
                      std::make_tuple(30 /* items_size */, 8 /* queue_size */,
                                      4 /* producers */, 4 /* consumers */),
                      std::make_tuple(10000 /* items_size */,
                                      128 /* queue_size */, 4 /* producers */,
                                      4 /* consumers */)));

}  // namespace tests::queues

namespace tests::queues {

TEST(lockless_mpmc_queue, rollover) {
  core::queues::lockless_mpmc_queue<int> queue(8);

  for (std::size_t i = 0; i < 100; ++i) {
    EXPECT_TRUE(queue.try_push(i));
    EXPECT_EQ(queue.pop(), i);
  }
}

TEST(queues_lockless_mpmc_queue, queued_producers_nonblocking) {
  core::queues::lockless_mpmc_queue<int> queue(2);
  queue.push(0);
  queue.push(1);

  std::vector<std::thread> producers(16);
  std::latch latch(producers.size() + 1);
  for (std::size_t i = 0; i < producers.size(); ++i) {
    producers[i] = std::thread([&queue, &latch, i] {
      latch.count_down();
      queue.push(i + queue.capacity());
    });
  }

  latch.arrive_and_wait();
  for (std::size_t i = 0; i < producers.size();) {
    int value;
    i += queue.try_pop(value);
  }

  for (auto& producer : producers) {
    producer.join();
  }
}

TEST(queues_lockless_mpmc_queue, queued_producers_blocking) {
  core::queues::lockless_mpmc_queue<int> queue(2);
  queue.push(0);
  queue.push(1);

  std::vector<std::thread> producers(16);
  std::latch latch(producers.size() + 1);
  for (std::size_t i = 0; i < producers.size(); ++i) {
    producers[i] = std::thread([&queue, &latch, i] {
      latch.count_down();
      queue.push(i + queue.capacity());
    });
  }

  latch.arrive_and_wait();
  for (std::size_t i = 0; i < producers.size(); ++i) {
    queue.pop();
  }

  for (auto& producer : producers) {
    producer.join();
  }
}

TEST(queues_lockless_mpmc_queue, queued_consumers_nonblocking) {
  core::queues::lockless_mpmc_queue<int> queue(2);

  std::vector<std::thread> consumers(16);
  std::latch latch(consumers.size() + 1);
  for (std::size_t i = 0; i < consumers.size(); ++i) {
    consumers[i] = std::thread([&queue, &latch] {
      latch.count_down();
      queue.pop();
    });
  }

  latch.arrive_and_wait();
  for (std::size_t i = 0; i < consumers.size();) {
    i += queue.try_push(i);
  }

  for (auto& consumer : consumers) {
    consumer.join();
  }
}

TEST(queues_lockless_mpmc_queue, queued_consumers_blocking) {
  core::queues::lockless_mpmc_queue<int> queue(2);

  std::vector<std::thread> consumers(16);
  std::latch latch(consumers.size() + 1);
  for (std::size_t i = 0; i < consumers.size(); ++i) {
    consumers[i] = std::thread([&queue, &latch] {
      latch.count_down();
      queue.pop();
    });
  }

  latch.arrive_and_wait();
  for (std::size_t i = 0; i < consumers.size(); ++i) {
    queue.push(i);
  }

  for (auto& consumer : consumers) {
    consumer.join();
  }
}

}  // namespace tests::queues
