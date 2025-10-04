#include "async/channel.hpp"

#include <gtest/gtest.h>

#include "queues/locked_mpmc_queue.hpp"
#include "queues/locked_std_deque.hpp"
#include "queues/lockfree_mpmc_queue.hpp"
#include "queues/lockfree_mpsc_queue.hpp"
#include "queues/lockfree_spmc_queue.hpp"
#include "queues/lockless_mpmc_queue.hpp"
#include "queues/single_threaded_queue.hpp"
#include "queues/waitfree_spsc_queue.hpp"

namespace tests::async {

TEST(async_channel, queues) {
  static_assert(
      core::async::concepts::channel<core::queues::locked_mpmc_queue<int>>);
  static_assert(
      core::async::concepts::channel<core::queues::locked_std_deque<int>>);
  static_assert(
      core::async::concepts::channel<core::queues::lockfree_mpmc_queue<int>>);
  static_assert(
      core::async::concepts::channel<core::queues::lockfree_mpsc_queue<int>>);
  static_assert(
      core::async::concepts::channel<core::queues::lockfree_spmc_queue<int>>);
  static_assert(
      core::async::concepts::channel<core::queues::lockless_mpmc_queue<int>>);
  static_assert(
      core::async::concepts::channel<core::queues::single_threaded_queue<int>>);
  static_assert(
      core::async::concepts::channel<core::queues::waitfree_spsc_queue<int>>);
}

}  // namespace tests::async
