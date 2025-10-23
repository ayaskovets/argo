#include <sandbox/queues/lockfree_mpmc_queue.hpp>
#include <sandbox/utils/leak_detector_allocator.hpp>

#include <gtest/gtest.h>

#include "templates/queues.hpp"
#include "templates/templates.hpp"

namespace tests::queues {

namespace queues = sandbox::queues;
namespace utils = sandbox::utils;

TEST(queues_lockfree_mpmc_queue, size) {
  static_assert(sizeof(queues::lockfree_mpmc_queue<int, std::size_t, 10>) ==
                192);
  static_assert(alignof(queues::lockfree_mpmc_queue<int, std::size_t, 10>) ==
                64);
  static_assert(sizeof(queues::lockfree_mpmc_queue<int>) == 192);
  static_assert(alignof(queues::lockfree_mpmc_queue<int>) == 64);
}

TEST(queues_lockfree_mpmc_queue, capacity) {
  EXPECT_EQ((queues::lockfree_mpmc_queue<int, std::size_t, 128>().capacity()),
            128);
  EXPECT_EQ((queues::lockfree_mpmc_queue<int, std::size_t, 64>().capacity()),
            64);
  EXPECT_EQ(queues::lockfree_mpmc_queue<int>(128).capacity(), 128);
  EXPECT_EQ(queues::lockfree_mpmc_queue<int>(64).capacity(), 64);
}

TEST_T(queues_lockfree_mpmc_queue, minimal_capacity,
       queues::lockfree_mpmc_queue<int>, 2);

TEST_T(queues_lockfree_mpmc_queue, nonblocking_smoke,
       queues::lockfree_mpmc_queue<int>);

TEST_T(queues_lockfree_mpmc_queue, blocking_smoke,
       queues::lockfree_mpmc_queue<int>);

TEST_T(queues_lockfree_mpmc_queue, mixed_smoke,
       queues::lockfree_mpmc_queue<int>);

TEST_T(queues_lockfree_mpmc_queue, blocking_push,
       queues::lockfree_mpmc_queue<int>);

TEST_T(queues_lockfree_mpmc_queue, blocking_pop,
       queues::lockfree_mpmc_queue<int>);

TEST(queues_lockfree_mpmc_queue, capacity_one) { GTEST_SKIP(); }

TEST(queues_lockfree_mpmc_queue, queue_destructor) { GTEST_SKIP(); }

TEST(queues_lockfree_mpmc_queue, item_destructor) { GTEST_SKIP(); }

TEST(queues_lockfree_mpmc_queue, non_copyable_item_type) { GTEST_SKIP(); }

using allocator_queue_t = queues::lockfree_mpmc_queue<
    double, std::size_t, 2,
    utils::leak_detector_allocator<
        std::atomic<
            queues::detail::lockfree_mpmc_queue_entry<double, std::size_t>>,
        true>>;
TEST_T(queues_lockfree_mpmc_queue, allocator, allocator_queue_t);

class queues_lockfree_mpmc_queue_workload
    : public ::testing::TestWithParam<std::tuple<
          std::size_t /* items_size */, std::size_t /* queue_size */,
          std::size_t /* producers */, std::size_t /* consumers */>> {};

TEST_P_T(queues_lockfree_mpmc_queue_workload, nonblocking,
         queues::lockfree_mpmc_queue<int>);

TEST_P_T(queues_lockfree_mpmc_queue_workload, blocking,
         queues::lockfree_mpmc_queue<int>);

INSTANTIATE_TEST_SUITE_P(queues_lockfree_mpmc_queue_workload,
                         queues_lockfree_mpmc_queue_workload,
                         ::testing::Values(std::make_tuple(5, 4, 1, 1),
                                           std::make_tuple(100, 16, 4, 1),
                                           std::make_tuple(100, 16, 1, 4),
                                           std::make_tuple(30, 8, 4, 4),
                                           std::make_tuple(10000, 128, 4, 4)));

TEST_T(queues_lockfree_mpmc_queue, queued_producers_nonblocking,
       queues::lockfree_mpmc_queue<int>);

TEST_T(queues_lockfree_mpmc_queue, queued_producers_blocking,
       queues::lockfree_mpmc_queue<int>);

TEST_T(queues_lockfree_mpmc_queue, queued_consumers_nonblocking,
       queues::lockfree_mpmc_queue<int>);

TEST_T(queues_lockfree_mpmc_queue, queued_consumers_blocking,
       queues::lockfree_mpmc_queue<int>);

TEST_T(queues_lockfree_mpmc_queue, rollover, queues::lockfree_mpmc_queue<int>);

}  // namespace tests::queues
