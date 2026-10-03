#pragma once

#include <cassert>
#include <format>
#include <variant>

#include "executor.hpp"
#include "task_ref.hpp"

namespace argo::awaiters {
class switch_to_awaiter;
}  // namespace argo::awaiters

namespace argo {

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
   * @brief TLS execution context. Set by resume(). Each co_await that suspends
   * a coroutine will use this execution context for scheduling
   */
  static thread_local inline execution_context * this_executor_ = nullptr;

 public:
  /**
   * @brief Somewhat analogous to std::this_thread for accessing the current
   * thread local execution context
   */
  static constexpr auto this_executor() noexcept -> execution_context & {
    assert(this_executor_ &&
           "execution_context::this_executor(): no resumed coroutine");
    return *this_executor_;
  }

 public:
  template <concepts::executor Executor>
  explicit constexpr execution_context(Executor const & executor) noexcept
      : executor_(&executor),
        post_(&execution_context::post<Executor>),
        park_(&execution_context::park<Executor>) {}

 public:
  /**
   * @brief Resume task by the passed executor instance. Sets up all internal
   * machinery needed for awaiters and task scheduling to work
   * @warning This function expects task being valid and non-null
   */
  constexpr void resume(task_ref task) {
    this_executor_ = this;
    task.resume();
  }

  /**
   * @brief Add the task handle to the back of the execution queue
   * @note Noexcept if input channel of the current executor is noexcept
   */
  constexpr void execute(task_ref task) { (this->*post_)(task); }

  /**
   * @brief Add the task to the back of the scheduling queue
   * @note The specific scheduler type is selected based on the type of the
   * function argument
   * @note Noexcept if input channel of the current executor is noexcept
   */
  template <typename Params>
  constexpr void schedule(Params const & params) {
    (this->*park_)(&params, typeid(&params));
  }

 protected:
  /**
   * @brief This pointer is set when a task is resumed by an executor to
   * access the executor instance
   */
  void const * executor_;

  /**
   * @brief Bypass scheduler and add the task back to the execution queue of an
   * executor
   */
  void (execution_context::*post_)(task_ref);

  /**
   * @brief Called when a coroutine encounters a co_await that causes it to
   * suspend and get transfered to a corresponding scheduler
   * @note Type info is passed for type safety to ensure that either scheduler
   * input channel or any other place that the params go to expect the
   * same type as the one passed as it is impossible to know what scheduler type
   * is going to be used with a certain params
   */
  void (execution_context::*park_)(void const * params,
                                   std::type_info const & type_info);

 private:
  template <concepts::executor Executor>
  constexpr void post(task_ref task) noexcept(
      concepts::noexcept_channel<typename Executor::in_channel_type>) {
    auto const & typed_executor = *static_cast<Executor const *>(executor_);

    // TODO: handle infinite wait
    typed_executor.in_channel().push(task);
  }

  /**
   * @brief Do type safety check and push the task to the output channel of
   * current executor
   */
  template <concepts::executor Executor>
    requires(concepts::monochannel<typename Executor::out_channel_type>)
  constexpr void park(void const * params, std::type_info const & type_info) {
    if (type_info != typeid(Executor::out_channel_type::value_type const *))
        [[unlikely]] {
      throw std::runtime_error(std::format(
          "execution_context::park(): incompatible params ({}) and output "
          "channel value type ({})",
          type_info.name(),
          typeid(Executor::out_channel_type::value_type const *).name()));
    }

    auto const & typed_executor = *static_cast<Executor const *>(executor_);
    auto const & typed_params =
        *static_cast<Executor::out_channel_type::value_type const *>(params);

    // TODO: handle infinite wait
    typed_executor.out_channel().push(typed_params);
  }

  /**
   * @brief Type safety check and linear search through all possible output
   * types of the executor. If there is a type in the output channel which is
   * the same as the params type then params are pushed to the output channel of
   * current executor
   */
  template <typename Executor>
    requires(concepts::multichannel<typename Executor::out_channel_type>)
  constexpr void park(void const * params, std::type_info const & type_info) {
    bool const match = [&]<typename... Args>(std::variant<Args...>) -> bool {
      return ([&]() -> bool {
        if (type_info != typeid(Args const *)) {
          return false;
        }

        auto const & typed_executor = *static_cast<Executor const *>(executor_);
        auto const & typed_params   = *static_cast<Args const *>(params);

        // TODO: handle infinite wait
        typed_executor.out_channel().push(typed_params);

        return true;
      } || ...);
    }(typename Executor::out_channel_type::value_types{});

    if (!match) [[unlikely]] {
      throw std::runtime_error(
          "execution_context::park(): incompatible params ({}) and output "
          "channel value types");
    }
  }

 private:
  /**
   * @brief To implement changing executors for a task through a single
   * incapsulated awaiter type
   */
  friend class awaiters::switch_to_awaiter;
};

}  // namespace argo

/**
 * @brief Utility namespace for accessing current executor same way as
 * std::this_thread
 */
namespace argo::this_executor {

constexpr void execute(task_ref task) {
  execution_context::this_executor().execute(task);
}

template <typename Params>
constexpr void schedule(Params && params) {
  execution_context::this_executor().schedule(std::forward<Params>(params));
}

}  // namespace argo::this_executor
