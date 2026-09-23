#include <argo/task_future.hpp>

#include <gtest/gtest.h>

namespace tests::unit {

TEST(task_future, unfinished) {
  auto child     = []() -> argo::task<int> { co_return 1; }();
  auto child_ref = argo::task_ref(child);

  auto task = [](argo::task<int> child) -> argo::task<void> {
    co_await argo::task_future(std::move(child));
    co_return;
  }(std::move(child));

  EXPECT_FALSE(task.done());
  EXPECT_FALSE(child_ref.done());

  task.resume();
  EXPECT_FALSE(task.done());
  EXPECT_FALSE(child_ref.done());

  child_ref.resume();
  EXPECT_TRUE(task.done());
}

TEST(task_future, finished) {
  auto child     = []() -> argo::task<int> { co_return 1; }();
  auto child_ref = argo::task_ref(child);

  auto task = [](argo::task<int> child) -> argo::task<void> {
    co_await argo::task_future(std::move(child));
    co_return;
  }(std::move(child));

  EXPECT_FALSE(task.done());
  EXPECT_FALSE(child_ref.done());

  child_ref.resume();
  EXPECT_FALSE(task.done());
  EXPECT_TRUE(child_ref.done());

  task.resume();
  EXPECT_TRUE(task.done());
}

}  // namespace tests::unit
