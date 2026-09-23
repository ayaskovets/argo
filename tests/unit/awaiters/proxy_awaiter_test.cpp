#include <argo/awaiters/continuation_awaiter.hpp>
#include <argo/awaiters/proxy_awaiter.hpp>
#include <argo/task.hpp>

#include <gtest/gtest.h>

namespace tests::unit::awaiters {

TEST(proxy_awaiter, smoke) {
  auto task = []() -> argo::task<void> {
    co_await argo::awaiters::proxy(argo::awaiters::continuation(nullptr),
                                   std::identity());
  }();

  task.resume();
  EXPECT_FALSE(task.done());

  task.resume();
  EXPECT_TRUE(task.done());
}

}  // namespace tests::unit::awaiters
