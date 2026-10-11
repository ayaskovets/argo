#pragma once

#include <coroutine>
#include <exception>
#include <optional>
#include <stdexcept>

#include <swiss/mixins.hpp>

#include "awaiter.hpp"
#include "execution_token.hpp"
#include "promise_ctrl.hpp"
#include "promise_ptr.hpp"

namespace argo {

/**
 * @brief Mandatory return value interface for a non-void-returning coroutine
 */
template <typename T>
class promise_retval {
 public:
  template <typename U>
  constexpr void return_value(U && value) noexcept {
    value_.emplace(std::forward<U>(value));
  }

  constexpr void unhandled_exception() noexcept {
    exception_ = std::current_exception();
  }

 protected:
  constexpr auto get() const -> T & {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
    if (!value_.has_value()) {
      throw std::runtime_error("promise_retval::get(): not done");
    }
    return *value_;
  }

 private:
  mutable std::optional<T> value_;
  std::exception_ptr exception_;
};

/**
 * @brief Mandatory return value interface for a void-returning coroutine
 */
template <>
class promise_retval<void> {
 public:
  constexpr void return_void() const noexcept {}

  constexpr void unhandled_exception() noexcept {
    exception_ = std::current_exception();
  }

 protected:
  constexpr void get() const {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
  }

 private:
  std::exception_ptr exception_;
};

/**
 * @brief Promise type with configurable return object type and a state machine
 * for lifetime management
 */
template <typename ReturnObject>
class promise_frame final
    : public promise_retval<typename ReturnObject::value_type>,
      public swiss::non_copyable,
      public swiss::non_movable {
 public:
  constexpr promise_frame() noexcept
      : ctrl_(std::coroutine_handle<promise_frame>::from_promise(*this)) {}

 public:
  constexpr auto get_return_object() -> ReturnObject {
    return ReturnObject(promise_ptr<promise_frame>(ctrl_));
  }

 public:
  /**
   * @brief Hide the raw coroutine handle from the user-facing awaiter types and
   * replace it with a fixed awaiter context with a curated set of possible
   * operations on the coroutine
   * @note Supports awaiters that do not care about the suspended task where
   * await_suspend takes no arguments
   */
  template <concepts::awaiter Awaiter>
  constexpr auto await_transform(Awaiter && awaiter) const noexcept -> auto {
    /* TODO:
      RUNNING -> {SUSPENDED if TRUE/VALID/VOID}, await_suspend
    */

    class proxy_awaiter final {
     public:
      explicit constexpr proxy_awaiter(
          swiss::co_await_result_t<Awaiter> awaiter) noexcept
          : awaiter_(std::move(awaiter)) {}

     public:
      constexpr auto await_ready() const noexcept -> bool {
        return awaiter_.await_ready();
      }

      constexpr auto await_suspend(std::coroutine_handle<promise_frame> handle)
          -> decltype(auto) {
        if constexpr (requires { awaiter_.await_suspend(); }) {
          return awaiter_.await_suspend();
        } else {
          return awaiter_.await_suspend(
              execution_token(promise_ptr(handle.promise().ctrl_)));
        }
      }

      constexpr auto await_resume() -> decltype(auto) {
        return awaiter_.await_resume();
      }

     private:
      swiss::co_await_result_t<Awaiter> awaiter_;
    };

    return proxy_awaiter(
        swiss::co_await_result_v(std::forward<Awaiter>(awaiter)));
  }

 public:
  /**
   * @brief Lazy execution by default
   */
  constexpr auto initial_suspend() const noexcept -> std::suspend_always {
    return {};
  }

  /**
   * @brief Suspend the task and do a symmetric transfer if there is a single
   * awaiting parent coroutine
   */
  constexpr auto final_suspend() const noexcept -> auto {
    /* TODO: impl, rewrite description
      SHARED(0) -> {DONE}, suspend_never, destroy, resume waitlist, continuation
      SHARED(_) -> {DONE}, suspend_always, resume waitlist, continuation
     */

    class continuation_awaiter final {
     public:
      explicit constexpr continuation_awaiter(
          promise_frame const & frame) noexcept
          : frame_(&frame) {}

     public:
      constexpr auto await_ready() const noexcept -> bool { return false; }

      constexpr auto await_suspend(std::coroutine_handle<>) const noexcept
          -> std::coroutine_handle<> {
        return frame_->ctrl_.get_continuation();
      }

      constexpr void await_resume() const noexcept {}

     private:
      promise_frame const * frame_;
    };

    return continuation_awaiter(*this);
  }

  /**
   * @brief Synchronously resume this coroutine via symmetric transfer from the
   * suspended task and return control to the parent in final_suspend()
   */
  constexpr auto operator co_await() && noexcept -> auto {
    /* TODO: impl
      SUSPENDED -> {RUNNING}, resume
      RUNNING -> throw
      DONE -> return
    */

    class sync_awaiter final {
     public:
      explicit constexpr sync_awaiter(promise_frame & frame) noexcept
          : frame_(&frame) {}

     public:
      constexpr auto await_ready() const noexcept -> bool {
        return frame_->ctrl_.done();
      }

      constexpr auto await_suspend(execution_token token) const noexcept
          -> std::coroutine_handle<> {
        frame_->ctrl_.set_continuation(
            token.ptr_.ctrl_->handle(swiss::badge<promise_frame>{}));
        return frame_->ctrl_.handle(swiss::badge<promise_frame>{});
      }

      constexpr auto await_resume() const -> decltype(auto) {
        return frame_->get();
      }

     private:
      promise_frame * frame_;
    };

    return sync_awaiter(*this);
  }

 public:
  /**
   * @brief Lock other calls to resume and this function. Create an object
   * that grants exclusive access resumption until it gets destructed
   */
  constexpr auto get_execution_token() noexcept -> execution_token {
    return execution_token(promise_ptr(ctrl_));
  }

  /**
   * @brief Add node to waitlist. Suspended nodes on waitlist are resumed in
   * final_suspend
   */
  constexpr auto add_to_waitlist(task_waitlist::node const & node) noexcept
      -> bool {
    return ctrl_.add_to_waitlist(node);
  }

 public:
  /**
   * @brief Resume a non-resumed coroutine that is not shared with a execution
   * channel, otherwise throw an exception
   */
  constexpr void resume() { return ctrl_.resume(); }

  /**
   * @brief Check whether coroutine is finished
   */
  constexpr auto done() const noexcept -> bool { return ctrl_.done(); }

  /**
   * @brief Extract value from a finished coroutine. Throws an exception for an
   * unfinished coroutine
   * @returns A reference to a value of the task type or void
   */
  constexpr auto get() const -> decltype(auto) {
    if (!ctrl_.done()) {
      throw std::runtime_error("promise_frame::get(): not done");
    }
    return promise_retval<typename ReturnObject::value_type>::get();
  }

 private:
  promise_ctrl ctrl_;
};

}  // namespace argo
