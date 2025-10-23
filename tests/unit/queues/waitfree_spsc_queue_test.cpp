#include <sandbox/queues/waitfree_spsc_queue.hpp>
#include <sandbox/utils/leak_detector_allocator.hpp>

#include <gtest/gtest.h>

#include "templates/queues.hpp"
#include "templates/templates.hpp"

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

TEST_T(queues_waitfree_spsc_queue, minimal_capacity,
       sandbox::queues::waitfree_spsc_queue<int>, 1);

TEST_T(queues_waitfree_spsc_queue, nonblocking_smoke,
       sandbox::queues::waitfree_spsc_queue<int>);

TEST_T(queues_waitfree_spsc_queue, blocking_smoke,
       sandbox::queues::waitfree_spsc_queue<int>);

TEST_T(queues_waitfree_spsc_queue, mixed_smoke,
       sandbox::queues::waitfree_spsc_queue<int>);

TEST_T(queues_waitfree_spsc_queue, blocking_push,
       sandbox::queues::waitfree_spsc_queue<int>);

TEST_T(queues_waitfree_spsc_queue, blocking_pop,
       sandbox::queues::waitfree_spsc_queue<int>);

TEST_T(queues_waitfree_spsc_queue, capacity_one,
       sandbox::queues::waitfree_spsc_queue<int>);

TEST_T(queues_waitfree_spsc_queue, queue_destructor,
       sandbox::queues::waitfree_spsc_queue<std::shared_ptr<int>>);

TEST_T(queues_waitfree_spsc_queue, item_destructor,
       sandbox::queues::waitfree_spsc_queue<std::shared_ptr<int>>);

TEST_T(queues_waitfree_spsc_queue, non_copyable_item_type,
       sandbox::queues::waitfree_spsc_queue<std::unique_ptr<int>>);

using allocator_queue_t = sandbox::queues::waitfree_spsc_queue<
    double, std::size_t, 2,
    sandbox::utils::leak_detector_allocator<
        sandbox::queues::detail::waitfree_spsc_queue_entry<double>, true>>;
TEST_T(queues_waitfree_spsc_queue, allocator, allocator_queue_t);

class queues_waitfree_spsc_queue_workload
    : public ::testing::TestWithParam<std::tuple<
          std::size_t /* items_size */, std::size_t /* queue_size */,
          std::size_t /* producers */, std::size_t /* consumers */>> {};

TEST_P_T(queues_waitfree_spsc_queue_workload, nonblocking,
         sandbox::queues::waitfree_spsc_queue<int>);

TEST_P_T(queues_waitfree_spsc_queue_workload, blocking,
         sandbox::queues::waitfree_spsc_queue<int>);

INSTANTIATE_TEST_SUITE_P(queues_waitfree_spsc_queue_workload,
                         queues_waitfree_spsc_queue_workload,
                         ::testing::Values(std::make_tuple(5, 4, 1, 1),
                                           std::make_tuple(20, 16, 1, 1),
                                           std::make_tuple(100, 16, 1, 1),
                                           std::make_tuple(30, 8, 1, 1),
                                           std::make_tuple(10000, 128, 1, 1)));

TEST(queues_waitfree_spsc_queue, queued_producers_nonblocking) { GTEST_SKIP(); }

TEST(queues_waitfree_spsc_queue, queued_producers_blocking) { GTEST_SKIP(); }

TEST(queues_waitfree_spsc_queue, queued_consumers_nonblocking) { GTEST_SKIP(); }

TEST(queues_waitfree_spsc_queue, queued_consumers_blocking) { GTEST_SKIP(); }

TEST_T(queues_waitfree_spsc_queue, rollover,
       sandbox::queues::waitfree_spsc_queue<int>);

}  // namespace tests::queues
