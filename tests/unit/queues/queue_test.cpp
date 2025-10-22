#include <sandbox/queues/locked_mpmc_queue.hpp>
#include <sandbox/queues/locked_std_deque.hpp>
#include <sandbox/queues/lockfree_mpmc_queue.hpp>
#include <sandbox/queues/lockfree_mpsc_queue.hpp>
#include <sandbox/queues/lockfree_spmc_queue.hpp>
#include <sandbox/queues/lockless_mpmc_queue.hpp>
#include <sandbox/queues/queue.hpp>
#include <sandbox/queues/single_threaded_queue.hpp>
#include <sandbox/queues/waitfree_spsc_queue.hpp>

#include <gtest/gtest.h>

namespace tests::queues {

TEST(queues_queue, concept) {
  static_assert(sandbox::queues::concepts::noexcept_queue<
                sandbox::queues::locked_mpmc_queue<int>>);
  static_assert(
      sandbox::queues::concepts::queue<sandbox::queues::locked_std_deque<int>>);
  static_assert(sandbox::queues::concepts::noexcept_queue<
                sandbox::queues::lockfree_mpmc_queue<int>>);
  static_assert(sandbox::queues::concepts::noexcept_queue<
                sandbox::queues::lockfree_mpsc_queue<int>>);
  static_assert(sandbox::queues::concepts::noexcept_queue<
                sandbox::queues::lockfree_spmc_queue<int>>);
  static_assert(sandbox::queues::concepts::noexcept_queue<
                sandbox::queues::lockless_mpmc_queue<int>>);
  static_assert(sandbox::queues::concepts::queue<
                sandbox::queues::single_threaded_queue<int>>);
  static_assert(sandbox::queues::concepts::noexcept_queue<
                sandbox::queues::waitfree_spsc_queue<int>>);
}

}  // namespace tests::queues
