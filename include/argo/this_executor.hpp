#pragma once

#include <coroutine>
#include <format>

#include <swiss/concepts.hpp>

#include "executor.hpp"

namespace argo::this_executor {

namespace detail {
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

/**
 * @brief Set park function before resuming a task on an executor. Call the
 * function after the task is suspended by an awaiter thus parking the suspended
 * task until it becomes ready and gets re-scheduled
 */
extern thread_local void (*park)(void const * awaiter,
                                 std::type_info const & type_info);

/**
 * @brief Set yield function before resuming a task on an executor. Call the
 * function after the task is suspended thus re-scheduling the task immediately
 * and bypassing the real task scheduler
 */
extern thread_local void (*yield)(std::coroutine_handle<> const);

/**
 * @brief Set the executor to this variable before resuming any task
 */
extern thread_local void const * this_executor;

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
}  // namespace detail

/**
 * @note Noexcept if input channel of the current executor is noexcept
 */
constexpr void yield(std::coroutine_handle<> const task) {
  detail::yield(task);

  detail::this_executor = nullptr;
  detail::park          = nullptr;
  detail::yield         = nullptr;
}

/**
 * @note Noexcept if output channel of the current executor is noexcept and the
 * channel value type is the same as const pointer to the awaiter
 */
template <typename Awaiter>
constexpr void schedule(Awaiter const & awaiter) {
  /**
   * @note Pass a const pointer to the awaiter along with its typeid. Park
   * functions verifies that typeid(const awaiter ptr) is the same as
   * typeid(current executor's output channel value type) and thus a scheduler
   * on the output channel's receiving end is compatible with the current
   * awaiter
   */
  detail::park(&awaiter, typeid(Awaiter const *));

  detail::this_executor = nullptr;
  detail::park          = nullptr;
  detail::yield         = nullptr;
}

template <concepts::executor Executor, swiss::nothrow_invocable Task>
constexpr void resume(Executor const &, Task && task) noexcept {
  /**
   * @note Inapplicable for non-coroutines
   */
  if (detail::this_executor != nullptr) [[unlikely]] {
    detail::this_executor = nullptr;
    detail::park          = nullptr;
    detail::yield         = nullptr;
  }

  std::invoke(std::forward<Task>(task));
}

template <concepts::executor Executor>
// NOLINTNEXTLINE(bugprone-exception-escape)
constexpr void resume(Executor const & executor,
                      std::coroutine_handle<> const task) noexcept {
  /**
   * @note Save on memory writes when using the same executor
   */
  if (detail::this_executor != &executor) [[unlikely]] {
    detail::this_executor = &executor;

    detail::park = [](void const * awaiter,
                      std::type_info const & type_info) -> void {
      auto const & this_executor =
          *static_cast<Executor const *>(detail::this_executor);
      auto const * typed_awaiter =
          static_cast<typename Executor::out_channel_type::value_type>(awaiter);

      if (type_info != typeid(typed_awaiter)) [[unlikely]] {
        throw std::runtime_error(
            std::format("incompatible awaiter {}, expected {}",
                        type_info.name(), typeid(typed_awaiter).name()));
      }

      // TODO: handle cancellation
      this_executor.out_channel().push(typed_awaiter);
    };

    detail::yield =
        [](std::coroutine_handle<> const task) noexcept(
            concepts::noexcept_channel<typename Executor::in_channel_type>)
        -> void {
      auto const & this_executor =
          *static_cast<Executor const *>(detail::this_executor);

      // TODO: handle cancellation
      this_executor.in_channel().push(task);
    };
  }

  /**
   * @note Task exception handling is done asynchronously with the standard
   * unhandled_exception method
   */
  task.resume();
}

}  // namespace argo::this_executor
