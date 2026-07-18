#include <argo/awaiters/continuation_awaiter.hpp>
#include <argo/task.hpp>

#include <coroutine>

#include <gtest/gtest.h>

namespace tests::unit::awaiters {
// NOLINTBEGIN(readability-magic-numbers)
// NOLINTBEGIN(cppcoreguidelines-avoid-reference-coroutine-parameters)

TEST(continuation_awaiter, smoke) {
  std::string log;
  std::coroutine_handle<> fst_handle = nullptr;
  std::coroutine_handle<> snd_handle = nullptr;

  auto const fst =
      [](std::string & log,
         std::coroutine_handle<> & snd_handle) -> argo::task<void> {
    log.append("1");
    co_await argo::awaiters::continuation(snd_handle);
    log.append("3");
    co_return;
  }(log, snd_handle);

  auto const snd =
      [](std::string & log,
         std::coroutine_handle<> & fst_handle) -> argo::task<void> {
    log.append("2");
    co_await argo::awaiters::continuation(fst_handle);
    log.append("4");
    co_return;
  }(log, fst_handle);

  fst_handle = fst.handle();
  snd_handle = snd.handle();

  EXPECT_TRUE(log.empty());

  fst_handle.resume();
  EXPECT_EQ(log, "123");

  snd_handle.resume();
  EXPECT_EQ(log, "1234");
}

// NOLINTEND(cppcoreguidelines-avoid-reference-coroutine-parameters)
// NOLINTEND(readability-magic-numbers)
}  // namespace tests::unit::awaiters
