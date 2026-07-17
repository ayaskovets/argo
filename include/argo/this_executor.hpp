#pragma once

#include <cassert>
#include <coroutine>
#include <format>

#include <swiss/concepts.hpp>

#include "executor.hpp"

namespace argo::this_executor {

namespace detail {
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

/**
 * @brief Set the executor to this variable before resuming any task
 */
extern thread_local void const * this_executor;

/**
 * @brief Set yield function before resuming a task on an executor. Call the
 * function after the task is suspended thus re-scheduling the task immediately
 * and bypassing the real task scheduler
 */
extern thread_local void (*yield)(std::coroutine_handle<> handle);

/**
 * @brief Set park function before resuming a task on an executor. Call the
 * function after the task is suspended by an awaiter thus parking the suspended
 * task until it becomes ready and gets re-scheduled
 */
extern thread_local void (*park)(void const * params,
                                 std::type_info const & type_info);

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
}  // namespace detail

/**
 * @note Noexcept if input channel of the current executor is noexcept
 */
constexpr void yield(std::coroutine_handle<> handle) {
  assert(detail::yield &&
         "yield(): coroutines should be resumed via resume() from an executor "
         "instance");
  detail::yield(handle);
  detail::yield = nullptr;
}

/**
 * @note Noexcept if output channel of the current executor is noexcept and the
 * channel value type is the same as const pointer to the parameters
 */
template <typename Params>
constexpr void schedule(Params const & params) {
  /**
   * @note Pass a const pointer to the scheduler parameters along with its
   * typeid. The parking function verifies that typeid of the scheduler
   * parameters is the same as typeid of the current executor's output channel
   * value and thus there is a compatible scheduler on the output channel's
   * receiving end
   */
  assert(
      detail::park &&
      "schedule(): coroutines should be resumed via resume() from an executor "
      "instance");
  detail::park(&params, typeid(&params));
  detail::park = nullptr;
}

template <concepts::executor Executor>
// NOLINTNEXTLINE(bugprone-exception-escape)
constexpr void resume(Executor const & executor,
                      std::coroutine_handle<> handle) noexcept {
  /**
   * @note Condition to decrease memory writes when using a single executor
   */
  if (detail::this_executor != &executor) [[unlikely]] {
    detail::this_executor = &executor;

    detail::park = [](void const * params,
                      std::type_info const & type_info) -> void {
      assert(detail::this_executor &&
             "park(): this function should not be called directly");
      auto const & this_executor =
          *static_cast<Executor const *>(detail::this_executor);

      auto const * typed_params =
          static_cast<typename Executor::out_channel_type::value_type const *>(
              params);

      // TODO: support variant
      if (type_info != typeid(typed_params)) [[unlikely]] {
        throw std::runtime_error(
            std::format("resume(): incompatible type {}, expected {}",
                        type_info.name(), typeid(typed_params).name()));
      }

      // TODO: handle cancellation
      this_executor.out_channel().push(*typed_params);
    };

    detail::yield =
        [](std::coroutine_handle<> handle) noexcept(
            concepts::noexcept_channel<typename Executor::in_channel_type>)
        -> void {
      assert(detail::this_executor &&
             "yield(): this function should not be called directly");
      auto const & this_executor =
          *static_cast<Executor const *>(detail::this_executor);

      // TODO: handle cancellation
      this_executor.in_channel().push(handle);
    };
  }

  /**
   * @note Task exception handling is done asynchronously with the standard
   * unhandled_exception method
   */
  handle.resume();
}

}  // namespace argo::this_executor
