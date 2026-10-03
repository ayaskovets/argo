#pragma once

#include "execution_context.hpp"
#include "task.hpp"
#include "task_ref.hpp"

namespace argo {

/**
 * @brief Non-owning future object for the task type
 * @note Can be used for awaiting a task from multiple awaiters
 * @note Thread-safe
 */
template <typename T>
class task_future final {
 private:
  using promise_type  = typename task<T>::promise_type;
  using waitlist_type = typename promise_type::waitlist_type;

 public:
  explicit constexpr task_future(task<T> & task) noexcept
      : promise_(task.handle_.promise()) {}

 public:
  constexpr auto operator co_await() noexcept -> decltype(auto) {
    class task_awaiter final {
     public:
      explicit constexpr task_awaiter(task_future<T> future) noexcept
          : node_(), promise_(future.promise_) {}

     public:
      constexpr auto await_ready() const noexcept -> bool {
        return promise_.get().waitlist().sealed();
      }

      constexpr auto await_suspend(task_ref task_ref) noexcept -> bool {
        node_ = typename waitlist_type::node(
            task_ref, &execution_context::this_executor());
        return promise_.get().waitlist().try_push(node_);
      }

      constexpr auto await_resume() -> decltype(auto) {
        return promise_.get().get();
      }

     private:
      waitlist_type::node node_;
      std::reference_wrapper<promise_type> promise_;
    };
    return task_awaiter(*this);
  }

 private:
  std::reference_wrapper<promise_type> promise_;
};

}  // namespace argo
