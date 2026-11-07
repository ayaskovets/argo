#pragma once

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
 * @note Symmetric transfer and resume from await_suspend() is prohibited for
 * user-defined awaiters
 * @note If await_suspend throws, it must release the token and cancel any
 * registration before propagating the exception as it would violate execution
 * token exclusive ownership of the resume operation otherwise
 */
template <typename T>
concept awaiter =
    requires(swiss::co_await_result_t<T> awaiter, execution_token token) {
      { awaiter.await_ready() } -> std::same_as<bool>;
      { awaiter.await_resume() };
      requires requires() {
        { awaiter.await_suspend(std::move(token)) } -> std::same_as<void>;
      } != requires() {
        { awaiter.await_suspend() } -> std::same_as<void>;
      };
    };

}  // namespace argo::concepts
