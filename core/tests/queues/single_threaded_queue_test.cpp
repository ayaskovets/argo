#include "queues/single_threaded_queue.hpp"

#include <gtest/gtest.h>

#include <latch>
#include <numeric>
#include <thread>

namespace tests::queues {

TEST(queues_single_threaded_queue, size) {
  static_assert(
      sizeof(core::queues::single_threaded_queue<int, std::size_t, 10>) == 32);
  static_assert(
      alignof(core::queues::single_threaded_queue<int, std::size_t, 10>) == 8);
  static_assert(sizeof(core::queues::single_threaded_queue<int>) == 40);
  static_assert(alignof(core::queues::single_threaded_queue<int>) == 8);
}

TEST(queues_single_threaded_queue, capacity) {
  EXPECT_EQ(
      (core::queues::single_threaded_queue<std::string, std::size_t, 128>()
           .capacity()),
      128);
  EXPECT_EQ((core::queues::single_threaded_queue<std::string, std::size_t, 64>()
                 .capacity()),
            64);
  EXPECT_EQ(core::queues::single_threaded_queue<std::string>(128).capacity(),
            128);
  EXPECT_EQ(core::queues::single_threaded_queue<std::string>(64).capacity(),
            64);
}

TEST(queues_single_threaded_queue, minimal_capacity) {
  EXPECT_ANY_THROW(core::queues::single_threaded_queue<int> queue(0));
  EXPECT_NO_THROW(core::queues::single_threaded_queue<int> queue(1));
}

TEST(queues_single_threaded_queue, nonblocking_smoke) {
  int value;
  core::queues::single_threaded_queue<int> queue(2);

  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);
  EXPECT_FALSE(queue.try_pop(value));
  EXPECT_TRUE(queue.empty());
  EXPECT_TRUE(queue.try_push(1));
  EXPECT_EQ(queue.size(), 1);
  EXPECT_FALSE(queue.empty());
  EXPECT_TRUE(queue.try_push(2));
  EXPECT_EQ(queue.size(), 2);
  EXPECT_FALSE(queue.try_push(3));
  EXPECT_FALSE(queue.empty());
  EXPECT_TRUE(queue.try_pop(value));
  EXPECT_EQ(queue.size(), 1);
  EXPECT_EQ(value, 1);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 1);
  EXPECT_TRUE(queue.try_pop(value));
  EXPECT_EQ(value, 2);
  EXPECT_EQ(queue.size(), 0);
  EXPECT_TRUE(queue.empty());
  EXPECT_FALSE(queue.try_pop(value));
  EXPECT_TRUE(queue.try_push(4));
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 1);
  EXPECT_TRUE(queue.try_pop(value));
  EXPECT_EQ(value, 4);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);
}

TEST(queues_single_threaded_queue, blocking_smoke) {
  core::queues::single_threaded_queue<int> queue(2);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);
  EXPECT_NO_THROW(queue.push(1));
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 1);
  EXPECT_NO_THROW(queue.push(2));
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 2);
  EXPECT_ANY_THROW(queue.push(3));
  EXPECT_EQ(queue.pop(), 1);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 1);
  EXPECT_EQ(queue.pop(), 2);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);
  EXPECT_ANY_THROW(queue.pop());
}

