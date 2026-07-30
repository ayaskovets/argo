#include <argo/awaiters/continuation_awaiter.hpp>
#include <argo/awaiters/proxy_awaiter.hpp>

#include <gtest/gtest.h>
#include <swiss/coroutine.hpp>

namespace tests::unit::awaiters {

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
  auto const task = []() -> swiss::task {
    co_await argo::awaiters::proxy(argo::awaiters::continuation(nullptr),
                                   std::identity());
    co_await argo::awaiters::proxy(member_operator_awaiter(nullptr),
                                   std::identity());
    co_await argo::awaiters::proxy(global_operator_awaiter(nullptr),
                                   std::identity());
  }();

  task.handle().resume();
  EXPECT_FALSE(task.handle().done());

  task.handle().resume();
  EXPECT_FALSE(task.handle().done());

  task.handle().resume();
  EXPECT_FALSE(task.handle().done());

  task.handle().resume();
  EXPECT_TRUE(task.handle().done());
}

}  // namespace tests::unit::awaiters
