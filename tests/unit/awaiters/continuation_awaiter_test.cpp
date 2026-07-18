#include <argo/awaiters/continuation_awaiter.hpp>
#include <argo/task.hpp>

#include <gtest/gtest.h>

namespace tests::unit::awaiters {
// NOLINTBEGIN(readability-magic-numbers)
// NOLINTBEGIN(cppcoreguidelines-avoid-reference-coroutine-parameters)

TEST(continuation_awaiter, smoke) {
  auto const make_task =
      [](int taskid, std::string & log,
         argo::const_coroutine_handle<> & continuation) -> argo::task<void> {
    log.append(std::format("{}::enter;", taskid));
    co_await argo::awaiters::continuation(continuation);
    log.append(std::format("{}::exit;", taskid));
    co_return;
  };

  std::string log;

  argo::const_coroutine_handle<> task1_handle = nullptr;
  argo::const_coroutine_handle<> task2_handle = nullptr;
  auto task1                                  = make_task(1, log, task2_handle);
  auto task2                                  = make_task(2, log, task1_handle);
  task1_handle                                = task1.handle();
  task2_handle                                = task2.handle();

  EXPECT_TRUE(log.empty());

  task1.resume();
  EXPECT_EQ(log, "1::enter;2::enter;1::exit;");

  task2.resume();
  EXPECT_EQ(log, "1::enter;2::enter;1::exit;2::exit;");

  EXPECT_TRUE(task1.done());
  EXPECT_NO_THROW(task1.get());

  EXPECT_TRUE(task2.done());
  EXPECT_NO_THROW(task2.get());
}

// NOLINTEND(cppcoreguidelines-avoid-reference-coroutine-parameters)
// NOLINTEND(readability-magic-numbers)
}  // namespace tests::unit::awaiters
