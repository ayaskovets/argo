#include <argo/executor.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/executors/thread_pool_executor.hpp>

#include <coroutine>

#include <flow/mpmc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(executor, inline_executor) {
  using executor_t = argo::executors::inline_executor<
      flow::mpmc_queue<std::coroutine_handle<>>,
      flow::mpmc_queue<std::coroutine_handle<>>>;

  static_assert(argo::concepts::executor<executor_t>);
}

TEST(executor, thread_pool_executor) {
  using executor_t = argo::executors::thread_pool_executor<
      flow::mpmc_queue<std::coroutine_handle<>>,
      flow::mpmc_queue<std::coroutine_handle<>>>;

  static_assert(argo::concepts::executor<executor_t>);
}

}  // namespace tests::unit
