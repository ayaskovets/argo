#include <sandbox/queues/waitfree_spsc_queue.hpp>
#include <sandbox/utils/leak_detector_allocator.hpp>

#include <algorithm>
#include <latch>
#include <numeric>
#include <thread>

#include <gtest/gtest.h>

namespace tests::queues {

TEST(queues_waitfree_spsc_queue, size) {
  static_assert(
      sizeof(sandbox::queues::waitfree_spsc_queue<int, std::size_t, 10>) ==
      320);
  static_assert(
      alignof(sandbox::queues::waitfree_spsc_queue<int, std::size_t, 10>) ==
      64);
  static_assert(sizeof(sandbox::queues::waitfree_spsc_queue<int>) == 320);
  static_assert(alignof(sandbox::queues::waitfree_spsc_queue<int>) == 64);
}

TEST(queues_waitfree_spsc_queue, capacity) {
  EXPECT_EQ(
      (sandbox::queues::waitfree_spsc_queue<std::string, std::size_t, 128>()
           .capacity()),
      128);
  EXPECT_EQ(
      (sandbox::queues::waitfree_spsc_queue<std::string, std::size_t, 64>()
           .capacity()),
      64);
  EXPECT_EQ(sandbox::queues::waitfree_spsc_queue<std::string>(128).capacity(),
            128);
  EXPECT_EQ(sandbox::queues::waitfree_spsc_queue<std::string>(64).capacity(),
            64);
}

TEST(queues_waitfree_spsc_queue, minimal_capacity) {
  EXPECT_ANY_THROW(sandbox::queues::waitfree_spsc_queue<int> queue(0));
  EXPECT_NO_THROW(sandbox::queues::waitfree_spsc_queue<int> queue(1));
}

TEST(queues_waitfree_spsc_queue, nonblocking_smoke) {
  int value;
  sandbox::queues::waitfree_spsc_queue<int> queue(2);

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

TEST(queues_waitfree_spsc_queue, blocking_smoke) {
  sandbox::queues::waitfree_spsc_queue<int> queue(2);

  queue.push(1);
  queue.push(2);
  EXPECT_EQ(queue.pop(), 1);
  EXPECT_EQ(queue.pop(), 2);
  queue.push(3);
  EXPECT_EQ(queue.pop(), 3);
}

TEST(queues_waitfree_spsc_queue, smoke) {
  int value;
  sandbox::queues::waitfree_spsc_queue<int> queue(2);

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

TEST(queues_waitfree_spsc_queue, blocking_push) {
  sandbox::queues::waitfree_spsc_queue<int> queue(2);

  queue.push(1);
  queue.push(2);

  std::jthread consumer([&queue] {
    int value;
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 1);
  });

  queue.push(3);
  EXPECT_EQ(queue.pop(), 2);
}

TEST(queues_waitfree_spsc_queue, blocking_pop) {
  sandbox::queues::waitfree_spsc_queue<int> queue(2);

  std::jthread producer([&queue] { EXPECT_TRUE(queue.try_push(42)); });

  EXPECT_EQ(queue.pop(), 42);
}

TEST(queues_waitfree_spsc_queue, capacity_one) {
  int value;
  sandbox::queues::waitfree_spsc_queue<int> queue(1);

  EXPECT_FALSE(queue.try_pop(value));
  queue.push(1);
  EXPECT_FALSE(queue.try_push(3));
  EXPECT_EQ(queue.pop(), 1);
  queue.push(4);
  EXPECT_FALSE(queue.try_push(5));
  EXPECT_EQ(queue.pop(), 4);
  EXPECT_FALSE(queue.try_pop(value));
}

TEST(queues_waitfree_spsc_queue, queue_destructor) {
  std::shared_ptr<int> value = std::make_shared<int>();
  {
    sandbox::queues::waitfree_spsc_queue<std::shared_ptr<int>> queue(2);

    EXPECT_EQ(value.use_count(), 1);
    queue.push(value);
    EXPECT_EQ(value.use_count(), 2);
    queue.push(value);
    EXPECT_EQ(value.use_count(), 3);
  }
  EXPECT_EQ(value.use_count(), 1);
}

TEST(queues_waitfree_spsc_queue, item_destructor) {
  std::shared_ptr<int> value = std::make_shared<int>();
  sandbox::queues::waitfree_spsc_queue<std::shared_ptr<int>> queue(2);

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

TEST(queues_waitfree_spsc_queue, non_copyable_item_type) {
  std::unique_ptr<int> value;
  sandbox::queues::waitfree_spsc_queue<std::unique_ptr<int>> queue(2);

  queue.push(std::move(value));
  queue.pop();
  queue.try_push(std::move(value));
  queue.try_pop(value);
}

TEST(queues_waitfree_spsc_queue, allocator) {
  using value_t = double;
  using index_t = std::size_t;
  using allocator_value_t = sandbox::queues::detail::waitfree_spsc_queue_entry<
      value_t, std::hardware_destructive_interference_size>;
  sandbox::utils::leak_detector_allocator<allocator_value_t, true> allocator;
  sandbox::utils::leak_detector_allocator<allocator_value_t, true>::clear();

  EXPECT_TRUE(allocator.is_clean());
  {
    sandbox::queues::waitfree_spsc_queue<value_t, index_t, 2,
                                         decltype(allocator)>
        queue(allocator);
    EXPECT_EQ(queue.get_allocator(), allocator);

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

class queues_waitfree_spsc_queue_workload
    : public ::testing::TestWithParam<std::tuple<
          std::size_t /* items_size */, std::size_t /* queue_size */,
          std::size_t /* producers */, std::size_t /* consumers */>> {};

TEST_P(queues_waitfree_spsc_queue_workload, nonblocking) {
  const auto& [items_size, queue_size, producers, consumers] = GetParam();

  sandbox::queues::waitfree_spsc_queue<int> queue(queue_size);

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

TEST_P(queues_waitfree_spsc_queue_workload, blocking) {
  const auto& [items_size, queue_size, producers, consumers] = GetParam();

  sandbox::queues::waitfree_spsc_queue<int> queue(queue_size);

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
    queues_waitfree_spsc_queue_workload, queues_waitfree_spsc_queue_workload,
    ::testing::Values(std::make_tuple(5 /* items_size */, 3 /* queue_size */,
                                      1 /* producers */, 1 /* consumers */),
                      std::make_tuple(20 /* items_size */, 10 /* queue_size */,
                                      1 /* producers */, 1 /* consumers */),
                      std::make_tuple(100 /* items_size */, 10 /* queue_size */,
                                      1 /* producers */, 1 /* consumers */),
                      std::make_tuple(10000 /* items_size */,
                                      100 /* queue_size */, 1 /* producers */,
                                      1 /* consumers */)));

}  // namespace tests::queues

namespace tests::queues {

TEST(waitfree_spsc_queue, rollover) {
  sandbox::queues::waitfree_spsc_queue<int> queue(8);

  for (std::size_t i = 0; i < 100; ++i) {
    EXPECT_TRUE(queue.try_push(i));
    EXPECT_EQ(queue.pop(), i);
  }
}

}  // namespace tests::queues
