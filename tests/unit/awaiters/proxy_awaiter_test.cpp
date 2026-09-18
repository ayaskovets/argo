#include <argo/awaiters/continuation_awaiter.hpp>
#include <argo/awaiters/proxy_awaiter.hpp>
#include <argo/task.hpp>

#include <gtest/gtest.h>

namespace tests::unit::awaiters {

namespace {

struct global_operator_awaiter final {};
constexpr auto operator co_await(global_operator_awaiter)
    -> argo::awaiters::continuation_awaiter {
  return argo::awaiters::continuation(nullptr);
}

struct member_operator_awaiter final {
  constexpr auto operator co_await() const noexcept
      -> argo::awaiters::continuation_awaiter {
    return argo::awaiters::continuation(nullptr);
  }
};

}  // namespace

TEST(proxy_awaiter, smoke) {
  auto task = []() -> argo::task<void> {
    co_await argo::awaiters::proxy(argo::awaiters::continuation(nullptr),
                                   std::identity());
    co_await argo::awaiters::proxy(member_operator_awaiter(), std::identity());
    co_await argo::awaiters::proxy(global_operator_awaiter(), std::identity());
  }();

  task.resume();
  EXPECT_FALSE(task.done());

  task.resume();
  EXPECT_FALSE(task.done());

  task.resume();
  EXPECT_FALSE(task.done());

  task.resume();
  EXPECT_TRUE(task.done());
}

}  // namespace tests::unit::awaiters
