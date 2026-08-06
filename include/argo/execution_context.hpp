#pragma once

#include <cassert>
#include <coroutine>
#include <format>

#include "executor.hpp"

namespace argo {

// TODO: temporarily change this_executor to impl transfer to other ctx
// TODO: replace coroutine_handle

/**
 * @brief Context for a single thread of execution. All scheduling and context
 * switching should be done through the functions defined in this namespace
 * @note With multiple threads of execution each thread needs its own context to
 * function properly as it is difficult and unnecessary to implement this class
 * in a way that does distinguishing between threads automatically
 */
class execution_context {
 protected:
  /**
   * @brief TLS execution context
   */
  static thread_local inline execution_context * this_executor_ = nullptr;

 public:
  /**
   * @brief Somewhat analogous to std::this_thread for accessing the current
   * thread local execution context
   */
  static constexpr auto this_executor() noexcept -> execution_context & {
    assert(this_executor_ && "this_executor(): no resumed coroutine");
    return *this_executor_;
  }

 public:
  template <concepts::executor Executor>
  explicit constexpr execution_context(Executor const & executor) noexcept
      : executor_(&executor),
        yield_(&execution_context::yield<Executor>),
        park_(&execution_context::park<Executor>) {}

 public:
  /**
   * @brief Resume task by the passed executor instance. Sets up all internal
   * machinery needed for awaiters and task scheduling to work
   * @note Can be treated as a start of an execution step for a certain
   * coroutine
   * @warning This functions expects handle being valid and non-null
   */
  // NOLINTNEXTLINE(bugprone-exception-escape)
  constexpr void resume(std::coroutine_handle<> handle) noexcept {
    this_executor_ = this;
    handle.resume();
  }

  /**
   * @brief Yield execution to another task adding handle to the back of
   * the execution queue
   * @note Can be treated as a end of an step execution for a certain coroutine
   * @note Noexcept if input channel of the current executor is noexcept
   */
  constexpr void yield(std::coroutine_handle<> handle) {
    (this->*yield_)(handle);
    this_executor_ = nullptr;
  }

  /**
   * @brief Hand over the task parameters to a task scheduler. The specific
   * scheduler type is selected based on the type of the function argument
   * @note Can be treated as a end of an step execution for a certain coroutine
   * @note Noexcept if input channel of the current executor is noexcept
   */
  // TODO: split params into params and task view
  template <typename Params>
  constexpr void schedule(Params const & params) {
    (this->*park_)(&params, typeid(&params));
    this_executor_ = nullptr;
  }

 protected:
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  /**
   * @brief This pointer is set when a task is resumed by an executor to
   * access the executor instance
   */
  void const * executor_;

  /**
   * @brief Yield function is set when a task is resumed by an executor. This
   * called when a coroutine encounters a co_await with the
   * awaiters::yield_awaiter
   * @note The rationale for this function is a mechanisn to bypass scheduler
   * and add the task back to the execution queue of an executor
   */
  void (execution_context::*yield_)(std::coroutine_handle<> handle);

  /**
   * @brief Park function is set when a task is resumed by an executor. This
   * function gets called when a coroutine encounters a co_await that causes it
   * to suspend. This function is expected to do
   * @note Type info is passed for type safety to ensure that either scheduler
   * input channel or any other place that the awaiter params go to expect the
   * same type as the one passed as it is impossible to know what scheduler type
   * is going to be used with a certain awaiter
   */
  void (execution_context::*park_)(void const * params,
                                   std::type_info const & type_info);

  // NOLINTEND(misc-non-private-member-variables-in-classes)
 private:
  template <concepts::executor Executor>
  constexpr void yield(std::coroutine_handle<> handle) noexcept(
      concepts::noexcept_channel<typename Executor::in_channel_type>) {
    auto const & typed_executor = *static_cast<Executor const *>(executor_);

    // TODO: handle infinite wait
    typed_executor.in_channel().push(handle);
  }

  template <concepts::executor Executor>
  constexpr void park(void const * params, std::type_info const & type_info) {
    auto const & typed_params =
        *static_cast<typename Executor::out_channel_type::value_type const *>(
            params);

    // TODO: support multiple types
    if (type_info != typeid(&typed_params)) [[unlikely]] {
      throw std::runtime_error(
          std::format("execution_context::park(): incompatible awaiter "
                      "parameters type ({}) and output channel value type ({})",
                      type_info.name(), typeid(&typed_params).name()));
    }

    auto const & typed_executor = *static_cast<Executor const *>(executor_);

    // TODO: handle infinite wait
    typed_executor.out_channel().push(typed_params);
  }
};

}  // namespace argo
