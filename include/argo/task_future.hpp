#include "task.hpp"

namespace argo {

/**
 * @brief Future object for task
 * @warning Non-awaited future object destruction is considered a programming
 * error and automatically calls std::terminate. For background tasks use
 * detach. This is done to prevent use after free bugs due to task handle being
 * shared with the executor when the task get destructed
 */
template <typename T>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class [[nodiscard]] task_future final {
 public:
  explicit constexpr task_future(task<T> && task) noexcept
      : task_(std::move(task)) {}

  constexpr ~task_future() noexcept {
    /**
     * @warning Does std::terminate if not awaited
     */
    if (task_) {
      std::terminate();
    }
  }

 public:
  constexpr auto operator co_await() && noexcept -> decltype(auto) {
    class [[nodiscard]] task_awaiter final {
     public:
      explicit constexpr task_awaiter(task<T> && task) noexcept
          : task_(std::move(task)) {}

     public:
      constexpr auto await_ready() const noexcept -> bool {
        return task_.handle_.promise().done_without_continuation();
      }

      constexpr auto await_suspend(std::coroutine_handle<> caller)
          const noexcept -> std::coroutine_handle<> {
        /**
         * @note If task is done at this point, compare_exchange_continuation()
         * will fail and the suspended caller can resume immediately
         */
        if (!task_.handle_.promise().compare_exchange_continuation(caller)) {
          return caller;
        }
        return std::noop_coroutine();
      }

      constexpr auto await_resume() -> decltype(auto) {
        return std::move(task_).get();
      }

     private:
      task<T> task_;
    };
    return task_awaiter(std::move(task_));
  }

 private:
  task<T> task_;
};

}  // namespace argo
