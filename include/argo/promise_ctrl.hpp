#pragma once

#include <atomic>
#include <cassert>
#include <coroutine>

#include <swiss/mixins.hpp>
#include <swiss/scope_exit.hpp>

#include "execution_context.hpp"
#include "execution_token.hpp"
#include "promise_exceptions.hpp"
#include "task_waitlist.hpp"

namespace argo {

// TODO: deadlines

/**
 * @brief Control object for promise type. Responsible for waitlist and state
 * mutations
 */
class promise_ctrl final : public swiss::non_copyable,
                           public swiss::non_movable {
 private:
  /**
   * @brief Coroutine state
   */
  // NOLINTNEXTLINE(performance-enum-size)
  enum status : std::uint32_t {
    kSuspended = 0U << 29,
    kReserved  = 1U << 29,
    kRunning   = 2U << 29,
    kDone      = 3U << 29,
  };

  /**
   * @section State bit layout flags
   * @note The layout in inclusive bit ranges is the following: [31..29]
   * coroutine state, [28] cancellation flag, [27..0] refcount
   */
 private:
  static constexpr std::uint32_t kStatusMask = 0xE0000000U;
  static constexpr std::uint32_t kCancelMask = 0x10000000U;
  static constexpr std::uint32_t kRefcntMask = 0x0FFFFFFFU;

 public:
  constexpr explicit promise_ctrl(std::coroutine_handle<> handle) noexcept
      : state_(status::kSuspended), handle_(handle) {}

  /**
   * @section Lifetime-related methods
   */
 public:
  /**
   * @brief Add one reference
   */
  constexpr void add_ref() noexcept {
    [[maybe_unused]] auto const previous =
        state_.fetch_add(1, std::memory_order::relaxed);

    assert((previous & kRefcntMask) != kRefcntMask &&
           "promise_ctrl::add_ref(): refcount overflow");
  }

  /**
   * @brief Remove one reference
   */
  constexpr void remove_ref() noexcept {
    auto const previous = state_.fetch_sub(1, std::memory_order::acq_rel);

    assert((previous & kRefcntMask) != 0 &&
           "promise_ctrl::remove_ref(): refcount underflow");

    if ((previous & kRefcntMask) == 1 && handle_) {
      handle_.destroy();
    }
  }

  /**
   * @brief Get number of references pointing to this coroutine frame
   */
  constexpr auto use_count() const noexcept -> std::uint32_t {
    return state_.load(std::memory_order::relaxed) & kRefcntMask;
  }

  /**
   * @section General public interface
   */
 public:
  /**
   * @brief Check whether the coroutine is done
   */
  constexpr auto done() const noexcept -> bool {
    auto const status = static_cast<enum status>(
        state_.load(std::memory_order::acquire) & kStatusMask);
    return status == status::kDone;
  }

  /**
   * @brief Resume the coroutine with an exclusive access
   */
  constexpr void resume() { lock_resume().resume(); }

 public:
  /**
   * @brief Asynchronous one-way cancel request. Cancel is visible when the
   * coroutine result is accessed or coroutine state change is attempted
   */
  constexpr void request_cancel() noexcept {
    state_.fetch_or(kCancelMask, std::memory_order::relaxed);
  }

  /**
   * @brief Check whether a cancel was requested
   */
  constexpr auto cancel_requested() const noexcept -> bool {
    return (state_.load(std::memory_order::relaxed) & kCancelMask) != 0;
  }

  /**
   * @brief Get reference to the promise object
   */
  template <typename PromiseFrame>
  constexpr auto promise() const noexcept -> PromiseFrame & {
    return std::coroutine_handle<PromiseFrame>::from_address(handle_.address())
        .promise();
  }

  /**
   * @brief Add a waiting node to the waitlist of this coroutine
   */
  constexpr auto try_add_to_waitlist(task_waitlist::node & node) noexcept
      -> bool {
    return waitlist_.try_push(node);
  }

  /**
   * @section Awaiter-related methods
   */
 public:
  /**
   * @brief Hook for when a coroutine gets resumed
   */
  constexpr void await_resume() const {
    if (cancel_requested()) {
      throw promise_cancelled();
    }
  }

  /**
   * @brief Keep the frame alive through the awaiter call and state rollback
   * @note Refcount is kept extended by one for the duration of the
   * await_suspend operation in await_transform to prolong the frame life until
   * at least promise_ctrl::await_suspend returns
   */
  template <typename Awaiter>
  constexpr auto await_suspend(Awaiter & awaiter) -> decltype(auto) {
    /**
     * @note Emulate the task getting resumed by execution_token through a
     * symmetric transfer
     */
    auto const emulate_resume = [this] -> auto {
      /**
       * @note A reference from a virtual execution_token
       */
      add_ref();

      auto expected = status::kSuspended;
      [[maybe_unused]] auto const success =
          compare_exchange_status(expected, status::kRunning);

      assert(success && "promise_ctrl::await_suspend(): invalid state");
    };

    swiss::scope_exit const scope_exit([&]() noexcept -> void {
      /**
       * @note Reference from a (token / sync await / symmetric transfer) is
       * discontinued
       */
      remove_ref();
    });

    auto expected = status::kRunning;
    [[maybe_unused]] auto const success =
        compare_exchange_status(expected, status::kSuspended);

    assert(success && "promise_ctrl::await_suspend(): invalid state");

    try {
      /**
       * @note Syspend a general-purpose awaiter without a token parameter
       */
      if constexpr (requires {
                      { awaiter.await_suspend() } -> std::same_as<void>;
                    }) {
        return awaiter.await_suspend();
      }
      /**
       * @note Suspend task_future. Resume the task if waitlist is sealed
       */
      else if constexpr (requires {
                           {
                             awaiter.await_suspend(lock_resume())
                           } -> std::same_as<bool>;
                         }) {
        if (!awaiter.await_suspend(lock_resume())) {
          emulate_resume();
          return false;
        }
        return true;
      }
      /**
       * @note Suspend a general-purpose awaiter or sync_awaiter with a token
       * parameter
       */
      else {
        return awaiter.await_suspend(lock_resume());
      }
    } catch (...) {
      emulate_resume();
      throw;
    }
  }

  /**
   * @brief Resume all waiting tasks and optionally do a symmetric transfer to
   * the parent
   * @note Refcount is kept extended by one for the duration of the
   * await_suspend operation in await_transform to prolong the frame life
   * until at least promise_ctrl::await_suspend returns
   */
  constexpr auto final_await_suspend() noexcept -> std::coroutine_handle<> {
    std::coroutine_handle<> continuation = std::noop_coroutine();

    /**
     * @note Parent reference remains valid until it gets suspended
     */
    promise_ctrl * const continuation_ctrl =
        std::exchange(continuation_.ctrl_, nullptr);

    /**
     * @note Emulate parent getting resumed by execution_token through a
     * symmetric transfer
     */
    if (continuation_ctrl != nullptr) {
      auto expected = status::kReserved;
      [[maybe_unused]] auto const success =
          continuation_ctrl->compare_exchange_status(expected,
                                                     status::kRunning);

      assert(continuation_ctrl->handle_ &&
             "promise_ctrl::final_await_suspend(): invalid parent handle");
      assert(success &&
             "promise_ctrl::final_await_suspend(): invalid parent state");

      continuation = continuation_ctrl->handle_;
    }

    auto expected = status::kRunning;
    [[maybe_unused]] auto const success =
        compare_exchange_status(expected, status::kDone);

    assert(success && "promise_ctrl::final_await_suspend(): invalid state");

    waitlist_.seal();

    auto iter      = waitlist_.begin();
    auto const end = waitlist_.end();

    /**
     * @note Reference from a (token / sync await / symmetric transfer) is
     * discontinued. Frame may get destroyed
     * @note Posterior operations must not reference the frame. Stack
     * iterator is valid after the stack is destroyed because
     * swiss::sealable_stack is non-owning
     */
    remove_ref();

    while (iter != end) {
      auto & [token, ctx] = *iter++;
      ctx->execute(std::move(token));
    }

    return continuation;
  }

  /**
   * @brief Perform symmetric transfer to the current coroutine frame and set
   * parent to the suspended coroutine
   */
  constexpr auto sync_await_suspend(execution_token token)
      -> std::coroutine_handle<> {
    /**
     * @note Emulate the child task getting resumed via an execution token so
     * refcount is managed correctly on suspension but do a symmetric transfer
     * in fact
     */
    add_ref();

    auto expected = status::kSuspended;
    [[maybe_unused]] auto const success =
        compare_exchange_status(expected, status::kRunning);

    assert(token.ctrl_ && "promise_ctrl::sync_await_suspend(): invalid token");
    assert(success && "promise_ctrl::sync_await_suspend(): invalid state");
    assert(!continuation_.ctrl_ &&
           "promise_ctrl::sync_await_suspend(): changed parent");

    continuation_ = std::move(token);
    return handle_;
  }

  /**
   * @section Execution token-related methods
   */
 public:
  /**
   * @brief Create a one-shot execution token for a suspended coroutine. If
   * successful, blocks another token creation until this token gets destroyed
   * or resumed
   */
  constexpr auto lock_resume() -> execution_token {
    auto expected = status::kSuspended;
    if (!compare_exchange_status(expected, status::kReserved)) {
      throw std::runtime_error(
          "promise_ctrl::lock_resume(): task is not suspended");
    }

    /**
     * @note Reference remains valid until token is released or task gets
     * suspended
     */
    add_ref();

    return execution_token(*this);
  }

  /**
   * @brief Run the coroutine and unlock token creation after it gets
   * suspended
   */
  constexpr void unlock_resume() noexcept {
    auto expected = status::kReserved;
    [[maybe_unused]] auto const success =
        compare_exchange_status(expected, status::kRunning);

    assert(handle_ && "promise_ctrl::unlock_resume(): invalid token");
    assert(success && "promise_ctrl::release_resume(): invalid state");

    handle_.resume();
  }

  /**
   * @brief Allow to create new token without running the task
   */
  constexpr void release_resume() noexcept {
    auto expected = status::kReserved;
    [[maybe_unused]] auto const success =
        compare_exchange_status(expected, status::kSuspended);

    assert(success && "promise_ctrl::release_resume(): invalid state");

    /**
     * @note Reference is discontinued for an unused token
     */
    remove_ref();
  }

 private:
  /**
   * @brief Do a CAS state compare_exchange_status
   */
  constexpr auto compare_exchange_status(status & expected,
                                         status desired) noexcept -> bool {
    auto state = state_.load(std::memory_order::acquire);
    while ((state & kStatusMask) == expected) {
      if (state_.compare_exchange_weak(state, (state & ~kStatusMask) | desired,
                                       std::memory_order::acq_rel,
                                       std::memory_order::acquire)) {
        return true;
      }
    }

    expected = static_cast<status>(state & kStatusMask);
    return false;
  }

 private:
  std::atomic<std::uint32_t> state_;
  std::coroutine_handle<> handle_;
  execution_token continuation_;
  task_waitlist waitlist_;
};

}  // namespace argo

/**
 * @note This section mitigates the circular dependency between promise control
 * and execution token
 */
namespace argo::detail {
inline void promise_ctrl_unlock_resume(promise_ctrl & ctrl) noexcept {
  ctrl.unlock_resume();
}
inline void promise_ctrl_release_resume(promise_ctrl & ctrl) noexcept {
  ctrl.release_resume();
}
}  // namespace argo::detail
