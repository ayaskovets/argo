#include <wip/queues/locked_mpmc_queue.hpp>
#include <wip/queues/locked_std_deque.hpp>
#include <wip/queues/lockfree_mpmc_queue.hpp>
#include <wip/queues/lockfree_mpsc_queue.hpp>
#include <wip/queues/lockfree_spmc_queue.hpp>
#include <wip/queues/lockless_mpmc_queue.hpp>
#include <wip/queues/queue.hpp>
#include <wip/queues/single_threaded_queue.hpp>
#include <wip/queues/waitfree_spsc_queue.hpp>

#include <gtest/gtest.h>

namespace tests::queues {

namespace queues = wip::queues;

TEST(queues_queue, concept) {
  static_assert(
      queues::concepts::noexcept_queue<queues::locked_mpmc_queue<int>>);
  static_assert(queues::concepts::queue<queues::locked_std_deque<int>>);
  static_assert(
      queues::concepts::noexcept_queue<queues::lockfree_mpmc_queue<int>>);
  static_assert(
      queues::concepts::noexcept_queue<queues::lockfree_mpsc_queue<int>>);
  static_assert(
      queues::concepts::noexcept_queue<queues::lockfree_spmc_queue<int>>);
  static_assert(
      queues::concepts::noexcept_queue<queues::lockless_mpmc_queue<int>>);
  static_assert(queues::concepts::queue<queues::single_threaded_queue<int>>);
  static_assert(
      queues::concepts::noexcept_queue<queues::waitfree_spsc_queue<int>>);
}

}  // namespace tests::queues