TEST(queues_single_threaded_queue, smoke) {
  int value;
  core::queues::single_threaded_queue<int> queue(2);

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

TEST(queues_single_threaded_queue, blocking_push) { GTEST_SKIP(); }

TEST(queues_single_threaded_queue, blocking_pop) { GTEST_SKIP(); }

TEST(queues_single_threaded_queue, capacity_one) {
  int value;
  core::queues::single_threaded_queue<int> queue(1);

  EXPECT_FALSE(queue.try_pop(value));
  EXPECT_TRUE(queue.try_push(1));
  EXPECT_FALSE(queue.try_push(3));
  EXPECT_TRUE(queue.try_pop(value));
  EXPECT_EQ(value, 1);
  EXPECT_TRUE(queue.try_push(4));
  EXPECT_FALSE(queue.try_push(5));
  EXPECT_TRUE(queue.try_pop(value));
  EXPECT_EQ(value, 4);
  EXPECT_FALSE(queue.try_pop(value));
}

TEST(queues_single_threaded_queue, queue_destructor) {
  std::shared_ptr<int> value = std::make_shared<int>();
  {
    core::queues::single_threaded_queue<std::shared_ptr<int>> queue(2);

    EXPECT_EQ(value.use_count(), 1);
    EXPECT_TRUE(queue.try_push(value));
    EXPECT_EQ(value.use_count(), 2);
    EXPECT_TRUE(queue.try_push(value));
  }
  EXPECT_EQ(value.use_count(), 1);
}

TEST(queues_single_threaded_queue, item_destructor) {
  std::shared_ptr<int> value = std::make_shared<int>();
  core::queues::single_threaded_queue<std::shared_ptr<int>> queue(2);

  EXPECT_EQ(value.use_count(), 1);
  EXPECT_TRUE(queue.try_push(value));
  EXPECT_EQ(value.use_count(), 2);
  EXPECT_TRUE(queue.try_push(value));
  EXPECT_EQ(value.use_count(), 3);
  {
    std::shared_ptr<int> value;
    EXPECT_TRUE(queue.try_pop(value));
  }
  EXPECT_EQ(value.use_count(), 2);
  EXPECT_TRUE(queue.try_push(value));
  EXPECT_EQ(value.use_count(), 3);
  {
    std::shared_ptr<int> value;
    EXPECT_TRUE(queue.try_pop(value));
  }
  EXPECT_EQ(value.use_count(), 2);
  {
    std::shared_ptr<int> value;
    EXPECT_TRUE(queue.try_pop(value));
  }
  EXPECT_EQ(value.use_count(), 1);
}

TEST(queues_single_threaded_queue, non_copyable_item_type) {
  std::unique_ptr<int> value;
  core::queues::single_threaded_queue<std::unique_ptr<int>> queue(2);

  queue.try_push(std::move(value));
  queue.try_pop(value);
}

namespace queues_single_threaded_queue_allocator {

template <typename T>
class allocator : public std::allocator<T> {
 public:
  using std::allocator<T>::allocator;

 public:
  constexpr T* allocate(std::size_t n) {
    T* ptr = std::allocator<T>::allocate(n);
    allocations_[ptr] += n;
    return ptr;
  }

  constexpr void deallocate(T* ptr, std::size_t n) {
    if ((allocations_.at(ptr) -= n) == 0) {
      allocations_.erase(ptr);
    }
    std::allocator<T>::deallocate(ptr, n);
  }

 public:
  template <class U>
  struct rebind {
    typedef allocator<U> other;
  };

 public:
  static constexpr bool is_clean() { return allocations_.empty(); }

 private:
  static inline std::unordered_map<void*, std::size_t> allocations_;
};

}  // namespace queues_single_threaded_queue_allocator

TEST(queues_single_threaded_queue, allocator) {
  using value_t = double;
  using index_t = std::size_t;
  class alignas(core::utils::kCacheLineSize) entry_t final {
   private:
    value_t value_;

   public:
    constexpr value_t& value() { return value_; }
  };

  EXPECT_TRUE(
      queues_single_threaded_queue_allocator::allocator<entry_t>::is_clean());
  {
    core::queues::single_threaded_queue<
        value_t, index_t, 2,
        queues_single_threaded_queue_allocator::allocator<entry_t>>
        queue;
    value_t value;

    EXPECT_TRUE(queue.try_push(1.1));
    EXPECT_TRUE(queue.try_push(2.2));
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 1.1);
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 2.2);
    EXPECT_FALSE(
        queues_single_threaded_queue_allocator::allocator<entry_t>::is_clean());
  }
  EXPECT_TRUE(
      queues_single_threaded_queue_allocator::allocator<entry_t>::is_clean());
}

TEST(queues_single_threaded_queue_workload, nonblocking) { GTEST_SKIP(); }

TEST(queues_single_threaded_queue_workload, blocking) { GTEST_SKIP(); }

}  // namespace tests::queues

namespace tests::queues {

TEST(single_threaded_queue, rollover) {
  core::queues::single_threaded_queue<int> queue(8);

  for (std::size_t i = 0; i < 100; ++i) {
    int value;
    EXPECT_TRUE(queue.try_push(i));
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, i);
  }
}

}  // namespace tests::queues
