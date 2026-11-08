#include <argo/executors/inline_executor.hpp>
#include <argo/promise_ctrl.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(promise_ctrl, refcount_lifetime) {
  class awaiter final {
   public:
    constexpr void await_suspend() const noexcept {}
  } operation;

  argo::promise_ctrl ctrl(std::noop_coroutine());
  ctrl.add_ref();
  {
    auto token = ctrl.lock_resume();
    EXPECT_EQ(ctrl.use_count(), 2);
    EXPECT_THROW(ctrl.lock_resume(), std::runtime_error);
  }
  EXPECT_EQ(ctrl.use_count(), 1);

  ctrl.resume();
  EXPECT_EQ(ctrl.use_count(), 2);
  EXPECT_THROW(ctrl.lock_resume(), std::runtime_error);
  ctrl.await_suspend(operation);
  EXPECT_EQ(ctrl.use_count(), 1);

  ctrl.resume();
  EXPECT_EQ(ctrl.use_count(), 2);
  ctrl.final_await_suspend();
  EXPECT_TRUE(ctrl.done());
  EXPECT_EQ(ctrl.use_count(), 1);
  EXPECT_THROW(ctrl.resume(), std::runtime_error);
  ctrl.remove_ref();
}

TEST(promise_ctrl, refcount_cancel) {
  argo::promise_ctrl ctrl(std::noop_coroutine());
  ctrl.add_ref();
  {
    auto token = ctrl.lock_resume();
    ctrl.request_cancel();
    EXPECT_EQ(ctrl.use_count(), 2);
  }
  EXPECT_EQ(ctrl.use_count(), 1);
  EXPECT_TRUE(ctrl.cancel_requested());

  ctrl.resume();
  EXPECT_THROW(ctrl.await_resume(), argo::promise_cancelled);
  ctrl.final_await_suspend();
  EXPECT_TRUE(ctrl.done());
  EXPECT_TRUE(ctrl.cancel_requested());
  ctrl.remove_ref();
}

TEST(promise_ctrl, suspend_false) {
  class awaiter final {
   public:
    constexpr awaiter(argo::promise_ctrl & ctrl, bool cancel) noexcept
        : ctrl_(&ctrl), cancel_(cancel) {}

    constexpr auto await_suspend(argo::execution_token) const noexcept -> bool {
      if (cancel_) {
        ctrl_->request_cancel();
      }
      return false;
    }

   private:
    argo::promise_ctrl * ctrl_;
    bool cancel_;
  };

  argo::promise_ctrl ctrl(std::noop_coroutine());
  ctrl.add_ref();
  ctrl.resume();

  awaiter operation(ctrl, false);
  EXPECT_FALSE(ctrl.await_suspend(operation));
  EXPECT_EQ(ctrl.use_count(), 2);
  EXPECT_THROW(ctrl.lock_resume(), std::runtime_error);
  EXPECT_NO_THROW(ctrl.await_resume());

  awaiter cancel(ctrl, true);
  EXPECT_FALSE(ctrl.await_suspend(cancel));
  EXPECT_EQ(ctrl.use_count(), 2);
  EXPECT_THROW(ctrl.lock_resume(), std::runtime_error);
  EXPECT_THROW(ctrl.await_resume(), argo::promise_cancelled);

  ctrl.final_await_suspend();
  EXPECT_TRUE(ctrl.done());
  ctrl.remove_ref();
}

TEST(promise_ctrl, suspend_token) {
  class awaiter final {
   public:
    explicit constexpr awaiter(argo::execution_token & token) noexcept
        : token_(&token) {}

    constexpr void await_suspend(argo::execution_token token) const noexcept {
      *token_ = std::move(token);
    }

   private:
    argo::execution_token * token_;
  };

  argo::promise_ctrl ctrl(std::noop_coroutine());
  ctrl.add_ref();
  ctrl.resume();

  argo::execution_token token;
  awaiter operation(token);
  ctrl.await_suspend(operation);
  EXPECT_EQ(ctrl.use_count(), 2);
  EXPECT_THROW(ctrl.lock_resume(), std::runtime_error);

  std::move(token).release();
  EXPECT_EQ(ctrl.use_count(), 1);
  ctrl.resume();
  ctrl.final_await_suspend();
  EXPECT_TRUE(ctrl.done());
  ctrl.remove_ref();
}

TEST(promise_ctrl, suspend_true) {
  class awaiter final {
   public:
    explicit constexpr awaiter(argo::execution_token & token) noexcept
        : token_(&token) {}

    constexpr auto await_suspend(argo::execution_token token) const noexcept
        -> bool {
      *token_ = std::move(token);
      return true;
    }

   private:
    argo::execution_token * token_;
  };

  argo::promise_ctrl ctrl(std::noop_coroutine());
  ctrl.add_ref();
  ctrl.resume();

  argo::execution_token token;
  awaiter operation(token);
  EXPECT_TRUE(ctrl.await_suspend(operation));
  EXPECT_EQ(ctrl.use_count(), 2);
  EXPECT_THROW(ctrl.lock_resume(), std::runtime_error);

  std::move(token).release();
  EXPECT_EQ(ctrl.use_count(), 1);
  ctrl.resume();
  ctrl.final_await_suspend();
  EXPECT_TRUE(ctrl.done());
  ctrl.remove_ref();
}

