#include <argo/awaiters/suspend_never_awaiter.hpp>

#include <gtest/gtest.h>
#include <swiss/coroutine.hpp>

namespace tests::unit::awaiters {

TEST(suspend_never_awaiter, smoke) {
  auto const task = []() -> swiss::task {
    co_await argo::awaiters::suspend_never();
  }();

  EXPECT_FALSE(task.handle().done());
  task.handle().resume();
  EXPECT_TRUE(task.handle().done());
}

}  // namespace tests::unit::awaiters
