#include <argo/task.hpp>

#include <gtest/gtest.h>

namespace tests::unit {

TEST(task, nonvoid_smoke) {
  auto task = []() -> argo::task<int> { co_return 1; }();

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_EQ(task.get(), 1);
}

TEST(task, void_smoke) {
  auto task = []() -> argo::task<void> { co_return; }();

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_TRUE(task.done());
  task.get();
}

TEST(task, noncopyable_type) {
  auto task = []() -> argo::task<std::unique_ptr<int>> {
    co_return std::make_unique<int>(1);
  }();

  task.resume();
  EXPECT_EQ(*task.get(), 1);
}

TEST(task, move_ctor) {
  auto task = []() -> argo::task<int> { co_return 1; }();

  task.resume();
  EXPECT_TRUE(task.done());

  argo::task<int> move(std::move(task));
  EXPECT_TRUE(move.done());
  EXPECT_EQ(move.get(), 1);
}

TEST(task, move_assignment) {
  auto task  = []() -> argo::task<int> { co_return 1; }();
  auto other = []() -> argo::task<int> { co_return 2; }();

  other.resume();
  EXPECT_TRUE(other.done());

  task = std::move(other);
  EXPECT_TRUE(task.done());
  EXPECT_EQ(task.get(), 2);
}

TEST(task, nonvoid_exception) {
  class exception final : public std::exception {};
  auto task = []() -> argo::task<int> {
    throw exception{};
    co_return 1;
  }();

  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_THROW(task.get(), exception);
}

TEST(task, void_exception) {
  class exception final : public std::exception {};
  auto task = []() -> argo::task<void> {
    throw exception{};
    co_return;
  }();

  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_THROW(task.get(), exception);
}

TEST(task, awaiter_smoke) {
  auto task = []() -> argo::task<int> {
    // NOLINTBEGIN(readability-magic-numbers)
    auto const one = co_await []() -> argo::task<float> { co_return 0.5F; }();
    auto const two = co_await []() -> argo::task<double> { co_return 2.6; }();
    // NOLINTEND(readability-magic-numbers)
    co_return static_cast<int>(one + two);
  }();

  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_EQ(task.get(), 3);
}

TEST(task, awaiter_exception) {
  class exception final : public std::exception {};
  auto task = []() -> argo::task<int> {
    auto const one = co_await []() -> argo::task<int> { co_return 1; }();
    auto const two = co_await []() -> argo::task<int> { throw exception{}; }();
    co_return one + two;
  }();

  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_THROW(task.get(), exception);
}

TEST(task, awaiter_recursion) {
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

  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_EQ(task.get(), 131328);
}

}  // namespace tests::unit
