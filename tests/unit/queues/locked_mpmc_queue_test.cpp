#include <sandbox/queues/locked_mpmc_queue.hpp>
#include <sandbox/utils/leak_detector_allocator.hpp>

#include <gtest/gtest.h>

#include "templates/queues.hpp"
#include "templates/templates.hpp"

namespace tests::queues {

TEST(queues_locked_mpmc_queue, size) {
  static_assert(
      sizeof(sandbox::queues::locked_mpmc_queue<int, std::size_t, 10>) == 256);
  static_assert(
      alignof(sandbox::queues::locked_mpmc_queue<int, std::size_t, 10>) == 64);
  static_assert(sizeof(sandbox::queues::locked_mpmc_queue<int>) == 256);
  static_assert(alignof(sandbox::queues::locked_mpmc_queue<int>) == 64);
}

TEST(queues_locked_mpmc_queue, capacity) {
  EXPECT_EQ((sandbox::queues::locked_mpmc_queue<std::string, std::size_t, 128>()
                 .capacity()),
            128);
  EXPECT_EQ((sandbox::queues::locked_mpmc_queue<std::string, std::size_t, 64>()
                 .capacity()),
            64);
  EXPECT_EQ(sandbox::queues::locked_mpmc_queue<std::string>(128).capacity(),
            128);
  EXPECT_EQ(sandbox::queues::locked_mpmc_queue<std::string>(64).capacity(), 64);
}

TEST_T(queues_locked_mpmc_queue, minimal_capacity,
       sandbox::queues::locked_mpmc_queue<int>, 0);

TEST_T(queues_locked_mpmc_queue, nonblocking_smoke,
       sandbox::queues::locked_mpmc_queue<int>);

TEST_T(queues_locked_mpmc_queue, blocking_smoke,
       sandbox::queues::locked_mpmc_queue<int>);

TEST_T(queues_locked_mpmc_queue, mixed_smoke,
       sandbox::queues::locked_mpmc_queue<int>);

TEST_T(queues_locked_mpmc_queue, blocking_push,
       sandbox::queues::locked_mpmc_queue<int>);

TEST_T(queues_locked_mpmc_queue, blocking_pop,
       sandbox::queues::locked_mpmc_queue<int>);

TEST_T(queues_locked_mpmc_queue, capacity_one,
       sandbox::queues::locked_mpmc_queue<int>);

TEST_T(queues_locked_mpmc_queue, queue_destructor,
       sandbox::queues::locked_mpmc_queue<std::shared_ptr<int>>);

TEST_T(queues_locked_mpmc_queue, item_destructor,
       sandbox::queues::locked_mpmc_queue<std::shared_ptr<int>>);

TEST_T(queues_locked_mpmc_queue, non_copyable_item_type,
       sandbox::queues::locked_mpmc_queue<std::unique_ptr<int>>);

using allocator_queue_t = sandbox::queues::locked_mpmc_queue<
    double, std::size_t, 2,
    sandbox::utils::leak_detector_allocator<
        sandbox::queues::detail::locked_mpmc_queue_entry<
            double, std::hardware_destructive_interference_size>,
        true>>;
TEST_T(queues_locked_mpmc_queue, allocator, allocator_queue_t);

class queues_locked_mpmc_queue_workload
    : public ::testing::TestWithParam<
          std::tuple<std::size_t /* items_size */, std::size_t /* queue_size*/,
                     std::size_t /* producers*/, std::size_t /*consumers*/>> {};

TEST_P_T(queues_locked_mpmc_queue_workload, nonblocking,
         sandbox::queues::locked_mpmc_queue<int>);

TEST_P_T(queues_locked_mpmc_queue_workload, blocking,
         sandbox::queues::locked_mpmc_queue<int>);

INSTANTIATE_TEST_SUITE_P(queues_locked_mpmc_queue_workload,
                         queues_locked_mpmc_queue_workload,
                         ::testing::Values(std::make_tuple(5, 3, 1, 1),
                                           std::make_tuple(20, 10, 2, 1),
                                           std::make_tuple(100, 10, 4, 1),
                                           std::make_tuple(100, 10, 1, 4),
                                           std::make_tuple(10000, 100, 4, 4)));

TEST_T(queues_locked_mpmc_queue, queued_producers_nonblocking,
       sandbox::queues::locked_mpmc_queue<int>);

TEST_T(queues_locked_mpmc_queue, queued_producers_blocking,
       sandbox::queues::locked_mpmc_queue<int>);

TEST_T(queues_locked_mpmc_queue, queued_consumers_nonblocking,
       sandbox::queues::locked_mpmc_queue<int>);

TEST_T(queues_locked_mpmc_queue, queued_consumers_blocking,
       sandbox::queues::locked_mpmc_queue<int>);

}  // namespace tests::queues
