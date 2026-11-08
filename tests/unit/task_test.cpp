#include <argo/awaiters/yield_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <semaphore>
#include <thread>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(task, nonvoid_smoke) {
  auto task = []() -> argo::task<int> { co_return 1; }();

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_EQ(task.get(), 1);
}

TEST(task, void_smoke) {
  auto task = []() -> argo::task<void> { co_return; }();

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_TRUE(task.done());
  task.get();
}

TEST(task, noncopyable_type) {
  auto task = []() -> argo::task<std::unique_ptr<int>> {
    co_return std::make_unique<int>(1);
  }();

  task.resume();
  EXPECT_EQ(*task.get(), 1);
}

TEST(task, move_ctor) {
  auto task = []() -> argo::task<int> { co_return 1; }();

  task.resume();
  EXPECT_TRUE(task.done());

  argo::task<int> move(std::move(task));
  EXPECT_TRUE(move.done());
  EXPECT_EQ(move.get(), 1);
}

TEST(task, move_assignment) {
  auto task  = []() -> argo::task<int> { co_return 1; }();
  auto other = []() -> argo::task<int> { co_return 2; }();

  other.resume();
  EXPECT_TRUE(other.done());

  task = std::move(other);
  EXPECT_TRUE(task.done());
  EXPECT_EQ(task.get(), 2);
}

TEST(task, nonvoid_exception) {
  class exception final : public std::exception {};
  auto task = []() -> argo::task<int> {
    throw exception{};
    co_return 1;
  }();

  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_THROW(task.get(), exception);
}

TEST(task, void_exception) {
  class exception final : public std::exception {};
  auto task = []() -> argo::task<void> {
    throw exception{};
    co_return;
  }();

  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_THROW(task.get(), exception);
}

TEST(task, awaiter_smoke) {
  auto task = []() -> argo::task<int> {
    // NOLINTBEGIN(readability-magic-numbers)
    auto const one = co_await []() -> argo::task<float> { co_return 0.5F; }();
    auto const two = co_await []() -> argo::task<double> { co_return 2.6; }();
    // NOLINTEND(readability-magic-numbers)
    co_return static_cast<int>(one + two);
  }();

  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_EQ(task.get(), 3);
}

TEST(task, awaiter_exception) {
  class exception final : public std::exception {};
  auto task = []() -> argo::task<int> {
    auto const one = co_await []() -> argo::task<int> { co_return 1; }();
    auto const two = co_await []() -> argo::task<int> { throw exception{}; }();
    co_return one + two;
  }();

  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_THROW(task.get(), exception);
}

TEST(task, awaiter_recursion) {
  std::function<argo::task<int>(int)> const recurse =
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
      [&](int depth) -> argo::task<int> {
    if (depth <= 0) {
      co_return 0;
    } else {
      co_return depth + co_await recurse(depth - 1);
    }
  };
  // NOLINTNEXTLINE(readability-magic-numbers)
  auto task = recurse(512);

  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_EQ(task.get(), 131328);
}

TEST(task, resume_cancelled) {
  bool started = false;
  auto task    = [](bool * started) -> argo::task<int> {
    *started = true;
    co_return 1;
  }(&started);

  task.request_cancel();
  EXPECT_FALSE(task.done());
  EXPECT_THROW(task.get(), argo::promise_cancelled);

  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_FALSE(started);
  EXPECT_THROW(task.get(), argo::promise_cancelled);
}

TEST(task, cancel_detached_task) {
  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  argo::execution_token token;
  bool started = false;
  {
    auto task = [](bool * started) -> argo::task<void> {
      *started = true;
      co_return;
    }(&started);
    task.request_cancel();
    token = task.get_execution_token();
  }

  in_channel.push(std::move(token));
  EXPECT_TRUE(executor.run_once());
  EXPECT_FALSE(started);
}

TEST(task, await_cancelled_task) {
  bool started = false;
  auto child   = [](bool * started) -> argo::task<int> {
    *started = true;
    co_return 1;
  }(&started);
  child.request_cancel();

  auto parent = [](argo::task<int> child) -> argo::task<int> {
    try {
      co_return co_await std::move(child);
    } catch (argo::promise_cancelled const &) {
      co_return 2;
    }
  }(std::move(child));

  parent.resume();
  EXPECT_TRUE(parent.done());
  EXPECT_FALSE(started);
  EXPECT_EQ(parent.get(), 2);
}

TEST(task, await_resume_cancelled_task) {
  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  bool continued = false;
  bool caught    = false;
  auto task      = [](bool * continued, bool * caught) -> argo::task<int> {
    try {
      co_await argo::awaiters::yield();
      *continued = true;
    } catch (argo::promise_cancelled const &) {
      *caught = true;
    }
    co_return 1;
  }(&continued, &caught);

  in_channel.push(task.get_execution_token());
  EXPECT_TRUE(executor.run_once());

  task.request_cancel();
  EXPECT_FALSE(task.done());
  EXPECT_FALSE(caught);

  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(task.done());
  EXPECT_TRUE(caught);
  EXPECT_FALSE(continued);
  EXPECT_THROW(task.get(), argo::promise_cancelled);
}

