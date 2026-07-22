#include <argo/awaiters/proxy_awaiter.hpp>

#include <coroutine>
#include <functional>

#include <gtest/gtest.h>
#include <swiss/coroutine.hpp>

#include "argo/awaiters/continuation_awaiter.hpp"

namespace tests::unit::awaiters {
// NOLINTBEGIN(cppcoreguidelines-avoid-reference-coroutine-parameters)

namespace {

struct global_operator_awaiter final {
  std::coroutine_handle<> handle;
};
constexpr auto operator co_await(global_operator_awaiter awaiter)
    -> argo::awaiters::continuation_awaiter {
  return argo::awaiters::continuation(awaiter.handle);
}

struct member_operator_awaiter final {
  constexpr auto operator co_await() const noexcept
      -> argo::awaiters::continuation_awaiter {
    return argo::awaiters::continuation(handle);
  }
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::coroutine_handle<> handle;
};

}  // namespace

TEST(proxy_awaiter, smoke) {
  std::string log;

  auto const task = [](std::string & log) -> swiss::task {
    log.append("1");
    co_await argo::awaiters::proxy(argo::awaiters::continuation(nullptr),
                                   std::identity());
    log.append("2");
    co_await argo::awaiters::proxy(member_operator_awaiter(nullptr),
                                   std::identity());
    log.append("3");
    co_await argo::awaiters::proxy(global_operator_awaiter(nullptr),
                                   std::identity());
    log.append("4");
  }(log);

  EXPECT_TRUE(log.empty());

  task.handle().resume();
  EXPECT_EQ(log, "1");
  task.handle().resume();
  EXPECT_EQ(log, "12");
  task.handle().resume();
  EXPECT_EQ(log, "123");
  task.handle().resume();
  EXPECT_EQ(log, "1234");

  EXPECT_TRUE(task.handle().done());
}

// NOLINTEND(cppcoreguidelines-avoid-reference-coroutine-parameters)
}  // namespace tests::unit::awaiters
