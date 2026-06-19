#pragma once

#include <swiss/mixins.hpp>

#include "../channel.hpp"
#include "current_executor.hpp"

namespace wip::executors {

/**
 * @brief Executor analogous to std::this_thread
 */
template <concepts::channel InChannel, concepts::channel OutChannel>
class inline_executor final : public swiss::non_copyable,
                              public swiss::non_movable {
 public:
  constexpr inline_executor(InChannel & in_channel,
                            OutChannel & out_channel) noexcept
      : in_channel_(in_channel), out_channel_(out_channel) {}

 public:
  constexpr auto in_channel() noexcept -> InChannel & { return in_channel_; }
  constexpr auto out_channel() noexcept -> OutChannel & { return out_channel_; }

 public:
  constexpr void run_once() noexcept {
    typename InChannel::value_type task{};
    if (in_channel_.try_pop(task)) {
      current_executor::resume(*this, task);
    }
  }

  template <std::invocable<bool> Predicate>
  constexpr void run_while(Predicate const & predicate) noexcept(
      std::is_nothrow_invocable_v<Predicate>) {
    while (predicate()) {
      run_once();
    }
  }

  template <typename Clock>
  constexpr void run_until(
      typename Clock::time_point const time_point) noexcept {
    run_while([time_point] { return Clock::now() < time_point; });
  }

  template <typename Duration>
  constexpr void run_for(Duration duration) noexcept {
    run_until(std::chrono::steady_clock::now() + duration);
  }

 private:
  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  InChannel & in_channel_;
  OutChannel & out_channel_;
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
};

}  // namespace wip::executors
