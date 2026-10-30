#pragma once

#include <atomic>
#include <cassert>
#include <coroutine>

#include <swiss/mixins.hpp>

#include "execution_context.hpp"
#include "execution_token.hpp"
#include "task_waitlist.hpp"

namespace argo {

// TODO: cancellation
// TODO: deadlines

/**
 * @brief Control object for promise type. Responsible for waitlist and state
 * mutations
 */
class promise_ctrl final : public swiss::non_copyable,
                           public swiss::non_movable {
 public:
  constexpr explicit promise_ctrl(std::coroutine_handle<> handle) noexcept
      : handle_(handle), continuation_(std::noop_coroutine()) {}

  /**
   * @section Lifetime-related methods
   */
 public:
  constexpr void add_ref() noexcept {
    // TODO:
    // SHARED(+1)
    use_count_.fetch_add(1);
  }

  constexpr void remove_ref() noexcept {
    // TODO:
    // (SUSPENDED | DONE) & SHARED(0) -> destroy
    // (SUSPENDED | DONE) & SHARED(_) ->
    if (use_count_.fetch_sub(1) == 1 && handle_) {
      handle_.destroy();
    }
  }

  constexpr auto use_count() const noexcept -> std::int16_t {
    return use_count_.load();
  }

  /**
   * @section State-machine-related methods
   */
 public:
  constexpr auto done() const noexcept -> bool {
    // TODO:
    // SUSPENDED | RUNNING -> false
    // DONE -> true
    return handle_.done();
  }

  constexpr void resume() {
    // TODO:
    // SUSPENDED -> {RUNNING}, resume
    // RUNNING | DONE -> throw
    handle_.resume();
  }

  constexpr auto lock_resume() -> execution_token {
    add_ref();
    return execution_token(*this);
  }

  constexpr void unlock_resume() noexcept { remove_ref(); }

  constexpr void await_suspend() {
    // TODO:
    //    RUNNING -> {SUSPENDED if TRUE/VALID/VOID}, await_suspend
  }

  constexpr void await_resume() {
    // TODO:
    // RUNNING -> {SUSPENDED if TRUE/VALID/VOID}, await_suspend
  }

  /**
   * @section Coroutine implementation-related methods
   */
 public:
  constexpr auto final_await_suspend() -> std::coroutine_handle<> {
    // TODO: impl, rewrite description
    // seal
    // SHARED(0) -> {DONE}, suspend_never, destroy, resume waitlist,
    // continuation SHARED(_) -> {DONE}, suspend_always, resume waitlist,
    // continuation
    for (auto & [token, ctx] : waitlist_) {
      ctx->execute(std::move(token));
    }
    return continuation_;
  }

  constexpr auto sync_await_suspend(execution_token token)
      -> std::coroutine_handle<> {
    // TODO: impl
    // SUSPENDED -> {RUNNING}, resume
    // RUNNING -> throw
    // DONE -> return
    // continuation_ = token.handle;
    continuation_ = token.ctrl_->handle_;
    return handle_;
  }

  template <typename PromiseFrame>
  constexpr auto promise() const noexcept -> PromiseFrame & {
    return std::coroutine_handle<PromiseFrame>::from_address(handle_.address())
        .promise();
  }

  constexpr auto add_to_waitlist(task_waitlist::node & node) noexcept -> bool {
    return waitlist_.try_push(node);
  }

 private:
  std::atomic<std::int16_t> use_count_;
  std::coroutine_handle<> handle_;
  std::coroutine_handle<> continuation_;
  task_waitlist waitlist_;
};

}  // namespace argo

/**
 * @note This section is required to mitigate circulare dependency between
 * promise control and execution token
 */
namespace argo::detail {
inline void promise_ctrl_resume(promise_ctrl & ctrl) { ctrl.resume(); }
inline void promise_ctrl_unlock_resume(promise_ctrl & ctrl) noexcept {
  ctrl.unlock_resume();
}
}  // namespace argo::detail
