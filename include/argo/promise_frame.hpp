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
template <typename T>
class task;
template <typename T>
class task_future;
}  // namespace argo

namespace argo {

/**
 * @brief Mandatory return value interface for a non-void-returning coroutine
 */
template <typename T>
class promise_retval {
 public:
  template <typename U>
  constexpr void return_value(U && value) {
    value_.emplace(std::forward<U>(value));
  }

  constexpr void unhandled_exception() noexcept {
    exception_ = std::current_exception();
  }

 protected:
  constexpr auto get() -> T & {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
    if (!value_.has_value()) {
      throw std::runtime_error("promise_retval::get(): not done");
    }
    return *value_;
  }

 private:
  std::optional<T> value_;
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
  constexpr void get() {
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
  template <typename Awaiter>
    requires(concepts::awaiter<Awaiter> ||
             swiss::instantiation_of<Awaiter, task> ||
             swiss::instantiation_of<Awaiter, task_future>)
  constexpr auto await_transform(Awaiter && awaiter) -> auto {
    class proxy_awaiter final {
     public:
      explicit constexpr proxy_awaiter(
          swiss::co_await_result_t<Awaiter> awaiter, promise_frame & frame)
          : awaiter_(std::forward<swiss::co_await_result_t<Awaiter>>(awaiter)),
            suspended_frame_(&frame) {}

     public:
      constexpr auto await_ready() -> bool {
        /**
         * @note Null frame pointer means that the coroutine was not suspended
         * by this awaiter
         */
        if (awaiter_.await_ready()) {
          suspended_frame_ = nullptr;
          return true;
        }

        return false;
      }

      constexpr auto await_suspend(std::coroutine_handle<promise_frame> handle)
          -> auto {
        suspended_frame_->ctrl_.await_suspend();

        if constexpr (requires { awaiter_.await_suspend(); }) {
          return awaiter_.await_suspend();
        } else {
          return awaiter_.await_suspend(handle.promise().get_execution_token());
        }
      }

      constexpr auto await_resume() -> decltype(auto) {
        /**
         * @note Null frame pointer means that the coroutine was not suspended
         * by this awaiter
         */
        if (suspended_frame_) {
          suspended_frame_->ctrl_.await_resume();
        }

        return awaiter_.await_resume();
      }

     private:
      swiss::co_await_result_t<Awaiter> awaiter_;
      promise_frame * suspended_frame_;
    };

    return proxy_awaiter(
        swiss::co_await_result_v(std::forward<Awaiter>(awaiter)), *this);
  }

 public:
  /**
   * @brief Lazy execution by default
   */
  constexpr auto initial_suspend() const noexcept -> std::suspend_always {
    return {};
  }

  /**
   * @brief Suspend the task, notify it's waitlist and optionally resume
   * continuation
   */
  constexpr auto final_suspend() noexcept -> auto {
    class final_awaiter final {
     public:
      explicit constexpr final_awaiter(promise_frame & frame) noexcept
          : frame_(&frame) {}

     public:
      constexpr auto await_ready() const noexcept -> bool { return false; }

      constexpr auto await_suspend(std::coroutine_handle<>)
          -> std::coroutine_handle<> {
        return frame_->ctrl_.final_await_suspend();
      }

      constexpr void await_resume() const noexcept {}

     private:
      promise_frame * frame_;
    };

    return final_awaiter(*this);
  }

  /**
   * @brief Synchronously resume this coroutine via symmetric transfer from the
   * suspended task and return control to the parent in final_suspend()
   */
  constexpr auto operator co_await() && noexcept -> auto {
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
        return frame_->ctrl_.sync_await_suspend(std::move(token));
      }

      constexpr auto await_resume() -> auto {
        if constexpr (std::same_as<typename ReturnObject::value_type, void>) {
          return frame_->get();
        } else {
          return std::move_if_noexcept(frame_->get());
        }
      }

     private:
      promise_frame * frame_;
    };

    return sync_awaiter(*this);
  }

 public:
  /**
   * @brief Lock other calls to resume and this function. Create an object
   * that grants exclusive access to resumption until it gets destructed
   */
  constexpr auto get_execution_token() -> execution_token {
    return ctrl_.lock_resume();
  }

  /**
   * @brief Add node to waitlist. Suspended nodes on waitlist are resumed in
   * final_suspend
   */
  constexpr auto add_to_waitlist(task_waitlist::node & node) noexcept -> bool {
    return ctrl_.add_to_waitlist(node);
  }

 public:
  /**
   * @brief Resume a non-resumed coroutine that is not shared with a execution
   * channel, otherwise throw an exception
   * @warning Synchronous interface. Not intended for general use within the
   * engine
   */
  constexpr void resume() { return ctrl_.resume(); }

  /**
   * @brief Check whether coroutine is finished
   * @warning Synchronous interface. Not intended for general use within the
   * engine
   */
  constexpr auto done() const noexcept -> bool { return ctrl_.done(); }

  /**
   * @brief Extract value from a finished coroutine. Throws an exception for an
   * unfinished coroutine
   * @returns A reference to a value of the task type or void
   * @warning Synchronous interface. Not intended for general use within the
   * engine
   */
  constexpr auto get() -> decltype(auto) {
    if (!ctrl_.done()) {
      throw std::runtime_error("promise_frame::get(): not done");
    }
    return promise_retval<typename ReturnObject::value_type>::get();
  }

 private:
  promise_ctrl ctrl_;
};

}  // namespace argo
