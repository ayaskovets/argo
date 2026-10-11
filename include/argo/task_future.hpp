#pragma once

#include "execution_context.hpp"
#include "execution_token.hpp"
#include "task_waitlist.hpp"

namespace argo {

/**
 * @brief Shared thread-safe task that can be awaited by multiple coroutines
 */
template <typename PromiseFrame>
class task_future final {
 public:
  explicit constexpr task_future(promise_ptr<PromiseFrame> ptr) noexcept
      : ptr_(std::move(ptr)) {}

 public:
  constexpr auto operator co_await() noexcept -> auto {
    class future_awaiter final {
     public:
      explicit constexpr future_awaiter(task_future & future) noexcept
          : future_(&future) {}

     public:
      constexpr auto await_ready() const noexcept -> bool {
        return future_->ptr_->done();
      }

      constexpr auto await_suspend(execution_token token) noexcept -> bool {
        token_ = std::move(token);
        node_ =
            task_waitlist::node(&token_, &execution_context::this_executor());
        return future_->ptr_->add_to_waitlist(node_);
      }

      constexpr auto await_resume() const -> decltype(auto) {
        return future_->ptr_->get();
      }

     private:
      execution_token token_;
      task_waitlist::node node_;
      task_future * future_;
    };

    return future_awaiter(*this);
  }

 private:
  promise_ptr<PromiseFrame> ptr_;
};

}  // namespace argo
