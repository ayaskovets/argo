#include <argo/awaiters/fork_awaiter.hpp>
#include <argo/execution_context.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit::awaiters {

TEST(fork_awaiter, unfinished) {
  auto child     = []() -> argo::task<void> { co_return; }();
  auto child_ref = argo::task_ref(child);

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  auto parent = [](argo::task<void> && child) -> argo::task<void> {
    co_await argo::awaiters::fork_awaiter(std::move(child));
    co_return;
  }(std::move(child));

  EXPECT_FALSE(parent.done());
  EXPECT_FALSE(child_ref.done());
  parent.resume();
  EXPECT_FALSE(parent.done());
  EXPECT_FALSE(child_ref.done());

  child_ref.resume();
  EXPECT_TRUE(parent.done());
}

TEST(fork_awaiter, finished) {
  auto child     = []() -> argo::task<void> { co_return; }();
  auto child_ref = argo::task_ref(child);

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  auto parent = [](argo::task<void> && child) -> argo::task<void> {
    co_await argo::awaiters::fork_awaiter(std::move(child));
    co_return;
  }(std::move(child));

  EXPECT_FALSE(parent.done());
  EXPECT_FALSE(child_ref.done());
  child_ref.resume();
  EXPECT_FALSE(parent.done());
  EXPECT_TRUE(child_ref.done());

  parent.resume();
  EXPECT_TRUE(parent.done());
}

TEST(fork_awaiter, retain_context) {
  // TODO: impl
}

}  // namespace tests::unit::awaiters
