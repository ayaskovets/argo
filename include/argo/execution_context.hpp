#pragma once

#include <cassert>
#include <coroutine>
#include <format>

#include "executor.hpp"

namespace argo {

/**
 * @brief Engine execution context. All scheduling and context switching should
 * be done through the functions defined in this namespace
 * @note With multiple threads of execution each thread needs its own context to
 * function properly as it is difficult and unnecessary to implement this class
 * to distinguish threads and to manage context automatically
 */
class execution_context {
 protected:
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  /**
   * @brief This pointer is set when a task is resumed by an executor to
   * access the executor instance
   */
  void const * this_executor_ = nullptr;

  /**
   * @brief Yield function is set when a task is resumed by an executor. This
   * called when a coroutine encounters a co_await with the
   * awaiters::yield_awaiter
   * @note The main reason for this function is to bypass scheduler and add the
   * task back to the execution queue
   */
  void (*yield_)(execution_context const & ctx,
                 std::coroutine_handle<> handle) = nullptr;

  /**
   * @brief Park function is set when a task is resumed by an executor. This
   * function gets called when a coroutine encounters a co_await that causes it
   * to suspend. This function is expected to do
   * @note Type info is passed for type safety to ensure that either scheduler
   * input channel or any other place that the awaiter params go to expect the
   * same type as the one passed as it is impossible to know what scheduler type
   * is going to be used with a certain awaiter
   */
  void (*park_)(execution_context const & ctx, void const * params,
                std::type_info const & type_info) = nullptr;

  // NOLINTEND(misc-non-private-member-variables-in-classes)
 public:
  /**
   * @brief Yield execution to another task adding handle to the back of
   * the execution queue
   * @note Can be treated as a end of an step execution for a certain coroutine
   * @note Noexcept if input channel of the current executor is noexcept
   */
  constexpr void yield(std::coroutine_handle<> handle) {
    assert(yield_ &&
           "execution_context::yield(): coroutines first have be resumed by "
           "from an executor");
    yield_(*this, handle);
  }

  /**
   * @brief Move task to a task scheduler. The specific scheduler is selected
   * based on the function argument
   * @note Can be treated as a end of an step execution for a certain coroutine
   * @note Noexcept if input channel of the current executor is noexcept
   */
  template <typename Params>
  constexpr void schedule(Params const & params) {
    assert(park_ &&
           "execution_context::schedule(): coroutines first have be resumed by "
           "an executor");
    park_(*this, &params, typeid(&params));
  }

  /**
   * @brief Resume task by the passed executor instance. Sets up all internal
   * machinery needed for awaiters and task scheduling to work
   * @note Can be treated as a start of an execution step for a certain
   * coroutine
   */
  template <concepts::executor Executor>
  // NOLINTNEXTLINE(bugprone-exception-escape)
  constexpr void resume(Executor const & executor,
                        std::coroutine_handle<> handle) noexcept {
    /**
     * @note Condition to decrease memory writes when using a single executor
     */
    if (this_executor_ != &executor) [[unlikely]] {
      this_executor_ = &executor;

      // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
      park_ = [](execution_context const & ctx, void const * params,
                 std::type_info const & type_info) -> void {
        assert(ctx.this_executor_ &&
               "execution_context::park(): this function should not be called "
               "directly");
        auto const & this_executor =
            *static_cast<Executor const *>(ctx.this_executor_);

        auto const * typed_params = static_cast<
            typename Executor::out_channel_type::value_type const *>(params);

        // TODO: support variant
        if (type_info != typeid(typed_params)) [[unlikely]] {
          throw std::runtime_error(std::format(
              "execution_context::park(): incompatible awaiter parameters type "
              "({}) and output channel value type ({})",
              type_info.name(), typeid(typed_params).name()));
        }

        // TODO: handle infinite wait, handle exception
        this_executor.out_channel().push(*typed_params);
      };

      yield_ =
          [](execution_context const & ctx,
             std::coroutine_handle<>
                 handle) noexcept(concepts::
                                      noexcept_channel<
                                          typename Executor::in_channel_type>)
          -> void {
        assert(ctx.this_executor_ &&
               "execution_context::yield(): this function should not be called "
               "directly");
        auto const & this_executor =
            *static_cast<Executor const *>(ctx.this_executor_);

        // TODO: handle infinite wait, handle exception
        this_executor.in_channel().push(handle);
      };
    }

    /**
     * @note Task exception handling is done asynchronously with the standard
     * unhandled_exception method
     */
    handle.resume();
  }
};

}  // namespace argo
