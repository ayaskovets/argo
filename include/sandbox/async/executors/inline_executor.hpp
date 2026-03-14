#pragma once

#include <wip/async/channel.hpp>
#include <wip/async/executors/current_executor.hpp>
#include <wip/utils/mixins.hpp>

#include <coroutine>

namespace wip::async::executors {

template <typename InChannel, typename OutChannel>
class inline_executor final : public utils::non_copyable,
                              public utils::non_movable {
 private:
  static_assert(async::concepts::channel<InChannel, std::coroutine_handle<>>);

 public:
  constexpr inline_executor(InChannel& in_channel,
                            OutChannel& out_channel) noexcept
      : in_channel_(in_channel), out_channel_(out_channel) {}

 public:
  InChannel& in_channel() noexcept { return in_channel_; }
  OutChannel& out_channel() noexcept { return out_channel_; }

 public:
  constexpr void run_once() noexcept {
    // TODO: reimplement
    std::size_t limit = 5;
    for (std::size_t i = 0; i < limit; ++i) {
      std::coroutine_handle<> handle;
      if (!in_channel_.try_pop(handle)) {
        break;
      }

      current_executor::resume(*this, handle);
    }
  }

  template <typename Predicate>
  constexpr void run_while(Predicate&& predicate) noexcept {
    while (predicate()) {
      run_once();
    }
  }

  template <typename Clock>
  constexpr void run_until(
      const typename Clock::time_point& time_point) noexcept {
    run_while([&time_point] { return Clock::now() < time_point; });
  }

  template <typename Duration>
  constexpr void run_for(Duration duration) noexcept {
    run_until(std::chrono::steady_clock::now() + duration);
  }

 private:
  InChannel& in_channel_;
  OutChannel& out_channel_;
};

}  // namespace wip::async::executors
