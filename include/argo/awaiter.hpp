#pragma once

#include <swiss/concepts.hpp>

#include "suspended.hpp"

namespace argo::concepts {

/**
 * @brief Awaiter concept with a coroutine handle wrapped in a type with
 * restricted access to the promise type. Otherwise this is a basic model for a
 * standard awaiter type
 */
template <typename T>
concept awaiter =
    requires(swiss::co_await_result_t<T> awaiter, argo::suspended task) {
      { awaiter.await_ready() } -> std::same_as<bool>;
      {
        awaiter.await_suspend(task)
      } -> swiss::one_of<void, bool, std::coroutine_handle<>>;
      { awaiter.await_resume() };
    };

}  // namespace argo::concepts