TEST(task, cancel_waiting_parent) {
  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  bool completed = false;
  auto child     = [](bool * completed) -> argo::task<int> {
    co_await argo::awaiters::yield();
    *completed = true;
    co_return 1;
  }(&completed);

  bool continued = false;
  auto parent = [](argo::task<int> child, bool * continued) -> argo::task<int> {
    int const value = co_await std::move(child);
    *continued      = true;
    co_return value;
  }(std::move(child), &continued);

  in_channel.push(parent.get_execution_token());
  EXPECT_TRUE(executor.run_once());

  parent.request_cancel();
  EXPECT_FALSE(parent.done());
  EXPECT_FALSE(completed);
  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(completed);
  EXPECT_FALSE(continued);
  EXPECT_TRUE(parent.done());
  EXPECT_THROW(parent.get(), argo::promise_cancelled);
}

TEST(task, suspended_lifetime) {
  class awaiter final {
   public:
    constexpr auto await_ready() const noexcept -> bool { return false; }
    constexpr void await_suspend() const noexcept {}
    constexpr void await_resume() const noexcept {}
  };

  std::weak_ptr<int> lifetime;
  {
    auto value = std::make_shared<int>(1);
    lifetime   = value;
    auto task  = [](std::shared_ptr<int>) -> argo::task<void> {
      co_await awaiter();
    }(std::move(value));
    task.resume();
    EXPECT_FALSE(task.done());
    EXPECT_FALSE(lifetime.expired());
  }
  EXPECT_TRUE(lifetime.expired());
}

TEST(task, continuation_suspend) {
  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);
  std::weak_ptr<int> lifetime;
  int result = 0;

  {
    auto value = std::make_shared<int>(1);
    lifetime   = value;
    auto task  = [](std::shared_ptr<int> value,
                   int * result) -> argo::task<void> {
      co_await []() -> argo::task<void> { co_await argo::awaiters::yield(); }();
      *result += *value;
      co_await argo::awaiters::yield();
      *result += *value;
    }(std::move(value), &result);
    in_channel.push(task.get_execution_token());
  }

  EXPECT_TRUE(executor.run_once());
  EXPECT_FALSE(lifetime.expired());
  EXPECT_EQ(result, 0);
  EXPECT_TRUE(executor.run_once());
  EXPECT_FALSE(lifetime.expired());
  EXPECT_EQ(result, 1);
  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(lifetime.expired());
  EXPECT_EQ(result, 2);
}

TEST(task, running_lifetime) {
  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  std::optional<argo::task<void>> owner;
  auto value                  = std::make_shared<int>(1);
  std::weak_ptr<int> lifetime = value;
  bool completed              = false;
  owner.emplace([](std::shared_ptr<int>,
                   std::optional<argo::task<void>> * owner,
                   bool * completed) -> argo::task<void> {
    owner->reset();
    co_await argo::awaiters::yield();
    *completed = true;
  }(std::move(value), &owner, &completed));

  in_channel.push(owner->get_execution_token());
  EXPECT_TRUE(executor.run_once());
  EXPECT_FALSE(owner.has_value());
  EXPECT_FALSE(lifetime.expired());
  EXPECT_FALSE(completed);
  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(lifetime.expired());
  EXPECT_TRUE(completed);
}

TEST(task, resume_during_suspend) {
  class awaiter final {
   public:
    constexpr awaiter(std::binary_semaphore * published,
                      flow::spsc_queue<argo::execution_token> * channel,
                      std::binary_semaphore * release) noexcept
        : channel_(channel), published_(published), release_(release) {}

    constexpr auto await_ready() const noexcept -> bool { return false; }
    void await_suspend(argo::execution_token token) const noexcept {
      auto * const ready = published_;
      auto * const wait  = release_;
      channel_->push(std::move(token));
      ready->release();
      wait->acquire();
    }
    constexpr void await_resume() const noexcept {}

   private:
    flow::spsc_queue<argo::execution_token> * channel_;
    std::binary_semaphore * published_;
    std::binary_semaphore * release_;
  };

  constexpr auto kTimeout = std::chrono::seconds(5);
  flow::spsc_queue<argo::execution_token> first_channel(1);
  flow::spsc_queue<argo::execution_token> pending_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor first_executor(first_channel, out_channel);
  argo::executors::inline_executor pending_executor(pending_channel,
                                                    out_channel);
  std::binary_semaphore published(0);
  std::binary_semaphore release(0);
  std::weak_ptr<int> lifetime;
  bool completed = false;

  {
    auto value = std::make_shared<int>(1);
    lifetime   = value;
    awaiter operation(&published, &pending_channel, &release);
    auto task = [](std::shared_ptr<int>, awaiter operation,
                   bool * completed) -> argo::task<void> {
      co_await operation;
      *completed = true;
    }(std::move(value), operation, &completed);
    first_channel.push(task.get_execution_token());
  }

  std::jthread thread([&] -> void { EXPECT_TRUE(first_executor.run_once()); });
  if (!published.try_acquire_for(kTimeout)) {
    release.release();
    thread.join();
    FAIL() << "Awaiter did not publish its token";
  }
  EXPECT_TRUE(pending_executor.run_once());
  EXPECT_TRUE(completed);
  EXPECT_FALSE(lifetime.expired());
  release.release();
  thread.join();
  EXPECT_TRUE(lifetime.expired());
}

}  // namespace tests::unit