TEST(promise_ctrl, suspend_transfer) {
  class awaiter final {
   public:
    explicit constexpr awaiter(argo::execution_token & token) noexcept
        : token_(&token) {}

    constexpr auto await_suspend(argo::execution_token token) const noexcept
        -> std::coroutine_handle<> {
      *token_ = std::move(token);
      return std::noop_coroutine();
    }

   private:
    argo::execution_token * token_;
  };

  argo::promise_ctrl ctrl(std::noop_coroutine());
  ctrl.add_ref();
  ctrl.resume();

  argo::execution_token token;
  awaiter operation(token);
  EXPECT_EQ(ctrl.await_suspend(operation), std::noop_coroutine());
  EXPECT_EQ(ctrl.use_count(), 2);
  EXPECT_THROW(ctrl.lock_resume(), std::runtime_error);

  std::move(token).release();
  EXPECT_EQ(ctrl.use_count(), 1);
  ctrl.resume();
  ctrl.final_await_suspend();
  EXPECT_TRUE(ctrl.done());
  ctrl.remove_ref();
}

TEST(promise_ctrl, suspend_exception) {
  class awaiter final {
   public:
    void await_suspend() const { throw std::runtime_error("suspend failed"); }
  } operation;

  argo::promise_ctrl ctrl(std::noop_coroutine());
  ctrl.add_ref();
  ctrl.resume();

  EXPECT_THROW(ctrl.await_suspend(operation), std::runtime_error);
  EXPECT_EQ(ctrl.use_count(), 2);
  EXPECT_THROW(ctrl.lock_resume(), std::runtime_error);
  EXPECT_NO_THROW(ctrl.await_resume());

  ctrl.final_await_suspend();
  EXPECT_TRUE(ctrl.done());
  ctrl.remove_ref();
}

TEST(promise_ctrl, suspend_token_exception) {
  class awaiter final {
   public:
    void await_suspend(argo::execution_token) const {
      throw std::runtime_error("suspend failed");
    }
  } operation;

  argo::promise_ctrl ctrl(std::noop_coroutine());
  ctrl.add_ref();
  ctrl.resume();

  EXPECT_THROW(ctrl.await_suspend(operation), std::runtime_error);
  EXPECT_EQ(ctrl.use_count(), 2);
  EXPECT_THROW(ctrl.lock_resume(), std::runtime_error);
  EXPECT_NO_THROW(ctrl.await_resume());

  ctrl.final_await_suspend();
  EXPECT_TRUE(ctrl.done());
  ctrl.remove_ref();
}

TEST(promise_ctrl, sync_await) {
  argo::promise_ctrl parent(std::noop_coroutine());
  argo::promise_ctrl child(std::noop_coroutine());
  parent.add_ref();
  child.add_ref();

  EXPECT_EQ(child.sync_await_suspend(parent.lock_resume()),
            std::noop_coroutine());
  EXPECT_EQ(parent.use_count(), 2);
  EXPECT_EQ(child.use_count(), 2);
  EXPECT_THROW(parent.lock_resume(), std::runtime_error);
  EXPECT_THROW(child.lock_resume(), std::runtime_error);

  EXPECT_EQ(child.final_await_suspend(), std::noop_coroutine());
  EXPECT_TRUE(child.done());
  EXPECT_EQ(child.use_count(), 1);
  EXPECT_EQ(parent.use_count(), 2);
  EXPECT_THROW(parent.lock_resume(), std::runtime_error);

  parent.final_await_suspend();
  EXPECT_TRUE(parent.done());
  EXPECT_EQ(parent.use_count(), 1);
  child.remove_ref();
  parent.remove_ref();
}

TEST(promise_ctrl, waitlist_sealed) {
  argo::promise_ctrl ctrl(std::noop_coroutine());
  ctrl.add_ref();
  ctrl.resume();
  EXPECT_EQ(ctrl.final_await_suspend(), std::noop_coroutine());

  argo::task_waitlist::node waiter(argo::execution_token{}, nullptr);
  EXPECT_FALSE(ctrl.try_add_to_waitlist(waiter));
  ctrl.remove_ref();
}

TEST(promise_ctrl, waitlist_notify) {
  flow::spsc_queue<argo::execution_token> in_channel(2);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  argo::promise_ctrl ctrl(std::noop_coroutine());
  argo::promise_ctrl waiter1(std::noop_coroutine());
  argo::promise_ctrl waiter2(std::noop_coroutine());
  ctrl.add_ref();
  waiter1.add_ref();
  waiter2.add_ref();

  argo::task_waitlist::node node1(waiter1.lock_resume(), &executor.context());
  argo::task_waitlist::node node2(waiter2.lock_resume(), &executor.context());
  EXPECT_TRUE(ctrl.try_add_to_waitlist(node1));
  EXPECT_TRUE(ctrl.try_add_to_waitlist(node2));

  ctrl.resume();
  ctrl.final_await_suspend();
  EXPECT_TRUE(ctrl.done());
  EXPECT_EQ(in_channel.size(), 2);

  argo::execution_token token;
  EXPECT_TRUE(in_channel.try_pop(token));
  std::move(token).release();
  EXPECT_TRUE(in_channel.try_pop(token));
  std::move(token).release();
  EXPECT_EQ(waiter1.use_count(), 1);
  EXPECT_EQ(waiter2.use_count(), 1);

  ctrl.remove_ref();
  waiter1.remove_ref();
  waiter2.remove_ref();
}

}  // namespace tests::unit
