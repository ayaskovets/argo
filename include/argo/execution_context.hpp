#pragma once

#include <cassert>
#include <format>
#include <variant>

#include <swiss/mixins.hpp>

#include "execution_token.hpp"
#include "executor.hpp"

namespace argo {

/**
 * @brief Execution context. Represents a trivially copyable set of pointers and
 * functions with type-erased operations on in/out channels of a certain
 * executor
 */
class execution_context final : public swiss::non_copyable,
                                public swiss::non_movable {
 private:
  /**
   * @brief TLS execution context. Used to access and switch the current
   * execution context for resumed and suspended coroutines
   */
  static thread_local inline execution_context const * this_executor_ = nullptr;

 public:
  /**
   * @brief Public accessor for the TLS execution context
   */
  static constexpr auto this_executor() noexcept -> execution_context const & {
    assert(this_executor_ &&
           "execution_context::this_executor(): no resumed coroutine");
    return *this_executor_;
  }

  /**
   * @brief Switch TLS execution context to the co_await'ed object without
   * suspending the current task
   */
  constexpr auto operator co_await() const noexcept -> decltype(auto) {
    class switch_awaiter final {
     public:
      explicit constexpr switch_awaiter(execution_context const & ctx) noexcept
          : ctx_(&ctx) {}

     public:
      constexpr auto await_ready() const noexcept -> bool { return true; }

      constexpr void await_suspend() const noexcept { std::unreachable(); }

      constexpr void await_resume() const noexcept {
        execution_context::this_executor_ = ctx_;
      }

     private:
      execution_context const * ctx_;
    };

    return switch_awaiter(*this);
  }

 public:
  template <concepts::executor Executor>
  explicit constexpr execution_context(Executor const & executor) noexcept
      : executor_(&executor),
        post_(&execution_context::post<Executor>),
        park_(&execution_context::park<Executor>) {}

 public:
  /**
   * @brief Synchronously resume task by the current executor
   */
  constexpr void resume(execution_token token) const noexcept {
    this_executor_ = this;
    token.resume();
  }

  /**
   * @brief Add the task to the input channel of the current executor
   * @note Noexcept if input channel of the current executor is noexcept
   */
  constexpr void execute(execution_token token) const {
    (this->*post_)(std::move(token));
  }

  /**
   * @brief Add the task to output channel of the current executor
   * @note Noexcept if input channel of the current executor is noexcept
   */
  template <typename Params>
  constexpr void schedule(Params const & params) const {
    (this->*park_)(&params, typeid(&params));
  }

 private:
  /**
   * @brief Push the task to the input channel of the executor
   */
  template <concepts::executor Executor>
  constexpr void post(execution_token token) const
      noexcept(concepts::noexcept_channel<typename Executor::in_channel_type>) {
    auto const & typed_executor = *static_cast<Executor const *>(executor_);

    // TODO: handle infinite wait
    typed_executor.in_channel().push(std::move(token));
  }

  /**
   * @brief Check whether the typeid matches the expected typeid in the channel
   * and push to the output channel of the executor
   */
  template <concepts::executor Executor>
    requires(concepts::monochannel<typename Executor::out_channel_type>)
  constexpr void park(void const * params,
                      std::type_info const & type_info) const {
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
   * @brief Do linear search through the list of possible typeid values in the
   * multichannel and push to the output channel of the executor
   */
  template <concepts::executor Executor>
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
  void const * executor_;
  void (execution_context::*post_)(execution_token) const;
  void (execution_context::*park_)(void const * params,
                                   std::type_info const & type_info) const;
};

}  // namespace argo
