#pragma once

#include <coroutine>

#include <swiss/concepts.hpp>

namespace argo {
class execution_token;
}  // namespace argo

namespace argo::concepts {

/**
 * @brief Awaiter concept with a coroutine handle wrapped in a type with
 * restricted access to the promise type. Otherwise this is a basic model for a
 * standard awaiter type
 * @note Types that do not care about the suspended task are supported by
 * await_suspend with no arguments
 */
template <typename T>
concept awaiter =
    requires(swiss::co_await_result_t<T> awaiter, execution_token token) {
      { awaiter.await_ready() } -> std::same_as<bool>;
      { awaiter.await_resume() };

      requires requires() {
        {
          awaiter.await_suspend(std::move(token))
        } -> swiss::one_of<void, bool, std::coroutine_handle<>>;
      } != requires() {
        {
          awaiter.await_suspend()
        } -> swiss::one_of<void, bool, std::coroutine_handle<>>;
      };
    };

}  // namespace argo::concepts
