#include <argo/awaiters/continuation_awaiter.hpp>

#include <gtest/gtest.h>
#include <swiss/coroutine.hpp>

namespace tests::unit::awaiters {
// NOLINTBEGIN(cppcoreguidelines-avoid-reference-coroutine-parameters)

TEST(continuation_awaiter, smoke) {
  std::string log;

  std::coroutine_handle<> this_handle = nullptr;
  std::coroutine_handle<> that_handle = nullptr;

  auto const this_task =
      [](std::string & log,
         std::coroutine_handle<> & continuation) -> swiss::task {
    log.append("1");
    co_await argo::awaiters::continuation(continuation);
    log.append("3");
  }(log, that_handle);

  auto const that_task =
      [](std::string & log,
         std::coroutine_handle<> & continuation) -> swiss::task {
    log.append("2");
    co_await argo::awaiters::continuation(continuation);
    log.append("4");
  }(log, this_handle);

  this_handle = this_task.handle();
  that_handle = that_task.handle();

  EXPECT_TRUE(log.empty());

  this_handle.resume();
  EXPECT_EQ(log, "123");

  that_handle.resume();
  EXPECT_EQ(log, "1234");

  EXPECT_TRUE(this_handle.done());
  EXPECT_TRUE(that_handle.done());
}

// NOLINTEND(cppcoreguidelines-avoid-reference-coroutine-parameters)
}  // namespace tests::unit::awaiters
