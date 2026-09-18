#include <argo/awaiters/continuation_awaiter.hpp>

#include <gtest/gtest.h>

namespace tests::unit::awaiters {
// NOLINTBEGIN(cppcoreguidelines-avoid-reference-coroutine-parameters)

namespace {

/**
 * @note Mock implementation of coroutine return object
 */
struct task final {
  struct promise_type final {
    constexpr auto get_return_object() noexcept -> task {
      return task(std::coroutine_handle<promise_type>::from_promise(*this));
    }
    constexpr auto initial_suspend() noexcept -> std::suspend_always {
      return {};
    }
    constexpr auto final_suspend() noexcept -> std::suspend_always {
      return {};
    }
    constexpr void unhandled_exception() noexcept {}
    constexpr void return_void() noexcept {}
  };
  std::coroutine_handle<promise_type> handle;
};

}  // namespace

TEST(continuation_awaiter, smoke) {
  std::coroutine_handle<> this_handle = nullptr;
  std::coroutine_handle<> that_handle = nullptr;

  auto const this_task = [](std::coroutine_handle<> & continuation) -> task {
    co_await argo::awaiters::continuation(continuation);
  }(that_handle);

  auto const that_task = [](std::coroutine_handle<> & continuation) -> task {
    co_await argo::awaiters::continuation(continuation);
  }(this_handle);

  this_handle = this_task.handle;
  that_handle = that_task.handle;

  this_task.handle.resume();
  EXPECT_TRUE(this_task.handle.done());
  EXPECT_FALSE(that_task.handle.done());

  that_task.handle.resume();
  EXPECT_TRUE(this_task.handle.done());
  EXPECT_TRUE(that_task.handle.done());
}

// NOLINTEND(cppcoreguidelines-avoid-reference-coroutine-parameters)
}  // namespace tests::unit::awaiters
