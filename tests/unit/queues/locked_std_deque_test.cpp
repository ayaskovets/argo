#include <sandbox/queues/locked_std_deque.hpp>
#include <sandbox/utils/leak_detector_allocator.hpp>

#include <new>

#include <gtest/gtest.h>

#include "templates/queues.hpp"
#include "templates/templates.hpp"

namespace tests::queues {

TEST(queues_locked_std_deque, size) {
  static_assert(
      sizeof(sandbox::queues::locked_std_deque<int, std::size_t, 10>) == 208);
  static_assert(
      alignof(sandbox::queues::locked_std_deque<int, std::size_t, 10>) == 8);
  static_assert(sizeof(sandbox::queues::locked_std_deque<int>) == 216);
  static_assert(alignof(sandbox::queues::locked_std_deque<int>) == 8);
}

TEST(queues_locked_std_deque, capacity) {
  EXPECT_EQ((sandbox::queues::locked_std_deque<std::string, std::size_t, 128>()
                 .capacity()),
            128);
  EXPECT_EQ((sandbox::queues::locked_std_deque<std::string, std::size_t, 64>()
                 .capacity()),
            64);
  EXPECT_EQ(sandbox::queues::locked_std_deque<std::string>(128).capacity(),
            128);
  EXPECT_EQ(sandbox::queues::locked_std_deque<std::string>(64).capacity(), 64);
}

TEST_T(queues_locked_std_deque, minimal_capacity,
       sandbox::queues::locked_std_deque<int>, 1);

TEST_T(queues_locked_std_deque, nonblocking_smoke,
       sandbox::queues::locked_std_deque<int>);

TEST_T(queues_locked_std_deque, blocking_smoke,
       sandbox::queues::locked_std_deque<int>);

TEST_T(queues_locked_std_deque, mixed_smoke,
       sandbox::queues::locked_std_deque<int>);

TEST_T(queues_locked_std_deque, blocking_push,
       sandbox::queues::locked_std_deque<int>);

TEST_T(queues_locked_std_deque, blocking_pop,
       sandbox::queues::locked_std_deque<int>);

TEST_T(queues_locked_std_deque, capacity_one,
       sandbox::queues::locked_std_deque<int>);

TEST_T(queues_locked_std_deque, queue_destructor,
       sandbox::queues::locked_std_deque<std::shared_ptr<int>>);

TEST_T(queues_locked_std_deque, item_destructor,
       sandbox::queues::locked_std_deque<std::shared_ptr<int>>);

TEST_T(queues_locked_std_deque, non_copyable_item_type,
       sandbox::queues::locked_std_deque<std::unique_ptr<int>>);

using allocator_queue_t = sandbox::queues::locked_std_deque<
    double, std::size_t, 2,
    sandbox::utils::leak_detector_allocator<
        sandbox::queues::detail::locked_std_deque_entry<
            double, std::hardware_destructive_interference_size>,
        true>>;
TEST_T(queues_locked_std_deque, allocator, allocator_queue_t);

class queues_locked_std_deque_workload
    : public ::testing::TestWithParam<
          std::tuple<std::size_t /* items_size */, std::size_t /* queue_size*/,
                     std::size_t /* producers*/, std::size_t /*consumers*/>> {};

TEST_P_T(queues_locked_std_deque_workload, nonblocking,
         sandbox::queues::locked_std_deque<int>);

TEST_P_T(queues_locked_std_deque_workload, blocking,
         sandbox::queues::locked_std_deque<int>);

INSTANTIATE_TEST_SUITE_P(queues_locked_std_deque_workload,
                         queues_locked_std_deque_workload,
                         ::testing::Values(std::make_tuple(5, 3, 1, 1),
                                           std::make_tuple(20, 10, 2, 1),
                                           std::make_tuple(100, 10, 4, 1),
                                           std::make_tuple(100, 10, 1, 4),
                                           std::make_tuple(10000, 100, 4, 4)));

TEST_T(queues_locked_std_deque, queued_producers_nonblocking,
       sandbox::queues::locked_std_deque<int>);

TEST_T(queues_locked_std_deque, queued_producers_blocking,
       sandbox::queues::locked_std_deque<int>);

TEST_T(queues_locked_std_deque, queued_consumers_nonblocking,
       sandbox::queues::locked_std_deque<int>);

TEST_T(queues_locked_std_deque, queued_consumers_blocking,
       sandbox::queues::locked_std_deque<int>);

}  // namespace tests::queues
