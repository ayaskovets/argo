#include <argo/awaiters/task_awaiter.hpp>
#include <argo/task.hpp>

#include <gtest/gtest.h>

namespace tests::unit::awaiters {

TEST(task_awaiter, smoke) {
  auto task = []() -> argo::task<int> {
    auto const one = co_await []() -> argo::task<int> { co_return 1; }();
    auto const two = co_await []() -> argo::task<int> { co_return 2; }();
    co_return one + two;
  }();

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_EQ(std::move(task).get(), 3);
}

TEST(task_awaiter, exception) {
  class exception final : public std::exception {};

  auto task = []() -> argo::task<int> {
    auto const one = co_await []() -> argo::task<int> { co_return 1; }();
    auto const two = co_await []() -> argo::task<int> { throw exception{}; }();
    co_return one + two;
  }();

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_THROW(std::move(task).get(), exception);
}

TEST(task_awaiter, recursion) {
  std::function<argo::task<int>(int)> const recurse =
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
      [&](int depth) -> argo::task<int> {
    if (depth <= 0) {
      co_return 0;
    } else {
      co_return depth + co_await recurse(depth - 1);
    }
  };

  // NOLINTNEXTLINE(readability-magic-numbers)
  auto task = recurse(512);

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_EQ(std::move(task).get(), 131328);
}

}  // namespace tests::unit::awaiters
