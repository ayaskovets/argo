#include <argo/executor.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/executors/thread_pool_executor.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(executor, inline_executor) {
  using executor_type = argo::executors::inline_executor<flow::spsc_queue<int>,
                                                         flow::spsc_queue<int>>;

  static_assert(argo::concepts::executor<executor_type>);
}

TEST(executor, thread_pool_executor) {
  using executor_type =
      argo::executors::thread_pool_executor<flow::spsc_queue<int>,
                                            flow::spsc_queue<int>>;

  static_assert(argo::concepts::executor<executor_type>);
}

}  // namespace tests::unit
