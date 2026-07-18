#include <argo/awaiters/yield_awaiter.hpp>
#include <argo/task.hpp>

#include <gtest/gtest.h>

namespace tests::unit::awaiters {
// NOLINTBEGIN(readability-magic-numbers)

TEST(yield_awaiter, smoke) {
  /**
   * @note Use TLS to capture log in detail::yield
   */
  thread_local std::string log;
  log.clear();

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  auto task = [](std::string & log) -> argo::task<void> {
    log.append("enter;");
    co_await argo::awaiters::yield();
    log.append("exit;");
    co_return;
  }(log);

  argo::this_executor::detail::yield =
      [](std::coroutine_handle<>) noexcept -> void { log.append("yield;"); };

  EXPECT_TRUE(log.empty());

  task.resume();
  EXPECT_EQ(log, "enter;yield;");

  task.resume();
  EXPECT_EQ(log, "enter;yield;exit;");

  EXPECT_TRUE(task.done());
  EXPECT_NO_THROW(task.get());
}

// NOLINTEND(readability-magic-numbers)
}  // namespace tests::unit::awaiters
