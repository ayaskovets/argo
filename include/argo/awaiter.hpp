#pragma once

#include <swiss/concepts.hpp>

#include "task_ref.hpp"

namespace argo::concepts {

/**
 * @brief Task executor concept with input/output channels
 */
template <typename T>
concept awaiter = requires(swiss::co_await_result_t<T> awaiter) {
  { awaiter.await_ready() } -> std::same_as<bool>;
  { awaiter.await_suspend(argo::task_ref()) };
  { awaiter.await_resume() };
};

}  // namespace argo::concepts
