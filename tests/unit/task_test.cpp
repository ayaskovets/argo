#include <argo/task.hpp>

#include <gtest/gtest.h>

namespace tests::unit {

namespace {

/**
 * @note Mock implementation of std::suspend_always awaiter
 */
struct suspend_always final {
  constexpr auto await_ready() const noexcept -> bool { return false; }
  constexpr void await_suspend(auto &&) const noexcept {}
  constexpr void await_resume() const noexcept {}
};

}  // namespace

TEST(task, nonvoid_smoke) {
  auto task = []() -> argo::task<int> {
    co_await suspend_always{};
    co_return 1;
  }();

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_FALSE(task.done());

  task();
  EXPECT_TRUE(task.done());
  EXPECT_EQ(task.get(), 1);
}

TEST(task, void_smoke) {
  auto task = []() -> argo::task<void> { co_await suspend_always{}; }();

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_FALSE(task.done());

  task();
  EXPECT_TRUE(task.done());
  task.get();
}

TEST(task, null_handle) {
  argo::task<void> task(nullptr);

  EXPECT_TRUE(task.done());
  EXPECT_ANY_THROW(task.resume());
  EXPECT_ANY_THROW(task.get());
}

TEST(task, noncopyable_type) {
  auto task = []() -> argo::task<std::unique_ptr<int>> {
    co_return std::make_unique<int>(1);
  }();

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_EQ(*task.get(), 1);
}

TEST(task, move_ctor) {
  auto task = []() -> argo::task<int> { co_return 1; }();

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_TRUE(task);

  auto move = std::move(task);
  EXPECT_FALSE(task);
  EXPECT_TRUE(move);
  EXPECT_TRUE(task.done());
  EXPECT_ANY_THROW(task.resume());
  EXPECT_TRUE(move.done());
  EXPECT_EQ(std::move(move).get(), 1);
}

TEST(task, move_assignment) {
  auto task  = []() -> argo::task<int> { co_return 1; }();
  auto other = []() -> argo::task<int> { co_return 2; }();

  other.resume();
  EXPECT_TRUE(other.done());
  EXPECT_FALSE(task.done());
  EXPECT_TRUE(task);

  task = std::move(other);
  EXPECT_TRUE(task);
  EXPECT_FALSE(other);
  EXPECT_TRUE(other.done());
  EXPECT_ANY_THROW(other.resume());
  EXPECT_TRUE(task.done());
  EXPECT_EQ(task.get(), 2);
}

TEST(task, nonvoid_exception) {
  class exception final : public std::exception {};

  auto task = []() -> argo::task<int> {
    throw exception{};
    co_return 1;
  }();

  EXPECT_FALSE(task.done());
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

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_THROW(task.get(), exception);
}

TEST(task, awaiter_smoke) {
  auto task = []() -> argo::task<int> {
    auto const one = co_await []() -> argo::task<int> { co_return 1; }();
    auto const two = co_await []() -> argo::task<int> { co_return 2; }();
    co_return one + two;
  }();

  EXPECT_FALSE(task.done());
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

  EXPECT_FALSE(task.done());
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

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_EQ(task.get(), 131328);
}

}  // namespace tests::unit
