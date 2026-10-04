#pragma once

#include <swiss/concepts.hpp>

#include "awaiter_context.hpp"

namespace argo::concepts {

/**
 * @brief Awaiter concept with a coroutine handle wrapped in a type with
 * restricted access to the promise type. Otherwise this is a basic model for a
 * standard awaiter type
 */
template <typename T>
concept awaiter =
    requires(swiss::co_await_result_t<T> awaiter, argo::awaiter_context<> ctx) {
      { awaiter.await_ready() } -> std::same_as<bool>;
      { awaiter.await_suspend(ctx) };
      { awaiter.await_resume() };
    };

}  // namespace argo::concepts
