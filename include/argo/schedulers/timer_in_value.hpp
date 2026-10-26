#pragma once

#include <chrono>

#include "../execution_token.hpp"

namespace argo::schedulers {

/**
 * @brief Input channel value type of the timer scheduler
 */
template <typename Clock = std::chrono::steady_clock>
class timer_in_value final {
 public:
  constexpr timer_in_value(argo::execution_token token,
                           Clock::time_point time_point) noexcept
      : execution_token_(std::move(token)), time_point_(time_point) {}

 public:
  constexpr auto get_execution_token() noexcept -> argo::execution_token && {
    return std::move(execution_token_);
  }

  constexpr auto get_time_point() const noexcept -> Clock::time_point {
    return time_point_;
  }

 private:
  argo::execution_token execution_token_;
  Clock::time_point time_point_;
};

}  // namespace argo::schedulers
