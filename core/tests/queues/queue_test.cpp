#include "queues/queue.hpp"

#include <gtest/gtest.h>

#include "queues/locked_mpmc_queue.hpp"
#include "queues/locked_std_deque.hpp"
#include "queues/lockfree_mpmc_queue.hpp"
#include "queues/lockfree_mpsc_queue.hpp"
#include "queues/lockfree_spmc_queue.hpp"
#include "queues/lockless_mpmc_queue.hpp"
#include "queues/single_threaded_queue.hpp"
#include "queues/waitfree_spsc_queue.hpp"

namespace tests::queues {

TEST(queues_queue, concept) {
  static_assert(core::queues::concepts::noexcept_queue<
                core::queues::locked_mpmc_queue<int>>);
  static_assert(
      core::queues::concepts::queue<core::queues::locked_std_deque<int>>);
  static_assert(core::queues::concepts::noexcept_queue<
                core::queues::lockfree_mpmc_queue<int>>);
  static_assert(core::queues::concepts::noexcept_queue<
                core::queues::lockfree_mpsc_queue<int>>);
  static_assert(core::queues::concepts::noexcept_queue<
                core::queues::lockfree_spmc_queue<int>>);
  static_assert(core::queues::concepts::noexcept_queue<
                core::queues::lockless_mpmc_queue<int>>);
  static_assert(
      core::queues::concepts::queue<core::queues::single_threaded_queue<int>>);
  static_assert(core::queues::concepts::noexcept_queue<
                core::queues::waitfree_spsc_queue<int>>);
}

}  // namespace tests::queues
