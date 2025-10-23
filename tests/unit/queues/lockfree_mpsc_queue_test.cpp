#include <sandbox/queues/lockfree_mpsc_queue.hpp>
#include <sandbox/utils/leak_detector_allocator.hpp>

#include <gtest/gtest.h>

#include "templates/queues.hpp"
#include "templates/templates.hpp"

namespace tests::queues {

TEST(queues_lockfree_mpsc_queue, size) {
  static_assert(
      sizeof(sandbox::queues::lockfree_mpsc_queue<int, std::size_t, 10>) ==
      256);
  static_assert(
      alignof(sandbox::queues::lockfree_mpsc_queue<int, std::size_t, 10>) ==
      64);
  static_assert(sizeof(sandbox::queues::lockfree_mpsc_queue<int>) == 256);
  static_assert(alignof(sandbox::queues::lockfree_mpsc_queue<int>) == 64);
}

TEST(queues_lockfree_mpsc_queue, capacity) {
  EXPECT_EQ(
      (sandbox::queues::lockfree_mpsc_queue<std::string, std::size_t, 128>()
           .capacity()),
      128);
  EXPECT_EQ(
      (sandbox::queues::lockfree_mpsc_queue<std::string, std::size_t, 64>()
           .capacity()),
      64);
  EXPECT_EQ(sandbox::queues::lockfree_mpsc_queue<std::string>(128).capacity(),
            128);
  EXPECT_EQ(sandbox::queues::lockfree_mpsc_queue<std::string>(64).capacity(),
            64);
}

TEST_T(queues_lockfree_mpsc_queue, minimal_capacity,
       sandbox::queues::lockfree_mpsc_queue<int>, 2);

TEST_T(queues_lockfree_mpsc_queue, nonblocking_smoke,
       sandbox::queues::lockfree_mpsc_queue<int>);

TEST_T(queues_lockfree_mpsc_queue, blocking_smoke,
       sandbox::queues::lockfree_mpsc_queue<int>);

TEST_T(queues_lockfree_mpsc_queue, mixed_smoke,
       sandbox::queues::lockfree_mpsc_queue<int>);

TEST_T(queues_lockfree_mpsc_queue, blocking_push,
       sandbox::queues::lockfree_mpsc_queue<int>);

TEST_T(queues_lockfree_mpsc_queue, blocking_pop,
       sandbox::queues::lockfree_mpsc_queue<int>);

TEST(queues_lockfree_mpsc_queue, capacity_one) { GTEST_SKIP(); }

TEST_T(queues_lockfree_mpsc_queue, queue_destructor,
       sandbox::queues::lockfree_mpsc_queue<std::shared_ptr<int>>);

TEST_T(queues_lockfree_mpsc_queue, item_destructor,
       sandbox::queues::lockfree_mpsc_queue<std::shared_ptr<int>>);

TEST_T(queues_lockfree_mpsc_queue, non_copyable_item_type,
       sandbox::queues::lockfree_mpsc_queue<std::unique_ptr<int>>);

using allocator_queue_t = sandbox::queues::lockfree_mpsc_queue<
    double, std::size_t, 2,
    sandbox::utils::leak_detector_allocator<
        sandbox::queues::detail::lockfree_mpsc_queue_entry<double>, true>>;
TEST_T(queues_lockfree_mpsc_queue, allocator, allocator_queue_t);

class queues_lockfree_mpsc_queue_workload
    : public ::testing::TestWithParam<std::tuple<
          std::size_t /* items_size */, std::size_t /* queue_size */,
          std::size_t /* producers */, std::size_t /* consumers */>> {};

TEST_P_T(queues_lockfree_mpsc_queue_workload, nonblocking,
         sandbox::queues::lockfree_mpsc_queue<int>);

TEST_P_T(queues_lockfree_mpsc_queue_workload, blocking,
         sandbox::queues::lockfree_mpsc_queue<int>);

INSTANTIATE_TEST_SUITE_P(queues_lockfree_mpsc_queue_workload,
                         queues_lockfree_mpsc_queue_workload,
                         ::testing::Values(std::make_tuple(5, 4, 1, 1),
                                           std::make_tuple(20, 16, 2, 1),
                                           std::make_tuple(100, 16, 4, 1),
                                           std::make_tuple(10000, 128, 4, 1)));

TEST_T(queues_lockfree_mpsc_queue, queued_producers_nonblocking,
       sandbox::queues::lockfree_mpsc_queue<int>);

TEST_T(queues_lockfree_mpsc_queue, queued_producers_blocking,
       sandbox::queues::lockfree_mpsc_queue<int>);

TEST(queues_lockfree_mpsc_queue, queued_consumers_nonblocking) { GTEST_SKIP(); }

TEST(queues_lockfree_mpsc_queue, queued_consumers_blocking) { GTEST_SKIP(); }

TEST_T(queues_lockfree_mpsc_queue, rollover,
       sandbox::queues::lockfree_mpsc_queue<int>);

}  // namespace tests::queues
