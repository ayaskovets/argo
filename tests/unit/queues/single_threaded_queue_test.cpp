#include <sandbox/queues/single_threaded_queue.hpp>
#include <sandbox/utils/leak_detector_allocator.hpp>

#include <gtest/gtest.h>

#include "templates/queues.hpp"
#include "templates/templates.hpp"

namespace tests::queues {

TEST(queues_single_threaded_queue, size) {
  static_assert(
      sizeof(sandbox::queues::single_threaded_queue<int, std::size_t, 10>) ==
      32);
  static_assert(
      alignof(sandbox::queues::single_threaded_queue<int, std::size_t, 10>) ==
      8);
  static_assert(sizeof(sandbox::queues::single_threaded_queue<int>) == 40);
  static_assert(alignof(sandbox::queues::single_threaded_queue<int>) == 8);
}

TEST(queues_single_threaded_queue, capacity) {
  EXPECT_EQ(
      (sandbox::queues::single_threaded_queue<std::string, std::size_t, 128>()
           .capacity()),
      128);
  EXPECT_EQ(
      (sandbox::queues::single_threaded_queue<std::string, std::size_t, 64>()
           .capacity()),
      64);
  EXPECT_EQ(sandbox::queues::single_threaded_queue<std::string>(128).capacity(),
            128);
  EXPECT_EQ(sandbox::queues::single_threaded_queue<std::string>(64).capacity(),
            64);
}

TEST_T(queues_single_threaded_queue, minimal_capacity,
       sandbox::queues::single_threaded_queue<int>, 1);

TEST_T(queues_single_threaded_queue, nonblocking_smoke,
       sandbox::queues::single_threaded_queue<int>);

TEST_T(queues_single_threaded_queue, blocking_smoke,
       sandbox::queues::single_threaded_queue<int>);

TEST_T(queues_single_threaded_queue, mixed_smoke,
       sandbox::queues::single_threaded_queue<int>);

TEST(queues_single_threaded_queue, blocking_push) { GTEST_SKIP(); }

TEST(queues_single_threaded_queue, blocking_pop) { GTEST_SKIP(); }

TEST_T(queues_single_threaded_queue, capacity_one,
       sandbox::queues::single_threaded_queue<int>);

TEST_T(queues_single_threaded_queue, queue_destructor,
       sandbox::queues::single_threaded_queue<std::shared_ptr<int>>);

TEST_T(queues_single_threaded_queue, item_destructor,
       sandbox::queues::single_threaded_queue<std::shared_ptr<int>>);

TEST_T(queues_single_threaded_queue, non_copyable_item_type,
       sandbox::queues::single_threaded_queue<std::unique_ptr<int>>);

using allocator_queue_t = sandbox::queues::single_threaded_queue<
    double, std::size_t, 2,
    sandbox::utils::leak_detector_allocator<
        sandbox::queues::detail::single_threaded_queue_entry<double>, true>>;
TEST_T(queues_single_threaded_queue, allocator, allocator_queue_t);

TEST(queues_single_threaded_queue_workload, nonblocking) { GTEST_SKIP(); }

TEST(queues_single_threaded_queue_workload, blocking) { GTEST_SKIP(); }

TEST(queues_single_threaded_queue, queued_producers_nonblocking) {
  GTEST_SKIP();
}

TEST(queues_single_threaded_queue, queued_producers_blocking) { GTEST_SKIP(); }

TEST(queues_single_threaded_queue, queued_consumers_nonblocking) {
  GTEST_SKIP();
}

TEST(queues_single_threaded_queue, queued_consumers_blocking) { GTEST_SKIP(); }

TEST_T(queues_single_threaded_queue, rollover,
       sandbox::queues::single_threaded_queue<int>);
}  // namespace tests::queues
